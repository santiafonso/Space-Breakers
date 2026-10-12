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

void markAstronomer(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // a star in a tilted orbit
    const float clk = classClock();
    draw::polygon(t, p, r * 0.22f, 4, clk * 0.6f, white(0.85f * a), white(0.6f * a));
    for (int i = 0; i < 16; ++i) {   // the orbit: a flattened dotted ellipse
        const float an = static_cast<float>(i) / 16.f * 2.f * kPi;
        const sf::Vector2f e{std::cos(an) * r * 0.55f, std::sin(an) * r * 0.22f};
        const sf::Vector2f q{e.x * 0.94f - e.y * 0.34f, e.x * 0.34f + e.y * 0.94f};
        draw::disc(t, p + q, 0.9f, white(0.5f * a), white(0.f), {1.f, 1.f}, 4);
    }
    const float an = clk * 1.5f;
    const sf::Vector2f e{std::cos(an) * r * 0.55f, std::sin(an) * r * 0.22f};
    draw::disc(t, p + sf::Vector2f{e.x * 0.94f - e.y * 0.34f, e.x * 0.34f + e.y * 0.94f}, r * 0.09f, white(0.9f * a),
               white(0.f), {1.f, 1.f}, 8);   // its planet
}

void markHermes(sf::RenderTarget& t, sf::Vector2f p, float r, float heading, float a) {   // two little wings
    const sf::Vector2f d{std::cos(heading), std::sin(heading)}, sd{-d.y, d.x};
    for (int s = -1; s <= 1; s += 2)
        for (int f = 0; f < 3; ++f) {   // three feathers a wing, swept back
            const sf::Vector2f root = p + sd * (static_cast<float>(s) * r * 0.25f);
            const sf::Vector2f tip = root + sd * (static_cast<float>(s) * r * (0.45f - 0.08f * static_cast<float>(f))) -
                                     d * (r * (0.1f + 0.15f * static_cast<float>(f)));
            draw::line(t, root, tip, 1.5f, white(0.8f * a));
        }
}

void markOdin(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // one eye, and a rune under it
    draw::disc(t, p + sf::Vector2f{-r * 0.18f, -r * 0.12f}, r * 0.11f, white(0.9f * a), white(0.f), {1.f, 1.f}, 8);
    draw::line(t, p + sf::Vector2f{r * 0.06f, -r * 0.24f}, p + sf::Vector2f{r * 0.3f, 0.f}, 1.5f, white(0.7f * a));   // the patch
    draw::line(t, p + sf::Vector2f{0.f, r * 0.05f}, p + sf::Vector2f{0.f, r * 0.38f}, 1.3f, white(0.7f * a));          // a rune: ᚠ
    draw::line(t, p + sf::Vector2f{0.f, r * 0.12f}, p + sf::Vector2f{r * 0.16f, r * 0.04f}, 1.3f, white(0.7f * a));
    draw::line(t, p + sf::Vector2f{0.f, r * 0.22f}, p + sf::Vector2f{r * 0.16f, r * 0.14f}, 1.3f, white(0.7f * a));
}

void markThanatos(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // a little scythe
    const sf::Vector2f top = p + sf::Vector2f{-r * 0.15f, -r * 0.35f};
    draw::line(t, top, p + sf::Vector2f{r * 0.15f, r * 0.4f}, 1.5f, white(0.8f * a));
    draw::ring(t, top + sf::Vector2f{r * 0.22f, 0.f}, r * 0.22f, 1.5f, white(0.8f * a), kPi, kPi * 1.9f, 10);
}

void markLoki(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // two little horns
    for (float s : {-1.f, 1.f})
        draw::ring(t, p + sf::Vector2f{s * r * 0.32f, r * 0.1f}, r * 0.3f, 1.5f, white(0.85f * a), s < 0.f ? kPi : -kPi * 0.6f,
                   s < 0.f ? kPi * 1.6f : 0.f, 8);
}

void markNergal(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // a little skull
    draw::disc(t, p - sf::Vector2f{0.f, r * 0.08f}, r * 0.3f, white(0.85f * a), white(0.6f * a), {1.f, 1.f}, 12);
    for (float s : {-1.f, 1.f})
        draw::disc(t, p + sf::Vector2f{s * r * 0.12f, -r * 0.1f}, r * 0.08f, sf::Color(20, 20, 20, static_cast<sf::Uint8>(230 * a)),
                   sf::Color(20, 20, 20, 0), {1.f, 1.f}, 6);
    draw::line(t, p + sf::Vector2f{-r * 0.12f, r * 0.22f}, p + sf::Vector2f{r * 0.12f, r * 0.22f}, 1.5f, white(0.8f * a));
}

void markApolo(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // a little sun
    draw::disc(t, p, r * 0.18f, white(0.9f * a), white(0.6f * a), {1.f, 1.f}, 10);
    for (int i = 0; i < 8; ++i) {
        const float an = static_cast<float>(i) * kPi / 4.f;
        draw::line(t, p + sf::Vector2f{std::cos(an), std::sin(an)} * (r * 0.26f), p + sf::Vector2f{std::cos(an), std::sin(an)} * (r * 0.4f), 1.3f, white(0.8f * a));
    }
}

void markAres(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // two crossed swords
    for (int i = 0; i < 2; ++i) {
        const float an = kPi * (i ? 0.25f : 0.75f);
        const sf::Vector2f d{std::cos(an), std::sin(an)}, sd{-d.y, d.x};
        draw::line(t, p - d * (r * 0.38f), p + d * (r * 0.38f), 1.6f, white(0.9f * a));
        draw::line(t, p - d * (r * 0.22f) - sd * (r * 0.1f), p - d * (r * 0.22f) + sd * (r * 0.1f), 1.4f, white(0.8f * a));
    }
}

// A crescent moon: a disc with a darker one bitten out of it (shared by Artemisa's mark and her prey).
void crescent(sf::RenderTarget& t, sf::Vector2f p, float R, sf::Color c, sf::Color bite, float a) {
    draw::disc(t, p, R, withAlpha(c, a), withAlpha(c, 0.8f * a), {1.f, 1.f}, 16);
    draw::disc(t, p + sf::Vector2f{R * 0.45f, -R * 0.2f}, R * 0.85f, withAlpha(bite, a), withAlpha(bite, a), {1.f, 1.f}, 16);
}

void markArtemisa(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // a crescent moon
    draw::glow(t, p, r * 0.45f, sf::Color(220, 230, 255), 0.25f * a);
    const sf::Color moon(235, 240, 255);
    draw::disc(t, p, r * 0.3f, white(0.9f * a), white(0.75f * a), {1.f, 1.f}, 14);
    (void)moon;
    draw::disc(t, p + sf::Vector2f{r * 0.14f, -r * 0.07f}, r * 0.25f, sf::Color(60, 140, 190, static_cast<sf::Uint8>(255 * a)),
               sf::Color(60, 140, 190, static_cast<sf::Uint8>(255 * a)), {1.f, 1.f}, 14);
}

