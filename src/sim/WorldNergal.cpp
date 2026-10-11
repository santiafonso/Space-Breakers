// Nergal, the Alchemist's god (2026-10-07): plague and pestilence. His
// element is his own - the black plague - and it reacts with every other
// element its own way (nergalReaction). Every hit infects (a rot: the poison
// status); he leaves a trail of swarms that hunt what's near; every infected
// enemy that dies lets out a plague rat. Blood rain (a great cloud over the
// thickest pack: it eats and marks), Rat tide (rats pour out of him), Locust
// swarm (his swarms turn to locusts), Black death (the infected burst when
// they die), Pale horse (a rider gallops across, spreading the plague).

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

namespace {
constexpr int kBossMark = -2;
}  // namespace

// A rot on an enemy: stacking, refreshed.
void World::nergalInfect(Enemy& e, float hit) {
    namespace N = cfg::nergal;
    e.poison = std::max(e.poison, N::rotTime);
    e.poisonDps = std::min(e.poisonDps + hit * N::rotFrac, std::max(e.poisonDps, hit * N::rotMax));
}

void World::nergalRat(int owner, sf::Vector2f at) {
    NergalWorld& nw = classWorld_.nergal;
    if (static_cast<int>(nw.rats.size()) >= cfg::nergal::ratMax) return;
    NergalWorld::Rat r;
    r.pos = at;
    r.vel = rng_.direction() * (cfg::nergal::ratSpeed * arenaScale() * 0.7f);
    r.life = cfg::nergal::ratLife;
    r.owner = owner;
    r.gait = rng_.range(0.f, 6.f);
    nw.rats.push_back(r);
}

namespace {
const Ball* nergalBall(const std::vector<Ball>& balls) {
    for (const Ball& b : balls)
        if (b.hasRole(BallRole::Nergal) && !b.ghost) return &b;
    return nullptr;
}
}  // namespace

// Everything of his leaves the black plague on what it touches - and it reacts with whatever element is there.
void World::nergalPlague(Enemy& e, int owner, float dmg, const WorldParams& p, FrameEvents& ev) {
    if (e.hp <= 0.f) return;
    const int keep = dmgOwner_;
    dmgOwner_ = owner;
    applyElement(e, Element::Plague, owner, dmg, p, ev);
    dmgOwner_ = keep;
}

// Its hit infects.
void World::nergalOnHit(Ball& b, Enemy& e, const WorldParams& p) {
    if (!b.hasRole(BallRole::Nergal) || e.hp <= 0.f) return;
    nergalInfect(e, ballDamage(b, p));
}

// An enemy died: if it was infected, the plague goes on - a rat, and with Black death a burst (it infects, it doesn't chain).
void World::nergalOnDeath(const Enemy& dead, const WorldParams& p, FrameEvents& ev) {
    namespace N = cfg::nergal;
    if (dead.poison <= 0.f) return;
    const Ball* o = nergalBall(balls_);
    if (!o) return;
    for (int i = 0; i < N::ratsPerDeath; ++i) nergalRat(o->owner, dead.pos);
    if (o->mods.cls.nergal.blackDeath > 0.f) {
        const float k = arenaScale();
        const float hit = ballDamage(*o, p);
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || &e == &dead || length(e.pos - dead.pos) > N::deathR * k + e.radius) continue;
            dmgOwner_ = o->owner;
            damageEnemy(e, hit * o->mods.cls.nergal.blackDeath * (e.poison > 0.f ? 0.5f : 1.f));   // (the already sick: half)
            dmgOwner_ = -1;
            nergalInfect(e, hit);
            nergalPlague(e, o->owner, hit, p, ev);
        }
        NergalWorld& nw = classWorld_.nergal;
        if (nw.fx.size() < 80) nw.fx.push_back({0, dead.pos, dead.pos, 0.45f, 0.45f, N::deathR * k});
    }
}

