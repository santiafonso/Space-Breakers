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
// Plays on chance: every hit rolls an outcome (WorldClasses.cpp).
struct JesterMods {
    float coinHeads = 0.f;     // "Coin flip": heads multiplier (0 = no coin)
    float wildChance = 0.f;    // "Wild card"
    float wildPower = 1.f;
    float reroll = 0.f;        // "Reroll": chance a missed roll gets a second try
    float chaosHit = 0.f;      // "Chaos bounce": the armed hit's multiplier (0 = none)
    float jackpotChance = 0.f; // "Jackpot"
    float jackpotBlast = 0.f;
    int jackpotGold = 0;
};
struct JesterState {
    float coinMul = 1.f;       // this hit's coin, flipped after the last one
    bool chaosArmed = false;   // off a chaos bounce: the next hit is armed
};
struct JesterWorld {
    struct Pop {               // a small outcome pip over an enemy (a double)
        sf::Vector2f pos;
        float t = 0.f;         // life left
    };
    std::vector<Pop> pops;
    float popCd = 0.f;
};

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
    // Classes (RoleMask bits) whose hooks also run for a ball that carries
    // their items without having the class (a single item). A class opts in
    // from its own fold (core/ClassSpec.cpp); its hooks must then check
    // b.hasRole() for the role's own effect. (Jester, 2026-09-26)
    unsigned loose = 0;
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
