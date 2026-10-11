// Abilities (the class framework, 2026-09-26): timed actives in a ball's
// ability slot(s). Each one fires by itself when its cooldown is up and it has
// something to act on; until then it waits, charged. Copies (ghosts, twins)
// never fire them. Tuning in cfg::ability.

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

const Enemy* World::nearestEnemy(sf::Vector2f from, float maxDist) const {
    const Enemy* best = nullptr;
    float bestD2 = maxDist * maxDist;
    for (const Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const float d2 = dot(e.pos - from, e.pos - from);
        if (d2 < bestD2) { bestD2 = d2; best = &e; }
    }
    return best;
}

void World::resetAbilityCooldowns(Ball& b) {
    for (int i = 0; i < kMaxAbilitySlots; ++i)
        b.abilityCd[i] = abilityCooldown(b.abilities[i].id, b.abilities[i].level) * (1.f - cfg::ability::firstDelay);
}

void World::refundAbility(Ball& b, int i, float seconds, float frac) {
    const AbilitySpec& a = b.abilities[i];
    if (a.id == Ability::None || b.abilityCd[i] <= 0.f) return;
    const float full = abilityCooldown(a.id, a.level);
    const float room = std::max(0.f, full * cfg::ability::refundCap - b.refunded[i]);
    const float take = std::min({seconds + frac * full, room, b.abilityCd[i]});
    b.abilityCd[i] -= take;
    b.refunded[i] += take;
}

void World::updateAbilities(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    b.abilityFlash = std::max(0.f, b.abilityFlash - dt * 2.5f);
    if (!waveRunning_) return;
    const float rate = mageCastRate(b);   // Mage "Focus"
    const bool odin = b.hasRole(BallRole::Odin);   // Odin casts in volleys only (sim/WorldGods.cpp)
    for (int i = 0; i < kMaxAbilitySlots; ++i) {
        const AbilitySpec& a = b.abilities[i];
        if (a.id == Ability::None || odin) continue;
        b.abilityCd[i] -= dt * rate;
        if (b.abilityCd[i] > 0.f) continue;
        // "Cascade": did this cast kill? (count the fallen before and after)
        auto fallen = [&] { int n = 0; for (const Enemy& e : enemies_) n += e.hp <= 0.f ? 1 : 0; return n; };
        const int before = b.mods.cls.common.cascade ? fallen() : 0;
        castBall_ = &b;   // "Spellweave" reads it in damageEnemy
        const bool fired = fireAbility(b, a, p, ev);
        castBall_ = nullptr;
        if (fired) {
            b.abilityCd[i] = abilityCooldown(a.id, a.level);
            b.refunded[i] = 0.f;   // a fresh cooldown, a fresh refund budget
            b.abilityFlash = 1.f;
            mageOnCast(b, i, false, p, ev);   // Mage items / Ancient Mage react to the cast
            godsOnCast(b, a, p, ev);          // Odin: ravens, runes, Draupnir
            commonOnCast(b, p, ev);           // Spellblade / Aether step / Sync / Chorus / Chronomancy
            if (b.mods.cls.common.cascade && fallen() > before)   // "Cascade": again almost at once
                b.abilityCd[i] = std::min(b.abilityCd[i], cfg::common::cascadeFloor);
        } else {
            b.abilityCd[i] = 0.f;   // charged: fires the moment it has a target
        }
    }
    mageTick(b, dt, p, ev);   // Mage: Twincast echoes
    abilityTick(b, dt, p, ev);   // the second batch's lasting ones: Beam, Blades, Leash
}

// Per ball, per step, while a wave runs: what the second batch's abilities
// keep doing after the cast.
void World::abilityTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::ability;
    CommonState& s = b.cls.common;
    const float as = arenaScale();
    if (s.empowerT > 0.f && (s.empowerT -= dt) <= 0.f) s.empower = 0.f;   // "Empower" fizzles unused
    s.beamOn = false;
    if (s.beamT > 0.f) {   // "Beam": burn the nearest enemy in reach
        s.beamT -= dt;
        Enemy* t = nullptr;
        float bd = A::beamRange * as;
        bd *= bd;
        if (const Enemy* f = focusEnemy(); f && dot(f->pos - b.pos, f->pos - b.pos) < bd)   // the player's target, in reach
            t = const_cast<Enemy*>(f), bd = -1.f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const float d2 = dot(e.pos - b.pos, e.pos - b.pos);
            if (d2 < bd) { bd = d2; t = &e; }
        }
        if (t) {
            s.beamOn = true;
            s.beamTo = t->pos;
            castBall_ = &b;
            if ((s.beamTickT -= dt) <= 0.f) {
                s.beamTickT += A::beamTick;
                damageEnemy(*t, s.beamDmg);
            }
            if ((s.beamElemT -= dt) <= 0.f) {
                s.beamElemT += A::beamElemEvery;
                applyElement(*t, hitElement(b, p), b.owner, s.beamDmg, p, ev);
            }
            castBall_ = nullptr;
        }
    }
    if (s.bladesT > 0.f) {   // "Blades": spin, and cut what they touch every tick
        s.bladesT -= dt;
        s.bladeAng += A::bladeSpin * dt;
        if ((s.bladeTickT -= dt) <= 0.f) {
            s.bladeTickT = A::bladeTick;
            castBall_ = &b;
            for (int i = 0; i < s.bladeN; ++i) {
                const float ang = s.bladeAng + 2.f * kPi * static_cast<float>(i) / static_cast<float>(s.bladeN);
                const sf::Vector2f at = b.pos + sf::Vector2f{std::cos(ang), std::sin(ang)} * (A::bladeOrbit * as + b.radius);
                for (Enemy& e : enemies_)
                    if (e.hp > 0.f && length(e.pos - at) <= A::bladeRadius * as + e.radius) damageEnemy(e, s.bladeDmg);
            }
            castBall_ = nullptr;
        }
    }
}

