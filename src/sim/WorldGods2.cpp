// The gods, part two (2026-10-06): Thanatos (the Assassin's) and Tiamat (the
// Summoner's). See WorldGods.cpp for Hermes and Odin, WorldPlanets.cpp for Atlas.
//
// Thanatos - one mark at a time. Its dagger flies to an enemy and it blinks to
// it (out the far side, the way the dagger went) and keeps stabbing it till it dies, then blinks to the
// next. Every kill gathers a soul: harder hits for the fight, and the next
// blink at once. Shadow step (blinks cut the way), Death mark (the weak just
// die), Eclipse (every 4th blink a nova), Reaper's scythe (each blink sweeps
// the 2 nearest), Final hour (every so often, through every enemy in a breath).
//
// Tiamat - she rides a dragon that breathes fire ahead of her; every kill
// hatches a lizard. Her items: a frost wyrm coiled round the core, a storm
// drake perched in the field, a golden dragon that crashes down on the
// strongest, lizard broods, and the brood mother.

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

namespace {
constexpr int kBossMark = -2;
}  // namespace

// ---------------------------------------------------------------- Thanatos

// Blink onto a mark: behind it, the cut and the sweep and the nova on the way.
void World::thanatosBlink(Ball& b, int target, const WorldParams& p, FrameEvents& ev) {
    namespace T = cfg::thanatos;
    const float k = arenaScale();
    ThanatosState& st = b.cls.thanatos;
    const ThanatosMods& m = b.mods.cls.thanatos;
    sf::Vector2f at;
    float tr = 0.f;
    if (target == kBossMark) {
        if (!boss_.targetable()) return;
        at = boss_.pos;
        tr = boss_.radius;
    } else {
        const Enemy* e = nullptr;
        for (const Enemy& o : enemies_)
            if (o.id == target && o.hp > 0.f) e = &o;
        if (!e) return;
        at = e->pos;
        tr = e->radius;
    }
    const sf::Vector2f from = b.pos;
    const sf::Vector2f away = normalized(at - from, rng_.direction());   // out the far side, the way its dagger went
    st.side = away;
    b.pos = at + away * (tr + b.radius + 2.f);
    b.pos.x = clampf(b.pos.x, b.radius, size_.x - b.radius);
    b.pos.y = clampf(b.pos.y, b.radius, size_.y - b.radius);
    b.trail.clear();
    st.target = target;
    st.stabT = 0.f;
    ++st.blinks;
    auto& fx = classWorld_.thanatos.fx;
    if (fx.size() < 80) fx.push_back({0, from, b.pos, 0.35f, 0.35f, b.radius});
    const float hit = ballDamage(b, p);
    if (m.shadowStep > 0.f) {   // the way there is cut
        const sf::Vector2f ab = b.pos - from;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || e.id == target) continue;
            const float t = clampf(dot(e.pos - from, ab) / std::max(1e-3f, dot(ab, ab)), 0.f, 1.f);
            if (length(e.pos - (from + ab * t)) > e.radius + T::cutWidth * k) continue;
            dmgOwner_ = b.owner;
            damageEnemy(e, hit * m.shadowStep);
            dmgOwner_ = -1;
        }
        if (fx.size() < 80) fx.push_back({3, from, b.pos, 0.3f, 0.3f, T::cutWidth * k});
    }
    if (m.scythe > 0) {   // a sweep: the nearest ones to its mark too
        std::vector<Enemy*> near;
        for (Enemy& e : enemies_)
            if (e.hp > 0.f && !e.orbiter && e.id != target && length(e.pos - at) < 220.f * k) near.push_back(&e);
        std::sort(near.begin(), near.end(), [&](const Enemy* x, const Enemy* y) { return length(x->pos - at) < length(y->pos - at); });
        for (int i = 0; i < std::min(m.scythe, static_cast<int>(near.size())); ++i) {
            Enemy& e = *near[static_cast<std::size_t>(i)];
            strike(b, e, normalized(b.pos - e.pos, {1.f, 0.f}), p, ev);
            if (fx.size() < 80) fx.push_back({1, b.pos, e.pos, 0.3f, 0.3f, e.radius});
        }
    }
    if (m.eclipse > 0 && st.blinks % m.eclipse == 0) {   // a shadow nova where it lands
        const float R = T::eclipseR * k;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - b.pos) > R + e.radius) continue;
            dmgOwner_ = b.owner;
            damageEnemy(e, hit * m.eclipseFrac);
            dmgOwner_ = -1;
            e.stagger = std::max(e.stagger, 0.4f);
        }
        if (fx.size() < 80) fx.push_back({2, b.pos, b.pos, 0.5f, 0.5f, R});
    }
}

// Its next mark: the player's target, else the nearest enemy, else the boss.
int World::thanatosPick(const Ball& b) const {
    if (const Enemy* f = focusEnemy()) return f->id;
    if (focusIsBoss() && boss_.targetable()) return kBossMark;
    const Enemy* best = nullptr;
    float bd = 1e18f;
    for (const Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const float d = length(e.pos - b.pos);
        if (d < bd) { bd = d; best = &e; }
    }
    if (best) return best->id;
    return boss_.targetable() ? kBossMark : -1;
}

