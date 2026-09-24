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

// Between-wave items picked this run. Reset when a run ends. Not persisted - a
// run is a short sprint, so there is no mid-run resume. heavyImpact / bigBall
// also carry the Heft / Mass web levels seeded at run start.
struct RunMods {
    int heavyImpact = 0;   // +contact damage picks (item + Heft), capped
    int bigBall = 0;       // +radius picks (item + Mass), capped
    bool spring = false;
    bool slowField = false;
    bool flingMomentum = false;
    bool wallRush = false;    // ball speeds up on wall bounces
    bool carom = false;       // ball speeds up on ball-vs-ball bounces
    bool strongArm = false;   // flung balls leave the hand harder
    bool ricochet = false;    // brief damage bonus after a wall bounce
    bool ceilingBreak = false;// higher top-speed ceiling
    bool warmUp = false;      // cruise speed ramps up over the wave
    bool heavyKnock = false;  // stronger enemy knockback
    bool conductor = false;   // electric arc jumps to a 2nd enemy
    bool shatter = false;     // bonus damage vs frozen enemies
    bool contagion = false;   // a poisoned enemy dying re-poisons nearby
    bool bedrock = false;     // stone rubble lasts far longer
    bool primed = false;      // +damage vs enemies under an element effect
    bool spearhead = false;   // the most recently flung ball cruises faster
    bool crit = false;        // chance of a double-damage contact hit
    bool bruiser = false;     // contact damage scales with ball speed
    bool executioner = false; // huge damage to low-HP enemies
    bool overkill = false;    // a kill's leftover damage splashes to a neighbour
    bool cleave = false;      // the ball passes through an enemy it kills
    bool tempo = false;       // ball recovers cruise speed faster after an enemy hit
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
    std::vector<int> balls;  // Element per ball
    std::vector<int> picks;  // UpgradeKind per between-wave choice made, in order
    RunMods mods;
};

struct GameData {
    MetaState meta;
    RunState run;
};

}  // namespace sb
