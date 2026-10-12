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
    int chain = 0;            // "Ion cannon": on a hit it becomes a bolt that jumps this many times...
    float chainKeep = 1.f;    // ...keeping this much per jump
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
    float swordEvery = 0.f;    // "Spectral sword": s between darts (0 = off)...
    float swordFrac = 0.f;     // ...each cut x the hit...
    int swords = 0;            // ...how many (one per item level)...
    float swordRange = 0.f;    // ...and how far they reach
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
    struct Sword {             // "Spectral sword" (one per item level)
        float t = 0.f;         // to the next dart
        bool out = false;      // placed (pos is valid)
        sf::Vector2f pos{0.f, 0.f};
        float ang = 0.f;       // where its point faces
        int phase = 0;         // 0 floating, 1 darting, 2 flying back
        float clock = 0.f;     // s into the phase
        sf::Vector2f from{0.f, 0.f}, to{0.f, 0.f};
        int hit[8] = {};       // enemies already cut this dart (Enemy::id)
        int hits = 0;
    };
    Sword swords[3];
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
    int items = 0;             // Jester items it carries (the chaos grows with them)
};
struct JesterState {
    float coinMul = 1.f;       // this hit's coin, flipped after the last one
    bool chaosArmed = false;   // off a chaos bounce: the next hit is armed
    float sleightT = 0.f;      // "Sleight": charge toward the next trick
    float chaosT = 0.f;        // the class's chaos: to its next trick
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
    float beckonCd = 0.f;      // "Beckon": s between blinks to a click on empty ground (0 = off)
    float execution = 0.f;     // "Execution throw": thrown first hit on an unhurt enemy x this (0 = off)
};
struct SlingerState {
    float sinceThrow = 99.f;   // seconds since you threw it ("Catch & release")
    float idleT = 0.f;         // the class: s since your throw / the wave's start (its cruise sinks after a while)
    bool armed = false;        // you threw it and its first hit hasn't landed...
    float armedT = 0.f;        // ...for this much longer
    int stacks = 0;            // "Catch & release"
    float beckonT = 0.f;       // "Beckon": s until it can blink again
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
    int pingPong = 0;          // "Ping-pong": walls per throw that aim it at the nearest enemy
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
    float slotEvery = 0.f;     // "777": s between spins (0 = off)...
    float slotBuff = 0.f;      // ...a triple lasts this long
    float luckyEvery = 0.f;    // "Lucky number": s between rolls (0 = off)
    float rouletteEvery = 0.f; // "Roulette": s between spins (0 = off)...
    float roulette = 0.f;      // ...red: + damage, black: + speed (green: both)
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
    float babyDragon = 0.f;    // "Baby dragon": its breath x the hit (0 = off)...
    float babyEvery = 0.f;     // ...every this many s
    // legendaries (2026-10-05)
    float titan = 0.f;         // "Titan": growth per core bounce (0 = off)...
    float titanQuake = 0.f, titanStun = 0.f;   // ...the quake at full size
    float archmage = 0.f;      // "Archmage": cast rate x this (0 = off)...
    float archStack = 0.f;     // ...+ ability damage per cast
    float timeStop = 0.f;      // "Time stop": s every enemy stands still (0 = off)
    float ion = 0.f;           // "Ion cannon": each jump keeps this of the hit (0 = off)...
    int ionJumps = 0;          // ...jumps at the base fire rate
    float gunship = 0.f;       // "Gunship": the other balls' bullet x their hit (0 = off)
    float leadStorm = 0.f;     // "Lead storm": s between storms (0 = off)
    float reaper = 0.f;        // "Reaper": the blink's cooldown (0 = off)
    float contract = 0.f;      // "Contract": share of max hp per hit (0 = off)...
    float contractBoss = 0.f;  // ...of a boss's
    float hive = 0.f;          // "Hive": a bite x its hit (0 = off)...
    int hivePer = 0;           // ...bugs per hatch
    float elderDragon = 0.f;   // "Elder dragon": its breath x the hit (0 = off)...
    float elderEvery = 0.f;    // ...every this many s
    bool rigged = false;       // "Rigged"
    float steady = 0.f;        // "Steady hand": time scale while aimed (0 = off)...
    float steadyBonus = 0.f;   // ...the throw's first hit + up to this
    float showtime = 0.f;      // "Showtime": per quick catch (0 = off)
    float toxic = 0.f;         // "Toxic trail": a puff's bite x the hit (0 = off)...
    float toxicLife = 0.f;     // ...how long a puff hangs
    float primal = 0.f;        // "Primal": power x this (0 = off)...
    float primalEvery = 0.f;   // ...every this many s
    float plague = 0.f;        // "Black plague": chance a hit infects (0 = off)...
    float plagueDps = 0.f;     // ...its sickness x the hit, per s
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
    int pingLeft = 0;          // "Ping-pong": aimed walls left this throw
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
    float slotT = 0.f;         // "777": to the next spin...
    float spinT = -1.f;        // ...s into the spin (-1 = still), the reels showing...
    int reel[3] = {0, 0, 0};   // ...symbols: 0 = 7, 1 = clover, 2 = coin
    float frenzyT = 0.f, rainT = 0.f, rainDropT = 0.f;   // ...a frenzy / a coin rain, s left
    float rouletteT = 0.f;     // "Roulette": to the next spin...
    float rouletteSpin = -1.f; // ...s into the spin (-1 = still)...
    int rouletteLand = -1;     // ...where it will land / landed: 0 red, 1 black, 2 green (-1 none yet)
    int rouletteWin = -1;      // ...the colour paying out now
    int luckyN = 0;            // "Lucky number": the roll showing (0 = none)...
    float luckyT = 0.f;        // ...to the next roll
    float luckyStopT = 0.f;    // ...a 1: stopped dead
    bool luckyArmed = false;   // ...a 1: the next hit x3
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
    float fireRateNow = 1.f;   // "Ion cannon": how fast it's firing right now (x its base)
    float stormT = 0.f;        // "Lead storm": to the next...
    float stormOn = 0.f;       // ...s left firing...
    float stormFireT = 0.f, stormAng = 0.f;
    int showStacks = 0;        // "Showtime": quick catches this fight
    float spiritT = 0.f;       // "Spirit host": to its next wisp
    float toxicT = 0.f;        // "Toxic trail": to the next puff...
    int toxicTurn = 0;         // ...whose element is next
    float primalT = 0.f;       // "Primal": to the next burst...
    float flameT = 0.f, flameTickT = 0.f;   // ...fire: the flamethrower burning
    sf::Vector2f flameDir{1.f, 0.f};
    int strikesLeft = 0;       // ...electric: bolts still to fall
    int quakesLeft = 0;        // ...stone: shocks still to come
    float quakeT = 0.f;
    float strikeT = 0.f;
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
    float leashT = 0.f, leashL = 0.f;    // "Leash": s of spin left (World::advanceLeash), rope length...
    float leashAng = 0.f, leashDir = 1.f; // ...where round the core it is, which way it turns...
    float leashFlyT = 0.f;                // ...then s of free flight that keeps its launch speed
    float leashFlyLen = 0.f;              // ...this long (set on the cast)
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
    float luckT = 0.f;         // "777" clovers: every chance x cfg::common::slotLuckMul while it runs
    std::vector<Cloud> trail;  // "Toxic trail" puffs
    float rainT = 0.f;         // "Primal" water: rain - enemies slower, balls quicker
    struct Bottle {            // "Primal" poison: thrown, it splashes where it lands
        sf::Vector2f from, to;
        float t = 0.f, dmg = 0.f;
        int owner = -1;
    };
    std::vector<Bottle> bottles;
    struct Field {             // "Primal": a lingering patch - frost (freezes) or a poison puddle
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, maxLife = 1.f, tickT = 0.f, tick = 0.5f, dmg = 0.f;
        int elem = 0, owner = -1;
    };
    std::vector<Field> fields;
    struct Boulder {           // "Primal" stone: a rock falling out of the sky
        sf::Vector2f at;
        float t = 0.f, dmg = 0.f;
        int owner = -1;
    };
    std::vector<Boulder> boulders;
    float rainDmg = 0.f, rainDropT = 0.f;   // "Primal" water: each drop's bite...
    int rainOwner = -1;                     // ...for this ball
    struct Coin {              // "777" coin rain: falls from the top, hits and pays
        sf::Vector2f pos, vel;
        float dmg = 0.f, spin = 0.f;
        int owner = -1;
    };
    std::vector<Coin> coins;
    // Summoner epics / legendaries (2026-10-05): dragons and the swarm.
    struct Dragon {            // "Baby dragon" (rides its ball) / "Elder dragon" (circles the arena)
        int owner = -1;
        bool elder = false;
        int side = 0;          // a baby's place round its ball (Twin call: two, opposite)
        sf::Vector2f pos, face{1.f, 0.f};
        float ang = 0.f;       // round the ball / the core
        float breathT = 0.f;   // s to the next breath
        float flap = 0.f;      // wing beat clock (drawn)
        float roar = 0.f;      // 1 -> 0 after a breath (drawn)
        float charge = -1.f;   // s into drawing breath (-1 = not), turning its head toward...
        int target = -1;       // ...this enemy (Enemy::id)
        sf::Vector2f head{1.f, 0.f};   // where its head (and the fire) points - the body keeps facing its flight
        float breathing = 0.f; // s of fire left in this breath...
        float tickT = 0.f;     // ...to its next bite
    };
    std::vector<Dragon> dragons;
    struct Bug {               // "Hive": a tiny biter
        sf::Vector2f pos, vel;
        float life = 0.f, biteT = 0.f, dmg = 0.f;
        int elem = 0, owner = -1;
        bool lit = false;      // a baby dragon breathed on it: its next bite explodes
    };
    std::vector<Bug> bugs;
    std::vector<float> hiveT;  // per real ball: s to its next hatch
    struct Fx {                // a cast's short flourish: drawn only (render/ClassRender.cpp)
        enum Kind { Hook, Smite, Warp, Swap, Implode, Shards, Meteor, Afterimage, Shock,
                    // items: each its own shape, so no two read alike
                    Slash, Clover, Petals, Coins, Reels, Boomerang, Bastion, Glyph, Cracks, Dust, Clock, Echo,
                    Star, Hexshield, Link, Sonic, Rune, Spokes, Pellets, Smoke, Wisps, Pillar, Blast, Fuse,
                    Confetti, Cards, Arrow, Rise, Slam, Spiral, Collapse, Sight, Ping, Beckon,
                    Mana, Leap, Saturate, Wild, Chrono, Scythe, Streak,
                    Breath, Swath, Frost, Rubble, Splash, Storm, Drop, Boulder, Ion, Card, Flip,
                    Execute, Glint, Twin };
        Kind kind = Shock;
        sf::Vector2f a, b;     // from / to (a hook's ball / its head; a swap's two ends...)
        float life = 0.f, maxLife = 1.f, r = 0.f;
        sf::Color color = sf::Color::White, color2 = sf::Color::White;
        int ball = -1;         // the ball (owner) `a` rides, -1 none
        int enemy = -1;        // the enemy (id) `b` rides, -1 none
    };
    std::vector<Fx> fx;
};

