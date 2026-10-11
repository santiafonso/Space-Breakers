// Ares, the Shooter's god (2026-10-07). A ring of ghostly bronze cannons turns
// round it and fires at everything without pause; every kill feeds its
// bloodlust (the whole arsenal fires faster). Every item is one more weapon:
// Phobos & Deimos (two moons firing homing missiles that strike terror),
// Dragon's teeth (bronze hoplites sprout and fire volleys), War chariot (fire-
// breathing horses charge across, a furrow of fire behind), Wrath of Olympus
// (a red star falls on the thickest pack) and Trojan war (a rain of bronze
// spears down the busiest lane).

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

// Bloodlust: a kill feeds it.
void World::aresOnKill(Ball& b) {
    namespace A = cfg::ares;
    AresState& st = b.cls.ares;
    st.lust = std::min(A::lustMax, st.lust + A::lustPerKill);
    st.lustT = A::lustHold;
}

// The enemy nearest `from` in reach, skipping the first `skip` nearest (so the cannons spread their fire).
const Enemy* World::aresTarget(sf::Vector2f from, float range, int skip) const {
    const Enemy* pick[3] = {nullptr, nullptr, nullptr};
    float d2s[3] = {range * range, range * range, range * range};
    for (const Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const float d2 = dot(e.pos - from, e.pos - from);
        for (int i = 0; i < 3; ++i)
            if (d2 < d2s[i]) {
                for (int j = 2; j > i; --j) { d2s[j] = d2s[j - 1]; pick[j] = pick[j - 1]; }
                d2s[i] = d2;
                pick[i] = &e;
                break;
            }
    }
    int n = 0;
    while (n < 3 && pick[n]) ++n;
    return n == 0 ? nullptr : pick[skip % n];
}