void World::addFx(const CommonWorld::Fx& f) {
    auto& fx = classWorld_.common.fx;
    if (fx.size() >= 96) fx.erase(fx.begin());
    fx.push_back(f);
}

std::vector<int> World::beckonReady() const {
    std::vector<int> out;
    if (!waveRunning_) return out;
    for (int i = 0; i < static_cast<int>(balls_.size()); ++i) {
        const Ball& b = balls_[i];
        if (b.mods.cls.slinger.beckonCd > 0.f && b.cls.slinger.beckonT <= 0.f && !b.held) out.push_back(i);
    }
    return out;
}

bool World::beckon(int i, sf::Vector2f at, const WorldParams& p) {
    if (i < 0 || i >= static_cast<int>(balls_.size())) return false;
    Ball& b = balls_[i];
    if (b.mods.cls.slinger.beckonCd <= 0.f || b.cls.slinger.beckonT > 0.f || b.held) return false;
    const sf::Vector2f from = b.pos;
    b.pos = {clampf(at.x, b.radius, size_.x - b.radius), clampf(at.y, b.radius, size_.y - b.radius)};
    if (length(b.vel) < ballCruise(b, p) * 0.5f)   // stopped: leave at cruise, at the nearest enemy
        if (const Enemy* t = nearestEnemy(b.pos, 1e9f)) b.vel = normalized(t->pos - b.pos, {1.f, 0.f}) * ballCruise(b, p);
    b.trail.clear();
    b.waterTrail.clear();
    b.cls.slinger.beckonT = b.mods.cls.slinger.beckonCd;
    b.regrabT = 0.f;   // it lands in your hand: grab it, aim, throw
    itemFx(CommonWorld::Fx::Beckon, b.pos, b.radius, b.color, 0.4f, from);
    return true;
}

void World::itemFx(CommonWorld::Fx::Kind k, sf::Vector2f at, float r, sf::Color c, float life, sf::Vector2f b, sf::Color c2) {
    CommonWorld::Fx f;
    f.kind = k;
    f.a = at;
    f.b = b;
    f.r = r;
    f.life = f.maxLife = life;
    f.color = c;
    f.color2 = c2;
    addFx(f);
}

