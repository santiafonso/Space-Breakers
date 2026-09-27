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
// Fires bullets.
struct ShooterMods {};
struct ShooterState {};
struct ShooterWorld {};

// ==================================================================== Assassin
// Teleports ("blinks") to the nearest enemy on a kill.
struct AssassinMods {
    float backstab = 0.f;     // "Backstab": first hit after a blink x this (0 = off)
    float cull = 0.f;         // "Cull": a hit leaving an enemy under this HP fraction kills it
    float spreePer = 0.f;     // "Killing spree": + damage per blink in a chain...
    int spreeMax = 0;         // ...up to this many
    float trailFrac = 0.f;    // "Shadow trail": the blink path hits for this x its hit
    float smokeFrac = 0.f;    // "Smoke bomb": burst on arrival, x its hit...
    float smokeRadius = 0.f;  // ...this wide, leaving its element
    float phantomLife = 0.f;  // "Phantom": a shadow copy stays behind this long (0 = off)
};
struct AssassinState {
    float cd = 0.f;           // seconds until it can blink again
    bool pending = false;     // a kill queued a blink (done on its next tick)
    float pendingAt = 0.f;    // AssassinWorld::clock when it was queued
    float armedT = 0.f;       // "Backstab": seconds the next hit stays the big one
    int spree = 0;            // "Killing spree": blinks in the current chain
    float spreeT = 0.f;       // ...seconds before the chain breaks
};
struct AssassinWorld {
    // A blink's fading line (from where it left to where it landed) for the
    // renderer; `r` = the ball's radius (the afterimage left behind).
    struct Blink {
        sf::Vector2f a, b;
        float life = 0.f;
        float r = 0.f;
        bool cuts = false;    // the path dealt damage (Shadow trail / Shadow Assassin)
    };
    std::vector<Blink> blinks;
    float clock = 0.f;        // seconds since the run started (drops stale queued blinks)
};

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
