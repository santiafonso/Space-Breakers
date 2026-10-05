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
    int banks = 0;            // "Bank shot": wall bounces left
    float hollow = 0.f;       // "Hollow point": + damage vs an enemy under an element
    float suppress = 0.f;     // "Suppressing fire": slow per hit
    float frag = 0.f;         // "Frag rounds": burst x the bullet
    bool mark = false;        // "Marksman": this one marks
    float smart = 0.f;        // "Smart rounds": turn rate (rad/s)
};
struct ShooterRail {          // Deadeye's rail shot, kept only to be drawn
    sf::Vector2f a, b;
    float life = 0.f;
    float maxLife = 0.f;      // 0 = Deadeye's
    float width = 0.f;        // "Ion cannon": a thick beam (0 = Deadeye's thin line)
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
    float wardenR = 0.f;       // the spirits' current orbit (0 = not set yet: the usual one)
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
    int pierce = 0;            // "Spirit host": a great spirit goes through this many more
    int lastHit = -1;          // ...not the same one twice in a row
    bool great = false;        // ...drawn big
    bool boom = false;         // "Necromancer": a ghost that explodes on the first enemy
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

// ==================================================================== Alchemist
// The class of elements (2026-09-28): 2 elements (3 ascended) that take turns
// hit by hit and react with each other. Its items shape its reactions
// (World::strike / triggerReaction read them through World::reactBall_).
struct AlchemistMods {
    float crucible = 1.f;      // "Crucible": its reactions' damage x this...
    float crucibleRadius = 1.f;// ...and reach x this
    float aftershock = 0.f;    // "Aftershock": its reactions leave its element this far around (0 = off)
    float flux = 0.f;          // "Flux": each reaction recharges its abilities this share
    int prism = 0;             // "Prism": every Nth hit carries all its elements (0 = off)
    bool conflux = false;      // "Conflux": its reactions leap once to another afflicted enemy
};
struct AlchemistState {
    int hits = 0;              // "Prism": hits so far
};
struct AlchemistWorld {};

// ---------------------------------------------------------------- bundles
// (no class logic below this line)

// ==================================================================== commons
// The 2026-10-04 commons, every class (sim/WorldCommons.cpp). They work on any
// ball that carries them - no class needed. 0 = not equipped.
struct CommonMods {
    float volatileMul = 0.f;   // "Volatile": its elements last + this
    float seepLife = 0.f;      // "Seep": a wall bounce leaves a puddle this long...
    float seepRadius = 0.f;    // ...this wide
    float reagent = 0.f;       // "Reagent": + damage vs another ball's element
    float tincture = 0.f;      // "Tincture": a clacked ball's next hit x this, with its element
    float distill = 0.f;       // "Distill": core hp per 3rd reaction
    float pickpocket = 0.f;    // "Pickpocket": chance a hit steals a coin
    float hotStreak = 0.f;     // "Hot streak": + damage per double in a row
    float luckyBreak = 0.f;    // "Lucky break": chance a hit recharges its abilities
    int confetti = 0;          // "Confetti": sparks per doubled hit...
    float confettiFrac = 0.f;  // ...each x the hit
    float spellblade = 0.f;    // "Spellblade": first hit after a cast + this
    float aetherStep = 0.f;    // "Aether step": a cast kicks its speed x this
    float insight = 0.f;       // "Insight": s off its cooldowns per kill
    float linger = 0.f;        // "Lingering": its summons last + this
    float packLeader = 0.f;    // "Pack leader": + damage per live summon
    float decoy = 0.f;         // "Decoy": its turrets / totems pull enemies at this speed
    float doom = 0.f;          // "Mark for death": its blink target takes + this...
    float doomTime = 0.f;      // ...for this long
    float bleed = 0.f;         // "Bleed": x the hit, over cfg::common::bleedTime
    float quickstep = 0.f;     // "Quickstep": a blink kicks its speed x this
    float opportunist = 0.f;   // "Opportunist": + damage vs frozen / staggered
    float hollow = 0.f;        // "Hollow point": its bullets + this vs an enemy under an element
    int bankShots = 0;         // "Bank shot": wall bounces per bullet
    int cover = 0;             // "Covering fire": bullets per volley when you throw another ball
    float longPull = 0.f;      // "Long pull": the throw's first hit + up to this
    float relay = 0.f;         // "Relay": the other ball leaves at this x its cruise
    float secondWind = 0.f;    // "Second wind": share of your Q's cooldown a catch refunds
    float plating = 0.f;       // "Plating": core max hp (App folds it, not the sim)
    float sentinel = 0.f;      // "Sentinel": + damage beside the core
    float brace = 0.f;         // "Brace": enemies it hits are slowed by this
    float heavyLanding = 0.f;  // "Heavy landing": core-bounce shockwave x its hit
    float firstBlood = 0.f;    // "First blood": first hit of a fight x this
    float comboHit = 0.f;      // "Combo hit": + damage on a quick second enemy
    float adrenaline = 0.f;    // "Adrenaline": + cruise per kill this fight
    float spotter = 0.f;       // "Spotter": what it hits takes + this from the others
    float anthem = 0.f;        // "Anthem": every other ball + this damage
    float medic = 0.f;         // "Medic": core hp per clack
    float sync = 0.f;          // "Sync": s off the others' cooldowns per cast
    float handOff = 0.f;       // "Hand-off": its kill gives the nearest ball + this
    // uncommons (2026-10-04)
    float fission = 0.f;       // "Fission": a death under its element spreads it this far
    float elixir = 0.f;        // "Elixir": its reactions give every ball + this for a moment
    float houseEdge = 0.f;     // "House edge": chance gold comes double (App reads it)
    float mirror = 0.f;        // "Mirror": chance the nearest ball copies its hit
    float sigil = 0.f;         // "Sigil": a cast leaves a glyph that bursts for this x its hit
    float arcaneCharge = 0.f;  // "Arcane charge": s off its cooldowns per wall bounce
    float conduit = 0.f;       // "Conduit": a cast primes enemies this far with another ball's element
    float martyr = 0.f;        // "Martyr": an ending turret / totem bursts for this x its hit
    float guardDog = 0.f;      // "Guard dog": its summons + this damage, aimed at the core's threat
    float chainKill = 0.f;     // "Chain kill": a kill this soon after a blink recharges it
    float venomBlade = 0.f;    // "Venom blade": the first hit after a blink + this, its element twice
    float suppress = 0.f;      // "Suppressing fire": slow per bullet
    int ammoFeed = 0;          // "Ammo feed": bullets per wall bounce
    float windUp = 0.f;        // "Wind-up": a throw + up to this when held long enough
    float bankThrow = 0.f;     // "Bank throw": + this per wall before the thrown first hit
    float bully = 0.f;         // "Bully": + damage vs bigger enemies (and bosses)
    float carom = 0.f;         // "Carom": + damage right after a clack
    // rares (2026-10-04)
    float transmute = 0.f;     // "Transmute": s it carries the other element after a reaction
    float residue = 0.f;       // "Residue": its reactions leave a priming puddle this long
    int aurum = 0;             // "Aurum": gold per kill by its reactions
    float highRoller = 0.f;    // "High roller": a doubled hit + this on top (every hit x penalty)
    int luckySeven = 0;        // "Lucky seven": every Nth hit is a double
    float fortune = 0.f;       // "Fortune": chances + this per double this fight
    bool projection = false;   // "Projection": its copies cast
    float spellweave = 0.f;    // "Spellweave": its abilities + this vs an enemy under an element
    bool cascade = false;      // "Cascade": an ability that kills is ready again
    int hatchery = 0;          // "Hatchery": wisps per summon kill
    float golemEvery = 0.f;    // "Golem": s between golems (0 = off)...
    float golemFrac = 0.f;     // ...its slam x the ball's hit
    bool soulbind = false;     // "Soulbind": summons leave the full element
    float predator = 0.f;      // "Predator": the hit after a (longer, weakest-first) blink + this
    float flicker = 0.f;       // "Flicker": chance a blink blinks again
    float frag = 0.f;          // "Frag rounds": bullet burst x the bullet
    int marksman = 0;          // "Marksman": every Nth bullet hits x3 and marks
    float skipping = 0.f;      // "Skipping stone": speed x this per skip (0 = off)
    float formation = 0.f;     // "Formation": the others follow your throw at this x its speed
    float shieldBash = 0.f;    // "Shield bash": knock near the core...
    float bashStun = 0.f;      // ...and stun
    // epics (2026-10-04)
    float saturate = 0.f;      // "Saturate": both elements' full effects this far around a reaction
    float maelstrom = 0.f;     // "Maelstrom": s between rings (0 = off)...
    float maelstromR = 0.f;    // ...this wide
    float sentence = 0.f;      // "Death sentence": the kill threshold (share of max hp)
    float razor = 0.f;         // "Razor wire": the line cuts for this x its hit...
    float razorLife = 0.f;     // ...this long
    float wardEvery = 0.f;     // "Ward ring": s between throws (0 = off)
    float juggernaut = 0.f;    // "Juggernaut": damage + this x its size bonus
    float lastLine = 0.f;      // "Last line": the blast x its hit (0 = off)...
    float lastLineStun = 0.f;  // ...and the stun
    float slotEvery = 0.f;     // "Slot machine": s between spins (0 = off)
    float wildCd = 0.f;        // "Wild magic": s between casts (0 = off)
    int chorus = 0;            // "Spell chorus": every Nth cast
    float chrono = 0.f;        // "Chronomancy": slow per cast
    float minigun = 0.f;       // "Minigun": spin-up per volley (0 = off)
    float smart = 0.f;         // "Smart rounds": bullet turn rate (rad/s, 0 = off)
    int shrapnel = 0;          // "Shrapnel": bullets per kill
    float perfect = 0.f;       // "Perfect release": the perfect throw's hit x this (0 = off)
    float homecoming = 0.f;    // "Homecoming": the catch shockwave x its hit (0 = off)
    float lightspeed = 0.f;    // "Lightspeed": chain x the hit (0 = off)...
    int lightChains = 0;       // ...this many
    float spiritHost = 0.f;    // "Spirit host": the spirit's damage x (0 = off)...
    int spiritPierce = 0;      // ...enemies it goes through
    bool twinCall = false;     // "Twin call"
    float fortify = 0.f;       // "Fortify": its turrets last x this (0 = off)
    // legendaries (2026-10-05)
    float titan = 0.f;         // "Titan": growth per core bounce (0 = off)...
    float titanQuake = 0.f, titanStun = 0.f;   // ...the quake at full size
    float archmage = 0.f;      // "Archmage": cast rate x this (0 = off)...
    float archStack = 0.f;     // ...+ ability damage per cast
    float timeStop = 0.f;      // "Time stop": s every enemy stands still (0 = off)
    float ion = 0.f;           // "Ion cannon": the laser x its hit (0 = off)...
    float ionCharge = 0.f;     // ...s to charge
    float gunship = 0.f;       // "Gunship": the other balls' bullet x their hit (0 = off)
    float leadStorm = 0.f;     // "Lead storm": s between storms (0 = off)
    float reaper = 0.f;        // "Reaper": the blink's cooldown (0 = off)
    float contract = 0.f;      // "Contract": share of max hp per hit (0 = off)...
    float contractBoss = 0.f;  // ...of a boss's
    float necro = 0.f;         // "Necromancer": the ghost's blast x its hit (0 = off)
    float overlord = 0.f;      // "Overlord": every summon + this damage (0 = off)
    bool rigged = false;       // "Rigged"
    float steady = 0.f;        // "Steady hand": time scale while aimed (0 = off)...
    float steadyBonus = 0.f;   // ...the throw's first hit + up to this
    float showtime = 0.f;      // "Showtime": per quick catch (0 = off)
    int philo = 0;             // "Philosopher's stone": elements per hit (0 = off)
    int grandEvery = 0;        // "Grand reaction": every Nth (0 = off)...
    float grand = 0.f;         // ...x the hit
};
struct CommonState {
    bool doubled = false;      // this hit was doubled (crit, coin heads, a Jester double)
    int streak = 0;            // "Hot streak": doubles in a row
    float spellT = 0.f;        // "Spellblade": s the armed hit waits
    bool firstBlood = false;   // "First blood": armed at each fight's start
    int lastHitId = -1;        // "Combo hit": the enemy it hit last...
    float lastHitT = 99.f;     // ...this long ago
    int adrenaline = 0;        // "Adrenaline": kills this fight
    float pull = 0.f;          // "Long pull": the throw's first hit + this (0 = spent)
    float handOff = 0.f;       // "Hand-off": the next hit + this (given by another ball)
    float medicCd = 0.f;       // "Medic": between repairs
    float landingCd = 0.f;     // "Heavy landing": between shockwaves
    int reactions = 0;         // "Distill": reactions it set off
    bool coverPending = false; // "Covering fire": fire on the next tick (it needs the params)
    float chainT = 0.f;        // "Chain kill": s left to land a kill after a blink
    bool venom = false;        // "Venom blade": armed by a blink
    bool bankArmed = false;    // "Bank throw": thrown, first hit not landed yet...
    int banks = 0;             // ...walls since
    float caromT = 0.f;        // "Carom": s left after a clack
    float transT = 0.f;        // "Transmute": s it still carries...
    int transElem = 0;         // ...this element (an Element)
    int hits7 = 0;             // "Lucky seven": hits toward the sure double
    bool forcedDouble = false; // ...this hit is it
    int fortune = 0;           // "Fortune": doubles this fight
    bool predArmed = false;    // "Predator": the hit after the blink
    int flickers = 0;          // "Flicker": blinks in this chain
    int shots = 0;             // "Marksman": bullets fired
    bool skipping = false;     // "Skipping stone": thrown, not back at the core yet
    float golemT = 0.f;        // "Golem": to the next one
    float maelT = 0.f;         // "Maelstrom": to the next ring
    float slotT = 0.f;         // "Slot machine": to the next spin...
    float slotDmgT = 0.f, slotSpeedT = 0.f, slotGoldT = 0.f;   // ...its buffs, s left
    float wildT = 0.f;         // "Wild magic": cooldown
    int casts = 0;             // "Spell chorus": casts so far
    float spin = 0.f;          // "Minigun": 0..1 spun up...
    float spinIdle = 0.f;      // ...s since its last volley
    bool perfect = false;      // "Perfect release": this throw is one (until a wall)
    bool homeArmed = false;    // "Homecoming": thrown, first hit not landed...
    float homeT = 0.f;         // ...s left curving home
    bool homePulse = false;    // ...caught on the way: pulse on the next tick
    float titan = 0.f;         // "Titan": 0..1 grown (1 = twice the size)
    int archStacks = 0;        // "Archmage": casts this fight
    int stopCasts = 0;         // "Time stop": casts toward the next
    float ionT = 0.f;          // "Ion cannon": 0..1 charged...
    sf::Vector2f ionDir{1.f, 0.f};   // ...where it'll fire
    float stormT = 0.f;        // "Lead storm": to the next...
    float stormOn = 0.f;       // ...s left firing...
    float stormFireT = 0.f, stormAng = 0.f;
    int showStacks = 0;        // "Showtime": quick catches this fight
    int grandN = 0;            // "Grand reaction": reactions so far
    // abilities, second batch (2026-10-05; sim/WorldAbilities.cpp)
    float empower = 0.f;       // "Empower": the next hit x this (0 = not armed)...
    float empowerR = 0.f;      // ...and its wave this wide...
    float empowerT = 0.f;      // ...if it lands before this runs out
    bool empowerHit = false;   // ...this hit was the empowered one (the wave goes off in commonOnHit)
    float beamT = 0.f, beamTickT = 0.f, beamElemT = 0.f, beamDmg = 0.f;   // "Beam": s left, tick timers, per tick
    sf::Vector2f beamTo;       // ...where it's burning (drawn)
    bool beamOn = false;
    float bladesT = 0.f, bladeAng = 0.f, bladeTickT = 0.f, bladeDmg = 0.f;   // "Blades"
    int bladeN = 0;
    float leashT = 0.f, leashL = 0.f, leashSpeed = 0.f;   // "Leash": s left, rope length, the swing's floor
    float portalCd = 0.f;      // "Portal": not again right away
};
struct CommonWorld {
    struct Puddle {            // "Seep"
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, maxLife = 1.f, tickT = 0.f, dmg = 0.f;
        int elem = 0, owner = -1;
        bool prime = false;    // "Residue": primes only (no touch, no damage)
    };
    std::vector<Puddle> puddles;
    struct Glyph {             // "Sigil"
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, dmg = 0.f;
        int elem = 0, owner = -1;
    };
    std::vector<Glyph> glyphs;
    float elixirT = 0.f, elixir = 0.f;   // "Elixir": every ball + elixir while elixirT runs
    struct Golem {             // "Golem": walks to the enemy nearest the core, slams
        sf::Vector2f pos;
        float life = 0.f, maxLife = 1.f, slamT = 0.f, dmg = 0.f;
        float flash = 0.f;     // 1 on a slam, fades (visual)
        int elem = 0, owner = -1;
    };
    std::vector<Golem> golems;
    struct Wire {              // "Razor wire"
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f, dmg = 0.f, tickT = 0.f;
    };
    std::vector<Wire> wires;
    float wardCd = 0.f;        // "Ward ring": s until it's up again...
    bool wardOn = false;       // ...some ball carries it (the renderer's ring)
    bool lastLineUsed = false; // "Last line": once a fight
    float chronoT = 0.f, chrono = 0.f;   // "Chronomancy": every enemy slowed by chrono while chronoT runs
    float stopT = 0.f;         // "Time stop": every enemy stands still while it runs
    int contractId = -1;       // "Contract": the enemy under contract (Enemy::id)...
    bool contractBoss = false; // ...or the boss
    bool rigged = false;       // "Rigged": some ball carries it (World::chance rolls twice)
    // abilities, second batch (2026-10-05)
    struct Mine {              // "Mines"
        sf::Vector2f pos;
        float life = 0.f, arm = 0.f, dmg = 0.f, r = 0.f;
        int elem = 0, owner = -1;
    };
    std::vector<Mine> mines;
    struct Vortex {            // "Vortex"
        sf::Vector2f pos;
        float life = 0.f, maxLife = 1.f, r = 0.f, dps = 0.f;
        int elem = 0;
    };
    std::vector<Vortex> vortices;
    struct Cloud {             // "Cloud"
        sf::Vector2f pos;
        float life = 0.f, maxLife = 1.f, r = 0.f, dmg = 0.f, tickT = 0.f;
        int elem = 0;
    };
    std::vector<Cloud> clouds;
    struct Portal {            // "Portal": a pair
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f;
        int owner = -1;
    };
    std::vector<Portal> portals;
    float tailT = 0.f, tailMul = 1.f;   // "Tailwind": every ball's cruise x tailMul while tailT runs
};

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
    AlchemistMods alchemist;
    CommonMods common;
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
    AlchemistState alchemist;
    CommonState common;
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
    AlchemistWorld alchemist;
    CommonWorld common;
};

}  // namespace sb
