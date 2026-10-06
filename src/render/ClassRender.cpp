// Class visuals: each class's mark on the ball and its own things on the
// field. One section per class; the dispatch is at the bottom.
//
// Marks layer from the outside in so two classes read together: Guardian = a
// heavy rim, Summoner = motes circling inside the rim, Assassin = a thin arc
// trailing behind, Support = an inner ring, Mage = a small diamond, Jester =
// two pips, Striker = a centre dot, Shooter = a short barrel toward its
// heading. White at low alpha over the ball's class-coloured body.

#include "render/ClassRender.hpp"

#include <cmath>

#include "render/Draw.hpp"

namespace sb {

namespace {

float classClock() {   // a shared clock for idle motion (the Summoner's motes)
    static sf::Clock clock;
    return clock.getElapsedTime().asSeconds();
}

sf::Color white(float a) { return withAlpha(sf::Color::White, a); }

// ==================================================================== Striker
void markStriker(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {
    draw::disc(t, p, r * 0.24f, white(0.85f * a), white(0.6f * a), {1.f, 1.f}, 16);
}

// ==================================================================== Guardian
void markGuardian(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {
    draw::ring(t, p, r, 4.f, white(0.5f * a));
}

// ==================================================================== Support
void markSupport(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {
    draw::ring(t, p, r * 0.45f, 2.f, white(0.65f * a));
}

// ==================================================================== Mage
void markMage(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {
    draw::polygonOutline(t, p, r * 0.3f, 4, 0.f, 1.5f, white(0.7f * a));
}
// Magic missiles: a small dim dot with a short curved trail, tinted by its
// element (Mage blue when plain) - dimmer than any ball. A Mage ball that just
// cast: its diamond swells out and fades (the cast flash).
void worldMage(sf::RenderTarget& t, const World& world) {
    const float k = world.arenaScale();
    for (const MageWorld::Missile& m : world.classWorld().mage.missiles) {
        const sf::Color base = m.elem != 0 ? elementColor(static_cast<Element>(m.elem)) : theme::classMage;
        const sf::Color c = lerpColor(base, sf::Color::White, 0.35f);
        const float fade = clampf(m.life / 0.2f, 0.f, 1.f);
        for (int i = 0; i + 1 < m.trailN; ++i) {
            const float f = 1.f - static_cast<float>(i) / 6.f;
            draw::line(t, m.trail[i], m.trail[i + 1], (1.8f * f + 0.4f) * k, withAlpha(c, 0.28f * f * fade));
        }
        draw::disc(t, m.pos, 2.8f * k, withAlpha(c, 0.8f * fade), withAlpha(c, 0.35f * fade), {1.f, 1.f}, 10);
    }
    for (const Ball& b : world.balls()) {
        if (!b.hasRole(BallRole::Mage) || b.abilityFlash <= 0.f) continue;
        draw::polygonOutline(t, b.pos, b.radius * (0.3f + 1.1f * (1.f - b.abilityFlash)), 4, 0.f, 1.5f,
                             white(0.45f * b.abilityFlash));
    }
}

// ==================================================================== Shooter
void markShooter(sf::RenderTarget& t, sf::Vector2f p, float r, float heading, float a) {
    const sf::Vector2f d{std::cos(heading), std::sin(heading)};
    draw::line(t, p + d * (r * 0.4f), p + d * (r * 0.92f), 2.f, white(0.7f * a));
}
// Bullets: a tiny dot with a short fading streak behind it, white (or the
// element's colour when it carries one) at low alpha - never brighter than a
// ball. Deadeye's rail: one thin line that fades fast.
void worldShooter(sf::RenderTarget& t, const World& world) {
    const ShooterWorld& sw = world.classWorld().shooter;
    const float k = world.arenaScale();
    for (const ShooterRail& r : sw.rails) {
        if (r.width > 0.f) {   // "Ion cannon": a thick beam that thins out
            const float f = clampf(r.life / std::max(0.01f, r.maxLife), 0.f, 1.f);
            draw::line(t, r.a, r.b, 2.f * r.width * f, withAlpha(theme::classShooter, 0.35f * f));
            draw::line(t, r.a, r.b, 0.7f * r.width * f + 1.f, white(0.85f * f));
            continue;
        }
        const float f = clampf(r.life / cfg::shooter::deadeyeRailLife, 0.f, 1.f);
        draw::line(t, r.a, r.b, 1.f + 1.5f * f, white(0.45f * f));
    }
    for (const ShooterBullet& u : sw.bullets) {
        const sf::Color c = u.elem != 0 ? lerpColor(elementColor(static_cast<Element>(u.elem)), sf::Color::White, 0.3f) : sf::Color::White;
        const float fade = clampf(u.life / 0.15f, 0.f, 1.f);
        draw::line(t, u.pos - u.vel * 0.03f, u.pos, 2.f * k, withAlpha(c, 0.3f * fade));
        draw::disc(t, u.pos, 2.6f * k, withAlpha(c, 0.85f * fade), withAlpha(c, 0.4f * fade), {1.f, 1.f}, 10);
        if (u.smart > 0.f) draw::ring(t, u.pos, 4.5f * k, 1.f * k, withAlpha(c, 0.5f * fade), 0.f, 6.283f, 10);   // "Smart rounds"
    }
}

// ==================================================================== Assassin
void markAssassin(sf::RenderTarget& t, sf::Vector2f p, float r, float heading, float a) {
    const float back = heading + kPi;   // a thin cloak trailing behind it
    draw::ring(t, p, r * 0.72f, 1.5f, white(0.6f * a), back - 0.9f, back + 0.9f, 16);
}
// Each blink: a thin fading line from where it left to where it landed, and
// a faint afterimage ring where it was. A path that cut (Shadow trail /
// Shadow Assassin) is a touch thicker, in the class colour.
void worldAssassin(sf::RenderTarget& t, const World& world) {
    for (const AssassinWorld::Blink& f : world.classWorld().assassin.blinks) {
        const float k = clampf(f.life / cfg::assassin::blinkFxLife, 0.f, 1.f);
        const sf::Color c = f.cuts ? withAlpha(theme::classAssassin, 0.55f * k) : white(0.3f * k);
        draw::line(t, f.a, f.b, f.cuts ? 2.5f : 1.5f, c);
        if (f.r > 0.f) draw::ring(t, f.a, f.r * (1.f + 0.25f * (1.f - k)), 1.5f, white(0.35f * k));
    }
}

// ==================================================================== Summoner
void markSummoner(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {
    const float spin = classClock() * 1.4f;
    for (int i = 0; i < 3; ++i) {
        const float ang = spin + 2.f * kPi * static_cast<float>(i) / 3.f;
        draw::disc(t, p + sf::Vector2f{std::cos(ang), std::sin(ang)} * (r * 0.66f), std::max(1.2f, r * 0.09f),
                   white(0.75f * a), white(0.5f * a), {1.f, 1.f}, 10);
    }
}
// Summons are small, see-through and tinted by the summoner's element - always
// clearly secondary to the balls. Turret = a small triangle with a barrel,
// shot / wisp = a dot, totem = a hexagon with a faint zone ring, warden = a
// small mote on a ring around the core, dragonling = a tiny winged body with a
// brief cone of breath.
sf::Color summonColor(int elem) {
    const Element e = static_cast<Element>(elem);
    return e == Element::Plain ? theme::classSummoner : lerpColor(elementColor(e), sf::Color::White, 0.25f);
}

void worldSummoner(sf::RenderTarget& t, const World& world) {
    const SummonerWorld& sw = world.classWorld().summoner;

    for (const SummonTotem& to : sw.totems) {
        const float fade = clampf(to.life / 0.6f, 0.f, 1.f) * clampf((to.maxLife - to.life) / 0.2f, 0.f, 1.f);
        const sf::Color c = summonColor(to.elem);
        draw::ring(t, to.pos, to.radius, 1.f, withAlpha(c, (0.10f + 0.12f * to.flash) * fade), 0.f, 2.f * kPi, 56);
        draw::polygon(t, to.pos, 6.f, 6, 0.f, withAlpha(c, 0.5f * fade), withAlpha(c, 0.3f * fade));
    }

    for (const DragonBreath& br : sw.breaths) {
        const float k = clampf(br.life / cfg::summoner::breathLife, 0.f, 1.f);
        const sf::Color c = withAlpha(summonColor(br.elem), 0.16f * k);
        sf::VertexArray fan(sf::TriangleFan);
        fan.append({br.pos, c});
        const float reach = br.range * (1.1f - 0.3f * k);   // it rolls outward as it fades
        for (int i = 0; i <= 8; ++i) {
            const float a = br.dir - br.cone + 2.f * br.cone * static_cast<float>(i) / 8.f;
            fan.append({br.pos + sf::Vector2f{std::cos(a), std::sin(a)} * reach, withAlpha(c, 0.f)});
        }
        t.draw(fan);
    }

    for (const SummonTurret& tu : sw.turrets) {
        const float fade = clampf(tu.life, 0.f, 1.f);
        const sf::Color c = summonColor(tu.elem);
        const sf::Vector2f d{std::cos(tu.aim), std::sin(tu.aim)};
        draw::line(t, tu.pos, tu.pos + d * 11.f, 2.f, withAlpha(c, 0.6f * fade));
        draw::polygon(t, tu.pos, 7.f, 3, tu.aim, withAlpha(c, 0.55f * fade), withAlpha(c, 0.35f * fade));
    }

    for (const SummonShot& s : sw.shots) {
        const sf::Color c = summonColor(s.elem);
        if (s.wisp) {
            const float r = cfg::summoner::wispRadius * (s.great ? 2.4f : 1.f);   // "Spirit host": a great one
            draw::glow(t, s.pos, s.great ? 26.f : 10.f, c, s.great ? 0.22f : 0.10f);
            draw::disc(t, s.pos, r, withAlpha(c, 0.75f), withAlpha(c, 0.35f), {1.f, 1.f}, 12);
        } else {
            draw::disc(t, s.pos, cfg::summoner::shotRadius, withAlpha(c, 0.8f), withAlpha(c, 0.5f), {1.f, 1.f}, 8);
        }
    }

    for (const Ball& b : world.balls()) {
        const SummonerMods& m = b.mods.cls.summoner;
        const SummonerState& s = b.cls.summoner;
        const sf::Color c = summonColor(static_cast<int>(b.element));
        for (int i = 0; i < m.wardens; ++i) {
            const float orbit = s.wardenR > 0.f ? s.wardenR : cfg::summoner::wardenOrbit * world.arenaScale();
            const sf::Vector2f at = summonerWardenPos(world.core().pos, s.wardenAng, b.owner, i, m.wardens, orbit);
            const float a = s.wardenRest[i] > 0.f ? 0.25f : 0.6f;   // dim while it rests after a hit
            draw::disc(t, at, cfg::summoner::wardenRadius * 0.8f, withAlpha(c, a), withAlpha(c, a * 0.5f), {1.f, 1.f}, 14);
        }
        for (const SummonerState::Sword& sw2 : s.swords) {   // "Spectral sword": a pale blade, crossguard and hilt
            if (!sw2.out || m.swordEvery <= 0.f) continue;
            const sf::Vector2f f{std::cos(sw2.ang), std::sin(sw2.ang)}, n{-f.y, f.x};
            const sf::Vector2f q = sw2.pos;
            const float L = 18.f * world.arenaScale();
            const sf::Color blade = withAlpha(lerpColor(c, sf::Color::White, 0.7f), 0.9f);
            if (sw2.phase == 1) draw::line(t, sw2.from, q, 6.f, withAlpha(c, 0.25f));   // its streak
            draw::glow(t, q, L * 0.9f, c, 0.12f);
            sf::ConvexShape bl(3);
            bl.setPoint(0, q + f * L);
            bl.setPoint(1, q + n * (L * 0.12f));
            bl.setPoint(2, q - n * (L * 0.12f));
            bl.setFillColor(blade);
            t.draw(bl);
            draw::line(t, q - n * (L * 0.3f), q + n * (L * 0.3f), 2.5f, withAlpha(c, 0.95f));   // crossguard
            draw::line(t, q, q - f * (L * 0.35f), 3.f, withAlpha(lerpColor(c, sf::Color::Black, 0.4f), 0.95f));   // hilt
        }
    }
}

// ==================================================================== Slinger
// A chevron pointing where it flies: the ball you throw.
void markSlinger(sf::RenderTarget& t, sf::Vector2f p, float r, float heading, float a) {
    const sf::Vector2f f{std::cos(heading), std::sin(heading)};
    const sf::Vector2f s{-f.y, f.x};
    const sf::Vector2f tip = p + f * (r * 0.42f);
    const sf::Vector2f back = p - f * (r * 0.05f);
    draw::line(t, tip, back + s * (r * 0.36f), 2.f, white(0.75f * a));
    draw::line(t, tip, back - s * (r * 0.36f), 2.f, white(0.75f * a));
}
// "Afterburner": the fire left along a throw - small embers that shrink as
// they burn out.
void worldSlinger(sf::RenderTarget& t, const World& world) {
    const float k = world.arenaScale();
    for (const SlingerWorld::Flame& f : world.classWorld().slinger.flames) {
        const float life = clampf(f.life / f.maxLife, 0.f, 1.f);
        const float r = cfg::slinger::flameRadius * k * (0.45f + 0.55f * life);
        draw::disc(t, f.pos, r, withAlpha(theme::ember, 0.35f * life), withAlpha(theme::elemFire, 0.f), {1.f, 1.f}, 16);
        draw::disc(t, f.pos, r * 0.35f, withAlpha(sf::Color(255, 220, 160), 0.6f * life),
                   withAlpha(theme::ember, 0.2f * life), {1.f, 1.f}, 10);
    }
}

// ==================================================================== Alchemist
// A small triangle: the old sign for the elements.
void markAlchemist(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {
    draw::polygonOutline(t, p, r * 0.34f, 3, -kPi / 2.f, 1.5f, white(0.75f * a));
}

// ==================================================================== speed items
// Anchor / Beacon: a faint aura that fills in as the ball slows. Wake: a
// dotted trail. Leyline: small runes waiting for the next cast.
void worldStyle(sf::RenderTarget& t, const World& world) {
    const float k = world.arenaScale();
    for (const Ball& b : world.balls()) {
        const float still = b.cls.mage.still;
        if (still <= 0.05f) continue;
        const ClassMods& c = b.mods.cls;
        if (c.guardian.anchor > 0.f)
            draw::ring(t, b.pos, c.guardian.anchorRadius * k, 1.5f * k, withAlpha(theme::core, 0.25f * still));
        if (c.support.beacon > 0.f) {
            const float R = c.support.beaconRadius * k * (0.4f + 0.6f * still);
            draw::disc(t, b.pos, R, withAlpha(theme::puSurge, 0.07f * still), withAlpha(theme::puSurge, 0.f), {1.f, 1.f}, 32);
            draw::ring(t, b.pos, R, 1.5f * k, withAlpha(theme::puSurge, 0.3f * still));
        }
    }
    for (const SupportWorld::WakePoint& w : world.classWorld().support.wake) {
        const float a = clampf(w.life / cfg::style::wakeLife, 0.f, 1.f);
        draw::disc(t, w.pos, 3.f * k, withAlpha(theme::puSurge, 0.45f * a), withAlpha(theme::puSurge, 0.f), {1.f, 1.f}, 8);
    }
    for (const MageWorld::Rune& r : world.classWorld().mage.runes) {
        const float a = clampf(r.life / 1.f, 0.f, 1.f);
        draw::polygonOutline(t, r.pos, 7.f * k, 4, 0.f, 1.5f * k, withAlpha(theme::classMage, 0.7f * a));
    }
    for (const CommonWorld::Puddle& pd : world.classWorld().common.puddles) {   // "Seep"
        const float a = clampf(pd.life / std::min(0.5f, pd.maxLife), 0.f, 1.f);
        const sf::Color c = elementColor(static_cast<Element>(pd.elem));
        draw::disc(t, pd.pos, pd.r, withAlpha(c, 0.14f * a), withAlpha(c, 0.03f * a), {1.f, 1.f}, 28);
        draw::ring(t, pd.pos, pd.r, 1.2f * k, withAlpha(c, 0.3f * a));
    }
    const CommonWorld& cw = world.classWorld().common;
    for (const CommonWorld::Wire& w : cw.wires) {   // "Razor wire"
        const float a = clampf(w.life / 0.5f, 0.f, 1.f);
        const sf::Vector2f d = w.b - w.a;
        sf::RectangleShape r({length(d), 2.f * k});
        r.setOrigin(0.f, k);
        r.setPosition(w.a);
        r.setRotation(std::atan2(d.y, d.x) * 180.f / kPi);
        r.setFillColor(withAlpha(theme::classAssassin, 0.7f * a));
        t.draw(r);
    }
    {   // "Contract": a crosshair on the mark
        const float spin = classClock() * 1.5f;
        auto mark = [&](sf::Vector2f at, float r) {
            draw::ring(t, at, r + 7.f * k, 1.5f * k, withAlpha(theme::classAssassin, 0.75f));
            for (int i = 0; i < 4; ++i) {
                const float a = spin + kPi * 0.5f * static_cast<float>(i);
                const sf::Vector2f d{std::cos(a), std::sin(a)};
                draw::line(t, at + d * (r + 3.f * k), at + d * (r + 12.f * k), 1.5f * k, withAlpha(theme::classAssassin, 0.85f));
            }
        };
        if (cw.contractBoss) mark(world.boss().pos, world.boss().radius);
        else if (cw.contractId >= 0)
            for (const Enemy& e : world.enemies())
                if (e.id == cw.contractId && e.hp > 0.f) { mark(e.pos, e.radius); break; }
    }
    if (cw.wardOn) {   // "Ward ring": bright when it's up
        const Core& co = world.core();
        draw::ring(t, co.pos, co.radius + 10.f * k, 2.f * k,
                   withAlpha(theme::core, cw.wardCd <= 0.f ? 0.7f : 0.15f));
    }
    for (const CommonWorld::Golem& g : world.classWorld().common.golems) {   // "Golem": a stone block
        const float a = clampf(g.life / 0.4f, 0.f, 1.f);
        const float r = cfg::common::golemRadius * k * (1.f + 0.25f * g.flash);
        draw::disc(t, g.pos, r, withAlpha(lerpColor(theme::elemStone, theme::classSummoner, 0.35f), 0.9f * a),
                   withAlpha(theme::elemStone, 0.6f * a), {1.f, 1.f}, 6);
        draw::polygonOutline(t, g.pos, r, 6, 0.f, 1.6f * k, withAlpha(theme::classSummoner, 0.85f * a));
    }
    for (const CommonWorld::Glyph& g : world.classWorld().common.glyphs) {   // "Sigil"
        const float a = clampf(g.life / 0.6f, 0.f, 1.f);
        draw::polygonOutline(t, g.pos, g.r, 6, g.life * 0.8f, 1.6f * k, withAlpha(theme::classMage, 0.75f * a));
        draw::polygonOutline(t, g.pos, g.r * 0.5f, 3, -g.life * 1.2f, 1.2f * k, withAlpha(theme::classMage, 0.5f * a));
    }
}

void fillShape(sf::RenderTarget& t, std::initializer_list<sf::Vector2f> pts, sf::Color fill);   // (below)
void drawDigit(sf::RenderTarget& t, sf::Vector2f c, float h, int d, sf::Color col);

// ==================================================================== Abilities
// Each ability has its own small gesture, in the ball's colours, short and
// thin: a hook that bites and reels, lightning from the sky, a ghost left
// behind by a warp... The lasting ones (mines, vortex, beam...) move a little
// so they read as alive. The flourishes are CommonWorld::fx (sim-timed).
namespace {

float hash01(float x) {   // a cheap stable pseudo-random 0..1
    const float s = std::sin(x) * 43758.5453f;
    return s - std::floor(s);
}
sf::Vector2f lerpv(sf::Vector2f a, sf::Vector2f b, float u) { return a + (b - a) * u; }
float easeOut(float u) { return 1.f - (1.f - u) * (1.f - u); }

void drawFx(sf::RenderTarget& t, const CommonWorld::Fx& f, float k) {
    using F = CommonWorld::Fx;
    const float fl = clampf(f.life / std::max(0.01f, f.maxLife), 0.f, 1.f);   // 1 -> 0
    const float u = 1.f - fl;                                                 // 0 -> 1
    const sf::Color hot = lerpColor(f.color, sf::Color::White, 0.55f);
    switch (f.kind) {
        case F::Hook: {   // out fast, bite, then the chain shortens as the enemy is reeled in
            const float out = 0.22f;
            const sf::Vector2f head = u < out ? lerpv(f.a, f.b, u / out) : f.b;
            const float a = clampf(fl / 0.3f, 0.f, 1.f);
            const sf::Vector2f d = head - f.a;
            const float L = length(d);
            if (L < 1.f) break;
            const sf::Vector2f dir = d / L, side{-dir.y, dir.x};
            const float step = 7.f * k;
            for (float x = step * 0.5f; x < L - 4.f * k; x += step)   // chain links
                draw::disc(t, f.a + dir * x, 1.7f * k, withAlpha(hot, 0.85f * a), withAlpha(f.color, 0.5f * a), {1.f, 1.f}, 6);
            for (float sd : {-1.f, 1.f})   // the barb, pointing back down the chain
                draw::line(t, head, head - dir * (8.f * k) + side * (sd * 5.f * k), 2.f * k, withAlpha(sf::Color::White, 0.9f * a));
            if (u >= out) {   // the bite: a ring snapping shut on it
                const float b = clampf((u - out) / 0.2f, 0.f, 1.f);
                draw::ring(t, f.b, (22.f - 10.f * b) * k, 2.f * k, withAlpha(hot, 0.8f * (1.f - b)));
            }
            break;
        }
        case F::Smite: {   // a jagged bolt from the top, flickering, and a scorch where it lands
            const int flick = static_cast<int>((f.maxLife - f.life) / 0.06f);
            const float a = fl * (flick % 2 == 0 ? 1.f : 0.6f);
            const sf::Vector2f d = f.b - f.a;
            const sf::Vector2f side = normalized(sf::Vector2f{-d.y, d.x}, {1.f, 0.f});
            constexpr int kSeg = 8;
            sf::Vector2f prev = f.a;
            for (int i = 1; i <= kSeg; ++i) {
                const float v = static_cast<float>(i) / kSeg;
                const float j = i == kSeg ? 0.f : (hash01(f.b.x * 0.1f + static_cast<float>(i * 7 + flick * 13)) - 0.5f) * 26.f * k;
                const sf::Vector2f q = f.a + d * v + side * j;
                draw::line(t, prev, q, (7.f * fl + 1.f) * k, withAlpha(f.color, 0.3f * a));
                draw::line(t, prev, q, (2.4f * fl + 0.6f) * k, withAlpha(sf::Color::White, 0.95f * a));
                prev = q;
            }
            if (u < 0.2f) draw::glow(t, f.b, (f.r + 30.f * k), sf::Color::White, 0.7f * (1.f - u / 0.2f));
            draw::ring(t, f.b, f.r + (6.f + 20.f * easeOut(u)) * k, 2.f * k, withAlpha(hot, 0.7f * fl));
            for (int i = 0; i < 6; ++i) {   // sparks
                const float an = static_cast<float>(i) * 1.047f + hash01(f.b.y) * 6.f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                const float r0 = f.r + 4.f * k + 22.f * k * easeOut(u);
                draw::line(t, f.b + sd * r0, f.b + sd * (r0 + 7.f * k * fl), 1.5f * k, withAlpha(hot, 0.85f * fl));
            }
            break;
        }
        case F::Warp: {   // a ghost of the ball where it was, a dashed seam, space folding shut where it lands
            draw::disc(t, f.a, f.r * (1.f - 0.4f * u), withAlpha(f.color, 0.5f * fl), withAlpha(f.color, 0.15f * fl), {1.f, 1.f}, 20);
            draw::ring(t, f.a, f.r * (1.f + 1.2f * u), 1.5f * k, withAlpha(hot, 0.5f * fl));
            const sf::Vector2f d = f.b - f.a;
            const float L = length(d);
            if (L > 1.f) {
                const sf::Vector2f dir = d / L;
                const float dash = 10.f * k;
                for (float x = 0.f; x < L; x += dash * 2.f)
                    draw::line(t, f.a + dir * x, f.a + dir * std::min(L, x + dash), 1.5f * k, withAlpha(hot, 0.35f * fl));
            }
            const float c = clampf(u / 0.35f, 0.f, 1.f);
            if (c < 1.f) draw::ring(t, f.b, f.r * (4.f - 3.f * easeOut(c)), 2.f * k, withAlpha(hot, 0.8f * (1.f - c)));
            break;
        }
        case F::Swap: {   // two comets crossing on bent paths: the ball one way, the enemy the other
            const sf::Vector2f d = f.b - f.a;
            const sf::Vector2f side = normalized(sf::Vector2f{-d.y, d.x}, {1.f, 0.f}) * (length(d) * 0.35f);
            const float head = clampf(u / 0.6f, 0.f, 1.f);
            const float a = clampf(fl / 0.4f, 0.f, 1.f);
            auto comet = [&](sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f bend, sf::Color c) {
                const sf::Vector2f m = (p0 + p1) * 0.5f + bend;
                auto at = [&](float v) { return p0 * ((1.f - v) * (1.f - v)) + m * (2.f * v * (1.f - v)) + p1 * (v * v); };
                constexpr int kN = 10;
                const float tail = std::max(0.f, head - 0.45f);
                for (int i = 0; i < kN; ++i) {
                    const float v0 = lerpf(tail, head, static_cast<float>(i) / kN);
                    const float v1 = lerpf(tail, head, static_cast<float>(i + 1) / kN);
                    const float w = static_cast<float>(i + 1) / kN;
                    draw::line(t, at(v0), at(v1), (1.f + 3.f * w) * k, withAlpha(c, 0.75f * w * a));
                }
                draw::disc(t, at(head), 3.f * k, withAlpha(sf::Color::White, 0.9f * a), withAlpha(c, 0.6f * a), {1.f, 1.f}, 10);
            };
            comet(f.a, f.b, side, hot);
            comet(f.b, f.a, side, lerpColor(f.color2, sf::Color::White, 0.3f));
            break;
        }
        case F::Implode: {   // a ring that closes in, ticks pointing inward
            const float e = easeOut(u);
            const float r = f.r * (1.f - 0.85f * e);
            draw::ring(t, f.a, r, 2.f * k, withAlpha(hot, 0.75f * fl));
            for (int i = 0; i < 8; ++i) {
                const float an = static_cast<float>(i) * 0.785f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                draw::line(t, f.a + sd * (r + 10.f * k), f.a + sd * (r + 3.f * k), 1.5f * k, withAlpha(hot, 0.6f * fl));
            }
            break;
        }
        case F::Shards: {   // shrapnel streaking out
            for (int i = 0; i < 9; ++i) {
                const float an = static_cast<float>(i) * 0.698f + hash01(f.a.x + f.a.y) * 6.f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                const float r0 = f.r * (0.2f + 0.9f * easeOut(u)) * (0.8f + 0.4f * hash01(static_cast<float>(i) + f.a.x));
                draw::line(t, f.a + sd * r0, f.a + sd * (r0 + 8.f * k * fl + 2.f * k), 2.f * k, withAlpha(hot, 0.9f * fl));
            }
            break;
        }
        case F::Meteor: {   // its fiery tail still in the air for a blink; the crater smoulders after
            const sf::Vector2f dir = normalized(sf::Vector2f{-0.55f, 1.f}, {0.f, 1.f});   // falling down-left
            const float tl = clampf(1.f - u / 0.35f, 0.f, 1.f);
            if (tl > 0.f) {
                const float L = f.r * 3.5f * tl;
                constexpr int kN = 8;
                for (int i = 0; i < kN; ++i) {
                    const float v0 = static_cast<float>(i) / kN, v1 = static_cast<float>(i + 1) / kN;
                    const sf::Color c = lerpColor(lerpColor(f.color, theme::elemFire, 0.45f), sf::Color::White, v1 * v1);
                    draw::line(t, f.a - dir * (L * (1.f - v0)), f.a - dir * (L * (1.f - v1)), (2.f + 9.f * v1) * k * tl,
                               withAlpha(c, 0.8f * v1 * tl));
                }
                draw::glow(t, f.a, f.r * 0.45f, lerpColor(theme::elemFire, sf::Color::White, 0.4f), 0.5f * tl);
            }
            draw::disc(t, f.a, f.r * 0.5f, withAlpha(lerpColor(f.color, sf::Color::Black, 0.6f), 0.35f * fl),
                       withAlpha(f.color, 0.f), {1.f, 1.f}, 24);
            draw::ring(t, f.a, f.r * 0.55f, 2.f * k, withAlpha(lerpColor(f.color, theme::elemFire, 0.5f), 0.6f * fl));
            for (int i = 0; i < 6; ++i) {   // embers drifting up
                const float h = hash01(f.a.x * 0.37f + static_cast<float>(i) * 3.1f);
                const sf::Vector2f p = f.a + sf::Vector2f{(h - 0.5f) * f.r, (hash01(h * 91.f) - 0.5f) * f.r * 0.6f} -
                                       sf::Vector2f{0.f, 30.f * k * u};
                draw::disc(t, p, 1.8f * k, withAlpha(theme::elemFire, 0.8f * fl), withAlpha(theme::elemFire, 0.f), {1.f, 1.f}, 8);
            }
            break;
        }
        case F::Afterimage: {   // a dash: fading copies behind it and three speed lines
            const sf::Vector2f back = -f.b, side{-f.b.y, f.b.x};
            for (int i = 1; i <= 3; ++i) {
                const float w = 1.f - 0.25f * static_cast<float>(i);
                draw::disc(t, f.a + back * (f.r * 1.3f * static_cast<float>(i)), f.r * w, withAlpha(f.color, 0.3f * w * fl),
                           withAlpha(f.color, 0.05f * fl), {1.f, 1.f}, 18);
            }
            for (float sd : {-1.f, 0.f, 1.f}) {
                const sf::Vector2f o = f.a + side * (sd * f.r * 0.7f);
                draw::line(t, o + back * (f.r * 1.2f), o + back * (f.r * (2.5f + 2.5f * fl)), 1.5f * k, withAlpha(hot, 0.5f * fl));
            }
            break;
        }
        // ---- items: one shape each
        case F::Slash: {   // "Death sentence": one long clean cut through it, and a hairline echo
            const sf::Vector2f d{0.707f, -0.707f};
            const float L = f.r * (0.4f + 0.6f * easeOut(clampf(u / 0.3f, 0.f, 1.f)));
            draw::line(t, f.a - d * L, f.a + d * L, (3.f * fl + 0.5f) * k, withAlpha(sf::Color::White, 0.9f * fl));
            draw::line(t, f.a - d * L + sf::Vector2f{4.f, 4.f} * k, f.a + d * L + sf::Vector2f{4.f, 4.f} * k, 1.f * k,
                       withAlpha(f.color, 0.7f * fl));
            break;
        }
        case F::Clover: {   // "Lucky break": four leaves opening around it
            for (int i = 0; i < 4; ++i) {
                const float an = static_cast<float>(i) * 1.571f + 0.785f + u * 1.5f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                draw::disc(t, f.a + sd * (f.r * (0.4f + 0.5f * easeOut(u))), f.r * 0.22f, withAlpha(hot, 0.7f * fl),
                           withAlpha(f.color, 0.3f * fl), {1.f, 1.f}, 12);
            }
            break;
        }
        case F::Petals: {   // "Maelstrom": a ring of beads in its elements
            const float r = f.r * easeOut(u);
            for (int i = 0; i < 12; ++i) {
                const float an = static_cast<float>(i) * 0.5236f + u * 2.f;
                const sf::Color c = i % 2 == 0 ? f.color : f.color2;
                draw::disc(t, f.a + sf::Vector2f{std::cos(an), std::sin(an)} * r, 3.f * k, withAlpha(c, 0.85f * fl),
                           withAlpha(c, 0.2f * fl), {1.f, 1.f}, 8);
            }
            break;
        }
        case F::Coins: {   // slot-machine "JACKPOT": a fountain of coins that fall back
            for (int i = 0; i < 12; ++i) {
                const float an = -kPi * 0.5f + (hash01(static_cast<float>(i) * 1.7f) - 0.5f) * 2.2f;
                const float sp = f.r * (0.6f + 0.6f * hash01(static_cast<float>(i) * 3.3f));
                const sf::Vector2f p = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (sp * u) + sf::Vector2f{0.f, 160.f * k * u * u};
                draw::disc(t, p, 3.5f * k, withAlpha(lerpColor(theme::puGolden, sf::Color::White, 0.4f), fl),
                           withAlpha(theme::puGolden, fl), {std::fabs(std::cos(u * 12.f + static_cast<float>(i))) * 0.8f + 0.2f, 1.f}, 10);
            }
            break;
        }
        case F::Reels: {   // slot-machine pair: three little reels over the ball, the last one lagging
            for (int i = 0; i < 3; ++i) {
                const float pop = clampf((u - 0.12f * static_cast<float>(i)) / 0.2f, 0.f, 1.f);
                const sf::Vector2f p = f.a + sf::Vector2f{(static_cast<float>(i) - 1.f) * 17.f * k, -f.r - 16.f * k};
                draw::polygonOutline(t, p, 10.f * k * pop, 4, 0.785f, 2.f * k, withAlpha(hot, 0.9f * fl));
                draw::disc(t, p, 3.f * k * pop, withAlpha(f.color, 0.9f * fl), withAlpha(f.color, 0.5f * fl), {1.f, 1.f}, 6);
            }
            break;
        }
        case F::Boomerang: {   // "Homecoming": three arcs sweeping round as they widen
            for (int i = 0; i < 3; ++i) {
                const float s0 = u * 7.f + static_cast<float>(i) * 2.094f;
                draw::ring(t, f.a, f.r * easeOut(u), 2.5f * k, withAlpha(hot, 0.8f * fl), s0, s0 + 1.1f, 12);
            }
            break;
        }
        case F::Bastion: {   // "Last line": a hexagonal wall thrown out from the core
            const float e = easeOut(u);
            draw::polygonOutline(t, f.a, f.r * e, 6, 0.f, (6.f * fl + 1.f) * k, withAlpha(hot, 0.7f * fl));
            draw::polygonOutline(t, f.a, f.r * e * 0.92f, 6, 0.f, 1.5f * k, withAlpha(sf::Color::White, 0.5f * fl));
            break;
        }
        case F::Glyph: {   // "Sigil": two triangles turning against each other as they spread
            const float r = f.r * easeOut(u);
            draw::polygonOutline(t, f.a, r, 3, u * 3.f, 1.8f * k, withAlpha(hot, 0.8f * fl));
            draw::polygonOutline(t, f.a, r, 3, kPi - u * 3.f, 1.8f * k, withAlpha(f.color, 0.6f * fl));
            break;
        }
        case F::Cracks: {   // "Titan" quake: the ground splits out from the core
            const float e = easeOut(clampf(u / 0.35f, 0.f, 1.f));
            for (int i = 0; i < 10; ++i) {
                float an = static_cast<float>(i) * 0.628f + hash01(static_cast<float>(i) * 5.1f) * 0.4f;
                sf::Vector2f prev = f.a;
                for (int j = 1; j <= 5; ++j) {
                    an += (hash01(static_cast<float>(i * 9 + j)) - 0.5f) * 0.5f;
                    const sf::Vector2f q = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * e * static_cast<float>(j) / 5.f);
                    draw::line(t, prev, q, (2.5f - 0.35f * static_cast<float>(j)) * k, withAlpha(hot, 0.75f * fl));
                    prev = q;
                }
            }
            break;
        }
        case F::Dust: {   // "Heavy landing": a flat ring of dust and puffs thrown sideways
            const float e = easeOut(u);
            draw::ring(t, f.a, f.r * e, 2.f * k, withAlpha(hot, 0.6f * fl));
            for (float sd : {-1.f, 1.f})
                for (int i = 0; i < 3; ++i) {
                    const sf::Vector2f p = f.a + sf::Vector2f{sd * f.r * e * (0.5f + 0.25f * static_cast<float>(i)), -6.f * k * u * static_cast<float>(i)};
                    draw::disc(t, p, (5.f - static_cast<float>(i)) * k * (1.f + u), withAlpha(f.color, 0.35f * fl),
                               withAlpha(f.color, 0.f), {1.f, 1.f}, 10);
                }
            break;
        }
        case F::Clock: {   // "Time stop": a clock face, one hand sweeping round while it holds
            const float a = clampf(fl / 0.2f, 0.f, 1.f) * clampf(u / 0.1f, 0.f, 1.f);
            draw::ring(t, f.a, f.r, 2.f * k, withAlpha(hot, 0.7f * a));
            for (int i = 0; i < 12; ++i) {
                const float an = static_cast<float>(i) * 0.5236f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                draw::line(t, f.a + sd * (f.r * 0.82f), f.a + sd * f.r, (i % 3 == 0 ? 2.5f : 1.2f) * k, withAlpha(hot, 0.7f * a));
            }
            const float hand = -kPi * 0.5f + u * 2.f * kPi;
            draw::line(t, f.a, f.a + sf::Vector2f{std::cos(hand), std::sin(hand)} * (f.r * 0.75f), 2.f * k, withAlpha(sf::Color::White, 0.9f * a));
            break;
        }
        case F::Echo: {   // "Spell chorus": three rings, one after another, like a chord
            for (int i = 0; i < 3; ++i) {
                const float v = clampf((u - 0.15f * static_cast<float>(i)) / 0.55f, 0.f, 1.f);
                if (v <= 0.f || v >= 1.f) continue;
                draw::ring(t, f.a, f.r * (0.4f + 0.8f * v), 1.5f * k, withAlpha(hot, 0.7f * (1.f - v)));
            }
            break;
        }
        case F::Star: {   // "Grand reaction": an eight-point star in both elements, flung across the arena
            const float r = f.r * easeOut(u);
            for (int i = 0; i < 16; ++i) {
                const float an = static_cast<float>(i) * 0.3927f;
                const float len = i % 2 == 0 ? r : r * 0.45f;
                draw::line(t, f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (len * 0.6f),
                           f.a + sf::Vector2f{std::cos(an), std::sin(an)} * len, (3.f * fl + 1.f) * k,
                           withAlpha(i % 2 == 0 ? f.color : f.color2, 0.75f * fl));
            }
            if (u < 0.15f) draw::glow(t, f.a, 90.f * k, sf::Color::White, 0.6f * (1.f - u / 0.15f));
            break;
        }
        case F::Hexshield: {   // "Ward ring": a hex plate lights up on the side it blocked
            const float an = std::atan2(f.b.y - f.a.y, f.b.x - f.a.x);
            draw::polygonOutline(t, f.a, f.r * (1.f + 0.2f * u), 6, an, 3.f * k, withAlpha(hot, 0.8f * fl));
            const sf::Vector2f sd{std::cos(an), std::sin(an)};
            draw::disc(t, f.a + sd * f.r, 8.f * k, withAlpha(sf::Color::White, 0.8f * fl), withAlpha(f.color, 0.f), {1.f, 1.f}, 12);
            break;
        }
        case F::Link: {   // "Fission": the element leaps to a neighbour along a thin thread
            const sf::Vector2f head = lerpv(f.a, f.b, easeOut(clampf(u / 0.5f, 0.f, 1.f)));
            draw::line(t, f.a, head, 1.5f * k, withAlpha(hot, 0.7f * fl));
            draw::disc(t, head, 3.f * k, withAlpha(sf::Color::White, 0.9f * fl), withAlpha(f.color, 0.5f * fl), {1.f, 1.f}, 8);
            break;
        }
        case F::Sonic: {   // Mega Striker: a sonic boom - arcs fanning out ahead of the hit
            const float an = std::atan2(f.b.y, f.b.x);
            for (int i = 0; i < 3; ++i) {
                const float v = clampf(u * 1.3f - 0.15f * static_cast<float>(i), 0.f, 1.f);
                if (v <= 0.f) continue;
                draw::ring(t, f.a, f.r * v, (2.5f - 0.5f * static_cast<float>(i)) * k, withAlpha(hot, 0.8f * (1.f - v)), an - 0.8f, an + 0.8f, 14);
            }
            break;
        }
        case F::Rune: {   // Ancient Mage nova: a turning diamond, the Mage's own mark, flung wide
            const float r = f.r * easeOut(u);
            draw::polygonOutline(t, f.a, r, 4, u * 2.f, 2.f * k, withAlpha(hot, 0.8f * fl));
            draw::polygonOutline(t, f.a, r * 0.6f, 4, -u * 2.f, 1.2f * k, withAlpha(f.color, 0.6f * fl));
            break;
        }
        case F::Spokes: {   // "Lead storm" spinning up: barrels all round
            for (int i = 0; i < 8; ++i) {
                const float an = static_cast<float>(i) * 0.785f + u * 6.f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                const float r0 = f.r * (0.4f + 0.4f * u);
                draw::line(t, f.a + sd * r0, f.a + sd * (r0 + 8.f * k), 2.f * k, withAlpha(hot, 0.85f * fl));
            }
            break;
        }
        case F::Pellets: {   // "Frag rounds": a pinch of tiny pellets
            for (int i = 0; i < 5; ++i) {
                const float an = static_cast<float>(i) * 1.2566f + hash01(f.a.x * 0.3f) * 6.f;
                draw::disc(t, f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * easeOut(u)), 1.8f * k,
                           withAlpha(hot, 0.9f * fl), withAlpha(f.color, 0.4f * fl), {1.f, 1.f}, 6);
            }
            break;
        }
        case F::Smoke: {   // "Smoke bomb": slow soft puffs that swell and thin out
            for (int i = 0; i < 6; ++i) {
                const float an = static_cast<float>(i) * 1.047f + hash01(f.a.y * 0.2f) * 6.f;
                const sf::Vector2f p = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * 0.45f * easeOut(u));
                draw::disc(t, p, f.r * (0.3f + 0.25f * u), withAlpha(f.color, 0.22f * fl), withAlpha(f.color, 0.f), {1.f, 1.f}, 20);
            }
            break;
        }
        case F::Wisps: {   // "Necromancer" ghost bursting: wisps rising and swaying
            for (int i = 0; i < 7; ++i) {
                const float h = hash01(static_cast<float>(i) * 2.3f + f.a.x * 0.01f);
                const sf::Vector2f p = f.a + sf::Vector2f{(h - 0.5f) * f.r + std::sin(u * 9.f + static_cast<float>(i)) * 6.f * k,
                                                          -f.r * 0.8f * u * (0.6f + 0.4f * h)};
                draw::disc(t, p, 2.5f * k, withAlpha(hot, 0.8f * fl), withAlpha(f.color, 0.f), {1.f, 1.f}, 8);
            }
            draw::ring(t, f.a, f.r * 0.5f * easeOut(u), 1.f * k, withAlpha(f.color, 0.4f * fl));
            break;
        }
        case F::Pillar: {   // "Martyr": a column of light where the summon fell
            const float w = f.r * 0.35f * (1.f - u * 0.6f);
            draw::line(t, f.a, f.a - sf::Vector2f{0.f, f.r * 2.f}, w, withAlpha(f.color, 0.25f * fl));
            draw::line(t, f.a, f.a - sf::Vector2f{0.f, f.r * 2.f}, w * 0.3f, withAlpha(sf::Color::White, 0.6f * fl));
            draw::ring(t, f.a, f.r * easeOut(u), 1.5f * k, withAlpha(hot, 0.6f * fl));
            break;
        }
        case F::Blast: {   // "Bomber": a white-hot fireball that collapses into a ring of flame
            draw::disc(t, f.a, f.r * 0.6f * (1.f - u), withAlpha(sf::Color::White, 0.8f * fl), withAlpha(f.color, 0.5f * fl), {1.f, 1.f}, 20);
            draw::ring(t, f.a, f.r * easeOut(u), (5.f * fl + 1.f) * k, withAlpha(f.color, 0.7f * fl));
            break;
        }
        case F::Fuse: {   // a Jester bomb: crackling sparks, a pop
            for (int i = 0; i < 12; ++i) {
                const float an = hash01(static_cast<float>(i) * 4.1f + f.a.x) * 6.283f;
                const float r0 = f.r * easeOut(u) * (0.5f + 0.5f * hash01(static_cast<float>(i)));
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                draw::line(t, f.a + sd * r0, f.a + sd * (r0 + 4.f * k), 1.5f * k, withAlpha(i % 3 == 0 ? sf::Color::White : f.color, 0.9f * fl));
            }
            break;
        }
        case F::Confetti: {   // Jester "JACKPOT": confetti in every colour, fluttering down
            static const sf::Color kC[4] = {theme::classJester, theme::puGolden, theme::elemWater, theme::elemFire};
            for (int i = 0; i < 16; ++i) {
                const float an = hash01(static_cast<float>(i) * 7.7f) * 6.283f;
                const float sp = f.r * (0.4f + 0.6f * hash01(static_cast<float>(i) * 1.9f));
                const sf::Vector2f p = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (sp * easeOut(u)) + sf::Vector2f{0.f, 50.f * k * u};
                const float spin = u * 10.f + static_cast<float>(i);
                const sf::Vector2f d{std::cos(spin) * 3.f * k, std::sin(spin) * 3.f * k};
                draw::line(t, p - d, p + d, 2.f * k, withAlpha(kC[i % 4], 0.9f * fl));
            }
            break;
        }
        case F::Cards: {   // "Sleight": three cards fanning out over the mark
            for (int i = 0; i < 3; ++i) {
                const float rot = (static_cast<float>(i) - 1.f) * 0.5f * easeOut(u);
                const sf::Vector2f p = f.a + sf::Vector2f{std::sin(rot), -std::cos(rot)} * (f.r * 0.6f);
                draw::polygonOutline(t, p, 7.f * k, 4, 0.785f + rot, 1.5f * k, withAlpha(i == 1 ? sf::Color::White : hot, 0.85f * fl));
            }
            break;
        }
        case F::Arrow: {   // "Ambush": chevrons driving through the target
            const sf::Vector2f d = f.b, side{-f.b.y, f.b.x};
            for (int i = 0; i < 3; ++i) {
                const sf::Vector2f c = f.a + d * (f.r * (u * 0.8f + 0.25f * static_cast<float>(i) - 0.4f));
                const float a = fl * (1.f - 0.25f * static_cast<float>(i));
                for (float sd : {-1.f, 1.f})
                    draw::line(t, c, c - d * (7.f * k) + side * (sd * 6.f * k), 2.f * k, withAlpha(hot, 0.85f * a));
            }
            break;
        }
        case F::Rise: {   // "Golem" called: a square stone plate heaves up, chips flying
            const float e = easeOut(u);
            draw::polygonOutline(t, f.a, f.r * (0.4f + 0.6f * e), 4, 0.785f, 2.f * k, withAlpha(hot, 0.75f * fl));
            for (int i = 0; i < 4; ++i) {
                const float an = static_cast<float>(i) * 1.571f + 0.785f;
                const sf::Vector2f p = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * e) + sf::Vector2f{0.f, 40.f * k * u * u - 20.f * k * u};
                draw::polygon(t, p, 2.5f * k, 4, u * 5.f, withAlpha(theme::elemStone, fl), withAlpha(theme::elemStone, fl));
            }
            break;
        }
        case F::Slam: {   // a Golem slam: a square shock rolling out
            draw::polygonOutline(t, f.a, f.r * easeOut(u), 4, 0.f, (4.f * fl + 1.f) * k, withAlpha(hot, 0.7f * fl));
            break;
        }
        case F::Spiral: {   // a great spirit arriving: motes unwinding outward
            for (int i = 0; i < 10; ++i) {
                const float v = static_cast<float>(i) / 10.f;
                const float an = v * 9.f + u * 4.f;
                draw::disc(t, f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * v * easeOut(u) * 1.4f), 3.f * k,
                           withAlpha(hot, fl * (1.f - v * 0.4f)), withAlpha(f.color, 0.f), {1.f, 1.f}, 6);
            }
            break;
        }
        case F::Collapse: {   // a black hole closing: a dark core folding in, then a thin shell thrown off
            draw::disc(t, f.a, f.r * 0.35f * (1.f - u), withAlpha(theme::bgDeep, 0.85f * fl), withAlpha(f.color, 0.3f * fl), {1.f, 1.f}, 20);
            if (u > 0.3f) draw::ring(t, f.a, f.r * easeOut((u - 0.3f) / 0.7f), 1.5f * k, withAlpha(hot, 0.7f * fl));
            break;
        }
        case F::Sight: {   // "Seeker": a dotted sight line to the next one, snapping on
            const sf::Vector2f d = f.b - f.a;
            const float L = length(d);
            if (L < 1.f) break;
            const sf::Vector2f dir = d / L;
            for (float x = f.r + 4.f * k; x < L; x += 9.f * k)
                draw::disc(t, f.a + dir * x, 1.4f * k, withAlpha(hot, 0.7f * fl), withAlpha(hot, 0.2f * fl), {1.f, 1.f}, 6);
            draw::ring(t, f.b, (14.f - 6.f * easeOut(u)) * k, 1.5f * k, withAlpha(hot, 0.8f * fl));
            break;
        }
        case F::Ping: {   // "Ping-pong": a paddle tap on the wall and a short arrow toward its next mark
            const sf::Vector2f d = f.b, side{-f.b.y, f.b.x};
            const sf::Vector2f c = f.a - d * f.r;
            draw::line(t, c - side * (f.r * 1.2f), c + side * (f.r * 1.2f), (3.f * fl + 1.f) * k, withAlpha(hot, 0.85f * fl));
            const sf::Vector2f tip = f.a + d * (f.r * (1.5f + 2.f * easeOut(u)));
            for (float sd : {-1.f, 1.f})
                draw::line(t, tip, tip - d * (6.f * k) + side * (sd * 4.f * k), 1.5f * k, withAlpha(hot, 0.8f * fl));
            break;
        }
        case F::Beckon: {   // "Beckon": four marks snap onto the spot you clicked; a faint dot fades where it was
            const float e = easeOut(clampf(u / 0.5f, 0.f, 1.f));
            for (int i = 0; i < 4; ++i) {
                const float an = static_cast<float>(i) * 1.571f + 0.785f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                const float r = f.r + (24.f - 18.f * e) * k;
                draw::line(t, f.a + sd * r, f.a + sd * (r + 6.f * k), 2.f * k, withAlpha(hot, 0.9f * fl));
            }
            draw::disc(t, f.b, f.r * 0.5f * fl, withAlpha(f.color, 0.5f * fl), withAlpha(f.color, 0.f), {1.f, 1.f}, 12);
            break;
        }
        case F::Mana: {   // "Mana spring": blue motes drawn into the ball
            for (int i = 0; i < 6; ++i) {
                const float an = static_cast<float>(i) * 1.047f + u * 2.f;
                const float r = f.r * (1.f - easeOut(u));
                draw::disc(t, f.a + sf::Vector2f{std::cos(an), std::sin(an)} * r, 2.2f * k, withAlpha(hot, 0.9f * fl),
                           withAlpha(f.color, 0.2f * fl), {1.f, 1.f}, 8);
            }
            break;
        }
        case F::Leap: {   // "Conflux": the reaction arcs over to the next enemy
            const sf::Vector2f d = f.b - f.a;
            const sf::Vector2f up = normalized(sf::Vector2f{-d.y, d.x}, {0.f, -1.f}) * (length(d) * 0.4f);
            const float head = easeOut(clampf(u / 0.6f, 0.f, 1.f));
            auto at = [&](float v) { const sf::Vector2f m = (f.a + f.b) * 0.5f + up;
                                     return f.a * ((1.f - v) * (1.f - v)) + m * (2.f * v * (1.f - v)) + f.b * (v * v); };
            for (int i = 0; i < 12; ++i) {
                const float v0 = head * static_cast<float>(i) / 12.f, v1 = head * static_cast<float>(i + 1) / 12.f;
                draw::line(t, at(v0), at(v1), 2.f * k, withAlpha(i % 2 ? f.color : f.color2, 0.8f * fl));
            }
            break;
        }
        case F::Saturate: {   // "Saturate": both elements soak in - two rings, one in each colour, sinking inward
            draw::ring(t, f.a, f.r * (1.f - 0.4f * u), 2.f * k, withAlpha(f.color, 0.7f * fl));
            draw::ring(t, f.a, f.r * (0.8f - 0.4f * u), 2.f * k, withAlpha(f.color2, 0.7f * fl));
            break;
        }
        case F::Wild: {   // "Wild magic": sparks of every colour spinning off
            static const sf::Color kC[5] = {theme::elemFire, theme::elemWater, theme::elemPoison, theme::classMage, theme::puGolden};
            for (int i = 0; i < 10; ++i) {
                const float an = static_cast<float>(i) * 0.628f + u * 6.f;
                const sf::Vector2f p = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * easeOut(u));
                draw::disc(t, p, 2.f * k, withAlpha(kC[i % 5], 0.9f * fl), withAlpha(kC[i % 5], 0.f), {1.f, 1.f}, 6);
            }
            break;
        }
        case F::Chrono: {   // "Chronomancy": a slow, wide, dashed ripple
            const float r = f.r * easeOut(u);
            for (int i = 0; i < 24; ++i) {
                const float a0 = static_cast<float>(i) * 0.2618f;
                draw::ring(t, f.a, r, 1.5f * k, withAlpha(hot, 0.4f * fl), a0, a0 + 0.13f, 4);
            }
            break;
        }
        case F::Scythe: {   // "Reaper": a crescent swept around the ball
            const float an = std::atan2(f.b.y, f.b.x);
            const float sweep = -1.5f + 3.f * easeOut(u);
            draw::ring(t, f.a, f.r, (3.f * fl + 1.f) * k, withAlpha(hot, 0.85f * fl), an + sweep - 1.2f, an + sweep, 14);
            break;
        }
        case F::Streak: {   // "Lightspeed": a white-hot straight streak to each chained enemy
            draw::line(t, f.a, f.b, (4.f * fl + 1.f) * k, withAlpha(f.color, 0.35f * fl));
            draw::line(t, f.a, f.b, 1.2f * k, withAlpha(sf::Color::White, 0.95f * fl));
            break;
        }
        case F::Breath:     // a baby dragon's breath and...
        case F::Swath: {    // ...the elder's: a roaring flame - billows swelling down the line, white-hot at the throat
            const bool big = f.kind == F::Swath;
            const sf::Vector2f dir = big ? normalized(f.b - f.a, {1.f, 0.f}) : f.b;
            const float L = big ? length(f.b - f.a) : f.r;
            const float spread = big ? f.r : std::tan(cfg::common::babyCone) * L;   // half-width at the far end
            const float reach = L * easeOut(clampf(u / 0.25f, 0.f, 1.f));
            const sf::Vector2f side{-dir.y, dir.x};
            const int n = big ? 26 : 14;
            const int fr = static_cast<int>(u * 30.f);
            for (int i = 0; i < n; ++i) {
                const float v = (static_cast<float>(i) + hash01(static_cast<float>(i * 5 + fr))) / static_cast<float>(n);
                if (v * L > reach) continue;
                const float w = (big ? 0.35f + 0.65f * v : 0.15f + 0.85f * v) * spread;
                const sf::Vector2f q = f.a + dir * (L * v) + side * ((hash01(static_cast<float>(i * 13 + fr * 3)) - 0.5f) * w);
                const float pr = (big ? 10.f + 26.f * v : 3.f + 9.f * v) * k * (0.7f + 0.6f * fl);
                const sf::Color core = v < 0.25f ? sf::Color(255, 245, 220) : v < 0.5f ? sf::Color(255, 200, 90) : f.color;
                draw::disc(t, q, pr, withAlpha(core, 0.7f * fl), withAlpha(lerpColor(f.color, sf::Color::Black, 0.4f), 0.f), {1.f, 1.f}, 12);
            }
            break;
        }
        case F::Drop: {   // "Primal" rain: a drop lands - a little ripple
            draw::ring(t, f.a, f.r * 0.3f + 10.f * k * easeOut(u), 1.2f * k, withAlpha(hot, 0.8f * fl), 0.f, 6.283f, 12);
            draw::line(t, f.a - sf::Vector2f{-4.f, 14.f} * k * fl, f.a, 1.2f * k, withAlpha(hot, 0.7f * fl));
            break;
        }
        case F::Boulder: break;   // (the boulders draw from their own list)
        case F::Ion: {   // "Ion cannon": a straight crackling bolt from one enemy to the next, a spark on each
            const sf::Vector2f d = f.b - f.a;
            const sf::Vector2f side = normalized(sf::Vector2f{-d.y, d.x}, {0.f, 1.f});
            const sf::Vector2f mid = (f.a + f.b) * 0.5f + side * ((hash01(f.a.x + u * 20.f) - 0.5f) * 10.f * k);
            draw::line(t, f.a, mid, (5.f * fl + 1.f) * k, withAlpha(sf::Color(120, 220, 255), 0.3f * fl));
            draw::line(t, mid, f.b, (5.f * fl + 1.f) * k, withAlpha(sf::Color(120, 220, 255), 0.3f * fl));
            draw::line(t, f.a, mid, 1.5f * k, withAlpha(sf::Color::White, 0.95f * fl));
            draw::line(t, mid, f.b, 1.5f * k, withAlpha(sf::Color::White, 0.95f * fl));
            draw::glow(t, f.b, 9.f * k, sf::Color(120, 220, 255), 0.6f * fl);
            break;
        }
        case F::Card: {   // the Jester draws a card: it flips up over the ball, suit and rank, then fades
            const int suit = static_cast<int>(f.b.x), rank = static_cast<int>(f.b.y);
            const float flip = std::fabs(std::cos(clampf(u / 0.25f, 0.f, 1.f) * kPi * 0.5f + kPi * 0.5f));   // turns face-up
            const float a = clampf(fl / 0.3f, 0.f, 1.f);
            const float W = 22.f * k * std::max(0.08f, flip), H = 30.f * k;
            const sf::Vector2f c = f.a - sf::Vector2f{0.f, 10.f * k * easeOut(clampf(u / 0.3f, 0.f, 1.f))};
            draw::box(t, {c.x - W * 0.5f, c.y - H * 0.5f, W, H}, 2.5f * k, withAlpha(sf::Color(250, 248, 240), a),
                      withAlpha(sf::Color(230, 226, 214), a), withAlpha(theme::classJester, a), 1.f);
            if (flip < 0.5f) break;
            const sf::Color ink = suit < 2 ? withAlpha(sf::Color(214, 40, 52), a) : withAlpha(sf::Color(30, 30, 36), a);
            const sf::Vector2f sp = c + sf::Vector2f{0.f, 4.f * k};
            const float r = 5.5f * k;
            switch (suit) {
                case 0:   // a heart: two lobes and a point
                    draw::disc(t, sp + sf::Vector2f{-r * 0.5f, -r * 0.3f}, r * 0.6f, ink, ink, {1.f, 1.f}, 10);
                    draw::disc(t, sp + sf::Vector2f{r * 0.5f, -r * 0.3f}, r * 0.6f, ink, ink, {1.f, 1.f}, 10);
                    fillShape(t, {sp + sf::Vector2f{-r * 1.05f, -r * 0.1f}, sp + sf::Vector2f{r * 1.05f, -r * 0.1f}, sp + sf::Vector2f{0.f, r * 1.1f}}, ink);
                    break;
                case 1: draw::polygon(t, sp, r, 4, 0.f, ink, ink); break;   // a diamond
                case 2:   // a spade: an upturned heart and a stem
                    draw::disc(t, sp + sf::Vector2f{-r * 0.5f, r * 0.3f}, r * 0.6f, ink, ink, {1.f, 1.f}, 10);
                    draw::disc(t, sp + sf::Vector2f{r * 0.5f, r * 0.3f}, r * 0.6f, ink, ink, {1.f, 1.f}, 10);
                    fillShape(t, {sp + sf::Vector2f{-r * 1.05f, r * 0.1f}, sp + sf::Vector2f{r * 1.05f, r * 0.1f}, sp + sf::Vector2f{0.f, -r * 1.1f}}, ink);
                    draw::line(t, sp + sf::Vector2f{0.f, r * 0.3f}, sp + sf::Vector2f{0.f, r * 1.2f}, r * 0.3f, ink);
                    break;
                default:   // a club: three leaves and a stem
                    for (const sf::Vector2f o : {sf::Vector2f{0.f, -0.55f}, sf::Vector2f{-0.55f, 0.15f}, sf::Vector2f{0.55f, 0.15f}})
                        draw::disc(t, sp + o * r, r * 0.5f, ink, ink, {1.f, 1.f}, 10);
                    draw::line(t, sp, sp + sf::Vector2f{0.f, r * 1.2f}, r * 0.3f, ink);
                    break;
            }
            const sf::Vector2f rc = c + sf::Vector2f{-W * 0.22f, -H * 0.3f};   // the rank, top left
            const float h = 8.f * k, th = std::max(1.f, 1.4f * k);
            switch (rank) {
                case 1:   // A
                    draw::line(t, rc + sf::Vector2f{-h * 0.3f, h * 0.5f}, rc + sf::Vector2f{0.f, -h * 0.5f}, th, ink);
                    draw::line(t, rc + sf::Vector2f{h * 0.3f, h * 0.5f}, rc + sf::Vector2f{0.f, -h * 0.5f}, th, ink);
                    draw::line(t, rc + sf::Vector2f{-h * 0.15f, h * 0.1f}, rc + sf::Vector2f{h * 0.15f, h * 0.1f}, th, ink);
                    break;
                case 11:   // J
                    draw::line(t, rc + sf::Vector2f{h * 0.2f, -h * 0.5f}, rc + sf::Vector2f{h * 0.2f, h * 0.4f}, th, ink);
                    draw::line(t, rc + sf::Vector2f{h * 0.2f, h * 0.4f}, rc + sf::Vector2f{-h * 0.25f, h * 0.3f}, th, ink);
                    break;
                case 12:   // Q
                    draw::ring(t, rc, h * 0.38f, th, ink, 0.f, 6.283f, 12);
                    draw::line(t, rc + sf::Vector2f{h * 0.1f, h * 0.15f}, rc + sf::Vector2f{h * 0.4f, h * 0.5f}, th, ink);
                    break;
                case 13:   // K
                    draw::line(t, rc + sf::Vector2f{-h * 0.25f, -h * 0.5f}, rc + sf::Vector2f{-h * 0.25f, h * 0.5f}, th, ink);
                    draw::line(t, rc + sf::Vector2f{-h * 0.2f, 0.f}, rc + sf::Vector2f{h * 0.3f, -h * 0.5f}, th, ink);
                    draw::line(t, rc + sf::Vector2f{-h * 0.2f, 0.f}, rc + sf::Vector2f{h * 0.3f, h * 0.5f}, th, ink);
                    break;
                default:
                    if (rank == 10) {
                        drawDigit(t, rc - sf::Vector2f{h * 0.3f, 0.f}, h, 1, ink);
                        drawDigit(t, rc + sf::Vector2f{h * 0.35f, 0.f}, h, 0, ink);
                    } else {
                        drawDigit(t, rc, h, rank, ink);
                    }
                    break;
            }
            break;
        }
        case F::Frost: {   // "Primal" ice: a frost cloud - pale, still, crystals glinting
            const float a = clampf(fl / 0.3f, 0.f, 1.f) * clampf(u / 0.1f, 0.f, 1.f);
            draw::disc(t, f.a, f.r, withAlpha(sf::Color(220, 240, 255), 0.18f * a), withAlpha(f.color, 0.04f * a), {1.f, 1.f}, 32);
            for (int i = 0; i < 9; ++i) {
                const float an = hash01(static_cast<float>(i) * 2.7f + f.a.x) * 6.283f;
                const sf::Vector2f c = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * 0.8f * hash01(static_cast<float>(i) * 5.1f));
                const float h = 4.f * k * (0.6f + 0.4f * std::sin(u * 12.f + static_cast<float>(i)));
                for (int j = 0; j < 3; ++j) {
                    const float b = static_cast<float>(j) * 1.047f;
                    draw::line(t, c - sf::Vector2f{std::cos(b), std::sin(b)} * h, c + sf::Vector2f{std::cos(b), std::sin(b)} * h, 1.f * k,
                               withAlpha(sf::Color::White, 0.7f * a));
                }
            }
            break;
        }
        case F::Rubble: {   // "Primal" stone: chunks jolted up around each enemy
            for (int i = 0; i < 4; ++i) {
                const float an = static_cast<float>(i) * 1.571f + hash01(f.a.x) * 6.f;
                const sf::Vector2f q = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r + 4.f * k) - sf::Vector2f{0.f, 14.f * k * std::sin(u * kPi)};
                draw::polygon(t, q, 2.6f * k, 4, u * 6.f + an, withAlpha(f.color, fl), withAlpha(f.color, fl));
            }
            break;
        }
        case F::Splash: {   // "Primal" poison: the bottle bursts - droplets and a puddle
            draw::disc(t, f.a, f.r * (0.4f + 0.5f * easeOut(u)), withAlpha(f.color, 0.25f * fl), withAlpha(f.color, 0.05f * fl), {1.f, 0.7f}, 24);
            for (int i = 0; i < 8; ++i) {
                const float an = static_cast<float>(i) * 0.785f + hash01(f.a.y) * 3.f;
                const sf::Vector2f q = f.a + sf::Vector2f{std::cos(an), std::sin(an) * 0.7f} * (f.r * easeOut(u)) - sf::Vector2f{0.f, 20.f * k * std::sin(u * kPi)};
                draw::disc(t, q, 2.2f * k, withAlpha(hot, 0.9f * fl), withAlpha(f.color, 0.4f * fl), {1.f, 1.f}, 6);
            }
            break;
        }
        case F::Storm: {   // "Primal" electric: a little thundercloud, a short crooked bolt
            const sf::Vector2f top = f.a - sf::Vector2f{0.f, 70.f * k};
            for (int i = -1; i <= 1; ++i)
                draw::disc(t, top + sf::Vector2f{static_cast<float>(i) * 10.f * k, (i == 0 ? -4.f : 0.f) * k}, 10.f * k,
                           withAlpha(sf::Color(70, 74, 92), 0.8f * fl), withAlpha(sf::Color(40, 42, 54), 0.5f * fl), {1.f, 1.f}, 12);
            if (u < 0.5f) {
                sf::Vector2f prev = top;
                for (int j = 1; j <= 4; ++j) {
                    const sf::Vector2f q = lerpv(top, f.a, static_cast<float>(j) / 4.f) +
                                           sf::Vector2f{(hash01(static_cast<float>(j) + f.a.x) - 0.5f) * 14.f * k * (j < 4 ? 1.f : 0.f), 0.f};
                    draw::line(t, prev, q, 2.f * k, withAlpha(sf::Color::White, 0.95f));
                    draw::line(t, prev, q, 5.f * k, withAlpha(f.color, 0.3f));
                    prev = q;
                }
            }
            break;
        }
        case F::Shock: {   // Empower landing: a sharp ring and a star of spikes
            const float e = easeOut(u);
            draw::ring(t, f.a, f.r * e, (3.f * fl + 1.f) * k, withAlpha(sf::Color::White, 0.8f * fl));
            for (int i = 0; i < 8; ++i) {
                const float an = static_cast<float>(i) * 0.785f + 0.39f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                draw::line(t, f.a + sd * (f.r * 0.5f * e), f.a + sd * (f.r * e), 2.f * k, withAlpha(hot, 0.85f * fl));
            }
            break;
        }
    }
}

}  // namespace

