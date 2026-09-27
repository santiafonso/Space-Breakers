#include "platform/Save.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

// Plain-text "key value" save file. Trivial to inspect, forward compatible
// (unknown keys ignored, missing keys keep defaults). Only the persistent meta
// is stored - a run is a short sprint and is never resumed.
namespace sb {

namespace {
constexpr int kSaveVersion = 15;  // v15: root Squad -> Calling (refund), class nodes appended (58 nodes); v14: snd.* sound mix; v13: appended Oath..Last stand (51 nodes); append-only since v9
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
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;
        sawAnything = true;

        if (key == "version") ls >> fileVersion;
        else if (key == "meta.cores") ls >> m.cores;
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

    // v15: the root "Squad" (+1 starting ball, 2 levels) became "Calling" (1
    // level). Pay back the cores spent on levels past the first.
    if (fileVersion < 15)
        for (int lvl = 1; lvl < m.unlock[MetaCalling]; ++lvl) m.cores += metaUnlockCost(MetaCalling, lvl);
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

}  // namespace sb