void markTiamat(sf::RenderTarget& t, sf::Vector2f p, float r, float, float a) {   // a dragon's eye
    draw::disc(t, p, r * 0.3f, white(0.85f * a), white(0.5f * a), {1.6f, 0.8f}, 14);
    draw::disc(t, p, r * 0.1f, sf::Color(20, 20, 20, static_cast<sf::Uint8>(255 * a)), sf::Color(20, 20, 20, 0), {0.5f, 1.4f}, 8);
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
void drawSlotSymbol(sf::RenderTarget& t, sf::Vector2f c, float h, int sym, float a);   // (below: 7, clover, coin)

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
        case F::Slash: {   // "Death sentence": a blade swipes across it, bright at the edge, then the cut opens and flecks fly
            const sf::Vector2f d{0.707f, -0.707f}, n{0.707f, 0.707f};
            const float sw = easeOut(clampf(u / 0.25f, 0.f, 1.f));
            const sf::Vector2f p0 = f.a - d * f.r, p1 = p0 + d * (2.f * f.r * sw);
            if (u < 0.35f) {   // the swipe itself
                const float s = 1.f - u / 0.35f;
                draw::line(t, p0, p1, (4.f * s + 1.f) * k, withAlpha(sf::Color::White, 0.95f * s));
                draw::glow(t, p1, 12.f * k, f.color, 0.6f * s);
            }
            if (u >= 0.2f) {   // the cut opens: its two lips drift apart
                const float v = easeOut(clampf((u - 0.2f) / 0.8f, 0.f, 1.f));
                const float o = 5.f * k * v;
                for (float sd : {-1.f, 1.f})
                    draw::line(t, f.a - d * (f.r * 0.85f) + n * (sd * o), f.a + d * (f.r * 0.85f) + n * (sd * o), 1.5f * k,
                               withAlpha(sd > 0.f ? hot : f.color, 0.85f * fl));
                for (int i = 0; i < 5; ++i) {   // flecks thrown off the cut
                    const float h = hash01(f.a.x * 0.13f + static_cast<float>(i) * 1.7f);
                    const sf::Vector2f q = f.a + d * (f.r * (h - 0.5f) * 1.4f) + n * ((i % 2 ? 1.f : -1.f) * f.r * 0.6f * v);
                    draw::disc(t, q, 1.6f * k, withAlpha(hot, 0.9f * fl), withAlpha(f.color, 0.3f * fl), {1.f, 1.f}, 6);
                }
            }
            break;
        }
        case F::Clover: {   // "Lucky break": four heart-shaped leaves unfold round it, turning a little; a glint at the heart
            const float e = easeOut(clampf(u / 0.45f, 0.f, 1.f));
            const sf::Color leaf = lerpColor(sf::Color(110, 220, 120), f.color, 0.2f);
            const sf::Color leafHi = lerpColor(leaf, sf::Color::White, 0.35f);
            for (int i = 0; i < 4; ++i) {
                const float an = static_cast<float>(i) * 1.571f + 0.785f + u * 0.8f;
                const sf::Vector2f dir{std::cos(an), std::sin(an)}, perp{-dir.y, dir.x};
                const sf::Vector2f base = f.a + dir * (f.r * 0.3f);
                const float L = f.r * 0.6f * e;
                if (L < 1.f) continue;
                const sf::Vector2f lobe = base + dir * (L * 0.72f);
                draw::disc(t, lobe + perp * (L * 0.27f), L * 0.33f, withAlpha(leafHi, 0.85f * fl), withAlpha(leaf, 0.75f * fl), {1.f, 1.f}, 12);
                draw::disc(t, lobe - perp * (L * 0.27f), L * 0.33f, withAlpha(leafHi, 0.85f * fl), withAlpha(leaf, 0.75f * fl), {1.f, 1.f}, 12);
                fillShape(t, {lobe + perp * (L * 0.58f), lobe - perp * (L * 0.58f), base}, withAlpha(leaf, 0.75f * fl));
            }
            if (u < 0.35f) {   // the glint
                const float g = 1.f - u / 0.35f;
                for (int i = 0; i < 4; ++i) {
                    const float an = static_cast<float>(i) * 1.571f;
                    draw::line(t, f.a, f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * 0.45f * g), 1.5f * k,
                               withAlpha(sf::Color::White, 0.9f * g));
                }
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
        case F::Reels: {   // slot-machine pair: three reels pop up over the ball and spin; two land on the symbol, the third rattles past
            const int sym = std::clamp(static_cast<int>(f.b.x), 0, 2);
            const float a = clampf(fl / 0.25f, 0.f, 1.f);
            for (int i = 0; i < 3; ++i) {
                const float pop = easeOut(clampf((u - 0.06f * static_cast<float>(i)) / 0.15f, 0.f, 1.f));
                if (pop <= 0.f) continue;
                const sf::Vector2f p = f.a + sf::Vector2f{(static_cast<float>(i) - 1.f) * 19.f * k, -f.r - 22.f * k};
                const float w = 16.f * k * pop, h = 20.f * k * pop;
                draw::box(t, {p.x - w * 0.5f, p.y - h * 0.5f, w, h}, 3.f * k, withAlpha(theme::bgDeep, 0.85f * a),
                          withAlpha(theme::bgDeep, 0.85f * a), withAlpha(hot, 0.8f * a), 1.f);
                const float stopAt = 0.28f + 0.14f * static_cast<float>(i);
                if (u < stopAt) {   // still spinning: symbols scroll down through the window
                    const float roll = u * 28.f + static_cast<float>(i) * 1.3f;
                    const float fr = roll - std::floor(roll);
                    drawSlotSymbol(t, p + sf::Vector2f{0.f, (fr - 0.5f) * h * 0.7f}, h * 0.8f, static_cast<int>(roll) % 3, 0.5f * a * pop);
                } else {           // stopped, with a little bounce
                    const float s = clampf((u - stopAt) / 0.12f, 0.f, 1.f);
                    const float bounce = std::sin(s * kPi) * 3.f * k * (1.f - s);
                    const int shown = i < 2 ? sym : (sym + 1) % 3;
                    drawSlotSymbol(t, p + sf::Vector2f{0.f, bounce}, h * 0.8f, shown, a * (i < 2 ? 1.f : 0.45f));
                    if (i == 1 && s < 1.f)   // the match: the first two flash together
                        draw::glow(t, (p + f.a + sf::Vector2f{-19.f * k, -f.r - 22.f * k}) * 0.5f, 22.f * k, f.color, 0.5f * (1.f - s));
                }
            }
            break;
        }
        case F::Boomerang: {   // "Homecoming": a boomerang spins out on a loop and comes back to the ball, a faint trail behind
            auto at = [&](float v) {
                const float th = v * 2.f * kPi;
                const float rad = f.r * std::sin(v * kPi);
                return f.a + sf::Vector2f{std::cos(th - kPi * 0.5f), std::sin(th - kPi * 0.5f)} * rad;
            };
            constexpr int kTrail = 10;
            for (int j = 0; j < kTrail; ++j) {
                const float v0 = u - 0.3f * static_cast<float>(j + 1) / kTrail, v1 = u - 0.3f * static_cast<float>(j) / kTrail;
                if (v1 <= 0.f) break;
                const float w = 1.f - static_cast<float>(j) / kTrail;
                draw::line(t, at(std::max(0.f, v0)), at(v1), (1.f + 2.f * w) * k, withAlpha(f.color, 0.5f * w * fl));
            }
            const sf::Vector2f head = at(u);
            const float spin = u * 28.f;
            for (float o : {0.f, 1.9f}) {   // the two arms of the V
                const sf::Vector2f arm{std::cos(spin + o), std::sin(spin + o)};
                draw::line(t, head, head + arm * (8.f * k), 2.6f * k, withAlpha(hot, 0.95f * fl));
            }
            if (u > 0.85f) draw::ring(t, f.a, f.r * 0.25f, 1.5f * k, withAlpha(hot, 0.8f * (1.f - (u - 0.85f) / 0.15f)));   // caught
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
        case F::Dust: {   // "Heavy landing": a flat shock along the floor, puffs rolling out both ways, grit thrown up
            const float e = easeOut(u);
            draw::disc(t, f.a + sf::Vector2f{0.f, f.r * 0.1f}, f.r * e, withAlpha(f.color, 0.f), withAlpha(hot, 0.45f * fl), {1.f, 0.35f}, 32);
            for (float sd : {-1.f, 1.f})
                for (int i = 0; i < 4; ++i) {
                    const float fi = static_cast<float>(i);
                    const sf::Vector2f p = f.a + sf::Vector2f{sd * f.r * e * (0.35f + 0.22f * fi), -5.f * k * u * fi};
                    draw::disc(t, p, (6.f - fi) * k * (1.f + 1.2f * u), withAlpha(f.color, 0.3f * fl), withAlpha(f.color, 0.f), {1.f, 1.f}, 12);
                }
            for (int i = 0; i < 6; ++i) {   // grit: up and back down
                const float h = hash01(static_cast<float>(i) * 3.3f + f.a.x * 0.1f);
                const float vx = (h - 0.5f) * f.r * 1.4f;
                const sf::Vector2f p = f.a + sf::Vector2f{vx * u, -f.r * 0.9f * (u - u * u) * (0.6f + 0.6f * h)};
                draw::polygon(t, p, 1.8f * k, 4, u * 8.f + h * 6.f, withAlpha(hot, 0.85f * fl), withAlpha(f.color, 0.85f * fl));
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
        case F::Echo: {   // "Spell chorus": three rings one after another like a chord, each carrying little rune ticks round
            for (int i = 0; i < 3; ++i) {
                const float v = clampf((u - 0.15f * static_cast<float>(i)) / 0.55f, 0.f, 1.f);
                if (v <= 0.f || v >= 1.f) continue;
                const float R = f.r * (0.45f + 0.85f * v);
                const float a = 0.75f * (1.f - v);
                draw::ring(t, f.a, R, (2.f - 0.4f * static_cast<float>(i)) * k, withAlpha(i == 0 ? hot : f.color, a));
                const float turn = static_cast<float>(i) * 0.6f + v * (i % 2 ? -1.6f : 1.6f);
                for (int j = 0; j < 6; ++j) {
                    const float an = turn + static_cast<float>(j) * 1.047f;
                    const sf::Vector2f sd{std::cos(an), std::sin(an)};
                    draw::line(t, f.a + sd * (R - 3.f * k), f.a + sd * (R + 3.f * k), 1.4f * k, withAlpha(sf::Color::White, a));
                }
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
        case F::Link: {   // "Fission": a bead of the element hops to a neighbour on a low arc, a tail behind it, and bursts there
            const sf::Vector2f d = f.b - f.a;
            const sf::Vector2f up = normalized(sf::Vector2f{-d.y, d.x}, {0.f, -1.f}) * (length(d) * 0.22f);
            auto at = [&](float v) { const sf::Vector2f m = (f.a + f.b) * 0.5f + up;
                                     return f.a * ((1.f - v) * (1.f - v)) + m * (2.f * v * (1.f - v)) + f.b * (v * v); };
            const float travel = easeOut(clampf(u / 0.55f, 0.f, 1.f));
            if (u < 0.6f)
                for (int j = 0; j < 8; ++j) {
                    const float v = travel - 0.04f * static_cast<float>(j);
                    if (v < 0.f) break;
                    const float w = 1.f - static_cast<float>(j) / 8.f;
                    draw::disc(t, at(v), (1.f + 2.5f * w) * k, withAlpha(j == 0 ? sf::Color::White : hot, 0.9f * w),
                               withAlpha(f.color, 0.4f * w), {1.f, 1.f}, 8);
                }
            if (u >= 0.55f) {   // the burst on the neighbour
                const float v = (u - 0.55f) / 0.45f;
                draw::ring(t, f.b, (4.f + 14.f * easeOut(v)) * k, 1.5f * k, withAlpha(hot, 0.8f * (1.f - v)));
                for (int i = 0; i < 5; ++i) {
                    const float an = static_cast<float>(i) * 1.2566f + f.b.x * 0.1f;
                    const sf::Vector2f sd{std::cos(an), std::sin(an)};
                    const float r0 = (5.f + 12.f * easeOut(v)) * k;
                    draw::line(t, f.b + sd * r0, f.b + sd * (r0 + 4.f * k), 1.5f * k, withAlpha(f.color, 0.9f * (1.f - v)));
                }
            }
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
        case F::Pellets: {   // "Frag rounds": a pinch of pellets spraying out, each with a short hot streak
            for (int i = 0; i < 7; ++i) {
                const float an = static_cast<float>(i) * 0.8976f + hash01(f.a.x * 0.3f) * 6.f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                const float reach = f.r * (0.7f + 0.3f * hash01(static_cast<float>(i) + f.a.y));
                const float r1 = reach * easeOut(u), r0 = std::max(0.f, r1 - 9.f * k * fl);
                draw::line(t, f.a + sd * r0, f.a + sd * r1, 1.4f * k, withAlpha(f.color, 0.6f * fl));
                draw::disc(t, f.a + sd * r1, 1.8f * k, withAlpha(sf::Color::White, 0.95f * fl), withAlpha(hot, 0.5f * fl), {1.f, 1.f}, 6);
            }
            if (u < 0.25f) draw::glow(t, f.a, f.r * 0.4f, hot, 0.6f * (1.f - u / 0.25f));   // the pop
            break;
        }
        case F::Smoke: {   // "Smoke bomb": a pop, then soft puffs rolling out and round, thinning as they swell
            if (u < 0.15f) draw::ring(t, f.a, f.r * 0.5f * (u / 0.15f), 2.f * k, withAlpha(hot, 0.7f * (1.f - u / 0.15f)));
            const sf::Color dark = lerpColor(f.color, theme::bgDeep, 0.55f);
            for (int i = 0; i < 7; ++i) {
                const float h = hash01(f.a.y * 0.2f + static_cast<float>(i) * 2.1f);
                const float an = static_cast<float>(i) * 0.8976f + h * 2.f + u * 0.9f;   // they roll round a little
                const sf::Vector2f p = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * (0.3f + 0.3f * h) * easeOut(u)) -
                                       sf::Vector2f{0.f, 8.f * k * u};
                const float pr = f.r * (0.22f + 0.22f * u) * (0.8f + 0.4f * h);
                draw::disc(t, p, pr, withAlpha(dark, 0.32f * fl), withAlpha(f.color, 0.f), {1.f, 1.f}, 20);
                draw::disc(t, p - sf::Vector2f{pr * 0.25f, pr * 0.25f}, pr * 0.45f, withAlpha(f.color, 0.18f * fl), withAlpha(f.color, 0.f), {1.f, 1.f}, 14);
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
        case F::Blast: {   // "Bomber": a white-hot fireball, tongues of flame licking out, a ring of fire and a curl of smoke
            draw::disc(t, f.a, f.r * 0.6f * (1.f - u), withAlpha(sf::Color::White, 0.85f * fl), withAlpha(f.color, 0.5f * fl), {1.f, 1.f}, 20);
            draw::ring(t, f.a, f.r * easeOut(u), (5.f * fl + 1.f) * k, withAlpha(f.color, 0.7f * fl));
            for (int i = 0; i < 8; ++i) {   // tongues: thick at the root, thin at the tip
                const float an = static_cast<float>(i) * 0.785f + hash01(f.a.x * 0.07f) * 3.f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                const float L = f.r * (0.5f + 0.35f * hash01(static_cast<float>(i) + f.a.y)) * easeOut(clampf(u / 0.4f, 0.f, 1.f));
                const float a = clampf(1.f - u / 0.6f, 0.f, 1.f);
                draw::line(t, f.a + sd * (L * 0.3f), f.a + sd * (L * 0.7f), 4.f * k * a, withAlpha(lerpColor(f.color, sf::Color::White, 0.4f), 0.8f * a));
                draw::line(t, f.a + sd * (L * 0.7f), f.a + sd * L, 1.8f * k * a, withAlpha(f.color, 0.8f * a));
            }
            if (u > 0.35f) {   // smoke rising off it
                const float v = (u - 0.35f) / 0.65f;
                for (int i = 0; i < 3; ++i)
                    draw::disc(t, f.a + sf::Vector2f{(static_cast<float>(i) - 1.f) * f.r * 0.3f, -f.r * 0.5f * v}, f.r * (0.2f + 0.2f * v),
                               withAlpha(sf::Color(70, 70, 80), 0.35f * (1.f - v)), withAlpha(sf::Color(70, 70, 80), 0.f), {1.f, 1.f}, 14);
            }
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
        case F::Cards: {   // "Sleight": three little cards fanned out over the mark, then flicked up and away
            const float fan = easeOut(clampf(u / 0.35f, 0.f, 1.f));
            const float lift = u > 0.55f ? easeOut((u - 0.55f) / 0.45f) : 0.f;
            const float a = clampf(fl / 0.3f, 0.f, 1.f);
            for (int i = 0; i < 3; ++i) {
                const float rot = (static_cast<float>(i) - 1.f) * 0.45f * fan;
                const sf::Vector2f up{std::sin(rot), -std::cos(rot)}, side{-up.y, up.x};
                const sf::Vector2f c = f.a + up * (f.r * 0.55f + 14.f * k * lift);
                const float hw = 5.f * k, hh = 7.5f * k;
                const sf::Vector2f p0 = c - side * hw - up * hh, p1 = c + side * hw - up * hh, p2 = c + side * hw + up * hh,
                                   p3 = c - side * hw + up * hh;
                fillShape(t, {p0, p1, p2, p3}, withAlpha(sf::Color(250, 248, 240), 0.95f * a));
                const sf::Color edge = withAlpha(i == 1 ? hot : f.color, a);
                draw::line(t, p0, p1, 1.f, edge);
                draw::line(t, p1, p2, 1.f, edge);
                draw::line(t, p2, p3, 1.f, edge);
                draw::line(t, p3, p0, 1.f, edge);
                draw::disc(t, c, 1.8f * k, withAlpha(i == 1 ? sf::Color(214, 40, 52) : sf::Color(30, 30, 36), a),
                           withAlpha(i == 1 ? sf::Color(214, 40, 52) : sf::Color(30, 30, 36), a), {1.f, 1.f}, 6);
            }
            break;
        }
        case F::Arrow: {   // "Ambush": chevrons drive right through the target along one speed line
            const sf::Vector2f d = f.b, side{-f.b.y, f.b.x};
            draw::line(t, f.a - d * (f.r * 1.2f), f.a + d * (f.r * 1.2f * easeOut(u)), 1.f * k, withAlpha(hot, 0.35f * fl));
            for (int i = 0; i < 4; ++i) {
                const float v = u * 1.3f - 0.15f * static_cast<float>(i);
                if (v < 0.f || v > 1.f) continue;
                const sf::Vector2f c = f.a + d * (f.r * (2.2f * v - 1.1f));
                const float a = fl * (1.f - 0.2f * static_cast<float>(i)) * std::sin(v * kPi);
                for (float sd : {-1.f, 1.f})
                    draw::line(t, c, c - d * (7.f * k) + side * (sd * 6.f * k), 2.f * k, withAlpha(i == 0 ? sf::Color::White : hot, 0.9f * a));
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
        case F::Mana: {   // "Mana spring": motes stream into the ball on short trails, and it brims with a soft ring
            for (int i = 0; i < 8; ++i) {
                const float an = static_cast<float>(i) * 0.785f + u * 2.f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                const float r = f.r * (1.f - easeOut(clampf(u / 0.75f, 0.f, 1.f)));
                if (r < 2.f) continue;
                draw::line(t, f.a + sd * r, f.a + sd * (r + 7.f * k), 1.2f * k, withAlpha(f.color, 0.5f * fl));
                draw::disc(t, f.a + sd * r, 2.2f * k, withAlpha(hot, 0.9f * fl), withAlpha(f.color, 0.2f * fl), {1.f, 1.f}, 8);
            }
            if (u > 0.7f) {
                const float v = (u - 0.7f) / 0.3f;
                draw::ring(t, f.a, f.r * (0.25f + 0.2f * v), 2.f * k, withAlpha(hot, 0.8f * (1.f - v)));
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
        case F::Saturate: {   // "Saturate": a drop of each element spirals in from either side, they meet, and both rings soak outward
            const float in = easeOut(clampf(u / 0.45f, 0.f, 1.f));
            if (u < 0.5f)
                for (int s = 0; s < 2; ++s)
                    for (int j = 0; j < 4; ++j) {
                        const float v = std::max(0.f, in - 0.06f * static_cast<float>(j));
                        const float an = static_cast<float>(s) * kPi + v * 3.f;
                        const sf::Vector2f p = f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * (1.f - v));
                        const float w = 1.f - 0.22f * static_cast<float>(j);
                        draw::disc(t, p, (3.2f * w) * k, withAlpha(s == 0 ? f.color : f.color2, 0.9f * w), withAlpha(s == 0 ? f.color : f.color2, 0.2f),
                                   {1.f, 1.f}, 8);
                    }
            if (u >= 0.4f) {
                const float v = easeOut((u - 0.4f) / 0.6f);
                draw::ring(t, f.a, f.r * (0.2f + 0.7f * v), 2.f * k, withAlpha(f.color, 0.75f * fl));
                draw::ring(t, f.a, f.r * (0.1f + 0.55f * v), 2.f * k, withAlpha(f.color2, 0.75f * fl));
            }
            break;
        }
        case F::Wild: {   // "Wild magic": sparks of every colour and shape spinning off, each on a curling tail
            static const sf::Color kC[5] = {theme::elemFire, theme::elemWater, theme::elemPoison, theme::classMage, theme::puGolden};
            for (int i = 0; i < 10; ++i) {
                const float base = static_cast<float>(i) * 0.628f;
                auto at = [&](float v) {
                    const float an = base + v * 4.f;
                    return f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * easeOut(v));
                };
                const sf::Color c = kC[i % 5];
                for (int j = 0; j < 4; ++j) {
                    const float v1 = u - 0.05f * static_cast<float>(j), v0 = v1 - 0.05f;
                    if (v0 <= 0.f) break;
                    draw::line(t, at(v0), at(v1), (2.f - 0.4f * static_cast<float>(j)) * k, withAlpha(c, 0.6f * fl * (1.f - 0.25f * static_cast<float>(j))));
                }
                const sf::Vector2f p = at(u);
                if (i % 3 == 0) draw::polygon(t, p, 3.f * k, 4, u * 9.f, withAlpha(sf::Color::White, 0.9f * fl), withAlpha(c, 0.9f * fl));
                else if (i % 3 == 1) draw::polygon(t, p, 3.2f * k, 3, -u * 9.f, withAlpha(sf::Color::White, 0.9f * fl), withAlpha(c, 0.9f * fl));
                else draw::disc(t, p, 2.4f * k, withAlpha(sf::Color::White, 0.9f * fl), withAlpha(c, 0.9f * fl), {1.f, 1.f}, 8);
            }
            break;
        }
        case F::Chrono: {   // "Chronomancy": a slow, wide, dashed ripple and a thinner one turning against it, ticks along the rim
            const float r = f.r * easeOut(u);
            for (int i = 0; i < 24; ++i) {
                const float a0 = static_cast<float>(i) * 0.2618f + u * 0.4f;
                draw::ring(t, f.a, r, 1.5f * k, withAlpha(hot, 0.4f * fl), a0, a0 + 0.13f, 4);
            }
            const float r2 = r * 0.82f;
            for (int i = 0; i < 12; ++i) {
                const float a0 = static_cast<float>(i) * 0.5236f - u * 0.8f;
                draw::ring(t, f.a, r2, 1.f * k, withAlpha(f.color, 0.35f * fl), a0, a0 + 0.2f, 4);
                const sf::Vector2f sd{std::cos(a0), std::sin(a0)};
                draw::line(t, f.a + sd * (r - 5.f * k), f.a + sd * r, 1.2f * k, withAlpha(hot, 0.45f * fl));
            }
            break;
        }
        case F::Scythe: {   // "Reaper": a crescent blade swept round the ball - a thick bright edge, a thin back, a fading wake
            const float an = std::atan2(f.b.y, f.b.x);
            const float sweep = -1.5f + 3.f * easeOut(u);
            const float a1 = an + sweep, a0 = a1 - 1.2f;
            for (int j = 1; j <= 3; ++j) {   // the wake
                const float back = 0.35f * static_cast<float>(j);
                draw::ring(t, f.a, f.r, 1.f * k, withAlpha(f.color, 0.35f * fl / static_cast<float>(j)), a0 - back, a1 - back, 12);
            }
            draw::ring(t, f.a, f.r, (3.5f * fl + 1.f) * k, withAlpha(hot, 0.9f * fl), a0, a1, 16);
            draw::ring(t, f.a, f.r * 0.82f, 1.2f * k, withAlpha(f.color, 0.7f * fl), a0 + 0.25f, a1, 12);
            const sf::Vector2f tip = f.a + sf::Vector2f{std::cos(a1), std::sin(a1)} * f.r;
            draw::glow(t, tip, 8.f * k, sf::Color::White, 0.6f * fl);
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
        case F::Flip: {   // "Coin flip": a coin tossed up, spinning, landing gold (heads) or grey (tails)
            const bool heads = f.b.x > 0.5f;
            const sf::Vector2f q = f.a - sf::Vector2f{0.f, 22.f * k * std::sin(clampf(u / 0.7f, 0.f, 1.f) * kPi)};
            const float spin = u < 0.7f ? std::fabs(std::cos(u * 30.f)) : 1.f;
            const sf::Color cc = u < 0.7f ? theme::puGolden : heads ? theme::puGolden : sf::Color(150, 150, 160);
            draw::disc(t, q, f.r, withAlpha(lerpColor(cc, sf::Color::White, 0.35f), fl), withAlpha(cc, fl), {0.15f + 0.85f * spin, 1.f}, 12);
            if (u >= 0.7f && heads) draw::glow(t, q, f.r * 2.5f, theme::puGolden, 0.4f * fl);
            break;
        }
        case F::Execute: {   // "Cull": two strokes close into an X over it, a white flash, then a ring snaps shut
            const float c = easeOut(clampf(u / 0.3f, 0.f, 1.f));
            for (int i = 0; i < 2; ++i) {
                const float an = 0.785f + static_cast<float>(i) * 1.571f;
                const sf::Vector2f d{std::cos(an), std::sin(an)};
                const float L = f.r * (1.6f - 0.6f * c);
                const sf::Vector2f off = sf::Vector2f{-d.y, d.x} * (f.r * 1.2f * (1.f - c) * (i == 0 ? 1.f : -1.f));   // each slides in from its own side
                draw::line(t, f.a - d * L * 0.5f + off, f.a + d * L * 0.5f + off, (3.f * fl + 0.8f) * k, withAlpha(i == 0 ? sf::Color::White : hot, 0.9f * fl));
            }
            if (u >= 0.3f && u < 0.5f) draw::glow(t, f.a, f.r * 1.4f, sf::Color::White, 0.6f * (1.f - (u - 0.3f) / 0.2f));
            if (u >= 0.3f) {
                const float v = easeOut((u - 0.3f) / 0.7f);
                draw::ring(t, f.a, f.r * (1.3f - 0.8f * v), 1.5f * k, withAlpha(f.color, 0.8f * fl));
            }
            break;
        }
        case F::Glint: {   // "Backstab": a four-point glint where the blade went in, and a short stab line along the ball's path
            const float g = clampf(1.f - u / 0.6f, 0.f, 1.f);
            const float s = f.r * (0.4f + 0.8f * easeOut(clampf(u / 0.2f, 0.f, 1.f))) * g;
            for (int i = 0; i < 4; ++i) {
                const float an = static_cast<float>(i) * 1.571f + 0.3f;
                const sf::Vector2f sd{std::cos(an), std::sin(an)};
                draw::line(t, f.a, f.a + sd * s * (i % 2 ? 0.6f : 1.f), (2.f * g + 0.6f) * k, withAlpha(sf::Color::White, 0.95f * g));
            }
            draw::glow(t, f.a, f.r * 0.8f, f.color, 0.5f * fl);
            const sf::Vector2f d = f.b;
            draw::line(t, f.a - d * (f.r * 1.4f), f.a - d * (f.r * (1.4f - 1.2f * easeOut(u))), 1.5f * k, withAlpha(hot, 0.7f * fl));
            break;
        }
        case F::Twin: {   // "Twincast": a ghost of the ball peels off to the side, casts its own small ring, and fades
            const sf::Vector2f side{-f.b.y, f.b.x};
            const float e = easeOut(clampf(u / 0.4f, 0.f, 1.f));
            const sf::Vector2f g = f.a + side * (f.r * 2.2f * e);
            draw::disc(t, g, f.r, withAlpha(f.color, 0.35f * fl), withAlpha(f.color, 0.08f * fl), {1.f, 1.f}, 20);
            draw::ring(t, g, f.r, 1.5f * k, withAlpha(hot, 0.7f * fl));
            draw::line(t, f.a, g, 1.f * k, withAlpha(hot, 0.3f * fl));
            if (u > 0.35f) {
                const float v = (u - 0.35f) / 0.65f;
                draw::ring(t, g, f.r * (1.2f + 1.8f * easeOut(v)), 1.5f * k, withAlpha(hot, 0.7f * (1.f - v)));
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

}  // namespace (drawDragonForm is shared: render/ClassRender.hpp)

// The Elder dragon's look, in other shapes (2026-10-06): Tiamat's dragons are
// each their own kind of dragon. form: 0 the classic (two wings), 1 a wyvern
// (its wings are its arms, two legs, a barbed tail), 2 four wings (two pairs
// beating out of step), 3 a serpent dragon (no wings: a long body winding
// along `trail`, whiskers, little legs). S = body length, phase drives the
// beat / the sway.
void drawDragonForm(sf::RenderTarget& t, int form, sf::Vector2f pos, sf::Vector2f face, sf::Vector2f headDir, float S, float phase,
                    sf::Color c, float glowA, const sf::Vector2f* trail, int trailN, float k, int heads,
                    const sf::Vector2f* headDirs) {
    const sf::Vector2f f = face, side{-f.y, f.x};
    const sf::Color body = lerpColor(c, sf::Color::Black, 0.25f);
    const sf::Color edge = lerpColor(c, sf::Color::White, 0.35f);
    const float flap = std::sin(phase) * 0.5f + 0.5f;
    if (glowA > 0.f) draw::glow(t, pos, S * 1.8f, c, glowA);
    sf::Vector2f hf = headDir, hside{-hf.y, hf.x};   // (a hydra turns them per head)
    auto head = [&](sf::Vector2f neck, float len, bool whiskers) {
        const sf::Vector2f h = neck + hf * (S * len);
        draw::line(t, neck, h, S * 0.13f, withAlpha(body, 1.f));
        fillShape(t, {h + hf * (S * 0.24f), h + hside * (S * 0.11f), h - hf * (S * 0.06f), h - hside * (S * 0.11f)}, withAlpha(edge, 1.f));
        for (float sd : {-1.f, 1.f}) {
            draw::disc(t, h + hf * (S * 0.06f) + hside * (sd * S * 0.05f), std::max(1.f, S * 0.035f), sf::Color::White, sf::Color::White,
                       {1.f, 1.f}, 6);
            if (whiskers)   // a serpent dragon's long whiskers, trailing back and waving
                draw::line(t, h + hf * (S * 0.18f) + hside * (sd * S * 0.06f),
                           h - hf * (S * 0.25f) + hside * (sd * S * (0.3f + 0.08f * std::sin(phase * 1.5f))), std::max(1.f, S * 0.02f),
                           withAlpha(edge, 0.8f));
            else   // horns swept back
                draw::line(t, h + hside * (sd * S * 0.08f), h - hf * (S * 0.18f) + hside * (sd * S * 0.14f), std::max(1.f, S * 0.03f),
                           withAlpha(edge, 0.9f));
        }
    };
    auto wing = [&](sf::Vector2f root, float sd, float span, float fl, float alpha) {
        // A bat's wing: an arm out to the tip, finger bones fanning back, the
        // membrane between them scalloped along its trailing edge.
        const sf::Vector2f elbow = root + side * (sd * S * span * (0.55f + 0.25f * fl)) + f * (S * 0.15f);
        const sf::Vector2f tip = root + side * (sd * S * span * (0.95f + 0.35f * fl)) - f * (S * (0.35f - 0.25f * fl));
        const sf::Vector2f back = root - f * (S * 0.4f) + side * (sd * S * 0.05f);
        const sf::Vector2f f1 = elbow + (tip - elbow) * 0.35f - f * (S * 0.38f) + side * (sd * S * 0.05f);   // finger tips
        const sf::Vector2f f2 = elbow - f * (S * 0.45f) - side * (sd * S * span * 0.12f);
        const sf::Vector2f mid = root + (elbow - root) * 0.5f;
        auto scallop = [&](sf::Vector2f a, sf::Vector2f b) { return (a + b) * 0.5f + (elbow - (a + b) * 0.5f) * 0.28f; };
        fillShape(t, {elbow, tip, scallop(tip, f1), f1}, withAlpha(body, alpha));
        fillShape(t, {elbow, f1, scallop(f1, f2), f2}, withAlpha(body, alpha * 0.92f));
        fillShape(t, {root, elbow, f2, scallop(f2, back), back}, withAlpha(body, alpha * 0.85f));
        draw::line(t, root, elbow, std::max(1.f, S * 0.05f), withAlpha(edge, 0.9f));
        draw::line(t, elbow, tip, std::max(1.f, S * 0.04f), withAlpha(edge, 0.9f));
        draw::line(t, elbow, f1, std::max(1.f, S * 0.025f), withAlpha(edge, 0.7f));
        draw::line(t, elbow, f2, std::max(1.f, S * 0.025f), withAlpha(edge, 0.7f));
        (void)mid;
        return elbow;
    };
    if (form == 3) {   // a serpent dragon: a long winding body along its path, no wings
        const int n = trailN >= 2 ? trailN : 10;
        for (int i = 1; i < n; ++i) {
            const float v = static_cast<float>(i) / static_cast<float>(n - 1);
            const bool real = trail && i < trailN;   // (a short trail: the rest straight behind)
            const sf::Vector2f a0 = real ? trail[i - 1] : pos - f * (S * 0.2f * static_cast<float>(i - 1));
            const sf::Vector2f a1 = real ? trail[i] : pos - f * (S * 0.2f * static_cast<float>(i));
            const sf::Vector2f sway = side * (std::sin(phase + v * 6.f) * S * 0.06f);
            draw::line(t, a0 + sway, a1 + sway, S * 0.3f * (1.f - 0.7f * v), withAlpha(body, 0.95f));
            draw::line(t, a0 + sway, a1 + sway, S * 0.12f * (1.f - 0.7f * v), withAlpha(edge, 0.5f));   // a pale belly line
            if (i % 3 == 0)   // a ridge of spines, and little legs now and then
                draw::line(t, a1 + sway, a1 + sway + side * (S * 0.12f) - f * (S * 0.05f), std::max(1.f, S * 0.025f), withAlpha(edge, 0.8f));
        }
        head(pos, 0.12f, true);
        return;
    }
    // tail: tapering segments that sway (a wyvern's is longer, with a barb)
    const float tailLen = form == 1 ? 1.3f : 1.f;
    sf::Vector2f prev = pos - f * (S * 0.45f);
    for (int i = 1; i <= 6; ++i) {
        const float v = static_cast<float>(i) / 6.f;
        const sf::Vector2f q = pos - f * (S * (0.45f + tailLen * v)) + side * (std::sin(phase * 0.6f + v * 3.5f) * S * 0.18f * v);
        draw::line(t, prev, q, S * 0.16f * (1.f - 0.8f * v), withAlpha(body, 0.95f));
        prev = q;
    }
    if (form == 1)   // the barb
        fillShape(t, {prev - f * (S * 0.22f), prev + side * (S * 0.12f), prev - side * (S * 0.12f)}, withAlpha(edge, 0.95f));
    else
        fillShape(t, {prev, prev - f * (S * 0.15f) + side * (S * 0.1f), prev - f * (S * 0.15f) - side * (S * 0.1f)}, withAlpha(edge, 0.9f));
    if (form == 2) {   // four wings: a smaller pair behind, beating out of step
        const float fl2 = std::sin(phase + 1.6f) * 0.5f + 0.5f;
        for (float sd : {-1.f, 1.f}) wing(pos - f * (S * 0.3f) + side * (sd * S * 0.07f), sd, 0.7f, fl2, 0.65f);
    }
    for (float sd : {-1.f, 1.f}) {
        const sf::Vector2f root = pos + f * (S * 0.05f) + side * (sd * S * 0.08f);
        const sf::Vector2f elbow = wing(root, sd, 1.f, flap, 0.78f);
        if (form == 1)   // a wyvern's wing is its arm: a claw at the elbow
            fillShape(t, {elbow, elbow + f * (S * 0.1f) + side * (sd * S * 0.04f), elbow + f * (S * 0.04f) - side * (sd * S * 0.05f)},
                      withAlpha(edge, 0.95f));
    }
    if (form == 1)   // ...and it stands on two legs, tucked under
        for (float sd : {-1.f, 1.f}) {
            const sf::Vector2f hip = pos - f * (S * 0.3f) + side * (sd * S * 0.1f);
            const sf::Vector2f foot = hip - f * (S * 0.2f) + side * (sd * S * 0.18f);
            draw::line(t, hip, foot, std::max(1.f, S * 0.06f), withAlpha(body, 1.f));
            for (int c2 = -1; c2 <= 1; ++c2)
                draw::line(t, foot, foot - f * (S * 0.07f) + side * (static_cast<float>(c2) * S * 0.04f), std::max(1.f, S * 0.025f),
                           withAlpha(edge, 0.9f));
        }
    fillShape(t, {pos + f * (S * 0.3f), pos + side * (S * 0.14f), pos - f * (S * 0.5f), pos - side * (S * 0.14f)}, withAlpha(body, 1.f));
    if (heads <= 1) {
        head(pos + f * (S * 0.25f), 0.3f, false);
    } else {   // several heads on long necks, each bending its own way (a hydra's)
        const sf::Vector2f keep = hf, base = pos + f * (S * 0.22f);
        for (int i = 0; i < heads; ++i) {
            sf::Vector2f nd;
            if (headDirs) {
                nd = headDirs[i];
            } else {
                const float an = (static_cast<float>(i) - static_cast<float>(heads - 1) * 0.5f) * 0.6f + std::sin(phase * 0.5f + static_cast<float>(i)) * 0.08f;
                nd = {keep.x * std::cos(an) - keep.y * std::sin(an), keep.x * std::sin(an) + keep.y * std::cos(an)};
            }
            const sf::Vector2f bend = base + normalized(f + nd, nd) * (S * 0.4f);   // out of the body first, then toward where it looks
            draw::line(t, base, bend, S * 0.13f, withAlpha(body, 1.f));
            draw::disc(t, bend, S * 0.065f, withAlpha(body, 1.f), withAlpha(body, 1.f), {1.f, 1.f}, 10);
            hf = nd;
            hside = sf::Vector2f{-nd.y, nd.x};
            head(bend, 0.38f, false);
        }
        hf = keep;
    }
}

namespace {

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
        if (c.rouletteEvery > 0.f) {   // "Roulette": a little wheel by the ball; while it pays, a tinted ring
            static const sf::Color kCol[3] = {sf::Color(220, 50, 56), sf::Color(36, 36, 44), sf::Color(70, 200, 100)};
            const sf::Vector2f wc = b.pos + sf::Vector2f{-b.radius - 14.f * k, -b.radius - 10.f * k};
            const float R = 10.f * k;
            const bool spinning = s.rouletteSpin >= 0.f;
            const float held = c.rouletteEvery - s.rouletteT;   // s since the last spin started
            if (spinning || held < cfg::common::rouletteSpin + 0.8f) {
                // the wheel: 7 slices, red / black alternating, one green; it slows to a stop on the result
                const float q = spinning ? s.rouletteSpin / cfg::common::rouletteSpin : 1.f;
                const float landAng = s.rouletteLand == 2 ? 0.f : (s.rouletteLand == 0 ? 1.f : 2.f) * 2.f * kPi / 7.f;
                const float ang = -kPi * 0.5f - landAng - (1.f - q) * (1.f - q) * 14.f;   // eases out onto the slice under the pointer
                for (int i = 0; i < 7; ++i) {
                    const float a0 = ang + static_cast<float>(i) * 2.f * kPi / 7.f;
                    const sf::Color sc = i == 0 ? kCol[2] : kCol[(i + 1) % 2];
                    fillShape(t, {wc, wc + sf::Vector2f{std::cos(a0), std::sin(a0)} * R,
                                  wc + sf::Vector2f{std::cos(a0 + 0.9f), std::sin(a0 + 0.9f)} * R}, sc);
                }
                draw::ring(t, wc, R, 1.2f * k, withAlpha(theme::puGolden, 0.9f), 0.f, 6.283f, 16);
                draw::disc(t, wc, 1.6f * k, theme::puGolden, theme::puGolden, {1.f, 1.f}, 6);
                fillShape(t, {wc - sf::Vector2f{0.f, R - 1.f * k}, wc - sf::Vector2f{2.5f * k, R + 3.f * k}, wc - sf::Vector2f{-2.5f * k, R + 3.f * k}},
                          sf::Color::White);   // the pointer
            }
            if (s.rouletteWin >= 0 && !spinning)
                draw::ring(t, b.pos, b.radius + 2.5f * k, 1.5f * k, withAlpha(s.rouletteWin == 1 ? sf::Color(200, 200, 210) : kCol[s.rouletteWin], 0.55f));
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

void drawGodLook(sf::RenderTarget& t, RoleMask roles, sf::Vector2f p, float r, float heading, float a);   // (below)

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
    if (look.second != BallRole::Normal) {   // third class = a tilted ring orbiting it in that colour (like Atlas's)
        BallRole third = BallRole::Normal;
        for (int i = 0; i < kClassCount && third == BallRole::Normal; ++i) {
            const BallRole c = classAt(i);
            if ((look.roles & roleBit(c)) != 0 && c != look.lead && c != look.second) third = c;
        }
        if (third != BallRole::Normal) {   // a small orbit of dots, tilted, turning - and a little moon on it
            const sf::Color tc = lerpColor(roleColor(third), sf::Color::White, 0.15f);
            const float spin = classClock() * 0.9f, tilt = -0.6f;
            auto at = [&](float an) {
                const sf::Vector2f e{std::cos(an) * r * 1.55f, std::sin(an) * r * 0.45f};
                return pos + sf::Vector2f{e.x * std::cos(tilt) - e.y * std::sin(tilt), e.x * std::sin(tilt) + e.y * std::cos(tilt)};
            };
            for (int i = 0; i < 24; ++i) {
                const float an = static_cast<float>(i) / 24.f * 2.f * kPi + spin;
                draw::disc(t, at(an), i % 4 == 0 ? 1.9f : 1.4f, withAlpha(tc, alpha), withAlpha(tc, 0.3f * alpha), {1.f, 1.f}, 6);
            }
            const sf::Vector2f moon = at(spin * 1.6f);
            draw::glow(t, moon, std::max(5.f, r * 0.4f), tc, 0.35f * alpha);
            draw::disc(t, moon, std::max(2.6f, r * 0.18f), withAlpha(lerpColor(tc, sf::Color::White, 0.3f), alpha), withAlpha(tc, alpha), {1.f, 1.f}, 12);
        }
    }
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
    drawGodLook(t, look.roles, pos, r, heading, alpha);
}

// A god's ball must look like one (2026-10-06): a divine halo over it in the
// god's colour, and each god its own big mark.
void drawGodLook(sf::RenderTarget& t, RoleMask roles, sf::Vector2f p, float r, float heading, float a) {
    const BallRole gods[] = {BallRole::Astronomer, BallRole::Hermes, BallRole::Odin, BallRole::Thanatos, BallRole::Tiamat, BallRole::Loki, BallRole::Nergal, BallRole::Apolo, BallRole::Ares, BallRole::Artemisa};
    const float clk = classClock();
    for (BallRole g : gods) {
        if ((roles & roleBit(g)) == 0) continue;
        const sf::Color c = roleColor(g);
        draw::glow(t, p, r * 2.4f, c, 0.22f * a);
        {   // the halo: a flattened ring floating over it, with rays
            const sf::Vector2f h = p + sf::Vector2f{0.f, -r * 1.35f};
            for (int i = 0; i < 28; ++i) {
                const float an = static_cast<float>(i) / 28.f * 2.f * kPi + clk * 0.8f;
                draw::disc(t, h + sf::Vector2f{std::cos(an) * r * 0.7f, std::sin(an) * r * 0.2f}, 1.4f,
                           withAlpha(lerpColor(c, sf::Color::White, 0.4f), 0.9f * a), withAlpha(c, 0.f), {1.f, 1.f}, 4);
            }
            draw::glow(t, h, r * 0.6f, c, 0.3f * a);
        }
        const sf::Vector2f d{std::cos(heading), std::sin(heading)}, sd{-d.y, d.x};
        switch (g) {
            case BallRole::Astronomer:   // an armillary: two tilted rings turning round it, stars on them
                for (int ring = 0; ring < 2; ++ring)
                    for (int i = 0; i < 30; ++i) {
                        const float an = static_cast<float>(i) / 30.f * 2.f * kPi + clk * (ring ? -0.6f : 0.9f);
                        const sf::Vector2f e{std::cos(an) * r * 1.55f, std::sin(an) * r * 0.45f};
                        const float tilt = ring ? 0.7f : -0.7f;
                        const sf::Vector2f q{e.x * std::cos(tilt) - e.y * std::sin(tilt), e.x * std::sin(tilt) + e.y * std::cos(tilt)};
                        draw::disc(t, p + q, i % 6 == 0 ? 2.f : 1.f, withAlpha(c, 0.85f * a), withAlpha(c, 0.f), {1.f, 1.f}, 4);
                    }
                break;
            case BallRole::Hermes:   // big golden wings, beating
                for (int s = -1; s <= 1; s += 2)
                    for (int f = 0; f < 5; ++f) {
                        const float flap = std::sin(clk * 10.f) * 0.25f;
                        const sf::Vector2f root = p + sd * (static_cast<float>(s) * r * 0.6f) - d * (r * 0.1f);
                        const float len = r * (1.5f - 0.18f * static_cast<float>(f));
                        const float sweep = 0.35f + 0.22f * static_cast<float>(f) + flap;
                        const sf::Vector2f dir = sd * (static_cast<float>(s) * std::cos(sweep)) - d * std::sin(sweep);
                        draw::line(t, root, root + dir * len, 2.5f - 0.3f * static_cast<float>(f), withAlpha(c, 0.9f * a));
                    }
                break;
            case BallRole::Odin: {   // a ring of runes round it, turning
                draw::ring(t, p, r * 1.6f, 1.2f, withAlpha(c, 0.6f * a));
                for (int i = 0; i < 8; ++i) {
                    const float an = clk * 0.5f + static_cast<float>(i) * kPi / 4.f;
                    const sf::Vector2f q = p + sf::Vector2f{std::cos(an), std::sin(an)} * (r * 1.6f);
                    draw::line(t, q - sf::Vector2f{0.f, r * 0.18f}, q + sf::Vector2f{0.f, r * 0.18f}, 1.5f, withAlpha(c, 0.9f * a));
                    draw::line(t, q - sf::Vector2f{0.f, r * 0.08f}, q + sf::Vector2f{r * 0.12f, -r * 0.16f}, 1.5f, withAlpha(c, 0.9f * a));
                }
                break;
            }
            case BallRole::Thanatos: {   // a hood of shadow over it, and the scythe behind
                draw::ring(t, p, r * 1.15f, r * 0.35f, sf::Color(20, 10, 30, static_cast<sf::Uint8>(200 * a)), kPi * 1.05f, kPi * 1.95f, 20);
                const sf::Vector2f haft0 = p - d * (r * 1.2f) + sd * (r * 0.9f), haft1 = p - d * (r * 1.2f) - sd * (r * 1.3f);
                draw::line(t, haft0, haft1, 2.f, withAlpha(sf::Color(120, 100, 90), 0.9f * a));
                draw::ring(t, haft0 + d * (r * 0.6f), r * 0.65f, 2.5f, withAlpha(c, 0.9f * a), std::atan2(-d.y, -d.x) - 0.2f,
                           std::atan2(-d.y, -d.x) + 1.6f, 14);
                break;
            }
            case BallRole::Tiamat: break;   // (her mount is drawn under the ball: worldGods2)
            case BallRole::Apolo:   // a crown of sun rays, turning
                for (int i = 0; i < 12; ++i) {
                    const float an = static_cast<float>(i) * kPi / 6.f + clk * 0.4f;
                    const float len = r * (i % 2 ? 0.45f : 0.7f);
                    draw::line(t, p + sf::Vector2f{std::cos(an), std::sin(an)} * (r * 1.15f), p + sf::Vector2f{std::cos(an), std::sin(an)} * (r * 1.15f + len),
                               2.f, withAlpha(sf::Color(255, 220, 120), 0.85f * a));
                }
                break;
            case BallRole::Artemisa: {   // a silver bow slung behind it, and a little crescent moon over it
                const sf::Color silver(220, 228, 245);
                const float tilt = -0.6f;
                const sf::Vector2f bd{std::cos(tilt), std::sin(tilt)}, bs{-bd.y, bd.x};
                sf::Vector2f prev;
                for (int i = 0; i <= 16; ++i) {   // the bow: an arc
                    const float u = static_cast<float>(i) / 16.f * 2.f - 1.f;
                    const sf::Vector2f q = p - bs * (r * 0.55f) + bd * (u * r * 1.5f) - bs * (r * 0.5f * (1.f - u * u));
                    if (i > 0) draw::line(t, prev, q, 2.f, withAlpha(silver, 0.9f * a));
                    prev = q;
                }
                draw::line(t, p - bs * (r * 0.55f) - bd * (r * 1.5f), p - bs * (r * 0.55f) + bd * (r * 1.5f), 0.8f, withAlpha(silver, 0.6f * a));   // its string
                const sf::Vector2f mp = p + sf::Vector2f{0.f, -r * 1.45f + std::sin(clk * 2.f) * r * 0.06f};
                draw::glow(t, mp, r * 0.5f, silver, 0.35f * a);
                crescent(t, mp, r * 0.28f, silver, sf::Color(14, 18, 34), a);
                break;
            }
            case BallRole::Ares: {   // a Spartan's crest: a red plume arching over it, front to back
                const sf::Color plume(220, 50, 40);
                for (int i = 0; i < 14; ++i) {
                    const float u = static_cast<float>(i) / 13.f;
                    const float an = kPi * (1.1f + 0.8f * u);   // over the top
                    const sf::Vector2f q = p + sf::Vector2f{std::cos(an) * r * 1.05f, std::sin(an) * r * 1.25f};
                    const float sway = std::sin(clk * 6.f + u * 4.f) * r * 0.05f;
                    draw::line(t, q, q + sf::Vector2f{sway, -r * (0.25f + 0.2f * std::sin(u * kPi))}, 2.2f, withAlpha(plume, 0.9f * a));
                }
                break;
            }
            case BallRole::Nergal:   // a sickly shroud, and flies buzzing round it
                draw::glow(t, p, r * 2.f, sf::Color(70, 110, 40), 0.3f * a);
                for (int i = 0; i < 5; ++i) {
                    const float an = clk * (3.f + static_cast<float>(i) * 0.7f) + static_cast<float>(i) * 1.3f;
                    const sf::Vector2f q = p + sf::Vector2f{std::cos(an) * r * (1.3f + 0.2f * std::sin(clk * 5.f + static_cast<float>(i))),
                                                            std::sin(an * 1.3f) * r * 1.1f};
                    draw::disc(t, q, 1.6f, sf::Color(30, 30, 25, static_cast<sf::Uint8>(230 * a)), sf::Color(30, 30, 25, 0), {1.f, 1.f}, 5);
                }
                break;
            case BallRole::Loki:   // a horned helm: two long golden horns curling up off it
                for (float s : {-1.f, 1.f}) {
                    sf::Vector2f q = p + sf::Vector2f{s * r * 0.55f, -r * 0.6f};
                    for (int i = 0; i < 6; ++i) {
                        const float v = static_cast<float>(i) / 5.f;
                        const sf::Vector2f nq = q + sf::Vector2f{s * r * (0.32f - 0.3f * v), -r * (0.18f + 0.12f * v)};
                        draw::line(t, q, nq, std::max(1.f, r * 0.22f * (1.f - 0.75f * v)), withAlpha(sf::Color(235, 200, 90), 0.95f * a));
                        q = nq;
                    }
                }
                break;
            default: break;
        }
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
        case BallRole::Astronomer: markAstronomer(t, pos, r, heading, alpha); break;
        case BallRole::Hermes: markHermes(t, pos, r, heading, alpha); break;
        case BallRole::Odin: markOdin(t, pos, r, heading, alpha); break;
        case BallRole::Thanatos: markThanatos(t, pos, r, heading, alpha); break;
        case BallRole::Tiamat: markTiamat(t, pos, r, heading, alpha); break;
        case BallRole::Loki: markLoki(t, pos, r, heading, alpha); break;
        case BallRole::Nergal: markNergal(t, pos, r, heading, alpha); break;
        case BallRole::Apolo: markApolo(t, pos, r, heading, alpha); break;
        case BallRole::Ares: markAres(t, pos, r, heading, alpha); break;
        case BallRole::Artemisa: markArtemisa(t, pos, r, heading, alpha); break;
    }
}

// ==================================================================== Astronomer
// Its planets (2026-10-06): each kind its own look, a faint orbit, the belt's
// rocks, Saturn's flung rings and Jupiter's gas.
void worldPlanets(sf::RenderTarget& t, const World& world) {
    const AstronomerWorld& aw = world.classWorld().astronomer;
    if (aw.planets.empty() && aw.rocks.empty()) return;
    const float k = world.arenaScale();
    const float clk = classClock();
    for (const auto& f : aw.puffs) {   // Jupiter's gas
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f);
        draw::glow(t, f.pos, f.r * 1.3f, sf::Color(214, 170, 120), 0.22f * a);
    }
    for (const auto& h : aw.halos) {   // Saturn's rings round the core: two tilted bands of gold dust, turning
        const sf::Vector2f c = world.core().pos;
        const float a = clampf(h.life / 0.5f, 0.f, 1.f) * clampf((h.maxLife - h.life) / 0.4f, 0.f, 1.f);
        for (int band = 0; band < 2; ++band) {
            const float R = h.r * (1.f + 0.07f * static_cast<float>(band));
            for (int i = 0; i < 90; ++i) {
                const float an = h.spin * (band ? -0.7f : 1.f) + static_cast<float>(i) / 90.f * 2.f * kPi;
                draw::disc(t, c + sf::Vector2f{std::cos(an), std::sin(an)} * R, (1.2f + 0.8f * hash01(static_cast<float>(i + band * 90))) * k,
                           sf::Color(240, 220, 160, static_cast<sf::Uint8>(200 * a)), sf::Color(240, 220, 160, 0), {1.f, 1.f}, 4);
            }
        }
        draw::ring(t, c, h.r, 6.f * k, sf::Color(232, 206, 140, static_cast<sf::Uint8>(40 * a)));
    }
    for (const auto& tr : aw.tritons) {   // Triton: a pale icy moon trailing frost
        const float a = clampf(tr.life / 0.5f, 0.f, 1.f);
        for (int i = 1; i < tr.trailN; ++i)
            draw::line(t, tr.trail[i - 1], tr.trail[i], tr.r * (1.4f - 0.12f * static_cast<float>(i)),
                       sf::Color(180, 220, 255, static_cast<sf::Uint8>(120 * a * (1.f - static_cast<float>(i) / 10.f))));
        draw::glow(t, tr.pos, tr.r * 3.f, sf::Color(150, 200, 255), 0.5f * a);
        draw::disc(t, tr.pos, tr.r, sf::Color(225, 240, 255, static_cast<sf::Uint8>(255 * a)), sf::Color(140, 180, 230, static_cast<sf::Uint8>(255 * a)),
                   {1.f, 1.f}, 14);
    }
    for (const auto& f : aw.flares) {   // Mercury's burning trail: embers
        const float a = clampf(f.life / cfg::planet::flareLife, 0.f, 1.f);
        draw::glow(t, f.pos, f.r * (0.8f + 0.6f * (1.f - a)), sf::Color(255, 170, 70), 0.3f * a);
    }
    for (const auto& st : aw.storms) {
        const float a = clampf(st.life / 0.5f, 0.f, 1.f) * clampf((st.maxLife - st.life) / 0.3f, 0.f, 1.f);
        if (st.spot) {   // the Great Red Spot: a flattened red spiral, turning
            draw::glow(t, st.pos, st.r, sf::Color(200, 80, 60), 0.22f * a);
            for (int arm = 0; arm < 3; ++arm)
                for (int i = 0; i < 26; ++i) {
                    const float u = static_cast<float>(i) / 25.f;
                    const float an = st.spin + static_cast<float>(arm) * 2.094f + u * 4.f;
                    const sf::Vector2f q{std::cos(an) * st.r * u, std::sin(an) * st.r * u * 0.6f};
                    draw::disc(t, st.pos + q, (1.5f + 3.f * (1.f - u)) * k,
                               sf::Color(230, 120, 90, static_cast<sf::Uint8>(200 * a * (1.f - 0.6f * u))),
                               sf::Color(230, 120, 90, 0), {1.f, 1.f}, 6);
                }
        } else {   // Venus's acid storm: sulphur-green wisps swirling round the planet
            draw::glow(t, st.pos, st.r, sf::Color(200, 220, 90), 0.18f * a);
            for (int i = 0; i < 10; ++i) {
                const float an = st.spin * (i % 2 ? 1.f : -0.7f) + static_cast<float>(i) * 0.63f;
                const float rr = st.r * (0.45f + 0.5f * hash01(static_cast<float>(i) * 3.7f));
                const sf::Vector2f q{std::cos(an) * rr, std::sin(an) * rr};
                draw::line(t, st.pos + q, st.pos + q + sf::Vector2f{-q.y, q.x} * 0.25f, 3.f * k,
                           sf::Color(215, 230, 110, static_cast<sf::Uint8>(150 * a)));
            }
        }
    }
    for (const auto& q : aw.projs) {
        using Proj = AstronomerWorld::Proj;
        switch (q.kind) {
            case Proj::Rocket: {   // a little rocket with a flame
                const sf::Vector2f d = normalized(q.vel, {1.f, 0.f}), sd{-d.y, d.x};
                draw::line(t, q.pos - d * (14.f * k), q.pos - d * (5.f * k), 4.f * k, sf::Color(255, 160, 60, 200));   // flame
                draw::glow(t, q.pos - d * (8.f * k), 7.f * k, sf::Color(255, 180, 80), 0.5f);
                draw::line(t, q.pos - d * (5.f * k), q.pos + d * (5.f * k), 4.f * k, sf::Color(230, 235, 245));        // body
                draw::line(t, q.pos - d * (5.f * k) + sd * (3.f * k), q.pos - d * (5.f * k) - sd * (3.f * k), 1.5f * k,
                           sf::Color(220, 80, 70));   // fins
                break;
            }
            case Proj::Moonlet:   // Phobos / Deimos: lumpy red-grey rocks
                draw::polygon(t, q.pos, 4.5f * k, 6, q.t * 6.f, sf::Color(170, 130, 115), sf::Color(110, 80, 70));
                draw::glow(t, q.pos, 9.f * k, sf::Color(255, 120, 70), 0.25f);
                break;
            case Proj::Meteor: {   // a rock with a fiery tail
                const sf::Vector2f d = normalized(q.to - q.from, {0.f, 1.f});
                draw::line(t, q.pos - d * (26.f * k), q.pos, 5.f * k, sf::Color(255, 140, 60, 140));
                draw::polygon(t, q.pos, 6.f * k, 5, q.t * 9.f, sf::Color(160, 140, 115), sf::Color(100, 85, 70));
                break;
            }
            case Proj::Star: {   // a falling star: a long bright streak and a twinkle
                const sf::Vector2f d = normalized(q.to - q.from, {0.f, 1.f});
                draw::line(t, q.pos - d * (60.f * k), q.pos, 2.f * k, sf::Color(255, 240, 190, 120));
                draw::glow(t, q.pos, 12.f * k, sf::Color(255, 236, 170), 0.8f);
                draw::polygon(t, q.pos, 5.f * k, 4, q.t * 8.f, sf::Color::White, sf::Color(255, 236, 170));
                break;
            }
        }
    }
    for (const auto& bm : aw.booms) {   // where they landed - each its own blast
        const float u = 1.f - clampf(bm.life / bm.maxLife, 0.f, 1.f);
        const float a = 1.f - u;
        if (bm.kind == 0) {   // rocket: an orange fireball and its ring
            draw::glow(t, bm.pos, bm.r * (0.6f + 0.6f * u), sf::Color(255, 150, 60), 0.6f * a);
            draw::ring(t, bm.pos, bm.r * (0.4f + 0.8f * u), 2.f * k, sf::Color(255, 210, 140, static_cast<sf::Uint8>(200 * a)));
        } else if (bm.kind == 1) {   // meteor: a crater of dust and sparks
            draw::ring(t, bm.pos, bm.r * (0.5f + 0.7f * u), 4.f * k, sf::Color(170, 140, 110, static_cast<sf::Uint8>(160 * a)));
            draw::glow(t, bm.pos, bm.r * 0.7f, sf::Color(255, 140, 60), 0.4f * a);
        } else {   // star: rays of light
            for (int i = 0; i < 8; ++i) {
                const float an = static_cast<float>(i) * kPi / 4.f;
                const sf::Vector2f d{std::cos(an), std::sin(an)};
                draw::line(t, bm.pos + d * (bm.r * 0.2f), bm.pos + d * (bm.r * (0.6f + 0.8f * u)), 2.f * k,
                           sf::Color(255, 240, 190, static_cast<sf::Uint8>(220 * a)));
            }
            draw::glow(t, bm.pos, bm.r, sf::Color(255, 236, 170), 0.5f * a);
        }
    }
    // the orbits, barely there
    std::vector<int> seen;
    for (const auto& pl : aw.planets) {
        const int ki = static_cast<int>(pl.kind);
        if (std::find(seen.begin(), seen.end(), ki) != seen.end()) continue;
        seen.push_back(ki);
        for (int i = 0; i < 64; ++i) {
            const sf::Vector2f q = world.planetOrbit(cfg::planet::orbit[ki], static_cast<float>(i) / 64.f * 2.f * kPi);
            draw::disc(t, q, 0.9f * k, white(0.07f), white(0.f), {1.f, 1.f}, 4);
        }
    }
    for (const auto& r : aw.rocks) {   // the belt
        if (r.regrow > 0.f) continue;
        const float rr = 6.f * k * r.size;
        draw::polygon(t, r.pos, rr, 5 + static_cast<int>(r.size * 3.f) % 3, r.ang * 3.f + r.size * 7.f,
                      sf::Color(150, 138, 120), sf::Color(95, 86, 74));
    }
    for (const auto& pl : aw.planets) {
        const float r = pl.r;
        const sf::Vector2f p = pl.pos;
        const float a = pl.bonus ? 0.75f : 1.f;
        auto body = [&](sf::Color c, sf::Color edge) {
            draw::glow(t, p, r * 2.f, c, 0.18f * a);
            draw::disc(t, p, r, withAlpha(c, a), withAlpha(edge, a), {1.f, 1.f}, 24);
            draw::disc(t, p + sf::Vector2f{-r * 0.3f, -r * 0.3f}, r * 0.45f, withAlpha(sf::Color::White, 0.18f * a),
                       withAlpha(sf::Color::White, 0.f), {1.f, 1.f}, 12);   // a highlight
        };
        switch (pl.kind) {
            case PlanetKind::Mercury:
                body(sf::Color(182, 176, 168), sf::Color(110, 104, 98));
                if (pl.boostT > 0.f) draw::line(t, pl.prev - (p - pl.prev) * 3.f, p, r * 1.2f, sf::Color(220, 220, 230, 90));
                break;
            case PlanetKind::Moon:
                body(sf::Color(214, 214, 220), sf::Color(140, 140, 150));
                draw::disc(t, p + sf::Vector2f{r * 0.3f, r * 0.2f}, r * 0.3f, sf::Color(160, 160, 170, 150), sf::Color(160, 160, 170, 0), {1.f, 1.f}, 8);
                break;
            case PlanetKind::Venus:
                draw::glow(t, p, r * 2.6f, sf::Color(240, 210, 120), 0.3f * a);   // its haze
                body(sf::Color(240, 214, 150), sf::Color(190, 150, 80));
                break;
            case PlanetKind::Earth:
                body(sf::Color(76, 140, 230), sf::Color(30, 70, 150));
                for (int i = 0; i < 3; ++i) {   // continents drifting by
                    const float an = clk * 0.6f + static_cast<float>(i) * 2.1f;
                    const sf::Vector2f q{std::cos(an) * r * 0.45f, std::sin(an * 1.3f) * r * 0.35f};
                    draw::disc(t, p + q, r * 0.32f, sf::Color(96, 176, 92, static_cast<sf::Uint8>(220 * a)),
                               sf::Color(70, 140, 70, 0), {1.f, 1.f}, 10);
                }
                draw::ring(t, p, r * 1.08f, 1.2f * k, sf::Color(170, 210, 255, static_cast<sf::Uint8>(110 * a)));   // the air
                break;
            case PlanetKind::Mars: {
                body(sf::Color(214, 96, 62), sf::Color(130, 46, 30));
                draw::disc(t, p + sf::Vector2f{r * 0.25f, r * 0.15f}, r * 0.35f, sf::Color(150, 60, 40, 160), sf::Color(150, 60, 40, 0), {1.f, 1.f}, 10);
                for (int i = pl.moonsOut; i < 2; ++i) {   // Phobos and Deimos, at home
                    const float an = clk * (2.4f - 0.8f * static_cast<float>(i)) + static_cast<float>(i) * kPi;
                    draw::polygon(t, p + sf::Vector2f{std::cos(an), std::sin(an) * 0.6f} * (r * (1.7f + 0.4f * static_cast<float>(i))),
                                  3.f * k, 6, an, sf::Color(170, 130, 115), sf::Color(110, 80, 70));
                }
                break;
            }
            case PlanetKind::Jupiter:
                body(sf::Color(222, 190, 150), sf::Color(150, 110, 80));
                for (int i = -2; i <= 2; ++i) {   // its bands
                    const float y = static_cast<float>(i) * r * 0.32f;
                    const float hw = std::sqrt(std::max(0.f, r * r - y * y)) * 0.92f;
                    draw::line(t, p + sf::Vector2f{-hw, y}, p + sf::Vector2f{hw, y}, r * 0.12f,
                               i % 2 == 0 ? sf::Color(180, 130, 90, 140) : sf::Color(240, 220, 190, 110));
                }
                draw::disc(t, p + sf::Vector2f{r * 0.35f, r * 0.3f}, r * 0.2f, sf::Color(200, 90, 70, 220), sf::Color(200, 90, 70, 60),
                           {1.4f, 1.f}, 10);   // the Great Red Spot
                break;
            case PlanetKind::Saturn: {
                body(sf::Color(232, 206, 146), sf::Color(170, 140, 80));
                if (pl.rings) {   // tilted rings: flattened dotted ellipses, turning
                    for (int ring = 0; ring < 2; ++ring) {
                        const float R = r * (1.55f + 0.3f * static_cast<float>(ring));
                        for (int i = 0; i < 40; ++i) {
                            const float an = static_cast<float>(i) / 40.f * 2.f * kPi + clk * 0.8f;
                            const sf::Vector2f e{std::cos(an) * R, std::sin(an) * R * 0.32f};
                            const sf::Vector2f q{e.x * 0.94f - e.y * 0.34f, e.x * 0.34f + e.y * 0.94f};
                            if (e.y < 0.f && length(q) < r) continue;   // behind the planet
                            draw::disc(t, p + q, 1.3f * k, sf::Color(240, 222, 170, static_cast<sf::Uint8>(200 * a)),
                                       sf::Color(240, 222, 170, 0), {1.f, 1.f}, 4);
                        }
                    }
                }
                break;
            }
            case PlanetKind::Neptune:
                draw::glow(t, p, r * 2.4f, sf::Color(90, 140, 255), 0.25f * a);
                body(sf::Color(80, 122, 236), sf::Color(36, 60, 160));
                break;
        }
        if (pl.hitT > 0.2f) draw::ring(t, p, r * 1.5f, 1.5f * k, sf::Color(255, 255, 255, 90));   // just hit something
    }
}

// ==================================================================== Hermes / Odin
void worldGods(sf::RenderTarget& t, const World& world) {
    const float k = world.arenaScale();
    const float clk = classClock();
    const HermesWorld& hw = world.classWorld().hermes;
    const OdinWorld& ow = world.classWorld().odin;
    const sf::Color gold = theme::classHermes, rune = theme::classOdin;
    for (const auto& f : hw.fx) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f);
        if (f.kind == 0) {   // Messenger: a golden streak there, and afterimages along it
            draw::line(t, f.a, f.b, 3.f * k, withAlpha(gold, 0.6f * a));
            for (int i = 1; i <= 4; ++i)
                draw::glow(t, f.a + (f.b - f.a) * (static_cast<float>(i) / 5.f), 10.f * k, gold, 0.35f * a);
        } else if (f.kind == 1) {   // Caduceus: two serpents of light winding round each other
            const sf::Vector2f d = f.b - f.a, sd = normalized(sf::Vector2f{-d.y, d.x}, {0.f, 1.f});
            for (int s = 0; s < 2; ++s) {
                sf::Vector2f prev = f.a;
                for (int i = 1; i <= 16; ++i) {
                    const float u = static_cast<float>(i) / 16.f;
                    const float w = std::sin(u * 3.f * kPi + static_cast<float>(s) * kPi) * 9.f * k * std::sin(u * kPi);
                    const sf::Vector2f q = f.a + d * u + sd * w;
                    draw::line(t, prev, q, 2.f * k, withAlpha(s ? gold : sf::Color(140, 230, 150), 0.85f * a));
                    prev = q;
                }
            }
        } else if (f.kind == 3) {   // the message delivered: a golden wave over the whole arena
            const float u = 1.f - a;
            draw::ring(t, f.a, f.r * u, 6.f * k, withAlpha(gold, 0.7f * a));
            draw::ring(t, f.a, f.r * u * 0.92f, 2.f * k, white(0.5f * a));
            draw::glow(t, f.a, f.r * 0.25f * (1.f - u), gold, 0.5f * a);
        } else {   // a sonic boom: a thin white ring and a pale shock
            const float u = 1.f - a;
            draw::ring(t, f.a, f.r * (0.3f + 0.7f * u), 2.f * k, white(0.7f * a));
            draw::glow(t, f.a, f.r * 0.6f, gold, 0.25f * a);
        }
    }
    for (const Ball& b : world.balls()) {
        if (b.hasRole(BallRole::Hermes) && b.cls.hermes.bounces > 0) {   // the message it carries: a halo that's full when it's due
            const float c = clampf(static_cast<float>(b.cls.hermes.bounces) / static_cast<float>(cfg::hermes::deliverAfter), 0.f, 1.f);
            draw::ring(t, b.pos, b.radius + (4.f + 10.f * c) * k, (1.f + 2.f * c) * k, withAlpha(gold, 0.3f + 0.5f * c));
            draw::glow(t, b.pos, b.radius * (1.5f + 2.f * c), gold, 0.15f + 0.3f * c);
        }
        if (b.cls.hermes.golden) {   // Petasos: golden, wings of light
            draw::glow(t, b.pos, b.radius * 3.f, gold, 0.5f);
            draw::ring(t, b.pos, b.radius + 3.f * k, 2.f * k, withAlpha(gold, 0.9f));
        }
        if (b.cls.hermes.fast && b.mods.cls.hermes.quicksilver > 0.f) {   // Quicksilver: ripples of silver
            for (int i = 0; i < 3; ++i) {
                const float u = std::fmod(clk * 1.2f + static_cast<float>(i) / 3.f, 1.f);
                draw::ring(t, b.pos, b.radius * (1.5f + 4.f * u), 1.5f * k, sf::Color(220, 225, 240, static_cast<sf::Uint8>(120 * (1.f - u))));
            }
        }
        const OdinMods& om = b.mods.cls.odin;
        if (b.cls.odin.circle > 0.f) {   // the all-father casts everything: a rune circle flares under it
            const float c = b.cls.odin.circle, R = b.radius * (2.2f + 1.5f * (1.f - c));
            draw::glow(t, b.pos, R * 1.3f, rune, 0.35f * c);
            draw::ring(t, b.pos, R, 2.f * k, withAlpha(rune, 0.8f * c));
            draw::ring(t, b.pos, R * 0.8f, 1.f * k, withAlpha(rune, 0.6f * c));
            for (int tri = 0; tri < 2; ++tri)
                draw::polygonOutline(t, b.pos, R * 0.76f, 3, clk * 0.8f + static_cast<float>(tri) * kPi, 1.5f * k, withAlpha(rune, 0.7f * c));
        }
        if (om.draupnir > 0.f) {   // Draupnir: a heavy gold ring turning round it, tilted...
            const sf::Color gold(255, 205, 90), deep(190, 130, 40);
            const auto ringAt = [&](sf::Vector2f c, float R, float tilt, float spin, float th, float a) {
                const sf::Vector2f ax{std::cos(spin), std::sin(spin)}, ay{-ax.y, ax.x};
                constexpr int kSeg = 20;
                for (int s = 0; s < kSeg; ++s) {
                    const float a0 = static_cast<float>(s) * 2.f * kPi / kSeg, a1 = static_cast<float>(s + 1) * 2.f * kPi / kSeg;
                    const sf::Vector2f p0 = c + ax * (std::cos(a0) * R) + ay * (std::sin(a0) * R * tilt);
                    const sf::Vector2f p1 = c + ax * (std::cos(a1) * R) + ay * (std::sin(a1) * R * tilt);
                    const bool front = std::sin(a0) > 0.f;   // the near half bright, the far half in shadow
                    draw::line(t, p0, p1, th, withAlpha(front ? gold : deep, a * (front ? 0.95f : 0.6f)));
                }
            };
            const float spin = clk * 0.6f;
            draw::glow(t, b.pos, b.radius * 2.2f, gold, 0.18f);
            ringAt(b.pos, b.radius * 1.65f, 0.38f + 0.08f * std::sin(clk * 1.3f), spin, 3.f * k, 1.f);
            const float gl = 0.5f + 0.5f * std::sin(clk * 2.f);   // a glint running round it
            draw::glow(t, b.pos + sf::Vector2f{std::cos(spin), std::sin(spin)} * (b.radius * 1.65f), 7.f * k, sf::Color(255, 245, 210), 0.5f * gl);
            if (const float c = b.cls.odin.circle; c > 0.f) {   // ...and every volley it drops eight rings like itself, flung out
                const float u = 1.f - c;
                for (int i = 0; i < 8; ++i) {
                    const float an = static_cast<float>(i) * kPi * 0.25f + spin;
                    const sf::Vector2f q = b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (b.radius * (1.6f + 4.5f * u));
                    ringAt(q, b.radius * (0.55f + 0.2f * u), 0.4f, an + clk * 3.f, 2.f * k, c);
                    draw::glow(t, q, 9.f * k, gold, 0.35f * c);
                }
            }
        }
        for (int i = 0; i < 2; ++i) {   // the ravens: Huginn (dark), Muninn (pale)
            if ((i == 0 && om.huginn <= 0.f) || (i == 1 && om.muninn <= 0.f)) continue;
            const sf::Vector2f q = b.cls.odin.raven[i];
            const float flap = std::sin(clk * 14.f + static_cast<float>(i) * 2.f) * 0.5f;
            const sf::Color body = i == 0 ? sf::Color(40, 44, 60) : sf::Color(200, 205, 220);
            const float s = 7.f * k;
            draw::line(t, q, q + sf::Vector2f{-s, -s * (0.4f + flap)}, 2.f * k, body);
            draw::line(t, q, q + sf::Vector2f{s, -s * (0.4f + flap)}, 2.f * k, body);
            draw::disc(t, q, 2.2f * k, body, body, {1.f, 1.f}, 6);
            if (b.cls.odin.ravenFlash[i] > 0.f) draw::glow(t, q, 16.f * k, rune, 0.6f * b.cls.odin.ravenFlash[i]);
        }
    }
    for (const auto& bl : ow.blasts) {   // Muninn: a bolt from the raven, an arcane burst on the pack
        const float a = clampf(bl.life / bl.maxLife, 0.f, 1.f), u = 1.f - a;
        draw::line(t, bl.from, bl.pos, 2.f * k, withAlpha(rune, 0.6f * a));
        draw::glow(t, bl.pos, bl.r * (0.6f + 0.5f * u), rune, 0.45f * a);
        draw::ring(t, bl.pos, bl.r * (0.3f + 0.7f * u), 3.f * k, withAlpha(sf::Color(200, 240, 255), 0.8f * a));
        draw::polygonOutline(t, bl.pos, bl.r * 0.6f, 6, u * 2.f, 1.5f * k, withAlpha(rune, 0.7f * a));
    }
    for (const auto& s : ow.spears) {   // Gungnir: a streak of light, a dark shaft lit with runes, a leaf blade of gold
        if (s.delay > 0.f) continue;   // (still in its hand)
        const sf::Vector2f d = normalized(s.back ? s.from - s.to : s.to - s.from, {1.f, 0.f}), sd{-d.y, d.x};
        const sf::Color gold(255, 214, 110), white(255, 248, 225), ash(70, 58, 52);
        const sf::Vector2f tip = s.pos;
        // its path behind it: a tapering streak (only as long as it has flown)
        const float flown = length(s.pos - (s.back ? s.to : s.from));
        const float streak = std::min(flown, 220.f * k);
        constexpr int kSeg = 8;
        for (int i = 0; i < kSeg; ++i) {
            const float u0 = static_cast<float>(i) / kSeg, u1 = static_cast<float>(i + 1) / kSeg;
            const float a = 1.f - u0;
            draw::line(t, tip - d * (streak * u0), tip - d * (streak * u1), (6.f * a + 1.f) * k, withAlpha(gold, 0.35f * a * a));
        }
        draw::line(t, tip - d * (streak * 0.6f), tip, 1.5f * k, withAlpha(white, 0.5f));
        // the shaft: dark ash, three runes glowing along it
        const sf::Vector2f neck = tip - d * (20.f * k), butt = tip - d * (95.f * k);
        draw::line(t, butt, neck, 3.5f * k, ash);
        draw::line(t, butt, neck, 1.2f * k, withAlpha(sf::Color(130, 110, 95), 0.8f));
        for (int i = 0; i < 3; ++i) {
            const sf::Vector2f q = neck - d * ((18.f + 20.f * static_cast<float>(i)) * k);
            draw::glow(t, q, 7.f * k, rune, 0.55f);
            draw::line(t, q - sd * (3.f * k) - d * (2.f * k), q + sd * (3.f * k) + d * (2.f * k), 1.5f * k, withAlpha(sf::Color(200, 240, 255), 0.9f));
        }
        // the crossguard and the blade: a long gold leaf with a white edge down the middle
        draw::line(t, neck - sd * (7.f * k), neck + sd * (7.f * k), 3.f * k, gold);
        const sf::Vector2f mid = neck + d * (7.f * k);
        fillShape(t, {neck, mid + sd * (5.5f * k), tip + d * (6.f * k), mid - sd * (5.5f * k)}, gold);
        draw::line(t, neck, tip + d * (4.f * k), 1.2f * k, white);
        draw::glow(t, tip, 26.f * k, gold, 0.5f);
        draw::glow(t, tip, 10.f * k, white, 0.6f);
    }
}

void worldGods2(sf::RenderTarget& t, const World& world) {
    const float k = world.arenaScale();
    const float clk = classClock();
    const sf::Color violet = theme::classThanatos, fire = theme::classTiamat;
    for (const auto& f : world.classWorld().thanatos.fx) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f);
        switch (f.kind) {
            case 0:   // a blink: a shadow streak and three fading afterimages
                draw::line(t, f.a, f.b, 4.f * k, sf::Color(40, 20, 60, static_cast<sf::Uint8>(160 * a)));
                for (int i = 1; i <= 3; ++i)
                    draw::disc(t, f.a + (f.b - f.a) * (static_cast<float>(i) / 4.f), f.r * 0.8f, withAlpha(violet, 0.3f * a),
                               withAlpha(violet, 0.f), {1.f, 1.f}, 12);
                break;
            case 1: {   // a stab: a quick bright slash on the mark
                const sf::Vector2f dir = normalized(f.b - f.a, {1.f, 0.f}), sd{-dir.y, dir.x};
                const sf::Vector2f c = f.b - dir * (f.r * 0.3f);
                draw::line(t, c - sd * (f.r * 0.9f) - dir * (f.r * 0.3f), c + sd * (f.r * 0.9f) + dir * (f.r * 0.3f), 2.5f * k,
                           sf::Color(255, 220, 255, static_cast<sf::Uint8>(230 * a)));
                break;
            }
            case 2:   // a nova / an execution: a violet ring and a dark core
                draw::ring(t, f.a, f.r * (1.f - 0.5f * a), 3.f * k, withAlpha(violet, 0.8f * a));
                draw::glow(t, f.a, f.r * 0.8f, sf::Color(60, 20, 90), 0.5f * a);
                break;
            default:   // a cut: a thin pale line along the way
                draw::line(t, f.a, f.b, 1.5f * k, sf::Color(230, 200, 255, static_cast<sf::Uint8>(200 * a)));
                break;
        }
    }
    for (const Ball& b : world.balls()) {   // Thanatos's dagger: in flight, bouncing; the marks it hit, till it blinks through them
        if (!b.hasRole(BallRole::Thanatos) || b.cls.thanatos.chain.empty()) continue;
        const ThanatosState& st = b.cls.thanatos;
        const int n = static_cast<int>(st.chainPos.size());
        const int hit = st.hop >= 0 ? n : n - 1;   // (in flight: the last one isn't hit yet)
        for (int i = std::max(0, st.hop); i < hit; ++i) {
            const sf::Vector2f q = st.chainPos[static_cast<std::size_t>(i)];
            draw::ring(t, q, 11.f * k, 2.f * k, withAlpha(violet, 0.8f));
            draw::glow(t, q, 16.f * k, violet, 0.35f);
            if (i + 1 < hit) draw::line(t, q, st.chainPos[static_cast<std::size_t>(i + 1)], 1.f * k, withAlpha(violet, 0.3f));
        }
        if (st.hop >= 0) continue;
        const sf::Vector2f dir = normalized(st.daggerPos - st.daggerFrom, {1.f, 0.f}), sd{-dir.y, dir.x};
        const sf::Vector2f tip = st.daggerPos;
        draw::line(t, st.daggerFrom + (tip - st.daggerFrom) * 0.4f, tip, 1.5f * k, withAlpha(violet, 0.35f));   // its wake
        fillShape(t, {tip, tip - dir * (9.f * k) + sd * (2.5f * k), tip - dir * (9.f * k) - sd * (2.5f * k)}, sf::Color(235, 220, 255));   // blade
        draw::line(t, tip - dir * (9.f * k) + sd * (4.f * k), tip - dir * (9.f * k) - sd * (4.f * k), 1.5f * k, violet);   // guard
        draw::line(t, tip - dir * (9.f * k), tip - dir * (14.f * k), 2.f * k, sf::Color(60, 30, 80));                       // grip
    }
    const TiamatWorld& tw = world.classWorld().tiamat;
    for (const auto& f : tw.fx) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f);
        if (f.kind == 0 || f.kind == 1) {   // a breath: a cone of billows, fire or ice
            const sf::Color c = f.kind == 0 ? sf::Color(255, 150, 60) : sf::Color(170, 220, 255);
            const sf::Vector2f dir = normalized(f.b - f.a, {1.f, 0.f}), sd{-dir.y, dir.x};
            for (int i = 0; i < 14; ++i) {
                const float u = std::fmod(static_cast<float>(i) / 14.f + (1.f - a) * 0.8f, 1.f);
                const sf::Vector2f q = f.a + dir * (f.r * u) + sd * ((hash01(static_cast<float>(i) * 3.1f) - 0.5f) * f.r * 0.6f * u);
                draw::glow(t, q, (6.f + 16.f * u) * k, c, 0.5f * a * (1.f - 0.5f * u));
            }
        } else if (f.kind == 2) {   // lightning: a jagged chain
            for (std::size_t i = 1; i < f.pts.size(); ++i) {
                const sf::Vector2f a0 = f.pts[i - 1], a1 = f.pts[i];
                const sf::Vector2f mid = (a0 + a1) * 0.5f + sf::Vector2f{-(a1 - a0).y, (a1 - a0).x} * (0.15f * (hash01(static_cast<float>(i)) - 0.5f));
                draw::line(t, a0, mid, 2.5f * k, sf::Color(170, 210, 255, static_cast<sf::Uint8>(230 * a)));
                draw::line(t, mid, a1, 2.5f * k, sf::Color(170, 210, 255, static_cast<sf::Uint8>(230 * a)));
                draw::glow(t, a1, 12.f * k, sf::Color(150, 200, 255), 0.5f * a);
            }
        } else if (f.kind == 8) {   // claws: three raking slashes on what's close
            const sf::Vector2f d = normalized(f.b - f.a, {1.f, 0.f}), sd{-d.y, d.x};
            for (int i = -1; i <= 1; ++i) {
                const sf::Vector2f c = f.b + sd * (static_cast<float>(i) * f.r * 0.35f);
                draw::line(t, c - d * (f.r * 0.7f) + sd * (f.r * 0.3f), c + d * (f.r * 0.5f) - sd * (f.r * 0.3f), 2.f * k,
                           sf::Color(255, 220, 200, static_cast<sf::Uint8>(220 * a)));
            }
        } else if (f.kind == 9) {   // the dive: a streak of fire down to the crash
            draw::line(t, f.a, f.b, 10.f * k * a, sf::Color(255, 150, 70, static_cast<sf::Uint8>(160 * a)));
            draw::ring(t, f.b, f.r * (1.f - 0.5f * a), 5.f * k, sf::Color(255, 200, 120, static_cast<sf::Uint8>(220 * a)));
            draw::glow(t, f.b, f.r, sf::Color(255, 140, 60), 0.5f * a);
        } else if (f.kind == 10) {   // the gold's right head roars: rings closing in on it
            const float u = 1.f - a;
            for (int i = 0; i < 3; ++i)
                draw::ring(t, f.b, f.r * std::max(0.05f, 1.f - u - 0.15f * static_cast<float>(i)), 2.5f * k,
                           sf::Color(255, 220, 140, static_cast<sf::Uint8>(150 * a)));
        } else if (f.kind == 11) {   // the ravager's run: a dark swath fading along its way
            draw::line(t, f.a, f.b, f.r * 1.4f * a, sf::Color(150, 70, 170, static_cast<sf::Uint8>(90 * a)));
            draw::line(t, f.a, f.b, 2.f * k, sf::Color(230, 200, 250, static_cast<sf::Uint8>(160 * a)));
        } else if (f.kind == 13) {   // a storm bolt's blast: a white flash, a blue ring rolling out
            const float u = 1.f - a;
            draw::glow(t, f.a, f.r * (0.6f + 0.4f * u), sf::Color(170, 200, 255), 0.6f * a);
            draw::ring(t, f.a, f.r * (0.3f + 0.7f * u), 3.f * k, sf::Color(210, 230, 255, static_cast<sf::Uint8>(220 * a)));
        } else if (f.kind == 4) {   // roar: rings of sound rolling out
            const float u = 1.f - a;
            for (int i = 0; i < 3; ++i)
                draw::ring(t, f.a, f.r * std::min(1.f, u + 0.15f * static_cast<float>(i)), 3.f * k,
                           sf::Color(255, 200, 150, static_cast<sf::Uint8>(150 * a)));
        } else if (f.kind == 5) {   // wing gust: curved strokes of wind sweeping out
            const float u = 1.f - a;
            for (int i = 0; i < 10; ++i) {
                const float an = static_cast<float>(i) / 10.f * 2.f * kPi + u;
                draw::ring(t, f.a, f.r * (0.3f + 0.7f * u), 2.f * k, sf::Color(220, 240, 230, static_cast<sf::Uint8>(170 * a)), an, an + 0.35f, 8);
            }
        } else if (f.kind == 6) {   // tail sweep: one hard arc spinning round
            const float u = 1.f - a;
            const float an = u * 2.f * kPi;
            draw::ring(t, f.a, f.r, 7.f * k, sf::Color(255, 170, 110, static_cast<sf::Uint8>(200 * a)), an, an + 2.4f, 24);
        } else if (f.kind == 7) {   // a fireball's burst
            const float u = 1.f - a;
            draw::glow(t, f.a, f.r * (0.5f + 0.6f * u), sf::Color(255, 140, 60), 0.6f * a);
            draw::ring(t, f.a, f.r * (0.3f + 0.7f * u), 3.f * k, sf::Color(255, 210, 140, static_cast<sf::Uint8>(220 * a)));
        } else {   // the golden dragon's crash
            const float u = 1.f - a;
            draw::ring(t, f.a, f.r * (0.3f + 0.7f * u), 5.f * k, sf::Color(255, 210, 110, static_cast<sf::Uint8>(220 * a)));
            draw::glow(t, f.a, f.r * 0.9f, sf::Color(255, 190, 90), 0.5f * a);
        }
    }
    for (const auto& f : tw.fields) {   // patches on the ground
        const float a = clampf(f.life / 0.5f, 0.f, 1.f) * clampf((f.maxLife - f.life) / 0.25f, 0.f, 1.f);
        switch (f.kind) {
            case 0:   // embers: a flickering orange smoulder
                draw::glow(t, f.pos, f.r * (1.f + 0.2f * std::sin(clk * 9.f + f.pos.x)), sf::Color(255, 120, 40), 0.35f * a);
                draw::disc(t, f.pos, 2.f * k, sf::Color(255, 210, 120, static_cast<sf::Uint8>(220 * a)), sf::Color(255, 140, 60, 0), {1.f, 1.f}, 6);
                break;
            case 1:   // frost: a pale patch, a few crystals
                draw::glow(t, f.pos, f.r, sf::Color(170, 220, 255), 0.25f * a);
                for (int i = 0; i < 3; ++i) {
                    const float an = static_cast<float>(i) * 2.1f + f.pos.x * 0.1f;
                    draw::polygon(t, f.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * 0.45f), 3.f * k, 4, an,
                                  sf::Color(230, 245, 255, static_cast<sf::Uint8>(200 * a)), sf::Color(160, 210, 255, static_cast<sf::Uint8>(150 * a)));
                }
                break;
            case 2:   // molten gold: a bright pool, bubbling
                draw::glow(t, f.pos, f.r, sf::Color(255, 190, 70), 0.45f * a);
                draw::disc(t, f.pos, f.r * 0.55f, sf::Color(255, 230, 140, static_cast<sf::Uint8>(170 * a)), sf::Color(255, 160, 50, 0), {1.3f, 1.f}, 20);
                break;
            case 4:   // a thunderstorm: a dark rolling cloud with flickers inside
                for (int i = 0; i < 7; ++i) {
                    const float an = static_cast<float>(i) * 0.9f + clk * 0.4f;
                    draw::glow(t, f.pos + sf::Vector2f{std::cos(an) * f.r * 0.55f, std::sin(an) * f.r * 0.25f - f.r * 0.2f}, f.r * 0.5f,
                               sf::Color(60, 70, 100), 0.35f * a);
                }
                if (std::sin(clk * 23.f + f.pos.x) > 0.85f) draw::glow(t, f.pos, f.r * 0.8f, sf::Color(170, 200, 255), 0.3f * a);
                break;
            default:   // the inferno: a ring of fire round its ball
                for (int i = 0; i < 24; ++i) {
                    const float an = static_cast<float>(i) / 24.f * 2.f * kPi + clk * 2.f;
                    const float fl = 0.7f + 0.3f * std::sin(clk * 12.f + static_cast<float>(i));
                    draw::glow(t, f.pos + sf::Vector2f{std::cos(an), std::sin(an)} * f.r, 14.f * k * fl, sf::Color(255, 130, 50), 0.55f * a);
                }
                draw::ring(t, f.pos, f.r, 3.f * k, sf::Color(255, 190, 90, static_cast<sf::Uint8>(200 * a)));
                break;
        }
    }
    for (const auto& f : tw.fireballs) {   // what the dragons spit
        const sf::Vector2f back = normalized(f.from - f.to, {0.f, 1.f});
        if (f.kind == 1) {   // an ice shard: a pale diamond with a cold streak
            draw::line(t, f.pos, f.pos + back * (26.f * k), 2.5f * k, sf::Color(190, 230, 255, 150));
            draw::polygon(t, f.pos, 6.f * k, 4, std::atan2(back.y, back.x), sf::Color(235, 248, 255), sf::Color(150, 200, 255));
            draw::glow(t, f.pos, 12.f * k, sf::Color(170, 220, 255), 0.5f);
        } else if (f.kind == 3) {   // ball lightning: a crackling blue orb
            draw::glow(t, f.pos, 22.f * k, sf::Color(130, 180, 255), 0.7f);
            draw::disc(t, f.pos, 7.f * k, sf::Color(230, 245, 255), sf::Color(120, 170, 255), {1.f, 1.f}, 12);
            for (int i = 0; i < 4; ++i) {
                const float an = clk * 13.f + static_cast<float>(i) * 1.57f;
                draw::line(t, f.pos, f.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (14.f * k), 1.5f * k, sf::Color(200, 230, 255, 200));
            }
        } else if (f.kind == 2) {   // molten gold: a heavy bright glob dripping
            draw::glow(t, f.pos, 20.f * k, sf::Color(255, 200, 80), 0.7f);
            draw::disc(t, f.pos, 8.f * k, sf::Color(255, 245, 190), sf::Color(240, 170, 50), {1.f, 1.f}, 14);
            draw::line(t, f.pos, f.pos + back * (18.f * k), 5.f * k, sf::Color(255, 190, 70, 150));
        } else {   // a fireball, arcing
            draw::glow(t, f.pos, 16.f * k, sf::Color(255, 150, 60), 0.7f);
            draw::disc(t, f.pos, 6.f * k, sf::Color(255, 240, 200), sf::Color(255, 140, 50), {1.f, 1.f}, 12);
            draw::line(t, f.pos, f.pos + back * (22.f * k), 4.f * k, sf::Color(255, 160, 70, 140));
        }
    }
    const sf::Color dragonCol[5] = {sf::Color(200, 230, 255), sf::Color(90, 150, 255), sf::Color(240, 190, 90), sf::Color(150, 70, 170),
                                    sf::Color(255, 150, 210)};
    for (const auto& d : tw.dragons) {   // each its own kind of dragon, after the Elder's look - grown as far as it has
        const sf::Color c = dragonCol[d.kind];
        if (d.stage == 0) {   // still an egg: it rocks, harder as it nears hatching, and cracks
            const float u = clampf(d.xp / cfg::tiamat::stageXp[0], 0.f, 1.f);
            const float r = (5.f + cfg::tiamat::adultSize[d.kind] * 0.12f) * k;
            const sf::Vector2f at = d.pos + sf::Vector2f{std::sin(clk * (6.f + 10.f * u)) * u * 2.f * k, 0.f};
            draw::glow(t, at, r * 2.2f, c, 0.15f + 0.25f * u);
            draw::disc(t, at, r, lerpColor(c, sf::Color::White, 0.55f), c, {0.78f, 1.f}, 16);
            if (u > 0.5f)
                draw::line(t, at + sf::Vector2f{-r * 0.5f, -r * 0.1f}, at + sf::Vector2f{r * 0.1f, r * 0.2f}, std::max(1.f, 1.2f * k), withAlpha(c, u));
            continue;
        }
        const float S = d.size * k;
        const float ph = d.kind == 0 ? clk * 6.f + static_cast<float>(d.owner) : d.flap;   // the winged: their own wingbeat (sim: beats, then glides)
        if (d.kind != 0 && d.kind != 3)   // on the wing: a faint wake where it has been
            for (int i = 1; i < d.trailN; ++i)
                draw::line(t, d.trail[i - 1], d.trail[i], std::max(1.f, S * 0.05f),
                           withAlpha(c, 0.22f * (1.f - static_cast<float>(i) / static_cast<float>(d.trailN))));
        const float glowA = 0.15f + 0.4f * d.flash;
        if (d.kind == 0) {   // the frost wyrm: a serpent dragon, pale ice, winding along its coil (longer as it grows)
            drawDragonForm(t, 3, d.pos, d.face, d.face, S, ph, c, glowA, d.trail, std::min(d.trailN, cfg::tiamat::wyrmBody[d.stage]), k);
        } else if (d.kind == 1) {   // the storm drake: a wyvern, storm blue, sparks crackling round it
            drawDragonForm(t, 1, d.pos, d.face, d.face, S, ph, c, glowA, nullptr, 0, k);
            for (int i = 0; i < d.stage; ++i) {
                const float an = clk * 7.f + static_cast<float>(i) * 2.1f;
                const sf::Vector2f q = d.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (S * 0.9f);
                draw::line(t, q, q + sf::Vector2f{std::cos(an * 3.f), std::sin(an * 3.f)} * (S * 0.2f), 1.5f * k, sf::Color(200, 230, 255));
            }
        } else if (d.kind == 2) {   // ancient gold: four wings, huge, a head for each stage
            drawDragonForm(t, 2, d.pos, d.face, d.headDir[0], S, ph, c, glowA, nullptr, 0, k, d.stage, d.stage > 1 ? d.headDir : nullptr);
        } else if (d.kind == 3) {   // the ravager: a dark dragon; on a run, wings folded fast and a streak behind it
            drawDragonForm(t, 0, d.pos, d.face, d.face, S, d.dive >= 0.f ? -kPi * 0.5f : ph, c, 0.12f + 0.3f * d.flash, nullptr, 0, k);   // on a run: wings swept back
        } else {   // the wyrmling: small and quick
            drawDragonForm(t, 0, d.pos, d.face, d.face, S, ph, c, glowA, nullptr, 0, k);
        }
    }
    for (const Ball& b : world.balls()) {   // Tiamat's mount, under her (the ball is drawn on top, riding it)
        if (!b.hasRole(BallRole::Tiamat)) continue;
        const sf::Vector2f d = normalized(b.vel, {1.f, 0.f});
        const float mxp = b.owner >= 0 && b.owner < TiamatWorld::kMounts ? tw.mountXp[b.owner] : 0.f;   // it grows all run (sim: tiamatTick)
        const float grown = cfg::tiamat::mountSize + (1.f - cfg::tiamat::mountSize) * clampf(mxp / cfg::tiamat::stageXp[2], 0.f, 1.f);
        drawDragonForm(t, 0, b.pos - d * (b.radius * 0.2f), d, d, b.radius * 3.4f * grown, clk * 4.f, roleColor(BallRole::Tiamat), 0.12f, nullptr, 0, k);
    }
    for (const auto& l : tw.lizards) {   // little lizards scuttling: a body, a tail, four legs paddling
        const sf::Vector2f dir = normalized(l.vel, {1.f, 0.f}), sd{-dir.y, dir.x};
        const sf::Color c(120, 190, 90, static_cast<sf::Uint8>(255 * clampf(l.life / 0.5f, 0.f, 1.f)));
        draw::line(t, l.pos - dir * (7.f * k), l.pos + dir * (5.f * k), 4.f * k, c);
        draw::line(t, l.pos - dir * (7.f * k), l.pos - dir * (13.f * k) + sd * (std::sin(l.gait) * 3.f * k), 2.f * k, c);
        for (int s = -1; s <= 1; s += 2)
            for (int f = 0; f < 2; ++f) {
                const float swing = std::sin(l.gait + static_cast<float>(f) * kPi) * static_cast<float>(s);
                const sf::Vector2f at = l.pos + dir * ((f ? -3.f : 3.f) * k);
                draw::line(t, at, at + sd * (static_cast<float>(s) * 5.f * k) + dir * (swing * 3.f * k), 1.3f * k, c);
            }
        draw::disc(t, l.pos + dir * (6.f * k), 2.f * k, sf::Color(255, 220, 90), sf::Color(255, 220, 90, 0), {1.f, 1.f}, 6);
    }
    (void)fire;
}

// Loki's clones at their games, its pachinko's steel balls, what the games paid.
void worldLoki(sf::RenderTarget& t, const World& world) {
    const LokiWorld& lw = world.classWorld().loki;
    if (lw.clones.empty() && lw.pins.empty() && lw.fx.empty()) return;
    const float k = world.arenaScale();
    const float clk = classClock();
    const sf::Color c = theme::classLoki, gold(240, 200, 90);
    for (const auto& f : lw.fx) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f), u = 1.f - a;
        switch (f.kind) {
            case 0: {   // a bolt from the sky
                const sf::Vector2f mid = f.a + (f.b - f.a) * 0.5f + sf::Vector2f{18.f * k, 0.f};
                draw::line(t, f.a, mid, 3.f * k, sf::Color(255, 240, 170, static_cast<sf::Uint8>(230 * a)));
                draw::line(t, mid, f.b, 3.f * k, sf::Color(255, 240, 170, static_cast<sf::Uint8>(230 * a)));
                draw::glow(t, f.b, f.r, gold, 0.6f * a);
                break;
            }
            case 1:   // red: a fire nova
                draw::ring(t, f.a, f.r * (0.3f + 0.7f * u), 4.f * k, sf::Color(255, 90, 70, static_cast<sf::Uint8>(220 * a)));
                draw::glow(t, f.a, f.r * 0.8f, sf::Color(255, 80, 50), 0.4f * a);
                break;
            case 2:   // black: a dark swirl pulling in
                for (int i = 0; i < 3; ++i)
                    draw::ring(t, f.a, f.r * std::max(0.05f, a - 0.2f * static_cast<float>(i)), 2.f * k, sf::Color(30, 20, 40, static_cast<sf::Uint8>(200 * a)));
                break;
            case 3:   // the jackpot: a burst of gold
                draw::glow(t, f.a, f.r, gold, 0.7f * a);
                draw::ring(t, f.a, f.r * (0.2f + 0.8f * u), 6.f * k, sf::Color(255, 230, 140, static_cast<sf::Uint8>(240 * a)));
                for (int i = 0; i < 12; ++i) {
                    const float an = static_cast<float>(i) * kPi / 6.f;
                    draw::disc(t, f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * u), 4.f * k, gold, withAlpha(gold, 0.f), {1.f, 1.f}, 8);
                }
                break;
            case 4: {   // a knife, flying
                const sf::Vector2f q = f.a + (f.b - f.a) * std::min(1.f, u * 1.6f), d = normalized(f.b - f.a, {1.f, 0.f});
                draw::line(t, q - d * (10.f * k), q, 2.f * k, sf::Color(230, 230, 240, static_cast<sf::Uint8>(255 * a)));
                break;
            }
            case 5:   // spades: a slash down the line
                draw::line(t, f.a, f.b, f.r * a, sf::Color(40, 40, 50, static_cast<sf::Uint8>(160 * a)));
                draw::line(t, f.a, f.b, 2.f * k, sf::Color(240, 240, 255, static_cast<sf::Uint8>(230 * a)));
                break;
            case 6:   // clubs: a blow
                draw::ring(t, f.a, f.r * (0.4f + 0.6f * u), 5.f * k, sf::Color(200, 210, 230, static_cast<sf::Uint8>(200 * a)));
                break;
            case 7: {   // hearts: a red glow flying to the core
                const sf::Vector2f q = f.a + (f.b - f.a) * u;
                draw::glow(t, q, 16.f * k, sf::Color(255, 90, 110), 0.7f * a);
                break;
            }
            case 8:   // diamonds: a shard
                draw::line(t, f.a + (f.b - f.a) * (u * 0.7f), f.a + (f.b - f.a) * std::min(1.f, u * 1.4f), 2.5f * k,
                           sf::Color(150, 220, 255, static_cast<sf::Uint8>(240 * a)));
                break;
            default:   // a clone splits off: a puff
                draw::ring(t, f.a, f.r * (1.f + u), 2.f * k, withAlpha(c, 0.6f * a));
                break;
        }
    }
    for (const auto& q : lw.pins) {   // steel balls
        draw::disc(t, q.pos, 4.5f * k, sf::Color(235, 240, 250), sf::Color(140, 150, 170), {1.f, 1.f}, 10);
    }
    for (const auto& cl : lw.clones) {
        const float a = clampf(cl.life / 0.5f, 0.f, 1.f) * clampf((cl.maxLife - cl.life) / 0.25f, 0.f, 1.f);
        float r = 14.f * k;
        for (const Ball& b : world.balls())
            if (b.owner == cl.owner) r = b.radius;   // as big as it is
        // the clone: a see-through copy of it, its horns on, flickering a little (an illusion)
        const float fl = 0.85f + 0.15f * std::sin(clk * 9.f + cl.pos.x * 0.05f);
        draw::glow(t, cl.pos, r * 2.2f, c, 0.3f * a);
        draw::disc(t, cl.pos, r, withAlpha(lerpColor(c, sf::Color::White, 0.25f), 0.7f * a * fl), withAlpha(c, 0.45f * a * fl), {1.f, 1.f}, 20);
        for (float s : {-1.f, 1.f}) {
            sf::Vector2f q = cl.pos + sf::Vector2f{s * r * 0.55f, -r * 0.6f};
            for (int i = 0; i < 4; ++i) {
                const float v = static_cast<float>(i) / 3.f;
                const sf::Vector2f nq = q + sf::Vector2f{s * r * (0.3f - 0.3f * v), -r * (0.2f + 0.12f * v)};
                draw::line(t, q, nq, std::max(1.f, r * 0.2f * (1.f - 0.7f * v)), withAlpha(gold, 0.85f * a));
                q = nq;
            }
        }
        if (cl.game < 0) continue;
        // its game, over its head: spinning, then what came up
        const float spin = clampf(cl.show / cl.showMax, 0.f, 1.f);
        const bool done = spin < 0.3f;
        const float s = 13.f * k;
        const sf::Vector2f g = cl.pos + sf::Vector2f{0.f, -r - s * 2.2f};
        draw::disc(t, g, s * 1.6f, withAlpha(sf::Color(15, 12, 25), 0.75f * a), withAlpha(sf::Color(15, 12, 25), 0.f), {1.f, 1.f}, 18);   // (a dark backdrop: it reads)
        switch (cl.game) {
            case 0: {   // a coin, flipping
                const float w = done ? 1.f : std::fabs(std::cos(clk * 22.f));
                const sf::Color fc = done ? (cl.a ? gold : sf::Color(180, 180, 190)) : gold;
                draw::disc(t, g, s, withAlpha(fc, a), withAlpha(lerpColor(fc, sf::Color::Black, 0.3f), a), {std::max(0.1f, w), 1.f}, 16);
                break;
            }
            case 1:   // two dice, tumbling
                for (int i = 0; i < 2; ++i) {
                    const sf::Vector2f dc = g + sf::Vector2f{(i ? 1.f : -1.f) * s * 0.75f, 0.f};
                    const float rot = done ? 0.f : clk * 14.f + static_cast<float>(i);
                    draw::polygon(t, dc, s * 0.85f, 4, rot + kPi * 0.25f, withAlpha(sf::Color(245, 245, 245), a), withAlpha(sf::Color(200, 200, 200), a));
                    const int pips = done ? (i ? cl.b : cl.a) : 1 + static_cast<int>(clk * 20.f + static_cast<float>(i) * 3.f) % 6;
                    for (int q = 0; q < pips; ++q) {
                        const float an = static_cast<float>(q) / static_cast<float>(pips) * 2.f * kPi;
                        const sf::Vector2f pp = pips == 1 ? dc : dc + sf::Vector2f{std::cos(an), std::sin(an)} * (s * 0.35f);
                        draw::disc(t, pp, 1.3f * k, withAlpha(sf::Color(30, 30, 30), a), withAlpha(sf::Color(30, 30, 30), a), {1.f, 1.f}, 6);
                    }
                }
                break;
            case 2: {   // a roulette wheel, spinning; the pocket it lands in lights up
                const float rot = done ? 0.f : clk * 12.f;
                for (int i = 0; i < 10; ++i) {
                    const float a0 = rot + static_cast<float>(i) * kPi / 5.f;
                    const sf::Color pc = i == 0 ? sf::Color(60, 170, 80) : i % 2 ? sf::Color(200, 40, 40) : sf::Color(25, 25, 30);
                    draw::ring(t, g, s * 0.8f, s * 0.45f, withAlpha(pc, a), a0, a0 + kPi / 5.f, 4);
                }
                draw::ring(t, g, s * 1.05f, 1.5f * k, withAlpha(gold, a));
                if (done) {
                    const sf::Color rc = cl.a == 0 ? sf::Color(60, 220, 100) : cl.a == 1 ? sf::Color(255, 70, 60) : sf::Color(120, 90, 160);
                    draw::glow(t, g, s * 2.2f, rc, 0.7f * a);
                }
                break;
            }
            case 3: {   // a card, turning over
                const float w = done ? 1.f : std::fabs(std::cos(clk * 10.f));
                const float cw = s * 0.75f * std::max(0.1f, w), ch = s * 1.05f;
                const bool face = done;
                const sf::Color back(110, 60, 150), paper(250, 248, 240);
                fillShape(t, {g + sf::Vector2f{-cw, -ch}, g + sf::Vector2f{cw, -ch}, g + sf::Vector2f{cw, ch}, g + sf::Vector2f{-cw, ch}},
                          withAlpha(face ? paper : back, a));
                if (face) {
                    if (cl.b == 1 || cl.b >= 11)   // a face card / an ace: a gold edge
                        for (int e = 0; e < 4; ++e) {
                            const sf::Vector2f p0 = g + sf::Vector2f{e == 1 || e == 2 ? cw : -cw, e >= 2 ? ch : -ch};
                            const sf::Vector2f p1 = g + sf::Vector2f{e == 0 || e == 1 ? cw : -cw, e == 0 || e == 3 ? -ch : ch};
                            draw::line(t, p0, p1, 1.5f * k, withAlpha(gold, a));
                        }
                    const sf::Color sc = cl.a == 2 || cl.a == 3 ? sf::Color(210, 40, 50) : sf::Color(25, 25, 30);
                    const float q = s * 0.3f;
                    if (cl.a == 2) {   // diamond
                        draw::polygon(t, g, q * 1.2f, 4, 0.f, withAlpha(sc, a), withAlpha(sc, a));
                    } else if (cl.a == 3) {   // heart
                        draw::disc(t, g + sf::Vector2f{-q * 0.5f, -q * 0.3f}, q * 0.6f, withAlpha(sc, a), withAlpha(sc, a), {1.f, 1.f}, 10);
                        draw::disc(t, g + sf::Vector2f{q * 0.5f, -q * 0.3f}, q * 0.6f, withAlpha(sc, a), withAlpha(sc, a), {1.f, 1.f}, 10);
                        fillShape(t, {g + sf::Vector2f{-q * 1.05f, -q * 0.1f}, g + sf::Vector2f{q * 1.05f, -q * 0.1f}, g + sf::Vector2f{0.f, q * 1.1f}}, withAlpha(sc, a));
                    } else if (cl.a == 0) {   // spade
                        draw::disc(t, g + sf::Vector2f{-q * 0.5f, q * 0.2f}, q * 0.6f, withAlpha(sc, a), withAlpha(sc, a), {1.f, 1.f}, 10);
                        draw::disc(t, g + sf::Vector2f{q * 0.5f, q * 0.2f}, q * 0.6f, withAlpha(sc, a), withAlpha(sc, a), {1.f, 1.f}, 10);
                        fillShape(t, {g + sf::Vector2f{-q * 1.05f, q * 0.1f}, g + sf::Vector2f{q * 1.05f, q * 0.1f}, g + sf::Vector2f{0.f, -q * 1.1f}}, withAlpha(sc, a));
                        draw::line(t, g, g + sf::Vector2f{0.f, q * 1.1f}, 1.5f * k, withAlpha(sc, a));
                    } else {   // club
                        for (int i = 0; i < 3; ++i) {
                            const float an = -kPi * 0.5f + static_cast<float>(i) * 2.f * kPi / 3.f;
                            draw::disc(t, g + sf::Vector2f{std::cos(an), std::sin(an)} * (q * 0.55f), q * 0.5f, withAlpha(sc, a), withAlpha(sc, a), {1.f, 1.f}, 10);
                        }
                        draw::line(t, g, g + sf::Vector2f{0.f, q * 1.1f}, 1.5f * k, withAlpha(sc, a));
                    }
                }
                break;
            }
            default: {   // a pachinko board: pegs, a steel ball dropping through
                fillShape(t, {g + sf::Vector2f{-s, -s * 1.2f}, g + sf::Vector2f{s, -s * 1.2f}, g + sf::Vector2f{s, s * 1.2f}, g + sf::Vector2f{-s, s * 1.2f}},
                          withAlpha(sf::Color(40, 30, 60), 0.85f * a));
                for (int row = 0; row < 3; ++row)
                    for (int col = 0; col < 3; ++col)
                        draw::disc(t, g + sf::Vector2f{(static_cast<float>(col) - 1.f + (row % 2 ? 0.5f : 0.f)) * s * 0.6f, (static_cast<float>(row) - 1.f) * s * 0.7f},
                                   1.2f * k, withAlpha(gold, a), withAlpha(gold, a), {1.f, 1.f}, 6);
                const float fall = 1.f - spin;
                draw::disc(t, g + sf::Vector2f{std::sin(fall * 17.f) * s * 0.5f, (fall * 2.f - 1.f) * s}, 2.f * k, withAlpha(sf::Color(235, 240, 250), a),
                           withAlpha(sf::Color(140, 150, 170), a), {1.f, 1.f}, 8);
                break;
            }
        }
    }
}

// Nergal's plague: blood clouds and miasmas, swarms, rats, the pale rider, bursts.
void worldNergal(sf::RenderTarget& t, const World& world) {
    const NergalWorld& nw = world.classWorld().nergal;
    const float k = world.arenaScale();
    const float clk = classClock();
    for (const Enemy& e : world.enemies()) {   // the black plague on them: they turn black, a bruise-violet rim
        if (e.hp <= 0.f || e.elem != Element::Plague || e.elemT <= 0.f) continue;
        const float a = clampf(e.elemT / 0.4f, 0.f, 1.f);
        draw::disc(t, e.pos, e.radius * 1.04f, withAlpha(theme::elemPlague, 0.88f * a), withAlpha(theme::elemPlague, 0.7f * a), {1.f, 1.f}, 20);
        draw::ring(t, e.pos, e.radius * 1.04f, 1.5f * k, withAlpha(theme::plagueEdge, 0.55f * a));
    }
    for (const auto& r : nw.rains) {
        const float a = clampf(r.life / 0.5f, 0.f, 1.f) * clampf((r.maxLife - r.life) / 0.3f, 0.f, 1.f);
        if (r.kind == 1) {   // a miasma: black smoke, rolling low
            for (int i = 0; i < 7; ++i) {
                const float an = static_cast<float>(i) * 0.9f + clk * 0.5f;
                draw::glow(t, r.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (r.r * 0.4f), r.r * 0.5f, sf::Color(20, 14, 26), 0.55f * a);
            }
            draw::ring(t, r.pos, r.r, 1.5f * k, withAlpha(theme::elemPlague, 0.5f * a));
            continue;
        }
        // a great dark red cloud, blood streaking down out of it
        for (int i = 0; i < 9; ++i) {
            const float an = static_cast<float>(i) * 0.7f + clk * 0.25f;
            draw::glow(t, r.pos + sf::Vector2f{std::cos(an) * r.r * 0.55f, std::sin(an) * r.r * 0.25f - r.r * 0.3f}, r.r * 0.42f, sf::Color(90, 15, 20), 0.4f * a);
        }
        for (int i = 0; i < 26; ++i) {   // the rain: streaks falling, each on its own beat, scattered
            const float fi = static_cast<float>(i);
            const float x = (std::fmod(fi * 0.6180339f, 1.f) - 0.5f) * 1.7f * r.r;
            const float fall = std::fmod(clk * 2.2f + std::fmod(fi * 0.7548777f * 3.1f, 1.f), 1.f);
            const sf::Vector2f top = r.pos + sf::Vector2f{x, -r.r * 0.3f + fall * r.r * 0.95f};
            draw::line(t, top, top + sf::Vector2f{-2.f * k, 11.f * k}, 1.5f * k, sf::Color(200, 30, 40, static_cast<sf::Uint8>(200 * a)));
        }
    }
    for (const auto& s : nw.swarms) {   // a swarm: specks darting about in a loose cloud
        const float a = clampf(s.life / 0.5f, 0.f, 1.f);
        const int n = 10 + static_cast<int>(s.r / k * 0.2f);
        for (int i = 0; i < n; ++i) {
            const float fi = static_cast<float>(i) + s.seed;
            const float an = clk * (3.f + std::fmod(fi * 0.37f, 2.f)) + fi * 2.4f;
            const float rr = s.r * (0.25f + 0.65f * std::fabs(std::sin(clk * 1.7f + fi)));
            const sf::Vector2f q = s.pos + sf::Vector2f{std::cos(an), std::sin(an * 1.13f)} * rr;
            draw::line(t, q, q + sf::Vector2f{std::cos(an + 1.6f), std::sin(an + 1.6f)} * (3.f * k), 1.8f * k,
                       sf::Color(110, 120, 70, static_cast<sf::Uint8>(235 * a)));
        }
    }
    for (const auto& r : nw.rats) {   // a rat: a grey-brown body, a pink tail swishing, ears, a scurry
        const sf::Vector2f dir = normalized(r.vel, {1.f, 0.f}), sd{-dir.y, dir.x};
        const float a = clampf(r.life / 0.5f, 0.f, 1.f);
        const sf::Color body(110, 95, 85, static_cast<sf::Uint8>(255 * a));
        draw::line(t, r.pos - dir * (6.f * k), r.pos - dir * (15.f * k) + sd * (std::sin(r.gait) * 4.f * k), 1.2f * k,
                   sf::Color(220, 150, 150, static_cast<sf::Uint8>(220 * a)));
        draw::disc(t, r.pos, 5.5f * k, body, body, {1.5f, 0.9f}, 12);
        draw::disc(t, r.pos + dir * (5.f * k), 3.f * k, body, body, {1.f, 1.f}, 8);
        for (float s : {-1.f, 1.f})
            draw::disc(t, r.pos + dir * (3.5f * k) + sd * (s * 2.8f * k), 1.6f * k, sf::Color(200, 140, 140, static_cast<sf::Uint8>(255 * a)),
                       sf::Color(200, 140, 140, 0), {1.f, 1.f}, 6);
        draw::disc(t, r.pos + dir * (7.f * k), 1.f * k, sf::Color(255, 60, 60, static_cast<sf::Uint8>(255 * a)), sf::Color(255, 60, 60, 0), {1.f, 1.f}, 4);
    }
    for (const auto& r : nw.riders) {   // the pale rider: a ghost horse at full gallop, a pale wake behind
        const sf::Vector2f d = normalized(r.to - r.from, {1.f, 0.f}), sd{-d.y, d.x};
        const sf::Color pale(200, 220, 200);
        draw::line(t, r.pos - d * (140.f * k), r.pos, 30.f * k, sf::Color(170, 200, 170, 40));
        draw::glow(t, r.pos, 50.f * k, pale, 0.45f);
        const float gal = std::sin(clk * 30.f);
        fillShape(t, {r.pos + d * (18.f * k), r.pos + sd * (8.f * k) - d * (4.f * k), r.pos - d * (20.f * k) + sd * (6.f * k),
                      r.pos - d * (20.f * k) - sd * (6.f * k), r.pos - sd * (8.f * k) - d * (4.f * k)}, withAlpha(pale, 0.85f));   // body
        draw::line(t, r.pos + d * (16.f * k), r.pos + d * (30.f * k) + sd * (5.f * k), 5.f * k, withAlpha(pale, 0.9f));    // neck, head
        for (int l = 0; l < 4; ++l) {   // legs at the gallop
            const float side = l % 2 ? 1.f : -1.f, front = l < 2 ? 1.f : -1.f;
            const sf::Vector2f hip = r.pos + d * (front * 12.f * k) + sd * (side * 6.f * k);
            draw::line(t, hip, hip + sd * (side * 10.f * k) + d * ((gal * front) * 10.f * k), 2.f * k, withAlpha(pale, 0.8f));
        }
        draw::disc(t, r.pos - d * (2.f * k), 5.f * k, sf::Color(30, 25, 35), sf::Color(30, 25, 35), {1.f, 1.f}, 10);   // the rider, dark
    }
    for (const auto& f : nw.fx) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f), u = 1.f - a;
        if (f.kind == 0) {   // the black death: a sickly cloud bursting
            draw::glow(t, f.a, f.r * 0.8f, sf::Color(40, 70, 20), 0.6f * a);
            draw::ring(t, f.a, f.r * (0.3f + 0.7f * u), 3.f * k, sf::Color(150, 200, 60, static_cast<sf::Uint8>(200 * a)));
        } else if (f.kind == 3) {   // fever: black lightning
            const sf::Vector2f mid = (f.a + f.b) * 0.5f + sf::Vector2f{(f.b - f.a).y, -(f.b - f.a).x} * 0.15f;
            draw::line(t, f.a, mid, 3.f * k, sf::Color(30, 20, 40, static_cast<sf::Uint8>(240 * a)));
            draw::line(t, mid, f.b, 3.f * k, sf::Color(30, 20, 40, static_cast<sf::Uint8>(240 * a)));
            draw::line(t, f.a, mid, 1.f * k, sf::Color(190, 160, 230, static_cast<sf::Uint8>(200 * a)));
            draw::line(t, mid, f.b, 1.f * k, sf::Color(190, 160, 230, static_cast<sf::Uint8>(200 * a)));
        } else if (f.kind == 4) {   // a mass grave: the ground cracks open, dark
            draw::disc(t, f.a, f.r * (0.4f + 0.6f * u), sf::Color(20, 15, 15, static_cast<sf::Uint8>(170 * a)), sf::Color(20, 15, 15, 0), {1.f, 1.f}, 20);
            for (int i = 0; i < 6; ++i) {
                const float an = static_cast<float>(i) * kPi / 3.f;
                draw::line(t, f.a, f.a + sf::Vector2f{std::cos(an), std::sin(an)} * (f.r * u), 2.f * k, sf::Color(90, 70, 60, static_cast<sf::Uint8>(200 * a)));
            }
        }
    }
}

// Apolo's muses: each in her own colour at her ball's side, the ball ringed in it; her weak point,
// arrows and arrival in it too. Gold is Apolo's (chords at the core).
void worldApolo(sf::RenderTarget& t, const World& world) {
    const ApoloWorld& aw = world.classWorld().apolo;
    if (aw.muses.empty() && aw.fx.empty()) return;
    const float k = world.arenaScale();
    const float clk = classClock();
    const sf::Color gold(255, 214, 110);
    // Strength red, Speed cyan, Twin violet, Giant green, Sun orange
    const sf::Color giftCol[5] = {sf::Color(255, 80, 70), sf::Color(90, 220, 255), sf::Color(190, 120, 255), sf::Color(110, 230, 110), sf::Color(255, 160, 40)};
    for (const Ball& b : world.balls()) {   // a ball with a gift: her colour round it
        const int g = b.cls.apolo.gift;
        if (g < 0) continue;
        const sf::Color c = giftCol[g];
        draw::glow(t, b.pos, b.radius * 2.f, c, 0.3f);
        draw::ring(t, b.pos, b.radius * 1.35f, 2.f * k, withAlpha(c, 0.7f));
        if (g == 4)   // the sun: a burning halo, wide
            for (int i = 0; i < 16; ++i) {
                const float an = static_cast<float>(i) * kPi / 8.f + clk * 1.2f;
                const float fl = 0.7f + 0.3f * std::sin(clk * 9.f + static_cast<float>(i));
                draw::glow(t, b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (cfg::apolo::sunR * k * 0.8f), 14.f * k * fl, c, 0.4f);
            }
    }
    for (const auto& mu : aw.muses) {   // a muse: a small figure robed in her colour, a gold head
        const sf::Color c = giftCol[mu.gift];
        draw::glow(t, mu.pos, 16.f * k, c, 0.45f);
        fillShape(t, {mu.pos + sf::Vector2f{0.f, -6.f * k}, mu.pos + sf::Vector2f{5.f * k, 7.f * k}, mu.pos + sf::Vector2f{-5.f * k, 7.f * k}},
                  withAlpha(c, 0.9f));   // her robe
        draw::disc(t, mu.pos + sf::Vector2f{0.f, -8.f * k}, 3.f * k, withAlpha(sf::Color(255, 240, 200), 0.95f), withAlpha(gold, 0.9f), {1.f, 1.f}, 10);
        draw::disc(t, mu.pos + sf::Vector2f{0.f, -15.f * k}, 2.2f * k, withAlpha(c, 0.95f), withAlpha(c, 0.f), {1.f, 1.f}, 8);   // her gift's spark
    }
    for (const auto& mu : aw.muses) {   // her weak point: a crosshair in her colour
        if (mu.mark < 0) continue;
        const sf::Color c = giftCol[mu.gift];
        for (const Enemy& e : world.enemies()) {
            if (e.id != mu.mark || e.hp <= 0.f) continue;
            const float R = e.radius + 6.f * k;
            draw::ring(t, e.pos, R, 1.5f * k, withAlpha(c, 0.85f));
            for (int i = 0; i < 4; ++i) {
                const float an = static_cast<float>(i) * kPi * 0.5f + clk;
                draw::line(t, e.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (R - 3.f * k), e.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (R + 5.f * k),
                           2.f * k, withAlpha(c, 0.95f));
            }
        }
    }
    for (const auto& f : aw.fx) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f), u = 1.f - a;
        const sf::Color c = giftCol[std::clamp(f.gift, 0, 4)];
        if (f.kind == 0) {   // a sun arrow
            const sf::Vector2f q = f.a + (f.b - f.a) * std::min(1.f, u * 1.8f), d = normalized(f.b - f.a, {1.f, 0.f});
            draw::line(t, q - d * (14.f * k), q, 2.f * k, withAlpha(c, a));
            draw::glow(t, q, 6.f * k, c, 0.6f * a);
        } else if (f.kind == 1) {   // a chord: golden rings off the core
            for (int i = 0; i < 3; ++i)
                draw::ring(t, f.a, f.r * std::max(0.05f, u - 0.15f * static_cast<float>(i)) + 20.f * k, 2.f * k, withAlpha(gold, 0.6f * a));
        } else {   // a muse arriving: a streak of her colour from where she was
            draw::line(t, f.a + (f.b - f.a) * (u * 0.7f), f.a + (f.b - f.a) * std::min(1.f, u * 1.3f), 3.f * k, withAlpha(c, 0.7f * a));
        }
    }
}

