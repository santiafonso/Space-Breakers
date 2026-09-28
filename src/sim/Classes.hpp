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
// Its speed items (2026-09-28, sim/WorldStyle.cpp): "Anchor" grows as it
// slows, "Plow" as it speeds up.
struct GuardianMods {
    float anchor = 0.f;        // "Anchor": enemies near it slowed by up to this share (at a standstill)...
    float anchorRadius = 0.f;  // ...this close...
    float anchorPull = 0.f;    // ...and drawn toward it (px/s at a standstill)
    float plow = 0.f;          // "Plow": fast, it shoves enemies it passes (knock x this, 0 = off)...
    float plowFrac = 0.f;      // ...and hits them for this x its hit
};
struct GuardianState {};
struct GuardianWorld {};

// ==================================================================== Support
// Its speed items (sim/WorldStyle.cpp): "Beacon" (slow), "Wake" (fast),
// "Pass" (a clack launches the other ball).
struct SupportMods {
    float beacon = 0.f;        // "Beacon": balls passing near it hit + this harder (at a standstill)...
    float beaconRadius = 0.f;  // ...within this (full size when still)
    float wake = 0.f;          // "Wake": balls crossing its trail are sped up x this (0 = off)
    float pass = 0.f;          // "Pass": a ball it clacks is launched x this with its element (0 = off)
};
struct SupportState {
    float beaconT = 0.f;       // a Beacon / Pass charge on THIS ball: its next hit x beaconMul...
    float beaconMul = 1.f;
    int beaconElem = 0;        // ...and leaves this element (Element; 0 = none)...
    int beaconOwner = -1;      // ...from this ball (so it reacts with its own)
    float wakeCd = 0.f;        // "Wake": can't be boosted again yet
    float wakeT = 0.f;         // "Wake" owner: to the next trail point
};
struct SupportWorld {
    struct WakePoint {
        sf::Vector2f pos;
        float life = 0.f;
        float boost = 1.f;
        int owner = -1;
    };
    std::vector<WakePoint> wake;
};

// ==================================================================== Mage
// More ability slots (see abilitySlotCount in progression/Offers.hpp). Its
// items act through the ball's abilities (World::mageCastRate / mageOnCast /
// mageTick, called from sim/WorldAbilities.cpp), so they work on any ball.
struct MageMods {
    float focus = 0.f;          // Focus (+ the web's Channel / Archive): abilities recharge this much faster (0.15 = 15%)
    int barrage = 0;            // "Barrage": + magic missiles per Magic missile volley (0 = off)...
    float barrageFrac = 0.f;    // ...and every OTHER cast looses one missile at this x the ball's hit
    float missileMul = 1.f;     // ...all its magic missiles hit this much harder
    float power = 1.f;          // Attunement: ability damage x this
    float twincast = 0.f;       // Twincast: chance a cast fires again
    float manaSpring = 0.f;     // Mana spring: a cast charges its other abilities this share
    float meditate = 0.f;       // "Meditate": abilities recharge up to this much faster at a standstill
    float leyline = 0.f;        // "Leyline": runes it drops moving burst on each cast for this x its hit (0 = off)
};
struct MageState {
    int echoSlot = -1;          // Twincast: the ability slot about to fire again...
    float echoT = 0.f;          // ...in this long
    float still = 0.f;          // how still it is right now (0 at cruise, 1 stopped) - "Meditate"
    float runeT = 0.f;          // "Leyline": to the next rune
};
struct MageWorld {
    // Magic missiles in flight (the ability and Barrage). They home in on
    // `target` (an Enemy::id), picking the nearest enemy when it dies.
    struct Missile {
        sf::Vector2f pos, vel;
        float dmg = 0.f;
        float life = 0.f;
        int elem = 0;           // Element (0 = plain)
        int owner = -1;         // the ball it came from (reactions need two owners)
        int target = -1;        // Enemy::id it follows
        sf::Vector2f trail[6];  // recent positions, newest first (drawn as a short curve)
        int trailN = 0;
        float trailT = 0.f;
    };
    std::vector<Missile> missiles;
    struct Rune {               // "Leyline": waits where it was dropped for its ball's next cast
        sf::Vector2f pos;
        float life = 0.f;
        int owner = -1;
    };
    std::vector<Rune> runes;
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
    float slug = 0.f;         // "Slug": fire rate + this (and range + half) at a standstill
    float strafe = 0.f;       // "Strafe": fast, it fires side volleys at this x a bullet (0 = off)
};
struct ShooterState {
    float fireT = 0.f;        // s to the next volley
    float burstCd = 0.f;      // "Hair trigger" cooldown
    int volleys = 0;          // Deadeye: every Nth one is a rail shot
    float strafeT = 0.f;      // "Strafe": to the next side volley
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
    float lurk = 0.f;         // "Lurk": the first hit after a blink + up to this, charged by staying slow
    float blur = 0.f;         // "Blur": fast, it passes through enemies and marks them (mark x this, 0 = off)
};
struct AssassinState {
    float cd = 0.f;           // seconds until it can blink again
    bool pending = false;     // a kill queued a blink (done on its next tick)
    float pendingAt = 0.f;    // AssassinWorld::clock when it was queued
    float armedT = 0.f;       // "Backstab": seconds the next hit stays the big one
    int spree = 0;            // "Killing spree": blinks in the current chain
    float spreeT = 0.f;       // ...seconds before the chain breaks
    float lurk = 0.f;         // "Lurk": charge 0..1 (grows while slow)
    bool blinked = false;     // it blinked and hasn't hit since
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
    float bond = 1.f;          // web "Bond": every summon hits harder and lasts longer by this
    float kennel = 0.f;        // "Kennel": s between wisps at a standstill (0 = off; slower = sooner)...
    float kennelFrac = 0.f;    // ...each x its hit
    float drop = 0.f;          // "Drop turret": your throw leaves a turret this strong (x its hit, 0 = off)
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
    float kennelT = 0.f;       // "Kennel": charge toward the next wisp
    bool dropPending = false;  // "Drop turret": you just let go of it here...
    sf::Vector2f dropAt{0.f, 0.f};
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
    float sleight = 0.f;       // "Sleight": the teleport-strike hits x this (0 = off)...
    float sleightEvery = 0.f;  // ...charged this long at a standstill (slower = sooner)
};
struct JesterState {
    float coinMul = 1.f;       // this hit's coin, flipped after the last one
    bool chaosArmed = false;   // off a chaos bounce: the next hit is armed
    float sleightT = 0.f;      // "Sleight": charge toward the next trick
};
struct JesterWorld {
    struct Pop {               // a small outcome pip over an enemy (a double)
        sf::Vector2f pos;
        float t = 0.f;         // life left
    };
    std::vector<Pop> pops;
    float popCd = 0.f;
};

