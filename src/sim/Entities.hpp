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

// ---------------------------------------------------------------- entities

struct Ball {
    sf::Vector2f pos;  // centre
    sf::Vector2f vel;
    float radius = cfg::ball::radius;
    bool held = false;
    float squash = 0.f;  // 0..1, decays; drives squash-and-stretch
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
    float hitFlash = 0.f;  // 0..1, decays; white flash on taking damage
};

// A placed structure that bends the balls' trajectories. MVP: a single kind,
// the black hole (an attractor). Draggable; nothing else moves it.
enum class FieldKind { BlackHole };

struct FieldObject {
    sf::Vector2f pos;
    FieldKind kind = FieldKind::BlackHole;
    float strength = 1.f;  // multiplier on the kind's base strength
    float radius = cfg::field::blackHoleRadius;  // influence radius
    bool held = false;
};

// The thing you defend. Enemies that reach it chip its health; at zero the run
// ends.
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

// One impact (arena edge or enemy), for the presentation layer.
struct BounceFx {
    sf::Vector2f pos;
    sf::Vector2f normal;
    float speed = 0.f;
    sf::Color color;
};

// Everything the simulation produced in one step, consumed by the app layer.
struct FrameEvents {
    std::vector<BounceFx> bounces;   // edge + enemy impacts, for rings / sound
    std::vector<sf::Vector2f> kills; // enemy death positions, for fx
    int scrapGained = 0;
    int comboTier = 0;
    bool comboTierUp = false;
    bool gotPickup = false;
    PowerUp pickupKind = PowerUp::Points2x;
    bool coreHit = false;
    bool waveCleared = false;
    bool runOver = false;
};

// What the simulation needs from progression each step; World owns no run state.
struct WorldParams {
    float damageMult = 1.f;
    float cruiseMult = 1.f;
    int wave = 1;
};

}  // namespace sb
