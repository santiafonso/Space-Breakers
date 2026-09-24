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
sf::Color powerUpColor(PowerUp p);
float powerUpDuration(PowerUp p);

// ---------------------------------------------------------------- ball elements

// Order matches the unlock chain; (value - 1) indexes WorldParams::elemMult and
// the element web-node slots (Plain aside).
enum class Element { Plain, Fire, Poison, Water, Ice, Stone, Electric };
inline constexpr int kElementCount = 7;

const char* elementName(Element e);
sf::Color elementColor(Element e);

// ---------------------------------------------------------------- entities

struct Ball {
    sf::Vector2f pos;  // centre
    sf::Vector2f vel;
    float radius = cfg::ball::radius;
    bool held = false;
    bool lead = false;      // "Spearhead": the most recently flung ball, cruises faster
    Element element = Element::Plain;
    float cooldown = 0.f;   // water drip / stone drop / electric zap timer
    float squash = 0.f;
    sf::Vector2f squashAxis{1.f, 0.f};
    sf::Color color = theme::ballSlow;
    float ricochetT = 0.f;   // "Ricochet": seconds of post-wall-bounce damage bonus left
    std::deque<sf::Vector2f> trail;
    std::deque<sf::Vector2f> waterTrail;   // water ball only: the damaging "worm" wake
};

// An enemy walks straight at the core. Balls damage it on contact.
struct Enemy {
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

struct FrameEvents {
    std::vector<BounceFx> bounces;
    std::vector<sf::Vector2f> kills;
    int comboTier = 0;
    bool comboTierUp = false;
    bool gotPickup = false;
    PowerUp pickupKind = PowerUp::Points2x;
    bool coreHit = false;
    bool bossHit = false;                 // a ball landed on the miniboss this step
    bool waveCleared = false;
    bool runOver = false;
};

// Per-step tuning handed to the simulation: the wave number plus whatever
// between-wave upgrades the player has picked this run.
struct WorldParams {
    float damageMult = 1.f;       // Heavy impact
    float cruiseMult = 1.f;
    int wave = 1;
    float ballRadiusMult = 1.f;   // Big ball
    float coreBounceBoost = 1.f;  // Spring
    float wallBounceBoost = 1.f;  // Wall rush
    float pairBounceBoost = 1.f;  // Carom (ball-vs-ball)
    float flingDecayMult = 1.f;   // Reflexes (< 1 keeps fling speed longer)
    bool slowField = false;       // Slow field: a zone around the core slows enemies
    float elemMult[kElementCount] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f};  // per-element potency (web levels)

    // More item toggles.
    bool ricochet = false;        // Ricochet: wall bounce arms a brief damage bonus
    float maxSpeedMult = 1.f;     // Ceiling break: multiplies the top-speed ceiling
    bool warmUp = false;          // Warm-up: cruise speed ramps up over the wave
    float knockbackMult = 1.f;    // Heavy knock
    bool conductor = false;       // Conductor: electric arc jumps to a 2nd enemy
    bool shatter = false;         // Shatter: bonus damage vs frozen enemies
    bool contagion = false;       // Contagion: a poisoned enemy dying re-poisons nearby
    bool bedrock = false;         // Bedrock: stone rubble lasts far longer
    bool primed = false;          // Primed: +damage vs enemies already under an element effect
    float leadBallCruise = 1.f;   // Spearhead: cruise-speed multiplier for the lead ball

    // "Ball combat" items (Fase A) - all lean on speed / damage picks for synergy.
    bool crit = false;            // Keen eye: chance of a double-damage contact hit
    bool bruiser = false;         // Battering: contact damage scales with ball speed
    bool executioner = false;     // Executioner: huge damage to low-HP enemies
    bool overkill = false;        // Overkill: a kill's leftover damage splashes to a neighbour
    bool cleave = false;          // Cleave: the ball passes through an enemy it kills
    bool tempo = false;           // Tempo: ball recovers cruise speed faster after an enemy hit
    int  emberLevel = 0;          // Ember web node: fire hits apply a burn DoT

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
