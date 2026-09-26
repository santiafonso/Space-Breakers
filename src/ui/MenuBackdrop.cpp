#include "ui/MenuBackdrop.hpp"

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

}  // namespace sb
