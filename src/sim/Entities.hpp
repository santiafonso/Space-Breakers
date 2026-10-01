#pragma once

#include <algorithm>
#include <deque>
#include <vector>

#include "core/Config.hpp"
#include "core/Math.hpp"
#include "core/Theme.hpp"
#include "sim/Classes.hpp"
#include "sim/CreedRules.hpp"
#include "sim/PactRules.hpp"

namespace sb {

// ---------------------------------------------------------------- power-ups

// Points2x / Surge drop from the start; SlowMo / Golden / Overdrive are gated by
// web nodes (see App::powerUpMask). Order is the bit index in WorldParams.powerUpMask.
enum class PowerUp { Points2x, SlowMo, Surge, Golden, Overdrive };
inline constexpr int kPowerUpCount = 5;

const char* powerUpName(PowerUp p);
const char* powerUpDesc(PowerUp p);
sf::Color powerUpColor(PowerUp p);
float powerUpDuration(PowerUp p);

// ---------------------------------------------------------------- ball elements

// Order matches the unlock chain; (value - 1) indexes WorldParams::elemMult and
// the element web-node slots (Plain aside).
enum class Element { Plain, Fire, Poison, Water, Ice, Stone, Electric };
inline constexpr int kElementCount = 7;

const char* elementName(Element e);
sf::Color elementColor(Element e);

// ---------------------------------------------------------------- ball classes

// A ball's classes ("roles") come from its item tags: 2 items of a tag give it
// that class, so with 4 item slots a ball has 0, 1 or 2 of them; 4 items of
// one tag make it the ASCENDED form of that class. Normal = no class. The
// order matches ItemTag (progression/Offers.hpp). Striker: worth flinging;
// Guardian: big, shoves, staggers, aims its bounces; Support: marks enemies +
// a stronger element (see cfg::role). The other five are built per class in
// sim/Classes.hpp + sim/WorldClasses.cpp.
enum class BallRole { Normal, Striker, Guardian, Support, Mage, Shooter, Assassin, Summoner, Jester, Slinger, Alchemist };
inline constexpr int kBallRoleCount = 11;
inline constexpr int kClassCount = 10;   // every role but Normal
inline constexpr int kMaxElements = 3;   // elements one ball can carry (the Alchemist: 2, its ascended form 3)

// A set of classes, one bit per BallRole (Normal has no bit).
using RoleMask = unsigned;
inline constexpr RoleMask roleBit(BallRole r) {
    return r == BallRole::Normal ? 0u : 1u << static_cast<unsigned>(r);
}
inline constexpr BallRole classAt(int i) { return static_cast<BallRole>(i + 1); }   // i = 0..kClassCount-1

const char* roleName(BallRole r);       // "Striker"
const char* roleDesc(BallRole r);       // what the class does
const char* ascendedName(BallRole r);   // "Mega Striker"
const char* ascendedDesc(BallRole r);   // what 4 items of the tag add
// A class's colour - the vivid one: it's the ball's body colour, its name in
// the UI, its items' tag. Normal = a neutral grey.
sf::Color roleColor(BallRole r);

// ---------------------------------------------------------------- abilities

// Timed actives in a ball's ability slot(s): each fires on its own when its
// cooldown is up (and it has something to act on). They don't count toward a
// class. Levelled by picking the same one again (cfg::ability).
enum class Ability { None, Dash, Nova, Split, Bulwark, Overclock, Arc, Meteor, MagicMissile };
inline constexpr int kAbilityCount = 9;        // with None
inline constexpr int kMaxAbilitySlots = 3;     // 1 by default, the Mage raises it (abilitySlotCount)

struct AbilitySpec {
    Ability id = Ability::None;
    int level = 0;
};

const char* abilityName(Ability a);
float abilityCooldown(Ability a, int level);   // seconds between two firings

// What a ball's equipped gear adds up to. Built by App from the run loadout and
// pushed into the World (World::syncBalls) - the sim never sees gear kinds.
// Item levels are already folded into these numbers; 0 means "not equipped"
// for the per-item values.
struct BallMods {
    float damageMult = 1.f;    // Heavy impact, item levels, Giant
    float radiusMult = 1.f;    // Big ball, Giant, Bumper
    float cruiseMult = 1.f;    // Swift, Giant
    float flingDecay = 1.f;    // Swift (< 1 keeps a fling's speed longer), Comet
    float maxSpeedMult = 1.f;  // Swift, Comet
    float knockMult = 1.f;     // Big ball, Bumper
    float elemMult = 1.f;      // element item level
    float copyLife = 1.f;      // web "Brood": its ghost copies (Split shot, Split, Mitosis, Phantom) last x this
    // modifiers (2026-09-28)
    float reach = 1.f;         // "Reach": grabbed from this x further
    float spin = 0.f;          // "Spin": chance a hit climbs the combo an extra step
    float leech = 0.f;         // "Leech": core hp per kill
    float heavyThrow = 0.f;    // "Heavy throw": the thrown first hit + this
    float bouncy = 1.f;        // "Bouncy": speed x this off the core
    // items
    float ricochetMult = 0.f;  // Ricochet: armed-hit damage x this...
    float wallBoost = 1.f;     // ...and speed x this per wall bounce
    bool cleave = false;
    float cleaveExec = 0.f;    // Cleave Lv2+: also finishes enemies left under this HP fraction
    float critChance = 0.f;    // Keen eye
    float critMult = 2.f;
    float executeThreshold = 0.f;   // Executioner
    float executeMult = 1.f;
    float overkillFrac = 0.f;  // Overkill
    int overkillTargets = 0;
    float shatterMult = 0.f;   // Shatter
    int conductorJumps = 0;    // only acts on an electric ball
    float bedrockLife = 0.f;   // only acts on a stone ball
    float echoChance = 0.f;    // Echo: chance a hit strikes twice
    float teslaChance = 0.f;   // Tesla: chance a hit zaps nearby enemies...
    int teslaTargets = 0;      // ...this many
    float bomberChance = 0.f;  // Bomber: chance a kill explodes...
    float bombRadius = 0.f;    // ...this wide
    float splitChance = 0.f;   // Split shot: chance a wall bounce spawns a ghost copy
    float rampartKnock = 0.f;  // Rampart: knockback x this...
    float rampartStagger = 0.f;// ...stagger x this
    float menderHeal = 0.f;    // Mender: core hp per core bounce
    // behaviour items (Fase M)
    float hunterMult = 0.f;    // Hunter: hits on its prey x this
    float hunterTurn = 0.f;
    float cometFling = 0.f;    // Comet: throw x this
    float cometPlow = 0.f;     // ...passes through enemies above this x cruise
    int mitosis = 0;           // Mitosis: copies per kill
    float mitosisLife = 0.f;
    float boomerangHit = 0.f;  // Boomerang: charged hit x this...
    float boomerangKick = 1.f; // ...speed x this leaving the core
    float bumperBoost = 0.f;   // Bumper: balls clacking off it x this speed
    float gluttonDamage = 0.f; // Glutton: + damage per kill-stack...
    int gluttonMax = 0;        // ...up to this many stacks
    float tetherFrac = 0.f;    // Tether: beam damage/s x the ball's hit...
    float tetherWidth = 0.f;   // ...half-width
    float blackHoleChance = 0.f;   // Black hole: chance a kill leaves one...
    float blackHoleFrac = 0.f;     // ...burst x the killing hit
    float blackHolePull = 1.f;     // ...pull x this
    float resonanceFrac = 0.f;     // Resonance: zap x the hit...
    float resonanceCd = 0.f;       // ...at most this often
    // game-changers (Fase J)
    float seekerTurn = 0.f;    // Seeker: rad/s (0 = off)
    float seekerRange = 0.f;
    bool piercing = false;
    float pierceMult = 1.f;
    float railFrac = 0.f;      // Railgun (0 = off)
    float railWidth = 0.f;
    float berserkPerHit = 0.f; // Berserk (0 = off)
    int berserkMax = 0;
    bool satellite = false;
    float satelliteDamage = 1.f;
    float gravityMult = 0.f;   // Gravity well: pull x this (0 = off)
    float stormFrac = 0.f;     // Storm (0 = off)
    float stormInterval = 0.f;
    int twins = 0;             // Gemini: ghost twins
    int midasGold = 0;         // Midas: extra gold per kill
    ClassMods cls;             // the five newer classes' item numbers (sim/Classes.hpp)
};

// Everything the World needs to build / refresh one ball.
struct BallSpec {
    RoleMask roles = 0;        // classes it has (2 items of a tag each)
    BallRole primary = BallRole::Normal;   // its first class (slot order): the body colour
    RoleMask ascended = 0;     // classes it has in ascended form (4 items of a tag)
    Element element = Element::Plain;
    Element elems[kMaxElements] = {};   // every element it carries (elems[0] = element); an Alchemist holds more
    int elemN = 0;
    BallMods mods;
    AbilitySpec abilities[kMaxAbilitySlots];   // active ability slots only
};

// ---------------------------------------------------------------- entities

struct Ball {
    sf::Vector2f pos;  // centre
    sf::Vector2f vel;
    float radius = cfg::ball::radius;
    bool held = false;
    int snaredBy = -1;       // a Snare's id while one holds it (it can't move or be grabbed)
    RoleMask roles = 0;      // its classes (see BallRole)
    RoleMask ascended = 0;   // ...and which of them are ascended
    BallRole primary = BallRole::Normal;   // its first class (see leadRole)
    float classPulse = 0.f;  // 1 -> 0: it just gained a class, a flare in its class colour
    float elemPulse = 0.f;   // 1 -> 0: it just took an element, rings in the element's colour
    bool pulseAscend = false;   // ...and that gain was the ascended form (a bigger flare)
    Element element = Element::Plain;   // the element of its next hit (an Alchemist's turns over hit by hit)
    Element elems[kMaxElements] = {};   // every element it carries; elems[0] is its own
    int elemN = 0;
    int elemTurn = 0;                   // Alchemist: whose turn it is
    bool hasElement(Element e) const {
        for (int i = 0; i < elemN; ++i)
            if (elems[i] == e) return true;
        return false;
    }
    BallMods mods;          // this ball's items + modifiers
    float cooldown = 0.f;   // water wake timer
    float zapT = 0.f;       // electric zap timer
    float squash = 0.f;
    sf::Vector2f squashAxis{1.f, 0.f};
    sf::Color color = theme::ballSlow;
    float ricochetT = 0.f;   // "Ricochet": seconds of post-wall-bounce damage bonus left
    float catchBonus = 0.f;  // catch reward: the next hit x (1 + this) (0 = unarmed)...
    float catchT = 0.f;      // ...for this many more seconds
    float sinceThrow = 0.f;  // "Quick Hands" pact: seconds since you last threw it
    int juggle = 0;          // "Juggler" pact: catches in a row without touching the core
    std::deque<sf::Vector2f> trail;
    std::deque<sf::Vector2f> waterTrail;   // (unused since water sends waves - kept for the copies' clears)
    int owner = -1;          // index of the (real) ball this is / was copied from - reactions need two owners
    bool ghost = false;      // "Split shot" / "Mitosis" copy: temporary, fades out
    float ghostLife = 0.f;
    float scale = 1.f;       // body size x this ("Mitosis" copies are small)
    bool twin = false;       // "Gemini": a permanent ghost that follows its parent's items
    int twinIdx = 0;         // which of its parent's twins
    int berserkStacks = 0;   // "Berserk": hits in a row since the last wall
    float stormT = 0.f;      // "Storm": time to the next zap
    float orbitAng = 0.f;    // "Satellite": angle around the core...
    float orbitR = 0.f;      // ...and its current radius (0 = not set yet)
    float age = 0.f;         // seconds since it appeared (spawn pop-in)
    int preyId = -1;         // "Hunter": the enemy it's locked on (Enemy::id)
    bool homing = false;     // "Boomerang": flying home to the core
    bool charged = false;    // "Boomerang": next enemy hit is the big one
    int gluttonStacks = 0;   // "Glutton": kills this wave
    float tetherT = 0.f;     // "Tether": time to the next damage tick
    float resonanceT = 0.f;  // "Resonance": cooldown
    float creedCharge = 0.f;  // "Living Core" creed: seconds left overcharged after a core bounce
    // abilities (sim/WorldAbilities.cpp)
    AbilitySpec abilities[kMaxAbilitySlots];
    float abilityCd[kMaxAbilitySlots] = {};   // seconds until each can fire again
    float abilityFlash = 0.f;                 // 1 when one just fired, fades (the cooldown arc flashes)
    float overclockT = 0.f;                   // "Overclock": seconds left hot...
    float overclockMul = 1.f;                 // ...hitting this much harder
    ClassState cls;          // per-class runtime state (sim/Classes.hpp)

