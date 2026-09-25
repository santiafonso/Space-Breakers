#pragma once

#include <deque>
#include <vector>

#include "core/Config.hpp"
#include "core/Math.hpp"
#include "core/Theme.hpp"

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

// ---------------------------------------------------------------- ball roles

// Every ball starts Normal; a ROLE pick turns it into Striker (worth
// flinging), Support (marks enemies + stronger element) or Guardian (big,
// shoves, staggers, aims its bounces) - see cfg::role.
enum class BallRole { Normal, Striker, Support, Guardian };
inline constexpr int kBallRoleCount = 4;

const char* roleName(BallRole r);
const char* roleDesc(BallRole r);

// What a ball's equipped gear adds up to. Built by App from the run loadout and
// pushed into the World (World::syncBalls) - the sim never sees gear kinds.
struct BallMods {
    float damageMult = 1.f;    // Heavy impact
    float radiusMult = 1.f;    // Big ball
    float cruiseMult = 1.f;    // Swift
    float wallBoost = 1.f;     // Wall rush: speed x this per wall bounce
    float pairBoost = 1.f;     // Carom: speed x this per ball-vs-ball clack
    float flingDecay = 1.f;    // Reflexes (< 1 keeps a fling's speed longer)
    float maxSpeedMult = 1.f;  // Ceiling break
    float knockMult = 1.f;     // Heavy knock
    float critChance = 0.f;    // Keen eye
    bool ricochet = false;
    bool warmUp = false;
    bool cleave = false;
    bool bruiser = false;
    bool executioner = false;
    bool overkill = false;
    bool tempo = false;
    bool shatter = false;
    bool conductor = false;    // only acts on an electric ball
    bool bedrock = false;      // only acts on a stone ball
    bool echo = false;         // chance a hit strikes twice
    bool tesla = false;        // chance a hit zaps nearby enemies
    bool bomber = false;       // chance a kill explodes
    bool splitShot = false;    // chance a wall bounce spawns a ghost copy
    bool rampart = false;      // hits shove further, stagger longer
    bool mender = false;       // core bounces repair the core
    bool mastery = false;      // 4 items of its role's tag
};

// Everything the World needs to build / refresh one ball.
struct BallSpec {
    BallRole role = BallRole::Normal;
    Element element = Element::Plain;
    BallMods mods;
};

// ---------------------------------------------------------------- entities

struct Ball {
    sf::Vector2f pos;  // centre
    sf::Vector2f vel;
    float radius = cfg::ball::radius;
    bool held = false;
    BallRole role = BallRole::Normal;
    Element element = Element::Plain;
    BallMods mods;          // this ball's items + modifiers
    float cooldown = 0.f;   // water drip / stone drop / electric zap timer
    float squash = 0.f;
    sf::Vector2f squashAxis{1.f, 0.f};
    sf::Color color = theme::ballSlow;
    float ricochetT = 0.f;   // "Ricochet": seconds of post-wall-bounce damage bonus left
    std::deque<sf::Vector2f> trail;
    std::deque<sf::Vector2f> waterTrail;   // water ball only: the damaging "worm" wake
    int owner = -1;          // index of the (real) ball this is / was copied from - reactions need two owners
    bool ghost = false;      // "Split shot" copy: temporary, fades out
    float ghostLife = 0.f;
};

// Grunt = the plain walker. The rest each want a different answer (cfg::enemy).
enum class EnemyKind { Grunt, Runner, Tank, Splitter, Shard, Shielded };

const char* enemyName(EnemyKind k);
const char* enemyDesc(EnemyKind k);

// An enemy walks straight at the core. Balls damage it on contact.
struct Enemy {
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
    float mark = 0.f;       // seconds left marked by a Support ball (takes more damage)
    float brittle = 0.f;    // seconds left brittle (Superconductor reaction): takes more damage
    Element elem = Element::Plain;   // last element a ball left on it, waiting for a reaction
    int elemOwner = -1;              // which ball left it
    float elemT = 0.f;               // how long it keeps waiting
    float stagger = 0.f;    // seconds left staggered by a Guardian (drifts, doesn't advance)
    bool orbiter = false;   // wave-20 shield: orbits the boss instead of seeking the core
    float orbitPhase = 0.f; // its slot angle on the ring
};

// An electric ball's arc: a brief line from the ball to the enemy it zapped.
// Damage lands when it is spawned; this is only the fading visual.
struct Bolt {
    sf::Vector2f a;
    sf::Vector2f b;
    float life = cfg::element::boltLife;
    float maxLife = cfg::element::boltLife;
};

// A stone ball's rubble: enemies are pushed out of it and take chip damage.
struct Obstacle {
    sf::Vector2f pos;
    float radius = cfg::element::obstacleRadius;
    float life = cfg::element::obstacleLife;
    float maxLife = cfg::element::obstacleLife;
};

// The thing you defend.
struct Core {
    sf::Vector2f pos;
    float radius = cfg::core::radius;
    float hp = cfg::core::baseHp;
    float maxHp = cfg::core::baseHp;
    float hitFlash = 0.f;
};

// Two bosses share this struct:
//  - Charger  (wave 10): walks dead straight at the core from the right.
//  - Orbital  (wave 20): spirals in toward the core behind a spinning ring of
//    shield enemies; smaller and lower HP, the ring is the real problem.
// Either one touching the core loses the run outright.
enum class BossKind { Charger, Orbital };

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

// A burst on the field: a reaction, an explosion, a mastery pulse. `label` is
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
    bool autoFlung = false;               // the auto-throw option launched a ball
    bool bossHit = false;                 // a ball landed on the miniboss this step
    bool waveCleared = false;
    bool runOver = false;
};

// Per-step tuning handed to the simulation: the wave number plus whatever
// between-wave upgrades the player has picked this run.
struct WorldParams {
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
    float elemMult[kElementCount] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f};  // per-element potency (web levels)
    int  emberLevel = 0;          // Ember web node: fire hits apply a burn DoT
    bool autoFling = false;       // option: the game throws a ball at the threat now and then

    // Meta web (Fase A).
    int  aegisHits = 0;           // Aegis: core ignores this many hits at the start of each wave
    float coreRegenPerSec = 0.f;  // Regen: core heals this fast during a wave
    bool stockpile = false;       // Stockpile: a random power-up refills a reserve slot (key Q)
    bool magnetPickups = false;   // Magnet: power-up orbs drift toward the nearest ball
    int  afterglowLevel = 0;      // Afterglow: continuous buffs fade out instead of cutting
    float chargedFrac = 0.f;      // Charged: power-ups start with + this fraction of duration

    unsigned powerUpMask = 0xffffffffu;  // bit i set => PowerUp(i) can drop
    float pickupSpawnMult = 1.f;  // scales the gap between power-ups (< 1 = more often)
    float pickupDurMult = 1.f;    // scales how long a power-up lasts
};

}  // namespace sb
