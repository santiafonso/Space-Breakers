#include "ui/MenuBackdrop.hpp"

#include <algorithm>
#include <cmath>

#include "core/Theme.hpp"
#include "render/Draw.hpp"

namespace sb {

namespace {
constexpr int kOrbCount = 11;
}  // namespace

void MenuBackdrop::init(sf::Vector2f size) {
    size_ = size;
    orbs_.clear();
    orbs_.reserve(kOrbCount);
    for (int i = 0; i < kOrbCount; ++i) {
        Orb o;
        o.radius = rng_.range(9.f, 30.f);
        o.pos = {rng_.range(o.radius, size_.x - o.radius),
                 rng_.range(o.radius, size_.y - o.radius)};
        o.vel = rng_.direction() * rng_.range(14.f, 46.f);
        // Mostly the calm blue-green end of the ball ramp, an odd warm one.
        const float t = rng_.range(0.f, 1.f);
        o.color = t < 0.75f ? lerpColor(theme::ballSlow, theme::ballMid, t / 0.75f)
                            : lerpColor(theme::ballMid, theme::ballFast, (t - 0.75f) / 0.25f);
        orbs_.push_back(o);
    }
}

void MenuBackdrop::update(float dt) {
    for (Orb& o : orbs_) {
        o.pos += o.vel * dt;
        if (o.pos.x < o.radius) { o.pos.x = o.radius; o.vel.x = std::fabs(o.vel.x); }
        else if (o.pos.x > size_.x - o.radius) { o.pos.x = size_.x - o.radius; o.vel.x = -std::fabs(o.vel.x); }
        if (o.pos.y < o.radius) { o.pos.y = o.radius; o.vel.y = std::fabs(o.vel.y); }
        else if (o.pos.y > size_.y - o.radius) { o.pos.y = size_.y - o.radius; o.vel.y = -std::fabs(o.vel.y); }
    }
}

void MenuBackdrop::draw(sf::RenderWindow& window, float alpha) const {
    if (alpha <= 0.01f) return;
    // The same radar the arena has, centred on the screen, and a handful of
    // small dim balls crossing it - the game idling behind glass.
    draw::radar(window, size_ * 0.5f, length(size_) * 0.55f, 96.f, 12, theme::grid, 0.10f * alpha);
    for (const Orb& o : orbs_) {
        const float depth = clampf((o.radius - 9.f) / 21.f, 0.f, 1.f);   // bigger = nearer = brighter
        const float r = 2.5f + 4.f * depth;
        const float a = (0.22f + 0.25f * depth) * alpha;
        const sf::Vector2f tail = o.pos - o.vel * 0.6f;
        sf::Vertex streak[4];
        const sf::Vector2f n = normalized({-o.vel.y, o.vel.x}) * (r * 0.8f);
        streak[0] = {tail, withAlpha(o.color, 0.f)};
        streak[1] = {o.pos + n, withAlpha(o.color, a * 0.5f)};
        streak[2] = {o.pos - n, withAlpha(o.color, a * 0.5f)};
        streak[3] = {tail, withAlpha(o.color, 0.f)};
        window.draw(streak, 4, sf::Quads);
        draw::disc(window, o.pos, r, withAlpha(lerpColor(o.color, sf::Color::White, 0.25f), a),
                   withAlpha(o.color, a * 0.8f), {1.f, 1.f}, 20);
    }
}

namespace {
struct Planet {
    float orbit;     // orbit radius, px
    float tilt;      // ellipse squash (1 = circle)
    float speed;     // rad/s (sign = direction)
    float phase;
    float radius;
    sf::Color color;
    bool ring;       // a Saturn ring
    bool moon;
};
const Planet kPlanets[] = {
    {300.f, 0.42f, 0.050f, 0.6f, 15.f, theme::ballSlow, false, true},
    {520.f, 0.38f, -0.032f, 2.4f, 26.f, theme::puSurge, true, false},
    {760.f, 0.45f, 0.022f, 4.1f, 20.f, theme::ember, false, true},
    {980.f, 0.40f, -0.016f, 5.3f, 34.f, theme::ballMid, true, false},
    {1240.f, 0.43f, 0.011f, 1.2f, 12.f, theme::venom, false, false},
};
}  // namespace

void drawOrbitingPlanets(sf::RenderWindow& w, sf::Vector2f c, float t, float k, float alpha) {
    if (alpha <= 0.01f) return;
    for (const Planet& pl : kPlanets) {
        const float R = pl.orbit * k;
        {   // its orbit: a faint ellipse
            sf::CircleShape o(R);
            o.setOrigin(R, R);
            o.setPosition(c);
            o.setScale(1.f, pl.tilt);
            o.setPointCount(120);
            o.setFillColor(sf::Color::Transparent);
            o.setOutlineThickness(1.f);
            o.setOutlineColor(withAlpha(theme::grid, 0.12f * alpha));
            w.draw(o);
        }
        const float a = pl.phase + pl.speed * t;
        const sf::Vector2f p = c + sf::Vector2f{std::cos(a) * R, std::sin(a) * R * pl.tilt};
        const float pr = pl.radius * k;
        const float near = 0.75f + 0.25f * std::sin(a);   // the near side of the orbit is a bit brighter
        const float pa = (0.45f + 0.2f * near) * alpha;
        draw::glow(w, p, pr * 2.8f, pl.color, 0.16f * alpha);
        // lit from the centre: a bright side toward it, a dark side away
        const sf::Vector2f toC = normalized(c - p, {0.f, -1.f});
        draw::disc(w, p, pr, withAlpha(lerpColor(pl.color, theme::bg, 0.35f), pa),
                   withAlpha(lerpColor(pl.color, theme::bg, 0.75f), pa), {1.f, 1.f}, 40);
        draw::disc(w, p + toC * (pr * 0.28f), pr * 0.62f, withAlpha(lerpColor(pl.color, sf::Color::White, 0.15f), pa * 0.55f),
                   withAlpha(pl.color, 0.f), {1.f, 1.f}, 32);
        if (pl.ring) {   // a flat ring across it
            sf::CircleShape rg(pr * 1.75f);
            rg.setOrigin(pr * 1.75f, pr * 1.75f);
            rg.setPosition(p);
            rg.setScale(1.f, 0.28f);
            rg.setRotation(-14.f);
            rg.setPointCount(60);
            rg.setFillColor(sf::Color::Transparent);
            rg.setOutlineThickness(2.f);
            rg.setOutlineColor(withAlpha(lerpColor(pl.color, sf::Color::White, 0.3f), pa * 0.7f));
            w.draw(rg);
        }
        if (pl.moon) {   // a small moon on a quick orbit of its own
            const float ma = t * 0.6f + pl.phase * 3.f;
            const sf::Vector2f mp = p + sf::Vector2f{std::cos(ma), std::sin(ma) * 0.55f} * (pr * 2.3f);
            draw::disc(w, mp, std::max(2.f, pr * 0.22f), withAlpha(theme::textLo, pa), withAlpha(theme::textDim, pa),
                       {1.f, 1.f}, 16);
        }
    }
}

}  // namespace sb