void worldAbilities(sf::RenderTarget& t, const World& world) {
    namespace A = cfg::ability;
    const CommonWorld& cw = world.classWorld().common;
    const float k = world.arenaScale();
    const float clk = classClock();
    for (const CommonWorld::Mine& m : cw.mines) {   // "Mines": dropped with a bounce, a closing ring while arming, then a blink
        const sf::Color c = m.elem != 0 ? elementColor(static_cast<Element>(m.elem)) : theme::accent;
        const float a = clampf(m.life / 0.5f, 0.f, 1.f);
        const float age = A::mineLife - m.life;
        const float drop = age < 0.2f ? 1.f + 0.6f * std::sin(age / 0.2f * kPi) : 1.f;
        if (m.arm > 0.f) {
            draw::disc(t, m.pos, 3.f * k * drop, withAlpha(c, 0.5f * a), withAlpha(c, 0.2f * a), {1.f, 1.f}, 10);
            draw::ring(t, m.pos, (7.f + 12.f * clampf(m.arm / A::mineArm, 0.f, 1.f)) * k, 1.f * k, withAlpha(c, 0.35f * a));
        } else {
            const float blink = 0.5f + 0.5f * std::sin(clk * 9.f + m.pos.x * 0.05f);
            draw::disc(t, m.pos, 3.5f * k, withAlpha(lerpColor(c, sf::Color::White, 0.5f * blink), 0.9f * a),
                       withAlpha(c, 0.4f * a), {1.f, 1.f}, 10);
            draw::ring(t, m.pos, 7.f * k, 1.f * k, withAlpha(c, (0.25f + 0.35f * blink) * a));
            draw::glow(t, m.pos, 9.f * k, c, 0.25f * blink * a);
        }
    }
    for (const CommonWorld::Vortex& v : cw.vortices) {   // "Vortex": arcs turning in, motes spiralling down, a dark eye
        const float a = clampf(v.life / 0.4f, 0.f, 1.f) * clampf((v.maxLife - v.life) / 0.2f, 0.f, 1.f);
        const float open = easeOut(clampf((v.maxLife - v.life) / 0.3f, 0.f, 1.f));
        const sf::Color c = v.elem != 0 ? elementColor(static_cast<Element>(v.elem)) : sf::Color::White;
        const float spin = clk * 5.f;
        const float R = v.r * open;
        draw::disc(t, v.pos, R * 0.22f, withAlpha(theme::bgDeep, 0.6f * a), withAlpha(c, 0.1f * a), {1.f, 1.f}, 20);
        for (int i = 0; i < 3; ++i) {
            const float r = R * (0.35f + 0.3f * static_cast<float>(i));
            const float s0 = spin * (1.f + 0.4f * static_cast<float>(i)) + static_cast<float>(i);
            draw::ring(t, v.pos, r, 1.5f * k, withAlpha(c, 0.4f * a), s0, s0 + 2.2f, 20);
        }
        for (int i = 0; i < 10; ++i) {
            const float ph = std::fmod(clk * 0.7f + static_cast<float>(i) * 0.1f, 1.f);
            const float an = static_cast<float>(i) * 2.4f + ph * 5.f + spin * 0.3f;
            draw::disc(t, v.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (R * (1.f - ph)), 1.8f * k,
                       withAlpha(c, 0.7f * a * ph), withAlpha(c, 0.f), {1.f, 1.f}, 6);
        }
    }
    for (const CommonWorld::Cloud& cl : cw.clouds) {   // "Cloud": a few soft puffs drifting, a pulse every time it bites
        const float a = clampf(cl.life / 0.5f, 0.f, 1.f) * clampf((cl.maxLife - cl.life) / 0.3f, 0.f, 1.f);
        const sf::Color c = cl.elem != 0 ? elementColor(static_cast<Element>(cl.elem)) : sf::Color::White;
        draw::disc(t, cl.pos, cl.r, withAlpha(c, 0.1f * a), withAlpha(c, 0.02f * a), {1.f, 1.f}, 32);
        for (int i = 0; i < 4; ++i) {
            const float an = clk * 0.4f + static_cast<float>(i) * 1.571f + cl.pos.x * 0.01f;
            const sf::Vector2f o{std::cos(an), std::sin(an * 1.3f)};
            draw::disc(t, cl.pos + o * (cl.r * 0.4f), cl.r * 0.45f, withAlpha(c, 0.09f * a), withAlpha(c, 0.f), {1.f, 1.f}, 24);
        }
        const float pulse = clampf((cl.tickT - (A::cloudTick - 0.3f)) / 0.3f, 0.f, 1.f);   // 1 right after a bite
        if (pulse > 0.f) draw::ring(t, cl.pos, cl.r * (1.f - 0.5f * pulse), 1.5f * k, withAlpha(c, 0.4f * pulse * a));
    }
    for (const CommonWorld::Portal& q : cw.portals) {   // "Portal": they open wide, two rings turning against each other
        const float a = clampf(q.life / 0.4f, 0.f, 1.f) * clampf((q.maxLife - q.life) / 0.25f, 0.f, 1.f);
        const float open = std::min(easeOut(clampf((q.maxLife - q.life) / 0.3f, 0.f, 1.f)), clampf(q.life / 0.3f, 0.f, 1.f));
        const float R = cfg::ability::portalRadius * k * open;
        const float spin = clk * 3.f;
        for (sf::Vector2f at : {q.a, q.b}) {
            draw::disc(t, at, R, withAlpha(theme::bgDeep, 0.5f * a), withAlpha(theme::classMage, 0.18f * a), {1.f, 1.f}, 24);
            draw::ring(t, at, R, 2.f * k, withAlpha(theme::classMage, 0.75f * a), spin, spin + 4.5f, 24);
            draw::ring(t, at, R * 0.6f, 1.5f * k, withAlpha(lerpColor(theme::classMage, sf::Color::White, 0.4f), 0.5f * a),
                       -spin * 1.6f, -spin * 1.6f + 3.f, 18);
        }
    }
    const sf::Vector2f core = world.core().pos;
    for (const Ball& b : world.balls()) {
        const CommonState& s = b.cls.common;
        const sf::Color hot = lerpColor(b.color, sf::Color::White, 0.5f);
        if (s.beamOn) {   // "Beam": a humming beam with a white core, sparks where it burns
            const float hum = 1.f + 0.35f * std::sin(clk * 40.f);
            draw::line(t, b.pos, s.beamTo, 8.f * k * hum, withAlpha(b.color, 0.2f));
            draw::line(t, b.pos, s.beamTo, 3.f * k * hum, withAlpha(hot, 0.8f));
            draw::line(t, b.pos, s.beamTo, 1.2f * k, white(0.95f));
            draw::glow(t, s.beamTo, 14.f * k * hum, hot, 0.5f);
            const int fr = static_cast<int>(clk * 20.f);
            for (int i = 0; i < 3; ++i) {
                const float an = hash01(static_cast<float>(fr * 3 + i)) * 6.283f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                draw::line(t, s.beamTo + sd * (5.f * k), s.beamTo + sd * (13.f * k), 1.5f * k, white(0.7f));
            }
        }
        if (s.bladesT > 0.f)   // "Blades": each one smears behind as it spins
            for (int i = 0; i < s.bladeN; ++i)
                for (int j = 2; j >= 0; --j) {
                    const float ang = s.bladeAng - 0.14f * static_cast<float>(j) +
                                      2.f * kPi * static_cast<float>(i) / static_cast<float>(s.bladeN);
                    const sf::Vector2f d{std::cos(ang), std::sin(ang)};
                    const sf::Vector2f at = b.pos + d * (cfg::ability::bladeOrbit * k + b.radius);
                    const sf::Vector2f side{-d.y, d.x};
                    draw::line(t, at - side * (7.f * k), at + side * (7.f * k), (2.5f - 0.6f * j) * k,
                               j == 0 ? white(0.85f) : withAlpha(hot, 0.35f - 0.1f * static_cast<float>(j)));
                }
        if (s.leashT > 0.f) {   // "Leash": a beaded rope that runs, brighter when it's taut
            const sf::Vector2f d = b.pos - core;
            const float L = length(d);
            const bool taut = L >= s.leashL - 3.f * k;
            if (L > 1.f) {
                const sf::Vector2f dir = d / L;
                draw::line(t, core, b.pos, 1.f * k, white(taut ? 0.4f : 0.2f));
                const float gap = 12.f * k;
                for (float x = std::fmod(clk * 40.f * k, gap); x < L; x += gap)
                    draw::disc(t, core + dir * x, 1.6f * k, white(taut ? 0.7f : 0.4f), white(0.1f), {1.f, 1.f}, 6);
            }
            draw::ring(t, b.pos, b.radius + 3.f * k, 1.5f * k, white(0.4f));
        }
        if (s.empower > 0.f) {   // "Empower": a breathing ring, three sparks turning in on it
            const float br = 0.5f + 0.5f * std::sin(clk * 8.f);
            draw::ring(t, b.pos, b.radius + (4.f + 2.f * br) * k, 2.f * k, withAlpha(b.color, 0.6f + 0.3f * br));
            draw::glow(t, b.pos, b.radius * 2.f, b.color, 0.2f + 0.15f * br);
            for (int i = 0; i < 3; ++i) {
                const float an = clk * 4.f + static_cast<float>(i) * 2.094f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                const float r = b.radius + (9.f + 5.f * br) * k;
                draw::line(t, b.pos + sd * r, b.pos + sd * (r + 5.f * k), 2.f * k, withAlpha(hot, 0.85f));
            }
        }
        if (cw.tailT > 0.f && !b.held && length(b.vel) > 1.f) {   // "Tailwind": wind streaks behind every ball
            const sf::Vector2f dir = b.vel / length(b.vel), side{-dir.y, dir.x};
            const float a = clampf(cw.tailT / 0.4f, 0.f, 1.f);
            for (int i = 0; i < 3; ++i) {
                const float sd = static_cast<float>(i - 1);
                const float fl = 0.6f + 0.4f * std::sin(clk * 17.f + static_cast<float>(i) * 2.f + b.pos.x * 0.02f);
                const sf::Vector2f o = b.pos + side * (sd * b.radius * 0.8f) - dir * (b.radius * (1.1f + 0.3f * std::fabs(sd)));
                draw::line(t, o, o - dir * (b.radius * 1.8f * fl), 1.3f * k, withAlpha(theme::puSurge, 0.45f * a));
            }
        }
        if (b.overclockT > 0.f) {   // "Overclock": running hot - a flickering heat ring
            const float fl = 0.6f + 0.4f * std::sin(clk * 23.f + b.pos.y);
            const sf::Color heat = lerpColor(b.color, theme::elemFire, 0.6f);
            draw::glow(t, b.pos, b.radius * 2.2f, heat, 0.18f * fl);
            const float s0 = clk * 6.f;
            for (int i = 0; i < 3; ++i)
                draw::ring(t, b.pos, b.radius + 3.f * k, 1.5f * k, withAlpha(heat, 0.7f * fl),
                           s0 + static_cast<float>(i) * 2.094f, s0 + static_cast<float>(i) * 2.094f + 1.2f, 10);
        }
    }
    for (const Enemy& e : world.enemies()) {   // "Bounty": a turning gold ring, a coin bobbing over the head
        if (e.bounty <= 0 || e.hp <= 0.f) continue;
        const float s0 = clk * 2.f;
        for (int i = 0; i < 4; ++i)
            draw::ring(t, e.pos, e.radius + 5.f * k, 2.f * k, withAlpha(theme::puGolden, 0.85f),
                       s0 + static_cast<float>(i) * 1.571f, s0 + static_cast<float>(i) * 1.571f + 1.f, 10);
        const sf::Vector2f coin = e.pos - sf::Vector2f{0.f, e.radius + (13.f + 2.f * std::sin(clk * 5.f)) * k};
        draw::disc(t, coin, 4.f * k, withAlpha(lerpColor(theme::puGolden, sf::Color::White, 0.35f), 0.95f),
                   withAlpha(theme::puGolden, 0.9f), {std::fabs(std::cos(clk * 3.f)) * 0.8f + 0.2f, 1.f}, 12);
    }
    for (const CommonWorld::Fx& f : cw.fx) drawFx(t, f, k);
}

