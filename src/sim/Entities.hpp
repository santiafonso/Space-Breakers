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

// A player-owned inner wall. Static by default; drag it to reposition, fling it
// (or right-click) to make it drift and bounce around the arena.
struct Wall {
    sf::Vector2f pos;   // centre
    sf::Vector2f half;  // half-extents
    sf::Vector2f vel{0.f, 0.f};
    bool held = false;
    bool drifting() const { return (vel.x * vel.x + vel.y * vel.y) > 1.f; }
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

// One bounce, for the presentation layer to turn into a ring / sound / edge glow.
struct BounceFx {
    sf::Vector2f pos;
    sf::Vector2f normal;
    float speed = 0.f;
    sf::Color color;
};

// Everything the simulation produced in one step, consumed by the app layer.
struct FrameEvents {
    std::vector<BounceFx> bounces;
    int pointsGained = 0;
    int comboTier = 0;
    bool comboTierUp = false;
    bool gotPickup = false;
    PowerUp pickupKind = PowerUp::Points2x;
};

// Upgrade levels the simulation cares about, passed in each step so World owns
// no progression state.
struct WorldParams {
    int speedLevel = 0;
    int pointsLevel = 0;
    int wallLevel = 0;
    int comboLevel = 0;
    int luckLevel = 0;
};

}  // namespace sb
