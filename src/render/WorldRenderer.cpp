#include "render/WorldRenderer.hpp"

#include <cmath>

#include "render/ClassRender.hpp"
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

// A ring cut into `n` equal segments with small gaps, filled up to `frac`
// (clockwise from the top); the rest shows as a faint track.
void segmentedRing(sf::RenderTarget& t, sf::Vector2f c, float r, float thick, int n, float frac,
                   sf::Color on, sf::Color off) {
    const float step = 2.f * kPi / static_cast<float>(n);
    const float gap = std::min(0.06f, step * 0.25f);
    for (int i = 0; i < n; ++i) {
        const float a0 = -kPi * 0.5f + step * static_cast<float>(i) + gap * 0.5f;
        const float fill = clampf(frac * static_cast<float>(n) - static_cast<float>(i), 0.f, 1.f);
        draw::ring(t, c, r, thick, off, a0, a0 + step - gap, 12);
        if (fill > 0.f) draw::ring(t, c, r, thick, on, a0, a0 + (step - gap) * fill, 12);
    }
}

// A small dart / chevron pointing along `heading`: tip, two back corners and a
// notch between them. Star-shaped around its centre, so a fan draws it.
void dart(sf::RenderTarget& t, sf::Vector2f p, float r, float heading, sf::Color inner, sf::Color outer,
          sf::Color rim, float rimW) {
    const float back = 2.45f;   // back corners at +-140 degrees
    const sf::Vector2f pts[4] = {
        p + sf::Vector2f{std::cos(heading), std::sin(heading)} * (r * 1.35f),
        p + sf::Vector2f{std::cos(heading + back), std::sin(heading + back)} * (r * 1.1f),
        p - sf::Vector2f{std::cos(heading), std::sin(heading)} * (r * 0.35f),
        p + sf::Vector2f{std::cos(heading - back), std::sin(heading - back)} * (r * 1.1f)};
    sf::VertexArray fan(sf::TriangleFan, 6);
    fan[0] = {p + sf::Vector2f{std::cos(heading), std::sin(heading)} * (r * 0.2f), inner};
    for (int i = 0; i < 5; ++i) fan[static_cast<std::size_t>(i) + 1] = {pts[i % 4], outer};
    t.draw(fan);
    for (int i = 0; i < 4; ++i) draw::line(t, pts[i], pts[(i + 1) % 4], rimW, rim);
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

    // The reactor: a dark shaded body, a bright eye, slow-turning inner vanes,
    // health as a segmented dial around it and a faint rotating tick bezel.
    draw::glow(window, c.pos, c.radius * 2.4f, tint, 0.06f + 0.03f * breathe + 0.3f * c.hitFlash);
    draw::disc(window, c.pos, c.radius, withAlpha(lerpColor(darken(tint, 0.45f), sf::Color::White, 0.3f * c.hitFlash), 0.95f),
               withAlpha(darken(tint, 0.78f), 0.95f));
    draw::ring(window, c.pos, c.radius, 1.5f, withAlpha(tint, 0.55f));
    segmentedRing(window, c.pos, c.radius + 8.f, 4.f, 24, frac, withAlpha(tint, 0.95f),
                  withAlpha(theme::arenaEdge, 0.55f));
    const float spin = t * 0.15f;
    for (int k = 0; k < 36; ++k) {
        const float a = spin + static_cast<float>(k) * 2.f * kPi / 36.f;
        const sf::Vector2f d{std::cos(a), std::sin(a)};
        const float len = k % 9 == 0 ? 7.f : 3.f;
        draw::line(window, c.pos + d * (c.radius + 15.f), c.pos + d * (c.radius + 15.f + len), 1.f,
                   withAlpha(tint, k % 9 == 0 ? 0.45f : 0.22f));
    }
    for (int k = 0; k < 3; ++k) {
        const float a = t * 0.8f + static_cast<float>(k) * 2.f * kPi / 3.f;
        draw::ring(window, c.pos, c.radius * 0.62f, 2.5f, withAlpha(lighten(tint, 0.3f), 0.6f), a, a + 1.2f);
    }
    draw::disc(window, c.pos, c.radius * 0.26f, withAlpha(lighten(tint, 0.7f), 0.95f), withAlpha(tint, 0.8f));
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
    // A faceted stone block: shaded hexagon, crisp rim, two facet lines.
    const float f = o.maxLife > 0.f ? clampf(o.life / o.maxLife, 0.f, 1.f) : 0.f;
    const float a = 0.45f + 0.5f * f;
    const float rot = 0.35f;
    draw::polygon(window, o.pos, o.radius, 6, rot, withAlpha(lighten(theme::elemStone, 0.1f), a),
                  withAlpha(darken(theme::elemStone, 0.45f), a));
    draw::polygonOutline(window, o.pos, o.radius, 6, rot, 1.5f, withAlpha(lighten(theme::elemStone, 0.3f), 0.7f * a));
    for (int k : {0, 2}) {
        const float ang = rot + static_cast<float>(k) * kPi / 3.f;
        draw::line(window, o.pos, o.pos + sf::Vector2f{std::cos(ang), std::sin(ang)} * o.radius, 1.f,
                   withAlpha(darken(theme::elemStone, 0.6f), 0.6f * a));
    }
}