// The black plague meets another element (World::triggerReaction): each its own way.
BurstFx World::nergalReaction(Element other, Enemy& e, float dmg, float rm, const WorldParams& p, FrameEvents& ev) {
    namespace N = cfg::nergal;
    (void)p;
    (void)ev;
    const float k = arenaScale();
    NergalWorld& nw = classWorld_.nergal;
    const Ball* o = nergalBall(balls_);
    const sf::Vector2f at = e.pos;
    const sf::Color black = theme::plagueEdge;   // (the bursts in its pale edge colour: black wouldn't show)
    auto around = [&](float R, auto&& f) {
        for (Enemy& q : enemies_)
            if (q.hp > 0.f && !q.orbiter && length(q.pos - at) < R + q.radius) f(q);
    };
    switch (other) {
        case Element::Fire: {   // a pyre: it burns, and the smoke carries the rot
            const float R = N::pyreR * k * rm;
            around(R, [&](Enemy& q) {
                damageEnemy(q, dmg * N::pyreFrac);
                q.burn = std::max(q.burn, 2.f);
                q.burnDps = std::max(q.burnDps, dmg * 0.3f);
                nergalInfect(q, dmg);
            });
            return {at, R, theme::elemFire, "PYRE"};
        }
        case Element::Ice: {   // cold death: it freezes, and the rot cracks out of them at once
            const float R = N::coldR * k * rm;
            around(R, [&](Enemy& q) {
                q.frozen = std::max(q.frozen, N::coldFreeze);
                damageEnemy(q, dmg * N::coldFrac + q.poisonDps * q.poison);
                q.poison = 0.f;
                q.poisonDps = 0.f;
            });
            return {at, R, theme::elemIce, "COLD DEATH"};
        }
        case Element::Poison: {   // pestilence: the rot doubles and spreads
            const float R = N::pestR * k * rm;
            const float rot = std::max(e.poisonDps, dmg * N::rotMax) * 2.f;
            around(R, [&](Enemy& q) {
                q.poison = std::max(q.poison, N::rotTime);
                q.poisonDps = std::max(q.poisonDps, rot);
            });
            return {at, R, theme::elemPoison, "PESTILENCE"};
        }
        case Element::Electric: {   // fever: black lightning leaps through the infected
            sf::Vector2f from = at;
            std::vector<const Enemy*> hit{&e};
            damageEnemy(e, dmg * N::feverFrac);
            for (int j = 0; j < N::feverJumps; ++j) {
                Enemy* next = nullptr;
                float bd = N::feverRange * k;
                for (Enemy& q : enemies_) {
                    if (q.hp <= 0.f || q.orbiter || q.poison <= 0.f || std::find(hit.begin(), hit.end(), &q) != hit.end()) continue;
                    const float d = length(q.pos - from);
                    if (d < bd) { bd = d; next = &q; }
                }
                if (!next) break;
                damageEnemy(*next, dmg * N::feverFrac);
                if (nw.fx.size() < 80) nw.fx.push_back({3, from, next->pos, 0.3f, 0.3f, 0.f});
                hit.push_back(next);
                from = next->pos;
            }
            return {at, 50.f * k, theme::elemElectric, "FEVER"};
        }
        case Element::Water:   // a miasma: a black cloud settles and lingers, eating and infecting
            if (nw.rains.size() < 12)
                nw.rains.push_back({at, N::miasmaR * k * rm, N::miasmaTime, N::miasmaTime, dmg * N::miasmaFrac, 0.f, o ? o->owner : -1, 1});
            return {at, N::miasmaR * k * rm, black, "MIASMA"};
        case Element::Stone: {   // a mass grave: the ground swallows them a moment, and rats crawl out
            const float R = N::graveR * k * rm;
            around(R, [&](Enemy& q) {
                damageEnemy(q, dmg * N::graveFrac);
                q.stagger = std::max(q.stagger, N::graveStun);
            });
            if (o)
                for (int i = 0; i < N::graveRats; ++i) nergalRat(o->owner, at);
            if (nw.fx.size() < 80) nw.fx.push_back({4, at, at, 0.6f, 0.6f, R});
            return {at, R, theme::elemStone, "MASS GRAVE"};
        }
        default:
            return {at, 60.f * k, black, "PLAGUE"};
    }
}

