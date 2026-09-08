#include "ui/MenuBackdrop.hpp"

#include "core/Theme.hpp"

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
    for (const Orb& o : orbs_) {
        // Bigger orbs read a touch brighter, like they sit closer to the glass.
        const float depth = clampf((o.radius - 9.f) / 21.f, 0.f, 1.f);

        sf::CircleShape glow(o.radius * 2.4f, 24);
        glow.setOrigin(glow.getRadius(), glow.getRadius());
        glow.setPosition(o.pos);
        glow.setFillColor(withAlpha(o.color, (0.03f + 0.03f * depth) * alpha));
        window.draw(glow);

        sf::CircleShape body(o.radius, 32);
        body.setOrigin(o.radius, o.radius);
        body.setPosition(o.pos);
        body.setFillColor(withAlpha(o.color, (0.04f + 0.04f * depth) * alpha));
        body.setOutlineThickness(1.5f);
        body.setOutlineColor(withAlpha(o.color, (0.16f + 0.16f * depth) * alpha));
        window.draw(body);
    }
}

}  // namespace sb
