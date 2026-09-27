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
// More ability slots (see abilitySlotCount in progression/Offers.hpp). Its
// items act through the ball's abilities (World::mageCastRate / mageOnCast /
// mageTick, called from sim/WorldAbilities.cpp), so they work on any ball.
struct MageMods {
    float focus = 0.f;          // Focus: abilities recharge this much faster (0.15 = 15%)
    float missileFrac = 0.f;    // Arcane missile: damage x the ball's hit (0 = off)...
    float missileEvery = 0.f;   // ...a timed volley this often (plus one per cast)...
    int missileTargets = 0;     // ...at this many enemies
    float power = 1.f;          // Attunement: ability damage x this
    float twincast = 0.f;       // Twincast: chance a cast fires again
    float manaSpring = 0.f;     // Mana spring: a cast charges its other abilities this share
};
struct MageState {
    float missileT = 0.f;       // Arcane missile: time to the next timed volley
    int echoSlot = -1;          // Twincast: the ability slot about to fire again...
    float echoT = 0.f;          // ...in this long
};
struct MageWorld {
    // Arcane missile visuals: short fading streaks (render/ClassRender.cpp).
    struct Streak {
        sf::Vector2f a, b;
        float life = 0.f;
    };
    std::vector<Streak> streaks;
};

// ==================================================================== Shooter
// Fires bullets.
struct ShooterMods {};
struct ShooterState {};
struct ShooterWorld {};

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