// On BallMods (`mods.cls`): what the ball's class items add up to.
// ==================================================================== Astronomer
// The Guardian's ascended class (2026-10-06): planets that orbit the core
// (sim/WorldPlanets.cpp, cfg::planet, drawn in render/ClassRender.cpp).
enum class PlanetKind { Mercury, Venus, Earth, Mars, Jupiter, Saturn, Neptune, Moon };
inline constexpr int kPlanetItems = 7;    // Mercury..Neptune are items; the Moon only comes as a bonus
inline constexpr int kPlanetKinds = 8;
struct AstronomerMods {
    int lvl[kPlanetItems] = {};   // each planet item's power (gearPower: 1 / 3 / 5), 0 = none
    int belt = 0;                 // "Asteroid belt" power, 0 = none
    int starry = 0;               // "Starry night" power, 0 = none
    int bonus = 0;                // random weaker planets each fight (an ascended Astronomer: 2)
    float size = 1.f;             // the class's own perk: planets x this
    bool any() const {
        for (int l : lvl)
            if (l > 0) return true;
        return belt > 0 || starry > 0 || bonus > 0;
    }
};
struct AstronomerWorld {
    struct Planet {
        PlanetKind kind = PlanetKind::Moon;
        int owner = -1;            // its ball
        float power = 1.f;         // its item's level (x), a bonus planet's weakness
        float ang = 0.f;           // round the core
        sf::Vector2f pos, prev;
        float r = 0.f;             // drawn / hit radius (px)
        float hitT = 0.f;          // s before it hits again
        float actT = 0.f;          // its own timer: Earth's next rocket, Jupiter's next puff, Saturn's ring regrowth
        float boostT = 0.f;        // Mercury: twice as fast while this runs
        bool rings = true;         // Saturn: its rings are on (they fly off on a core bounce)
        int moonsOut = 0;          // Mars: Phobos / Deimos away from it (thrown)
        float flareT = 0.f;        // Mercury: to its next trail flare
        bool bonus = false;        // a random one for this fight (Starry night, the ascension)
    };
    std::vector<Planet> planets;
    struct Rock {                  // "Asteroid belt"
        float ang = 0.f, rad = 0.f, size = 1.f;
        int owner = -1;
        float power = 1.f;
        float hitT = 0.f;
        float regrow = 0.f;        // > 0: flung, growing back
        sf::Vector2f pos;
    };
    std::vector<Rock> rocks;
    struct Halo {                  // Saturn's rings, settled round the core for a while: a wall that cuts and shoves out
        float r = 0.f, life = 0.f, maxLife = 1.f, dmg = 0.f, spin = 0.f;
        int owner = -1;
        int planet = -1;           // the Saturn they came from (its rings come home after)
        std::vector<std::pair<int, float>> cd;   // enemy id -> s before it's cut again
    };
    std::vector<Halo> halos;
    struct Triton {                // Neptune's moon, loosed backwards round the core: it freezes what it meets
        float ang = 0.f, life = 0.f, maxLife = 1.f, dmg = 0.f, orbit = 0.f, r = 0.f;
        sf::Vector2f pos;
        int owner = -1;
        std::vector<std::pair<int, float>> cd;
        sf::Vector2f trail[10];
        int trailN = 0;
    };
    std::vector<Triton> tritons;
    struct Puff {                  // Jupiter's gas: slows and hurts what's inside
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, maxLife = 1.f, dmg = 0.f, tickT = 0.f;
        int owner = -1;
    };
    std::vector<Puff> puffs;
    // What the planets throw - each its own kind, its own flight, its own look.
    struct Proj {
        enum Kind { Rocket, Moonlet, Meteor, Star } kind = Rocket;
        sf::Vector2f pos, vel, from, to;   // (a Meteor / Star: from where to where)
        float dmg = 0.f, life = 0.f, t = 0.f;
        int target = -1;           // Enemy::id it steers for (-1: the boss / straight on)
        int owner = -1;
        int home = -1;             // a Moonlet: the index of its Mars in `planets` (it flies back)
        bool back = false;         // a Moonlet on its way home
        int elem = 0;
    };
    std::vector<Proj> projs;
    struct Storm {                 // Venus's acid storm (rides the planet) / Jupiter's Great Red Spot (drifts to the crowd)
        bool spot = false;
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, maxLife = 1.f, dps = 0.f, tickT = 0.f, spin = 0.f, retarget = 0.f;
        sf::Vector2f goal;
        int owner = -1;
        int planet = -1;           // Venus: the planet it rides
    };
    std::vector<Storm> storms;
    struct Flare {                 // Mercury's trail while it races: it burns
        sf::Vector2f pos;
        float life = 0.f, r = 0.f, dps = 0.f;
        int owner = -1;
    };
    std::vector<Flare> flares;
    struct Boom {                  // a blast where something landed (drawn only)
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, maxLife = 1.f;
        int kind = 0;              // 0 rocket, 1 meteor, 2 star
    };
    std::vector<Boom> booms;
    unsigned sig = 0;              // what the planets were built from (rebuilt when it changes)
    int builtWave = -1;
};
struct AstronomerState {
    int growth = 0;           // the class: kills this fight - each one a little bigger (cfg::planet::growPer)
};

