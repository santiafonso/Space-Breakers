#pragma once

#include <cstdint>
#include <vector>

#include "core/Config.hpp"
#include "core/Math.hpp"  // FieldSnapshot
#include "progression/Offers.hpp"

namespace sb {

// Lifetime records shown on the Stats screen. Part of the persistent meta.
struct Stats {
    std::uint64_t enemiesKilled = 0;
    std::uint64_t lifetimeScrap = 0;
    std::uint32_t bestWave = 0;
    std::uint32_t bestCombo = 0;   // longest damage-combo streak
    std::uint32_t runs = 0;
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

// The current run. Reset when a run ends; saved so a run in progress can be
// resumed after quitting.
struct RunState {
    bool active = false;
    std::uint32_t scrap = 0;
    int wave = 0;
    int ballCount = 1;
    float damageMult = 1.f;
    float coreHp = cfg::core::baseHp;
    float coreMaxHp = cfg::core::baseHp;
    std::vector<FieldSnapshot> field;
};

struct GameData {
    MetaState meta;
    RunState run;
};

}  // namespace sb
