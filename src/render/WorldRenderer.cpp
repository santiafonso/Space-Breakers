#include "render/WorldRenderer.hpp"

#include <cmath>

namespace sb {

void WorldRenderer::drawCore(sf::RenderWindow& window, const Core& c) const {
    const float frac = c.maxHp > 0.f ? clampf(c.hp / c.maxHp, 0.f, 1.f) : 0.f;
    const sf::Color tint = lerpColor(theme::coreLow, theme::core, frac);

    sf::CircleShape body(c.radius, 40);
    body.setOrigin(c.radius, c.radius);
    body.setPosition(c.pos);
    body.setFillColor(withAlpha(tint, 0.18f + 0.3f * c.hitFlash));
    body.setOutlineThickness(2.f);
    body.setOutlineColor(withAlpha(tint, 0.85f));
    window.draw(body);

    sf::CircleShape hp(c.radius - 6.f, 40);
    hp.setOrigin(hp.getRadius(), hp.getRadius());
    hp.setPosition(c.pos);
    hp.setScale(frac, frac);
    hp.setFillColor(withAlpha(tint, 0.5f));
    window.draw(hp);
}

void WorldRenderer::drawPuddle(sf::RenderWindow& window, const Puddle& p) const {
    const float f = p.maxLife > 0.f ? clampf(p.life / p.maxLife, 0.f, 1.f) : 0.f;
    sf::CircleShape s(p.radius, 20);
    s.setOrigin(p.radius, p.radius);
    s.setPosition(p.pos);
    s.setFillColor(withAlpha(theme::elemWater, 0.10f + 0.16f * f));
    window.draw(s);
}

void WorldRenderer::drawObstacle(sf::RenderWindow& window, const Obstacle& o) const {
    const float f = o.maxLife > 0.f ? clampf(o.life / o.maxLife, 0.f, 1.f) : 0.f;
    sf::CircleShape s(o.radius, 6);  // hexagon-ish "rock"
    s.setOrigin(o.radius, o.radius);
    s.setPosition(o.pos);
    s.setRotation(20.f);
    s.setFillColor(withAlpha(theme::elemStone, 0.35f + 0.4f * f));
    s.setOutlineThickness(1.5f);
    s.setOutlineColor(withAlpha(theme::elemStone, 0.6f * f));
    window.draw(s);
}

void WorldRenderer::drawProjectile(sf::RenderWindow& window, const Projectile& pr) const {
    sf::CircleShape s(3.5f, 10);
    s.setOrigin(s.getRadius(), s.getRadius());
    s.setPosition(pr.pos);
    s.setFillColor(theme::elemWind);
    window.draw(s);
}

void WorldRenderer::drawEnemy(sf::RenderWindow& window, const Enemy& e) const {
    const float frac = e.maxHp > 0.f ? clampf(e.hp / e.maxHp, 0.f, 1.f) : 0.f;
    sf::Color fill = lerpColor(theme::enemy, sf::Color::White, e.hitFlash);
    if (e.burn > 0.f) fill = lerpColor(fill, theme::elemFire, 0.5f);

    sf::CircleShape body(e.radius, 24);
    body.setOrigin(e.radius, e.radius);
    body.setPosition(e.pos);
    body.setFillColor(withAlpha(fill, 0.9f));
    body.setOutlineThickness(2.f);
    body.setOutlineColor(withAlpha(sf::Color::White, 0.15f + 0.5f * e.hitFlash));
    window.draw(body);

    sf::CircleShape hpDot(e.radius * 0.5f * frac + 1.f, 16);
    hpDot.setOrigin(hpDot.getRadius(), hpDot.getRadius());
    hpDot.setPosition(e.pos);
    hpDot.setFillColor(withAlpha(theme::bg, 0.55f));
    window.draw(hpDot);
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

    sf::CircleShape corePt(pu.radius * 0.4f, 16);
    corePt.setOrigin(corePt.getRadius(), corePt.getRadius());
    corePt.setPosition(pu.pos);
    corePt.setFillColor(withAlpha(col, fade));
    window.draw(corePt);
}

void WorldRenderer::drawBall(sf::RenderWindow& window, const Ball& b,
                             const std::optional<ActiveEffect>& effect) const {
    sf::Color col = b.element == Element::Plain ? b.color
                                               : lerpColor(b.color, elementColor(b.element), 0.7f);
    float alpha = 1.f;
    if (effect) {
        if (effect->kind == PowerUp::Golden) col = lerpColor(col, theme::puGolden, 0.85f);
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
    const float perpS = 1.f + 0.22f * b.squash;

    sf::CircleShape c(b.radius, 40);
    c.setOrigin(b.radius, b.radius);
    c.setPosition(b.pos);
    c.setScale(lerpf(perpS, along, ax), lerpf(perpS, along, ay));
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
    for (const Puddle& p : world.puddles()) drawPuddle(window, p);
    for (const Obstacle& o : world.obstacles()) drawObstacle(window, o);
    drawCore(window, world.core());
    for (const Enemy& e : world.enemies()) drawEnemy(window, e);
    for (const Projectile& pr : world.projectiles()) drawProjectile(window, pr);
    for (const Pickup& pu : world.pickups()) drawPickup(window, pu);
    for (const Ball& b : world.balls()) drawBall(window, b, world.effect());
}

}  // namespace sb