void World::thanatosTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace T = cfg::thanatos;
    ThanatosState& st = b.cls.thanatos;
    const ThanatosMods& m = b.mods.cls.thanatos;
    st.attached = false;
    if (!b.hasRole(BallRole::Thanatos) || b.held || b.ghost || !waveRunning_) {
        st.target = -1;
        st.chain.clear();
        st.chainPos.clear();
        st.hop = -1;
        return;
    }
    // Final hour: through every enemy in a breath.
    if (m.finalHour > 0.f && st.hourLeft <= 0 && (st.hourT -= dt) <= 0.f) {
        st.hourT = m.finalHour;
        st.hourLeft = T::hourBlinks;
    }
    if (st.hourLeft > 0) {
        if ((st.stabT -= dt) <= 0.f) {
            --st.hourLeft;
            const int t = thanatosPick(b);
            if (t == -1) { st.hourLeft = 0; return; }
            thanatosBlink(b, t, p, ev);
            st.stabT = T::hourStep;
            if (t == kBossMark) ballHitsBoss(b, 0.3f, p, ev, false);
            else
                for (Enemy& e : enemies_)
                    if (e.id == t && e.hp > 0.f) strike(b, e, normalized(b.pos - e.pos, {1.f, 0.f}), p, ev);
            st.target = -1;   // each one once: the next blink picks again
        }
        st.attached = true;
        return;
    }
    // Is its mark still there?
    Enemy* mark = nullptr;
    if (st.target >= 0)
        for (Enemy& e : enemies_)
            if (e.id == st.target && e.hp > 0.f) mark = &e;
    const bool onBoss = st.target == kBossMark && boss_.targetable() && enemies_.empty();
    st.blinkCd = std::max(0.f, st.blinkCd - dt);
    st.sinceBlink += dt;
    if (!mark && !onBoss) {   // dead (or none yet): its dagger flies, bounces 3 times, and it blinks along every one it hit
        st.target = -1;
        const float k = arenaScale();
        auto alive = [&](int id, sf::Vector2f& at) {   // (the boss or an enemy, still there: where)
            if (id == kBossMark) { at = boss_.pos; return boss_.targetable(); }
            for (const Enemy& e : enemies_)
                if (e.id == id && e.hp > 0.f) { at = e.pos; return true; }
            return false;
        };
        if (st.hop >= 0) {   // blinking through the chain, node by node, a stab on each
            st.attached = true;
            if ((st.hopT -= dt) > 0.f) return;
            while (st.hop < static_cast<int>(st.chain.size())) {
                const int id = st.chain[static_cast<std::size_t>(st.hop)];
                ++st.hop;
                sf::Vector2f at;
                if (!alive(id, at)) continue;
                thanatosBlink(b, id, p, ev);
                if (id == kBossMark) ballHitsBoss(b, 0.3f, p, ev, false);
                for (Enemy& e : enemies_)
                    if (e.id == id && e.hp > 0.f) strike(b, e, normalized(b.pos - e.pos, {1.f, 0.f}), p, ev);
                st.hopT = T::hopStep;
                if (st.hop < static_cast<int>(st.chain.size())) return;
                break;
            }
            st.hop = -1;   // the last node: its mark now (if it's still standing)
            st.chain.clear();
            st.blinkCd = T::blinkCd;
            st.sinceBlink = 0.f;
            for (Enemy& e : enemies_)
                if (e.id == st.target && e.hp > 0.f) mark = &e;
            if (!mark && !(st.target == kBossMark && boss_.targetable())) { st.target = -1; return; }
        } else if (!st.chain.empty()) {   // the dagger's in flight: it stands still, watching it go
            st.attached = true;
            const int leg = static_cast<int>(st.chain.size()) - 1;
            sf::Vector2f to = st.chainPos[static_cast<std::size_t>(leg)];
            const bool there = alive(st.chain[static_cast<std::size_t>(leg)], to);
            st.chainPos[static_cast<std::size_t>(leg)] = to;
            st.throwT += dt;
            const float u = std::min(1.f, st.throwT / st.throwDur);
            st.daggerPos = st.daggerFrom + (to - st.daggerFrom) * u;
            b.vel = normalized(st.daggerPos - b.pos, {1.f, 0.f}) * std::max(length(b.vel), ballCruise(b, p));
            if (u < 1.f) return;
            if (there) {   // it lands: a cut, and it bounces on to the nearest one it hasn't hit
                const float hit = ballDamage(b, p) * T::daggerFrac;
                if (st.chain[static_cast<std::size_t>(leg)] == kBossMark) { boss_.hp -= hit * 0.5f; boss_.hitFlash = 1.f; }
                for (Enemy& e : enemies_)
                    if (e.id == st.chain[static_cast<std::size_t>(leg)] && e.hp > 0.f) {
                        dmgOwner_ = b.owner;
                        damageEnemy(e, hit);
                        dmgOwner_ = -1;
                    }
                auto& fx = classWorld_.thanatos.fx;
                if (fx.size() < 80) fx.push_back({1, st.daggerFrom, to, 0.2f, 0.2f, 12.f * k});
            }
            // how far a point is off a leg of its flight
            const auto offLine = [](sf::Vector2f q, sf::Vector2f a, sf::Vector2f c) {
                const sf::Vector2f ab = c - a;
                const float t = clampf(dot(q - a, ab) / std::max(1.f, dot(ab, ab)), 0.f, 1.f);
                return length(q - (a + ab * t));
            };
            {   // everything it flew through on the way takes a lighter cut
                const float pass = ballDamage(b, p) * T::passFrac;
                const int target = st.chain[static_cast<std::size_t>(leg)];
                for (Enemy& e : enemies_) {
                    if (e.hp <= 0.f || e.orbiter || e.id == target) continue;
                    if (offLine(e.pos, st.daggerFrom, to) > T::daggerWidth * k + e.radius) continue;
                    dmgOwner_ = b.owner;
                    damageEnemy(e, pass);
                    dmgOwner_ = -1;
                }
            }
            // it bounces on to a far one it hasn't hit, the way that cuts through the most on the way
            const Enemy* next = nullptr;
            if (leg < T::daggerBounces) {
                float best = -1.f;
                const float range = T::bounceRange * k;
                for (const Enemy& e : enemies_) {
                    if (e.hp <= 0.f || e.orbiter || std::find(st.chain.begin(), st.chain.end(), e.id) != st.chain.end()) continue;
                    const float dd = length(e.pos - to);
                    if (dd > range) continue;
                    int through = 0;
                    for (const Enemy& o : enemies_)
                        if (&o != &e && o.hp > 0.f && !o.orbiter && offLine(o.pos, to, e.pos) <= T::daggerWidth * k + o.radius) ++through;
                    const float score = static_cast<float>(through) + dd / range;   // most cut through; then the farthest
                    if (score > best) { best = score; next = &e; }
                }
            }
            if (next) {
                st.chain.push_back(next->id);
                st.chainPos.push_back(next->pos);
                st.daggerFrom = to;
                st.throwT = 0.f;
                st.throwDur = clampf(length(next->pos - to) / (T::daggerSpeed * k), T::daggerMin, T::daggerMax);
            } else {
                st.hop = 0;   // done bouncing: now the blinks
                st.hopT = 0.f;
            }
            return;
        } else {
            if (st.blinkCd > 0.f || st.sinceBlink < T::blinkMin) return;   // flying free till then
            const int t = thanatosPick(b);
            if (t == -1) return;
            sf::Vector2f to = boss_.pos;
            alive(t, to);
            st.chain = {t};   // the dagger first
            st.chainPos = {to};
            st.throwT = 0.f;
            st.throwDur = clampf(length(to - b.pos) / (T::daggerSpeed * k), T::daggerMin, T::daggerMax);
            st.daggerFrom = st.daggerPos = b.pos;
            st.attached = true;
            return;
        }
    }
    // Stay on it, behind it, and stab.
    st.attached = true;
    const sf::Vector2f at = mark ? mark->pos : boss_.pos;
    const float tr = mark ? mark->radius : boss_.radius;
    if (length(st.side) < 0.5f) st.side = normalized(at - b.pos, {1.f, 0.f});
    const sf::Vector2f away = st.side;   // (the side it blinked in on)
    b.pos = at + away * (tr + b.radius + 2.f);
    b.pos.x = clampf(b.pos.x, b.radius, size_.x - b.radius);
    b.pos.y = clampf(b.pos.y, b.radius, size_.y - b.radius);
    b.vel = -away * std::max(length(b.vel), ballCruise(b, p));   // (it faces its mark: a throw from here goes through it)
    if ((st.stabT -= dt) > 0.f) return;
    st.stabT = T::stabEvery;
    auto& fx = classWorld_.thanatos.fx;
    if (mark) {
        if (m.deathMark > 0.f && mark->hp < mark->maxHp * m.deathMark) {   // the weak just die
            dmgOwner_ = b.owner;
            damageEnemy(*mark, mark->hp + 1.f);
            dmgOwner_ = -1;
            if (fx.size() < 80) fx.push_back({2, mark->pos, mark->pos, 0.35f, 0.35f, mark->radius * 2.f});
        } else {
            strike(b, *mark, normalized(b.pos - mark->pos, {1.f, 0.f}), p, ev);
        }
        if (fx.size() < 80) fx.push_back({1, b.pos, at, 0.15f, 0.15f, tr});
    } else {
        ballHitsBoss(b, 0.3f, p, ev, false);
        if (fx.size() < 80) fx.push_back({1, b.pos, at, 0.15f, 0.15f, tr});
    }
}

// ---------------------------------------------------------------- Tiamat

