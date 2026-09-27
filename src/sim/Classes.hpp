#pragma once

#include <vector>

#include "core/Math.hpp"

// Per-class simulation data (the class framework, 2026-09-26).
//
// A ball's classes come from its item tags (2 items of a tag = that class, so
// a ball can hold two; 4 = the ascended form). Everything a class needs inside
// the simulation lives in ONE section of each of these files, so every class
// can be built on its own:
//   - here:                  its per-ball numbers (XxxMods, filled by
//                            core/ClassSpec.cpp from its items), its per-ball
//                            runtime state (XxxState) and any world-level
//                            state it owns (XxxWorld: bullets, turrets...).
//   - sim/WorldClasses.cpp:  its hooks (tick, hit, kill, wall / core bounce,
//                            damage, a once-per-step world tick, wave start).
//   - render/ClassRender.cpp: its mark on the ball and its world visuals.
//   - core/ConfigClasses.hpp: its tuning numbers (cfg::<class>).
// Striker / Guardian / Support keep their long-standing numbers in BallMods;
// their sections here are empty on purpose.

namespace sb {

// ==================================================================== Striker
struct StrikerMods {};
struct StrikerState {};
struct StrikerWorld {};

// ==================================================================== Guardian
struct GuardianMods {};
struct GuardianState {};
struct GuardianWorld {};

// ==================================================================== Support
struct SupportMods {};
struct SupportState {};
struct SupportWorld {};

// ==================================================================== Mage
// More ability slots (see abilitySlotCount in progression/Offers.hpp).
struct MageMods {};
struct MageState {};
struct MageWorld {};

// ==================================================================== Shooter
// Fires bullets at nearby enemies (sim/WorldClasses.cpp).
struct ShooterMods {
    float rate = 1.f;         // "Rapid fire": volleys per second x this
    int pellets = 1;          // "Scattershot": bullets per volley
    float pelletFrac = 1.f;   // ...each one's damage x this
    int hops = 0;             // "Rebound": enemy-to-enemy hops per bullet
    float hopKeep = 1.f;      // ...damage kept per hop
    float tracerChance = 0.f; // "Tracer": chance a bullet carries the ball's element
    int pierce = 0;           // "Drill rounds": enemies a bullet passes through (and shields)
    int burst = 0;            // "Hair trigger": bullets fired when the ball lands a hit
    float burstFrac = 1.f;    // ...each one's damage x this
    float dmgMul = 1.f;       // bullet damage from Tracer / Drill levels
};
struct ShooterState {
    float fireT = 0.f;        // s to the next volley
    float burstCd = 0.f;      // "Hair trigger" cooldown
    int volleys = 0;          // Deadeye: every Nth one is a rail shot
};
struct ShooterBullet {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float dmg = 0.f;
    float life = 0.f;
    int elem = 0;             // Element (0 = plain); an int so this header stays sim-free
    int owner = -1;           // the ball it came from (reactions need two owners)
    int pierce = 0;           // enemies it can still pass through
    int hops = 0;             // hops left
    float hopKeep = 1.f;
    int lastHit = -1;         // Enemy::id it last hit (no double hits while passing through)
};
struct ShooterRail {          // Deadeye's rail shot, kept only to be drawn
    sf::Vector2f a, b;
    float life = 0.f;
};
struct ShooterWorld {
    std::vector<ShooterBullet> bullets;
    std::vector<ShooterRail> rails;
};

// ==================================================================== Assassin
// Teleports to the nearest enemy on a kill.
struct AssassinMods {};
struct AssassinState {};
struct AssassinWorld {};

// ==================================================================== Summoner
// Summons things: short-lived balls, turrets, a small dragon...
struct SummonerMods {};
struct SummonerState {};
struct SummonerWorld {};

// ==================================================================== Jester
// Plays on chance.
struct JesterMods {};
struct JesterState {};
struct JesterWorld {};

// ---------------------------------------------------------------- bundles
// (no class logic below this line)

// On BallMods (`mods.cls`): what the ball's class items add up to.
struct ClassMods {
    StrikerMods striker;
    GuardianMods guardian;
    SupportMods support;
    MageMods mage;
    ShooterMods shooter;
    AssassinMods assassin;
    SummonerMods summoner;
    JesterMods jester;
};

// On Ball (`cls`): per-ball runtime state. Copied into ghosts / twins.
struct ClassState {
    StrikerState striker;
    GuardianState guardian;
    SupportState support;
    MageState mage;
    ShooterState shooter;
    AssassinState assassin;
    SummonerState summoner;
    JesterState jester;
};

// On World (`World::classWorld()`): state a class owns outside the balls.
// Cleared on World::startRun.
struct ClassWorldState {
    StrikerWorld striker;
    GuardianWorld guardian;
    SupportWorld support;
    MageWorld mage;
    ShooterWorld shooter;
    AssassinWorld assassin;
    SummonerWorld summoner;
    JesterWorld jester;
};

}  // namespace sb