// Ares: ghost cannons round its ball, bloodlust, two moons; missiles, hoplites, chariots, fire, a falling star, a rain of spears.
void worldAres(sf::RenderTarget& t, const World& world) {
    namespace A = cfg::ares;
    const AresWorld& aw = world.classWorld().ares;
    const float k = world.arenaScale();
    const float clk = classClock();
    const sf::Color bronze(205, 150, 80), blood(220, 40, 30), fire(255, 140, 40), ember(255, 210, 120);
    // fire on the ground first (under everything)
    for (const auto& f : aw.flames) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f) * std::min(1.f, (f.maxLife - f.life) * 6.f + 0.3f);
        const float fl = 0.8f + 0.2f * std::sin(clk * 13.f + f.pos.x * 0.07f);
        draw::glow(t, f.pos, f.r * 1.3f * fl, fire, 0.35f * a);
        draw::glow(t, f.pos + sf::Vector2f{0.f, -f.r * 0.2f}, f.r * 0.6f * fl, ember, 0.35f * a);
    }
    for (const Ball& b : world.balls()) {
        if (!b.hasRole(BallRole::Ares) || b.ghost) continue;
        const AresState& st = b.cls.ares;
        if (st.lust > 0.f) {   // bloodlust: it burns red
            draw::glow(t, b.pos, b.radius * (2.f + 1.5f * st.lust), blood, 0.25f + 0.35f * st.lust);
            if (st.lust > 0.6f)
                for (int i = 0; i < 6; ++i) {
                    const float an = clk * 4.f + static_cast<float>(i) * kPi / 3.f;
                    draw::glow(t, b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (b.radius * 1.1f), b.radius * 0.5f, fire, 0.4f * st.lust);
                }
        }
        const sf::Vector2f ad{std::cos(st.aim), std::sin(st.aim)}, as{-ad.y, ad.x};
        for (int c = 0; c < A::cannons; ++c) {   // the ghost cannons: bronze barrels, aimed, kicking back as they fire
            const float an = static_cast<float>(c) * 2.f * kPi / A::cannons + b.age * 0.8f;
            const sf::Vector2f at = b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (b.radius + 18.f * k) - ad * (st.kick[c] * 4.f * k);
            draw::glow(t, at, 10.f * k, bronze, 0.25f);
            draw::line(t, at - ad * (5.f * k), at + ad * (9.f * k), 5.f * k, withAlpha(bronze, 0.75f));
            draw::line(t, at - ad * (5.f * k), at + ad * (9.f * k), 1.5f * k, withAlpha(ember, 0.5f));
            draw::disc(t, at - ad * (5.f * k), 3.5f * k, withAlpha(bronze, 0.8f), withAlpha(bronze, 0.3f), {1.f, 1.f}, 8);
            if (st.kick[c] > 0.5f || st.lust > 0.6f)   // a muzzle flash (and, in its fury, fire at every mouth)
                draw::glow(t, at + ad * (11.f * k), (6.f + 4.f * st.kick[c]) * k, st.lust > 0.6f ? fire : ember, 0.7f * std::max(st.kick[c], st.lust * 0.6f));
        }
        (void)as;
        if (b.mods.cls.ares.moons > 0.f)   // Phobos (red) and Deimos (black, red-rimmed)
            for (int i = 0; i < 2; ++i) {
                const float an = b.age * 1.6f + static_cast<float>(i) * kPi;
                const sf::Vector2f q = b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (b.radius + 46.f * k);
                const float R = (i ? 6.f : 7.5f) * k;
                draw::glow(t, q, R * 2.5f, blood, 0.3f);
                draw::disc(t, q, R, i ? sf::Color(30, 22, 24) : sf::Color(200, 70, 50), i ? sf::Color(120, 30, 30) : sf::Color(120, 40, 30), {1.f, 1.f}, 14);
                draw::ring(t, q, R, 1.2f * k, withAlpha(i ? blood : ember, 0.7f));
            }
    }
    for (const auto& m : aw.missiles) {   // a missile: a smoke trail, a flame, a red or black body
        const sf::Vector2f d = normalized(m.vel, {1.f, 0.f});
        for (int i = 1; i <= 6; ++i) {
            const float u = static_cast<float>(i) / 6.f;
            draw::disc(t, m.pos - d * (u * 34.f * k), (2.f + 3.f * u) * k, sf::Color(150, 140, 140, static_cast<sf::Uint8>(110 * (1.f - u))),
                       sf::Color(150, 140, 140, 0), {1.f, 1.f}, 8);
        }
        draw::glow(t, m.pos - d * (6.f * k), 7.f * k, fire, 0.7f);
        draw::line(t, m.pos - d * (6.f * k), m.pos + d * (5.f * k), 3.5f * k, m.moon ? sf::Color(40, 30, 32) : sf::Color(210, 70, 50));
        draw::disc(t, m.pos + d * (5.f * k), 1.8f * k, blood, blood, {1.f, 1.f}, 6);
    }
    for (const auto& h : aw.hoplites) {   // a hoplite: rising from the ground, a round shield, a spear at the enemy, a red crest
        const float grow = 1.f - clampf(h.sprout / A::sprout, 0.f, 1.f);
        const float a = std::min(1.f, h.life / 0.5f) * (0.4f + 0.6f * grow);
        const float s = (0.4f + 0.6f * grow) * k;
        const sf::Vector2f d{std::cos(h.aim), std::sin(h.aim)}, sd{-d.y, d.x};
        draw::glow(t, h.pos, 16.f * s, bronze, 0.25f * a);
        draw::line(t, h.pos - d * (8.f * s), h.pos + d * (20.f * s), 1.8f * k, withAlpha(sf::Color(160, 120, 70), a));   // the spear
        draw::disc(t, h.pos + d * (20.f * s), 2.f * k, withAlpha(ember, a), withAlpha(ember, 0.f), {1.f, 1.f}, 6);
        draw::disc(t, h.pos, 6.f * s, withAlpha(bronze, a), withAlpha(sf::Color(130, 90, 45), a), {1.f, 1.f}, 12);   // the body
        draw::line(t, h.pos - d * (4.f * s), h.pos + d * (4.f * s), 2.2f * k, withAlpha(blood, a));                    // the crest
        const sf::Vector2f sh = h.pos + sd * (7.f * s) + d * (3.f * s);   // the shield
        draw::disc(t, sh, 6.5f * s, withAlpha(sf::Color(220, 170, 90), a), withAlpha(sf::Color(150, 100, 50), a), {1.f, 1.f}, 14);
        draw::ring(t, sh, 6.5f * s, 1.2f * k, withAlpha(sf::Color(110, 70, 35), a));
        draw::disc(t, sh, 1.8f * s, withAlpha(blood, a), withAlpha(blood, a), {1.f, 1.f}, 6);
    }
    for (const auto& ch : aw.chariots) {   // the war chariot: two fire-breathing horses ahead, the car behind, flames everywhere
        const sf::Vector2f d = normalized(ch.to - ch.from, {1.f, 0.f}), sd{-d.y, d.x};
        draw::glow(t, ch.pos, 70.f * k, fire, 0.35f);
        for (int i = 0; i < 2; ++i) {
            const sf::Vector2f hp = ch.pos + d * (30.f * k) + sd * ((i ? 1.f : -1.f) * 12.f * k);
            const float gallop = std::sin(clk * 18.f + static_cast<float>(i) * 1.7f);
            draw::line(t, hp - d * (8.f * k), hp + d * (8.f * k), 10.f * k, sf::Color(70, 30, 20));   // its body
            draw::disc(t, hp + d * (13.f * k) + sd * (gallop * 1.5f * k), 4.5f * k, sf::Color(80, 35, 22), sf::Color(50, 20, 12), {1.4f, 1.f}, 10);
            for (int j = 0; j < 4; ++j) {   // its mane and its breath: fire
                const float u = static_cast<float>(j) / 4.f;
                draw::glow(t, hp + d * ((18.f + 14.f * u) * k) + sd * (std::sin(clk * 20.f + static_cast<float>(j)) * 4.f * k), (6.f + 5.f * u) * k,
                           j % 2 ? fire : ember, 0.6f * (1.f - u * 0.5f));
            }
            draw::glow(t, hp - d * (2.f * k), 6.f * k, fire, 0.5f);
        }
        const sf::Vector2f car = ch.pos - d * (6.f * k);
        draw::line(t, car, ch.pos + d * (22.f * k), 2.f * k, bronze);   // the pole
        draw::line(t, car - sd * (11.f * k), car + sd * (11.f * k), 12.f * k, sf::Color(170, 120, 60));   // the car
        draw::line(t, car - sd * (11.f * k) + d * (5.f * k), car + sd * (11.f * k) + d * (5.f * k), 2.5f * k, sf::Color(230, 180, 100));
        for (int i = 0; i < 2; ++i) {   // the wheels, spinning
            const sf::Vector2f w = car + sd * ((i ? 1.f : -1.f) * 14.f * k);
            draw::ring(t, w, 6.f * k, 1.8f * k, bronze);
            const float sp = clk * 20.f;
            draw::line(t, w + sf::Vector2f{std::cos(sp), std::sin(sp)} * (6.f * k), w - sf::Vector2f{std::cos(sp), std::sin(sp)} * (6.f * k), 1.f * k, bronze);
        }
        draw::disc(t, car - d * (2.f * k), 4.f * k, blood, blood, {1.f, 1.f}, 8);   // the charioteer's crest
    }
    for (const auto& s : aw.stars) {   // Wrath of Olympus: a red ring closing, the star falling into it
        const float u = clampf(s.t / A::wrathWarn, 0.f, 1.f);
        draw::ring(t, s.pos, s.r, 2.f * k, withAlpha(blood, 0.35f + 0.3f * u));
        draw::ring(t, s.pos, s.r * (1.f - 0.85f * u), 3.f * k, withAlpha(blood, 0.5f + 0.5f * u));
        draw::glow(t, s.pos, s.r * 0.8f, blood, 0.15f + 0.25f * u);
        const sf::Vector2f from = s.pos + sf::Vector2f{-260.f * k, -620.f * k};
        const sf::Vector2f q = from + (s.pos - from) * (u * u);
        const sf::Vector2f d = normalized(s.pos - from, {0.f, 1.f});
        for (int i = 0; i < 8; ++i) {
            const float v = static_cast<float>(i) / 8.f;
            draw::line(t, q - d * (v * 120.f * k), q - d * ((v + 0.125f) * 120.f * k), (8.f * (1.f - v) + 1.f) * k, withAlpha(i < 2 ? ember : blood, 0.7f * (1.f - v)));
        }
        draw::glow(t, q, 30.f * k, blood, 0.7f);
        draw::glow(t, q, 12.f * k, sf::Color(255, 240, 220), 0.9f);
    }
    for (const auto& ln : aw.lanes) {   // Trojan war: the next spears falling down the lane, their shadows under them
        const sf::Vector2f side = normalized(ln.to - ln.from, {1.f, 0.f});
        const float step = A::troyTime / static_cast<float>(A::troySpears);
        for (int i = ln.fired; i < std::min(A::troySpears, ln.fired + 4); ++i) {
            const float land = step * static_cast<float>(i);   // when it lands
            const float u = clampf(1.f - (land - ln.t) / (step * 4.f), 0.f, 1.f);   // 0 high up, 1 down
            const sf::Vector2f at = ln.from + (ln.to - ln.from) * ((static_cast<float>(i) + 0.5f) / static_cast<float>(A::troySpears));
            draw::disc(t, at, (4.f + 10.f * u) * k, sf::Color(0, 0, 0, static_cast<sf::Uint8>(90 * u)), sf::Color(0, 0, 0, 0), {1.6f, 0.7f}, 12);
            const sf::Vector2f tip = at + sf::Vector2f{-side.y * 0.f, -(1.f - u) * 300.f * k} + side * (-(1.f - u) * 60.f * k);
            const sf::Vector2f dd = normalized(at - (at + sf::Vector2f{0.f, -300.f * k} - side * (60.f * k)), {0.f, 1.f});
            draw::line(t, tip - dd * (46.f * k), tip, 2.f * k, withAlpha(sf::Color(150, 110, 65), 0.9f * u + 0.1f));
            draw::line(t, tip - dd * (9.f * k), tip, 3.5f * k, withAlpha(bronze, u * 0.9f + 0.1f));
        }
    }
    for (const auto& f : aw.fx) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f), u = 1.f - a;
        switch (f.kind) {
            case 0:   // a missile's blast
                draw::glow(t, f.a, f.r * (0.6f + 0.6f * u), fire, 0.6f * a);
                draw::ring(t, f.a, f.r * (0.3f + 0.8f * u), 2.5f * k, withAlpha(ember, 0.8f * a));
                break;
            case 1:   // a volley's flash
                draw::glow(t, f.a, f.r * 1.4f, ember, 0.8f * a);
                break;
            case 2:   // the star strikes: a white flash, a red shockwave, a dark crater
                draw::glow(t, f.a, f.r * (1.f + 0.6f * u), sf::Color(255, 240, 220), 0.8f * a * a);
                draw::ring(t, f.a, f.r * (0.4f + 1.1f * u), 6.f * k * a + 1.f, withAlpha(blood, 0.9f * a));
                draw::ring(t, f.a, f.r * (0.2f + 0.7f * u), 3.f * k, withAlpha(ember, 0.7f * a));
                draw::disc(t, f.a, f.r * 0.45f, sf::Color(30, 15, 10, static_cast<sf::Uint8>(150 * a)), sf::Color(30, 15, 10, 0), {1.f, 1.f}, 20);
                break;
            case 3:   // a spear lands: a blast of dust and fire, the spear standing in it
                draw::glow(t, f.a, f.r * (0.6f + 0.5f * u), fire, 0.5f * a);
                draw::ring(t, f.a, f.r * (0.3f + 0.7f * u), 2.f * k, withAlpha(ember, 0.7f * a));
                draw::line(t, f.a, f.a + sf::Vector2f{-10.f * k, -34.f * k}, 2.f * k, withAlpha(sf::Color(150, 110, 65), a));
                break;
            default:   // a tooth sown: the ground breaks open
                draw::ring(t, f.a, f.r * (0.4f + 0.8f * u), 2.f * k, withAlpha(bronze, 0.7f * a));
                draw::glow(t, f.a, f.r, bronze, 0.4f * a);
                break;
        }
    }
}

