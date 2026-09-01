#pragma once

#include <deque>
#include <vector>

#include "core/Config.hpp"
#include "core/Math.hpp"
#include "core/Theme.hpp"

namespace sb {

// ---------------------------------------------------------------- power-ups

enum class PowerUp { Points2x, SlowMo, Surge, Golden, Ghost, Frenzy };
inline constexpr int kPowerUpCount = 6;

const char* powerUpName(PowerUp p);
sf::Color powerUpColor(PowerUp p);
float powerUpDuration(PowerUp p);

// ---------------------------------------------------------------- ball elements

enum class Element { Plain, Fire, Wind, Water, Stone };
inline constexpr int kElementCount = 5;

const char* elementName(Element e);
sf::Color elementColor(Element e);

// ---------------------------------------------------------------- entities

struct Ball {
    sf::Vector2f pos;  // centre
    sf::Vector2f vel;
    float radius = cfg::ball::radius;
    bool held = false;
    Element element = Element::Plain;
    float cooldown = 0.f;   // wind bolt / stone drop / water drip timer
    float squash = 0.f;
    sf::Vector2f squashAxis{1.f, 0.f};
    sf::Color color = theme::ballSlow;
    std::deque<sf::Vector2f> trail;
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
    float burn = 0.f;       // seconds of burn remaining
    float burnDps = 0.f;
};

// A wind ball's bolt.
struct Projectile {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float life = cfg::element::windLife;
    float damage = cfg::element::windDamage;
};

// A water ball's trail: a short-lived damaging puddle.
struct Puddle {
    sf::Vector2f pos;
    float radius = cfg::element::puddleRadius;
    float life = cfg::element::puddleLife;
    float maxLife = cfg::element::puddleLife;
};

// A stone ball's rubble: enemies are pushed out of it.
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
};

struct FrameEvents {
    std::vector<BounceFx> bounces;
    std::vector<sf::Vector2f> kills;
    int scrapGained = 0;
    int comboTier = 0;
    bool comboTierUp = false;
    bool gotPickup = false;
    PowerUp pickupKind = PowerUp::Points2x;
    bool coreHit = false;
    bool waveCleared = false;
    bool runOver = false;
};

struct WorldParams {
    float damageMult = 1.f;
    float cruiseMult = 1.f;
    int wave = 1;
};

}  // namespace sb