// ==================================================================== Hermes
// The Striker's god (2026-10-06, sim/WorldGods.cpp): speed. "Flying fast" =
// over cfg::hermes::fastAt x its cruise.
struct HermesMods {
    float talaria = 0.f;      // "Talaria": speed x (1 + this) per wall bounce (0 = off)
    float caduceus = 0.f;     // "Caduceus": fast, a hit also strikes the 2 nearest for this x it (0 = off)
    float messenger = 0.f;    // "Messenger": s between its runs to the farthest enemy and back (0 = off)...
    float messengerHit = 0.f; // ...hitting it for this x its hit
    float petasos = 0.f;      // "Petasos": fast, it passes through and booms for this x its hit (0 = off)
    float quicksilver = 0.f;  // "Quicksilver": fast, every enemy slower by this (0 = off)
};
struct HermesState {
    float messengerT = 0.f;   // to its next run
    float lastBoom = -9.f;    // "Petasos": the ball's age at its last sonic boom
    bool fast = false, golden = false;   // (drawn) flying fast; golden under Petasos
    int bounces = 0;          // bounces since it last touched the core (each one faster)
    float peak = 0.f;         // the fastest it has gone since: it never drops below (it only gains speed)
    float charge = 0.f;       // the message it carries: walls hit and time flown fast, delivered at the core
};
struct HermesWorld {
    struct Streak {           // a Messenger run / a Caduceus strike / a sonic boom (drawn)
        int kind = 0;         // 0 messenger, 1 caduceus, 2 boom
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f, r = 0.f;
    };
    std::vector<Streak> fx;
    float slowT = 0.f;        // "Quicksilver": enemies slowed while this runs...
    float slow = 0.f;         // ...by this
};

