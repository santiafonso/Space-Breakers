// Artemisa, the Slinger's god (2026-10-07): the huntress. Every throw - yours,
// or her own when you leave her be (every 4 s she throws herself at the
// enemies, as if you had) - looses a silver arrow down its line that marks the
// first enemy on it as her prey; the ball's first hit on it lands x2. Her items
// all fire on the throw or the catch: Silver hounds (two hounds race to the
// prey), Crescent moon (two copies fly the same path), Arrow rain (a catch
// rains arrows where the throw hit), Golden stag (a full-pull throw: a stag
// charges down the line), Night of the hunt (every enemy prey; a throw runs
// them all through).

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

void World::artemisaOnThrow(Ball& b, float pull) {
    if (!b.hasRole(BallRole::Artemisa) || b.ghost) return;
    ArtemisaState& st = b.cls.artemisa;
    if (!st.hits.empty()) st.rainPending = true;   // (thrown again without a catch: the rain still comes)
    st.throwPending = true;
    st.pull = std::min(1.f, pull);
    st.throwFrom = b.pos;
    st.throwDir = normalized(b.vel, {1.f, 0.f});
    st.autoT = cfg::artemisa::autoEvery;
    st.armed = true;
}

void World::artemisaOnGrab(Ball& b) {
    if (!b.hasRole(BallRole::Artemisa) || b.ghost) return;
    if (!b.cls.artemisa.hits.empty()) b.cls.artemisa.rainPending = true;
}

// Her prey takes the ball's first hit x2; and every hit is remembered for Arrow rain.
float World::artemisaPreHit(Ball& b, const Enemy& e) {
    if (!b.hasRole(BallRole::Artemisa) || b.ghost) return 1.f;
    ArtemisaState& st = b.cls.artemisa;
    if (b.mods.cls.artemisa.rain > 0.f && static_cast<int>(st.hits.size()) < cfg::artemisa::rainMax) st.hits.push_back(e.pos);
    if (st.armed && e.id == st.prey) {
        st.armed = false;
        return cfg::artemisa::preyMul;
    }
    return 1.f;
}

void World::artemisaTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::artemisa;
    if (!b.hasRole(BallRole::Artemisa) || b.ghost) return;
    ArtemisaState& st = b.cls.artemisa;
    const ArtemisaMods& m = b.mods.cls.artemisa;
    ArtemisaWorld& aw = classWorld_.artemisa;
    const float k = arenaScale();
    if (!waveRunning_) { st.route.clear(); return; }

    // Night of the hunt: every so often night falls.
    if (m.night > 0.f && (st.nightT -= dt) <= 0.f) {
        st.nightT = A::nightEvery;
        aw.night = A::nightTime + (m.night - 1.f) * 5.f;   // (a level: a second longer)
        ev.callouts.push_back({b.pos, "NIGHT OF THE HUNT"});
    }

    // A night run: at full speed through every enemy, one after another.
    if (!st.route.empty()) {
        float step = A::nightSpeed * k * dt;
        while (step > 0.f && st.routeAt < static_cast<int>(st.route.size())) {
            Enemy* t = nullptr;
            for (Enemy& e : enemies_)
                if (e.id == st.route[static_cast<std::size_t>(st.routeAt)] && e.hp > 0.f) { t = &e; break; }
            if (!t) { ++st.routeAt; continue; }
            const sf::Vector2f to = t->pos - b.pos;
            const float d = length(to);
            if (d > step) {
                b.vel = to / d * (A::nightSpeed * k);
                b.pos += to / d * step;
                step = 0.f;
            } else {
                b.pos = t->pos;
                step -= d;
                dmgOwner_ = b.owner;
                damageEnemy(*t, st.routeDmg);
                dmgOwner_ = -1;
                t->stagger = std::max(t->stagger, 0.4f);
                if (aw.fx.size() < 80) aw.fx.push_back({2, t->pos, t->pos, 0.3f, 0.3f, t->radius + 10.f * k});
                ++st.routeAt;
            }
        }
        if (st.routeAt >= static_cast<int>(st.route.size())) {   // through them all: off she flies
            st.route.clear();
            b.vel = normalized(b.vel, {1.f, 0.f}) * (ballCruise(b, p) * 1.5f);
        }
        b.pos.x = clampf(b.pos.x, b.radius, size_.x - b.radius);
        b.pos.y = clampf(b.pos.y, b.radius, size_.y - b.radius);
        return;
    }

    // Left alone, she throws herself - as if you had.
    if ((st.autoT -= dt) <= 0.f) {
        std::optional<sf::Vector2f> at = focusPos();
        if (!at) at = nearestTarget(b.pos);
        if (at) {
            const sf::Vector2f dir = normalized(*at - b.pos, {1.f, 0.f});
            b.regrabT = cfg::app::regrabDelay;
            launchThrow(b, dir * (cfg::app::slingMaxSpeed * k * A::autoPull), 0.f, 0.f);
            st.pull = 1.f;   // (her own throws count as full pulls: the stag comes)
        } else {
            st.autoT = 0.5f;
        }
    }
}