// The field things of the second batch: mines, vortices, clouds, portals, the tailwind.
void World::abilityWorldTick(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::ability;
    CommonWorld& cw = classWorld_.common;
    const float as = arenaScale();
    for (auto& f : cw.fx) {   // flourishes age, and ride what they're tied to
        f.life -= dt;
        if (f.ball >= 0)
            for (const Ball& b : balls_)
                if (b.owner == f.ball && !b.ghost) { f.a = b.pos; break; }
        if (f.enemy >= 0)
            for (const Enemy& e : enemies_)
                if (e.id == f.enemy) { if (e.hp > 0.f) f.b = e.pos; break; }
    }
    cw.fx.erase(std::remove_if(cw.fx.begin(), cw.fx.end(), [](const CommonWorld::Fx& f) { return f.life <= 0.f; }),
                cw.fx.end());
    if (cw.tailT > 0.f && (cw.tailT -= dt) <= 0.f) cw.tailMul = 1.f;   // "Tailwind" dies down
    cw.luckT = std::max(0.f, cw.luckT - dt);   // "777" clovers
    cw.rainT = std::max(0.f, cw.rainT - dt);   // "Primal" water
    for (auto& c : cw.trail) {   // "Toxic trail": every so often, what's inside is hurt and takes the element
        c.life -= dt;
        if ((c.tickT -= dt) > 0.f) continue;
        c.tickT = cfg::common::toxicTick;
        const float R = cfg::common::toxicNow(c.r, c.life, c.maxLife);
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - c.pos) > R + e.radius) continue;
            damageEnemy(e, c.dmg);
            if (c.elem != 0) applyElement(e, static_cast<Element>(c.elem), -1, c.dmg, p, ev);   // no owner: reacts with anything
        }
    }
    cw.trail.erase(std::remove_if(cw.trail.begin(), cw.trail.end(), [](const CommonWorld::Cloud& c) { return c.life <= 0.f; }),
                   cw.trail.end());
    for (auto& bt : cw.bottles) {   // "Primal" poison: a bottle in the air, then a splash
        bt.t += dt / cfg::common::bottleFlight;
        if (bt.t < 1.f) continue;
        const float R = cfg::common::bottleRadius * as;
        dmgOwner_ = bt.owner;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - bt.to) > R + e.radius) continue;
            damageEnemy(e, bt.dmg);
            applyElement(e, Element::Poison, bt.owner, bt.dmg, p, ev);
        }
        dmgOwner_ = -1;
        itemFx(CommonWorld::Fx::Splash, bt.to, R, elementColor(Element::Poison), 0.5f);
        CommonWorld::Field pd;   // ...and a puddle that keeps poisoning
        pd.pos = bt.to;
        pd.r = R * 0.8f;
        pd.life = pd.maxLife = cfg::common::puddleTime;
        pd.tick = cfg::common::puddleTick;
        pd.dmg = bt.dmg * cfg::common::puddleFrac / cfg::common::bottleFrac;
        pd.elem = static_cast<int>(Element::Poison);
        pd.owner = bt.owner;
        cw.fields.push_back(pd);
    }
    for (auto& f : cw.fields) {   // "Primal" frost / puddles: they keep biting
        f.life -= dt;
        if ((f.tickT -= dt) > 0.f) continue;
        f.tickT = f.tick;
        dmgOwner_ = f.owner;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - f.pos) > f.r + e.radius) continue;
            damageEnemy(e, f.dmg);
            applyElement(e, static_cast<Element>(f.elem), f.owner, f.dmg, p, ev);
            if (f.elem == static_cast<int>(Element::Ice)) e.frozen = std::max(e.frozen, cfg::common::frostFreeze);
        }
        dmgOwner_ = -1;
    }
    cw.fields.erase(std::remove_if(cw.fields.begin(), cw.fields.end(), [](const CommonWorld::Field& f) { return f.life <= 0.f; }),
                    cw.fields.end());
    for (auto& bo : cw.boulders) {   // "Primal" stone: rocks fall, and land hard
        const float before = bo.t;
        bo.t += dt / cfg::common::boulderFall;
        if (before >= 1.f || bo.t < 1.f) continue;
        const float R = cfg::common::boulderRadius * as;
        dmgOwner_ = bo.owner;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - bo.at) > R + e.radius) continue;
            damageEnemy(e, bo.dmg);
            applyElement(e, Element::Stone, bo.owner, bo.dmg, p, ev);
            e.stagger = std::max(e.stagger, cfg::common::quakeStun);
        }
        dmgOwner_ = -1;
        itemFx(CommonWorld::Fx::Rubble, bo.at, R * 0.6f, elementColor(Element::Stone), 0.6f);
        ev.kick = std::max(ev.kick, 5.f);
    }
    cw.boulders.erase(std::remove_if(cw.boulders.begin(), cw.boulders.end(), [](const CommonWorld::Boulder& b) { return b.t >= 1.2f; }),
                      cw.boulders.end());
    if (cw.rainT > 0.f && (cw.rainDropT -= dt) <= 0.f) {   // "Primal" water: drops pelting the enemies
        cw.rainDropT = cfg::common::rainDropEvery;
        std::vector<Enemy*> live;
        for (Enemy& e : enemies_)
            if (e.hp > 0.f && !e.orbiter && e.pos.x > 0.f && e.pos.y > 0.f && e.pos.x < size_.x && e.pos.y < size_.y) live.push_back(&e);
        if (!live.empty()) {
            Enemy& e = *live[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(live.size()) - 1))];
            e.lastBy = cw.rainOwner;
            damageEnemy(e, cw.rainDmg);
            applyElement(e, Element::Water, cw.rainOwner, cw.rainDmg, p, ev);
            itemFx(CommonWorld::Fx::Drop, e.pos + sf::Vector2f{rng_.range(-1.f, 1.f), rng_.range(-1.f, 1.f)} * e.radius, e.radius,
                   elementColor(Element::Water), 0.35f);
        }
    }
    cw.bottles.erase(std::remove_if(cw.bottles.begin(), cw.bottles.end(), [](const CommonWorld::Bottle& b) { return b.t >= 1.f; }),
                     cw.bottles.end());
    for (Enemy& e : enemies_) {   // "Black plague": the mad pass it on to whatever they touch
        if (e.plagueT <= 0.f || e.hp <= 0.f) continue;
        for (Enemy& o : enemies_)
            if (&o != &e && o.hp > 0.f && o.plagueT <= 0.f && !o.orbiter && length(o.pos - e.pos) <= o.radius + e.radius + 2.f) {
                damageEnemy(o, e.plagueDps * cfg::common::plagueBite);   // a bite...
                infect(o, e.plagueDps);                                  // ...and it spreads
            }
    }
    for (auto& co : cw.coins) {   // "777" coin rain: falling, they hit what they meet and pay
        co.pos += co.vel * dt;
        co.spin += dt * 9.f;
        const float R = cfg::common::slotRainRadius * as;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - co.pos) > e.radius + 6.f * as) continue;
            dmgOwner_ = co.owner;
            for (Enemy& o : enemies_)
                if (o.hp > 0.f && length(o.pos - co.pos) <= R + o.radius) damageEnemy(o, co.dmg);
            dmgOwner_ = -1;
            ev.midasGold += cfg::common::slotGold;
            itemFx(CommonWorld::Fx::Pellets, co.pos, R, theme::puGolden, 0.25f);
            co.pos.y = size_.y + 100.f;   // spent
            break;
        }
    }
    cw.coins.erase(std::remove_if(cw.coins.begin(), cw.coins.end(), [&](const CommonWorld::Coin& co) { return co.pos.y > size_.y + 20.f; }),
                   cw.coins.end());
    for (auto& m : cw.mines) {   // "Mines": an enemy steps on one
        m.life -= dt;
        m.arm -= dt;
        if (m.arm > 0.f || m.life <= 0.f) continue;
        bool hit = false;
        for (const Enemy& e : enemies_)
            if (e.hp > 0.f && length(e.pos - m.pos) <= A::mineTrigger * as + e.radius) { hit = true; break; }
        if (!hit) continue;
        m.life = 0.f;
        dmgOwner_ = m.owner;   // kill credit
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - m.pos) > m.r + e.radius) continue;
            damageEnemy(e, m.dmg);
            if (m.elem != 0) applyElement(e, static_cast<Element>(m.elem), m.owner, m.dmg, p, ev);
        }
        dmgOwner_ = -1;
        const sf::Color mc = m.elem != 0 ? elementColor(static_cast<Element>(m.elem)) : theme::accent;
        ev.bursts.push_back({m.pos, m.r, mc, nullptr});
        CommonWorld::Fx f;   // shrapnel flying out
        f.kind = CommonWorld::Fx::Shards;
        f.a = m.pos;
        f.r = m.r;
        f.life = f.maxLife = 0.3f;
        f.color = mc;
        addFx(f);
        ev.kick = std::max(ev.kick, 2.5f);
    }
    cw.mines.erase(std::remove_if(cw.mines.begin(), cw.mines.end(), [](const CommonWorld::Mine& m) { return m.life <= 0.f; }),
                   cw.mines.end());
    for (auto& v : cw.vortices) {   // "Vortex": drag what's inside toward the eye, and grind it
        v.life -= dt;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const sf::Vector2f d = v.pos - e.pos;
            const float l = length(d);
            if (l > v.r + e.radius || l < 1.f) continue;
            e.pos += d / l * std::min(l, A::vortexPull * 0.15f * as * e.knockTaken * dt);
            damageEnemy(e, v.dps * dt);
        }
    }
    cw.vortices.erase(std::remove_if(cw.vortices.begin(), cw.vortices.end(), [](const CommonWorld::Vortex& v) { return v.life <= 0.f; }),
                      cw.vortices.end());
    for (auto& c : cw.clouds) {   // "Cloud": every so often, what's inside takes its element
        c.life -= dt;
        if ((c.tickT -= dt) > 0.f) continue;
        c.tickT = A::cloudTick;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - c.pos) > c.r + e.radius) continue;
            damageEnemy(e, c.dmg);
            if (c.elem != 0) applyElement(e, static_cast<Element>(c.elem), -1, c.dmg, p, ev);   // no owner: reacts with anything
        }
    }
    cw.clouds.erase(std::remove_if(cw.clouds.begin(), cw.clouds.end(), [](const CommonWorld::Cloud& c) { return c.life <= 0.f; }),
                    cw.clouds.end());
    auto portalBalls = [&](std::vector<Ball>& bs) {   // "Portal": in one, out the other
        for (Ball& b : bs) {
            b.cls.common.portalCd = std::max(0.f, b.cls.common.portalCd - dt);
            if (b.held || b.cls.common.portalCd > 0.f || b.cls.common.leashT > 0.f) continue;
            for (const auto& q : cw.portals) {
                const float R = A::portalRadius * as;
                const bool inA = length(b.pos - q.a) < R, inB = length(b.pos - q.b) < R;
                if (!inA && !inB) continue;
                const sf::Vector2f to = inA ? q.b : q.a;
                CommonWorld::Fx f;   // swallowed: a ring closes on the mouth it went in by
                f.kind = CommonWorld::Fx::Implode;
                f.a = inA ? q.a : q.b;
                f.r = R * 1.5f;
                f.life = f.maxLife = 0.3f;
                f.color = theme::classMage;
                addFx(f);
                // shot out of the far mouth, straight at the enemy nearest it (or on its way, if none)
                sf::Vector2f dir = normalized(b.vel, {1.f, 0.f});
                if (const Enemy* t = nearestEnemy(to, 1e9f)) dir = normalized(t->pos - to, dir);
                b.pos = to + dir * (R + b.radius + 2.f);
                b.pos = {clampf(b.pos.x, b.radius, size_.x - b.radius), clampf(b.pos.y, b.radius, size_.y - b.radius)};
                b.vel = dir * std::min(std::max(length(b.vel), ballCruise(b, p) * A::portalLaunch), ballMaxSpeed(b, p));
                b.homing = false;
                b.squash = 1.f;
                b.squashAxis = dir;
                b.trail.clear();
                b.waterTrail.clear();
                b.cls.common.portalCd = A::portalCd;
                ev.bursts.push_back({to, R * 1.6f, b.color, nullptr});
                CommonWorld::Fx out;   // the shot: speed lines off the far mouth
                out.kind = CommonWorld::Fx::Afterimage;
                out.a = b.pos;
                out.b = dir;
                out.r = b.radius;
                out.life = out.maxLife = 0.3f;
                out.color = theme::classMage;
                addFx(out);
                ev.kick = std::max(ev.kick, 2.f);
                break;
            }
        }
    };
    for (auto& q : cw.portals) q.life -= dt;
    cw.portals.erase(std::remove_if(cw.portals.begin(), cw.portals.end(), [](const CommonWorld::Portal& q) { return q.life <= 0.f; }),
                     cw.portals.end());
    if (!cw.portals.empty()) {
        portalBalls(balls_);
        portalBalls(ghosts_);
    }
}