// ==================================================================== Odin
// The Mage's god (2026-10-06, sim/WorldGods.cpp): ravens, runes, a spear.
struct OdinMods {
    float huginn = 0.f;       // "Huginn": s between its triple casts - every ability 3 times running (0 = off)
    float muninn = 0.f;       // "Muninn": every cast also blasts the thickest pack for this x its hit (0 = off)
    float valknut = 0.f;      // "Valknut": + recharge, and the all-cast comes this much sooner (share)
    float gungnir = 0.f;      // "Gungnir": every few s a spear for this x its hit (0 = off)...
    float gungnirEvery = 0.f;
    float draupnir = 0.f;     // "Draupnir": on (only drawn here; its power is in the abilities)
};
struct OdinState {
    float allT = 0.f;         // the class: to its next volley (everything at once)
    int volleys = 0;          // volleys this fight ("Huginn": every 3rd, three times)
    float huginnT = 0.f, gungnirT = 0.f;
    int burst = 0;            // "Huginn": casts of everything still to come...
    float burstT = 0.f;       // ...the next in this long
    sf::Vector2f raven[2];    // the ravens (drawn): Huginn, Muninn
    float ravenFlash[2] = {};
    float circle = 0.f;       // 1 -> 0: the rune circle under it as it casts everything (drawn)
    int spells = 0;           // "Gungnir": spells cast since its last throw (one more spear each)
};
struct OdinWorld {
    struct Blast {            // "Muninn": an arcane blast on a pack (drawn; the damage is done)
        sf::Vector2f pos, from;
        float r = 0.f, life = 0.f, maxLife = 1.f;
    };
    std::vector<Blast> blasts;
    struct Spear {            // "Gungnir": out to its mark through the line, then home
        sf::Vector2f from, to, pos;
        float t = 0.f, dmg = 0.f;
        float delay = 0.f;    // still in its hand this long (a volley goes one after another)
        bool back = false;
        int owner = -1;
        std::vector<int> hit;
    };
    std::vector<Spear> spears;
    int wave = -1;            // the fight these belong to
};