void WorldRenderer::drawBolt(sf::RenderWindow& window, const Bolt& bo) const {
    const float f = bo.maxLife > 0.f ? clampf(bo.life / bo.maxLife, 0.f, 1.f) : 0.f;
    if (bo.beam) {   // Railgun: a gold beam with a white-hot core, thinning as it fades
        const float thick = 2.f + 8.f * f;
        draw::line(window, bo.a, bo.b, thick, withAlpha(theme::puGolden, 0.2f + 0.45f * f));
        draw::line(window, bo.a, bo.b, std::max(1.f, thick * 0.3f), withAlpha(lighten(theme::puGolden, 0.7f), 0.4f + 0.6f * f));
        return;
    }
    // A jagged 3-segment arc between the two endpoints: a soft wide pass under
    // a thin bright one.
    const sf::Vector2f d = bo.b - bo.a;
    const sf::Vector2f n = normalized({-d.y, d.x});
    sf::Vector2f pts[4];
    for (int i = 0; i < 4; ++i) {
        const float t = static_cast<float>(i) / 3.f;
        float off = 0.f;
        if (i == 1) off = 7.f;
        else if (i == 2) off = -6.f;
        pts[i] = bo.a + d * t + n * off;
    }
    for (int i = 0; i < 3; ++i) {
        draw::line(window, pts[i], pts[i + 1], 4.f, withAlpha(theme::elemElectric, 0.18f * f));
        draw::line(window, pts[i], pts[i + 1], 1.5f, withAlpha(lighten(theme::elemElectric, 0.45f), 0.4f + 0.55f * f));
    }
}