    bool hasRole(BallRole r) const { return (roles & roleBit(r)) != 0; }
    bool isAscended(BallRole r) const { return (ascended & roleBit(r)) != 0; }
    // Its identity class (body colour): `primary` while it still has it, else
    // the first it has, else Normal. secondRole() = the other one (dual role).
    BallRole leadRole() const {
        if (hasRole(primary)) return primary;
        for (int i = 0; i < kClassCount; ++i)
            if (hasRole(classAt(i))) return classAt(i);
        return BallRole::Normal;
    }
    BallRole secondRole() const {
        const BallRole lead = leadRole();
        for (int i = 0; i < kClassCount; ++i)
            if (classAt(i) != lead && hasRole(classAt(i))) return classAt(i);
        return BallRole::Normal;
    }
};

// A ball's identity colour, speed aside: its lead class's colour, or a neutral
// grey for a classless ball (HUD dots, tethers, sights).
sf::Color ballHue(const Ball& b);

// Grunt = the plain walker. The rest each want a different answer (cfg::enemy).
// Brute is the miniboss: elite fights bring one (cfg::enemy).
enum class EnemyKind { Grunt, Runner, Tank, Splitter, Shard, Shielded, Blinker, Mender, Brute, Snare };

// How an enemy walks to the core: straight, zig-zagging across its line, or
// circling the core as it closes in (World::rollGait).
enum class Gait { Straight, Weave, Spiral };

const char* enemyName(EnemyKind k);
const char* enemyDesc(EnemyKind k);

// An enemy walks straight at the core. Balls damage it on contact.
struct Enemy {
    int id = 0;             // unique per run ("Hunter" locks on by id)
    EnemyKind kind = EnemyKind::Grunt;
    float knockTaken = 1.f;                     // Tank: barely moves when hit
    float coreDamage = cfg::core::enemyDamage;  // hp the core loses if it arrives
    sf::Vector2f pos;
    sf::Vector2f vel;
    float radius = cfg::wave::enemyRadius;
    float hp = 3.f;
    float maxHp = 3.f;
    float speed = 55.f;
    float hitFlash = 0.f;
    float poison = 0.f;     // seconds of poison remaining (from a poison ball)
    float poisonDps = 0.f;  // current poison damage/s, stacks up on each hit
    float frozen = 0.f;     // seconds left frozen in place (from an ice ball)
    float burn = 0.f;       // seconds of burn remaining ("Ember": fire ball DoT)
    float burnDps = 0.f;    // current burn damage/s while it lasts
    float soak = 0.f;       // seconds left soaked by water (slower, knocked further, frozen longer)
    int cracks = 0;         // stone cracks: takes more from every hit...
    float crackT = 0.f;     // ...until this runs out
    float mark = 0.f;       // seconds left marked by a Support ball (takes more damage)
    float brittle = 0.f;    // seconds left brittle (Superconductor reaction): takes more damage
    Element elem = Element::Plain;   // last element a ball left on it, waiting for a reaction
    int elemOwner = -1;              // which ball left it
    float elemT = 0.f;               // how long it keeps waiting
    float pierceCd = 0.f;            // Piercing / Satellite balls can't re-hit it until this runs out
    float age = 0.f;                 // seconds alive (drives the spawn pop-in)
    float stagger = 0.f;    // seconds left staggered by a Guardian (drifts, doesn't advance)
    bool orbiter = false;   // wave-20 shield: orbits the boss instead of seeking the core
    float orbitPhase = 0.f; // its slot angle on the ring
    float blinkT = 0.f;     // Blinker: seconds to its next jump
    float blinkFx = 0.f;    // Blinker: 1 right after a jump, fades (the render's flicker)
    sf::Vector2f blinkFrom; // Blinker: where it jumped from (a fading afterimage)
    bool snaring = false;   // Snare: it's holding a ball (until it dies)
    Gait gait = Gait::Straight;
    float gaitPhase = 0.f;  // Weave: where in its sway it started
    float gaitSign = 1.f;   // Weave / Spiral: which way it leans
};

// An electric ball's arc: a brief line from the ball to the enemy it zapped.
// Damage lands when it is spawned; this is only the fading visual.
struct Bolt {
    sf::Vector2f a;
    sf::Vector2f b;
    float life = cfg::element::boltLife;
    float maxLife = cfg::element::boltLife;
    bool beam = false;   // a Railgun beam: straight and thick instead of a jagged arc
};

// Rubble: enemies are pushed out of it and take chip damage. (Stone used to
// drop it; unused for now.)
struct Obstacle {
    sf::Vector2f pos;
    float radius = cfg::element::obstacleRadius;
    float life = cfg::element::obstacleLife;
    float maxLife = cfg::element::obstacleLife;
};

// A water ball's wave: an arc rolling out from where it was sent, growing
// wider, shoving and soaking each enemy it crosses once.
struct Wave {
    sf::Vector2f origin;
    float dir = 0.f;        // heading (rad)
    float r = 0.f;          // how far it has rolled
    float reach = 300.f;    // ...and how far it goes
    float push = 0.f;       // knock on each enemy it crosses
    float hitDmg = 0.f;     // the ball's hit, for reactions
    int owner = -1;
    std::vector<int> hit;   // Enemy::ids it already crossed
};

// A patch of ground left by a stone reaction: lava burns, mud slows, toxic
// dust poisons whatever stands in it.
enum class PoolKind { Lava, Mud, Toxic };
struct Pool {
    sf::Vector2f pos;
    float radius = 60.f;
    float life = 1.f, maxLife = 1.f;
    float power = 0.f;      // lava dps / toxic dps
    PoolKind kind = PoolKind::Lava;
};

// "Black hole": left where a kill landed. Pulls enemies in, then bursts with
// the element of the ball that made it.
struct BlackHole {
    sf::Vector2f pos;
    float life = cfg::changer::blackHoleLife;
    float pull = 1.f;       // pull strength x this (item level)
    float dmg = 0.f;        // burst damage
    Element elem = Element::Plain;
    int owner = -1;
};

// "Tether": this step's laser between a ball and its partner (visual; the
// damage is dealt by World).
struct TetherBeam {
    sf::Vector2f a;
    sf::Vector2f b;
    float width = 8.f;
    sf::Color color;
};

// The thing you defend.
struct Core {
    sf::Vector2f pos;
    float radius = cfg::core::radius;
    float hp = cfg::core::baseHp;
    float maxHp = cfg::core::baseHp;
    float hitFlash = 0.f;
};

// One boss per act, all sharing this struct:
//  - Charger  (act 1): walks dead straight at the core from the right.
//  - Hive     (act 2): drifts in on a sway, bursting fans of runners.
//  - Warden   (act 3): a turning shield arc blocks balls on one side; walks, plants, walks.
//  - Dasher   (act 4): stalks, telegraphs a line, dashes; every hit knocks it back.
//  - Orbital  (act 5): spirals in toward the core behind a spinning ring of
//    shield enemies; the ring is the real problem.
// Any of them touching the core loses the run outright.
enum class BossKind { Charger, Hive, Warden, Dasher, Orbital };
const char* bossName(BossKind k);
const char* bossDesc(BossKind k);
inline BossKind bossOfAct(int act) {   // act 1..cfg::run::acts
    static const BossKind kinds[] = {BossKind::Charger, BossKind::Hive, BossKind::Warden, BossKind::Dasher,
                                     BossKind::Orbital};
    return kinds[std::clamp(act, 1, 5) - 1];
}

struct Boss {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float radius = cfg::boss::radius;
    float hp = cfg::boss::hp;
    float maxHp = cfg::boss::hp;
    float hitFlash = 0.f;
    bool alive = false;
    BossKind kind = BossKind::Charger;
    float ang = 0.f;          // Orbital: current angle around the core
    float dist = 0.f;         // Orbital: current distance from the core
    float ringAng = 0.f;      // Orbital: shield-ring rotation
    float shieldTimer = 0.f;  // Orbital: countdown to the next orbiter refill
    float intro = 0.f;        // Orbital: >0 while sliding in from the edge (invulnerable, no spiral yet)
    float hitCd = 0.f;        // i-frames after a ball lands, so it can't be melted in place
    float timer = 0.f;        // Hive: to the next burst; Warden / Dasher: to the next phase
    int phase = 0;            // Warden: 0 walk / 1 plant; Dasher: 0 stalk / 1 aim / 2 dash
    float shieldAng = 0.f;    // Warden: centre of its shield arc
    sf::Vector2f dashDir{-1.f, 0.f};   // Dasher: the line it's aiming / dashing along
    bool enraged = false;     // below cfg::boss::enrageAt: faster everything
    int summons = 0;          // Brutes it has called in (cfg::boss::summonAt)
    float shockR = 0.f;       // Charger: its shockwave's reach (world units; the render's warning ring)
    float pace() const { return enraged ? cfg::boss::enragePace : 1.f; }
};

struct Pickup {
    sf::Vector2f pos;
    sf::Vector2f vel;
    PowerUp kind = PowerUp::Points2x;
    float radius = cfg::pickup::radius;
    float age = 0.f;
    float ttl = cfg::pickup::ttl;
};

struct ActiveEffect {
    PowerUp kind = PowerUp::Points2x;
    float remaining = 0.f;
    float duration = 1.f;
};

// ---------------------------------------------------------------- frame I/O

struct BounceFx {
    sf::Vector2f pos;
    sf::Vector2f normal;
    float speed = 0.f;
    sf::Color color;
    bool ballPair = false;   // ball-vs-ball clack (vs a wall / core / enemy impact)
};

// A burst on the field: a reaction, an explosion, an ascended pulse, an ability. `label` is
// set for element reactions (shown as a small floating word).
struct BurstFx {
    sf::Vector2f pos;
    float radius = 80.f;
    sf::Color color;
    const char* label = nullptr;
};

struct FrameEvents {
    std::vector<BurstFx> bursts;
    std::vector<BounceFx> bounces;
    std::vector<sf::Vector2f> kills;
    int comboTier = 0;
    bool comboTierUp = false;
    bool gotPickup = false;
    PowerUp pickupKind = PowerUp::Points2x;
    bool coreHit = false;
    bool shieldBlock = false;             // a hit bounced off a Shielded enemy's shield
    bool bossEnraged = false;             // the boss just entered its second phase
    bool bossSummon = false;              // the boss just called in a Brute
    bool bossShock = false;               // the Charger's shockwave went off
    bool launched = false;                // the fight-opening whirl just let the balls fly
    bool autoFlung = false;               // the "Clockwork" creed launched a ball
    int midasGold = 0;                    // extra gold from kills by Midas balls
    bool phoenix = false;                 // the Phoenix relic just saved the core
    bool bossHit = false;                 // a ball landed on the miniboss this step
    bool waveCleared = false;
    bool runOver = false;
};

// Per-step tuning handed to the simulation: the wave number plus whatever
// between-wave upgrades the player has picked this run.
struct WorldParams {
    bool hard = false;            // hard mode (cfg::hard)
    float damageMult = 1.f;       // Heft (web): every ball
    float cruiseMult = 1.f;
    int wave = 1;
    float ballRadiusMult = 1.f;   // Mass (web): every ball
    float coreBounceBoost = 1.f;  // Spring core (relic)
    bool slowField = false;       // Slow field (relic): a zone around the core slows enemies
    bool contagion = false;       // Contagion (relic): a poisoned enemy dying re-poisons nearby
    bool primed = false;          // Primed (relic): +damage vs enemies already under an element effect
    bool catalyst = false;        // Catalyst (relic): reactions harder + wider
    bool chainReaction = false;   // Chain reaction (relic): reactions can cascade
    bool magneticCore = false;    // Magnetic core (relic): core bounces aim at the nearest enemy
    float luck = 1.f;             // Lucky clover (relic): every chance x this
    bool prismCore = false;       // Prism core (relic): elementless balls leave a random element
    bool timeDilation = false;    // Time dilation (relic): enemies move slower
    bool overcharge = false;      // Overcharge (relic): higher combo cap
    float elemMult[kElementCount] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f};  // per-element potency (web levels)
    int  emberLevel = 0;          // Ember web node: fire hits apply a burn DoT

    // Meta web (Fase A).
    int  aegisHits = 0;           // Aegis: core ignores this many hits at the start of each wave
    float coreRegenPerSec = 0.f;  // Regen: core heals this fast during a wave
    bool magnetPickups = false;   // Magnet: power-up orbs drift toward the nearest ball
    int  afterglowLevel = 0;      // Afterglow: continuous buffs fade out instead of cutting
    float chargedFrac = 0.f;      // Charged: power-ups start with + this fraction of duration

    unsigned powerUpMask = 0xffffffffu;  // bit i set => PowerUp(i) can drop
    float pickupSpawnMult = 1.f;  // scales the gap between power-ups (< 1 = more often)
    float pickupDurMult = 1.f;    // scales how long a power-up lasts
    float markMul = 1.35f;        // any hit vs a Support-marked enemy x this (cfg::role::markDamageMul + web "Rally")

    CreedRules creed;               // the run's creeds (sim/CreedRules.hpp); defaults = none
    PactRules pact;                 // the run's pacts (sim/PactRules.hpp); defaults = none
};

}  // namespace sb
