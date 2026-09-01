#pragma once

#include <cstdint>
#include <vector>

#include "core/Math.hpp"  // WallSnapshot
#include "progression/Upgrades.hpp"

namespace sb {

// Lifetime records shown on the Stats screen. Persisted alongside the run.
struct Stats {
    std::uint64_t lifetimeBounces = 0;
    std::uint64_t lifetimePoints = 0;
    std::uint32_t bestScore = 0;
    std::uint32_t bestCombo = 0;   // longest bounce streak
    float maxSpeed = 0.f;          // px/s
    double timePlayed = 0.0;       // seconds
};

// Everything that survives between sessions: the current run, the upgrade
// levels, the placed walls and the settings.
struct GameData {
    std::uint32_t points = 0;
    int level[UpgradeCount] = {};
    bool soundOn = true;
    bool fullscreen = false;
    std::vector<WallSnapshot> walls;
    Stats stats;
};

}  // namespace sb
