// Speed items for the older classes (2026-09-28). A "still" item grows as its
// ball slows (full at a standstill), a "moving" one as it speeds up past its
// cruise - never on / off. Guardian (Anchor, Plow), Support (Beacon, Wake,
// Pass) and Mage (Leyline) live here because those classes' items work on any
// ball; Slug / Strafe (Shooter), Lurk / Blur (Assassin), Sleight (Jester),
// Meditate (Mage) and Kennel / Drop turret (Summoner) sit in their class
// sections in sim/WorldClasses.cpp and read the same two numbers.

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

float World::styleStill(const Ball& b, const WorldParams& p) const {
    return clampf(1.f - length(b.vel) / std::max(1.f, ballCruise(b, p)), 0.f, 1.f);
}

float World::styleFast(const Ball& b, const WorldParams& p) const {
    const float over = length(b.vel) / std::max(1.f, ballCruise(b, p)) - 1.f;
    return clampf(over / cfg::style::fastFull, 0.f, 1.f);
}

bool World::styleBlurring(const Ball& b, const WorldParams& p) const {   // "Blur"
    return b.mods.cls.assassin.blur > 0.f && length(b.vel) > ballCruise(b, p) * cfg::style::blurAt;
}

void World::styleTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace S = cfg::style;
    const float k = arenaScale();
    const float still = b.held ? 1.f : styleStill(b, p);
    const float fast = b.held ? 0.f : styleFast(b, p);
    b.cls.mage.still = still;   // the class sections (Slug, Meditate, Kennel, Sleight) and the renderer read it
    SupportState& ss = b.cls.support;
    ss.beaconT = std::max(0.f, ss.beaconT - dt);
    ss.wakeCd = std::max(0.f, ss.wakeCd - dt);
    const ClassMods& c = b.mods.cls;

    // "Plow": fast, it shoves aside what it passes (and hits it once per shove).
    if (c.guardian.plow > 0.f && fast > 0.f) {
        const sf::Vector2f h = normalized(b.vel, {1.f, 0.f});
        const float reach = b.radius * S::plowReach;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || e.stagger > 0.f) continue;
            const sf::Vector2f d = e.pos - b.pos;
            if (length(d) > reach + e.radius) continue;
            const sf::Vector2f side = normalized(d - h * dot(d, h), normalized(d, {0.f, 1.f}));
            e.vel += side * c.guardian.plow * fast * k * e.knockTaken;
            e.stagger = S::plowStagger;
            damageEnemy(e, ballDamage(b, p) * c.guardian.plowFrac);
        }
    }
    // "Beacon": still, its glow charges every other ball passing through.
    if (c.support.beacon > 0.f && still > 0.05f && !b.ghost) {
        const float R = c.support.beaconRadius * k * (0.4f + 0.6f * still);
        for (Ball& o : balls_) {
            if (&o == &b || o.held || length(o.pos - b.pos) > R + o.radius) continue;
            SupportState& os = o.cls.support;
            os.beaconT = S::beaconTime;
            os.beaconMul = 1.f + c.support.beacon * still;
            os.beaconElem = static_cast<int>(b.element);
            os.beaconOwner = b.owner;
        }
    }
    // "Wake": fast, it lays a trail that speeds up the other balls.
    if (c.support.wake > 0.f && fast > 0.f && !b.ghost && (ss.wakeT -= dt) <= 0.f) {
        ss.wakeT = S::wakeEvery;
        auto& wk = classWorld_.support.wake;
        if (static_cast<int>(wk.size()) >= S::maxWake) wk.erase(wk.begin());
        wk.push_back({b.pos, S::wakeLife, c.support.wake, b.owner});
    }
    // "Leyline": moving, it drops runes for its next cast.
    if (c.mage.leyline > 0.f && !b.ghost && length(b.vel) > ballCruise(b, p) * 0.9f && (b.cls.mage.runeT -= dt) <= 0.f) {
        b.cls.mage.runeT = S::runeEvery;
        auto& rs = classWorld_.mage.runes;
        const int mine = static_cast<int>(std::count_if(rs.begin(), rs.end(), [&](const MageWorld::Rune& r) {
            return r.owner == b.owner;
        }));
        if (mine < S::runesPerBall) rs.push_back({b.pos, S::runeLife, b.owner});
    }
    // "Lurk": staying slow charges the hit after its next blink.
    if (c.assassin.lurk > 0.f)
        b.cls.assassin.lurk = std::min(1.f, b.cls.assassin.lurk + still * dt / S::lurkFill);
    (void)ev;
}

