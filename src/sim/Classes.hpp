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
// Teleports to the nearest enemy on a kill.
struct AssassinMods {};
struct AssassinState {};
struct AssassinWorld {};

// ==================================================================== Summoner
// Summons things: spritelings (the class), turrets, wisps, totems, wardens and
// a dragonling (its items). Levels are already folded in; 0 = not equipped.
struct SummonerMods {
    float turretLife = 0.f;    // Turret: a wall bounce plants one
    float turretRate = 0.f;    // ...shots/s
    float turretFrac = 0.f;    // ...shot x the ball's hit
    int turretMax = 0;         // ...alive at once (per ball)
    int wisps = 0;             // Wisps: per kill
    float wispFrac = 0.f;
    float totemInterval = 0.f; // Totem: seconds between plants (0 = off)
    float totemLife = 0.f;
    float totemRadius = 0.f;
    float totemSlow = 0.f;
    int wardens = 0;           // Warden: spirits around the core
    float wardenFrac = 0.f;
    float dragonInterval = 0.f;   // Dragonling: seconds between breaths (0 = off)
    float dragonFrac = 0.f;
    float dragonCone = 0.f;
};
inline constexpr int kMaxWardens = 4;
struct SummonerState {
    bool sprite = false;       // this ball IS a spriteling (never summons)
    float spriteT = 0.f;       // time to the next spriteling
    float turretCd = 0.f;
    float totemT = 0.f;
    float dragonT = 0.f;
    bool dragonOut = false;    // the dragon has been placed (dragonPos is valid)
    sf::Vector2f dragonPos{0.f, 0.f};
    float dragonHeading = 0.f;
    float wardenAng = 0.f;
    float wardenRest[kMaxWardens] = {};
    sf::Vector2f prevVel{0.f, 0.f};   // last step's heading (a wall bounce flips it)
};
// Where a ball's i-th of n Warden spirits is (the sim and the renderer agree).
inline sf::Vector2f summonerWardenPos(sf::Vector2f core, float ang, int owner, int i, int n, float orbit) {
    const float a = ang + 0.9f * static_cast<float>(owner) +
                    6.2831853f * static_cast<float>(i) / static_cast<float>(n > 0 ? n : 1);
    const float r = orbit * (1.f + 0.15f * static_cast<float>(owner % 3));   // two Warden balls, two rings
    return core + sf::Vector2f{std::cos(a), std::sin(a)} * r;
}
// A turret shot or a wisp: a small projectile that hits one enemy.
struct SummonShot {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float dmg = 0.f;
    float life = 0.f;
    int elem = 0;              // Element (sim/Entities.hpp)
    int owner = -1;
    bool wisp = false;         // homes in on the nearest enemy
};
struct SummonTurret {
    sf::Vector2f pos;
    float life = 0.f, maxLife = 1.f;
    float rate = 1.f, fireT = 0.f;
    float dmg = 0.f;
    float aim = 0.f;           // barrel angle (visual)
    int elem = 0;
    int owner = -1;
};
struct SummonTotem {
    sf::Vector2f pos;
    float life = 0.f, maxLife = 1.f;
    float radius = 0.f, slow = 0.f;
    float pulseT = 0.f, dmg = 0.f;
    float flash = 0.f;         // 1 on a pulse, fades (visual)
    int elem = 0;
    int owner = -1;
};
struct DragonBreath {          // visual only
    sf::Vector2f pos;
    float dir = 0.f, cone = 0.f, range = 0.f;
    float life = 0.f;
    int elem = 0;
};
struct SummonerWorld {
    std::vector<SummonShot> shots;
    std::vector<SummonTurret> turrets;
    std::vector<SummonTotem> totems;
    std::vector<DragonBreath> breaths;
};

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
