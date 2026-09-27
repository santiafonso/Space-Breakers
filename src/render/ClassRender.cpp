// Class visuals: each class's mark on the ball and its own things on the
// field. One section per class; the dispatch is at the bottom.
//
// Marks layer from the outside in so two classes read together: Guardian = a
// heavy rim, Summoner = motes circling inside the rim, Assassin = a thin arc
// trailing behind, Support = an inner ring, Mage = a small diamond, Jester =
// two pips, Striker = a centre dot, Shooter = a short barrel toward its
// heading. White at low alpha - the element colour stays the ball's colour.

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
    for (const ShooterRail& r : sw.rails) {
        const float f = clampf(r.life / cfg::shooter::deadeyeRailLife, 0.f, 1.f);
        draw::line(t, r.a, r.b, 1.f + 1.5f * f, white(0.45f * f));
    }
    const float k = world.arenaScale();
    for (const ShooterBullet& u : sw.bullets) {
        const sf::Color c = u.elem != 0 ? lerpColor(elementColor(static_cast<Element>(u.elem)), sf::Color::White, 0.3f) : sf::Color::White;
        const float fade = clampf(u.life / 0.15f, 0.f, 1.f);
        draw::line(t, u.pos - u.vel * 0.03f, u.pos, 2.f * k, withAlpha(c, 0.3f * fade));
        draw::disc(t, u.pos, 2.6f * k, withAlpha(c, 0.85f * fade), withAlpha(c, 0.4f * fade), {1.f, 1.f}, 10);
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
            draw::glow(t, s.pos, 10.f, c, 0.10f);
            draw::disc(t, s.pos, cfg::summoner::wispRadius, withAlpha(c, 0.75f), withAlpha(c, 0.35f), {1.f, 1.f}, 12);
        } else {
            draw::disc(t, s.pos, cfg::summoner::shotRadius, withAlpha(c, 0.8f), withAlpha(c, 0.5f), {1.f, 1.f}, 8);
        }
    }

    for (const Ball& b : world.balls()) {
        const SummonerMods& m = b.mods.cls.summoner;
        const SummonerState& s = b.cls.summoner;
        const sf::Color c = summonColor(static_cast<int>(b.element));
        for (int i = 0; i < m.wardens; ++i) {
            const sf::Vector2f at = summonerWardenPos(world.core().pos, s.wardenAng, b.owner, i, m.wardens,
                                                      cfg::summoner::wardenOrbit * world.arenaScale());
            const float a = s.wardenRest[i] > 0.f ? 0.25f : 0.6f;   // dim while it rests after a hit
            draw::disc(t, at, cfg::summoner::wardenRadius * 0.8f, withAlpha(c, a), withAlpha(c, a * 0.5f), {1.f, 1.f}, 14);
        }
        if (m.dragonInterval > 0.f && s.dragonOut) {
            const float h = s.dragonHeading;
            const sf::Vector2f f{std::cos(h), std::sin(h)}, n{-f.y, f.x};
            const sf::Vector2f p = s.dragonPos;
            const float flap = 0.6f + 0.4f * std::sin(classClock() * 11.f + static_cast<float>(b.owner));
            // a small round body, a snout, and two wings that flap (no
            // arrowhead - that shape reads as an enemy)
            const sf::Color cc = withAlpha(c, 0.7f);
            for (float sd : {-1.f, 1.f})
                draw::line(t, p, p - f * (3.f * flap) + n * (sd * 8.f * flap), 1.5f, withAlpha(c, 0.5f));
            draw::line(t, p, p + f * 6.f, 1.5f, cc);
            draw::disc(t, p, 3.5f, cc, withAlpha(c, 0.45f), {1.f, 1.f}, 12);
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
    }
}

void drawClassWorld(sf::RenderTarget& t, const World& world) {
    worldMage(t, world);
    worldShooter(t, world);
    worldAssassin(t, world);
    worldSummoner(t, world);
    worldJester(t, world);
}

}  // namespace sb
