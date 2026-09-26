#include "render/WorldRenderer.hpp"

#include <cmath>

#include "render/Draw.hpp"

namespace sb {

namespace {

sf::Color lighten(sf::Color c, float k) { return lerpColor(c, sf::Color::White, k); }
sf::Color darken(sf::Color c, float k) { return lerpColor(c, theme::bg, k); }

// 0 -> 1 with a small overshoot: things pop into existence instead of fading.
float popIn(float age, float dur) {
    const float t = clampf(age / dur, 0.f, 1.f);
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    return 1.f + c3 * std::pow(t - 1.f, 3.f) + c1 * std::pow(t - 1.f, 2.f);
}

float clockSeconds() {   // a shared visual clock for idle motion (spins, pulses)
    static sf::Clock clock;
    return clock.getElapsedTime().asSeconds();
}

// ---- behaviour-item overlays (Fase M): thin lines and rings only.

// "Tether": a faint band showing the damage width, a bright pulsing core line.
void drawTether(sf::RenderWindow& w, const TetherBeam& t) {
    const sf::Vector2f d = t.b - t.a;
    const float len = length(d);
    if (len < 1.f) return;
    const float ang = std::atan2(d.y, d.x) * 180.f / kPi;
    const float pulse = 0.5f + 0.5f * std::sin(clockSeconds() * 14.f);
    for (int k = 0; k < 2; ++k) {
        const float thick = k == 0 ? t.width * 2.f : 2.f + pulse;
        sf::RectangleShape r({len, thick});
        r.setOrigin(0.f, thick * 0.5f);
        r.setPosition(t.a);
        r.setRotation(ang);
        r.setFillColor(k == 0 ? withAlpha(t.color, 0.10f) : withAlpha(lighten(t.color, 0.5f), 0.8f));
        w.draw(r);
    }
}

// "Black hole": a dark pit with a rim in its element colour, and a faint ring
// closing in on it as it charges toward the burst.
void drawBlackHole(sf::RenderWindow& w, const BlackHole& h) {
    const float f = clampf(h.life / cfg::changer::blackHoleLife, 0.f, 1.f);   // 1 -> 0
    const sf::Color col = h.elem == Element::Plain ? theme::accent : elementColor(h.elem);
    const float r = 12.f + 6.f * (1.f - f);
    draw::disc(w, h.pos, r, withAlpha(theme::bg, 0.95f), withAlpha(theme::bg, 0.7f));
    draw::ring(w, h.pos, r, 2.f, withAlpha(col, 0.85f));
    draw::ring(w, h.pos, r + 70.f * f, 1.5f, withAlpha(col, 0.15f + 0.35f * (1.f - f)));
}

// "Hunter": two small arcs turning around its prey - the lock.
void drawHunterLock(sf::RenderWindow& w, const Ball& b, const Enemy& e) {
    const float a = clockSeconds() * 3.f;
    const float r = e.radius + 12.f;
    const sf::Color col = withAlpha(lighten(b.color, 0.3f), 0.75f);
    draw::ring(w, e.pos, r, 2.f, col, a, a + 1.1f, 12);
    draw::ring(w, e.pos, r, 2.f, col, a + kPi, a + kPi + 1.1f, 12);
}

}  // namespace

void WorldRenderer::drawCore(sf::RenderWindow& window, const Core& c) const {
    const float frac = c.maxHp > 0.f ? clampf(c.hp / c.maxHp, 0.f, 1.f) : 0.f;
    const sf::Color tint = lerpColor(theme::coreLow, theme::core, frac);
    const float t = clockSeconds();
    const float breathe = 0.5f + 0.5f * std::sin(t * 2.f);

    draw::glow(window, c.pos, c.radius * 2.8f, tint, 0.09f + 0.04f * breathe + 0.35f * c.hitFlash);
    draw::disc(window, c.pos, c.radius, withAlpha(lighten(tint, 0.15f), 0.55f + 0.3f * c.hitFlash),
               withAlpha(darken(tint, 0.35f), 0.35f));
    // Health as an arc around the core, over a faint full track.
    draw::ring(window, c.pos, c.radius + 7.f, 4.f, withAlpha(theme::arenaEdge, 0.6f));
    draw::ring(window, c.pos, c.radius + 7.f, 4.f, withAlpha(tint, 0.95f), -kPi * 0.5f,
               -kPi * 0.5f + 2.f * kPi * frac);
    // Three slow-turning segments inside: the core is alive.
    for (int k = 0; k < 3; ++k) {
        const float a = t * 0.8f + static_cast<float>(k) * 2.f * kPi / 3.f;
        draw::ring(window, c.pos, c.radius * 0.62f, 2.5f, withAlpha(lighten(tint, 0.4f), 0.55f), a, a + 1.2f);
    }
    draw::disc(window, c.pos, c.radius * 0.28f, withAlpha(sf::Color::White, 0.8f), withAlpha(tint, 0.6f));
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
    if (bo.beam) {   // Railgun: a straight bright beam that thins out as it fades
        const sf::Vector2f d = bo.b - bo.a;
        const float thick = 3.f + 9.f * f;
        sf::RectangleShape beam({length(d), thick});
        beam.setOrigin(0.f, thick * 0.5f);
        beam.setPosition(bo.a);
        beam.setRotation(std::atan2(d.y, d.x) * 180.f / kPi);
        beam.setFillColor(withAlpha(theme::puGolden, 0.25f + 0.65f * f));
        window.draw(beam);
        return;
    }
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
    const float r = e.radius * popIn(e.age, 0.28f);   // spawns pop in
    if (r <= 0.5f) return;
    // Each kind keeps the enemy red but reads differently: runners / shards are
    // lighter arrowheads, tanks darker turning hexagons, splitters cracked,
    // shielded ones carry a bright arc on the side facing the core.
    sf::Color base = theme::enemy;
    if (e.kind == EnemyKind::Runner || e.kind == EnemyKind::Shard) base = lerpColor(theme::enemy, theme::ballFast, 0.35f);
    if (e.kind == EnemyKind::Tank) base = lerpColor(theme::enemy, theme::bg, 0.25f);
    sf::Color fill = base;
    if (e.poison > 0.f) fill = lerpColor(fill, theme::elemPoison, 0.5f);
    if (e.burn > 0.f)   fill = lerpColor(fill, theme::elemFire, 0.5f);
    if (e.frozen > 0.f) fill = lerpColor(fill, theme::elemIce, 0.65f);
    fill = lerpColor(fill, sf::Color::White, e.hitFlash * 0.8f);
    const sf::Color inner = lighten(fill, 0.25f), outer = darken(fill, 0.3f);
    const sf::Color rim = withAlpha(e.mark > 0.f ? sf::Color::White : lighten(fill, 0.35f),
                                    e.mark > 0.f ? 0.85f : 0.55f);

    draw::glow(window, e.pos, r * 1.7f, fill, 0.05f + 0.2f * e.hitFlash);
    const float heading = std::atan2(e.vel.y, e.vel.x);
    switch (e.kind) {
        case EnemyKind::Runner:
        case EnemyKind::Shard:
            draw::polygon(window, e.pos, r * 1.2f, 3, heading, inner, outer);
            draw::polygonOutline(window, e.pos, r * 1.2f, 3, heading, 2.f, rim);
            break;
        case EnemyKind::Tank: {
            const float spin = e.age * 0.4f;
            draw::polygon(window, e.pos, r * 1.08f, 6, spin, inner, outer);
            draw::polygonOutline(window, e.pos, r * 1.08f, 6, spin, 4.f, rim);
            break;
        }
        default:
            draw::disc(window, e.pos, r, inner, outer);
            draw::ring(window, e.pos, r, e.mark > 0.f ? 3.f : 2.f, rim);
            break;
    }

    if (e.kind == EnemyKind::Splitter) {   // a crack across the middle
        sf::RectangleShape crack({r * 1.6f, 2.f});
        crack.setOrigin(r * 0.8f, 1.f);
        crack.setPosition(e.pos);
        crack.setRotation(35.f);
        crack.setFillColor(withAlpha(theme::bg, 0.8f));
        window.draw(crack);
    }
    if (e.kind == EnemyKind::Shielded) {   // the shield arc, facing the core
        const float face = std::atan2(corePos.y - e.pos.y, corePos.x - e.pos.x);
        const float arc = cfg::enemy::shieldArc;
        draw::ring(window, e.pos, r + 5.f, 4.f,
                   withAlpha(e.frozen > 0.f ? theme::elemIce : theme::textHi, 0.9f), face - arc, face + arc);
    }
    if (e.frozen > 0.f) draw::ring(window, e.pos, r + 2.f, 2.f, withAlpha(theme::elemIce, 0.7f));
    if (frac < 0.999f)   // health left, as a thin arc - only once it's been hurt
        draw::ring(window, e.pos, r + 9.f, 2.f, withAlpha(lighten(fill, 0.4f), 0.75f), -kPi * 0.5f,
                   -kPi * 0.5f + 2.f * kPi * frac, 32);
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
    // A "Split shot" ghost is see-through and fades out over its last second.
    const float alpha = b.ghost ? 0.45f * clampf(b.ghostLife, 0.f, 1.f) : 1.f;
    if (effect && effect->kind == PowerUp::Golden) col = lerpColor(col, theme::puGolden, 0.85f);
    const float r = b.radius * popIn(b.age, 0.3f);
    if (r <= 0.5f) return;

    if (!b.held && !b.trail.empty()) {   // a soft comet tail
        const int n = static_cast<int>(b.trail.size());
        for (int k = 0; k < n; ++k) {
            const float f = static_cast<float>(k + 1) / static_cast<float>(n + 1);
            draw::disc(window, b.trail[k], r * (0.25f + 0.6f * f), withAlpha(col, (0.04f + 0.11f * f) * alpha),
                       withAlpha(col, 0.f), {1.f, 1.f}, 20);
        }
    }

    const float ax = std::fabs(b.squashAxis.x);
    const float ay = std::fabs(b.squashAxis.y);
    const float along = 1.f - 0.32f * b.squash;
    const float perpS = 1.f + 0.22f * b.squash;
    const sf::Vector2f sc{lerpf(perpS, along, ax), lerpf(perpS, along, ay)};

    draw::glow(window, b.pos, r * 1.9f, col, 0.09f * alpha);
    draw::disc(window, b.pos, r, withAlpha(lighten(col, 0.18f), alpha), withAlpha(darken(col, 0.22f), alpha), sc);

    // Role marks, kept minimal: a Guardian has a heavy rim, a Support a small
    // inner ring, a Striker a small centre dot; a Normal ball is plain.
    const bool guardian = b.role == BallRole::Guardian;
    draw::ring(window, b.pos, r, guardian ? 4.f : 1.5f,
               withAlpha(sf::Color::White, (b.held ? 0.85f : (guardian ? 0.5f : 0.22f)) * alpha));
    if (b.role == BallRole::Support)
        draw::ring(window, b.pos, r * 0.45f, 2.f, withAlpha(sf::Color::White, 0.65f * alpha));
    else if (b.role == BallRole::Striker)
        draw::disc(window, b.pos, r * 0.24f, withAlpha(sf::Color::White, 0.85f * alpha),
                   withAlpha(sf::Color::White, 0.6f * alpha), {1.f, 1.f}, 16);
    if (b.mods.mastery)   // mastered: a second, outer halo ring
        draw::ring(window, b.pos, r + 5.f, 1.5f, withAlpha(lighten(col, 0.5f), 0.6f * alpha));

    // A specular highlight, top-left: reads as a solid, shiny ball.
    draw::disc(window, b.pos + sf::Vector2f{-0.34f, -0.38f} * r, r * 0.26f,
               withAlpha(sf::Color::White, 0.22f * alpha), withAlpha(sf::Color::White, 0.f), {1.f, 0.8f}, 16);

    if (b.held) draw::ring(window, b.pos, r + 8.f, 2.f, withAlpha(theme::accent, 0.75f));
}

void WorldRenderer::draw(sf::RenderWindow& window, const World& world) const {
    for (const Ball& b : world.balls())
        if (b.element == Element::Water) drawWaterTrail(window, b);
    for (const Obstacle& o : world.obstacles()) drawObstacle(window, o);
    for (const BlackHole& h : world.blackHoles()) drawBlackHole(window, h);
    drawCore(window, world.core());
    drawBoss(window, world.boss(), world.core().pos);
    for (const Enemy& e : world.enemies()) drawEnemy(window, e, world.core().pos);
    for (const Ball& b : world.balls()) {
        if (b.mods.hunterMult <= 0.f || b.preyId < 0) continue;
        for (const Enemy& e : world.enemies())
            if (e.id == b.preyId) drawHunterLock(window, b, e);
    }
    for (const TetherBeam& t : world.tethers()) drawTether(window, t);
    for (const Bolt& bo : world.bolts()) drawBolt(window, bo);
    for (const Pickup& pu : world.pickups()) drawPickup(window, pu);
    for (const Ball& g : world.ghosts()) drawBall(window, g, world.effect());
    for (const Ball& b : world.balls()) drawBall(window, b, world.effect());
}

}  // namespace sb