// ==================================================================== Thanatos
// The Assassin's god (2026-10-06, sim/WorldGods2.cpp): it blinks onto an
// enemy and keeps stabbing it till it dies, then blinks to the next.
struct ThanatosMods {
    float shadowStep = 0.f;   // "Shadow step": each blink cuts what's on the way for this x its hit (0 = off)
    float deathMark = 0.f;    // "Death mark": a stabbed enemy under this share of its health dies (0 = off)
    int eclipse = 0;          // "Eclipse": every this-many-th blink, a shadow nova (0 = off)...
    float eclipseFrac = 0.f;
    int scythe = 0;           // "Reaper's scythe": each blink also strikes this many nearest (0 = off)
    float finalHour = 0.f;    // "Final hour": s between its blinks through every enemy (0 = off)
};
struct ThanatosState {
    int target = -1;          // the enemy (id) it's stabbing - kBossMark style -2 for the boss
    float stabT = 0.f;        // to its next stab
    int souls = 0;            // kills this fight: harder hits
    int blinks = 0;
    float hourT = 0.f;        // "Final hour": to the next...
    int hourLeft = 0;         // ...blinks left in this one
    float blinkCd = 0.f;      // s before it can blink again (free flight meanwhile)
    float sinceBlink = 9.f;   // s since its last blink (never closer than cfg::thanatos::blinkMin)
    bool attached = false;    // (it's on its mark: the sim holds it there)
    std::vector<int> chain;   // its dagger's way: the marks it has hit / is flying at (last), bouncing on - it stands still meanwhile
    std::vector<sf::Vector2f> chainPos;
    int hop = -1;             // then it blinks along them (>= 0: the next one), a stab on each; the last is its mark
    float hopT = 0.f;
    float throwT = 0.f, throwDur = 0.f;
    sf::Vector2f daggerFrom, daggerPos;
    sf::Vector2f side;        // which side of its mark it's on (the way its dagger came in)
};
struct ThanatosWorld {
    struct Streak {           // a blink (from, to), a stab, a nova (drawn)
        int kind = 0;         // 0 blink, 1 stab, 2 nova, 3 cut
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f, r = 0.f;
    };
    std::vector<Streak> fx;
};

// ==================================================================== Tiamat
// The Summoner's god (2026-10-06, sim/WorldGods2.cpp): she rides a dragon;
// her items put dragons in the field, each its own, and lizard armies.
struct TiamatMods {
    float frostWyrm = 0.f;    // each dragon: its power (0 = off)
    float stormDrake = 0.f;
    float ancientGold = 0.f;
    float devourer = 0.f;
    float wyrmling = 0.f;
    float horde = 0.f;        // "Lizard horde": s between its broods (0 = off)
    bool brood = false;       // "Brood mother": lizards bite twice as hard and breed on their kills
    float xp[6] = {};         // each dragon's growth so far this run (BallLoadout::dragonXp), by kind; [5] her mount
};
struct TiamatState {
    float breathT = 0.f;      // the mount: to its next fireball...
    float skillT = 0.f;       // ...its next move of its own...
    float clawT = 0.f;        // ...its next rake of the claws...
    float flameT = 0.f;       // ...its next breath of fire (a cone, between its moves)...
    float emberT = 0.f;       // ...and the next ember it drops
    float hordeT = 0.f;
    int stage = 0;            // the mount's stage last frame (1 hatchling, 2 young, 3 adult)
};
struct TiamatWorld {
    struct Dragon {
        int kind = 0;         // 0 frost wyrm, 1 storm drake, 2 ancient gold, 3 ravager, 4 wyrmling
        int owner = -1;
        float power = 1.f;
        sf::Vector2f pos, face{1.f, 0.f}, perch, from;
        float ang = 0.f, actT = 0.f, moveT = 0.f, dive = -1.f;   // (gold: -1 aloft, 0..1 diving)
        int target = -1;
        sf::Vector2f trail[14];
        int trailN = 0;
        float flash = 0.f;
        float trailT = 0.f;   // the drake's static: its next tick
        sf::Vector2f vel, goal;   // roaming the field: where it's headed
        float spitT = 0.f;    // to the next thing it spits (ice shards, molten gold, ball lightning)
        int head = 0;         // the three-headed gold: whose turn it is
        float headT[3] = {};  // ...each head's own clock
        sf::Vector2f headDir[3];          // ...where each looks (turning on its own)
        int headTgt[3] = {-1, -1, -1};    // ...the enemy each has picked
        float headPick[3] = {};           // ...till it picks another
        std::vector<int> struck;          // the ravager: hit on this run (dive >= 0: running from `from` to `perch`)
        float xp = 0.f;                   // its growth, all run long: an egg, then hatchling, young, adult (cfg::tiamat::stageXp)
        int stage = 0;
        float size = 0.f;                 // its body length now (px, before the arena scale)
        float goalT = 0.f;                // roaming: till it picks somewhere else
        float flap = 0.f;                 // its wingbeat (phase), and how long its glide still runs (> 0: wings still)
        float glide = 0.f;
        bool toGlide = false;             // ...it'll hold its wings out at the top of this beat
        int runSide = 1;                  // the ravager's run: which way it veers off first, and whether it's through the pack yet
        float throughT = -1.f;
        int chain = 0;                    // ...an adult's second run straight after
        float burst = 0.f;                // the wyrmling: its pounce after a kill
    };
    static constexpr int kMounts = 8;   // (>= cfg::maxBalls)
    float mountXp[kMounts] = {};        // each Tiamat ball's mount: how grown (by owner), all run
    std::vector<Dragon> dragons;
    struct Lizard {
        sf::Vector2f pos, vel;
        float life = 0.f, biteT = 0.f, dmg = 0.f, gait = 0.f;
        int owner = -1;
    };
    std::vector<Lizard> lizards;
    struct Fireball {         // spat from a dragon's mouth: it bursts where it lands
        sf::Vector2f from, to, pos;
        float t = 0.f, dur = 0.5f, dmg = 0.f, r = 0.f;
        int owner = -1;
        int kind = 0;         // 0 a fireball (arcs), 1 an ice shard (straight, fast), 2 molten gold (arcs, leaves a pool), 3 ball lightning (slow, zaps on its way)
        float tickT = 0.f;
    };
    std::vector<Fireball> fireballs;
    struct Field {            // a patch on the ground: embers (the mount's wake), frost (the wyrm's), molten gold, the inferno ring
        int kind = 0;         // 0 embers, 1 frost, 2 molten gold, 3 inferno (rides its ball), 4 a thunderstorm (strikes inside it)
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, maxLife = 1.f, dps = 0.f, tickT = 0.f;
        int owner = -1;
    };
    std::vector<Field> fields;
    struct Fx {               // a breath (cone), a bolt chain, a crash (drawn)
        int kind = 0;         // 0 fire breath, 1 ice breath, 2 lightning, 3 crash, 4 roar, 5 gust, 6 tail sweep, 7 fireball burst, 8 claws, 9 dive,
                              // 10 the gold's pulling roar, 11 (unused), 12 (unused), 13 a storm bolt's blast
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f, r = 0.f;
        std::vector<sf::Vector2f> pts;
    };
    std::vector<Fx> fx;
    unsigned sig = 0;
    int wave = -1;
};