// Epic / Legendary items that act all the time: a quiet mark on the ball (or
// the enemy) while they're doing their thing, so you can tell when they apply.
void worldItemMarks(sf::RenderTarget& t, const World& world) {
    const float k = world.arenaScale();
    const float clk = classClock();
    for (const Ball& b : world.balls()) {
        const CommonMods& c = b.mods.cls.common;
        const CommonState& s = b.cls.common;
        const float R = b.radius;
        if (s.homeT > 0.f) {   // "Homecoming": heading home - a U-turn arrow wrapped round it
            const float an = std::atan2(b.vel.y, b.vel.x) + kPi;
            draw::ring(t, b.pos, R + 6.f * k, 1.8f * k, withAlpha(theme::classSlinger, 0.8f), an - 1.6f, an + 1.6f, 16);
            const sf::Vector2f tip = b.pos + sf::Vector2f{std::cos(an + 1.6f), std::sin(an + 1.6f)} * (R + 6.f * k);
            const sf::Vector2f tan{-std::sin(an + 1.6f), std::cos(an + 1.6f)};
            for (float sd : {-1.f, 1.f})
                draw::line(t, tip, tip - tan * (5.f * k) + normalized(tip - b.pos, {1.f, 0.f}) * (sd * 3.5f * k), 1.6f * k,
                           withAlpha(theme::classSlinger, 0.9f));
        }
        if (s.perfect) {   // "Perfect release": a perfect throw shines
            draw::glow(t, b.pos, R * 2.5f, sf::Color::White, 0.3f);
            for (int i = 0; i < 4; ++i) {
                const float an = clk * 3.f + static_cast<float>(i) * 1.571f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                draw::line(t, b.pos + sd * (R + 3.f * k), b.pos + sd * (R + 9.f * k), 1.5f * k, white(0.85f));
            }
        }
        if (c.juggernaut > 0.f)   // "Juggernaut": a heavy double rim
            draw::ring(t, b.pos, R + 2.5f * k, 2.5f * k, withAlpha(lerpColor(b.color, sf::Color::Black, 0.4f), 0.7f));
        if (s.spin > 0.05f) {   // "Minigun": barrels spinning up inside it
            const float an = clk * (4.f + 26.f * s.spin);
            for (int i = 0; i < 3; ++i) {
                const float a = an + static_cast<float>(i) * 2.094f;
                const sf::Vector2f sd{std::cos(a), std::sin(a)};
                draw::line(t, b.pos + sd * (R * 0.25f), b.pos + sd * (R * 0.8f), 1.5f * k, white(0.35f + 0.45f * s.spin));
            }
        }
        if (c.lightspeed > 0.f && length(b.vel) > cfg::ball::baseCruise * k * b.mods.cruiseMult * cfg::common::lightspeedAt) {   // "Lightspeed": crackling when it's fast enough
            const int fr = static_cast<int>(clk * 24.f);
            sf::Vector2f prev;
            for (int i = 0; i <= 10; ++i) {
                const float an = static_cast<float>(i) * 0.6283f;
                const float j = (hash01(static_cast<float>(fr * 11 + i)) - 0.5f) * 5.f * k;
                const sf::Vector2f q = b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (R + 4.f * k + j);
                if (i > 0) draw::line(t, prev, q, 1.2f * k, white(0.7f));
                prev = q;
            }
        }
        if (s.archStacks > 0) {   // "Archmage": a mote for each cast this fight (up to 8)
            const int n = std::min(s.archStacks, 8);
            for (int i = 0; i < n; ++i) {
                const float an = clk * 1.2f + static_cast<float>(i) * 6.283f / static_cast<float>(n);
                draw::disc(t, b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (R + 7.f * k), 1.6f * k,
                           withAlpha(theme::classMage, 0.9f), withAlpha(theme::classMage, 0.3f), {1.f, 1.f}, 6);
            }
        }
        if (s.showStacks > 0) {   // "Showtime": a spotlight that warms with every catch
            const float g = std::min(1.f, static_cast<float>(s.showStacks) / 8.f);
            draw::glow(t, b.pos, R * (2.f + g), theme::puGolden, 0.12f + 0.25f * g);
        }
        if (c.rigged) {   // "Rigged": a loaded die riding along
            const sf::Vector2f d = b.pos + sf::Vector2f{R * 0.9f, -R * 0.9f};
            draw::polygonOutline(t, d, 4.f * k, 4, 0.785f, 1.2f * k, withAlpha(theme::classJester, 0.9f));
            draw::disc(t, d, 1.f * k, white(0.9f), white(0.9f), {1.f, 1.f}, 6);
        }
        if (c.gunship > 0.f)   // "Gunship": two little wings
            for (float sd : {-1.f, 1.f})
                draw::line(t, b.pos + sf::Vector2f{sd * R, 0.f}, b.pos + sf::Vector2f{sd * (R + 6.f * k), 3.f * k}, 2.f * k,
                           withAlpha(theme::classShooter, 0.8f));
        if (b.preyId >= 0 && b.mods.hunterMult > 0.f)   // "Hunter": chevrons closing on its prey
            for (const Enemy& e : world.enemies())
                if (e.id == b.preyId && e.hp > 0.f) {
                    const float pulse = 0.5f + 0.5f * std::sin(clk * 6.f);
                    for (float sd : {-1.f, 1.f}) {
                        const sf::Vector2f c0 = e.pos + sf::Vector2f{sd * (e.radius + (6.f + 3.f * pulse) * k), 0.f};
                        draw::line(t, c0, c0 + sf::Vector2f{sd * 5.f * k, -5.f * k}, 1.6f * k, withAlpha(b.color, 0.85f));
                        draw::line(t, c0, c0 + sf::Vector2f{sd * 5.f * k, 5.f * k}, 1.6f * k, withAlpha(b.color, 0.85f));
                    }
                    break;
                }
    }
    for (const Enemy& e : world.enemies()) {   // "Death sentence": a small X over the sentenced
        if (e.sentenceT <= 0.f || e.hp <= 0.f) continue;
        const sf::Vector2f c = e.pos - sf::Vector2f{0.f, e.radius + 8.f * k};
        const float h = 3.5f * k;
        draw::line(t, c - sf::Vector2f{h, h}, c + sf::Vector2f{h, h}, 1.6f * k, withAlpha(theme::classAssassin, 0.9f));
        draw::line(t, c - sf::Vector2f{h, -h}, c + sf::Vector2f{h, -h}, 1.6f * k, withAlpha(theme::classAssassin, 0.9f));
    }
}