bool World::fireAbility(Ball& b, const AbilitySpec& a, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::ability;
    const float n = static_cast<float>(std::max(0, a.level - 1));   // levels past the first
    const float as = arenaScale();
    const float pw = b.mods.cls.mage.power;              // Mage "Attunement": damage x this...
    const float reach = 1.f + (pw - 1.f) * 0.5f;         // ...radius / duration x this
    const bool anyEnemy = !enemies_.empty() || (boss_.alive && boss_.intro <= 0.f);
    auto inArena = [&](sf::Vector2f q) { return q.x >= 0.f && q.y >= 0.f && q.x <= size_.x && q.y <= size_.y; };
    // The enemy the player clicked (2026-10-06): the picky abilities go for it first.
    Enemy* const foc = [&]() -> Enemy* {
        const Enemy* f = focusEnemy();
        return f && !f->orbiter && inArena(f->pos) ? const_cast<Enemy*>(f) : nullptr;
    }();
    auto enemyNearCore = [&]() -> Enemy* {   // the player's target, else the live enemy nearest the core, on the field
        if (foc) return foc;
        Enemy* best = nullptr;
        float bd = 1e18f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || !inArena(e.pos)) continue;
            const float d2 = dot(e.pos - core_.pos, e.pos - core_.pos);
            if (d2 < bd) { bd = d2; best = &e; }
        }
        return best;
    };
    switch (a.id) {
        case Ability::None: return false;

        case Ability::Dash: {   // a burst straight at the nearest enemy
            sf::Vector2f target;
            if (const auto f = focusPos()) target = *f;   // the player's target
            else if (const Enemy* e = nearestEnemy(b.pos, A::dashRange * as)) target = e->pos;
            else if (boss_.alive && boss_.intro <= 0.f) target = boss_.pos;
            else return false;
            const sf::Vector2f d = normalized(target - b.pos, {1.f, 0.f});
            const float sp = std::max(length(b.vel), ballCruise(b, p) * (A::dashSpeed + A::dashSpeedPerLevel * n) * reach);
            b.vel = d * std::min(sp, ballMaxSpeed(b, p));
            b.homing = false;
            b.squash = 1.f;
            b.squashAxis = d;
            ev.bursts.push_back({b.pos, b.radius * 2.2f, b.color, nullptr});
            CommonWorld::Fx f;   // speed lines left behind, pointing the way it went
            f.kind = CommonWorld::Fx::Afterimage;
            f.a = b.pos;
            f.b = d;
            f.r = b.radius;
            f.life = f.maxLife = 0.35f;
            f.color = b.color;
            addFx(f);
            return true;
        }

        case Ability::Nova: {   // a shockwave around the ball
            const float R = (A::novaRadius + A::novaRadiusPerLevel * n) * as * reach;
            if (!nearestEnemy(b.pos, R + cfg::wave::enemyRadius)) return false;   // wait until something is close
            const float dmg = ballDamage(b, p) * (A::novaFrac + A::novaFracPerLevel * n) * pw;
            const Element el = hitElement(b, p);
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                const sf::Vector2f d = e.pos - b.pos;
                if (length(d) > R + e.radius) continue;
                damageEnemy(e, dmg);
                e.vel += normalized(d, {1.f, 0.f}) * A::novaKnock * e.knockTaken;
                applyElement(e, el, b.owner, dmg, p, ev);
            }
            ev.bursts.push_back({b.pos, R, b.color, nullptr});
            return true;
        }

        case Ability::Split: {   // two short-lived ghost copies fan out
            if (!anyEnemy) return false;
            for (float side : {-1.f, 1.f}) {
                if (static_cast<int>(ghosts_.size() + pendingGhosts_.size()) >= cfg::synergy::maxGhosts) break;
                Ball g = b;
                g.ghost = true;
                g.twin = false;
                g.ghostLife = (A::splitLife + A::splitLifePerLevel * n) * reach * b.mods.copyLife;   // web "Brood"
                g.age = 0.f;
                g.held = false;
                g.trail.clear();
                g.waterTrail.clear();
                const float ang = side * A::splitSpread;
                const float c = std::cos(ang), sn = std::sin(ang);
                sf::Vector2f v = b.vel;
                if (length(v) < 1e-3f) v = rng_.direction() * ballCruise(b, p);
                g.vel = {v.x * c - v.y * sn, v.x * sn + v.y * c};
                pendingGhosts_.push_back(g);
            }
            return true;
        }

        case Ability::Bulwark: {   // the core pushes out a pulse
            const float R = (A::bulwarkRadius + A::bulwarkRadiusPerLevel * n) * as * reach;
            if (!nearestEnemy(core_.pos, R)) return false;   // only when enemies close in
            const float dmg = ballDamage(b, p) * A::bulwarkFrac * pw;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                const sf::Vector2f d = e.pos - core_.pos;
                if (length(d) > R + e.radius) continue;
                damageEnemy(e, dmg);
                e.vel += normalized(d, {1.f, 0.f}) * A::bulwarkKnock * e.knockTaken;
                e.stagger = std::max(e.stagger, A::bulwarkStagger + A::bulwarkStaggerPerLevel * n);
            }
            ev.bursts.push_back({core_.pos, R, theme::core, nullptr});
            return true;
        }

        case Ability::Overclock: {   // a few seconds hot
            if (!anyEnemy) return false;
            b.overclockT = (A::overclockTime + A::overclockTimePerLevel * n) * reach;
            b.overclockMul = A::overclockDamage + A::overclockDamagePerLevel * n;
            return true;
        }

        // ---- added with the Mage (generic: any ball can carry them) ----

        case Ability::Arc: {   // a bolt leaps from the ball through a chain of enemies
            const float dmg = ballDamage(b, p) * (A::arcFrac + A::arcFracPerLevel * n) * pw;
            const Element el = hitElement(b, p);
            const int hops = A::arcTargets + static_cast<int>(n);
            std::vector<const Enemy*> hit;
            sf::Vector2f from = b.pos;
            float reachNext = A::arcRange * as * reach;   // the first leap, then arcJump
            while (static_cast<int>(hit.size()) < hops) {
                Enemy* t = nullptr;   // the nearest enemy not hit yet (the first leap: the player's target, if it's in reach)
                float best = reachNext * reachNext;
                if (hit.empty() && foc && dot(foc->pos - from, foc->pos - from) <= best * 2.25f) best = -1.f, t = foc;
                for (Enemy& o : enemies_) {
                    if (o.hp <= 0.f || o.orbiter || std::find(hit.begin(), hit.end(), &o) != hit.end()) continue;
                    const float d2 = dot(o.pos - from, o.pos - from);
                    if (d2 < best) { best = d2; t = &o; }
                }
                if (!t) break;
                damageEnemy(*t, dmg);
                applyElement(*t, el, b.owner, dmg, p, ev);
                if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                    bolts_.push_back(Bolt{from, t->pos, cfg::element::boltLife, cfg::element::boltLife});
                hit.push_back(t);
                from = t->pos;
                reachNext = A::arcJump * as * reach;
            }
            return !hit.empty();   // nothing in range: wait, charged
        }

        case Ability::Meteor: {   // crushes the thickest pack of enemies, anywhere
            const float R = (A::meteorRadius + A::meteorRadiusPerLevel * n) * as * reach;
            const Enemy* at = nullptr;
            int most = 0;
            for (const Enemy& e : enemies_) {   // the enemy with the most others around it
                if (e.hp <= 0.f || e.orbiter) continue;
                int c = 0;
                for (const Enemy& o : enemies_)
                    if (o.hp > 0.f && !o.orbiter && dot(o.pos - e.pos, o.pos - e.pos) < R * R) ++c;
                if (c > most) { most = c; at = &e; }
            }
            if (foc) at = foc;   // the player's target, pack or not
            if (!at) return false;
            const sf::Vector2f c = at->pos;
            const float dmg = ballDamage(b, p) * (A::meteorFrac + A::meteorFracPerLevel * n) * pw;
            const Element el = hitElement(b, p);
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || length(e.pos - c) > R + e.radius) continue;
                damageEnemy(e, dmg);
                e.stagger = std::max(e.stagger, A::meteorStagger);
                applyElement(e, el, b.owner, dmg, p, ev);
            }
            ev.bursts.push_back({c, R, b.color, nullptr});
            CommonWorld::Fx f;   // the rock's fiery tail, still in the air, and a scorched crater
            f.kind = CommonWorld::Fx::Meteor;
            f.a = c;
            f.r = R;
            f.life = f.maxLife = 0.9f;
            f.color = b.color;
            addFx(f);
            ev.kick = std::max(ev.kick, 7.f);
            return true;
        }

        case Ability::MagicMissile: {   // the Mage's signature: homing missiles (WorldClasses.cpp)
            const int count = 1 + (a.level >= 3 ? 1 : 0) + (a.level >= 5 ? 1 : 0) + b.mods.cls.mage.barrage;
            return mageMissiles(b, count, A::missileFrac + A::missileFracPerLevel * n, p);
        }

        // ---- the second batch (2026-10-05): any ball can carry them ----

        case Ability::Hook: {   // yank the enemy nearest the core over to the ball
            Enemy* t = enemyNearCore();
            if (!t) return false;
            const float dmg = ballDamage(b, p) * (A::hookFrac + A::hookFracPerLevel * n) * pw;
            damageEnemy(*t, dmg);
            applyElement(*t, hitElement(b, p), b.owner, dmg, p, ev);
            t->vel = normalized(b.pos - t->pos, {1.f, 0.f}) * A::hookSpeed * as * t->knockTaken;
            t->stagger = std::max(t->stagger, (A::hookStun + A::hookStunPerLevel * n) * reach);
            CommonWorld::Fx f;   // a chain shoots out, bites, and reels the enemy in
            f.kind = CommonWorld::Fx::Hook;
            f.a = b.pos;
            f.b = t->pos;
            f.ball = b.owner;
            f.enemy = t->id;
            f.life = f.maxLife = 0.45f;
            f.color = b.color;
            addFx(f);
            return true;
        }

        case Ability::Mines: {   // a short row of mines behind it
            if (!anyEnemy) return false;
            auto& ms = classWorld_.common.mines;
            const int count = A::mines + (a.level >= 3 ? 1 : 0) + (a.level >= 5 ? 1 : 0);
            const sf::Vector2f back = -normalized(b.vel, {0.f, 1.f});
            for (int i = 0; i < count; ++i) {
                if (static_cast<int>(ms.size()) >= A::maxMines) ms.erase(ms.begin());
                CommonWorld::Mine m;
                const sf::Vector2f at = b.pos + back * (A::mineGap * as * static_cast<float>(i));
                m.pos = {clampf(at.x, 10.f, size_.x - 10.f), clampf(at.y, 10.f, size_.y - 10.f)};
                m.life = A::mineLife;
                m.arm = A::mineArm + 0.1f * static_cast<float>(i);
                m.dmg = ballDamage(b, p) * (A::mineFrac + A::mineFracPerLevel * n) * pw;
                m.r = A::mineRadius * as * reach;
                m.elem = static_cast<int>(hitElement(b, p));
                m.owner = b.owner;
                ms.push_back(m);
            }
            return true;
        }

        case Ability::Empower: {   // the next hit: x3 and a wave
            if (!anyEnemy) return false;
            CommonState& s = b.cls.common;
            s.empower = (A::empower + A::empowerPerLevel * n) * pw;
            s.empowerR = (A::empowerRadius + A::empowerRadiusPerLevel * n) * as * reach;
            s.empowerT = A::empowerWindow;
            ev.bursts.push_back({b.pos, b.radius * 2.f, b.color, nullptr});
            return true;
        }

        case Ability::Tailwind: {   // every ball, faster a while
            if (!anyEnemy) return false;
            CommonWorld& cw = classWorld_.common;
            const float mul = A::tailwind + A::tailwindPerLevel * n;
            cw.tailT = std::max(cw.tailT, (A::tailwindTime + A::tailwindTimePerLevel * n) * reach);
            cw.tailMul = std::max(cw.tailMul, mul);
            for (Ball& o : balls_)
                if (!o.held) boostSpeed(o, mul, p);
            ev.bursts.push_back({b.pos, b.radius * 3.f, theme::puSurge, nullptr});
            return true;
        }

        case Ability::Vortex: {   // a whirlpool where it is
            const float R = (A::vortexRadius + A::vortexRadiusPerLevel * n) * as * reach;
            if (!nearestEnemy(b.pos, R + cfg::wave::enemyRadius)) return false;
            CommonWorld::Vortex v;
            v.pos = b.pos;
            v.life = v.maxLife = (A::vortexTime + A::vortexTimePerLevel * n) * reach;
            v.r = R;
            v.dps = ballDamage(b, p) * A::vortexDps * pw;
            v.elem = static_cast<int>(hitElement(b, p));
            classWorld_.common.vortices.push_back(v);
            return true;
        }

        case Ability::Beam: {   // a steady beam on the nearest enemy (abilityTick burns it)
            if (!nearestEnemy(b.pos, A::beamRange * as * reach)) return false;
            CommonState& s = b.cls.common;
            s.beamT = (A::beamTime + A::beamTimePerLevel * n) * reach;
            s.beamTickT = s.beamElemT = 0.f;
            s.beamDmg = ballDamage(b, p) * (A::beamFrac + A::beamFracPerLevel * n) * pw;
            return true;
        }

        case Ability::Blades: {   // blades spin around it (abilityTick cuts)
            if (!anyEnemy) return false;
            CommonState& s = b.cls.common;
            s.bladesT = (A::bladesTime + A::bladesTimePerLevel * n) * reach;
            s.bladeN = A::blades + (a.level >= 3 ? 1 : 0) + (a.level >= 5 ? 1 : 0);
            s.bladeDmg = ballDamage(b, p) * (A::bladeFrac + A::bladeFracPerLevel * n) * pw;
            return true;
        }

        case Ability::Cloud: {   // a cloud of its element hangs here
            const float R = (A::cloudRadius + A::cloudRadiusPerLevel * n) * as * reach;
            if (!nearestEnemy(b.pos, R * 2.f)) return false;
            auto& cs = classWorld_.common.clouds;
            if (static_cast<int>(cs.size()) >= A::maxClouds) cs.erase(cs.begin());
            CommonWorld::Cloud c;
            c.pos = b.pos;
            c.life = c.maxLife = (A::cloudTime + A::cloudTimePerLevel * n) * reach;
            c.r = R;
            c.dmg = ballDamage(b, p) * A::cloudFrac * pw;
            c.elem = static_cast<int>(hitElement(b, p));
            cs.push_back(c);
            return true;
        }

        case Ability::Smite: {   // lightning on the strongest enemies
            const int count = A::smites + (a.level >= 3 ? 1 : 0) + (a.level >= 5 ? 1 : 0);
            const float dmg = ballDamage(b, p) * (A::smiteFrac + A::smiteFracPerLevel * n) * pw;
            const Element el = hitElement(b, p);
            std::vector<Enemy*> pick;
            for (Enemy& e : enemies_)
                if (e.hp > 0.f && !e.orbiter && inArena(e.pos)) pick.push_back(&e);
            std::sort(pick.begin(), pick.end(), [foc](const Enemy* x, const Enemy* y) {   // the player's target, then the strongest
                if ((x == foc) != (y == foc)) return x == foc;
                return x->maxHp > y->maxHp;
            });
            if (static_cast<int>(pick.size()) > count) pick.resize(static_cast<std::size_t>(count));
            const bool boss = boss_.alive && boss_.intro <= 0.f;
            if (pick.empty() && !boss) return false;
            const sf::Color sc = el != Element::Plain ? elementColor(el) : b.color;
            auto smiteFx = [&](sf::Vector2f at, float r) {   // a jagged bolt from the sky, a scorch where it lands
                CommonWorld::Fx f;
                f.kind = CommonWorld::Fx::Smite;
                f.a = {at.x + std::sin(at.x * 12.9898f + at.y * 78.233f) * 40.f * as, 0.f};   // a slant, not off the sim's dice
                f.b = at;
                f.r = r;
                f.life = f.maxLife = 0.5f;
                f.color = sc;
                addFx(f);
            };
            for (Enemy* e : pick) {
                damageEnemy(*e, dmg);
                e->stagger = std::max(e->stagger, A::smiteStagger);
                applyElement(*e, el, b.owner, dmg, p, ev);
                smiteFx(e->pos, e->radius);
            }
            if (boss) {
                boss_.hp -= dmg * A::missileBossFrac;
                boss_.hitFlash = 1.f;
                smiteFx(boss_.pos, boss_.radius);
            }
            ev.kick = std::max(ev.kick, 6.f);
            return true;
        }

        case Ability::Warp: {   // onto the enemy nearest the core, between it and the core, and strike it away
            if (b.held) return false;
            Enemy* t = enemyNearCore();
            if (!t) return false;
            const sf::Vector2f out = normalized(t->pos - core_.pos, {1.f, 0.f});
            const sf::Vector2f at = t->pos - out * (t->radius + b.radius + 4.f * as);
            const sf::Vector2f from = b.pos;
            b.pos = {clampf(at.x, b.radius, size_.x - b.radius), clampf(at.y, b.radius, size_.y - b.radius)};
            b.vel = out * std::max(length(b.vel), ballCruise(b, p));
            b.trail.clear();
            b.waterTrail.clear();
            b.squash = 1.f;
            b.squashAxis = out;
            const float dmg = ballDamage(b, p) * (A::warpFrac + A::warpFracPerLevel * n) * pw;
            damageEnemy(*t, dmg);
            applyElement(*t, hitElement(b, p), b.owner, dmg, p, ev);
            t->vel += out * A::warpKnock * as * t->knockTaken;
            ev.bursts.push_back({b.pos, b.radius * 2.5f, b.color, nullptr});
            CommonWorld::Fx f;   // a ghost of the ball fades where it was; space folds shut where it lands
            f.kind = CommonWorld::Fx::Warp;
            f.a = from;
            f.b = b.pos;
            f.r = b.radius;
            f.life = f.maxLife = 0.45f;
            f.color = b.color;
            addFx(f);
            ev.kick = std::max(ev.kick, 3.f);
            commonOnBlink(b, *t, p, from);   // Assassin items that ride a blink ride this one too
            return true;
        }

        case Ability::Portal: {   // two portals close together: by the core, and a short way out toward the threat
            Enemy* t = enemyNearCore();
            if (!t && !(boss_.alive && boss_.intro <= 0.f)) return false;
            const sf::Vector2f toward = t ? t->pos : boss_.pos;
            const sf::Vector2f d = normalized(toward - core_.pos, {1.f, 0.f});
            const sf::Vector2f in = core_.pos + d * (core_.radius + 70.f * as);
            const float far = std::min(length(toward - in) * 0.6f, A::portalRange * as * reach);
            const float m = 40.f * as;
            sf::Vector2f out = in + d * std::max(far, A::portalRadius * 3.f * as);
            out = {clampf(out.x, m, size_.x - m), clampf(out.y, m, size_.y - m)};
            auto& ps = classWorld_.common.portals;
            ps.erase(std::remove_if(ps.begin(), ps.end(), [&](const CommonWorld::Portal& q) { return q.owner == b.owner; }),
                     ps.end());
            CommonWorld::Portal q;
            q.a = in;
            q.b = out;
            q.life = q.maxLife = (A::portalTime + A::portalTimePerLevel * n) * reach;
            q.owner = b.owner;
            ps.push_back(q);
            return true;
        }

        case Ability::Leash: {   // tied to the core it spins up, then it's let go (World::advanceLeash)
            if (!anyEnemy || b.held) return false;
            CommonState& s = b.cls.common;
            if (s.leashT > 0.f || s.leashFlyT > 0.f) return false;   // one swing at a time (a 2nd Leash waits its turn)
            const sf::Vector2f r = b.pos - core_.pos;
            // The rope fits inside the arena, so the spin never meets a wall.
            const float room = std::min({core_.pos.x, size_.x - core_.pos.x, core_.pos.y, size_.y - core_.pos.y}) -
                               b.radius - 4.f;
            s.leashT = A::leashTime;
            s.leashL = std::min(clampf(length(r), A::leashMin * as, A::leashMax * as), room);
            s.leashL = std::max(s.leashL, core_.radius + b.radius + 6.f);
            s.leashAng = std::atan2(r.y, r.x);
            s.leashDir = (r.x * b.vel.y - r.y * b.vel.x) >= 0.f ? 1.f : -1.f;   // the way it was already going round
            s.leashFlyLen = (A::leashFly + A::leashFlyPerLevel * n) * reach;
            return true;
        }

        case Ability::Swap: {   // trade places with the enemy nearest the core (only if it's closer in than the ball)
            if (b.held) return false;
            Enemy* t = enemyNearCore();
            if (t == foc && t && length(t->pos - core_.pos) >= length(b.pos - core_.pos)) {   // the target is further out: the usual pick
                Enemy* const keep = t;
                t = nullptr;
                float bd = 1e18f;
                for (Enemy& e : enemies_) {
                    if (&e == keep || e.hp <= 0.f || e.orbiter || !inArena(e.pos)) continue;
                    if (const float d2 = dot(e.pos - core_.pos, e.pos - core_.pos); d2 < bd) { bd = d2; t = &e; }
                }
            }
            if (!t || length(t->pos - core_.pos) >= length(b.pos - core_.pos)) return false;
            const sf::Vector2f was = b.pos;
            b.pos = t->pos;
            t->pos = was;
            t->vel = {0.f, 0.f};
            t->stagger = std::max(t->stagger, (A::swapStun + A::swapStunPerLevel * n) * reach);
            const float dmg = ballDamage(b, p) * (A::swapFrac + A::swapFracPerLevel * n) * pw;
            damageEnemy(*t, dmg);
            applyElement(*t, hitElement(b, p), b.owner, dmg, p, ev);
            b.trail.clear();
            b.waterTrail.clear();
            ev.bursts.push_back({was, b.radius * 2.f, b.color, nullptr});
            ev.bursts.push_back({b.pos, b.radius * 2.f, b.color, nullptr});
            CommonWorld::Fx f;   // two comets crossing: the ball one way, the enemy the other
            f.kind = CommonWorld::Fx::Swap;
            f.a = was;
            f.b = b.pos;
            f.r = b.radius;
            f.life = f.maxLife = 0.35f;
            f.color = b.color;
            f.color2 = theme::enemy;
            addFx(f);
            return true;
        }

        case Ability::Shrink: {   // what's near shrinks: smaller, slower, and it takes double
            const float R = (A::shrinkRadius + A::shrinkRadiusPerLevel * n) * as * reach;
            bool any = false;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || length(e.pos - b.pos) > R + e.radius) continue;
                if (e.shrinkT <= 0.f) {
                    e.shrinkR = e.radius;
                    e.radius *= A::shrinkSize;
                    e.shrinkPop = 1.f;   // squeezes down with a wobble (drawn)
                }
                e.shrinkT = std::max(e.shrinkT, (A::shrinkTime + A::shrinkTimePerLevel * n) * reach);
                any = true;
            }
            if (!any) return false;
            CommonWorld::Fx f;   // a ring that closes in instead of going out
            f.kind = CommonWorld::Fx::Implode;
            f.a = b.pos;
            f.r = R;
            f.life = f.maxLife = 0.35f;
            f.color = b.color;
            addFx(f);
            return true;
        }

        case Ability::Bounty: {   // a price on the strongest head
            Enemy* best = foc && foc->bounty <= 0 ? foc : nullptr;   // the player's target, else the strongest
            if (!best)
                for (Enemy& e : enemies_)
                    if (e.hp > 0.f && !e.orbiter && e.bounty <= 0 && inArena(e.pos) && (!best || e.maxHp > best->maxHp)) best = &e;
            if (!best) return false;
            best->bounty = A::bounty + A::bountyPerLevel * static_cast<int>(n);
            ev.bursts.push_back({best->pos, best->radius * 2.5f, theme::puGolden, nullptr});
            return true;
        }
    }
    return false;
}

}  // namespace sb