// Artemisa: the night (the arena dims; her ball and her prey shine), silver arrows, hounds, a golden stag, arrow rain.
void worldArtemisa(sf::RenderTarget& t, const World& world) {
    namespace A = cfg::artemisa;
    const ArtemisaWorld& aw = world.classWorld().artemisa;
    const float k = world.arenaScale();
    const float clk = classClock();
    const sf::Color silver(225, 232, 250), moon(200, 220, 255), gold(255, 210, 110), night(8, 10, 26);
    bool any = false;
    for (const Ball& b : world.balls()) any = any || b.hasRole(BallRole::Artemisa);
    if (!any && aw.arrows.empty() && aw.hounds.empty() && aw.stags.empty() && aw.rains.empty() && aw.fx.empty()) return;
    if (aw.night > 0.f) {   // night: the arena dims a little (the balls are drawn over it, so they stand out)
        const float a = std::min(1.f, aw.night / 0.6f) * std::min(1.f, (A::nightTime - aw.night) / 0.6f + 0.2f);
        sf::RectangleShape dim(world.viewSize());
        dim.setFillColor(withAlpha(night, 0.45f * clampf(a, 0.f, 1.f)));
        t.draw(dim);
    }
    for (const Ball& b : world.balls()) {
        if (!b.hasRole(BallRole::Artemisa) || b.ghost) continue;
        if (aw.night > 0.f) draw::glow(t, b.pos, b.radius * 4.f, moon, 0.45f);   // she shines in the dark
        const ArtemisaState& st = b.cls.artemisa;
        if (!st.route.empty()) {   // a night run: the way ahead, enemy to enemy
            sf::Vector2f at = b.pos;
            for (std::size_t i = static_cast<std::size_t>(st.routeAt); i < st.route.size(); ++i)
                for (const Enemy& e : world.enemies())
                    if (e.id == st.route[i] && e.hp > 0.f) {
                        draw::line(t, at, e.pos, 1.5f * k, withAlpha(silver, 0.35f));
                        at = e.pos;
                    }
        }
        for (const Enemy& e : world.enemies())   // her prey: a crescent over it
            if (e.id == st.prey && e.hp > 0.f && e.mark > 0.f) {
                const sf::Vector2f q = e.pos + sf::Vector2f{0.f, -e.radius - 12.f * k};
                draw::glow(t, q, 14.f * k, moon, 0.5f);
                crescent(t, q, 6.f * k, silver, sf::Color(20, 24, 44), 1.f);
            }
    }
    if (aw.night > 0.f)   // ...and at night every enemy is prey: a small crescent on each
        for (const Enemy& e : world.enemies()) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const sf::Vector2f q = e.pos + sf::Vector2f{0.f, -e.radius - 9.f * k};
            crescent(t, q, 4.f * k, silver, sf::Color(20, 24, 44), 0.85f);
        }
    for (const auto& ar : aw.arrows) {   // the silver arrow: a streak racing down the line, a faint wake behind
        const float a = clampf(ar.life / ar.maxLife, 0.f, 1.f), u = 1.f - a;
        const sf::Vector2f d = normalized(ar.b - ar.a, {1.f, 0.f});
        const float L = length(ar.b - ar.a);
        const sf::Vector2f head = ar.a + d * (L * std::min(1.f, u * 2.5f));
        draw::line(t, ar.a, head, 1.2f * k, withAlpha(silver, 0.35f * a));
        draw::line(t, head - d * (60.f * k), head, 2.5f * k, withAlpha(silver, 0.9f * a));
        draw::glow(t, head, 10.f * k, moon, 0.6f * a);
    }
    for (const auto& h : aw.hounds) {   // a hound: a spectral silver dog at full stretch
        const float a = std::min(1.f, h.life / 0.4f);
        const sf::Vector2f d = normalized(h.vel, {1.f, 0.f}), sd{-d.y, d.x};
        const float s = k;
        draw::glow(t, h.pos, 20.f * s, moon, 0.3f * a);
        for (int i = 1; i <= 3; ++i)   // a ghostly wake
            draw::disc(t, h.pos - d * (static_cast<float>(i) * 9.f * s), (5.f - static_cast<float>(i)) * s, withAlpha(moon, 0.25f * a), withAlpha(moon, 0.f), {1.f, 1.f}, 8);
        draw::line(t, h.pos - d * (9.f * s), h.pos + d * (6.f * s), 7.f * s, withAlpha(silver, 0.85f * a));   // body
        draw::disc(t, h.pos + d * (10.f * s), 4.f * s, withAlpha(silver, 0.9f * a), withAlpha(silver, 0.6f * a), {1.f, 1.f}, 10);   // head
        draw::line(t, h.pos + d * (11.f * s), h.pos + d * (16.f * s), 2.5f * s, withAlpha(silver, 0.9f * a));                     // snout
        draw::line(t, h.pos + d * (9.f * s) + sd * (2.f * s), h.pos + d * (7.f * s) + sd * (6.f * s), 1.8f * s, withAlpha(silver, 0.9f * a));   // an ear
        const float g = std::sin(h.gait);
        for (int leg = 0; leg < 2; ++leg) {   // the legs, galloping
            const sf::Vector2f hip = h.pos + d * ((leg ? 5.f : -7.f) * s);
            const float sw = (leg ? g : -g) * 5.f * s;
            draw::line(t, hip, hip + sd * (6.f * s) + d * sw, 1.6f * s, withAlpha(silver, 0.8f * a));
            draw::line(t, hip, hip - sd * (6.f * s) - d * sw, 1.6f * s, withAlpha(silver, 0.8f * a));
        }
        draw::line(t, h.pos - d * (9.f * s), h.pos - d * (16.f * s) + sd * (std::sin(h.gait * 0.7f) * 3.f * s), 1.6f * s, withAlpha(silver, 0.7f * a));   // tail
    }
    for (const auto& s : aw.stags) {   // the golden stag: a trail of light, a golden body, great antlers
        const sf::Vector2f d = s.dir, sd{-d.y, d.x};
        for (int i = 1; i <= 8; ++i) {
            const float u = static_cast<float>(i) / 8.f;
            draw::glow(t, s.pos - d * (u * 160.f * k), (18.f - 12.f * u) * k, gold, 0.4f * (1.f - u));
        }
        draw::glow(t, s.pos, 40.f * k, gold, 0.45f);
        draw::line(t, s.pos - d * (16.f * k), s.pos + d * (10.f * k), 13.f * k, sf::Color(230, 180, 80));   // body
        const sf::Vector2f head = s.pos + d * (20.f * k);
        draw::line(t, s.pos + d * (8.f * k), head, 5.f * k, sf::Color(235, 190, 90));                          // neck
        draw::disc(t, head, 5.5f * k, sf::Color(245, 205, 110), sf::Color(220, 170, 70), {1.f, 1.f}, 12);
        for (int side = -1; side <= 1; side += 2) {   // antlers: a beam and its tines
            const sf::Vector2f base = head + sd * (static_cast<float>(side) * 3.f * k);
            const sf::Vector2f tip = base + sd * (static_cast<float>(side) * 18.f * k) - d * (6.f * k);
            draw::line(t, base, tip, 2.2f * k, sf::Color(255, 225, 140));
            for (int j = 1; j <= 2; ++j) {
                const sf::Vector2f q = base + (tip - base) * (0.35f * static_cast<float>(j));
                draw::line(t, q, q + d * (8.f * k) + sd * (static_cast<float>(side) * 3.f * k), 1.6f * k, sf::Color(255, 225, 140));
            }
        }
        const float g = std::sin(clk * 22.f);
        for (int leg = 0; leg < 2; ++leg) {
            const sf::Vector2f hip = s.pos + d * ((leg ? 6.f : -12.f) * k);
            draw::line(t, hip, hip + sd * (10.f * k) + d * ((leg ? g : -g) * 7.f * k), 2.f * k, sf::Color(220, 170, 70));
            draw::line(t, hip, hip - sd * (10.f * k) - d * ((leg ? g : -g) * 7.f * k), 2.f * k, sf::Color(220, 170, 70));
        }
    }
    for (const auto& r : aw.rains) {   // arrow rain: a volley falling, its shadow gathering under it
        const float u = clampf(1.f - (r.land - r.t) / A::rainFall, 0.f, 1.f);
        if (r.t < r.land - A::rainFall) continue;
        draw::disc(t, r.pos, A::rainR * k * (0.4f + 0.5f * u), sf::Color(0, 0, 0, static_cast<sf::Uint8>(70 * u)), sf::Color(0, 0, 0, 0), {1.f, 0.6f}, 16);
        for (int i = 0; i < 6; ++i) {
            const float ox = (hash01(static_cast<float>(i) * 2.3f + r.pos.x) - 0.5f) * A::rainR * k * 1.4f;
            const float oy = (hash01(static_cast<float>(i) * 4.1f + r.pos.y) - 0.5f) * A::rainR * k * 0.8f;
            const sf::Vector2f land = r.pos + sf::Vector2f{ox, oy};
            const sf::Vector2f tip = land - sf::Vector2f{-60.f * k, 240.f * k} * (1.f - u);
            draw::line(t, tip - sf::Vector2f{-9.f, 36.f} * k * 0.7f, tip, 1.6f * k, withAlpha(silver, 0.9f));
        }
    }
    for (const auto& f : aw.fx) {
        const float a = clampf(f.life / f.maxLife, 0.f, 1.f), u = 1.f - a;
        if (f.kind == 0) {   // a bite: three silver slashes
            const sf::Vector2f d = normalized(f.b - f.a, {1.f, 0.f}), sd{-d.y, d.x};
            for (int i = -1; i <= 1; ++i)
                draw::line(t, f.b + sd * (static_cast<float>(i) * 4.f * k) - d * (f.r * 0.6f), f.b + sd * (static_cast<float>(i) * 4.f * k) + d * (f.r * 0.6f),
                           1.8f * k, withAlpha(silver, 0.9f * a));
        } else if (f.kind == 1) {   // a volley lands: a ring, and arrows standing in the ground
            draw::ring(t, f.a, f.r * (0.4f + 0.7f * u), 2.f * k, withAlpha(silver, 0.7f * a));
            draw::glow(t, f.a, f.r * 0.7f, moon, 0.35f * a);
            for (int i = 0; i < 5; ++i) {
                const sf::Vector2f q = f.a + sf::Vector2f{(hash01(static_cast<float>(i) * 1.9f + f.a.x) - 0.5f) * f.r, (hash01(static_cast<float>(i) * 3.7f + f.a.y) - 0.5f) * f.r * 0.6f};
                draw::line(t, q, q + sf::Vector2f{6.f, -16.f} * k, 1.5f * k, withAlpha(silver, a));
            }
        } else {   // a night strike: a silver cross where she ran through
            draw::line(t, f.a - sf::Vector2f{f.r, f.r}, f.a + sf::Vector2f{f.r, f.r}, 2.5f * k, withAlpha(silver, a));
            draw::line(t, f.a - sf::Vector2f{f.r, -f.r}, f.a + sf::Vector2f{f.r, -f.r}, 2.5f * k, withAlpha(silver, a));
            draw::glow(t, f.a, f.r * 1.4f, moon, 0.6f * a);
        }
    }
}