void World::tiamatTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace T = cfg::tiamat;
    if (!b.hasRole(BallRole::Tiamat) || b.held || b.ghost || !waveRunning_) return;
    const float k = arenaScale();
    TiamatState& st = b.cls.tiamat;
    TiamatWorld& tw = classWorld_.tiamat;
    // The mount grows all run (every death feeds it): a hatchling - fireballs and claws; young - its
    // wake burns and it has moves of its own; adult - all its moves, sooner. Bigger and harder all the way.
    const float mxp = b.owner >= 0 && b.owner < TiamatWorld::kMounts ? tw.mountXp[b.owner] : 0.f;
    const int mstage = 1 + (mxp >= T::stageXp[1] ? 1 : 0) + (mxp >= T::stageXp[2] ? 1 : 0);
    const float mpow = T::mountPow + (1.f - T::mountPow) * clampf(mxp / T::stageXp[2], 0.f, 1.f);
    if (mstage != st.stage) {
        if (st.stage > 0) ev.callouts.push_back({b.pos, mstage == 3 ? "ADULT" : "YOUNG"});   // (not on the first frame)
        st.stage = mstage;
    }
    // The mount: fireballs lobbed at the thickest pack (the player's target first)...
    auto pack = [&](float range) -> const Enemy* {
        if (const Enemy* f = focusEnemy()) return f;
        const Enemy* best = nullptr;
        int most = 0;
        for (const Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - b.pos) > range) continue;
            int c = 0;
            for (const Enemy& o : enemies_)
                if (o.hp > 0.f && length(o.pos - e.pos) < T::fireR * k) ++c;
            if (c > most) { most = c; best = &e; }
        }
        return best;
    };
    auto lob = [&](sf::Vector2f to, float dur) {
        if (tw.fireballs.size() >= 40) return;
        TiamatWorld::Fireball f;
        f.from = f.pos = b.pos + normalized(b.vel, {1.f, 0.f}) * (b.radius * 1.7f);   // out of the mount's mouth
        f.to = to;
        f.dur = dur;
        f.dmg = ballDamage(b, p) * T::fireFrac * mpow;
        f.r = T::fireR * k * (0.7f + 0.3f * mpow);
        f.owner = b.owner;
        tw.fireballs.push_back(f);
    };
    if ((st.breathT -= dt) <= 0.f) {
        st.breathT = T::fireEvery;
        if (const Enemy* t = pack(T::fireRange * k)) lob(t->pos, T::fireFlight);
        else if (boss_.targetable()) lob(boss_.pos, T::fireFlight);
    }
    // Between its moves it breathes fire: a cone at the nearest within reach (or the boss).
    if ((st.flameT -= dt) <= 0.f) {
        const float R = T::breathRange * k * (0.7f + 0.3f * mpow);
        const Enemy* t = nearestEnemy(b.pos, R);
        const bool atBoss = !t && boss_.targetable() && length(boss_.pos - b.pos) < R + boss_.radius;
        if (t || atBoss) {
            st.flameT = T::breathEvery;
            const sf::Vector2f dir = normalized((t ? t->pos : boss_.pos) - b.pos, {1.f, 0.f});
            const sf::Vector2f mouth = b.pos + dir * (b.radius * 1.2f);
            const float dmg = ballDamage(b, p) * T::breathFrac * mpow, cosC = std::cos(T::breathCone);
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                const sf::Vector2f v = e.pos - mouth;
                const float l = length(v);
                if (l > R + e.radius || (l > 1.f && dot(v / l, dir) < cosC)) continue;
                dmgOwner_ = b.owner;
                damageEnemy(e, dmg);
                applyElement(e, Element::Fire, b.owner, dmg, p, ev);
                dmgOwner_ = -1;
            }
            if (boss_.targetable() && length(boss_.pos - mouth) < R + boss_.radius &&
                dot(normalized(boss_.pos - mouth, dir), dir) >= cosC - 0.3f) {
                boss_.hp -= dmg * 0.5f;
                boss_.hitFlash = 1.f;
            }
            if (tw.fx.size() < 60) tw.fx.push_back({0, mouth, mouth + dir * R, 0.4f, 0.4f, R, {}});
        } else {
            st.flameT = 0.2f;
        }
    }
    // Always at it: claws rake what's close, and its wake burns.
    if ((st.clawT -= dt) <= 0.f) {
        st.clawT = T::clawEvery;
        const float R = b.radius * T::clawReach;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - b.pos) > R + e.radius) continue;
            dmgOwner_ = b.owner;
            damageEnemy(e, ballDamage(b, p) * T::clawFrac * mpow);
            dmgOwner_ = -1;
            if (tw.fx.size() < 60) tw.fx.push_back({8, b.pos, e.pos, 0.25f, 0.25f, e.radius, {}});
        }
    }
    if (mstage >= 2 && (st.emberT -= dt) <= 0.f && length(b.vel) > 1.f) {
        st.emberT = T::emberEvery;
        if (tw.fields.size() < 160)
            tw.fields.push_back({0, b.pos - normalized(b.vel, {1.f, 0.f}) * (b.radius * 1.5f), T::emberR * k, T::emberLife, T::emberLife,
                                 ballDamage(b, p) * T::emberFrac * mpow, 0.f, b.owner});
    }
    // ...and every so often a move of its own, named over the field.
    if (mstage >= 2 && (st.skillT -= dt) <= 0.f) {   // (young: roar or dive; adult: inferno and meteors too, sooner)
        st.skillT = mstage >= 3 ? T::mountSkillAdult : T::skillEvery;
        const int move = rng_.irange(0, mstage >= 3 ? 3 : 1);
        const float hit = ballDamage(b, p) * mpow;
        switch (move) {
            case 0: {   // roar: everything near is stunned
                const float R = T::roarR * k;
                for (Enemy& e : enemies_)
                    if (e.hp > 0.f && !e.orbiter && length(e.pos - b.pos) < R + e.radius) e.stagger = std::max(e.stagger, T::roarStun);
                tw.fx.push_back({4, b.pos, b.pos, 0.7f, 0.7f, R, {}});
                ev.callouts.push_back({b.pos, "ROAR"});
                ev.kick = std::max(ev.kick, 5.f);
                break;
            }
            case 1: {   // dive: it swoops onto the thickest pack and crashes into it
                const Enemy* t = pack(1e9f);
                if (!t) break;
                const sf::Vector2f from = b.pos;
                b.pos = {clampf(t->pos.x, b.radius, size_.x - b.radius), clampf(t->pos.y, b.radius, size_.y - b.radius)};
                b.trail.clear();
                const float R = T::diveR * k;
                for (Enemy& e : enemies_) {
                    if (e.hp <= 0.f || e.orbiter || length(e.pos - b.pos) > R + e.radius) continue;
                    dmgOwner_ = b.owner;
                    damageEnemy(e, hit * T::diveFrac);
                    dmgOwner_ = -1;
                    e.vel += normalized(e.pos - b.pos, {1.f, 0.f}) * (350.f * k * e.knockTaken);
                    e.stagger = std::max(e.stagger, 0.6f);
                }
                tw.fx.push_back({9, from, b.pos, 0.5f, 0.5f, R, {}});
                ev.callouts.push_back({b.pos, "DIVE"});
                ev.kick = std::max(ev.kick, 6.f);
                break;
            }
            case 2:   // inferno: a ring of fire rides with it a while
                tw.fields.push_back({3, b.pos, T::infernoR * k, T::infernoTime, T::infernoTime, hit * T::infernoFrac, 0.f, b.owner});
                ev.callouts.push_back({b.pos, "INFERNO"});
                break;
            default: {   // meteors: called down on the packs, one after another
                std::vector<const Enemy*> es;
                for (const Enemy& e : enemies_)
                    if (e.hp > 0.f && !e.orbiter) es.push_back(&e);
                for (int i = 0; i < T::meteors && !es.empty(); ++i) {
                    const Enemy* e = es[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(es.size()) - 1))];
                    if (tw.fireballs.size() >= 40) break;
                    TiamatWorld::Fireball f;
                    f.to = e->pos;
                    f.from = f.pos = e->pos + sf::Vector2f{-160.f * k, -520.f * k};   // from high up, slanting in
                    f.dur = 0.6f + 0.35f * static_cast<float>(i);
                    f.dmg = hit * T::meteorFrac;
                    f.r = T::meteorR * k;
                    f.owner = b.owner;
                    tw.fireballs.push_back(f);
                }
                ev.callouts.push_back({b.pos, "METEORS"});
                break;
            }
        }
    }
    if (b.mods.cls.tiamat.horde > 0.f && (st.hordeT -= dt) <= 0.f) {   // a brood of lizards pours out
        st.hordeT = b.mods.cls.tiamat.horde;
        for (int i = 0; i < T::hordeSize; ++i) tiamatHatch(b.owner, b.pos);
    }
}

void World::tiamatHatch(int owner, sf::Vector2f at) {
    namespace T = cfg::tiamat;
    TiamatWorld& tw = classWorld_.tiamat;
    const Ball* o = nullptr;
    for (const Ball& b : balls_)
        if (b.owner == owner) o = &b;
    if (!o) return;
    const int cap = o->mods.cls.tiamat.brood ? T::broodMax : T::lizardMax;
    if (static_cast<int>(tw.lizards.size()) >= cap) return;
    TiamatWorld::Lizard l;
    l.pos = at;
    l.vel = rng_.direction() * (T::lizardSpeed * arenaScale() * 0.6f);
    l.life = T::lizardLife * (o->mods.cls.tiamat.brood ? 1.5f : 1.f);
    l.owner = owner;
    l.gait = rng_.range(0.f, 6.f);
    tw.lizards.push_back(l);
}