// The weapons on its ball: the cannons, bloodlust, and the timers of every item's weapon.
void World::aresTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::ares;
    if (!b.hasRole(BallRole::Ares) || b.ghost) return;
    AresState& st = b.cls.ares;
    for (float& kk : st.kick) kk = std::max(0.f, kk - dt * 6.f);
    if (!waveRunning_) return;
    const float k = arenaScale();
    const AresMods& m = b.mods.cls.ares;
    AresWorld& aw = classWorld_.ares;
    if ((st.lustT -= dt) <= 0.f) st.lust = std::max(0.f, st.lust - A::lustFade * dt);
    const float rate = 1.f + st.lust;   // bloodlust: everything fires faster
    const float hit = ballDamage(b, p);
    if (const Enemy* t = aresTarget(b.pos, A::range * k, 0)) {   // the cannons turn toward the fight
        const float want = std::atan2(t->pos.y - b.pos.y, t->pos.x - b.pos.x);
        st.aim += clampf(std::remainder(want - st.aim, 2.f * kPi), -8.f * dt, 8.f * dt);
    }

    // The class: the ghost cannons, one after another.
    if ((st.fireT -= dt * rate) <= 0.f) {
        st.fireT += A::fireEvery;
        if (st.fireT < 0.f) st.fireT = 0.f;
        const int c = st.cannon;
        st.cannon = (st.cannon + 1) % A::cannons;
        const float an = static_cast<float>(c) * 2.f * kPi / A::cannons + b.age * 0.8f;
        const sf::Vector2f at = b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (b.radius + 18.f * k);
        if (const Enemy* t = aresTarget(at, A::range * k, c)) {
            aresShot(b, at, normalized(t->pos - at, {1.f, 0.f}), hit * A::bulletFrac, p);
            st.kick[c] = 1.f;
        } else if (boss_.targetable() && length(boss_.pos - at) < A::range * k + boss_.radius) {
            aresShot(b, at, normalized(boss_.pos - at, {1.f, 0.f}), hit * A::bulletFrac, p);
            st.kick[c] = 1.f;
        }
    }

    // Phobos & Deimos: each moon fires a missile at what comes near.
    if (m.moons > 0.f)
        for (int i = 0; i < 2; ++i) {
            if ((st.moonT[i] -= dt * rate) > 0.f) continue;
            const float an = b.age * 1.6f + static_cast<float>(i) * kPi;
            const sf::Vector2f mp = b.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (b.radius + 46.f * k);
            const Enemy* t = aresTarget(mp, A::range * k, i);
            if (!t || aw.missiles.size() >= 40) { st.moonT[i] = 0.2f; continue; }
            st.moonT[i] = A::moonEvery;
            AresWorld::Missile ms;
            ms.pos = mp;
            ms.vel = sf::Vector2f{std::cos(an + kPi * 0.5f), std::sin(an + kPi * 0.5f)} * (A::missileSpeed * k * 0.6f);   // launched off its orbit
            ms.dmg = hit * A::missileFrac * m.moons;
            ms.life = 3.f;
            ms.owner = b.owner;
            ms.target = t->id;
            ms.moon = i;
            aw.missiles.push_back(ms);
        }

    // Dragon's teeth: sown in a row across its path; hoplites spring up.
    if (m.teeth > 0.f && (st.teethT -= dt * rate) <= 0.f) {
        st.teethT = A::teethEvery;
        const sf::Vector2f fwd = normalized(b.vel, {1.f, 0.f}), side{-fwd.y, fwd.x};
        for (int i = 0; i < A::teeth; ++i) {
            if (static_cast<int>(aw.hoplites.size()) >= A::hopliteMax) aw.hoplites.erase(aw.hoplites.begin());   // the oldest falls
            AresWorld::Hoplite h;
            h.pos = b.pos + side * ((static_cast<float>(i) - 1.f) * 34.f * k) - fwd * (30.f * k);
            h.pos.x = clampf(h.pos.x, 30.f * k, size_.x - 30.f * k);
            h.pos.y = clampf(h.pos.y, 30.f * k, size_.y - 30.f * k);
            h.life = h.maxLife = A::hopliteLife;
            h.sprout = A::sprout;
            h.fireT = A::sprout + 0.15f * static_cast<float>(i);
            h.dmg = hit * A::volleyFrac * m.teeth;
            h.owner = b.owner;
            aw.hoplites.push_back(h);
            if (aw.fx.size() < 80) aw.fx.push_back({4, h.pos, h.pos, 0.5f, 0.5f, 20.f * k});
        }
    }

    // The thickest pack (for the chariot and the star).
    auto thickest = [&](float R) -> const Enemy* {
        const Enemy* best = nullptr;
        int most = 0;
        for (const Enemy& a : enemies_) {
            if (a.hp <= 0.f || a.orbiter) continue;
            int c = 0;
            for (const Enemy& o : enemies_)
                if (o.hp > 0.f && !o.orbiter && length(o.pos - a.pos) < R) ++c;
            if (c > most) { most = c; best = &a; }
        }
        return best;
    };

    // War chariot: edge to edge through the thickest pack.
    if (m.chariot > 0.f && (st.chariotT -= dt * rate) <= 0.f) {
        if (const Enemy* c = thickest(150.f * k)) {
            st.chariotT = A::chariotEvery;
            const sf::Vector2f dir = rng_.range(0.f, 1.f) < 0.5f ? sf::Vector2f{1.f, 0.f} : sf::Vector2f{-1.f, 0.f};
            const sf::Vector2f slant = normalized(dir + sf::Vector2f{0.f, rng_.range(-0.35f, 0.35f)}, dir);
            AresWorld::Chariot ch;
            const float back = (dir.x > 0.f ? c->pos.x : size_.x - c->pos.x) + 90.f * k;
            const float fwd = (dir.x > 0.f ? size_.x - c->pos.x : c->pos.x) + 90.f * k;
            ch.from = c->pos - slant * (back / std::max(0.3f, std::fabs(slant.x)));
            ch.to = c->pos + slant * (fwd / std::max(0.3f, std::fabs(slant.x)));
            const float my = 50.f * k;
            ch.from.y = clampf(ch.from.y, my, size_.y - my);
            ch.to.y = clampf(ch.to.y, my, size_.y - my);
            ch.pos = ch.from;
            ch.dur = length(ch.to - ch.from) / (A::chariotSpeed * k);
            ch.dmg = hit * A::chariotFrac * m.chariot;
            ch.burn = hit * A::flameFrac * m.chariot;
            ch.owner = b.owner;
            aw.chariots.push_back(ch);
            ev.callouts.push_back({c->pos, "WAR CHARIOT"});
        }
    }

    // Wrath of Olympus: a red star on the thickest pack.
    // (on its own clock - bloodlust doesn't hurry it - and never more than wrathMax stars and craters at once)
    if (m.wrath > 0.f && (st.wrathT -= dt) <= 0.f) {
        sf::Vector2f at;
        bool have = false;
        if (const Enemy* c = thickest(A::wrathR * k)) { at = c->pos; have = true; }
        else if (boss_.targetable()) { at = boss_.pos; have = true; }
        const int out = static_cast<int>(aw.stars.size()) +
                        static_cast<int>(std::count_if(aw.flames.begin(), aw.flames.end(), [](const AresWorld::Flame& f) { return f.crater; }));
        if (have && out < A::wrathMax) {
            st.wrathT = A::wrathEvery;
            aw.stars.push_back({at, 0.f, A::wrathR * k, hit * A::wrathFrac * m.wrath, b.owner});
        } else {
            st.wrathT = 0.3f;
        }
    }

    // Trojan war: the busiest lane, across or down, gets a rain of spears.
    if (m.troy > 0.f && (st.troyT -= dt * rate) <= 0.f) {
        const float band = A::troyBand * k;
        int best = 0;
        bool across = true;
        float at = 0.f;
        for (const Enemy& a : enemies_) {
            if (a.hp <= 0.f || a.orbiter) continue;
            int row = 0, col = 0;
            for (const Enemy& o : enemies_) {
                if (o.hp <= 0.f || o.orbiter) continue;
                row += std::fabs(o.pos.y - a.pos.y) < band * 0.5f ? 1 : 0;
                col += std::fabs(o.pos.x - a.pos.x) < band * 0.5f ? 1 : 0;
            }
            if (row > best) { best = row; across = true; at = a.pos.y; }
            if (col > best) { best = col; across = false; at = a.pos.x; }
        }
        if (best > 0) {
            st.troyT = A::troyEvery;
            AresWorld::Lane ln;
            const bool flip = rng_.range(0.f, 1.f) < 0.5f;
            const float m0 = 40.f * k;
            if (across) {
                ln.from = {flip ? size_.x - m0 : m0, at};
                ln.to = {flip ? m0 : size_.x - m0, at};
            } else {
                ln.from = {at, flip ? size_.y - m0 : m0};
                ln.to = {at, flip ? m0 : size_.y - m0};
            }
            ln.dmg = hit * A::troyFrac * m.troy;
            ln.owner = b.owner;
            aw.lanes.push_back(ln);
            ev.callouts.push_back({(ln.from + ln.to) * 0.5f, "TROJAN WAR"});
        } else {
            st.troyT = 0.5f;
        }
    }
}

