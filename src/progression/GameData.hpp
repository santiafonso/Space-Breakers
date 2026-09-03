#pragma once

#include <cstdint>
#include <vector>

#include "core/Config.hpp"
#include "progression/Offers.hpp"

namespace sb {

// Lifetime records shown on the Stats screen. Part of the persistent meta.
struct Stats {
    std::uint64_t enemiesKilled = 0;
    std::uint64_t coresEarned = 0;
    std::uint32_t bestWave = 0;
    std::uint32_t bestCombo = 0;   // longest damage-combo streak
    std::uint32_t runs = 0;
    std::uint32_t wins = 0;        // runs that cleared the final wave
    float maxSpeed = 0.f;          // px/s
    double timePlayed = 0.0;       // seconds
};

// Everything that survives between sessions: the meta currency, permanent
// unlocks and settings.
struct MetaState {
    std::uint32_t cores = 0;
    int unlock[MetaUnlockCount] = {};
    bool soundOn = true;
    bool fullscreen = false;
    Stats stats;
};

// Between-wave upgrades picked this run. Reset when a run ends. Not persisted -
// a run is a short sprint, so there is no mid-run resume.
struct RunMods {
    int coreArmor = 0;      // +max HP picks
    int heavyImpact = 0;    // +contact damage picks
    int bigBall = 0;        // +radius picks
    bool spring = false;
    bool retaliate = false;
    bool flingMomentum = false;
    bool strayBolt = false;
    bool loot = false;
    bool secondChance = false;
    bool secondChanceUsed = false;
};

// The current run, in memory only.
struct RunState {
    bool active = false;
    int wave = 0;
    float coreHp = cfg::core::baseHp;
    float coreMaxHp = cfg::core::baseHp;
    std::vector<int> balls;  // Element per ball
    std::vector<int> picks;  // UpgradeKind per between-wave choice made, in order
    RunMods mods;
};

struct GameData {
    MetaState meta;
    RunState run;
};

}  // namespace sb