void WorldRenderer::drawBoss(sf::RenderWindow& window, const Boss& b, sf::Vector2f corePos) const {
    if (!b.alive) return;
    const float frac = b.maxHp > 0.f ? clampf(b.hp / b.maxHp, 0.f, 1.f) : 0.f;
    const float t = clockSeconds();

    // Orbital boss: draw a faint guide along the spiral it's about to travel, so
    // the player can read where it comes from and how it moves (dotted).
    if (b.kind == BossKind::Orbital) {
        float ang = b.ang, dist = b.dist;
        if (b.intro > 0.f) {  // during the slide-in the spiral hasn't started
            ang = cfg::finalBoss::startAngle;
        }
        for (int i = 0; i < 30; ++i) {
            ang += cfg::finalBoss::spiralOmega * 0.05f;
            dist = std::max(0.f, dist - cfg::finalBoss::spiralShrink * 0.05f);
            const sf::Vector2f pt = corePos + sf::Vector2f{std::cos(ang), std::sin(ang)} * dist;
            const float a = 0.35f * (1.f - static_cast<float>(i) / 30.f);
            draw::disc(window, pt, 2.f, withAlpha(theme::coreLow, a), withAlpha(theme::coreLow, a), {1.f, 1.f}, 8);
        }
    }

    // Charger: before a shockwave, its reach tightens in as a pulsing ring.
    if (b.kind == BossKind::Charger && b.timer < cfg::boss::shockWarn) {
        const float k = 1.f - clampf(b.timer / cfg::boss::shockWarn, 0.f, 1.f);   // 0 -> 1
        const float pulse = 0.5f + 0.5f * std::sin(t * 30.f);
        draw::ring(window, b.pos, b.shockR, 2.f, withAlpha(theme::coreLow, 0.15f + 0.45f * k * pulse));
        draw::ring(window, b.pos, b.radius + (b.shockR - b.radius) * (1.f - k), 3.f,
                   withAlpha(theme::coreLow, 0.25f + 0.5f * k));
    }

    // Dasher: while it aims, the dash line it's about to take (brightening).
    if (b.kind == BossKind::Dasher && b.phase == 1) {
        const float k = 1.f - clampf(b.timer / cfg::boss::dasherAim, 0.f, 1.f);
        const float len = cfg::boss::dasherDashSpeed * cfg::boss::dasherDash + b.radius;
        for (int i = 1; i <= 14; ++i) {
            const sf::Vector2f pt = b.pos + b.dashDir * (b.radius + len * static_cast<float>(i) / 14.f);
            draw::disc(window, pt, 2.5f, withAlpha(theme::coreLow, 0.2f + 0.6f * k),
                       withAlpha(theme::coreLow, 0.2f + 0.6f * k), {1.f, 1.f}, 8);
        }
    }

    // A heavy armoured body: dark plated, bright rim, a slow counter-turning
    // inner ring and a single glowing eye. The sides tell the bosses apart:
    // Charger / Orbital octagon, Hive hexagon, Warden square, Dasher triangle.
    const sf::Color col = lerpColor(theme::enemy, sf::Color::White, b.hitFlash * 0.7f);
    const float intro = b.intro > 0.f ? 0.5f + 0.5f * std::sin(b.intro * 12.f) : 0.f;
    // The Hive swells before a burst.
    const float swell = b.kind == BossKind::Hive ? clampf(1.f - b.timer / 0.7f, 0.f, 1.f) : 0.f;
    const float rage = b.enraged ? 0.18f + 0.12f * std::sin(t * 7.f) : 0.f;   // phase two: a hot pulse
    draw::glow(window, b.pos, b.radius * (b.enraged ? 2.2f : 1.8f), theme::coreLow,
               0.08f + 0.12f * intro + 0.25f * b.hitFlash + 0.3f * swell + rage);
    int sides = 8;
    float rot = t * 0.25f;
    switch (b.kind) {
        case BossKind::Hive:   sides = 6; rot = t * 0.15f; break;
        case BossKind::Warden: sides = 4; rot = b.shieldAng + kPi * 0.25f; break;
        case BossKind::Dasher: sides = 3; rot = std::atan2(b.dashDir.y, b.dashDir.x); break;
        default: break;
    }
    const float br = b.radius * 1.04f * (1.f + 0.06f * swell);
    draw::polygon(window, b.pos, br, sides, rot, withAlpha(darken(col, 0.45f), 0.97f),
                  withAlpha(darken(col, 0.72f), 0.97f));
    draw::polygonOutline(window, b.pos, br, sides, rot, 3.f, withAlpha(lighten(col, 0.15f), 0.9f));
    draw::polygonOutline(window, b.pos, br * 0.77f, sides, rot, 1.f, withAlpha(col, 0.35f));
    if (b.kind == BossKind::Warden) {   // the shield: a thick bright arc that turns around it
        const float arc = cfg::boss::wardenShieldArc;
        draw::ring(window, b.pos, b.radius + 10.f, 6.f, withAlpha(theme::textHi, 0.95f), b.shieldAng - arc,
                   b.shieldAng + arc, 40);
        draw::ring(window, b.pos, b.radius + 18.f, 1.5f, withAlpha(theme::textHi, 0.35f), b.shieldAng - arc * 0.7f,
                   b.shieldAng + arc * 0.7f, 32);
    }
    for (int k = 0; k < 4; ++k) {
        const float a = -t * 0.9f + static_cast<float>(k) * kPi * 0.5f;
        draw::ring(window, b.pos, b.radius * 0.58f, 3.f, withAlpha(col, 0.7f), a, a + 0.9f, 24);
    }
    draw::disc(window, b.pos, b.radius * 0.26f, withAlpha(lighten(col, 0.55f), 0.95f), withAlpha(col, 0.85f));

    // Health bar just above the boss: segmented, with bracket ends.
    const float bw = 96.f, bh = 5.f;
    const float y = b.pos.y - b.radius - 20.f;
    const sf::FloatRect bar{b.pos.x - bw * 0.5f, y - bh * 0.5f, bw, bh};
    sf::RectangleShape track({bw, bh});
    track.setPosition(bar.left, bar.top);
    track.setFillColor(withAlpha(theme::arenaEdge, 0.8f));
    window.draw(track);
    sf::RectangleShape hp({bw * frac, bh});
    hp.setPosition(bar.left, bar.top);
    hp.setFillColor(theme::coreLow);
    window.draw(hp);
    for (int i = 1; i < 8; ++i) {
        const float x = bar.left + bw * static_cast<float>(i) / 8.f;
        draw::line(window, {x, bar.top}, {x, bar.top + bh}, 1.5f, withAlpha(theme::bg, 0.9f));
    }
    draw::brackets(window, {bar.left - 4.f, bar.top - 4.f, bw + 8.f, bh + 8.f}, 5.f, 1.5f,
                   withAlpha(theme::coreLow, 0.7f));
}