// ==================================================================== Slinger
// The class of your hands: throwing and catching (2026-09-28). Its items act
// on your throws and catches (World::classOnGrab / classOnThrow) and work on
// any ball that carries one (ClassMods::loose).
struct SlingerMods {
    float coil = 0.f;          // "Coil": your throw x this (0 = off); left alone it coasts to a stop...
    float coilDrag = 0.f;      // ...losing speed this fast (per s)
    float releasePer = 0.f;    // "Catch & release": + damage per stack...
    int releaseMax = 0;        // ...up to this many
    float burnFrac = 0.f;      // "Afterburner": flame damage/s x the ball's hit (0 = off)...
    float burnTime = 0.f;      // ...for this long after your throw
    float momentum = 0.f;      // "Momentum": + damage per cruise of speed above its cruise
    float gripTurn = 0.f;      // "Grip": rad/s toward your pointer...
    float gripRange = 0.f;     // ...within this
    float ambush = 0.f;        // "Ambush": the thrown first hit blinks on and hits the next enemy x this (0 = off)
    float trick = 1.f;         // "Trick shot": chances x this until the thrown first hit
    float doubleDown = 0.f;    // "Double down": the won hit x this (0 = off)
    float execution = 0.f;     // "Execution throw": thrown first hit on an unhurt enemy x this (0 = off)
};
struct SlingerState {
    float sinceThrow = 99.f;   // seconds since you threw it ("Catch & release", "Double down")
    bool armed = false;        // you threw it and its first hit hasn't landed...
    float armedT = 0.f;        // ...for this much longer
    int stacks = 0;            // "Catch & release"
    bool doubleDown = false;   // "Double down": the next hit rolls double or nothing
    float burnT = 0.f;         // "Afterburner": seconds of flames left...
    float flameT = 0.f;        // ...to the next flame
};
struct SlingerWorld {
    struct Flame {             // "Afterburner": a patch of fire left along the throw
        sf::Vector2f pos;
        float life = 0.f, maxLife = 1.f;
        float dps = 0.f;
        int owner = -1;
    };
    std::vector<Flame> flames;
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
    SlingerMods slinger;
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
    SlingerState slinger;
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
    SlingerWorld slinger;
};

}  // namespace sb