// The Summoner's dragons and the Hive's bugs (2026-10-05). A baby dragon: a
// small dart with flapping wings and a tail. The elder: the same shape, huge,
// slow wingbeats, a glow at the throat when it breathes. Bugs: dots with
// scurrying legs; a lit one glows.
void fillShape(sf::RenderTarget& t, std::initializer_list<sf::Vector2f> pts, sf::Color fill) {
    sf::ConvexShape sh(pts.size());
    std::size_t i = 0;
    for (const sf::Vector2f& q : pts) sh.setPoint(i++, q);
    sh.setFillColor(fill);
    t.draw(sh);
}

void drawDragon(sf::RenderTarget& t, const CommonWorld::Dragon& d, sf::Color c, float k) {
    const float S = (d.elder ? 46.f : 10.f) * k;   // body length
    const sf::Vector2f f = d.face, side{-f.y, f.x};
    const sf::Color body = lerpColor(c, sf::Color::Black, 0.25f);
    const sf::Color edge = lerpColor(c, sf::Color::White, 0.35f);
    const float flap = std::sin(d.flap) * 0.5f + 0.5f;   // 0 down .. 1 up
    if (d.elder) draw::glow(t, d.pos, S * 1.8f, c, 0.10f + 0.15f * d.roar);
    // tail: tapering segments that sway
    sf::Vector2f prev = d.pos - f * (S * 0.45f);
    for (int i = 1; i <= 5; ++i) {
        const float v = static_cast<float>(i) / 5.f;
        const sf::Vector2f q = d.pos - f * (S * (0.45f + 1.f * v)) + side * (std::sin(d.flap * 0.6f + v * 3.5f) * S * 0.18f * v);
        draw::line(t, prev, q, S * 0.16f * (1.f - 0.8f * v), withAlpha(body, 0.95f));
        prev = q;
    }
    fillShape(t, {prev, prev - f * (S * 0.15f) + side * (S * 0.1f), prev - f * (S * 0.15f) - side * (S * 0.1f)}, withAlpha(edge, 0.9f));   // tail tip
    // wings: membranes that beat
    for (float sd : {-1.f, 1.f}) {
        const sf::Vector2f root = d.pos + f * (S * 0.05f) + side * (sd * S * 0.08f);
        const sf::Vector2f elbow = root + side * (sd * S * (0.55f + 0.25f * flap)) + f * (S * 0.15f);
        const sf::Vector2f tip = root + side * (sd * S * (0.95f + 0.35f * flap)) - f * (S * (0.35f - 0.25f * flap));
        const sf::Vector2f back = d.pos - f * (S * 0.35f) + side * (sd * S * 0.1f);
        fillShape(t, {root, elbow, tip, back}, withAlpha(body, d.elder ? 0.75f : 0.85f));
        draw::line(t, root, elbow, (d.elder ? 2.5f : 1.f) * k, withAlpha(edge, 0.9f));
        draw::line(t, elbow, tip, (d.elder ? 2.f : 1.f) * k, withAlpha(edge, 0.9f));
    }
    // body, then a neck and head that turn on their own (toward what it breathes on)
    fillShape(t, {d.pos + f * (S * 0.3f), d.pos + side * (S * 0.14f), d.pos - f * (S * 0.5f), d.pos - side * (S * 0.14f)},
              withAlpha(body, 1.f));
    const sf::Vector2f hf = d.head, hside{-hf.y, hf.x};
    const sf::Vector2f neck = d.pos + f * (S * 0.25f);
    const sf::Vector2f head = neck + hf * (S * 0.3f);
    draw::line(t, neck, head, S * 0.13f, withAlpha(body, 1.f));
    fillShape(t, {head + hf * (S * 0.24f), head + hside * (S * 0.11f), head - hf * (S * 0.06f), head - hside * (S * 0.11f)}, withAlpha(edge, 1.f));
    if (d.elder)   // two eyes
        for (float sd : {-1.f, 1.f})
            draw::disc(t, head + hf * (S * 0.06f) + hside * (sd * S * 0.05f), 1.6f * k, sf::Color::White, sf::Color::White, {1.f, 1.f}, 6);
    if (d.charge >= 0.f) {   // drawing breath: fire gathering in its throat
        const float q = clampf(d.charge / (d.elder ? cfg::common::elderCharge : cfg::common::babyCharge), 0.f, 1.f);
        draw::glow(t, head + hf * (S * 0.2f), S * (0.2f + 0.5f * q), lerpColor(c, sf::Color(255, 220, 140), 0.5f), 0.3f + 0.5f * q);
        for (int i = 0; i < 4; ++i) {   // embers drawn in
            const float an = classClock() * 6.f + static_cast<float>(i) * 1.571f;
            const sf::Vector2f e = head + sf::Vector2f{std::cos(an), std::sin(an)} * (S * 0.6f * (1.f - q));
            draw::disc(t, e, (d.elder ? 2.5f : 1.2f) * k, withAlpha(sf::Color(255, 200, 120), 0.9f), withAlpha(c, 0.f), {1.f, 1.f}, 6);
        }
    }
}