void WorldRenderer::drawEnemy(sf::RenderWindow& window, const Enemy& e, sf::Vector2f corePos) const {
    const float frac = e.maxHp > 0.f ? clampf(e.hp / e.maxHp, 0.f, 1.f) : 0.f;
    const float r = e.radius * popIn(e.age, 0.28f);   // spawns pop in
    if (r <= 0.5f) return;
    // Enemies are dark hulls with a crisp bright rim - solid, readable, but
    // never as bright as a ball. Each kind has its own silhouette: grunt disc
    // with an eye, runner / shard darts, tank double hexagon, splitter cracked
    // disc, shielded disc with a bright arc facing the core.
    sf::Color base = theme::enemy;
    if (e.kind == EnemyKind::Runner || e.kind == EnemyKind::Shard) base = lerpColor(theme::enemy, theme::ballFast, 0.35f);
    if (e.kind == EnemyKind::Tank) base = lerpColor(theme::enemy, theme::bg, 0.15f);
    sf::Color fill = base;
    if (e.poison > 0.f) fill = lerpColor(fill, theme::elemPoison, 0.5f);
    if (e.burn > 0.f)   fill = lerpColor(fill, theme::elemFire, 0.5f);
    if (e.frozen > 0.f) fill = lerpColor(fill, theme::elemIce, 0.65f);
    fill = lerpColor(fill, sf::Color::White, e.hitFlash * 0.8f);
    const float hull = 0.42f * (1.f - e.hitFlash);   // how dark the body sits under its rim
    const sf::Color inner = darken(fill, hull), outer = darken(fill, hull + 0.28f);
    const sf::Color rim = e.mark > 0.f ? sf::Color::White : withAlpha(lighten(fill, 0.12f), 0.95f);
    const float rimW = e.mark > 0.f ? 3.f : 2.f;

    if (e.hitFlash > 0.02f) draw::glow(window, e.pos, r * 1.7f, fill, 0.25f * e.hitFlash);
    const float heading = std::atan2(e.vel.y, e.vel.x);
    switch (e.kind) {
        case EnemyKind::Runner:
        case EnemyKind::Shard:
            dart(window, e.pos, r, heading, inner, outer, rim, rimW);
            break;
        case EnemyKind::Blinker: {   // a triangle pointing at the core; flickers just before a jump
            const float face = std::atan2(corePos.y - e.pos.y, corePos.x - e.pos.x);
            const float warn = e.blinkT < 0.45f ? 0.5f + 0.5f * std::sin(e.blinkT * 40.f) : 0.f;
            draw::polygon(window, e.pos, r * 1.15f, 3, face, inner, outer);
            draw::polygonOutline(window, e.pos, r * 1.15f, 3, face, rimW, withAlpha(rim, 1.f - 0.6f * warn));
            if (e.blinkFx > 0.f) {   // where it jumped from: a fading outline
                draw::polygonOutline(window, e.blinkFrom, r * 1.15f, 3, face, 1.5f, withAlpha(rim, 0.5f * e.blinkFx));
                draw::line(window, e.blinkFrom, e.pos, 1.f, withAlpha(rim, 0.25f * e.blinkFx));
            }
            break;
        }
        case EnemyKind::Brute: {     // the miniboss: a big heavy pentagon with an ember core
            const float spin = e.age * 0.25f;
            draw::glow(window, e.pos, r * 1.5f, theme::ember, 0.12f);
            draw::polygon(window, e.pos, r * 1.06f, 5, spin, inner, outer);
            draw::polygonOutline(window, e.pos, r * 1.06f, 5, spin, 3.5f, rim);
            draw::polygonOutline(window, e.pos, r * 0.7f, 5, -spin, 1.5f, withAlpha(theme::ember, 0.6f));
            draw::disc(window, e.pos, r * 0.22f, withAlpha(theme::ember, 0.95f), withAlpha(darken(theme::ember, 0.3f), 0.9f));
            break;
        }
        case EnemyKind::Tank: {
            const float spin = e.age * 0.4f;
            draw::polygon(window, e.pos, r * 1.08f, 6, spin, inner, outer);
            draw::polygonOutline(window, e.pos, r * 1.08f, 6, spin, 3.f, rim);
            draw::polygonOutline(window, e.pos, r * 0.62f, 6, spin, 1.5f, withAlpha(fill, 0.6f));
            break;
        }
        default:
            draw::disc(window, e.pos, r, inner, outer);
            draw::ring(window, e.pos, r, rimW, rim);
            break;
    }

    if (e.kind == EnemyKind::Mender) {   // a plus, and a faint ring where it heals
        const sf::Color pc = withAlpha(theme::venom, 0.9f);
        draw::line(window, e.pos - sf::Vector2f{r * 0.45f, 0.f}, e.pos + sf::Vector2f{r * 0.45f, 0.f}, 3.f, pc);
        draw::line(window, e.pos - sf::Vector2f{0.f, r * 0.45f}, e.pos + sf::Vector2f{0.f, r * 0.45f}, 3.f, pc);
        draw::ring(window, e.pos, cfg::enemy::mendRadius, 1.f, withAlpha(theme::venom, 0.10f), 0.f, 2.f * kPi, 48);
    }
    if (e.kind == EnemyKind::Grunt)   // a small bright eye
        draw::disc(window, e.pos, r * 0.26f, withAlpha(lighten(fill, 0.3f), 0.95f), withAlpha(fill, 0.8f), {1.f, 1.f}, 16);
    if (e.kind == EnemyKind::Splitter) {   // a zig-zag crack right through the middle
        const sf::Vector2f u{std::cos(0.6f), std::sin(0.6f)}, v{-u.y, u.x};
        const sf::Vector2f p0 = e.pos - u * r * 0.95f, p1 = e.pos - u * r * 0.3f + v * r * 0.22f,
                           p2 = e.pos + u * r * 0.3f - v * r * 0.22f, p3 = e.pos + u * r * 0.95f;
        const sf::Color crack = withAlpha(lighten(fill, 0.2f), 0.9f);
        draw::line(window, p0, p1, 2.f, crack);
        draw::line(window, p1, p2, 2.f, crack);
        draw::line(window, p2, p3, 2.f, crack);
    }
    if (e.kind == EnemyKind::Shielded) {   // the shield arc, facing the core
        const float face = std::atan2(corePos.y - e.pos.y, corePos.x - e.pos.x);
        const float arc = cfg::enemy::shieldArc;
        const sf::Color sc = e.frozen > 0.f ? theme::elemIce : theme::textHi;
        draw::ring(window, e.pos, r + 5.f, 4.f, withAlpha(sc, 0.95f), face - arc, face + arc);
        draw::ring(window, e.pos, r + 10.f, 1.f, withAlpha(sc, 0.4f), face - arc * 0.7f, face + arc * 0.7f);
        draw::disc(window, e.pos, r * 0.22f, withAlpha(lighten(fill, 0.3f), 0.9f), withAlpha(fill, 0.8f), {1.f, 1.f}, 12);
    }
    if (e.frozen > 0.f) draw::ring(window, e.pos, r + 2.f, 2.f, withAlpha(theme::elemIce, 0.7f));
    if (frac < 0.999f)   // health left, as a thin arc - only once it's been hurt
        draw::ring(window, e.pos, r + 8.f, 2.f, withAlpha(lighten(fill, 0.4f), 0.75f), -kPi * 0.5f,
                   -kPi * 0.5f + 2.f * kPi * frac, 32);
}

