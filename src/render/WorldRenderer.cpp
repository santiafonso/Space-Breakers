#include "render/WorldRenderer.hpp"

#include <cmath>

namespace sb {

void WorldRenderer::drawWall(sf::RenderWindow& window, const Wall& w) const {
    const bool active = w.held || w.drifting();
    sf::RectangleShape rs({w.half.x * 2.f, w.half.y * 2.f});
    rs.setOrigin(w.half);
    rs.setPosition(w.pos);
    rs.setFillColor(withAlpha(theme::arenaEdge, 0.5f));
    rs.setOutlineThickness(1.5f);
    rs.setOutlineColor(withAlpha(theme::accent, active ? 0.6f : 0.28f));
    window.draw(rs);

    if (w.held) {
        sf::RectangleShape hl({w.half.x * 2.f + 10.f, w.half.y * 2.f + 10.f});
        hl.setOrigin(hl.getSize() * 0.5f);
        hl.setPosition(w.pos);
        hl.setFillColor(sf::Color::Transparent);
        hl.setOutlineThickness(2.f);
        hl.setOutlineColor(withAlpha(theme::accent, 0.7f));
        window.draw(hl);
    }
}

void WorldRenderer::drawPickup(sf::RenderWindow& window, const Pickup& pu) const {
    const sf::Color col = powerUpColor(pu.kind);
    const float pulse = 0.5f + 0.5f * std::sin(pu.age * 4.f);
    const float fade =
        pu.age > pu.ttl - 2.f ? clampf((pu.ttl - pu.age) / 2.f, 0.f, 1.f) : 1.f;

    sf::CircleShape glow(pu.radius * 1.9f, 24);
    glow.setOrigin(glow.getRadius(), glow.getRadius());
    glow.setPosition(pu.pos);
    glow.setFillColor(withAlpha(col, 0.10f * fade));
    window.draw(glow);

    sf::CircleShape ring(pu.radius + pulse * 2.f, 28);
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(pu.pos);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(2.f);
    ring.setOutlineColor(withAlpha(col, 0.8f * fade));
    window.draw(ring);

    sf::CircleShape core(pu.radius * 0.4f, 16);
    core.setOrigin(core.getRadius(), core.getRadius());
    core.setPosition(pu.pos);
    core.setFillColor(withAlpha(col, fade));
    window.draw(core);
}

void WorldRenderer::drawBall(sf::RenderWindow& window, const Ball& b,
                             const std::optional<ActiveEffect>& effect) const {
    sf::Color col = b.color;
    float alpha = 1.f;
    if (effect) {
        if (effect->kind == PowerUp::Golden) col = lerpColor(b.color, theme::puGolden, 0.85f);
        else if (effect->kind == PowerUp::Ghost) alpha = 0.4f;
    }

    if (!b.held && !b.trail.empty()) {
        const int n = static_cast<int>(b.trail.size());
        for (int k = 0; k < n; ++k) {
            const float f = static_cast<float>(k + 1) / static_cast<float>(n + 1);
            sf::CircleShape g(b.radius * (0.3f + 0.55f * f), 16);
            g.setOrigin(g.getRadius(), g.getRadius());
            g.setPosition(b.trail[k]);
            g.setFillColor(withAlpha(col, (0.04f + 0.12f * f) * alpha));
            window.draw(g);
        }
    }

    const float ax = std::fabs(b.squashAxis.x);
    const float ay = std::fabs(b.squashAxis.y);
    const float along = 1.f - 0.32f * b.squash;
    const float perp = 1.f + 0.22f * b.squash;

    sf::CircleShape c(b.radius, 40);
    c.setOrigin(b.radius, b.radius);
    c.setPosition(b.pos);
    c.setScale(lerpf(perp, along, ax), lerpf(perp, along, ay));
    c.setFillColor(withAlpha(col, alpha));
    c.setOutlineThickness(2.f);
    c.setOutlineColor(withAlpha(sf::Color::White, (b.held ? 0.85f : 0.16f) * alpha));
    window.draw(c);

    if (b.held) {
        sf::CircleShape ring(b.radius + 7.f, 40);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(b.pos);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(withAlpha(theme::accent, 0.7f));
        window.draw(ring);
    }
}

void WorldRenderer::draw(sf::RenderWindow& window, const World& world) const {
    for (const Wall& w : world.walls()) drawWall(window, w);
    for (const Pickup& pu : world.pickups()) drawPickup(window, pu);
    for (const Ball& b : world.balls()) drawBall(window, b, world.effect());
}

}  // namespace sb
