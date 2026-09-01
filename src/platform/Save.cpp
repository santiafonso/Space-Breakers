#include "platform/Save.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

// Plain-text "key value" save file. It is trivial to inspect, forward
// compatible (unknown keys are ignored, missing keys keep their defaults) and
// has none of the struct-padding hazards of the previous binary blob.
namespace sb {

namespace {
constexpr int kSaveVersion = 2;
constexpr std::size_t kMaxWalls = 6;
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

    f << "version " << kSaveVersion << '\n';
    f << "points " << d.points << '\n';
    f << "speedLevel " << d.level[UpgSpeed] << '\n';
    f << "pointsLevel " << d.level[UpgPoints] << '\n';
    f << "multiballLevel " << d.level[UpgMultiball] << '\n';
    f << "wallLevel " << d.level[UpgWalls] << '\n';
    f << "comboLevel " << d.level[UpgCombo] << '\n';
    f << "luckLevel " << d.level[UpgLuck] << '\n';
    f << "sound " << (d.soundOn ? 1 : 0) << '\n';
    f << "fullscreen " << (d.fullscreen ? 1 : 0) << '\n';
    for (std::size_t i = 0; i < d.walls.size() && i < kMaxWalls; ++i) {
        const WallSnapshot& w = d.walls[i];
        f << "wall " << i << ' ' << w.cx << ' ' << w.cy << ' ' << w.hx << ' ' << w.hy
          << ' ' << w.vx << ' ' << w.vy << '\n';
    }
    f << "stat.lifetimeBounces " << d.stats.lifetimeBounces << '\n';
    f << "stat.lifetimePoints " << d.stats.lifetimePoints << '\n';
    f << "stat.bestScore " << d.stats.bestScore << '\n';
    f << "stat.bestCombo " << d.stats.bestCombo << '\n';
    f << "stat.maxSpeed " << d.stats.maxSpeed << '\n';
    f << "stat.timePlayed " << d.stats.timePlayed << '\n';
    return f.good();
}

bool loadGame(const std::string& path, GameData& d) {
    std::ifstream f(path);
    if (!f) return false;

    bool sawAnything = false;
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;
        sawAnything = true;

        if (key == "points") ls >> d.points;
        else if (key == "speedLevel") ls >> d.level[UpgSpeed];
        else if (key == "pointsLevel") ls >> d.level[UpgPoints];
        else if (key == "multiballLevel") ls >> d.level[UpgMultiball];
        else if (key == "wallLevel") ls >> d.level[UpgWalls];
        else if (key == "comboLevel") ls >> d.level[UpgCombo];
        else if (key == "luckLevel") ls >> d.level[UpgLuck];
        else if (key == "sound") { int v = 1; ls >> v; d.soundOn = v != 0; }
        else if (key == "fullscreen") { int v = 0; ls >> v; d.fullscreen = v != 0; }
        else if (key == "wall") {
            std::size_t idx = 0;
            if (ls >> idx && idx < kMaxWalls) {
                if (d.walls.size() <= idx) d.walls.resize(idx + 1);
                WallSnapshot& w = d.walls[idx];
                ls >> w.cx >> w.cy >> w.hx >> w.hy >> w.vx >> w.vy;
            }
        }
        else if (key == "stat.lifetimeBounces") ls >> d.stats.lifetimeBounces;
        else if (key == "stat.lifetimePoints") ls >> d.stats.lifetimePoints;
        else if (key == "stat.bestScore") ls >> d.stats.bestScore;
        else if (key == "stat.bestCombo") ls >> d.stats.bestCombo;
        else if (key == "stat.maxSpeed") ls >> d.stats.maxSpeed;
        else if (key == "stat.timePlayed") ls >> d.stats.timePlayed;
        // "version" and anything unrecognised: ignored on purpose.
    }

    for (int i = 0; i < UpgradeCount; ++i)
        if (d.level[i] < 0) d.level[i] = 0;

    return sawAnything;
}

}  // namespace sb
