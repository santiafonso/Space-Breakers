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
    for (const Ball& b : world.balls()) {   // "Ion cannon" charging: a sight line that firms up, a glow
        const float c = b.cls.common.ionT;
        if (b.mods.cls.common.ion <= 0.f || c <= 0.05f) continue;
        draw::line(t, b.pos, b.pos + b.cls.common.ionDir * 2000.f, (0.6f + 1.4f * c) * k,
                   withAlpha(theme::classShooter, 0.08f + 0.3f * c * c));
        draw::glow(t, b.pos, b.radius * (1.f + 1.5f * c), theme::classShooter, 0.25f * c);
    }
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
        if (s.boom) {   // "Necromancer": a pale ghost
            const sf::Color g = lerpColor(c, sf::Color::White, 0.6f);
            draw::glow(t, s.pos, 14.f, g, 0.12f);
            draw::disc(t, s.pos, cfg::summoner::wispRadius * 1.3f, withAlpha(g, 0.55f), withAlpha(g, 0.2f), {1.f, 1.f}, 12);
        } else if (s.wisp) {
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
    // ---- the second batch of abilities (2026-10-05): quiet shapes, the ball's colours
    for (const CommonWorld::Mine& m : cw.mines) {   // "Mines": a small dot, a ring once armed
        const sf::Color c = m.elem != 0 ? elementColor(static_cast<Element>(m.elem)) : theme::accent;
        const float a = clampf(m.life / 0.5f, 0.f, 1.f);
        draw::disc(t, m.pos, 3.5f * k, withAlpha(c, 0.85f * a), withAlpha(c, 0.4f * a), {1.f, 1.f}, 10);
        if (m.arm <= 0.f) draw::ring(t, m.pos, 7.f * k, 1.f * k, withAlpha(c, 0.45f * a));
    }
    for (const CommonWorld::Vortex& v : cw.vortices) {   // "Vortex": two arcs turning in
        const float a = clampf(v.life / 0.4f, 0.f, 1.f) * clampf((v.maxLife - v.life) / 0.2f, 0.f, 1.f);
        const sf::Color c = v.elem != 0 ? elementColor(static_cast<Element>(v.elem)) : sf::Color::White;
        const float spin = classClock() * 5.f;
        for (int i = 0; i < 3; ++i) {
            const float r = v.r * (0.35f + 0.3f * static_cast<float>(i));
            const float s0 = spin * (1.f + 0.4f * static_cast<float>(i)) + static_cast<float>(i);
            draw::ring(t, v.pos, r, 1.5f * k, withAlpha(c, 0.4f * a), s0, s0 + 2.2f, 20);
        }
    }
    for (const CommonWorld::Cloud& cl : cw.clouds) {   // "Cloud": a soft tinted disc
        const float a = clampf(cl.life / 0.5f, 0.f, 1.f) * clampf((cl.maxLife - cl.life) / 0.3f, 0.f, 1.f);
        const sf::Color c = cl.elem != 0 ? elementColor(static_cast<Element>(cl.elem)) : sf::Color::White;
        draw::disc(t, cl.pos, cl.r, withAlpha(c, 0.16f * a), withAlpha(c, 0.02f * a), {1.f, 1.f}, 32);
    }
    for (const CommonWorld::Portal& q : cw.portals) {   // "Portal": a pair of turning rings
        const float a = clampf(q.life / 0.4f, 0.f, 1.f) * clampf((q.maxLife - q.life) / 0.25f, 0.f, 1.f);
        const float R = cfg::ability::portalRadius * k;
        const float spin = classClock() * 3.f;
        for (sf::Vector2f at : {q.a, q.b}) {
            draw::disc(t, at, R, withAlpha(theme::classMage, 0.12f * a), withAlpha(theme::classMage, 0.f), {1.f, 1.f}, 24);
            draw::ring(t, at, R, 2.f * k, withAlpha(theme::classMage, 0.75f * a), spin, spin + 4.5f, 24);
        }
    }
    for (const Ball& b : world.balls()) {
        const CommonState& s = b.cls.common;
        if (s.beamOn)   // "Beam"
            draw::line(t, b.pos, s.beamTo, 3.f * k, withAlpha(lerpColor(b.color, sf::Color::White, 0.5f), 0.8f));
        if (s.bladesT > 0.f)   // "Blades"
            for (int i = 0; i < s.bladeN; ++i) {
                const float ang = s.bladeAng + 2.f * kPi * static_cast<float>(i) / static_cast<float>(s.bladeN);
                const sf::Vector2f d{std::cos(ang), std::sin(ang)};
                const sf::Vector2f at = b.pos + d * (cfg::ability::bladeOrbit * k + b.radius);
                const sf::Vector2f side{-d.y, d.x};
                draw::line(t, at - side * (7.f * k), at + side * (7.f * k), 2.5f * k, white(0.8f));
            }
        if (s.leashT > 0.f)   // "Leash": the rope
            draw::line(t, world.core().pos, b.pos, 1.5f * k, white(0.35f));
        if (s.empower > 0.f)   // "Empower": armed
            draw::ring(t, b.pos, b.radius + 4.f * k, 2.f * k, withAlpha(b.color, 0.8f));
    }
    for (const Enemy& e : world.enemies())   // "Bounty": a gold ring
        if (e.bounty > 0 && e.hp > 0.f) draw::ring(t, e.pos, e.radius + 5.f * k, 2.f * k, withAlpha(theme::puGolden, 0.85f));
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
    return l;
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
}

}  // namespace sb