// The ball: its trail of swarms, its tides, its clouds, its rider.
void World::nergalTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace N = cfg::nergal;
    if (!b.hasRole(BallRole::Nergal) || b.ghost || !waveRunning_) return;
    const float k = arenaScale();
    const NergalMods& m = b.mods.cls.nergal;
    NergalState& st = b.cls.nergal;
    NergalWorld& nw = classWorld_.nergal;
    const float hit = ballDamage(b, p);
    const float loc = m.locusts > 0.f ? m.locusts : 1.f;   // "Locust swarm": more, bigger, harder, longer
    if ((st.swarmT -= dt) <= 0.f) {   // the class: a swarm left behind it, every so often
        st.swarmT = N::swarmEvery / loc;
        if (static_cast<int>(nw.swarms.size()) < static_cast<int>(static_cast<float>(N::swarmMax) * loc)) {
            NergalWorld::Swarm s;
            s.pos = b.pos;
            s.vel = rng_.direction() * (40.f * k);
            s.life = s.maxLife = N::swarmLife * (m.locusts > 0.f ? 1.5f : 1.f);
            s.r = N::swarmR * k * (m.locusts > 0.f ? 1.3f : 1.f);
            s.dps = hit * N::swarmFrac * loc;
            s.owner = b.owner;
            s.seed = rng_.range(0.f, 100.f);
            nw.swarms.push_back(s);
        }
    }
    if (m.tideEvery > 0.f && (st.tideT -= dt) <= 0.f) {
        st.tideT = m.tideEvery;
        for (int i = 0; i < N::tideSize; ++i) nergalRat(b.owner, b.pos);
    }
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
    if (m.rainEvery > 0.f && (st.rainT -= dt) <= 0.f) {   // a great blood cloud over the thickest pack
        st.rainT = m.rainEvery;
        if (const Enemy* best = thickest(N::rainR * k); best && nw.rains.size() < 12)
            nw.rains.push_back({best->pos, N::rainR * k, N::rainTime, N::rainTime, hit * N::rainFrac, 0.f, b.owner, 0});
    }
    if (m.horseEvery > 0.f && (st.horseT -= dt) <= 0.f) {   // the pale rider: edge to edge, through the thickest pack
        if (const Enemy* c = thickest(150.f * k)) {   // (no pack yet: it waits for one)
            st.horseT = m.horseEvery;
            const sf::Vector2f dir = rng_.range(0.f, 1.f) < 0.5f ? sf::Vector2f{1.f, 0.f} : sf::Vector2f{-1.f, 0.f};
            const sf::Vector2f slant = normalized(dir + sf::Vector2f{0.f, rng_.range(-0.35f, 0.35f)}, dir);
            NergalWorld::Rider r;
            // from off one side, through it, off the other
            const float back = (dir.x > 0.f ? c->pos.x : size_.x - c->pos.x) + 80.f * k;
            const float fwd = (dir.x > 0.f ? size_.x - c->pos.x : c->pos.x) + 80.f * k;
            r.from = r.pos = c->pos - slant * (back / std::max(0.3f, std::fabs(slant.x)));
            r.to = c->pos + slant * (fwd / std::max(0.3f, std::fabs(slant.x)));
            const float my = 50.f * k;   // (it stays in the arena, top to bottom)
            r.from.y = clampf(r.from.y, my, size_.y - my);
            r.to.y = clampf(r.to.y, my, size_.y - my);
            r.pos = r.from;
            r.dur = length(r.to - r.from) / (N::horseSpeed * k);
            r.dmg = hit * N::horseFrac;
            r.owner = b.owner;
            nw.riders.push_back(r);
            ev.callouts.push_back({c->pos, "PALE HORSE"});
        }
    }
}