// ==================================================================== Loki
// The Jester's god (2026-10-07, sim/WorldLoki.cpp): clones of it fly about
// playing games of chance - a coin always; its items add dice, a roulette,
// cards and a pachinko - and what comes up, happens.
struct LokiMods {
    float dice = 0.f, roulette = 0.f, cards = 0.f, pachinko = 0.f;   // each game: its payout (0 = it doesn't play it)
    float high = 0.f;          // "High table": every game x this more, more clones, sooner (0 = off)
};
struct LokiState {
    float cloneT = 0.f;        // to its next clone
};
struct LokiWorld {
    struct Clone {
        sf::Vector2f pos, vel;
        float life = 0.f, maxLife = 1.f;
        int owner = -1;
        float gameT = 0.f;     // to its next game...
        int game = -1;         // ...the one it's playing (0 coin, 1 dice, 2 roulette, 3 cards, 4 pachinko; -1 none)
        float show = 0.f, showMax = 1.f;   // ...still spinning / showing (it pays when this runs out)
        int a = 0, b = 0;      // ...what came up (coin 1 heads; dice the two; roulette 0 green 1 red 2 black; cards suit, rank)
    };
    std::vector<Clone> clones;
    struct Pin {               // a pachinko's steel ball, falling through the enemies
        sf::Vector2f pos, vel;
        float life = 0.f, dmg = 0.f, hitCd = 0.f;
        int owner = -1;
    };
    std::vector<Pin> pins;
    struct Fx {
        int kind = 0;          // 0 a bolt from the sky, 1 a fire nova, 2 a pull, 3 a jackpot, 4 a knife, 5 a slash, 6 a stun, 7 a mend, 8 a shard, 9 a clone's puff
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f, r = 0.f;
        int gift = 0;          // whose colour (arrows, arrivals)
    };
    std::vector<Fx> fx;
    int wave = -1;
};