void drawClassWorld(sf::RenderTarget& t, const World& world) {
    worldPlanets(t, world);
    worldGods(t, world);
    worldGods2(t, world);
    worldLoki(t, world);
    worldNergal(t, world);
    worldApolo(t, world);
    worldAres(t, world);
    worldArtemisa(t, world);
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

bool drawFxSheet(sf::RenderTarget& t, int page, const sf::Font& font) {
    using F = CommonWorld::Fx;
    // kind, label, radius, life, b: 0 none, 1 a direction, 2 a point (offset), 3 card / coin data
    struct Row { F::Kind kind; const char* name; float r; float life; int bMode; sf::Color c, c2; };
    const sf::Color mage = theme::classMage, jes = theme::classJester, ass = theme::classAssassin, sum = theme::classSummoner,
                    sho = theme::classShooter, sli = theme::classSlinger, fire = theme::elemFire, water = theme::elemWater;
    const Row rows[] = {
        {F::Hook, "Hook (ab)", 0.f, 0.6f, 2, theme::accent, sf::Color::White},
        {F::Smite, "Smite (ab)", 14.f, 0.4f, 2, theme::accent, sf::Color::White},
        {F::Warp, "Warp (ab)", 12.f, 0.5f, 2, theme::accent, sf::Color::White},
        {F::Swap, "Swap (ab)", 0.f, 0.5f, 2, theme::accent, ass},
        {F::Implode, "Implode", 40.f, 0.4f, 0, theme::accent, sf::Color::White},
        {F::Shards, "Shards", 40.f, 0.4f, 0, theme::accent, sf::Color::White},
        {F::Meteor, "Meteor (ab)", 40.f, 0.7f, 0, theme::accent, sf::Color::White},
        {F::Afterimage, "Dash (ab)", 10.f, 0.4f, 1, theme::accent, sf::Color::White},
        {F::Shock, "Empower (ab)", 36.f, 0.4f, 0, theme::accent, sf::Color::White},
        {F::Slash, "Death sentence", 28.f, 0.4f, 0, ass, sf::Color::White},
        {F::Clover, "Lucky break", 24.f, 0.5f, 0, jes, sf::Color::White},
        {F::Petals, "Maelstrom", 40.f, 0.5f, 0, fire, water},
        {F::Coins, "Jackpot coins", 40.f, 0.8f, 0, theme::puGolden, sf::Color::White},
        {F::Reels, "Slot pair", 10.f, 0.6f, 4, jes, sf::Color::White},
        {F::Boomerang, "Homecoming", 40.f, 0.45f, 0, sli, sf::Color::White},
        {F::Bastion, "Last line", 50.f, 0.7f, 0, theme::core, sf::Color::White},
        {F::Glyph, "Sigil", 40.f, 0.45f, 0, mage, sf::Color::White},
        {F::Cracks, "Titan quake", 50.f, 0.8f, 0, theme::core, sf::Color::White},
        {F::Dust, "Heavy landing", 40.f, 0.5f, 0, theme::core, sf::Color::White},
        {F::Clock, "Time stop", 40.f, 0.8f, 0, mage, sf::Color::White},
        {F::Echo, "Spell chorus", 30.f, 0.6f, 0, mage, sf::Color::White},
        {F::Star, "Grand reaction", 50.f, 0.6f, 0, fire, water},
        {F::Hexshield, "Ward ring", 26.f, 0.5f, 2, theme::core, sf::Color::White},
        {F::Link, "Fission", 0.f, 0.35f, 2, theme::elemPoison, sf::Color::White},
        {F::Sonic, "Mega Striker", 40.f, 0.35f, 1, theme::ballFast, sf::Color::White},
        {F::Rune, "Ancient nova", 40.f, 0.45f, 0, mage, sf::Color::White},
        {F::Spokes, "Lead storm", 30.f, 0.4f, 0, sho, sf::Color::White},
        {F::Pellets, "Frag rounds", 30.f, 0.25f, 0, sho, sf::Color::White},
        {F::Smoke, "Smoke bomb", 40.f, 0.8f, 0, ass, sf::Color::White},
        {F::Wisps, "Necromancer", 30.f, 0.6f, 0, sum, sf::Color::White},
        {F::Pillar, "Martyr", 22.f, 0.6f, 0, sum, sf::Color::White},
        {F::Blast, "Bomber", 36.f, 0.45f, 0, fire, sf::Color::White},
        {F::Fuse, "Jester bomb", 36.f, 0.45f, 0, fire, sf::Color::White},
        {F::Confetti, "Jester jackpot", 44.f, 0.9f, 0, theme::puGolden, sf::Color::White},
        {F::Cards, "Sleight", 24.f, 0.5f, 0, jes, sf::Color::White},
        {F::Arrow, "Ambush", 30.f, 0.4f, 1, sli, sf::Color::White},
        {F::Rise, "Golem rises", 30.f, 0.5f, 0, sum, sf::Color::White},
        {F::Slam, "Golem slam", 40.f, 0.4f, 0, sum, sf::Color::White},
        {F::Spiral, "Great spirit", 40.f, 0.6f, 0, sum, sf::Color::White},
        {F::Collapse, "Black hole", 40.f, 0.5f, 0, theme::accent, sf::Color::White},
        {F::Sight, "Seeker", 10.f, 0.3f, 2, theme::accent, sf::Color::White},
        {F::Ping, "Ping-pong", 10.f, 0.35f, 1, theme::accent, sf::Color::White},
        {F::Beckon, "Beckon", 10.f, 0.4f, 2, theme::accent, sf::Color::White},
        {F::Mana, "Mana spring", 30.f, 0.5f, 0, mage, sf::Color::White},
        {F::Leap, "Conflux", 0.f, 0.4f, 2, fire, water},
        {F::Saturate, "Saturate", 30.f, 0.5f, 0, fire, water},
        {F::Wild, "Wild magic", 26.f, 0.5f, 0, jes, sf::Color::White},
        {F::Chrono, "Chronomancy", 50.f, 0.9f, 0, mage, sf::Color::White},
        {F::Scythe, "Reaper", 24.f, 0.3f, 1, ass, sf::Color::White},
        {F::Streak, "Lightspeed", 10.f, 0.25f, 2, theme::accent, sf::Color::White},
        {F::Drop, "Primal rain", 10.f, 0.4f, 0, water, sf::Color::White},
        {F::Ion, "Ion cannon", 0.f, 0.22f, 2, sho, sf::Color::White},
        {F::Card, "Jester card", 10.f, 1.1f, 3, jes, sf::Color::White},
        {F::Frost, "Primal ice", 36.f, 0.8f, 0, theme::elemIce, sf::Color::White},
        {F::Rubble, "Primal stone", 12.f, 0.5f, 0, theme::elemStone, sf::Color::White},
        {F::Splash, "Primal poison", 30.f, 0.5f, 0, theme::elemPoison, sf::Color::White},
        {F::Storm, "Primal electric", 12.f, 0.4f, 0, theme::elemElectric, sf::Color::White},
        {F::Flip, "Coin flip", 5.f, 0.6f, 3, theme::puGolden, sf::Color::White},
        {F::Execute, "Cull (new)", 14.f, 0.35f, 0, ass, sf::Color::White},
        {F::Glint, "Backstab (new)", 13.f, 0.3f, 1, ass, sf::Color::White},
        {F::Twin, "Twincast (new)", 9.f, 0.45f, 1, mage, sf::Color::White},
    };
    constexpr int kRows = static_cast<int>(sizeof(rows) / sizeof(rows[0]));
    constexpr int kPerRow = 3, kRowsPerPage = 5, kPerPage = kPerRow * kRowsPerPage;
    const int first = page * kPerPage;
    if (first >= kRows) return false;
    const float phases[3] = {0.12f, 0.4f, 0.75f};
    const float cell = 118.f;
    for (int n = 0; n < kPerPage && first + n < kRows; ++n) {
        const Row& row = rows[first + n];
        const float gx = 20.f + static_cast<float>(n % kPerRow) * (cell * 3.f + 54.f);
        const float gy = 30.f + static_cast<float>(n / kPerRow) * (cell + 34.f);
        sf::Text label(row.name, font, 15);
        label.setFillColor(theme::textLo);
        label.setPosition(gx, gy - 22.f);
        t.draw(label);
        for (int ph = 0; ph < 3; ++ph) {
            const sf::Vector2f c{gx + cell * (static_cast<float>(ph) + 0.5f), gy + cell * 0.5f};
            draw::box(t, {c.x - cell * 0.5f + 2.f, c.y - cell * 0.5f + 2.f, cell - 4.f, cell - 4.f}, 3.f,
                      withAlpha(theme::bgDeep, 1.f), withAlpha(theme::bgDeep, 1.f), withAlpha(theme::arenaEdge, 0.4f), 1.f);
            F f;
            f.kind = row.kind;
            f.r = row.r;
            f.maxLife = row.life;
            f.life = row.life * (1.f - phases[ph]);
            f.color = row.c;
            f.color2 = row.c2;
            f.a = c;
            switch (row.bMode) {
                case 1: f.b = normalized(sf::Vector2f{1.f, -0.4f}, {1.f, 0.f}); break;
                case 2: f.a = c + sf::Vector2f{-34.f, 22.f}; f.b = c + sf::Vector2f{34.f, -22.f}; break;
                case 3: f.b = {0.f, 12.f}; if (row.kind == F::Flip) f.b = {1.f, 0.f}; break;
                case 4: f.b = {0.f, 0.f}; break;
                default: break;
            }
            if (row.kind == F::Smite) { f.a = c + sf::Vector2f{0.f, -60.f}; f.b = c + sf::Vector2f{0.f, 30.f}; }
            if (row.kind == F::Hexshield) { f.a = c; f.b = c + sf::Vector2f{40.f, -10.f}; }
            if (row.kind == F::Beckon) { f.a = c + sf::Vector2f{20.f, -10.f}; f.b = c + sf::Vector2f{-30.f, 20.f}; }
            if (row.kind != F::Hook && row.kind != F::Link && row.kind != F::Leap && row.kind != F::Ion && row.kind != F::Swap &&
                row.kind != F::Streak && row.kind != F::Sight && row.kind != F::Smite)   // a stand-in ball / enemy under it
                draw::disc(t, c, 9.f, withAlpha(row.c, 0.55f), withAlpha(row.c, 0.25f), {1.f, 1.f}, 20);
            drawFx(t, f, 1.f);
        }
    }
    return true;
}

}  // namespace sb