void World::styleWorldTick(float dt, const WorldParams& p) {
    namespace S = cfg::style;
    const float k = arenaScale();
    auto& wk = classWorld_.support.wake;
    for (auto& w : wk) w.life -= dt;
    wk.erase(std::remove_if(wk.begin(), wk.end(), [](const SupportWorld::WakePoint& w) { return w.life <= 0.f; }), wk.end());
    if (!wk.empty()) {   // "Wake": a ball crossing someone else's trail is sped up
        const float r = S::wakeRadius * k;
        for (Ball& o : balls_) {
            if (o.held || o.cls.support.wakeCd > 0.f) continue;
            for (const auto& w : wk) {
                if (w.owner == o.owner || length(o.pos - w.pos) > r + o.radius) continue;
                boostSpeed(o, w.boost, p);
                o.cls.support.wakeCd = S::wakeCd;
                break;
            }
        }
    }
    auto& rs = classWorld_.mage.runes;
    for (auto& r : rs) r.life -= dt;
    rs.erase(std::remove_if(rs.begin(), rs.end(), [](const MageWorld::Rune& r) { return r.life <= 0.f; }), rs.end());
}

// "Anchor": enemies near a slow Anchor ball move slower and drift toward it.
void World::styleEnemyDrag(Enemy& e, float& edt, float dt) {
    float slow = 0.f;
    for (const Ball& b : balls_) {
        const GuardianMods& g = b.mods.cls.guardian;
        if (g.anchor <= 0.f) continue;
        const float still = b.cls.mage.still;
        if (still <= 0.f) continue;
        const sf::Vector2f to = b.pos - e.pos;
        const float d = length(to);
        if (d > g.anchorRadius * arenaScale() + e.radius || d < 1e-3f) continue;
        slow = std::max(slow, g.anchor * still);
        e.pos += to / d * g.anchorPull * still * arenaScale() * dt;
    }
    edt *= 1.f - slow;
}

// A Beacon / Pass charge riding on this ball: its hit is bigger and leaves
// the charging ball's element (a different owner, so it can react).
float World::stylePreHit(Ball& b, Enemy& e, const WorldParams& p, FrameEvents& ev) {
    SupportState& s = b.cls.support;
    if (s.beaconT <= 0.f) return 1.f;
    s.beaconT = 0.f;
    if (s.beaconElem != 0) applyElement(e, static_cast<Element>(s.beaconElem), s.beaconOwner, ballDamage(b, p), p, ev);
    return s.beaconMul;
}

void World::styleOnHit(Ball& b, Enemy& e, const WorldParams& p) {
    if (styleBlurring(b, p))   // "Blur": what it cuts through is marked
        e.mark = std::max(e.mark, cfg::role::markDuration * b.mods.cls.assassin.blur);
}

// "Pass": a clack launches the other ball, carrying this one's element.
void World::stylePass(const Ball& a, Ball& o, const WorldParams& p) {
    const float pass = a.mods.cls.support.pass;
    if (pass <= 0.f || o.held) return;
    const float sp = std::max(length(o.vel), ballCruise(o, p)) * pass;
    o.vel = normalized(o.pos - a.pos, {1.f, 0.f}) * std::min(sp, ballMaxSpeed(o, p));
    SupportState& os = o.cls.support;
    os.beaconT = cfg::style::beaconTime;
    os.beaconMul = cfg::style::passCharge;
    os.beaconElem = static_cast<int>(a.element);
    os.beaconOwner = a.owner;
}

// "Leyline": a cast bursts every rune the ball left.
void World::styleOnCast(Ball& b, const WorldParams& p, FrameEvents& ev) {
    if (b.mods.cls.mage.leyline <= 0.f) return;
    auto& rs = classWorld_.mage.runes;
    const float R = cfg::style::runeRadius * arenaScale();
    const float dmg = ballDamage(b, p) * b.mods.cls.mage.leyline;
    const Element el = hitElement(b, p);
    for (const MageWorld::Rune& r : rs) {
        if (r.owner != b.owner) continue;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - r.pos) > R + e.radius) continue;
            damageEnemy(e, dmg);
            applyElement(e, el, b.owner, dmg, p, ev);
        }
        ev.bursts.push_back({r.pos, R, el == Element::Plain ? theme::classMage : elementColor(el), nullptr});
    }
    rs.erase(std::remove_if(rs.begin(), rs.end(), [&](const MageWorld::Rune& r) { return r.owner == b.owner; }), rs.end());
}

}  // namespace sb
