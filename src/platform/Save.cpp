#include "platform/Save.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

// Plain-text "key value" save file. Trivial to inspect, forward compatible
// (unknown keys ignored, missing keys keep defaults). Only the persistent meta
// is stored - a run is a short sprint and is never resumed.
namespace sb {

namespace {
constexpr int kSaveVersion = 5;
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
    for (int i = 0; i < MetaUnlockCount; ++i)
        f << "meta.unlock " << i << ' ' << m.unlock[i] << '\n';
    f << "sound " << (m.soundOn ? 1 : 0) << '\n';
    f << "fullscreen " << (m.fullscreen ? 1 : 0) << '\n';
    f << "stat.enemiesKilled " << m.stats.enemiesKilled << '\n';
    f << "stat.coresEarned " << m.stats.coresEarned << '\n';
    f << "stat.bestWave " << m.stats.bestWave << '\n';
    f << "stat.bestCombo " << m.stats.bestCombo << '\n';
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
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;
        sawAnything = true;

        if (key == "meta.cores") ls >> m.cores;
        else if (key == "meta.unlock") {
            int i = -1, lvl = 0;
            if (ls >> i >> lvl && i >= 0 && i < MetaUnlockCount) m.unlock[i] = lvl;
        }
        // v4 and earlier stored these two unlocks by name.
        else if (key == "meta.startBalls") ls >> m.unlock[MetaStartBalls];
        else if (key == "meta.coreHp") ls >> m.unlock[MetaCoreHp];
        else if (key == "sound") { int v = 1; ls >> v; m.soundOn = v != 0; }
        else if (key == "fullscreen") { int v = 0; ls >> v; m.fullscreen = v != 0; }
        else if (key == "stat.enemiesKilled") ls >> m.stats.enemiesKilled;
        else if (key == "stat.coresEarned") ls >> m.stats.coresEarned;
        else if (key == "stat.bestWave") ls >> m.stats.bestWave;
        else if (key == "stat.bestCombo") ls >> m.stats.bestCombo;
        else if (key == "stat.runs") ls >> m.stats.runs;
        else if (key == "stat.wins") ls >> m.stats.wins;
        else if (key == "stat.maxSpeed") ls >> m.stats.maxSpeed;
        else if (key == "stat.timePlayed") ls >> m.stats.timePlayed;
        // "version", old run.* / ball / stat.lifetimeScrap lines: ignored.
    }

    for (int i = 0; i < MetaUnlockCount; ++i) {
        if (m.unlock[i] < 0) m.unlock[i] = 0;
        if (m.unlock[i] > metaUnlockDef(i).maxLevel) m.unlock[i] = metaUnlockDef(i).maxLevel;
    }
    return sawAnything;
}

}  // namespace sb