void WorldRenderer::drawPickup(sf::RenderWindow& window, const Pickup& pu) const {
    // A power-up is a diamond (the only square thing in the arena, so it never
    // reads as a ball or an enemy): a filled centre and a slowly turning frame.
    const sf::Color col = powerUpColor(pu.kind);
    const float pulse = 0.5f + 0.5f * std::sin(pu.age * 4.f);
    const float fade =
        pu.age > pu.ttl - 2.f ? clampf((pu.ttl - pu.age) / 2.f, 0.f, 1.f) : 1.f;
    const float r = pu.radius * popIn(pu.age, 0.3f);
    if (r <= 0.5f) return;

    draw::glow(window, pu.pos, r * 2.f, col, 0.10f * fade);
    draw::polygon(window, pu.pos, r * 0.55f, 4, 0.f, withAlpha(lighten(col, 0.4f), fade), withAlpha(col, fade));
    const float spin = pu.age * 1.2f;
    draw::polygonOutline(window, pu.pos, r + 2.f + 2.f * pulse, 4, spin, 2.f, withAlpha(col, 0.85f * fade));
}

void WorldRenderer::drawBall(sf::RenderWindow& window, const Ball& b,
                             const std::optional<ActiveEffect>& effect) const {
    sf::Color col = b.color;   // World bakes the lead class's hue into b.color (see World::ballTint)
    // A "Split shot" ghost is see-through and fades out over its last second.
    const float alpha = b.ghost ? 0.45f * clampf(b.ghostLife, 0.f, 1.f) : 1.f;
    if (effect && effect->kind == PowerUp::Golden) col = lerpColor(col, theme::puGolden, 0.85f);
    const float r = b.radius * popIn(b.age, 0.3f);
    if (r <= 0.5f) return;

    if (!b.held && b.trail.size() >= 2) {   // a clean tapering streak along its path
        // the element tints the streak (muted) - the body stays the class's
        const sf::Color tc = b.element == Element::Plain ? col : lerpColor(col, elementColor(b.element), 0.55f);
        const int n = static_cast<int>(b.trail.size());
        sf::VertexArray ribbon(sf::TriangleStrip);
        for (int k = 0; k <= n; ++k) {
            const sf::Vector2f p = k < n ? b.trail[static_cast<std::size_t>(k)] : b.pos;
            const sf::Vector2f q = k < n ? (k + 1 < n ? b.trail[static_cast<std::size_t>(k + 1)] : b.pos)
                                         : b.pos + (b.pos - b.trail.back());
            const sf::Vector2f dir = normalized(q - p, {1.f, 0.f});
            const sf::Vector2f nrm{-dir.y, dir.x};
            const float f = static_cast<float>(k) / static_cast<float>(n);   // 0 tail -> 1 head
            const sf::Color c = withAlpha(tc, 0.22f * f * f * alpha);
            ribbon.append({p + nrm * (r * 0.8f * f), c});
            ribbon.append({p - nrm * (r * 0.8f * f), c});
        }
        window.draw(ribbon);
    }

    const float ax = std::fabs(b.squashAxis.x);
    const float ay = std::fabs(b.squashAxis.y);
    const float along = 1.f - 0.32f * b.squash;
    const float perpS = 1.f + 0.22f * b.squash;
    const sf::Vector2f sc{lerpf(perpS, along, ax), lerpf(perpS, along, ay)};

    const BallLook look = ballLook(b);
    const bool rich = (look.ascended & look.roles) != 0;
    draw::glow(window, b.pos, r * (rich ? 2.f : 1.6f), col, (rich ? 0.11f : 0.06f) * alpha);
    draw::disc(window, b.pos, r, withAlpha(lighten(col, 0.2f), alpha), withAlpha(darken(col, 0.18f), alpha), sc);

    // Who it is (render/ClassRender.cpp): element rim, second-class arc, class
    // marks, ascended halo. A Normal ball is plain.
    const float heading = std::atan2(b.vel.y, b.vel.x);
    drawBallIdentity(window, look, b.pos, r, heading, alpha, b.held);
    if (!b.ghost) drawClassPulse(window, look.lead, b.pulseAscend, b.pos, r, b.classPulse);
    if (!b.ghost) drawElementPulse(window, b.element, b.pos, r, b.elemPulse);

    // Ability charge: a hairline arc per ability slot just outside the rim,
    // filling as it recharges; it flashes when one fires.
    if (!b.ghost) {
        int n = 0;
        for (const AbilitySpec& a : b.abilities)
            if (a.id != Ability::None) ++n;
        if (n > 0) {
            const float step = 2.f * kPi / static_cast<float>(n);
            const float gap = n > 1 ? 0.35f : 0.f;
            int k = 0;
            for (int i = 0; i < kMaxAbilitySlots; ++i) {
                const AbilitySpec& a = b.abilities[i];
                if (a.id == Ability::None) continue;
                const float full = abilityCooldown(a.id, a.level);
                const float charge = full > 0.f ? clampf(1.f - b.abilityCd[i] / full, 0.f, 1.f) : 1.f;
                const float a0 = -kPi * 0.5f + step * static_cast<float>(k++) + gap * 0.5f;
                const float span = step - gap;
                const float rr = r + 3.5f;
                draw::ring(window, b.pos, rr, 1.5f, withAlpha(col, 0.10f * alpha), a0, a0 + span, 24);
                const float lit = (charge >= 1.f ? 0.45f : 0.26f) + 0.5f * b.abilityFlash;
                if (charge > 0.f)
                    draw::ring(window, b.pos, rr, 1.5f, withAlpha(lighten(col, 0.35f), std::min(1.f, lit) * alpha),
                               a0, a0 + span * charge, 24);
            }
        }
    }

    // A specular highlight, top-left: reads as a solid, shiny ball.
    draw::disc(window, b.pos + sf::Vector2f{-0.34f, -0.38f} * r, r * 0.26f,
               withAlpha(sf::Color::White, 0.24f * alpha), withAlpha(sf::Color::White, 0.f), {1.f, 0.8f}, 16);

    if (b.held) {   // grabbed: a bracketed target lock around it
        const float h = r + 9.f;
        draw::brackets(window, {b.pos.x - h, b.pos.y - h, 2.f * h, 2.f * h}, 7.f, 2.f, withAlpha(theme::accent, 0.85f));
    }
}

void WorldRenderer::draw(sf::RenderWindow& window, const World& world) const {
    // The radar grid: rings and spokes around the core, the arena's own
    // coordinate system. Very faint - it's there to be felt, not read.
    const sf::Vector2f sz = world.size();
    const float as = world.arenaScale();
    draw::radar(window, world.core().pos, length(sz) * 0.6f, 120.f * as, 12, theme::grid, 0.10f);

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
    drawClassWorld(window, world);   // per-class things on the field (ClassRender.cpp)
    for (const Ball& g : world.ghosts()) drawBall(window, g, world.effect());
    for (const Ball& b : world.balls()) drawBall(window, b, world.effect());
}

}  // namespace sb