// ==================================================================== Nergal
// The Alchemist's god (2026-10-07, sim/WorldNergal.cpp): plague - his hits
// infect (a rot: poison), the infected die into rats; blood rain, rat tides,
// locusts, the black death, the harvest.
struct NergalMods {
    float rainEvery = 0.f;     // "Blood rain": s between clouds (0 = off)
    float tideEvery = 0.f;     // "Rat tide": s between tides (0 = off)
    float locusts = 0.f;       // "Locust swarm": its trail's swarms x this (more, bigger, harder; 0 = plain swarms)
    float blackDeath = 0.f;    // "Black death": the burst x the hit (0 = off)
    float horseEvery = 0.f;    // "Pale horse": s between rides (0 = off)
};
struct NergalState {
    float rainT = 0.f, tideT = 0.f, horseT = 0.f, swarmT = 0.f;
};
struct NergalWorld {
    struct Rat {
        sf::Vector2f pos, vel;
        float life = 0.f, biteT = 0.f, gait = 0.f;
        int owner = -1;
    };
    std::vector<Rat> rats;
    struct Rain {              // a blood cloud over a pack, drifting after it (kind 1: a black miasma, the plague meeting water)
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, maxLife = 1.f, dps = 0.f, tickT = 0.f;
        int owner = -1;
        int kind = 0;
    };
    std::vector<Rain> rains;
    struct Swarm {             // its trail: a swarm of flies (locusts) hunting what's near, chewing and infecting
        sf::Vector2f pos, vel;
        float life = 0.f, maxLife = 1.f, r = 0.f, dps = 0.f, tickT = 0.f, seed = 0.f;
        int owner = -1;
    };
    std::vector<Swarm> swarms;
    struct Rider {             // "Pale horse": galloping across the arena
        sf::Vector2f from, to, pos;
        float t = 0.f, dur = 1.f, dmg = 0.f;
        int owner = -1;
        std::vector<int> struck;
    };
    std::vector<Rider> riders;
    struct Fx {
        int kind = 0;          // 0 a burst (black death), 1 (unused), 2 a bite, 3 a black bolt (a.. to b), 4 a grave opening
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f, r = 0.f;
        int gift = 0;          // whose colour (arrows, arrivals)
    };
    std::vector<Fx> fx;
    int wave = -1;
};

// ==================================================================== Apolo
// The Support's god (2026-10-07, sim/WorldApolo.cpp): four muses (five with
// the Sun chariot), one per ball, each lending a gift and marking a weak point;
// they rotate between the balls - never two gifts on one ball.
struct ApoloMods {
    float lyre = 0.f;          // "Lyre": core mended per rotation (0 = off); a lost gift lingers at half
    float laurel = 0.f;        // "Laurel crown": the gifts + this (0 = off)
    float bow = 0.f;           // "Silver bow": each arrow x the hit (0 = off)
    float chariot = 0.f;       // "Sun chariot": the sun's burn x the hit a second (0 = off)
    float oracle = 0.f;        // "Oracle": the arrival crit (0 = off); rotations twice as fast
};
struct ApoloState {            // on every ball: the muse's gift it has now (the sim sets it each frame)
    int gift = -1;             // 0 strength, 1 speed, 2 twin, 3 giant, 4 the sun (-1 none)
    float power = 1.f;
    int glow = -1;             // "Lyre": a lost gift lingering at half...
    float glowT = 0.f;         // ...this long
    float foresight = 0.f;     // "Oracle": the next hit x this (0 = none)
};
struct ApoloWorld {
    struct Muse {
        int gift = 0;
        sf::Vector2f pos;
        int ball = -1;         // the ball (owner) she's with (-1: waiting by the core)
        int mark = -1;         // the enemy (id) she's marked
        float arrowT = 0.f, sunT = 0.f;
    };
    std::vector<Muse> muses;
    int owner = -1;            // Apolo's ball
    int rot = 0;
    float rotT = 0.f;
    struct Fx {
        int kind = 0;          // 0 an arrow, 1 a chord at the core, 2 a muse arriving
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f, r = 0.f;
        int gift = 0;          // whose colour (arrows, arrivals)
    };
    std::vector<Fx> fx;
    int wave = -1;
};

