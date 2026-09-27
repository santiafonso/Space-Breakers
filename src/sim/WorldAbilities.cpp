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
        if (fireAbility(b, a, p, ev)) {
            b.abilityCd[i] = abilityCooldown(a.id, a.level);
            b.abilityFlash = 1.f;
            mageOnCast(b, i, false, p, ev);   // Mage items / Ancient Mage react to the cast
        } else {
            b.abilityCd[i] = 0.f;   // charged: fires the moment it has a target
        }
    }
    mageTick(b, dt, p, ev);   // Mage: arcane missiles, Twincast echoes
}

bool World::fireAbility(Ball& b, const AbilitySpec& a, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::ability;
    const float n = static_cast<float>(std::max(0, a.level - 1));   // levels past the first
    const float as = arenaScale();
    const float pw = b.mods.cls.mage.power;              // Mage "Attunement": damage x this...
    const float reach = 1.f + (pw - 1.f) * 0.5f;         // ...radius / duration x this
    const bool anyEnemy = !enemies_.empty() || (boss_.alive && boss_.intro <= 0.f);
    switch (a.id) {
        case Ability::None: return false;

        case Ability::Dash: {   // a burst straight at the nearest enemy
            if (b.mods.satellite) return false;   // an orbit can't dash
            sf::Vector2f target;
            if (const Enemy* e = nearestEnemy(b.pos, A::dashRange * as)) target = e->pos;
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
                g.ghostLife = (A::splitLife + A::splitLifePerLevel * n) * reach;
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
    }
    return false;
}

}  // namespace sb
