#include "platform/Save.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

// Plain-text "key value" save file. Trivial to inspect, forward compatible
// (unknown keys ignored, missing keys keep defaults). Two blocks: `meta.*` and
// `stat.*` always persist; `run.*` + `field` only when a run is in progress.
namespace sb {

namespace {
constexpr int kSaveVersion = 3;
constexpr std::size_t kMaxField = 16;
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
    f << "meta.startBalls " << m.unlock[MetaStartBalls] << '\n';
    f << "meta.coreHp " << m.unlock[MetaCoreHp] << '\n';
    f << "sound " << (m.soundOn ? 1 : 0) << '\n';
    f << "fullscreen " << (m.fullscreen ? 1 : 0) << '\n';
    f << "stat.enemiesKilled " << m.stats.enemiesKilled << '\n';
    f << "stat.lifetimeScrap " << m.stats.lifetimeScrap << '\n';
    f << "stat.bestWave " << m.stats.bestWave << '\n';
    f << "stat.bestCombo " << m.stats.bestCombo << '\n';
    f << "stat.runs " << m.stats.runs << '\n';
    f << "stat.maxSpeed " << m.stats.maxSpeed << '\n';
    f << "stat.timePlayed " << m.stats.timePlayed << '\n';

    const RunState& r = d.run;
    f << "run.active " << (r.active ? 1 : 0) << '\n';
    if (r.active) {
        f << "run.scrap " << r.scrap << '\n';
        f << "run.wave " << r.wave << '\n';
        f << "run.ballCount " << r.ballCount << '\n';
        f << "run.damageMult " << r.damageMult << '\n';
        f << "run.coreHp " << r.coreHp << '\n';
        f << "run.coreMaxHp " << r.coreMaxHp << '\n';
        for (std::size_t i = 0; i < r.field.size() && i < kMaxField; ++i) {
            const FieldSnapshot& s = r.field[i];
            f << "field " << i << ' ' << s.x << ' ' << s.y << ' ' << s.strength << ' ' << s.kind
              << '\n';
        }
    }
    return f.good();
}

bool loadGame(const std::string& path, GameData& d) {
    std::ifstream f(path);
    if (!f) return false;

    MetaState& m = d.meta;
    RunState& r = d.run;
    bool sawAnything = false;
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;
        sawAnything = true;

        if (key == "meta.cores") ls >> m.cores;
        else if (key == "meta.startBalls") ls >> m.unlock[MetaStartBalls];
        else if (key == "meta.coreHp") ls >> m.unlock[MetaCoreHp];
        else if (key == "sound") { int v = 1; ls >> v; m.soundOn = v != 0; }
        else if (key == "fullscreen") { int v = 0; ls >> v; m.fullscreen = v != 0; }
        else if (key == "stat.enemiesKilled") ls >> m.stats.enemiesKilled;
        else if (key == "stat.lifetimeScrap") ls >> m.stats.lifetimeScrap;
        else if (key == "stat.bestWave") ls >> m.stats.bestWave;
        else if (key == "stat.bestCombo") ls >> m.stats.bestCombo;
        else if (key == "stat.runs") ls >> m.stats.runs;
        else if (key == "stat.maxSpeed") ls >> m.stats.maxSpeed;
        else if (key == "stat.timePlayed") ls >> m.stats.timePlayed;
        else if (key == "run.active") { int v = 0; ls >> v; r.active = v != 0; }
        else if (key == "run.scrap") ls >> r.scrap;
        else if (key == "run.wave") ls >> r.wave;
        else if (key == "run.ballCount") ls >> r.ballCount;
        else if (key == "run.damageMult") ls >> r.damageMult;
        else if (key == "run.coreHp") ls >> r.coreHp;
        else if (key == "run.coreMaxHp") ls >> r.coreMaxHp;
        else if (key == "field") {
            std::size_t idx = 0;
            if (ls >> idx && idx < kMaxField) {
                if (r.field.size() <= idx) r.field.resize(idx + 1);
                FieldSnapshot& s = r.field[idx];
                ls >> s.x >> s.y >> s.strength >> s.kind;
            }
        }
        // "version" and anything unrecognised: ignored on purpose.
    }

    for (int i = 0; i < MetaUnlockCount; ++i)
        if (m.unlock[i] < 0) m.unlock[i] = 0;
    if (r.ballCount < 1) r.ballCount = 1;
    if (r.damageMult <= 0.f) r.damageMult = 1.f;

    return sawAnything;
}

}  // namespace sb