void World::updateNergal(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace N = cfg::nergal;
    const float k = arenaScale();
    NergalWorld& nw = classWorld_.nergal;
    for (auto& f : nw.fx) f.life -= dt;
    nw.fx.erase(std::remove_if(nw.fx.begin(), nw.fx.end(), [](const NergalWorld::Fx& f) { return f.life <= 0.f; }), nw.fx.end());
    if (nw.wave != wave_) {
        nw.wave = wave_;
        nw.rats.clear();
        nw.rains.clear();
        nw.swarms.clear();
        nw.riders.clear();
    }
    if (!waveRunning_) return;
    auto ownerBall = [&](int owner) -> const Ball* {
        for (const Ball& b : balls_)
            if (b.owner == owner && !b.ghost) return &b;
        return nullptr;
    };
    for (auto& r : nw.rains) {   // blood rain / miasma: it drifts after the enemies, eating what's under it
        r.life -= dt;
        if (const Enemy* n = nearestEnemy(r.pos, 400.f * k)) r.pos += normalized(n->pos - r.pos, {0.f, 0.f}) * (N::rainDrift * k * dt);
        if ((r.tickT -= dt) > 0.f) continue;
        r.tickT = 0.25f;
        const Ball* o = ownerBall(r.owner);
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - r.pos) > r.r + e.radius) continue;
            dmgOwner_ = r.owner;
            damageEnemy(e, r.dps * 0.25f);
            dmgOwner_ = -1;
            if (r.kind == 0) e.mark = std::max(e.mark, N::rainMark);   // blood: marked
            if (o) {
                nergalInfect(e, ballDamage(*o, p));
                nergalPlague(e, r.owner, r.dps, p, ev);
            }
        }
    }
    nw.rains.erase(std::remove_if(nw.rains.begin(), nw.rains.end(), [](const NergalWorld::Rain& r) { return r.life <= 0.f; }), nw.rains.end());
    for (auto& s : nw.swarms) {   // a swarm: it drifts to the nearest enemy close by and chews what's in it
        s.life -= dt;
        const Ball* o = ownerBall(s.owner);
        if (!o) { s.life = 0.f; continue; }
        if (const Enemy* n = nearestEnemy(s.pos, N::swarmRange * k)) {
            const sf::Vector2f want = normalized(n->pos - s.pos, {0.f, 0.f}) * (N::swarmSpeed * k);
            s.vel += (want - s.vel) * std::min(1.f, dt * 3.f);
        } else {
            s.vel *= std::exp(-1.5f * dt);
        }
        s.pos += s.vel * dt;
        if ((s.tickT -= dt) > 0.f) continue;
        s.tickT = 0.25f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - s.pos) > s.r + e.radius) continue;
            dmgOwner_ = s.owner;
            damageEnemy(e, s.dps * 0.25f);
            dmgOwner_ = -1;
            nergalInfect(e, ballDamage(*o, p));
            nergalPlague(e, s.owner, s.dps, p, ev);
        }
    }
    nw.swarms.erase(std::remove_if(nw.swarms.begin(), nw.swarms.end(), [](const NergalWorld::Swarm& s) { return s.life <= 0.f; }), nw.swarms.end());
    for (auto& r : nw.riders) {   // the pale rider: all it rides through - hit once, and the black plague on them
        const sf::Vector2f prev = r.pos;
        r.t = std::min(1.f, r.t + dt / std::max(0.05f, r.dur));
        r.pos = r.from + (r.to - r.from) * r.t;
        const sf::Vector2f seg = r.pos - prev;
        const float W = N::horseW * k;
        auto near = [&](sf::Vector2f q, float rad) {
            const float u = clampf(dot(q - prev, seg) / std::max(1e-3f, dot(seg, seg)), 0.f, 1.f);
            return length(q - (prev + seg * u)) < W + rad;
        };
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || !near(e.pos, e.radius) || std::find(r.struck.begin(), r.struck.end(), e.id) != r.struck.end()) continue;
            r.struck.push_back(e.id);
            dmgOwner_ = r.owner;
            damageEnemy(e, r.dmg);
            applyElement(e, Element::Plague, r.owner, r.dmg, p, ev);
            dmgOwner_ = -1;
            e.stagger = std::max(e.stagger, 0.4f);
        }
        if (boss_.targetable() && near(boss_.pos, boss_.radius) && std::find(r.struck.begin(), r.struck.end(), kBossMark) == r.struck.end()) {
            r.struck.push_back(kBossMark);
            boss_.hp -= r.dmg * 0.5f;
            boss_.hitFlash = 1.f;
        }
        if (r.t >= 1.f) r.dur = -1.f;
    }
    nw.riders.erase(std::remove_if(nw.riders.begin(), nw.riders.end(), [](const NergalWorld::Rider& r) { return r.dur < 0.f; }), nw.riders.end());
    for (auto& r : nw.rats) {   // rats: a scurry at the nearest enemy, a bite, the plague
        r.life -= dt;
        r.biteT -= dt;
        r.gait += dt * 16.f;
        const Ball* o = ownerBall(r.owner);
        if (!o) { r.life = 0.f; continue; }
        Enemy* t = nullptr;
        float bd = 1e18f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const float d = length(e.pos - r.pos);
            if (d < bd) { bd = d; t = &e; }
        }
        if (!t) { r.vel *= std::exp(-3.f * dt); r.pos += r.vel * dt; continue; }
        const sf::Vector2f dir = normalized(t->pos - r.pos, {1.f, 0.f});
        const float wob = std::sin(r.gait * 0.4f) * 0.5f;
        const sf::Vector2f want = sf::Vector2f{dir.x - dir.y * wob, dir.y + dir.x * wob} * (N::ratSpeed * k);
        r.vel += (want - r.vel) * std::min(1.f, dt * 8.f);
        if (bd > t->radius + 3.f) r.pos += r.vel * dt;
        if (bd > t->radius + 6.f * k || r.biteT > 0.f) continue;
        r.biteT = N::ratBite;
        const float hit = ballDamage(*o, p);
        dmgOwner_ = r.owner;
        damageEnemy(*t, hit * N::ratFrac * (o->mods.cls.nergal.tideEvery > 0.f ? N::tideBite : 1.f));
        dmgOwner_ = -1;
        nergalInfect(*t, hit);
        nergalPlague(*t, r.owner, hit, p, ev);
    }
    nw.rats.erase(std::remove_if(nw.rats.begin(), nw.rats.end(), [](const NergalWorld::Rat& r) { return r.life <= 0.f; }), nw.rats.end());
}

}  // namespace sb