void worldSummons(sf::RenderTarget& t, const World& world) {
    const CommonWorld& cw = world.classWorld().common;
    const float k = world.arenaScale();
    const float clk = classClock();
    for (const CommonWorld::Bug& g : cw.bugs) {
        const sf::Color c = g.elem != 0 ? elementColor(static_cast<Element>(g.elem)) : theme::classSummoner;
        const float a = clampf(g.life / 0.4f, 0.f, 1.f);
        const sf::Vector2f f = normalized(g.vel, {1.f, 0.f}), side{-f.y, f.x};
        const float wig = std::sin(g.life * 40.f) * 1.5f * k;
        for (float sd : {-1.f, 1.f})
            draw::line(t, g.pos + side * (sd * 1.5f * k), g.pos + side * (sd * 4.f * k) + f * wig, 1.f * k, withAlpha(c, 0.7f * a));
        if (g.lit) draw::glow(t, g.pos, 7.f * k, theme::elemFire, 0.5f * a);
        draw::disc(t, g.pos, 2.2f * k, withAlpha(lerpColor(c, sf::Color::White, 0.3f), a), withAlpha(c, 0.8f * a), {1.f, 1.f}, 8);
    }
    for (const CommonWorld::Dragon& d : cw.dragons) {
        sf::Color c = theme::classSummoner;
        for (const Ball& b : world.balls())
            if (b.owner == d.owner && !b.ghost) { c = b.element != Element::Plain ? elementColor(b.element) : b.color; break; }
        if (d.breathing > 0.f) {   // a sustained jet of fire out of its mouth, tracking with the head
            const float S = (d.elder ? 46.f : 10.f) * k;
            const sf::Vector2f mouth = d.pos + d.face * (S * 0.25f) + d.head * (S * 0.62f);
            const float L = d.elder ? cfg::common::elderLength * k : cfg::common::babyRange * k;
            const float spread = d.elder ? cfg::common::elderWidth * k : std::tan(cfg::common::babyCone) * L;
            const float total = d.elder ? cfg::common::elderBreathTime : cfg::common::babyBreathTime;
            const float age = total - d.breathing;
            const float grow = clampf(age / 0.2f, 0.f, 1.f), fade = clampf(d.breathing / 0.25f, 0.f, 1.f);
            const sf::Vector2f sd{-d.head.y, d.head.x};
            const int n = d.elder ? 34 : 16;
            for (int i = 0; i < n; ++i) {   // billows racing out, each one born at the mouth and swelling as it goes
                const float v = std::fmod(static_cast<float>(i) / static_cast<float>(n) + clk * (d.elder ? 1.6f : 2.6f), 1.f);
                if (v > grow) continue;
                const float w = (d.elder ? 0.25f + 0.75f * v : 0.1f + 0.9f * v) * spread;
                const sf::Vector2f q = mouth + d.head * (L * v) + sd * ((hash01(static_cast<float>(i) * 7.3f) - 0.5f) * w);
                const float pr = (d.elder ? 9.f + 28.f * v : 2.5f + 9.f * v) * k;
                const sf::Color core = v < 0.2f ? sf::Color(255, 248, 225) : v < 0.45f ? sf::Color(255, 205, 95) : c;
                draw::disc(t, q, pr, withAlpha(core, 0.6f * fade * (1.f - 0.5f * v)), withAlpha(lerpColor(c, sf::Color::Black, 0.4f), 0.f),
                           {1.f, 1.f}, 12);
            }
            draw::glow(t, mouth, S * 0.5f, sf::Color(255, 230, 160), 0.6f * fade);
        }
        drawDragon(t, d, c, k);
    }
}

