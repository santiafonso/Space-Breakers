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

void World::updateAbilities(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    b.abilityFlash = std::max(0.f, b.abilityFlash - dt * 2.5f);
    if (!waveRunning_) return;
    const float rate = mageCastRate(b);   // Mage "Focus"
    for (int i = 0; i < kMaxAbilitySlots; ++i) {
        const AbilitySpec& a = b.abilities[i];
        if (a.id == Ability::None) continue;
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
            b.abilityFlash = 1.f;
            mageOnCast(b, i, false, p, ev);   // Mage items / Ancient Mage react to the cast
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
    if (s.leashT > 0.f) {   // "Leash": a rope to the core - never further than its length, always swinging
        s.leashT -= dt;
        if (b.held) {
            s.leashT = 0.f;
        } else {
            const sf::Vector2f r = b.pos - core_.pos;
            const float d = std::max(1.f, length(r));
            const sf::Vector2f rh = r / d;
            if (d > s.leashL) {
                b.pos = core_.pos + rh * s.leashL;
                if (const float vr = dot(b.vel, rh); vr > 0.f) b.vel -= rh * vr;
            }
            const sf::Vector2f tn{-rh.y, rh.x};
            const float vt = dot(b.vel, tn);
            const float floor = ballCruise(b, p) * s.leashSpeed;
            if (std::fabs(vt) < floor) b.vel += tn * ((vt < 0.f ? -floor : floor) - vt);
        }
    }
}

// The field things of the second batch: mines, vortices, clouds, portals, the tailwind.
void World::abilityWorldTick(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::ability;
    CommonWorld& cw = classWorld_.common;
    const float as = arenaScale();
    if (cw.tailT > 0.f && (cw.tailT -= dt) <= 0.f) cw.tailMul = 1.f;   // "Tailwind" dies down
    for (auto& m : cw.mines) {   // "Mines": an enemy steps on one
        m.life -= dt;
        m.arm -= dt;
        if (m.arm > 0.f || m.life <= 0.f) continue;
        bool hit = false;
        for (const Enemy& e : enemies_)
            if (e.hp > 0.f && length(e.pos - m.pos) <= A::mineTrigger * as + e.radius) { hit = true; break; }
        if (!hit) continue;
        m.life = 0.f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - m.pos) > m.r + e.radius) continue;
            damageEnemy(e, m.dmg);
            if (m.elem != 0) applyElement(e, static_cast<Element>(m.elem), m.owner, m.dmg, p, ev);
        }
        ev.bursts.push_back({m.pos, m.r, m.elem != 0 ? elementColor(static_cast<Element>(m.elem)) : theme::accent, nullptr});
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
            if (b.held || b.mods.satellite || b.cls.common.portalCd > 0.f || b.cls.common.leashT > 0.f) continue;
            for (const auto& q : cw.portals) {
                const float R = A::portalRadius * as;
                const bool inA = length(b.pos - q.a) < R, inB = length(b.pos - q.b) < R;
                if (!inA && !inB) continue;
                const sf::Vector2f to = inA ? q.b : q.a;
                const sf::Vector2f dir = normalized(b.vel, {1.f, 0.f});
                b.pos = to + dir * (R + b.radius + 2.f);
                b.pos = {clampf(b.pos.x, b.radius, size_.x - b.radius), clampf(b.pos.y, b.radius, size_.y - b.radius)};
                b.trail.clear();
                b.waterTrail.clear();
                b.cls.common.portalCd = A::portalCd;
                boostSpeed(b, A::portalBoost, p);
                ev.bursts.push_back({to, R * 1.6f, b.color, nullptr});
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
    auto enemyNearCore = [&]() -> Enemy* {   // the live enemy nearest the core, on the field
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
            if (b.mods.satellite) return false;   // an orbit can't dash
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
                Enemy* t = nullptr;   // the nearest enemy not hit yet
                float best = reachNext * reachNext;
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
            Bolt line{b.pos, t->pos, 0.25f, 0.25f};
            line.beam = true;
            bolts_.push_back(line);
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
            std::sort(pick.begin(), pick.end(), [](const Enemy* x, const Enemy* y) { return x->maxHp > y->maxHp; });
            if (static_cast<int>(pick.size()) > count) pick.resize(static_cast<std::size_t>(count));
            const bool boss = boss_.alive && boss_.intro <= 0.f;
            if (pick.empty() && !boss) return false;
            for (Enemy* e : pick) {
                damageEnemy(*e, dmg);
                e->stagger = std::max(e->stagger, A::smiteStagger);
                applyElement(*e, el, b.owner, dmg, p, ev);
                if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                    bolts_.push_back(Bolt{{e->pos.x, 0.f}, e->pos, cfg::element::boltLife * 1.5f, cfg::element::boltLife * 1.5f});
            }
            if (boss) {
                boss_.hp -= dmg * A::missileBossFrac;
                boss_.hitFlash = 1.f;
                bolts_.push_back(Bolt{{boss_.pos.x, 0.f}, boss_.pos, cfg::element::boltLife * 1.5f, cfg::element::boltLife * 1.5f});
            }
            return true;
        }

        case Ability::Warp: {   // onto the enemy nearest the core, between it and the core, and strike it away
            if (b.mods.satellite || b.held) return false;
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
            ev.bursts.push_back({from, b.radius * 2.f, b.color, nullptr});
            ev.bursts.push_back({b.pos, b.radius * 2.5f, b.color, nullptr});
            commonOnBlink(b, *t, p, from);   // Assassin items that ride a blink ride this one too
            return true;
        }

        case Ability::Portal: {   // two portals: by the core, and out by the enemies
            if (!anyEnemy) return false;
            const Enemy* far = nullptr;
            float fd = -1.f;
            for (const Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || !inArena(e.pos)) continue;
                const float d2 = dot(e.pos - core_.pos, e.pos - core_.pos);
                if (d2 > fd) { fd = d2; far = &e; }
            }
            sf::Vector2f out = far ? far->pos : boss_.pos;
            if (!far && !(boss_.alive && boss_.intro <= 0.f)) return false;
            const float m = 50.f * as;
            out = {clampf(out.x, m, size_.x - m), clampf(out.y, m, size_.y - m)};
            const sf::Vector2f in = core_.pos + normalized(out - core_.pos, {1.f, 0.f}) * (core_.radius + 70.f * as);
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

        case Ability::Leash: {   // tied to the core a while: it swings around it (abilityTick)
            if (!anyEnemy || b.mods.satellite || b.held) return false;
            CommonState& s = b.cls.common;
            s.leashT = (A::leashTime + A::leashTimePerLevel * n) * reach;
            s.leashL = clampf(length(b.pos - core_.pos), A::leashMin * as, A::leashMax * as);
            s.leashSpeed = A::leashSpeed + A::leashSpeedPerLevel * n;
            return true;
        }

        case Ability::Swap: {   // trade places with the enemy nearest the core (only if it's closer in than the ball)
            if (b.mods.satellite || b.held) return false;
            Enemy* t = enemyNearCore();
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
                }
                e.shrinkT = std::max(e.shrinkT, (A::shrinkTime + A::shrinkTimePerLevel * n) * reach);
                any = true;
            }
            if (!any) return false;
            ev.bursts.push_back({b.pos, R, b.color, nullptr});
            return true;
        }

        case Ability::Bounty: {   // a price on the strongest head
            Enemy* best = nullptr;
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
