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
    std::uint32_t bestScore = 0;   // highest run score
    std::uint32_t runs = 0;
    std::uint32_t wins = 0;        // runs that cleared the final wave
    float maxSpeed = 0.f;          // px/s
    double timePlayed = 0.0;       // seconds
};

// Everything that survives between sessions: the meta currency, permanent
// unlocks and settings.
struct MetaState {
    std::uint32_t cores = 0;
    std::uint32_t prisms = 0;   // special currency from beating the miniboss
    int unlock[MetaUnlockCount] = {};
    bool soundOn = true;
    bool fullscreen = false;
    Stats stats;
};

// Whole-run relics picked between waves. Ball gear lives on each ball's
// loadout instead (RunState::balls). Reset when a run ends; not persisted - a
// run is a short sprint, so there is no mid-run resume.
struct RunMods {
    bool spring = false;
    bool slowField = false;
    bool strongArm = false;   // flung balls leave the hand harder
    bool contagion = false;   // a poisoned enemy dying re-poisons nearby
    bool primed = false;      // +damage vs enemies under an element effect
};

// The current run, in memory only.
struct RunState {
    bool active = false;
    int wave = 0;
    float coreHp = cfg::core::baseHp;
    float coreMaxHp = cfg::core::baseHp;
    int score = 0;             // arcade points, +100 per kill (x2 under DOUBLE POINTS)
    int rerollsLeft = 0;       // "Foresight" web node: item-reroll charges remaining this run
    float bountyCores = 0.f;   // cores accrued from the "Fortune" node this run
    std::vector<BallLoadout> balls;   // one per ball in play, same order as World::balls()
    RunMods mods;
};

struct GameData {
    MetaState meta;
    RunState run;
};

}  // namespace sb