// The Jester's two (2026-10-05). "777": three little reels over the ball;
// they spin, stop left to right and hold the result a moment. "Lucky number":
// the roll floats over the ball as a seven-segment digit - green above 5,
// red below, gold on a 10.
void drawSlotSymbol(sf::RenderTarget& t, sf::Vector2f c, float h, int sym, float a) {
    if (sym == 0) {   // a 7
        const sf::Color red = withAlpha(sf::Color(236, 70, 70), a);
        draw::line(t, c + sf::Vector2f{-h * 0.3f, -h * 0.35f}, c + sf::Vector2f{h * 0.3f, -h * 0.35f}, h * 0.16f, red);
        draw::line(t, c + sf::Vector2f{h * 0.3f, -h * 0.35f}, c + sf::Vector2f{-h * 0.05f, h * 0.38f}, h * 0.16f, red);
    } else if (sym == 1) {   // a clover
        const sf::Color g = withAlpha(sf::Color(110, 220, 120), a);
        for (int i = 0; i < 3; ++i) {
            const float an = -kPi * 0.5f + static_cast<float>(i) * 2.094f;
            draw::disc(t, c + sf::Vector2f{std::cos(an), std::sin(an)} * (h * 0.18f) - sf::Vector2f{0.f, h * 0.05f}, h * 0.16f, g, g, {1.f, 1.f}, 8);
        }
        draw::line(t, c + sf::Vector2f{0.f, h * 0.05f}, c + sf::Vector2f{h * 0.1f, h * 0.38f}, h * 0.07f, g);
    } else {   // a coin
        draw::disc(t, c, h * 0.3f, withAlpha(lerpColor(theme::puGolden, sf::Color::White, 0.4f), a), withAlpha(theme::puGolden, a), {1.f, 1.f}, 12);
    }
}