void World::updateGods2(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace T = cfg::tiamat;
    const float k = arenaScale();
    ThanatosWorld& thw = classWorld_.thanatos;
    TiamatWorld& tw = classWorld_.tiamat;
    for (auto& f : thw.fx) f.life -= dt;
    thw.fx.erase(std::remove_if(thw.fx.begin(), thw.fx.end(), [](const ThanatosWorld::Streak& s) { return s.life <= 0.f; }), thw.fx.end());
    for (auto& f : tw.fx) f.life -= dt;
    tw.fx.erase(std::remove_if(tw.fx.begin(), tw.fx.end(), [](const TiamatWorld::Fx& f) { return f.life <= 0.f; }), tw.fx.end());

    // The dragons: rebuilt when what the balls carry changes, and each fight.
    unsigned sig = 2166136261u;
    auto mix = [&sig](unsigned v) { sig = (sig ^ v) * 16777619u; };
    for (const Ball& b : balls_) {
        if (b.ghost) continue;
        const TiamatMods& m = b.mods.cls.tiamat;
        mix(static_cast<unsigned>(b.owner) + 1u);
        mix(static_cast<unsigned>(m.frostWyrm * 100.f));
        mix(static_cast<unsigned>(m.stormDrake * 100.f) * 3u);
        mix(static_cast<unsigned>(m.ancientGold * 100.f) * 7u);
        mix(static_cast<unsigned>(m.devourer * 100.f) * 11u);
        mix(static_cast<unsigned>(m.wyrmling * 100.f) * 13u);
    }
    if (sig != tw.sig || tw.wave != wave_) {
        if (tw.wave != wave_) {
            tw.lizards.clear();
            tw.fx.clear();
            tw.fireballs.clear();
            tw.fields.clear();
            for (Ball& b : balls_) b.cls.thanatos = ThanatosState{};
        }
        tw.sig = sig;
        tw.wave = wave_;
        const std::vector<TiamatWorld::Dragon> old = std::move(tw.dragons);
        tw.dragons.clear();
        for (const Ball& b : balls_) {
            if (b.ghost) continue;
            const TiamatMods& m = b.mods.cls.tiamat;
            if (b.owner >= 0 && b.owner < TiamatWorld::kMounts) tw.mountXp[b.owner] = std::max(tw.mountXp[b.owner], m.xp[5]);
            const float pw[5] = {m.frostWyrm, m.stormDrake, m.ancientGold, m.devourer, m.wyrmling};
            for (int kind = 0; kind < 5; ++kind) {
                if (pw[kind] <= 0.f) continue;
                TiamatWorld::Dragon d;
                d.kind = kind;
                d.owner = b.owner;
                d.power = pw[kind];
                d.xp = m.xp[kind];   // grown all run: what the run kept, or what it grew since
                for (const auto& o : old)
                    if (o.owner == d.owner && o.kind == kind) d.xp = std::max(d.xp, o.xp);
                while (d.stage < 3 && d.xp >= T::stageXp[d.stage]) ++d.stage;
                d.ang = rng_.range(0.f, 2.f * kPi);
                const float r = kind == 0 ? T::wyrmOrbit : rng_.range(110.f, 170.f);   // laid round the core (the wyrm's on its round)
                d.pos = d.perch = core_.pos + sf::Vector2f{std::cos(d.ang), std::sin(d.ang)} * (r * k);
                d.face = {-std::sin(d.ang), std::cos(d.ang)};
                d.actT = 1.f + static_cast<float>(kind);
                tw.dragons.push_back(d);
            }
        }
    }
    if (!waveRunning_) return;
    auto ownerBall = [&](int owner) -> const Ball* {
        for (const Ball& b : balls_)
            if (b.owner == owner) return &b;
        return nullptr;
    };

    auto enemyById = [&](int id) -> Enemy* {
        if (id < 0) return nullptr;
        for (Enemy& e : enemies_)
            if (e.id == id && e.hp > 0.f && !e.orbiter) return &e;
        return nullptr;
    };
    auto thickestPack = [&](float R) -> const Enemy* {   // the enemy with the most others round it
        const Enemy* best = nullptr;
        int most = 0;
        for (const Enemy& a : enemies_) {
            if (a.hp <= 0.f || a.orbiter) continue;
            int c = 0;
            for (const Enemy& b2 : enemies_)
                if (b2.hp > 0.f && !b2.orbiter && length(b2.pos - a.pos) < R) ++c;
            if (c > most) { most = c; best = &a; }
        }
        return best;
    };
    auto rotate = [](sf::Vector2f v, float a) { return sf::Vector2f{v.x * std::cos(a) - v.y * std::sin(a), v.x * std::sin(a) + v.y * std::cos(a)}; };

    // A dragon on the wing: it picks somewhere (often right over an enemy) and carves a wide arc there,
    // turning only so fast. It flies like it has wings: a few beats - each downstroke a push - then a
    // glide on still wings, bleeding speed till it has to beat again; it strains harder in the turns.
    auto fly = [&](TiamatWorld::Dragon& d, float speed) {
        const float m = 60.f * k;
        if (length(d.goal) < 1.f || length(d.goal - d.pos) < 90.f * k || (d.goalT -= dt) <= 0.f) {
            d.goalT = rng_.range(2.5f, 4.5f);
            std::vector<const Enemy*> es;
            for (const Enemy& e : enemies_)
                if (e.hp > 0.f && !e.orbiter) es.push_back(&e);
            if (boss_.targetable() && (es.empty() || rng_.range(0.f, 1.f) < 0.5f)) d.goal = boss_.pos;   // a boss: it goes for it
            else if (!es.empty() && rng_.range(0.f, 1.f) < 0.6f) d.goal = es[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(es.size()) - 1))]->pos;
            else d.goal = {rng_.range(0.1f, 0.9f) * size_.x, rng_.range(0.12f, 0.88f) * size_.y};
        }
        d.goal.x = clampf(d.goal.x, m * 1.5f, size_.x - m * 1.5f);   // (enemies still coming in from outside: not past the walls)
        d.goal.y = clampf(d.goal.y, m * 1.5f, size_.y - m * 1.5f);
        sf::Vector2f h = normalized(d.vel, d.face);
        const sf::Vector2f want = normalized(d.goal - d.pos, h);
        const float maxA = T::flyTurn[d.kind] * dt;
        const float a = clampf(std::atan2(h.x * want.y - h.y * want.x, dot(h, want)), -maxA, maxA);
        h = rotate(h, a);
        const float bank = maxA > 0.f ? std::fabs(a) / maxA : 0.f;   // how hard it's turning (0..1)
        const float top = speed * k;
        float s = length(d.vel);
        if (d.glide > 0.f) {   // gliding: wings out and still, slowly losing way; a hard turn or too slow - it beats again
            d.glide -= dt;
            s *= std::exp(-T::glideDrag * dt);
            if (s > top) s += (top - s) * std::min(1.f, dt * 2.5f);   // (out of a rush: it sheds the extra fast)
            if (bank > 0.8f || s < top * 0.7f) d.glide = 0.f;
        } else {
            const float sz = d.size / T::adultSize[d.kind];
            const float effort = clampf(std::max(bank, (top - s) / std::max(1.f, top)), 0.f, 1.f);
            d.flap += (T::beatRate + T::beatEffort * effort) * (1.5f - 0.5f * sz) * dt;   // (the small ones beat faster)
            const float down = std::max(0.f, std::sin(d.flap));                          // the downstroke pushes
            s += ((top * 1.1f - s) * 1.5f + top * T::beatPush * (down - 0.32f)) * dt;
            if (!d.toGlide && s > top && bank < 0.4f && rng_.range(0.f, 1.f) < dt * 1.5f) d.toGlide = true;
            if (d.toGlide && std::sin(d.flap) > 0.55f && std::cos(d.flap) > 0.f) {   // at the top of the beat: hold them there
                d.toGlide = false;
                d.glide = rng_.range(T::glideMin, T::glideMax);
            }
        }
        d.vel = h * s;
        d.pos += d.vel * dt;
        d.pos.x = clampf(d.pos.x, m, size_.x - m);
        d.pos.y = clampf(d.pos.y, m, size_.y - m);
    };
    auto spit = [&](const TiamatWorld::Dragon& d, sf::Vector2f mouth, sf::Vector2f to, int kind, float dmg, float r, float dur) {
        if (tw.fireballs.size() >= 60) return;
        TiamatWorld::Fireball f;
        f.from = f.pos = mouth;
        f.to = to;
        f.dur = dur;
        f.dmg = dmg;
        f.r = r;
        f.owner = d.owner;
        f.kind = kind;
        tw.fireballs.push_back(f);
    };
    // A dragon's own hit (true: it killed).
    auto hurt = [&](TiamatWorld::Dragon& d, Enemy& e, float dmg, Element el) {
        const bool alive = e.hp > 0.f;
        dmgOwner_ = d.owner;
        damageEnemy(e, dmg);
        applyElement(e, el, d.owner, dmg, p, ev);
        dmgOwner_ = -1;
        return alive && e.hp <= 0.f;
    };
    // The boss: within r of q? And a dragon's hit on it (half, like everything that isn't a ball).
    auto bossNear = [&](sf::Vector2f q, float r) { return boss_.targetable() && length(boss_.pos - q) < r + boss_.radius; };
    auto hurtBoss = [&](float dmg) {
        boss_.hp -= dmg * 0.5f;
        boss_.hitFlash = 1.f;
    };
    for (auto& d : tw.dragons) {
        const Ball* o = ownerBall(d.owner);
        if (!o) continue;
        d.flash = std::max(0.f, d.flash - dt * 2.f);
        // Growing (every death feeds it - World::sweepDeadEnemies): an egg, a hatchling, young, adult.
        int st = 0;
        while (st < 3 && d.xp >= T::stageXp[st]) ++st;
        if (st != d.stage) { d.stage = st; d.flash = 1.f; }
        const float g = clampf((d.xp - T::stageXp[0]) / (T::stageXp[2] - T::stageXp[0]), 0.f, 1.f);
        d.size = T::adultSize[d.kind] * (T::hatchSize + (1.f - T::hatchSize) * g);
        if (d.stage == 0) continue;   // still an egg, where it was laid
        const float sz = d.size / T::adultSize[d.kind];   // (how far grown: 0.35 .. 1)
        const float hit = ballDamage(*o, p) * d.power * T::adultPow[d.kind] * (T::hatchPow + (1.f - T::hatchPow) * g);
        const sf::Vector2f prevPos = d.pos;
        if (d.kind == 0) {   // the frost wyrm: circling the core; what its body touches freezes - young: ice breath; adult: a longer body, frost where it breathes
            d.ang += T::wyrmSpin * dt;
            d.pos = core_.pos + sf::Vector2f{std::cos(d.ang), std::sin(d.ang)} * (T::wyrmOrbit * k);
            if ((d.moveT -= dt) <= 0.f) {   // a new lap: all can be caught again
                d.moveT = 2.f * kPi / T::wyrmSpin;
                d.struck.clear();
            }
            const int body = std::min(d.trailN, T::wyrmBody[d.stage]);
            const float reach = T::coilR * k * sz;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || std::find(d.struck.begin(), d.struck.end(), e.id) != d.struck.end()) continue;
                bool touch = length(e.pos - d.pos) < reach + e.radius;
                for (int i = 0; i < body && !touch; ++i) touch = length(e.pos - d.trail[i]) < reach + e.radius;
                if (!touch) continue;
                d.struck.push_back(e.id);
                hurt(d, e, hit * T::coilFrac, Element::Ice);
                e.frozen = std::max(e.frozen, T::coilFreeze);
            }
            if (std::find(d.struck.begin(), d.struck.end(), kBossMark) == d.struck.end()) {   // its coils round the boss too
                bool touch = bossNear(d.pos, reach);
                for (int i = 0; i < body && !touch; ++i) touch = bossNear(d.trail[i], reach);
                if (touch) { d.struck.push_back(kBossMark); hurtBoss(hit * T::coilFrac); }
            }
            if (d.stage >= 2 && (d.actT -= dt) <= 0.f) {
                d.actT = T::wyrmEvery;
                const Enemy* t = nullptr;   // at the one closest to the core, within its reach
                float bd = 1e18f;
                for (const Enemy& e : enemies_) {
                    if (e.hp <= 0.f || e.orbiter || length(e.pos - d.pos) > T::wyrmRange * k) continue;
                    const float dd = length(e.pos - core_.pos);
                    if (dd < bd) { bd = dd; t = &e; }
                }
                const bool atBoss = !t && bossNear(d.pos, T::wyrmRange * k);
                if (t || atBoss) {
                    const sf::Vector2f dir = normalized((t ? t->pos : boss_.pos) - d.pos, {1.f, 0.f});
                    const sf::Vector2f at = t ? t->pos : boss_.pos;
                    const float R = T::wyrmRange * k, cosC = std::cos(0.45f);
                    if (bossNear(d.pos, R) && dot(normalized(boss_.pos - d.pos, dir), dir) >= cosC) hurtBoss(hit * T::wyrmFrac);
                    for (Enemy& e : enemies_) {
                        if (e.hp <= 0.f || e.orbiter) continue;
                        const sf::Vector2f v = e.pos - d.pos;
                        const float l = length(v);
                        if (l > R + e.radius || (l > 1.f && dot(v / l, dir) < cosC)) continue;
                        hurt(d, e, hit * T::wyrmFrac, Element::Ice);
                        e.frozen = std::max(e.frozen, T::wyrmFreeze);
                    }
                    if (d.stage >= 3 && tw.fields.size() < 160)
                        tw.fields.push_back({1, at, T::patchR * k, T::patchLife, T::patchLife, hit * T::patchFrac, 0.f, d.owner});
                    if (tw.fx.size() < 60) tw.fx.push_back({1, d.pos, d.pos + dir * R, 0.5f, 0.5f, R, {}});
                    d.flash = 1.f;
                }
            }
        } else if (d.kind == 1) {   // the storm drake: static zaps - young: chain lightning; adult: a thunderstorm over the thickest pack
            fly(d, T::drakeSpeed);
            if ((d.trailT -= dt) <= 0.f) {
                d.trailT = T::staticEvery;
                if (Enemy* e = const_cast<Enemy*>(nearestEnemy(d.pos, T::staticRange * k))) {
                    hurt(d, *e, hit * T::staticFrac, Element::Plain);
                    TiamatWorld::Fx f;
                    f.kind = 2;
                    f.life = f.maxLife = 0.15f;
                    f.pts = {d.pos + d.face * (d.size * 0.5f * k), e->pos};
                    if (tw.fx.size() < 60) tw.fx.push_back(std::move(f));
                } else if (bossNear(d.pos, T::staticRange * k)) {
                    hurtBoss(hit * T::staticFrac);
                    TiamatWorld::Fx f;
                    f.kind = 2;
                    f.life = f.maxLife = 0.15f;
                    f.pts = {d.pos + d.face * (d.size * 0.5f * k), boss_.pos};
                    if (tw.fx.size() < 60) tw.fx.push_back(std::move(f));
                }
            }
            if (d.stage >= 3 && (d.moveT -= dt) <= 0.f) {   // adult: storm clouds over the thickest packs, one each
                d.moveT = T::stormEvery;
                std::vector<sf::Vector2f> over;
                for (int c = 0; c < T::stormClouds; ++c) {
                    const Enemy* best = nullptr;
                    int most = 0;
                    for (const Enemy& a : enemies_) {
                        if (a.hp <= 0.f || a.orbiter) continue;
                        bool taken = false;
                        for (const sf::Vector2f& q : over) taken = taken || length(a.pos - q) < T::stormR * 1.5f * k;
                        if (taken) continue;
                        int n = 0;
                        for (const Enemy& b2 : enemies_)
                            if (b2.hp > 0.f && !b2.orbiter && length(b2.pos - a.pos) < T::stormR * k) ++n;
                        if (n > most) { most = n; best = &a; }
                    }
                    if (!best) break;
                    over.push_back(best->pos);
                    tw.fields.push_back({4, best->pos, T::stormR * k, T::stormTime, T::stormTime, hit * T::stormFrac, 0.f, d.owner});
                }
                if (over.empty() && boss_.targetable())   // nothing but the boss: a storm over it
                    for (int c = 0; c < T::stormClouds; ++c) {
                        const sf::Vector2f q = boss_.pos + sf::Vector2f{(c ? 1.f : -1.f) * boss_.radius * 0.5f, 0.f};
                        over.push_back(q);
                        tw.fields.push_back({4, q, T::stormR * k, T::stormTime, T::stormTime, hit * T::stormFrac, 0.f, d.owner});
                    }
                if (!over.empty()) d.flash = 1.f;
            }
            if (d.stage >= 2 && (d.actT -= dt) <= 0.f) {
                d.actT = T::drakeEvery;
                TiamatWorld::Fx f;
                f.kind = 2;
                f.life = f.maxLife = 0.35f;
                const sf::Vector2f mouth = d.pos + d.face * (d.size * 0.5f * k);   // lightning out of its mouth
                f.pts.push_back(mouth);
                sf::Vector2f from = mouth;
                std::vector<int> hit2;
                if (!nearestEnemy(mouth, T::drakeRange * k) && bossNear(mouth, T::drakeRange * k)) {   // only the boss in reach
                    hurtBoss(hit * T::drakeFrac);
                    f.pts.push_back(boss_.pos);
                    from = boss_.pos;
                }
                for (int j = 0; j < T::drakeChain; ++j) {
                    Enemy* best = nullptr;
                    float bd = T::drakeRange * k;
                    for (Enemy& e : enemies_) {
                        if (e.hp <= 0.f || e.orbiter || std::find(hit2.begin(), hit2.end(), e.id) != hit2.end()) continue;
                        const float dd = length(e.pos - from);
                        if (dd < bd) { bd = dd; best = &e; }
                    }
                    if (!best) break;
                    hit2.push_back(best->id);
                    hurt(d, *best, hit * T::drakeFrac, Element::Electric);
                    f.pts.push_back(best->pos);
                    from = best->pos;
                }
                if (f.pts.size() > 1 && tw.fx.size() < 60) { tw.fx.push_back(std::move(f)); d.flash = 1.f; }
            }
        } else if (d.kind == 2) {   // ancient gold: a head a stage - a flamethrower, then molten gold, then a pulling roar; each its own mind
            fly(d, T::goldSpeed);
            d.ang += dt;
            const int heads = d.stage;
            const float S = d.size * k;   // (its drawn body length: render/ClassRender.cpp)
            const float spread = heads == 1 ? 0.f : heads == 2 ? 0.35f : 0.6f;
            const float reach[3] = {T::flameRange * k + S, 700.f * k, T::pullR * k};
            const sf::Vector2f neckBase = d.pos + d.face * (S * 0.22f);
            auto mouthOf = [&](int h) {   // out at the end of its long, bent neck
                const sf::Vector2f hd = d.headDir[h];
                if (heads == 1) return d.pos + d.face * (S * 0.25f) + hd * (S * 0.4f);
                return neckBase + normalized(d.face + hd, hd) * (S * 0.4f) + hd * (S * 0.38f);
            };
            Enemy* tgt[3] = {};
            bool onBoss[3] = {};   // a head with no enemy in reach goes for the boss
            for (int h = 0; h < heads; ++h) {
                const float baseAng = heads == 1 ? 0.f : spread * (2.f * static_cast<float>(h) / static_cast<float>(heads - 1) - 1.f);
                d.headT[h] -= dt;
                if (length(d.headDir[h]) < 0.5f) d.headDir[h] = rotate(d.face, baseAng);
                Enemy* t = enemyById(d.headTgt[h]);
                if (t && length(t->pos - d.pos) > reach[h]) t = nullptr;
                if (!t || (d.headPick[h] -= dt) <= 0.f) {   // a whim: another enemy - one the other heads haven't got, if it can
                    d.headPick[h] = rng_.range(T::headPickMin, T::headPickMax);
                    std::vector<Enemy*> free, any;
                    for (Enemy& e : enemies_) {
                        if (e.hp <= 0.f || e.orbiter || length(e.pos - d.pos) > reach[h]) continue;
                        any.push_back(&e);
                        if (e.id != d.headTgt[(h + 1) % 3] && e.id != d.headTgt[(h + 2) % 3]) free.push_back(&e);
                    }
                    const std::vector<Enemy*>& pool = free.empty() ? any : free;
                    t = pool.empty() ? nullptr : pool[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(pool.size()) - 1))];
                    d.headTgt[h] = t ? t->id : -1;
                }
                tgt[h] = t;
                onBoss[h] = !t && bossNear(d.pos, reach[h]);
                // where it wants to look: its enemy (or the boss), else weaving idly; a neck only turns so far
                sf::Vector2f want = t ? normalized(t->pos - neckBase, d.face) : onBoss[h] ? normalized(boss_.pos - neckBase, d.face)
                                      : rotate(d.face, baseAng + 0.45f * std::sin(d.ang * (0.8f + 0.35f * static_cast<float>(h)) + 2.1f * static_cast<float>(h)));
                const float a = clampf(std::atan2(d.face.x * want.y - d.face.y * want.x, dot(d.face, want)), -T::headSwing, T::headSwing);
                want = rotate(d.face, a);
                d.headDir[h] = normalized(d.headDir[h] + (want - d.headDir[h]) * std::min(1.f, dt * T::headTurn), want);
            }
            // the first head: a flamethrower, burning down the way it looks
            if (d.headT[0] <= 0.f && ((tgt[0] && length(tgt[0]->pos - mouthOf(0)) < T::flameRange * k + tgt[0]->radius) ||
                                      (onBoss[0] && bossNear(mouthOf(0), T::flameRange * k)))) {
                d.headT[0] = T::flameEvery;
                const sf::Vector2f mouth = mouthOf(0), dir = d.headDir[0];
                const float R = T::flameRange * k, cosC = std::cos(0.3f);
                if (bossNear(mouth, R) && dot(normalized(boss_.pos - mouth, dir), dir) >= cosC - 0.3f) hurtBoss(hit * T::flameFrac);
                for (Enemy& e : enemies_) {
                    if (e.hp <= 0.f || e.orbiter) continue;
                    const sf::Vector2f v = e.pos - mouth;
                    const float l = length(v);
                    if (l > R + e.radius || (l > 1.f && dot(v / l, dir) < cosC)) continue;
                    hurt(d, e, hit * T::flameFrac, Element::Fire);
                }
                if (tw.fx.size() < 60) tw.fx.push_back({0, mouth, mouth + dir * R, 0.32f, 0.32f, R, {}});
            }
            // young, a second: globs of molten gold that leave pools
            if (heads >= 2 && d.headT[1] <= 0.f && (tgt[1] || onBoss[1])) {
                d.headT[1] = T::globHeadEvery;
                spit(d, mouthOf(1), tgt[1] ? tgt[1]->pos : boss_.pos, 2, hit * T::globFrac, T::globR * k, 0.7f);
            }
            // adult, a third: a roar that drags everything near in toward the other two
            if (heads >= 3 && d.headT[2] <= 0.f && tgt[2]) {
                d.headT[2] = T::pullEvery;
                const float R = T::pullR * k;
                for (Enemy& e : enemies_) {
                    if (e.hp <= 0.f || e.orbiter || length(e.pos - d.pos) > R + e.radius) continue;
                    e.vel += normalized(d.pos - e.pos, {1.f, 0.f}) * (T::pullSpeed * k * e.knockTaken);
                    e.stagger = std::max(e.stagger, 0.5f);
                }
                if (tw.fx.size() < 60) tw.fx.push_back({10, mouthOf(2), d.pos, 0.6f, 0.6f, R, {}});
                d.flash = 1.f;
            }
        } else if (d.kind == 3) {   // the ravager: swoops through the thickest pack - young: its kills hatch lizards; adult: two runs back to back
            auto charge = [&]() {
                const Enemy* c = thickestPack(150.f * k);
                if (!c && !boss_.targetable()) return false;
                const sf::Vector2f h = normalized(d.vel, d.face);
                const sf::Vector2f cp = c ? c->pos : boss_.pos;   // no pack: the boss
                d.target = c ? c->id : kBossMark;
                d.perch = cp;
                // it veers off the other way first, so it hooks round into them
                d.runSide = h.x * (cp.y - d.pos.y) - h.y * (cp.x - d.pos.x) > 0.f ? -1 : 1;
                d.dive = 0.f;
                d.throughT = -1.f;
                d.glide = 0.f;
                d.struck.clear();
                d.flash = 1.f;
                return true;
            };
            if (d.dive < 0.f) {
                fly(d, T::ravageSpeed);
                if ((d.actT -= dt) <= 0.f) {
                    d.actT = std::max(0.9f, T::ravageEvery + T::ravageEveryPerLevel * (d.power - 1.f) / T::dragonPerLevel);
                    charge();
                }
            } else {   // the run: a veer and a brake (wings back), then it hooks round and rushes in, steering onto the pack
                d.dive += dt;
                sf::Vector2f h = normalized(d.vel, d.face);
                float s = length(d.vel);
                if (d.dive < T::windup) {
                    h = rotate(h, static_cast<float>(d.runSide) * T::runTurn * 0.7f * dt);
                    s += (T::ravageSpeed * k * 0.6f - s) * std::min(1.f, dt * 6.f);
                } else {
                    if (const Enemy* c = enemyById(d.target)) d.perch = c->pos;   // (the pack moves: it follows)
                    else if (d.target == kBossMark && boss_.targetable()) d.perch = boss_.pos;
                    if (d.throughT < 0.f) {
                        const sf::Vector2f to = d.perch - d.pos;
                        if (length(to) < 35.f * k || dot(to, h) < 0.f) d.throughT = d.dive;   // through them: on straight, no more steering
                        else {
                            const sf::Vector2f want = normalized(to, h);
                            const float maxA = T::runTurn * dt * (1.f + 150.f * k / std::max(30.f * k, length(to)));   // (close in: it turns tighter)
                            h = rotate(h, clampf(std::atan2(h.x * want.y - h.y * want.x, dot(h, want)), -maxA, maxA));
                        }
                    }
                    s += (T::ravageRun * k - s) * std::min(1.f, dt * T::runAccel);
                }
                d.vel = h * s;
                d.pos += d.vel * dt;
                d.goal = {};
                const float m = 50.f * k;
                const bool wall = d.pos.x < m || d.pos.y < m || d.pos.x > size_.x - m || d.pos.y > size_.y - m;
                d.pos.x = clampf(d.pos.x, m, size_.x - m);
                d.pos.y = clampf(d.pos.y, m, size_.y - m);
                const float W = T::ravageWidth * k * (0.6f + 0.4f * sz);
                const sf::Vector2f seg = d.pos - prevPos, side{-h.y, h.x};
                auto near = [&](sf::Vector2f q, float r) {   // along the way it came this frame
                    const float t = clampf(dot(q - prevPos, seg) / std::max(1e-3f, dot(seg, seg)), 0.f, 1.f);
                    return length(q - (prevPos + seg * t)) < W + r;
                };
                int hits = 0;
                if (d.dive >= T::windup) {
                    for (Enemy& e : enemies_) {
                        if (e.hp <= 0.f || e.orbiter || !near(e.pos, e.radius)) continue;
                        if (std::find(d.struck.begin(), d.struck.end(), e.id) != d.struck.end()) continue;
                        d.struck.push_back(e.id);
                        const bool killed = hurt(d, e, hit * T::ravageFrac, Element::Plain);
                        e.vel += side * ((dot(e.pos - d.pos, side) < 0.f ? -1.f : 1.f) * 320.f * k * e.knockTaken);
                        e.stagger = std::max(e.stagger, 0.4f);
                        if (killed && d.stage >= 2) tiamatHatch(d.owner, e.pos);
                        ++hits;
                    }
                    if (boss_.targetable() && near(boss_.pos, boss_.radius) &&
                        std::find(d.struck.begin(), d.struck.end(), kBossMark) == d.struck.end()) {
                        d.struck.push_back(kBossMark);
                        boss_.hp -= hit * T::ravageFrac * 0.5f;
                        boss_.hitFlash = 1.f;
                        ++hits;
                    }
                }
                if (hits > 0) ev.kick = std::max(ev.kick, 2.f + static_cast<float>(hits) * 0.5f);
                if (wall || d.dive > T::runMax || (d.throughT >= 0.f && d.dive > d.throughT + 0.3f)) {   // out the far side: it glides out of it
                    d.dive = -1.f;
                    d.glide = 0.7f;
                    if (d.stage >= 3 && d.chain == 0 && charge()) d.chain = 1;   // an adult: straight into another
                    else d.chain = 0;
                }
            }
        } else {   // the wyrmling: darts from enemy to enemy biting the weakest - young: a pounce after each kill; adult: its bites splash
            Enemy* t = enemyById(d.target);
            if (!t) {   // the weakest within reach, else the nearest anywhere
                float lo = 1e18f;
                for (Enemy& e : enemies_)
                    if (e.hp > 0.f && !e.orbiter && length(e.pos - d.pos) < T::wyrmlingRange * k && e.hp < lo) { lo = e.hp; t = &e; }
                if (!t) t = const_cast<Enemy*>(nearestEnemy(d.pos, 1e9f));
                d.target = t ? t->id : -1;
            }
            d.burst = std::max(0.f, d.burst - dt);
            d.flap += (T::beatRate + T::beatEffort) * 1.4f * dt;   // a little thing: always beating, fast
            const float sp = T::wyrmlingSpeed * k * (d.burst > 0.f ? T::pounceSpeed : 1.f);
            const bool atBoss = !t && boss_.targetable();   // nothing else: it goes for the boss
            const sf::Vector2f want = t ? t->pos : atBoss ? boss_.pos : core_.pos + sf::Vector2f{std::cos(d.ang), std::sin(d.ang)} * (120.f * k);
            if (!t && !atBoss) d.ang += dt;
            d.vel += (normalized(want - d.pos, d.face) * sp - d.vel) * std::min(1.f, dt * 8.f);
            d.pos += d.vel * dt;
            d.actT -= dt;
            if (atBoss && d.actT <= 0.f && bossNear(d.pos, d.size * 0.6f * k)) {   // a bite on the boss
                d.actT = T::biteEvery;
                hurtBoss(hit * T::biteFrac);
                if (tw.fx.size() < 60) tw.fx.push_back({8, d.pos, d.pos + normalized(boss_.pos - d.pos, d.face) * (d.size * 0.6f * k), 0.25f, 0.25f, 20.f * k, {}});
                d.flash = std::max(d.flash, 0.6f);
            }
            if (t && d.actT <= 0.f && length(t->pos - d.pos) < t->radius + d.size * 0.6f * k) {   // a bite
                d.actT = T::biteEvery;
                const sf::Vector2f at = t->pos;
                const bool killed = hurt(d, *t, hit * T::biteFrac, Element::Fire);
                if (tw.fx.size() < 60) tw.fx.push_back({8, d.pos, at, 0.25f, 0.25f, t->radius, {}});   // the bite: claw marks, and it lights up
                d.flash = std::max(d.flash, 0.6f);
                if (d.stage >= 3)
                    for (Enemy& e : enemies_)
                        if (&e != t && e.hp > 0.f && !e.orbiter && length(e.pos - at) < T::splashR * k + e.radius)
                            hurt(d, e, hit * T::biteFrac * T::splashFrac, Element::Fire);
                if (killed) {
                    d.target = -1;
                    if (d.stage >= 2) { d.actT = 0.f; d.burst = T::pounceTime; }   // straight on to the next
                }
            }
        }
        const sf::Vector2f step = d.pos - prevPos;
        if (length(step) > 0.5f) d.face = normalized(step, d.face);
        if (d.trailN == 0 || length(d.pos - d.trail[0]) > 9.f * k) {   // a point every few px: a long body, whatever its pace
            for (int i = std::min(d.trailN, 13); i > 0; --i) d.trail[i] = d.trail[i - 1];
            d.trailN = std::min(d.trailN + 1, 14);
        }
        d.trail[0] = d.pos;
    }

    // Lizards: run at the nearest enemy (the player's target first), bite, breed on kills (Brood mother).
    std::vector<std::pair<int, sf::Vector2f>> breed;
    for (auto& l : tw.lizards) {
        l.life -= dt;
        l.biteT -= dt;
        l.gait += dt * 14.f;
        const Ball* o = ownerBall(l.owner);
        if (!o) { l.life = 0.f; continue; }
        Enemy* t = const_cast<Enemy*>(focusEnemy());
        if (!t) {
            float bd = 1e18f;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                const float dd = length(e.pos - l.pos);
                if (dd < bd) { bd = dd; t = &e; }
            }
        }
        if (!t && boss_.targetable()) {   // nothing else: they swarm the boss
            const sf::Vector2f dir = normalized(boss_.pos - l.pos, {1.f, 0.f});
            l.vel += (dir * (T::lizardSpeed * k) - l.vel) * std::min(1.f, dt * 7.f);
            if (length(boss_.pos - l.pos) > boss_.radius + 3.f) l.pos += l.vel * dt;
            if (length(boss_.pos - l.pos) < boss_.radius + 6.f * k && l.biteT <= 0.f) {
                l.biteT = T::lizardBite;
                boss_.hp -= ballDamage(*o, p) * T::lizardFrac * (o->mods.cls.tiamat.brood ? 2.f : 1.f) * 0.5f;
                boss_.hitFlash = 1.f;
            }
            continue;
        }
        if (!t) { l.vel *= std::exp(-3.f * dt); l.pos += l.vel * dt; continue; }
        const sf::Vector2f to = t->pos - l.pos;
        const float dist = length(to);
        const float wob = std::sin(l.gait * 0.5f) * 0.6f;   // a scuttle, not a beeline
        const sf::Vector2f dir = normalized(to, {1.f, 0.f});
        const sf::Vector2f want = sf::Vector2f{dir.x - dir.y * wob, dir.y + dir.x * wob} * (T::lizardSpeed * k);
        l.vel += (want - l.vel) * std::min(1.f, dt * 7.f);
        if (dist > t->radius + 3.f) l.pos += l.vel * dt;
        if (dist > t->radius + 6.f * k || l.biteT > 0.f) continue;
        l.biteT = T::lizardBite;
        const bool alive = t->hp > 0.f;
        const float dmg = ballDamage(*o, p) * T::lizardFrac * (o->mods.cls.tiamat.brood ? 2.f : 1.f);
        dmgOwner_ = l.owner;
        damageEnemy(*t, dmg);
        dmgOwner_ = -1;
        if (alive && t->hp <= 0.f && o->mods.cls.tiamat.brood) breed.push_back({l.owner, t->pos});
    }
    for (const auto& [owner, at] : breed) { tiamatHatch(owner, at); tiamatHatch(owner, at); }
    for (auto& f : tw.fireballs) {   // the mount's fireballs: an arc, then a burst
        f.t += dt / f.dur;
        const float u = std::min(1.f, f.t);
        if (f.kind == 3 && (f.tickT -= dt) <= 0.f) {   // ball lightning: zaps what it passes
            f.tickT = T::orbZapEvery;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || length(e.pos - f.pos) > T::orbZapR * k + e.radius) continue;
                dmgOwner_ = f.owner;
                damageEnemy(e, f.dmg * 0.35f);
                applyElement(e, Element::Electric, f.owner, f.dmg * 0.35f, p, ev);
                dmgOwner_ = -1;
                TiamatWorld::Fx z;
                z.kind = 2;
                z.life = z.maxLife = 0.12f;
                z.pts = {f.pos, e.pos};
                if (tw.fx.size() < 80) tw.fx.push_back(std::move(z));
            }
        }
        f.pos = f.from + (f.to - f.from) * u - sf::Vector2f{0.f, (f.kind == 1 || f.kind == 3 ? 0.f : 90.f * k) * 4.f * u * (1.f - u)};
        if (f.t < 1.f) continue;
        if (f.kind == 1) {   // an ice shard: a small cold burst, it freezes what it hits
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || length(e.pos - f.to) > f.r + e.radius) continue;
                dmgOwner_ = f.owner;
                damageEnemy(e, f.dmg);
                applyElement(e, Element::Ice, f.owner, f.dmg, p, ev);
                dmgOwner_ = -1;
                e.frozen = std::max(e.frozen, 0.6f);
            }
            if (tw.fx.size() < 60) tw.fx.push_back({1, f.to, f.to, 0.25f, 0.25f, f.r, {}});
            continue;
        }
        if (f.kind == 2) tw.fields.push_back({2, f.to, f.r * 0.7f, 2.f, 2.f, f.dmg * 0.6f, 0.f, f.owner});   // molten gold: a pool
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - f.to) > f.r + e.radius) continue;
            dmgOwner_ = f.owner;
            damageEnemy(e, f.dmg);
            applyElement(e, Element::Fire, f.owner, f.dmg, p, ev);
            dmgOwner_ = -1;
        }
        if (boss_.targetable() && length(boss_.pos - f.to) < f.r + boss_.radius) {
            boss_.hp -= f.dmg * 0.5f;
            boss_.hitFlash = 1.f;
        }
        if (tw.fx.size() < 60) tw.fx.push_back({7, f.to, f.to, 0.45f, 0.45f, f.r, {}});
    }
    tw.fireballs.erase(std::remove_if(tw.fireballs.begin(), tw.fireballs.end(), [](const TiamatWorld::Fireball& f) { return f.t >= 1.f; }),
                       tw.fireballs.end());
    for (auto& f : tw.fields) {   // patches on the ground: embers burn, frost slows, molten gold burns and slows, the inferno rides its ball
        f.life -= dt;
        if (f.kind == 4) {   // a thunderstorm: it drifts after the enemies; bolts from it blast what's round where they land
            if (const Enemy* n = nearestEnemy(f.pos, 400.f * k)) f.pos += normalized(n->pos - f.pos, {0.f, 0.f}) * (T::cloudDrift * k * dt);
            else if (boss_.targetable()) f.pos += normalized(boss_.pos - f.pos, {0.f, 0.f}) * (T::cloudDrift * k * dt);
            if ((f.tickT -= dt) > 0.f) continue;
            f.tickT = T::stormStrikeEvery;
            std::vector<Enemy*> in;
            for (Enemy& e : enemies_)
                if (e.hp > 0.f && !e.orbiter && length(e.pos - f.pos) < f.r + e.radius) in.push_back(&e);
            const bool onBoss = bossNear(f.pos, f.r);
            if (in.empty() && !onBoss) continue;
            const sf::Vector2f at = in.empty() || (onBoss && rng_.range(0.f, 1.f) < 0.4f)
                                        ? boss_.pos : in[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(in.size()) - 1))]->pos;
            const float R = T::boltR * k;
            dmgOwner_ = f.owner;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || length(e.pos - at) > R + e.radius) continue;
                damageEnemy(e, f.dps);
                applyElement(e, Element::Electric, f.owner, f.dps, p, ev);
            }
            dmgOwner_ = -1;
            if (boss_.targetable() && length(boss_.pos - at) < R + boss_.radius) { boss_.hp -= f.dps * 0.5f; boss_.hitFlash = 1.f; }
            TiamatWorld::Fx z;   // the bolt, jagged down out of the cloud...
            z.kind = 2;
            z.life = z.maxLife = 0.3f;
            z.pts = {f.pos + sf::Vector2f{rng_.range(-30.f, 30.f) * k, -200.f * k}, at + sf::Vector2f{rng_.range(-25.f, 25.f) * k, -110.f * k},
                     at + sf::Vector2f{rng_.range(-12.f, 12.f) * k, -45.f * k}, at};
            if (tw.fx.size() < 80) tw.fx.push_back(std::move(z));
            if (tw.fx.size() < 80) tw.fx.push_back({13, at, at, 0.4f, 0.4f, R, {}});   // ...and the blast where it lands
            ev.kick = std::max(ev.kick, 2.5f);
            continue;
        }
        if (f.kind == 3)
            if (const Ball* o = ownerBall(f.owner)) f.pos = o->pos;
        const bool tick = (f.tickT -= dt) <= 0.f;
        if (tick) f.tickT = 0.25f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - f.pos) > f.r + e.radius) continue;
            if (f.kind == 1 || f.kind == 2) {
                e.braceT = std::max(e.braceT, 0.3f);
                e.braceSlow = std::max(e.braceSlow, f.kind == 1 ? 0.45f : 0.3f);
            }
            if (!tick) continue;
            dmgOwner_ = f.owner;
            damageEnemy(e, f.dps * 0.25f);
            if (f.kind != 1) { e.burn = std::max(e.burn, 0.5f); e.burnDps = std::max(e.burnDps, f.dps * 0.1f); }
            dmgOwner_ = -1;
        }
    }
    tw.fields.erase(std::remove_if(tw.fields.begin(), tw.fields.end(), [](const TiamatWorld::Field& f) { return f.life <= 0.f; }),
                    tw.fields.end());
    tw.lizards.erase(std::remove_if(tw.lizards.begin(), tw.lizards.end(), [](const TiamatWorld::Lizard& l) { return l.life <= 0.f; }),
                     tw.lizards.end());
}

// An enemy died (anyone's kill): Tiamat's dragons and mounts grow.
void World::tiamatFeed() {
    namespace T = cfg::tiamat;
    TiamatWorld& tw = classWorld_.tiamat;
    for (auto& d : tw.dragons) d.xp += T::killXp * T::growRate[d.kind];
    for (const Ball& b : balls_)
        if (b.hasRole(BallRole::Tiamat) && !b.ghost && b.owner >= 0 && b.owner < TiamatWorld::kMounts) tw.mountXp[b.owner] += T::killXp * T::mountRate;
}

// A ball's dragon of that kind (5: its mount): how grown so far this run (App keeps it in the ball's loadout).
float World::tiamatXp(int owner, int kind) const {
    if (kind == 5) return owner >= 0 && owner < TiamatWorld::kMounts ? classWorld_.tiamat.mountXp[owner] : 0.f;
    for (const auto& d : classWorld_.tiamat.dragons)
        if (d.owner == owner && d.kind == kind) return d.xp;
    return 0.f;
}

}  // namespace sb