void World::updateArtemisa(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::artemisa;
    const float k = arenaScale();
    ArtemisaWorld& aw = classWorld_.artemisa;
    for (auto& f : aw.fx) f.life -= dt;
    aw.fx.erase(std::remove_if(aw.fx.begin(), aw.fx.end(), [](const ArtemisaWorld::Fx& f) { return f.life <= 0.f; }), aw.fx.end());
    for (auto& a : aw.arrows) a.life -= dt;
    aw.arrows.erase(std::remove_if(aw.arrows.begin(), aw.arrows.end(), [](const ArtemisaWorld::Arrow& a) { return a.life <= 0.f; }),
                    aw.arrows.end());
    if (aw.wave != wave_) {
        aw.wave = wave_;
        aw.hounds.clear();
        aw.echoes.clear();
        aw.rains.clear();
        aw.stags.clear();
        aw.night = 0.f;
    }
    if (!waveRunning_) { aw.night = 0.f; return; }
    aw.night = std::max(0.f, aw.night - dt);
    if (aw.night > 0.f)   // the night: every enemy is prey
        for (Enemy& e : enemies_)
            if (e.hp > 0.f && !e.orbiter) e.mark = std::max(e.mark, 0.2f);

    auto blast = [&](sf::Vector2f at, float r, float dmg, int owner) {
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - at) > r + e.radius) continue;
            dmgOwner_ = owner;
            damageEnemy(e, dmg);
            dmgOwner_ = -1;
        }
        if (boss_.targetable() && length(boss_.pos - at) < r + boss_.radius) {
            boss_.hp -= dmg * 0.5f;
            boss_.hitFlash = 1.f;
        }
    };

    // What her throws and catches set off.
    for (Ball& b : balls_) {
        if (!b.hasRole(BallRole::Artemisa) || b.ghost) continue;
        ArtemisaState& st = b.cls.artemisa;
        const ArtemisaMods& m = b.mods.cls.artemisa;
        const float hit = ballDamage(b, p);
        if (st.rainPending) {   // Arrow rain: wherever the last throw hit
            st.rainPending = false;
            if (m.rain > 0.f)
                for (std::size_t i = 0; i < st.hits.size(); ++i)
                    aw.rains.push_back({st.hits[i], 0.f, A::rainGap * static_cast<float>(i) + A::rainFall, hit * A::rainFrac * m.rain, b.owner});
            st.hits.clear();
        }
        if (!st.throwPending) continue;
        st.throwPending = false;
        const sf::Vector2f from = st.throwFrom, dir = st.throwDir;
        // the silver arrow: down the whole line to the wall; the first it meets is her prey
        float len = 1e9f;
        if (dir.x > 1e-4f) len = std::min(len, (size_.x - from.x) / dir.x);
        if (dir.x < -1e-4f) len = std::min(len, -from.x / dir.x);
        if (dir.y > 1e-4f) len = std::min(len, (size_.y - from.y) / dir.y);
        if (dir.y < -1e-4f) len = std::min(len, -from.y / dir.y);
        len = clampf(len, 0.f, 5000.f);
        Enemy* first = nullptr;
        float firstAlong = 1e9f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const float along = dot(e.pos - from, dir);
            if (along < 0.f || along > len || length(e.pos - (from + dir * along)) > A::arrowWidth * k + e.radius) continue;
            dmgOwner_ = b.owner;
            damageEnemy(e, hit * A::arrowFrac);
            dmgOwner_ = -1;
            if (e.hp > 0.f && along < firstAlong) { firstAlong = along; first = &e; }
        }
        if (boss_.targetable()) {
            const float along = dot(boss_.pos - from, dir);
            if (along > 0.f && along < len && length(boss_.pos - (from + dir * along)) < A::arrowWidth * k + boss_.radius) {
                boss_.hp -= hit * A::arrowFrac * 0.5f;
                boss_.hitFlash = 1.f;
            }
        }
        st.prey = first ? first->id : -1;
        if (first) first->mark = std::max(first->mark, A::preyTime);
        if (aw.arrows.size() < 20) aw.arrows.push_back({from, from + dir * len, 0.35f, 0.35f});
        if (m.hounds > 0.f)   // Silver hounds: two, racing alongside
            for (int i = 0; i < 2; ++i) {
                ArtemisaWorld::Hound h;
                h.pos = from + sf::Vector2f{-dir.y, dir.x} * ((i ? 1.f : -1.f) * 16.f * k);
                h.vel = dir * (A::houndSpeed * k);
                h.life = A::houndLife;
                h.dmg = hit * A::houndFrac * m.hounds;
                h.owner = b.owner;
                h.target = st.prey;
                h.gait = static_cast<float>(i) * 1.7f;
                if (aw.hounds.size() < 12) aw.hounds.push_back(h);
            }
        if (m.moon > 0.f)   // Crescent moon: two more on the very same path
            for (int i = 0; i < 2; ++i) aw.echoes.push_back({from, b.vel, A::echoDelay * static_cast<float>(i + 1), b.owner});
        if (m.stag > 0.f && st.pull >= A::stagPull && aw.stags.size() < 4) {   // Golden stag: a full pull
            ArtemisaWorld::Stag s;
            s.pos = from;
            s.dir = dir;
            s.dmg = hit * A::stagFrac * m.stag;
            s.owner = b.owner;
            aw.stags.push_back(s);
        }
        if (m.night > 0.f && aw.night > 0.f) {   // a night throw: through every enemy on the screen
            st.route.clear();
            st.routeAt = 0;
            std::vector<const Enemy*> left;
            for (const Enemy& e : enemies_)
                if (e.hp > 0.f && !e.orbiter) left.push_back(&e);
            sf::Vector2f at = b.pos;
            while (!left.empty() && static_cast<int>(st.route.size()) < A::nightMax) {   // the nearest next, each time
                auto it = std::min_element(left.begin(), left.end(), [&](const Enemy* x, const Enemy* y) {
                    return dot(x->pos - at, x->pos - at) < dot(y->pos - at, y->pos - at);
                });
                st.route.push_back((*it)->id);
                at = (*it)->pos;
                left.erase(it);
            }
            st.routeDmg = hit * A::nightFrac * m.night;
            if (!st.route.empty()) ev.kick = std::max(ev.kick, 4.f);
        }
    }

    // Hounds: to the prey (else the nearest), biting all on the way.
    for (auto& h : aw.hounds) {
        h.life -= dt;
        h.gait += dt * 16.f;
        const Enemy* t = nullptr;
        for (const Enemy& e : enemies_)
            if (e.id == h.target && e.hp > 0.f) { t = &e; break; }
        if (!t) {
            t = nearestEnemy(h.pos, 700.f * k);
            h.target = t ? t->id : -1;
        }
        const float sp = A::houndSpeed * k;
        if (t) h.vel += (normalized(t->pos - h.pos, {1.f, 0.f}) * sp - h.vel) * std::min(1.f, dt * 6.f);
        h.pos += h.vel * dt;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - h.pos) > A::houndBite * k + e.radius) continue;
            if (std::find(h.struck.begin(), h.struck.end(), e.id) != h.struck.end()) continue;
            h.struck.push_back(e.id);
            dmgOwner_ = h.owner;
            damageEnemy(e, h.dmg);
            dmgOwner_ = -1;
            if (aw.fx.size() < 80) aw.fx.push_back({0, h.pos, e.pos, 0.25f, 0.25f, e.radius});
            if (e.id == h.target) h.struck.clear();   // its prey: it worries it again
        }
        if (h.pos.x < -60.f || h.pos.y < -60.f || h.pos.x > size_.x + 60.f || h.pos.y > size_.y + 60.f) h.life = 0.f;
    }
    aw.hounds.erase(std::remove_if(aw.hounds.begin(), aw.hounds.end(), [](const ArtemisaWorld::Hound& h) { return h.life <= 0.f; }),
                    aw.hounds.end());

    // Crescent moon: the copies set off down the path, a moment behind.
    for (auto& e : aw.echoes) {
        if ((e.delay -= dt) > 0.f) continue;
        const Ball* o = nullptr;
        for (const Ball& b : balls_)
            if (b.owner == e.owner && !b.ghost) o = &b;
        if (o && static_cast<int>(ghosts_.size() + pendingGhosts_.size()) < cfg::synergy::maxGhosts) {
            Ball g = *o;
            g.ghost = true;
            g.twin = false;
            g.held = false;
            g.classPulse = 0.f;
            g.ghostLife = A::echoLife * o->mods.cls.artemisa.moon;
            g.pos = e.pos;
            g.vel = e.vel;
            g.trail.clear();
            g.waterTrail.clear();
            g.cls.artemisa = ArtemisaState{};
            pendingGhosts_.push_back(g);
        }
        e.delay = -1.f;
    }
    aw.echoes.erase(std::remove_if(aw.echoes.begin(), aw.echoes.end(), [](const ArtemisaWorld::Echo& e) { return e.delay < 0.f; }),
                    aw.echoes.end());

    // Arrow rain: each volley lands, in turn.
    for (auto& r : aw.rains) {
        r.t += dt;
        if (r.t < r.land) continue;
        blast(r.pos, A::rainR * k, r.dmg, r.owner);
        if (aw.fx.size() < 80) aw.fx.push_back({1, r.pos, r.pos, 0.4f, 0.4f, A::rainR * k});
        r.t = -1.f;
    }
    aw.rains.erase(std::remove_if(aw.rains.begin(), aw.rains.end(), [](const ArtemisaWorld::Rain& r) { return r.t < 0.f; }), aw.rains.end());

    // The golden stag: down the line to the wall; all it touches is prey.
    for (auto& s : aw.stags) {
        s.life += dt;
        s.pos += s.dir * (A::stagSpeed * k * dt);
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - s.pos) > A::stagW * k + e.radius) continue;
            if (std::find(s.struck.begin(), s.struck.end(), e.id) != s.struck.end()) continue;
            s.struck.push_back(e.id);
            dmgOwner_ = s.owner;
            damageEnemy(e, s.dmg);
            dmgOwner_ = -1;
            e.mark = std::max(e.mark, A::preyTime);
            e.stagger = std::max(e.stagger, 0.5f);
            e.vel += normalized(e.pos - s.pos, s.dir) * (240.f * k);
        }
        if (boss_.targetable() && length(boss_.pos - s.pos) < A::stagW * k + boss_.radius &&
            std::find(s.struck.begin(), s.struck.end(), -7) == s.struck.end()) {
            s.struck.push_back(-7);
            boss_.hp -= s.dmg * 0.5f;
            boss_.hitFlash = 1.f;
        }
    }
    aw.stags.erase(std::remove_if(aw.stags.begin(), aw.stags.end(), [&](const ArtemisaWorld::Stag& s) {
                       return s.pos.x < -80.f || s.pos.y < -80.f || s.pos.x > size_.x + 80.f || s.pos.y > size_.y + 80.f || s.life > 4.f;
                   }),
                   aw.stags.end());
}

}  // namespace sb
