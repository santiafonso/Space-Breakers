#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

// Vector maths, colour blending and a small RNG wrapper. The rest of the game
// leans on these so the other translation units stay focused on behaviour.
namespace sb {

inline constexpr float kPi = 3.14159265358979323846f;

inline float length(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }

inline float dot(sf::Vector2f a, sf::Vector2f b) { return a.x * b.x + a.y * b.y; }

inline sf::Vector2f normalized(sf::Vector2f v, sf::Vector2f fallback = {1.f, 0.f}) {
    const float l = length(v);
    return l < 1e-4f ? fallback : v / l;
}

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

inline sf::Color lerpColor(sf::Color a, sf::Color b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto mix = [&](std::uint8_t x, std::uint8_t y) {
        return static_cast<std::uint8_t>(std::lround(x + (y - x) * t));
    };
    return sf::Color(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a));
}

inline sf::Color withAlpha(sf::Color c, float alpha01) {
    c.a = static_cast<std::uint8_t>(clampf(alpha01, 0.f, 1.f) * 255.f);
    return c;
}

// Serialisable snapshot of one inner wall: centre, half-extents, drift velocity.
struct WallSnapshot {
    float cx = 0.f, cy = 0.f, hx = 0.f, hy = 0.f, vx = 0.f, vy = 0.f;
};

// Thin wrapper so call sites read as intent ("a random angle jitter") rather
// than distribution boilerplate.
struct Rng {
    std::mt19937 gen{std::random_device{}()};
    float range(float a, float b) { return std::uniform_real_distribution<float>(a, b)(gen); }
    int irange(int a, int b) { return std::uniform_int_distribution<int>(a, b)(gen); }
    float unit() { return range(-1.f, 1.f); }
    sf::Vector2f direction() { return normalized({unit(), unit()}, {1.f, 0.3f}); }
};

}  // namespace sb