// ==================================================================== Ares
// The Shooter's god (2026-10-07, sim/WorldAres.cpp): ghost cannons and
// bloodlust; each item one more weapon - Phobos & Deimos (missile moons),
// Dragon's teeth (bronze hoplites), War chariot, Wrath of Olympus (a falling
// red star), Trojan war (a rain of spears down a lane).
struct AresMods {
    float moons = 0.f, teeth = 0.f, chariot = 0.f, wrath = 0.f, troy = 0.f;   // each weapon's power (0 = off)
};
struct AresState {
    float fireT = 0.f;
    int cannon = 0;            // the next to fire
    float lust = 0.f, lustT = 0.f;   // bloodlust: + fire rate, and how long it's held
    float moonT[2] = {0.f, 0.5f}, teethT = 0.f, chariotT = 2.f, wrathT = 1.5f, troyT = 4.f;
    float kick[6] = {};        // each cannon's recoil (drawn)
    float aim = 0.f;           // where the cannons point (drawn)
};
struct AresWorld {
    struct Missile {           // Phobos & Deimos: homing, a blast, terror
        sf::Vector2f pos, vel;
        float dmg = 0.f, life = 0.f;
        int owner = -1, target = -1, moon = 0;
    };
    std::vector<Missile> missiles;
    struct Hoplite {           // Dragon's teeth: a bronze soldier sprung from the ground
        sf::Vector2f pos;
        float life = 0.f, maxLife = 1.f, sprout = 0.f, fireT = 0.f, dmg = 0.f, aim = 0.f;
        int owner = -1;
    };
    std::vector<Hoplite> hoplites;
    struct Chariot {           // War chariot: edge to edge through the pack, a furrow of fire behind
        sf::Vector2f from, to, pos;
        float t = 0.f, dur = 1.f, dmg = 0.f, flameT = 0.f, burn = 0.f;
        int owner = -1;
        std::vector<int> struck;
    };
    std::vector<Chariot> chariots;
    struct Flame {             // a burning patch (the chariot's furrow, a crater)
        sf::Vector2f pos;
        float r = 0.f, life = 0.f, maxLife = 1.f, dps = 0.f, tickT = 0.f;
        int owner = -1;
        bool crater = false;   // a fallen star's (Wrath of Olympus counts these)
    };
    std::vector<Flame> flames;
    struct Star {              // Wrath of Olympus: a closing ring, then the impact
        sf::Vector2f pos;
        float t = 0.f, r = 0.f, dmg = 0.f;
        int owner = -1;
    };
    std::vector<Star> stars;
    struct Lane {              // Trojan war: spears fall one after another along it
        sf::Vector2f from, to;
        float t = 0.f, dmg = 0.f;
        int fired = 0, owner = -1;
    };
    std::vector<Lane> lanes;
    struct Fx {
        int kind = 0;          // 0 a blast, 1 a muzzle flash, 2 a star's impact, 3 a spear landing, 4 a hoplite sprouting
        sf::Vector2f a, b;
        float life = 0.f, maxLife = 1.f, r = 0.f;
    };
    std::vector<Fx> fx;
    int wave = -1;
};

// ==================================================================== Artemisa
// The Slinger's god (2026-10-07, sim/WorldArtemisa.cpp): every throw (yours,
// or her own when left alone) looses a silver arrow and marks a prey; her
// items all fire on the throw or the catch.
struct ArtemisaMods {
    float hounds = 0.f, moon = 0.f, rain = 0.f, stag = 0.f, night = 0.f;   // each one's power (0 = off)
};
struct ArtemisaState {
    float autoT = 0.f;         // to her own throw
    int prey = -1;             // the enemy (id) her last arrow marked
    bool armed = false;        // the ball's first hit on its prey is still to come
    std::vector<sf::Vector2f> hits;   // where this throw has hit (Arrow rain, on the catch)
    float nightT = 0.f;        // to the next night
    std::vector<int> route;    // a night throw: the enemies still to run through
    int routeAt = 0;
    float routeDmg = 0.f;
    // a throw / a catch just happened (handled in World::updateArtemisa, which has the params)
    bool throwPending = false, rainPending = false;
    float pull = 0.f;          // how hard the pending throw was (1 = full)
    sf::Vector2f throwFrom, throwDir;
};
struct ArtemisaWorld {
    struct Arrow { sf::Vector2f a, b; float life = 0.f, maxLife = 1.f; };   // drawn: a silver streak down the line
    std::vector<Arrow> arrows;
    struct Hound {
        sf::Vector2f pos, vel;
        float life = 0.f, dmg = 0.f, gait = 0.f;
        int owner = -1, target = -1;
        std::vector<int> struck;
    };
    std::vector<Hound> hounds;
    struct Echo { sf::Vector2f pos, vel; float delay = 0.f; int owner = -1; };   // Crescent moon: a ball still to set off
    std::vector<Echo> echoes;
    struct Rain { sf::Vector2f pos; float t = 0.f, land = 0.f, dmg = 0.f; int owner = -1; };
    std::vector<Rain> rains;
    struct Stag {
        sf::Vector2f pos, dir;
        float life = 0.f, dmg = 0.f;
        int owner = -1;
        std::vector<int> struck;
    };
    std::vector<Stag> stags;
    float night = 0.f;         // s of night left (drawn: the arena dims)
    struct Fx { int kind = 0; sf::Vector2f a, b; float life = 0.f, maxLife = 1.f, r = 0.f; };   // 0 a bite, 1 a rain landing, 2 a night strike
    std::vector<Fx> fx;
    int wave = -1;
};

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
    AstronomerMods astronomer;
    HermesMods hermes;
    OdinMods odin;
    ThanatosMods thanatos;
    TiamatMods tiamat;
    LokiMods loki;
    NergalMods nergal;
    ApoloMods apolo;
    AresMods ares;
    ArtemisaMods artemisa;
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
    AstronomerState astronomer;
    HermesState hermes;
    OdinState odin;
    ThanatosState thanatos;
    TiamatState tiamat;
    LokiState loki;
    NergalState nergal;
    ApoloState apolo;
    AresState ares;
    ArtemisaState artemisa;
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
    AstronomerWorld astronomer;
    HermesWorld hermes;
    OdinWorld odin;
    ThanatosWorld thanatos;
    TiamatWorld tiamat;
    LokiWorld loki;
    NergalWorld nergal;
    ApoloWorld apolo;
    AresWorld ares;
    ArtemisaWorld artemisa;
    CommonWorld common;
};

}  // namespace sb