// Everything it has let loose: missiles, hoplites, chariots and their fire, stars, spears.
void World::updateAres(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::ares;
    const float k = arenaScale();
    AresWorld& aw = classWorld_.ares;
    for (auto& f : aw.fx) f.life -= dt;
    aw.fx.erase(std::remove_if(aw.fx.begin(), aw.fx.end(), [](const AresWorld::Fx& f) { return f.life <= 0.f; }), aw.fx.end());
    if (aw.wave != wave_) {
        aw.wave = wave_;
        aw.missiles.clear();
        aw.hoplites.clear();
        aw.chariots.clear();
        aw.flames.clear();
        aw.stars.clear();
        aw.lanes.clear();
    }
    if (!waveRunning_) return;
    auto ownerBall = [&](int owner) -> Ball* {
        for (Ball& b : balls_)
            if (b.owner == owner && !b.ghost) return &b;
        return nullptr;
    };
    auto fx = [&](int kind, sf::Vector2f a, sf::Vector2f b, float life, float r) {
        if (aw.fx.size() < 120) aw.fx.push_back({kind, a, b, life, life, r});
    };
    // a blast: everything within r takes dmg (the boss half), staggered `stun` s
    auto blast = [&](sf::Vector2f at, float r, float dmg, int owner, float stun) {
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - at) > r + e.radius) continue;
            dmgOwner_ = owner;
            damageEnemy(e, dmg);
            dmgOwner_ = -1;
            if (stun > 0.f) e.stagger = std::max(e.stagger, stun);
        }
        if (boss_.targetable() && length(boss_.pos - at) < r + boss_.radius) {
            boss_.hp -= dmg * 0.5f;
            boss_.hitFlash = 1.f;
        }
    };

    // Phobos & Deimos: homing missiles; a blast and terror.
    for (auto& ms : aw.missiles) {
        ms.life -= dt;
        const Enemy* t = nullptr;
        for (const Enemy& e : enemies_)
            if (e.id == ms.target && e.hp > 0.f) { t = &e; break; }
        if (!t) {
            t = aresTarget(ms.pos, A::range * k, 0);
            ms.target = t ? t->id : -1;
        }
        const float sp = A::missileSpeed * k;
        float cur = std::atan2(ms.vel.y, ms.vel.x);
        if (t) {
            const float want = std::atan2(t->pos.y - ms.pos.y, t->pos.x - ms.pos.x);
            cur += clampf(std::remainder(want - cur, 2.f * kPi), -A::missileTurn * dt, A::missileTurn * dt);
        }
        const float v = std::min(sp, length(ms.vel) + sp * 2.f * dt);   // it picks up speed
        ms.vel = sf::Vector2f{std::cos(cur), std::sin(cur)} * v;
        ms.pos += ms.vel * dt;
        bool boom = ms.life <= 0.f || ms.pos.x < 0.f || ms.pos.y < 0.f || ms.pos.x > size_.x || ms.pos.y > size_.y;
        if (t && length(t->pos - ms.pos) < t->radius + 8.f * k) boom = true;
        if (boom) {
            blast(ms.pos, A::missileR * k, ms.dmg, ms.owner, A::fear);
            fx(0, ms.pos, ms.pos, 0.35f, A::missileR * k);
            ms.life = -1.f;
        }
    }
    aw.missiles.erase(std::remove_if(aw.missiles.begin(), aw.missiles.end(), [](const AresWorld::Missile& m) { return m.life < 0.f; }),
                      aw.missiles.end());

    // Dragon's teeth: the hoplites fire volleys at the nearest.
    for (auto& h : aw.hoplites) {
        h.life -= dt;
        h.sprout = std::max(0.f, h.sprout - dt);
        if (h.sprout > 0.f) continue;
        Ball* o = ownerBall(h.owner);
        const float rate = o ? 1.f + o->cls.ares.lust : 1.f;
        const Enemy* t = aresTarget(h.pos, A::range * k, 0);
        if (t) h.aim = std::atan2(t->pos.y - h.pos.y, t->pos.x - h.pos.x);
        if ((h.fireT -= dt * rate) > 0.f || !t || !o) continue;
        h.fireT = A::volleyEvery;
        for (int i = 0; i < A::volleyBullets; ++i) {
            const float a = h.aim + (static_cast<float>(i) - 1.f) * 0.12f;
            aresShot(*o, h.pos, {std::cos(a), std::sin(a)}, h.dmg, p);
        }
        fx(1, h.pos + sf::Vector2f{std::cos(h.aim), std::sin(h.aim)} * (12.f * k), h.pos, 0.12f, 10.f * k);
    }
    aw.hoplites.erase(std::remove_if(aw.hoplites.begin(), aw.hoplites.end(), [](const AresWorld::Hoplite& h) { return h.life <= 0.f; }),
                      aw.hoplites.end());

    // War chariot: it runs everything down and leaves fire behind.
    for (auto& ch : aw.chariots) {
        ch.t += dt / std::max(0.1f, ch.dur);
        ch.pos = ch.from + (ch.to - ch.from) * std::min(1.f, ch.t);
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - ch.pos) > 42.f * k + e.radius) continue;
            if (std::find(ch.struck.begin(), ch.struck.end(), e.id) != ch.struck.end()) continue;
            ch.struck.push_back(e.id);
            dmgOwner_ = ch.owner;
            damageEnemy(e, ch.dmg);
            applyElement(e, Element::Fire, ch.owner, ch.dmg, p, ev);
            dmgOwner_ = -1;
            e.stagger = std::max(e.stagger, 0.5f);
            e.vel += normalized(e.pos - ch.pos, {0.f, 1.f}) * (260.f * k);
        }
        if (boss_.targetable() && length(boss_.pos - ch.pos) < boss_.radius + 42.f * k &&
            std::find(ch.struck.begin(), ch.struck.end(), -7) == ch.struck.end()) {
            ch.struck.push_back(-7);
            boss_.hp -= ch.dmg * 0.5f;
            boss_.hitFlash = 1.f;
        }
        if ((ch.flameT -= dt) <= 0.f && aw.flames.size() < 140 && ch.pos.x > 0.f && ch.pos.x < size_.x) {
            ch.flameT = A::flameEvery;
            aw.flames.push_back({ch.pos, A::flameR * k, A::flameLife, A::flameLife, ch.burn, 0.f, ch.owner});
        }
    }
    aw.chariots.erase(std::remove_if(aw.chariots.begin(), aw.chariots.end(), [](const AresWorld::Chariot& c) { return c.t >= 1.f; }),
                      aw.chariots.end());

    // Fire on the ground: it burns what stands in it.
    for (auto& f : aw.flames) {
        f.life -= dt;
        if ((f.tickT -= dt) > 0.f) continue;
        f.tickT = 0.25f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - f.pos) > f.r + e.radius) continue;
            dmgOwner_ = f.owner;
            damageEnemy(e, f.dps * 0.25f);
            applyElement(e, Element::Fire, f.owner, f.dps * 0.25f, p, ev);
            dmgOwner_ = -1;
        }
    }
    aw.flames.erase(std::remove_if(aw.flames.begin(), aw.flames.end(), [](const AresWorld::Flame& f) { return f.life <= 0.f; }),
                    aw.flames.end());

    // Wrath of Olympus: the ring closes, then the star strikes.
    for (auto& s : aw.stars) {
        s.t += dt;
        if (s.t < A::wrathWarn) continue;
        blast(s.pos, s.r, s.dmg, s.owner, 0.8f);
        for (Enemy& e : enemies_)
            if (e.hp > 0.f && !e.orbiter && length(e.pos - s.pos) < s.r + e.radius)
                e.vel += normalized(e.pos - s.pos, {0.f, 1.f}) * (300.f * k);
        if (aw.flames.size() < 140)
            aw.flames.push_back({s.pos, s.r * 0.6f, A::craterLife, A::craterLife, s.dmg / A::wrathFrac * A::craterFrac, 0.f, s.owner, true});
        fx(2, s.pos, s.pos, 0.7f, s.r);
        ev.kick = std::max(ev.kick, 6.f);
        s.t = -1.f;
    }
    aw.stars.erase(std::remove_if(aw.stars.begin(), aw.stars.end(), [](const AresWorld::Star& s) { return s.t < 0.f; }), aw.stars.end());

    // Trojan war: spears land one after another, end to end.
    for (auto& ln : aw.lanes) {
        ln.t += dt;
        const int due = std::min(A::troySpears, static_cast<int>(ln.t / A::troyTime * static_cast<float>(A::troySpears)) + 1);
        while (ln.fired < due) {
            const float u = (static_cast<float>(ln.fired) + 0.5f) / static_cast<float>(A::troySpears);
            sf::Vector2f at = ln.from + (ln.to - ln.from) * u;
            const sf::Vector2f side = normalized(ln.to - ln.from, {1.f, 0.f});
            at += sf::Vector2f{-side.y, side.x} * rng_.range(-30.f * k, 30.f * k);
            blast(at, A::troyR * k, ln.dmg, ln.owner, 0.3f);
            fx(3, at, at, 0.5f, A::troyR * k);
            ++ln.fired;
        }
        if (ln.fired >= A::troySpears) ln.t = -1.f;
    }
    aw.lanes.erase(std::remove_if(aw.lanes.begin(), aw.lanes.end(), [](const AresWorld::Lane& l) { return l.t < 0.f; }), aw.lanes.end());
}

}  // namespace sb
