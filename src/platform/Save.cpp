#include "platform/Save.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

// Plain-text "key value" save file. Trivial to inspect, forward compatible
// (unknown keys ignored, missing keys keep defaults). The persistent meta goes
// here; a run in progress has its own file (saveRun / loadRun, below).
namespace sb {

namespace {
constexpr int kRunVersion = 1;   // run.txt layout; a different one is dropped, not misread
constexpr int kSaveVersion = 18;  // v18: Rares / 2nd Legendary / gods on the web (grandfathered for owned classes); v17: epic / legendary gates per class + bridges appended (grandfathered for owned classes); v16: the web became class routes (layout only), Calling = 4th first-ability card, route perks + ability unlocks appended (86 nodes); v15: root Squad -> Calling (refund), class nodes appended (58 nodes); v14: snd.* sound mix; v13: appended Oath..Last stand (51 nodes); append-only since v9
}  // namespace

bool hasSavedGame(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

bool saveGame(const std::string& path, const GameData& d) {
    const std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;

    const MetaState& m = d.meta;
    f << "version " << kSaveVersion << '\n';
    f << "meta.cores " << m.cores << '\n';
    f << "meta.prisms " << m.prisms << '\n';
    for (int i = 0; i < MetaUnlockCount; ++i)
        f << "meta.unlock " << i << ' ' << m.unlock[i] << '\n';
    f << "sound " << (m.soundOn ? 1 : 0) << '\n';
    f << "snd.master " << m.sound.master << '\n';
    f << "snd.music " << m.sound.music << '\n';
    f << "snd.sfx " << m.sound.sfx << '\n';
    f << "snd.on " << (m.sound.musicOn ? 1 : 0) << ' ' << (m.sound.sfxOn ? 1 : 0) << '\n';
    for (int i = 0; i < SoundCatCount; ++i)   // category index, volume, style
        f << "snd.cat " << i << ' ' << m.sound.vol[i] << ' ' << m.sound.style[i] << '\n';
    f << "fullscreen " << (m.fullscreen ? 1 : 0) << '\n';
    f << "hard " << (m.hardMode ? 1 : 0) << '\n';
    f << "simpleText " << (m.simpleText ? 1 : 0) << '\n';
    f << "asc " << m.ascMax << ' ' << m.ascSel << '\n';
    f << "hints " << m.hints << '\n';
    f << "stat.enemiesKilled " << m.stats.enemiesKilled << '\n';
    f << "stat.coresEarned " << m.stats.coresEarned << '\n';
    f << "stat.bestWave " << m.stats.bestWave << '\n';
    f << "stat.bestCombo " << m.stats.bestCombo << '\n';
    f << "stat.bestScore " << m.stats.bestScore << '\n';
    f << "stat.runs " << m.stats.runs << '\n';
    f << "stat.wins " << m.stats.wins << '\n';
    f << "stat.maxSpeed " << m.stats.maxSpeed << '\n';
    f << "stat.timePlayed " << m.stats.timePlayed << '\n';
    return f.good();
}

bool loadGame(const std::string& path, GameData& d) {
    std::ifstream f(path);
    if (!f) return false;

    MetaState& m = d.meta;
    bool sawAnything = false;
    int fileVersion = 0;
    bool sawAsc = false;   // an "asc" line (2026-10-12+)
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;
        sawAnything = true;

        if (key == "version") ls >> fileVersion;
        else if (key == "meta.cores") ls >> m.cores;
        else if (key == "hints") ls >> m.hints;
        else if (key == "meta.prisms") ls >> m.prisms;
        else if (key == "meta.unlock") {
            // v9 inserted nodes and shifted indices; older unlock allocations
            // would land on the wrong nodes, so drop them and let the tree be
            // re-bought. Cores / prisms / stats are kept.
            int i = -1, lvl = 0;
            if (fileVersion >= 9 && ls >> i >> lvl && i >= 0 && i < MetaUnlockCount)
                m.unlock[i] = lvl;
        }
        // v4 and earlier stored these two unlocks by name.
        else if (key == "meta.startBalls") ls >> m.unlock[MetaCalling];
        else if (key == "meta.coreHp") ls >> m.unlock[MetaCoreHp];
        else if (key == "sound") { int v = 1; ls >> v; m.soundOn = v != 0; }
        // v14: the sound mix. Older saves keep the defaults.
        else if (key == "snd.master") ls >> m.sound.master;
        else if (key == "snd.music") ls >> m.sound.music;
        else if (key == "snd.sfx") ls >> m.sound.sfx;
        else if (key == "snd.on") {   // music / effects switches; missing = both on
            int mu = 1, fx = 1;
            ls >> mu >> fx;
            m.sound.musicOn = mu != 0;
            m.sound.sfxOn = fx != 0;
        }
        else if (key == "snd.cat") {
            int i = -1, vol = 100, style = 0;
            if (ls >> i >> vol >> style && i >= 0 && i < SoundCatCount) {
                m.sound.vol[static_cast<std::size_t>(i)] = vol;
                m.sound.style[static_cast<std::size_t>(i)] = style;
            }
        }
        else if (key == "fullscreen") { int v = 0; ls >> v; m.fullscreen = v != 0; }
        else if (key == "hard") { int v = 0; ls >> v; m.hardMode = v != 0; }
        else if (key == "simpleText") { int v = 0; ls >> v; m.simpleText = v != 0; }
        else if (key == "asc") {
            sawAsc = true;
            ls >> m.ascMax >> m.ascSel;
            m.ascMax = std::clamp(m.ascMax, 0, cfg::asc::maxLevel);
            m.ascSel = std::clamp(m.ascSel, 0, m.ascMax);
        }
        else if (key == "stat.enemiesKilled") ls >> m.stats.enemiesKilled;
        else if (key == "stat.coresEarned") ls >> m.stats.coresEarned;
        else if (key == "stat.bestWave") ls >> m.stats.bestWave;
        else if (key == "stat.bestCombo") ls >> m.stats.bestCombo;
        else if (key == "stat.bestScore") ls >> m.stats.bestScore;
        else if (key == "stat.runs") ls >> m.stats.runs;
        else if (key == "stat.wins") ls >> m.stats.wins;
        else if (key == "stat.maxSpeed") ls >> m.stats.maxSpeed;
        else if (key == "stat.timePlayed") ls >> m.stats.timePlayed;
        // "version", old run.* / ball / stat.lifetimeScrap lines: ignored.
    }
    // Ascension came in 2026-10-12: a save that already won a run opens level 1.
    if (!sawAsc && m.stats.wins > 0) {
        m.ascMax = 1;
        m.ascSel = 1;
    }

    // v15: the root "Squad" (+1 starting ball, 2 levels) became "Calling" (1
    // level). Pay back the cores spent on levels past the first.
    if (fileVersion < 15)
        for (int lvl = 1; lvl < m.unlock[MetaCalling]; ++lvl) m.cores += metaUnlockCost(MetaCalling, lvl);
    // v17: a class's epics and legendaries got their own nodes; a class you
    // already had keeps rolling them.
    if (fileVersion < 17)
        for (int t = 1; t <= 10; ++t) {
            const ItemTag tag = static_cast<ItemTag>(t);
            if (const int n = classUnlockNode(tag); n >= 0 && m.unlock[n] > 0) {
                m.unlock[epicGateNode(tag)] = 1;
                m.unlock[legendGateNode(tag)] = 1;
            }
        }
    // v18: Rares, the second Legendary and the god got their own nodes too.
    if (fileVersion < 18)
        for (int t = 1; t <= 10; ++t)
            if (const int n = classUnlockNode(static_cast<ItemTag>(t)); n >= 0 && m.unlock[n] > 0) {
                m.unlock[MetaRareStriker + t - 1] = 1;
                if (!metaNodeRetired(MetaLegend2Striker + t - 1)) m.unlock[MetaLegend2Striker + t - 1] = 1;
                m.unlock[MetaGodStriker + t - 1] = 1;
            }
    for (int i = 0; i < MetaUnlockCount; ++i) {
        if (m.unlock[i] < 0) m.unlock[i] = 0;
        if (m.unlock[i] > metaUnlockDef(i).maxLevel) m.unlock[i] = metaUnlockDef(i).maxLevel;
    }
    auto pct = [](int& v) { v = std::clamp(v, 0, 100); };
    pct(m.sound.master);
    pct(m.sound.music);
    pct(m.sound.sfx);
    for (int i = 0; i < SoundCatCount; ++i) {
        pct(m.sound.vol[static_cast<std::size_t>(i)]);
        int& st = m.sound.style[static_cast<std::size_t>(i)];
        st = std::clamp(st, 0, SoundStyleCount - 1);
    }
    return sawAnything;
}

// ---------------------------------------------------------------- the run

bool saveRun(const std::string& path, const RunState& r) {
    const std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    const std::string tmp = path + ".tmp";   // written whole, then swapped in
    {
        std::ofstream f(tmp, std::ios::trunc);
        if (!f) return false;
        f << "run.version " << kRunVersion << '\n';
        f << "run.hard " << (r.hard ? 1 : 0) << '\n';
        f << "run.asc " << r.ascension << '\n';
        f << "run.tutorial " << (r.tutorial ? 1 : 0) << '\n';
        f << "run.plating " << r.platingHp << '\n';
        f << "run.ascend " << r.ascensions << ' ' << r.poolOut << ' ' << r.poolIn << '\n';
        f << "run.bosses";
        for (int b : r.bosses) f << ' ' << b;
        f << '\n';
        f << "run.wave " << r.wave << '\n';
        f << "run.core " << r.coreHp << ' ' << r.coreMaxHp << '\n';
        f << "run.score " << r.score << '\n';
        f << "run.rerolls " << r.rerollsLeft << '\n';
        f << "run.bounty " << r.bountyCores << '\n';
        f << "run.gold " << r.gold << ' ' << r.goldFrac << '\n';
        f << "run.pos " << r.mapNode << ' ' << r.mapRow << '\n';
        f << "run.altar " << r.cleanStreak << ' ' << r.altarState << '\n';
        f << "run.saves " << r.lastStandLeft << ' ' << (r.phoenixUsedAct ? 1 : 0) << ' '
          << (r.repairedThisAct ? 1 : 0) << '\n';
        const RunMods& m = r.mods;
        f << "run.mods " << m.spring << ' ' << m.slowField << ' ' << m.strongArm << ' ' << m.contagion << ' '
          << m.primed << ' ' << m.catalyst << ' ' << m.chainReaction << ' ' << m.luckyClover << ' '
          << m.glassCannon << ' ' << m.magneticCore << ' ' << m.prismCore << ' ' << m.phoenix << ' '
          << m.timeDilation << ' ' << m.overcharge << '\n';
        f << "run.creeds " << r.creeds.size();
        for (int c : r.creeds) f << ' ' << c;
        f << '\n';
        f << "run.pacts " << r.pacts.size();
        for (int c : r.pacts) f << ' ' << c;
        f << '\n' << "run.pactsPure " << r.pactsPure;
        f << '\n';
        auto writeBall = [&](const BallLoadout& L) {
            for (int i = 0; i < kBaseItemSlots; ++i) f << ' ' << L.gear[i] << ' ' << L.gearLvl[i];
            f << ' ' << L.type << ' ' << L.typeLvl;
            for (int i = 0; i < kMaxElements - 1; ++i) f << ' ' << L.extraType[i];
            for (int i = 0; i < kMaxAbilitySlots; ++i) f << ' ' << L.ability[i] << ' ' << L.abilityLvl[i];
            for (int i = 0; i < kModifierCount; ++i) f << ' ' << L.mods[i];
            f << ' ' << (L.freeMissile ? 1 : 0) << ' ' << (L.freePoison ? 1 : 0);
            // (2026-10-06) the Shrine: open item slots, the 5th / 6th, used or not -
            // at the end, so a run saved before still loads (its 4 slots stay first)
            f << ' ' << L.itemSlots << ' ' << (L.shrineUsed ? 1 : 0);
            for (int i = kBaseItemSlots; i < kBallSlots; ++i) f << ' ' << L.gear[i] << ' ' << L.gearLvl[i];
            f << ' ' << L.ascendedFrom << ' ' << L.keptSize;
            for (float x : L.dragonXp) f << ' ' << x;   // (2026-10-07) Tiamat's dragons' growth
            f << '\n';
        };
        for (const BallLoadout& L : r.balls) {
            f << "run.ball";
            writeBall(L);
        }
        for (const RunState::Benched& bn : r.bench) {   // "Recruiter": away on a contract
            f << "run.bench " << bn.fights << ' ' << bn.slot;
            writeBall(bn.ball);
        }
        f << "run.map " << r.map.act << ' ' << r.map.rows << '\n';   // rows 0 = the act's usual length
        for (const MapNode& n : r.map.nodes) {   // type row lane visited, then its links
            f << "run.node " << static_cast<int>(n.type) << ' ' << n.row << ' ' << n.lane << ' '
              << (n.visited ? 1 : 0) << ' ' << n.next.size();
            for (int x : n.next) f << ' ' << x;
            f << '\n';
        }
        if (!f.good()) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    return !ec;
}

bool loadRun(const std::string& path, RunState& out) {
    std::ifstream f(path);
    if (!f) return false;
    RunState r;
    int version = 0;
    bool sawMap = false;
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;
        if (key == "run.version") ls >> version;
        else if (key == "run.hard") { int v = 0; ls >> v; r.hard = v != 0; }
        else if (key == "run.asc") ls >> r.ascension;
        else if (key == "run.tutorial") { int v = 0; ls >> v; r.tutorial = v != 0; }
        else if (key == "run.plating") ls >> r.platingHp;
        else if (key == "run.ascend") ls >> r.ascensions >> r.poolOut >> r.poolIn;
        else if (key == "run.bosses") {   // older saves: the fixed order
            for (int& b : r.bosses) ls >> b;
            // No boss twice in a run: anything but an order of the four goes back to the fixed one.
            bool seen[4] = {};
            bool ok = true;
            for (int b : r.bosses) {
                if (b < 0 || b > 3 || seen[b]) { ok = false; break; }
                seen[b] = true;
            }
            if (!ok) r.bosses = {0, 1, 2, 3};
        }
        else if (key == "run.wave") ls >> r.wave;
        else if (key == "run.core") ls >> r.coreHp >> r.coreMaxHp;
        else if (key == "run.score") ls >> r.score;
        else if (key == "run.rerolls") ls >> r.rerollsLeft;
        else if (key == "run.bounty") ls >> r.bountyCores;
        else if (key == "run.gold") ls >> r.gold >> r.goldFrac;
        else if (key == "run.pos") ls >> r.mapNode >> r.mapRow;
        else if (key == "run.altar") ls >> r.cleanStreak >> r.altarState;
        else if (key == "run.saves") {
            int ph = 0, rep = 0;
            ls >> r.lastStandLeft >> ph >> rep;
            r.phoenixUsedAct = ph != 0;
            r.repairedThisAct = rep != 0;
        } else if (key == "run.mods") {
            RunMods& m = r.mods;
            bool* fields[] = {&m.spring, &m.slowField, &m.strongArm, &m.contagion, &m.primed,
                              &m.catalyst, &m.chainReaction, &m.luckyClover, &m.glassCannon, &m.magneticCore,
                              &m.prismCore, &m.phoenix, &m.timeDilation, &m.overcharge};
            for (bool* b : fields) { int v = 0; ls >> v; *b = v != 0; }
        } else if (key == "run.pactsPure") {
            ls >> r.pactsPure;
        } else if (key == "run.creeds" || key == "run.pacts") {
            std::size_t n = 0;
            ls >> n;
            std::vector<int>& v = key == "run.creeds" ? r.creeds : r.pacts;
            for (std::size_t i = 0; i < n && i < 64; ++i) { int x = 0; if (ls >> x) v.push_back(x); }
        } else if (key == "run.ball" || key == "run.bench") {
            RunState::Benched bn;
            if (key == "run.bench") ls >> bn.fights >> bn.slot;
            BallLoadout& L = bn.ball;
            for (int i = 0; i < kBaseItemSlots; ++i) ls >> L.gear[i] >> L.gearLvl[i];
            ls >> L.type >> L.typeLvl;
            for (int i = 0; i < kMaxElements - 1; ++i) ls >> L.extraType[i];
            for (int i = 0; i < kMaxAbilitySlots; ++i) ls >> L.ability[i] >> L.abilityLvl[i];
            for (int i = 0; i < kModifierCount; ++i) ls >> L.mods[i];
            if (!ls.fail()) {
                if (int fm = 0; ls >> fm) L.freeMissile = fm != 0;   // (older saves: not there)
                if (int fp = 0; ls >> fp) L.freePoison = fp != 0;
                if (int slots = 0, used = 0; ls >> slots >> used) {
                    L.itemSlots = std::clamp(slots, kBaseItemSlots, kBallSlots);
                    L.shrineUsed = used != 0;
                    for (int i = kBaseItemSlots; i < kBallSlots; ++i) {
                        int g = -1, gl = 0;
                        if (!(ls >> g >> gl)) break;
                        L.gear[i] = g;
                        L.gearLvl[i] = gl;
                    }
                    if (int af = 0; ls >> af) L.ascendedFrom = af;
                    if (float ks = 1.f; ls >> ks) L.keptSize = std::clamp(ks, 1.f, 4.f);
                    for (float& x : L.dragonXp)
                        if (float v = 0.f; ls >> v) x = std::max(0.f, v);
                }
                if (key == "run.bench") r.bench.push_back(bn);
                else r.balls.push_back(L);
            }
        } else if (key == "run.map") {
            ls >> r.map.act;
            if (!(ls >> r.map.rows)) r.map.rows = 0;   // older saves: the usual length
            sawMap = true;
        } else if (key == "run.node") {
            MapNode n;
            int type = 0, visited = 0;
            std::size_t links = 0;
            ls >> type >> n.row >> n.lane >> visited >> links;
            n.type = static_cast<MapNodeType>(std::clamp(type, 0, kMapNodeTypeCount - 1));
            n.visited = visited != 0;
            for (std::size_t i = 0; i < links && i < 16; ++i) { int x = 0; if (ls >> x) n.next.push_back(x); }
            r.map.nodes.push_back(n);
        }
    }
    // Anything off - another layout, no balls, a broken map - and it's not resumed.
    const int nodes = static_cast<int>(r.map.nodes.size());
    if (version != kRunVersion || !sawMap || r.balls.empty() || nodes == 0 || r.mapNode >= nodes) return false;
    for (const MapNode& n : r.map.nodes)
        for (int x : n.next)
            if (x < 0 || x >= nodes) return false;
    r.active = true;
    out = r;
    return true;
}

void clearRun(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

}  // namespace sb
