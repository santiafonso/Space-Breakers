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

// The water ball's "worm" wake: a tapering ribbon along its recent path, widest
// at the head (by the ball), fading to nothing at the tail.
void WorldRenderer::drawWaterTrail(sf::RenderWindow& window, const Ball& b) const {
    const auto& pts = b.waterTrail;
    const int n = static_cast<int>(pts.size());
    if (n < 2) return;
    const float w0 = cfg::element::waterTrailWidth;

    sf::VertexArray ribbon(sf::TriangleStrip, static_cast<std::size_t>(n) * 2);
    for (int i = 0; i < n; ++i) {
        const sf::Vector2f prev = pts[i > 0 ? i - 1 : i];
        const sf::Vector2f next = pts[i < n - 1 ? i + 1 : i];
        sf::Vector2f dir = next - prev;
        const float dl = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir = dl > 1e-4f ? dir / dl : sf::Vector2f{1.f, 0.f};
        const sf::Vector2f nrm{-dir.y, dir.x};

        const float taper = static_cast<float>(i + 1) / static_cast<float>(n);  // 0 tail -> ~1 head
        const float half = w0 * taper;
        const sf::Color col = withAlpha(theme::elemWater, (0.10f + 0.28f * taper));
        ribbon[static_cast<std::size_t>(i) * 2].position = pts[i] + nrm * half;
        ribbon[static_cast<std::size_t>(i) * 2].color = col;
        ribbon[static_cast<std::size_t>(i) * 2 + 1].position = pts[i] - nrm * half;
        ribbon[static_cast<std::size_t>(i) * 2 + 1].color = col;
    }
    window.draw(ribbon);
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

void WorldRenderer::drawBolt(sf::RenderWindow& window, const Bolt& bo) const {
    const float f = bo.maxLife > 0.f ? clampf(bo.life / bo.maxLife, 0.f, 1.f) : 0.f;
    const sf::Color col = withAlpha(theme::elemElectric, 0.35f + 0.55f * f);

    // A jagged 3-segment arc between the two endpoints.
    const sf::Vector2f d = bo.b - bo.a;
    const sf::Vector2f n = normalized({-d.y, d.x});
    sf::VertexArray arc(sf::LineStrip, 4);
    for (int i = 0; i < 4; ++i) {
        const float t = static_cast<float>(i) / 3.f;
        float off = 0.f;
        if (i == 1) off = 7.f;
        else if (i == 2) off = -6.f;
        arc[i].position = bo.a + d * t + n * off;
        arc[i].color = col;
    }
    window.draw(arc);
}

void WorldRenderer::drawBoss(sf::RenderWindow& window, const Boss& b, sf::Vector2f corePos) const {
    if (!b.alive) return;
    const float frac = b.maxHp > 0.f ? clampf(b.hp / b.maxHp, 0.f, 1.f) : 0.f;
    const sf::Color fill = lerpColor(theme::enemy, sf::Color::White, b.hitFlash);

    // Orbital boss: draw a faint guide along the spiral it's about to travel, so
    // the player can read where it comes from and how it moves.
    if (b.kind == BossKind::Orbital) {
        float ang = b.ang, dist = b.dist;
        if (b.intro > 0.f) {  // during the slide-in the spiral hasn't started
            ang = cfg::finalBoss::startAngle;
        }
        sf::VertexArray path(sf::LineStrip);
        for (int i = 0; i < 30; ++i) {
            ang += cfg::finalBoss::spiralOmega * 0.05f;
            dist = std::max(0.f, dist - cfg::finalBoss::spiralShrink * 0.05f);
            const sf::Vector2f pt =
                corePos + sf::Vector2f{std::cos(ang), std::sin(ang)} * dist;
            const float a = 0.22f * (1.f - static_cast<float>(i) / 30.f);
            path.append(sf::Vertex(pt, withAlpha(theme::coreLow, a)));
        }
        window.draw(path);
    }

    const float glowPulse = b.intro > 0.f ? 1.7f + 0.5f * std::sin(b.intro * 12.f) : 1.7f;
    sf::CircleShape glow(b.radius * glowPulse, 28);
    glow.setOrigin(glow.getRadius(), glow.getRadius());
    glow.setPosition(b.pos);
    glow.setFillColor(withAlpha(theme::coreLow, b.intro > 0.f ? 0.18f : 0.12f));
    window.draw(glow);

    sf::CircleShape body(b.radius, 30);
    body.setOrigin(b.radius, b.radius);
    body.setPosition(b.pos);
    body.setFillColor(withAlpha(fill, 0.92f));
    body.setOutlineThickness(3.f);
    body.setOutlineColor(withAlpha(theme::coreLow, 0.8f));
    window.draw(body);

    sf::CircleShape core(b.radius * 0.42f, 20);
    core.setOrigin(core.getRadius(), core.getRadius());
    core.setPosition(b.pos);
    core.setFillColor(withAlpha(theme::bg, 0.5f));
    window.draw(core);

    // Small health bar just above the boss.
    const float bw = 76.f, bh = 6.f;
    const float y = b.pos.y - b.radius - 16.f;
    sf::RectangleShape track({bw, bh});
    track.setOrigin(bw * 0.5f, bh * 0.5f);
    track.setPosition(b.pos.x, y);
    track.setFillColor(withAlpha(theme::arenaEdge, 0.9f));
    window.draw(track);

    sf::RectangleShape hp({bw * frac, bh});
    hp.setOrigin(bw * 0.5f, bh * 0.5f);
    hp.setPosition(b.pos.x, y);
    hp.setFillColor(theme::coreLow);
    window.draw(hp);
}

void WorldRenderer::drawEnemy(sf::RenderWindow& window, const Enemy& e, sf::Vector2f corePos) const {
    const float frac = e.maxHp > 0.f ? clampf(e.hp / e.maxHp, 0.f, 1.f) : 0.f;
    // Each kind keeps the enemy red but reads differently: runners/shards
    // lighter and smaller, tanks darker with a heavy rim, splitters cracked,
    // shielded ones carry a bright arc on the side facing the core.
    sf::Color base = theme::enemy;
    if (e.kind == EnemyKind::Runner || e.kind == EnemyKind::Shard) base = lerpColor(theme::enemy, theme::ballFast, 0.35f);
    if (e.kind == EnemyKind::Tank) base = lerpColor(theme::enemy, theme::bg, 0.3f);
    sf::Color fill = lerpColor(base, sf::Color::White, e.hitFlash);
    if (e.poison > 0.f) fill = lerpColor(fill, theme::elemPoison, 0.5f);
    if (e.burn > 0.f)   fill = lerpColor(fill, theme::elemFire, 0.5f);
    if (e.frozen > 0.f) fill = lerpColor(fill, theme::elemIce, 0.65f);

    sf::CircleShape body(e.radius, 24);
    body.setOrigin(e.radius, e.radius);
    body.setPosition(e.pos);
    body.setFillColor(withAlpha(fill, 0.9f));
    body.setOutlineThickness(e.kind == EnemyKind::Tank ? 4.f : (e.mark > 0.f ? 3.f : 2.f));   // marked: brighter rim
    body.setOutlineColor(withAlpha(sf::Color::White,
                                   (e.mark > 0.f ? 0.7f : 0.15f) + 0.3f * e.hitFlash));
    window.draw(body);

    if (e.kind == EnemyKind::Splitter) {   // a crack across the middle
        sf::RectangleShape crack({e.radius * 1.6f, 2.f});
        crack.setOrigin(e.radius * 0.8f, 1.f);
        crack.setPosition(e.pos);
        crack.setRotation(35.f);
        crack.setFillColor(withAlpha(theme::bg, 0.8f));
        window.draw(crack);
    }
    if (e.kind == EnemyKind::Shielded) {   // the shield arc, facing the core
        const float face = std::atan2(corePos.y - e.pos.y, corePos.x - e.pos.x);
        const float arc = cfg::enemy::shieldArc;
        const int segs = 10;
        const float rr = e.radius + 5.f;
        for (int i = 0; i < segs; ++i) {
            const float a0 = face - arc + 2.f * arc * static_cast<float>(i) / segs;
            const float a1 = face - arc + 2.f * arc * static_cast<float>(i + 1) / segs;
            const sf::Vector2f p0 = e.pos + sf::Vector2f{std::cos(a0), std::sin(a0)} * rr;
            const sf::Vector2f p1 = e.pos + sf::Vector2f{std::cos(a1), std::sin(a1)} * rr;
            const sf::Vector2f d = p1 - p0;
            sf::RectangleShape seg({length(d) + 1.f, 3.5f});
            seg.setOrigin(0.f, 1.75f);
            seg.setPosition(p0);
            seg.setRotation(std::atan2(d.y, d.x) * 180.f / kPi);
            seg.setFillColor(withAlpha(e.frozen > 0.f ? theme::elemIce : theme::textHi, 0.85f));
            window.draw(seg);
        }
    }

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
    sf::Color col = b.color;   // World bakes the element hue into b.color (see elementSpeedColor)
    float alpha = 1.f;
    if (effect && effect->kind == PowerUp::Golden) col = lerpColor(col, theme::puGolden, 0.85f);

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
    // Role marks, kept minimal: a Guardian has a heavy rim, a Support a small
    // inner ring, a Striker a small centre dot; a Normal ball is plain.
    const bool guardian = b.role == BallRole::Guardian;
    c.setOutlineThickness(guardian ? 4.f : 2.f);
    c.setOutlineColor(withAlpha(sf::Color::White,
                                (b.held ? 0.85f : (guardian ? 0.5f : 0.16f)) * alpha));
    window.draw(c);
    if (b.role == BallRole::Support) {
        const float ir = b.radius * 0.45f;
        sf::CircleShape inner(ir, 24);
        inner.setOrigin(ir, ir);
        inner.setPosition(b.pos);
        inner.setFillColor(sf::Color::Transparent);
        inner.setOutlineThickness(2.f);
        inner.setOutlineColor(withAlpha(sf::Color::White, 0.65f * alpha));
        window.draw(inner);
    } else if (b.role == BallRole::Striker) {
        const float dr = b.radius * 0.26f;
        sf::CircleShape dot(dr, 16);
        dot.setOrigin(dr, dr);
        dot.setPosition(b.pos);
        dot.setFillColor(withAlpha(sf::Color::White, 0.75f * alpha));
        window.draw(dot);
    }

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
    for (const Ball& b : world.balls())
        if (b.element == Element::Water) drawWaterTrail(window, b);
    for (const Obstacle& o : world.obstacles()) drawObstacle(window, o);
    drawCore(window, world.core());
    drawBoss(window, world.boss(), world.core().pos);
    for (const Enemy& e : world.enemies()) drawEnemy(window, e, world.core().pos);
    for (const Bolt& bo : world.bolts()) drawBolt(window, bo);
    for (const Pickup& pu : world.pickups()) drawPickup(window, pu);
    for (const Ball& b : world.balls()) drawBall(window, b, world.effect());
}

}  // namespace sb