void drawDigit(sf::RenderTarget& t, sf::Vector2f c, float h, int d, sf::Color col) {   // seven segments
    static const unsigned char kSeg[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    const float w = h * 0.5f, hh = h * 0.5f, th = std::max(1.2f, h * 0.14f);
    const sf::Vector2f A{-w * 0.5f, -hh}, B{w * 0.5f, -hh}, C{w * 0.5f, 0.f}, D{w * 0.5f, hh}, E{-w * 0.5f, hh}, F{-w * 0.5f, 0.f};
    const sf::Vector2f seg[7][2] = {{A, B}, {B, C}, {C, D}, {D, E}, {E, F}, {F, A}, {F, C}};
    for (int i = 0; i < 7; ++i)
        if (kSeg[d] & (1 << i)) draw::line(t, c + seg[i][0], c + seg[i][1], th, col);
}

void worldJesterToys(sf::RenderTarget& t, const World& world) {
    const float k = world.arenaScale();
    const float clk = classClock();
    for (const CommonWorld::Coin& co : world.classWorld().common.coins)   // the coin rain
        draw::disc(t, co.pos, 5.f * k, withAlpha(lerpColor(theme::puGolden, sf::Color::White, 0.4f), 0.95f), withAlpha(theme::puGolden, 0.95f),
                   {std::fabs(std::cos(co.spin)) * 0.8f + 0.2f, 1.f}, 12);
    for (const Ball& b : world.balls()) {
        const CommonState& s = b.cls.common;
        const CommonMods& c = b.mods.cls.common;
        if (s.frenzyT > 0.f) {   // a frenzy: red heat and flicker
            const float fl = 0.6f + 0.4f * std::sin(clk * 30.f);
            draw::glow(t, b.pos, b.radius * 2.6f, sf::Color(236, 60, 60), 0.3f * fl);
        }
        const bool spinning = s.spinT >= 0.f;
        const float held = c.slotEvery - s.slotT;   // s since the last result
        if (c.slotEvery > 0.f && (spinning || held < 0.9f)) {   // the reels
            const float h = 14.f * k;
            const sf::Vector2f base = b.pos - sf::Vector2f{0.f, b.radius + 16.f * k};
            const float a = spinning ? 1.f : clampf((0.9f - held) / 0.3f, 0.f, 1.f);
            const bool triple = !spinning && s.reel[0] == s.reel[1] && s.reel[1] == s.reel[2];
            const sf::Vector2f shake = spinning ? sf::Vector2f{std::sin(clk * 70.f), std::cos(clk * 53.f)} * (1.2f * k) : sf::Vector2f{};
            const bool blink = static_cast<int>(clk * 16.f) % 2 == 0;   // marquee lights
            const sf::Color rim = triple ? theme::puGolden : spinning ? (blink ? sf::Color(250, 210, 40) : sf::Color(140, 60, 200)) : theme::classJester;
            if (triple) draw::glow(t, base, h * 3.f, theme::puGolden, 0.45f * a);
            draw::box(t, {base.x - h * 1.75f + shake.x, base.y - h * 0.65f + shake.y, h * 3.5f, h * 1.3f}, 2.f, withAlpha(theme::bgDeep, 0.9f * a),
                      withAlpha(theme::bgDeep, 0.9f * a), withAlpha(rim, 0.95f * a), 1.5f);
            for (int i = 0; i < 6; ++i) {   // bulbs round the frame
                const float x = base.x - h * 1.75f + h * 3.5f * (static_cast<float>(i) + 0.5f) / 6.f + shake.x;
                const bool on = spinning ? ((i + static_cast<int>(clk * 16.f)) % 2 == 0) : triple;
                draw::disc(t, {x, base.y - h * 0.65f + shake.y}, 1.6f * k, withAlpha(on ? sf::Color(255, 230, 120) : sf::Color(90, 80, 60), a),
                           withAlpha(on ? sf::Color(255, 230, 120) : sf::Color(90, 80, 60), a), {1.f, 1.f}, 6);
            }
            for (int i = 0; i < 3; ++i) {
                const sf::Vector2f cpos = base + shake + sf::Vector2f{(static_cast<float>(i) - 1.f) * h * 1.1f, 0.f};
                const float stopAt = cfg::common::slotSpin * (i == 2 ? 1.f : 0.3f + 0.25f * static_cast<float>(i));   // the last one keeps you waiting
                const bool rolling = spinning && s.spinT < stopAt;
                if (rolling) {   // a blur of symbols sliding down - the last reel slowing as it nears its stop
                    const float slow = i == 2 ? clampf(1.f - (s.spinT - stopAt * 0.6f) / (stopAt * 0.4f), 0.2f, 1.f) : 1.f;
                    const float rate = 28.f * slow;
                    const float roll = std::fmod(clk * rate + static_cast<float>(i) * 0.37f, 1.f);
                    const int sym = static_cast<int>(clk * rate + static_cast<float>(i)) % 3;
                    drawSlotSymbol(t, cpos + sf::Vector2f{0.f, (roll - 0.5f) * h * 0.6f}, h * 0.9f, sym, 0.55f * a);
                } else {
                    drawSlotSymbol(t, cpos, h, s.reel[i], a);
                }
            }
        }
        if (c.luckyEvery > 0.f && s.luckyN > 0) {   // the d10's roll
            const int n = s.luckyN;
            const sf::Color col = n == 10 ? theme::puGolden : n > 5 ? sf::Color(120, 230, 130) : n < 5 ? sf::Color(240, 100, 100) : sf::Color::White;
            const float pop = clampf((c.luckyEvery - s.luckyT) / 0.25f, 0.f, 1.f);   // pops in on a fresh roll
            const float h = 9.f * k * (0.6f + 0.4f * pop) * (1.f + 0.4f * (1.f - pop));
            const sf::Vector2f at = b.pos + sf::Vector2f{b.radius + 8.f * k, -b.radius - 6.f * k};
            if (n == 10) {
                drawDigit(t, at - sf::Vector2f{h * 0.35f, 0.f}, h, 1, withAlpha(col, 0.95f));
                drawDigit(t, at + sf::Vector2f{h * 0.4f, 0.f}, h, 0, withAlpha(col, 0.95f));
            } else {
                drawDigit(t, at, h, n, withAlpha(col, 0.95f));
            }
        }
    }
}

// The Alchemist's legendaries (2026-10-05): the toxic trail's puffs, the
// flamethrower, the rain, the bottles in the air, the plagued (black, with
// flies).
void worldAlchemy(sf::RenderTarget& t, const World& world) {
    const CommonWorld& cw = world.classWorld().common;
    const float k = world.arenaScale();
    const float clk = classClock();
    for (const CommonWorld::Cloud& c : cw.trail) {   // a sickly puff, its element showing through
        const float a = clampf(c.life / 0.6f, 0.f, 1.f) * clampf((c.maxLife - c.life) / 0.15f, 0.f, 1.f);
        const sf::Color ec = c.elem != 0 ? elementColor(static_cast<Element>(c.elem)) : theme::elemPoison;
        const sf::Color tox = lerpColor(ec, sf::Color(90, 140, 60), 0.35f);
        const float wob = 1.f + 0.08f * std::sin(clk * 3.f + c.pos.x * 0.05f);
        const float R = cfg::common::toxicNow(c.r, c.life, c.maxLife) * wob;
        draw::disc(t, c.pos, R, withAlpha(tox, 0.2f * a), withAlpha(tox, 0.02f * a), {1.f, 1.f}, 22);
        for (int i = 0; i < 2; ++i) {   // a couple of bubbles rising in it
            const float ph = std::fmod(clk * 0.8f + hash01(c.pos.x * 0.1f + static_cast<float>(i)), 1.f);
            draw::ring(t, c.pos + sf::Vector2f{(hash01(c.pos.y + static_cast<float>(i)) - 0.5f) * R, R * 0.4f - R * ph}, 2.2f * k, 1.f,
                       withAlpha(lerpColor(tox, sf::Color::White, 0.3f), 0.5f * a * (1.f - ph)), 0.f, 6.283f, 8);
        }
    }
    if (cw.rainT > 0.f) {   // rain: thin slanted streaks over the arena
        const float a = clampf(cw.rainT / 0.5f, 0.f, 1.f);
        const sf::Vector2f sz = world.size();
        for (int i = 0; i < 70; ++i) {
            const float x = hash01(static_cast<float>(i) * 3.3f) * sz.x;
            const float y = std::fmod(hash01(static_cast<float>(i) * 7.7f) * sz.y + clk * 700.f, sz.y + 40.f) - 20.f;
            draw::line(t, {x, y}, {x - 5.f * k, y + 16.f * k}, 1.f, withAlpha(elementColor(Element::Water), 0.35f * a));
        }
    }
    for (const CommonWorld::Field& f : cw.fields) {   // a poison puddle bubbling (frost draws as its own flourish)
        if (f.elem != static_cast<int>(Element::Poison)) continue;
        const float a = clampf(f.life / 0.5f, 0.f, 1.f);
        const sf::Color pc = elementColor(Element::Poison);
        draw::disc(t, f.pos, f.r, withAlpha(pc, 0.22f * a), withAlpha(pc, 0.05f * a), {1.f, 0.75f}, 24);
        for (int i = 0; i < 3; ++i) {
            const float ph = std::fmod(clk * 1.3f + static_cast<float>(i) * 0.33f + f.pos.x * 0.01f, 1.f);
            draw::ring(t, f.pos + sf::Vector2f{(static_cast<float>(i) - 1.f) * f.r * 0.4f, 0.f}, 3.f * k * ph + 1.f, 1.f,
                       withAlpha(lerpColor(pc, sf::Color::White, 0.4f), 0.7f * a * (1.f - ph)), 0.f, 6.283f, 8);
        }
    }
    for (const CommonWorld::Boulder& bo : cw.boulders) {   // a rock falling: a growing shadow, the rock dropping in
        if (bo.t < 0.f || bo.t >= 1.f) continue;
        const sf::Color sc = elementColor(Element::Stone);
        draw::disc(t, bo.at, 22.f * k * bo.t, withAlpha(sf::Color::Black, 0.35f * bo.t), withAlpha(sf::Color::Black, 0.f), {1.f, 0.6f}, 16);
        const sf::Vector2f q = bo.at - sf::Vector2f{0.f, 260.f * k * (1.f - bo.t)};
        draw::polygon(t, q, 14.f * k, 6, bo.t * 4.f, withAlpha(lerpColor(sc, sf::Color::White, 0.15f), 0.95f), withAlpha(sc, 0.95f));
    }
    for (const CommonWorld::Bottle& bt : cw.bottles) {   // a bottle lobbed in an arc, tumbling
        const sf::Vector2f q = lerpv(bt.from, bt.to, bt.t) - sf::Vector2f{0.f, 80.f * k * std::sin(bt.t * kPi)};
        const sf::Color pc = elementColor(Element::Poison);
        draw::polygon(t, q, 4.f * k, 4, bt.t * 9.f, withAlpha(pc, 0.95f), withAlpha(lerpColor(pc, sf::Color::Black, 0.3f), 0.95f));
        draw::line(t, q, q + sf::Vector2f{std::cos(bt.t * 9.f), std::sin(bt.t * 9.f)} * (6.f * k), 1.5f * k, withAlpha(sf::Color::White, 0.7f));
    }
    for (const Ball& b : world.balls()) {   // the flamethrower: a roaring tongue at the nearest enemy
        const CommonState& s = b.cls.common;
        if (s.flameT <= 0.f) continue;
        const float an = std::atan2(s.flameDir.y, s.flameDir.x);
        const float R = cfg::common::flameRange * k;
        const int fr = static_cast<int>(clk * 30.f);
        for (int i = 0; i < 9; ++i) {
            const float a = an + (static_cast<float>(i) / 8.f - 0.5f) * 2.f * cfg::common::flameCone;
            const float l = R * (0.6f + 0.4f * hash01(static_cast<float>(i * 17 + fr)));
            const sf::Vector2f d{std::cos(a), std::sin(a)};
            draw::line(t, b.pos, b.pos + d * l, 7.f * k, withAlpha(theme::elemFire, 0.16f));
            draw::line(t, b.pos + d * (b.radius), b.pos + d * (l * 0.7f), 2.f * k, withAlpha(sf::Color(255, 220, 140), 0.6f));
        }
    }
    for (const Enemy& e : world.enemies()) {   // the plagued: blackened, flies buzzing round
        if (e.plagueT <= 0.f || e.hp <= 0.f) continue;
        draw::disc(t, e.pos, e.radius * 1.05f, withAlpha(sf::Color(12, 10, 14), 0.8f), withAlpha(sf::Color(40, 30, 50), 0.6f), {1.f, 1.f}, 20);
        for (int i = 0; i < 3; ++i) {
            const float a = clk * (5.f + static_cast<float>(i)) + static_cast<float>(i) * 2.1f;
            draw::disc(t, e.pos + sf::Vector2f{std::cos(a), std::sin(a * 1.3f)} * (e.radius + 5.f * k), 1.3f * k,
                       withAlpha(sf::Color(30, 30, 30), 0.95f), withAlpha(sf::Color(30, 30, 30), 0.95f), {1.f, 1.f}, 6);
        }
    }
}

// ==================================================================== Jester
void markJester(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {
    for (float side : {-1.f, 1.f})
        draw::disc(t, p + sf::Vector2f{side * r * 0.42f, -side * r * 0.42f}, std::max(1.2f, r * 0.1f),
                   white(0.8f * a), white(0.55f * a), {1.f, 1.f}, 10);
}
// A double: two small pips pop up over the enemy and fade (spaced in the sim).
void worldJester(sf::RenderTarget& t, const World& world) {
    for (const JesterWorld::Pop& q : world.classWorld().jester.pops) {
        const float k = q.t / cfg::jester::popLife;   // 1 -> 0
        const sf::Vector2f c = q.pos + sf::Vector2f{0.f, -18.f - 10.f * (1.f - k)};
        for (float side : {-1.f, 1.f})
            draw::disc(t, c + sf::Vector2f{side * 4.f, -side * 4.f}, 2.2f, white(0.85f * k), white(0.5f * k),
                       {1.f, 1.f}, 10);
    }
}

}  // namespace

// ---------------------------------------------------------------- identity

BallLook ballLook(const Ball& b) {
    BallLook l;
    l.roles = b.roles;
    l.ascended = b.ascended;
    l.lead = b.leadRole();
    l.second = b.secondRole();
    l.element = b.element;
    l.legend = b.legend;
    return l;
}

// Legendary cosmetics (2026-10-05): a ball carrying a class's Legendary wears
// that class's thing. Hats sit on top whatever the heading; one hat at most.
void drawLegendCosmetics(sf::RenderTarget& t, RoleMask legend, sf::Vector2f p, float r, float heading, float a) {
    a *= 0.9f;
    auto has = [&](BallRole c) { return (legend & roleBit(c)) != 0; };
    auto C = [&](int R, int G, int B) { return withAlpha(sf::Color(static_cast<sf::Uint8>(R), static_cast<sf::Uint8>(G), static_cast<sf::Uint8>(B)), a); };
    const sf::Color dark = C(28, 26, 34);
    const float facing = std::cos(heading) >= 0.f ? 1.f : -1.f;   // which way it looks (beaks, the scouter)
    const sf::Vector2f up{0.f, -1.f};
    const float wave = std::sin(classClock() * 9.f) * r * 0.12f;
    bool hat = false;
    auto band = [&](sf::Color col) {   // a band straight across the whole ball, the knot's tails streaming behind
        const float y = -r * 0.35f;
        const float half = std::sqrt(std::max(0.f, r * r - y * y));
        draw::line(t, p + sf::Vector2f{-half, y}, p + sf::Vector2f{half, y}, r * 0.3f, col);
        const sf::Vector2f knot = p + sf::Vector2f{-facing * half, y};
        draw::line(t, knot, knot + sf::Vector2f{-facing * r * 0.75f, -r * 0.15f + wave}, r * 0.16f, col);
        draw::line(t, knot, knot + sf::Vector2f{-facing * r * 0.65f, r * 0.25f - wave}, r * 0.14f, col);
    };
    if (has(BallRole::Striker)) band(C(214, 40, 48));   // Rambo's red headband
    if (has(BallRole::Support) && !has(BallRole::Striker)) {   // a plain white medic's band (no tails)...
        const float y = -r * 0.35f;
        const float half = std::sqrt(std::max(0.f, r * r - y * y));
        draw::line(t, p + sf::Vector2f{-half, y}, p + sf::Vector2f{half, y}, r * 0.3f, C(245, 245, 250));
    }
    if (has(BallRole::Support)) {   // ...with a red cross on the front
        const sf::Vector2f c = p + sf::Vector2f{facing * r * 0.3f, -r * 0.35f};
        draw::line(t, c - sf::Vector2f{r * 0.18f, 0.f}, c + sf::Vector2f{r * 0.18f, 0.f}, r * 0.1f, C(220, 40, 50));
        draw::line(t, c - sf::Vector2f{0.f, r * 0.13f}, c + sf::Vector2f{0.f, r * 0.13f}, r * 0.1f, C(220, 40, 50));
    }
    if (has(BallRole::Shooter)) {   // a one-eyed red scouter
        const sf::Vector2f eye = p + sf::Vector2f{facing * r * 0.35f, -r * 0.2f};
        const sf::Vector2f ear = p + sf::Vector2f{facing * r * 0.95f, -r * 0.05f};
        draw::line(t, ear, eye + sf::Vector2f{facing * r * 0.2f, 0.f}, r * 0.1f, C(70, 72, 80));
        draw::line(t, ear, ear + sf::Vector2f{0.f, r * 0.3f}, r * 0.14f, C(70, 72, 80));
        fillShape(t, {eye + sf::Vector2f{-r * 0.3f, -r * 0.2f}, eye + sf::Vector2f{r * 0.3f, -r * 0.24f},
                      eye + sf::Vector2f{r * 0.3f, r * 0.18f}, eye + sf::Vector2f{-r * 0.3f, r * 0.14f}}, C(235, 40, 50));
        draw::line(t, eye + sf::Vector2f{-r * 0.2f, -r * 0.1f}, eye + sf::Vector2f{r * 0.05f, -r * 0.12f}, r * 0.06f, C(255, 170, 170));
    }
    if (has(BallRole::Guardian)) {   // a dark grey kite shield on its side
        const sf::Vector2f c = p + sf::Vector2f{facing * r * 0.95f, r * 0.35f};
        fillShape(t, {c + sf::Vector2f{0.f, -r * 0.55f}, c + sf::Vector2f{r * 0.42f, -r * 0.32f}, c + sf::Vector2f{0.f, r * 0.6f},
                      c + sf::Vector2f{-r * 0.42f, -r * 0.32f}}, C(74, 78, 88));
        draw::line(t, c + sf::Vector2f{0.f, -r * 0.42f}, c + sf::Vector2f{0.f, r * 0.45f}, std::max(1.f, r * 0.1f), C(140, 146, 158));
    }
    if (has(BallRole::Assassin)) {   // a broad dagger at its hip
        const sf::Vector2f h = p + sf::Vector2f{-facing * r * 0.9f, r * 0.4f};
        const sf::Vector2f d = normalized(sf::Vector2f{-facing * 0.4f, 1.f}, {0.f, 1.f}), sd{-d.y, d.x};
        fillShape(t, {h + sd * (r * 0.16f), h + d * (r * 1.15f), h - sd * (r * 0.16f)}, C(232, 236, 244));   // the blade
        draw::line(t, h - sd * (r * 0.3f), h + sd * (r * 0.3f), r * 0.14f, C(150, 40, 60));   // guard
        draw::line(t, h, h - d * (r * 0.35f), r * 0.16f, dark);   // grip
    }
    const sf::Vector2f crown = p + up * (r * 0.8f);
    if (has(BallRole::Mage)) {   // a battered old wizard's hat, its tip folded over
        const sf::Color hc = C(96, 82, 70), hd = C(66, 56, 48);
        draw::line(t, crown + sf::Vector2f{-r * 1.f, 0.f}, crown + sf::Vector2f{r * 1.f, 0.f}, r * 0.2f, hd);   // brim
        const sf::Vector2f bend = crown + sf::Vector2f{facing * r * 0.15f, -r * 1.f};
        fillShape(t, {crown + sf::Vector2f{-r * 0.6f, 0.f}, crown + sf::Vector2f{r * 0.6f, 0.f}, bend + sf::Vector2f{r * 0.16f, 0.f},
                      bend - sf::Vector2f{r * 0.16f, 0.f}}, hc);
        fillShape(t, {bend - sf::Vector2f{r * 0.16f, 0.f}, bend + sf::Vector2f{r * 0.16f, 0.f},
                      bend + sf::Vector2f{-facing * r * 0.6f, r * 0.15f + wave * 0.3f}}, hd);   // the fold drooping over
        draw::line(t, crown + sf::Vector2f{-r * 0.4f, -r * 0.35f}, crown + sf::Vector2f{r * 0.3f, -r * 0.4f}, r * 0.06f, hd);   // a crease
        hat = true;
    }
    if (!hat && has(BallRole::Jester)) {   // a jester's cap: purple and yellow points, bells
        for (float sd : {-1.f, 1.f}) {
            const sf::Vector2f tip = crown + sf::Vector2f{sd * r * 1.25f, -r * 0.15f + (sd < 0.f ? wave : -wave)};
            fillShape(t, {crown + sf::Vector2f{-r * 0.1f * sd, r * 0.15f}, crown + sf::Vector2f{sd * r * 0.6f, -r * 0.95f}, tip,
                          crown + sf::Vector2f{sd * r * 0.65f, r * 0.15f}}, sd < 0.f ? C(140, 60, 200) : C(250, 210, 40));
            draw::disc(t, tip, r * 0.17f, C(250, 210, 40), C(250, 210, 40), {1.f, 1.f}, 8);
        }
        draw::line(t, crown + sf::Vector2f{-r * 0.75f, r * 0.15f}, crown + sf::Vector2f{r * 0.75f, r * 0.15f}, r * 0.16f, C(140, 60, 200));
        hat = true;
    }
    if (!hat && has(BallRole::Slinger)) {   // a big cowboy hat
        const sf::Color sc = C(150, 100, 52), sd = C(110, 70, 36);
        draw::disc(t, crown + sf::Vector2f{0.f, r * 0.05f}, r * 1.35f, sc, sc, {1.f, 0.24f}, 24);
        fillShape(t, {crown + sf::Vector2f{-r * 0.55f, r * 0.02f}, crown + sf::Vector2f{-r * 0.5f, -r * 0.75f},
                      crown + sf::Vector2f{0.f, -r * 0.58f}, crown + sf::Vector2f{r * 0.5f, -r * 0.75f}, crown + sf::Vector2f{r * 0.55f, r * 0.02f}}, sc);
        draw::line(t, crown + sf::Vector2f{-r * 0.53f, -r * 0.12f}, crown + sf::Vector2f{r * 0.53f, -r * 0.12f}, r * 0.14f, sd);   // hat band
        hat = true;
    }
    if (has(BallRole::Alchemist)) {   // a plague doctor's mask: a long curved beak, round glass eyes
        const sf::Color leather = C(22, 20, 24), shade = C(70, 66, 74);
        const sf::Vector2f eye = p + sf::Vector2f{facing * r * 0.25f, -r * 0.25f};
        const sf::Vector2f root = p + sf::Vector2f{facing * r * 0.45f, r * 0.05f};
        fillShape(t, {root + sf::Vector2f{0.f, -r * 0.32f}, root + sf::Vector2f{facing * r * 0.7f, -r * 0.05f},
                      root + sf::Vector2f{facing * r * 1.35f, r * 0.55f}, root + sf::Vector2f{facing * r * 0.5f, r * 0.3f},
                      root + sf::Vector2f{0.f, r * 0.3f}}, leather);
        draw::line(t, root + sf::Vector2f{0.f, -r * 0.05f}, root + sf::Vector2f{facing * r * 1.1f, r * 0.42f}, r * 0.05f, shade);
        for (float o : {0.f, -facing * r * 0.5f}) {
            draw::disc(t, eye + sf::Vector2f{o, 0.f}, r * 0.2f, C(70, 66, 74), C(70, 66, 74), {1.f, 1.f}, 12);
            draw::disc(t, eye + sf::Vector2f{o, 0.f}, r * 0.11f, C(170, 220, 160), C(110, 160, 100), {1.f, 1.f}, 10);
        }
    }
    if (!hat && has(BallRole::Summoner))   // two red horns, set wide
        for (float sd : {-1.f, 1.f})
            fillShape(t, {crown + sf::Vector2f{sd * r * 0.4f, r * 0.2f}, crown + sf::Vector2f{sd * r * 0.85f, r * 0.2f},
                          crown + sf::Vector2f{sd * r * 1.05f, -r * 0.6f}}, C(200, 40, 40));
}

void drawBallIdentity(sf::RenderTarget& t, const BallLook& look, sf::Vector2f pos, float r, float heading,
                      float alpha, bool held) {
    // Layers, inside out, each told apart by shape and place (not only hue -
    // a Shooter's orange and fire's orange must still read as two things):
    //  body          = the lead class's colour (the disc itself);
    //  second class  = a thick SOLID band just inside the edge, its colour pure;
    //  element       = a thin DASHED ring outside the edge, past a dark gap.
    if (look.second != BallRole::Normal) {
        const float band = std::max(2.2f, r * 0.2f);   // thin enough that the body (the lead class) still dominates
        draw::ring(t, pos, r - band * 0.5f, band,
                   withAlpha(lerpColor(roleColor(look.second), sf::Color::White, 0.08f), 0.97f * alpha));
        draw::ring(t, pos, r - band - 0.4f, 1.f, withAlpha(theme::bgDeep, 0.35f * alpha));   // seam against the body
    }
    draw::ring(t, pos, r, 1.5f, withAlpha(sf::Color::White, (held ? 0.85f : 0.22f) * alpha));
    if (look.element != Element::Plain) {
        const sf::Color ec = lerpColor(elementColor(look.element), sf::Color::White, 0.12f);
        const float er = r + 4.f;
        constexpr int kDashes = 10;
        for (int k = 0; k < kDashes; ++k) {
            const float a0 = static_cast<float>(k) * 2.f * kPi / kDashes;
            draw::ring(t, pos, er, 2.4f, withAlpha(ec, 0.95f * alpha), a0, a0 + 0.62f * 2.f * kPi / kDashes, 6);
        }
        draw::ring(t, pos, er + 3.f, 1.f, withAlpha(ec, 0.2f * alpha));   // a faint halo of its colour
    }
    for (int i = 0; i < kClassCount; ++i) {
        const BallRole c = classAt(i);
        if ((look.roles & roleBit(c)) == 0) continue;
        const bool up = (look.ascended & roleBit(c)) != 0;
        drawClassMark(t, c, pos, r, heading, alpha * (up ? 1.f : 0.8f));
    }
    // Ascended: a class-coloured halo with four slowly turning ticks.
    for (int i = 0; i < kClassCount; ++i) {
        const BallRole c = classAt(i);
        if ((look.ascended & look.roles & roleBit(c)) == 0) continue;
        const sf::Color hc = lerpColor(roleColor(c), sf::Color::White, 0.25f);
        const float hr = r + 7.f;
        draw::ring(t, pos, hr, 1.5f, withAlpha(hc, 0.7f * alpha));
        const float spin = classClock() * 0.6f;
        for (int k = 0; k < 4; ++k) {
            const float a = spin + kPi * 0.5f * static_cast<float>(k);
            draw::ring(t, pos, hr, 3.f, withAlpha(hc, 0.9f * alpha), a - 0.12f, a + 0.12f, 8);
        }
        break;   // a ball ascends in one class at most
    }
    if (look.legend != 0) drawLegendCosmetics(t, look.legend, pos, r, heading, alpha);
}

void drawClassPulse(sf::RenderTarget& t, BallRole lead, bool ascended, sf::Vector2f pos, float r, float k) {
    if (k <= 0.f || lead == BallRole::Normal) return;
    const sf::Color c = lerpColor(roleColor(lead), sf::Color::White, 0.2f);
    const float e = 1.f - k;                          // 0 -> 1 as it plays
    const float grow = 1.f - (1.f - e) * (1.f - e);   // ease-out
    draw::glow(t, pos, r * (2.2f + (ascended ? 1.6f : 0.8f) * grow), c, 0.22f * k);
    draw::ring(t, pos, r * (1.15f + (ascended ? 2.8f : 1.9f) * grow), 1.f + 2.5f * k, withAlpha(c, 0.85f * k));
    if (ascended)
        draw::ring(t, pos, r * (1.1f + 1.7f * grow), 1.f + 1.5f * k, withAlpha(c, 0.55f * k));
}

// A ball that just took an element: two rings of its colour ripple out.
void drawElementPulse(sf::RenderTarget& t, Element el, sf::Vector2f pos, float r, float k) {
    if (k <= 0.f || el == Element::Plain) return;
    const sf::Color c = lerpColor(elementColor(el), sf::Color::White, 0.15f);
    const float e = 1.f - k;
    const float grow = 1.f - (1.f - e) * (1.f - e);
    draw::glow(t, pos, r * (2.f + 1.2f * grow), c, 0.2f * k);
    draw::ring(t, pos, r * (1.2f + 2.2f * grow), 1.f + 2.5f * k, withAlpha(c, 0.9f * k));
    const float e2 = clampf(e * 1.4f - 0.3f, 0.f, 1.f);   // a second ring a beat later
    if (e2 > 0.f) draw::ring(t, pos, r * (1.2f + 1.6f * e2), 1.f + 1.5f * k, withAlpha(c, 0.6f * k));
}

// ---------------------------------------------------------------- dispatch

void drawClassMark(sf::RenderTarget& t, BallRole role, sf::Vector2f pos, float r, float heading, float alpha) {
    switch (role) {
        case BallRole::Normal:   break;
        case BallRole::Striker:  markStriker(t, pos, r, heading, alpha); break;
        case BallRole::Guardian: markGuardian(t, pos, r, heading, alpha); break;
        case BallRole::Support:  markSupport(t, pos, r, heading, alpha); break;
        case BallRole::Mage:     markMage(t, pos, r, heading, alpha); break;
        case BallRole::Shooter:  markShooter(t, pos, r, heading, alpha); break;
        case BallRole::Assassin: markAssassin(t, pos, r, heading, alpha); break;
        case BallRole::Summoner: markSummoner(t, pos, r, heading, alpha); break;
        case BallRole::Jester:   markJester(t, pos, r, heading, alpha); break;
        case BallRole::Slinger:  markSlinger(t, pos, r, heading, alpha); break;
        case BallRole::Alchemist: markAlchemist(t, pos, r, heading, alpha); break;
    }
}

void drawClassWorld(sf::RenderTarget& t, const World& world) {
    worldMage(t, world);
    worldShooter(t, world);
    worldAssassin(t, world);
    worldSummoner(t, world);
    worldJester(t, world);
    worldSlinger(t, world);
    worldStyle(t, world);
    worldAbilities(t, world);
    worldItemMarks(t, world);
    worldSummons(t, world);
    worldJesterToys(t, world);
    worldAlchemy(t, world);
}

}  // namespace sb
