// Class hooks (the class framework, 2026-09-26). Everything a class does
// inside the simulation lives here, one section per class; World.cpp only
// calls the dispatchers at the bottom (one line per event).
//
// A section is a ClassHooks<BallRole::X> specialisation deriving from
// NoClassHooks (sim/World.hpp): it hides the hooks it needs and leaves the rest
// as no-ops. The hooks are static and get the World; ClassHooks is a friend of
// World, so they reach enemies_, balls_, ghosts_, damageEnemy, strike, rng_...
// directly. Inside a hook:
//   - b.hasRole(BallRole::X) is always true (hooks only run for the ball's
//     own classes); b.isAscended(BallRole::X) = 4 items of the tag.
//   - b.ghost: a Split shot / Mitosis / Gemini / Split-ability copy.
//   - b.mods.cls.x: what the ball's X items add up to (core/ClassSpec.cpp),
//     b.cls.x: per-ball state, w.classWorld_.x: world state (sim/Classes.hpp).
//   - tuning goes in cfg::x (core/ConfigClasses.hpp).

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

// ==================================================================== Striker
// Base: flung hits scale with speed (World::ballDamage, releaseHeld,
// regulateSpeed). Mega Striker: a hit well above cruise throws a shockwave.
template <> struct ClassHooks<BallRole::Striker> : NoClassHooks {
    static void onHit(World& w, Ball& b, Enemy& e, float dmg, bool, const WorldParams& p, FrameEvents& ev) {
        namespace S = cfg::synergy;
        if (!b.isAscended(BallRole::Striker) || length(b.vel) <= w.ballCruise(b, p) * S::strikerShockSpeed) return;
        w.areaDamage(e.pos, S::strikerShockRadius, dmg * S::strikerShockFrac, &e);
        ev.bursts.push_back({e.pos, S::strikerShockRadius, theme::ballFast, nullptr});
    }
};

// ==================================================================== Guardian
// Base: big, shoves harder, aims its bounces, smashes shields (World.cpp) and
// staggers what it hits. Iron Guardian: every core bounce pulses.
template <> struct ClassHooks<BallRole::Guardian> : NoClassHooks {
    static void onHit(World&, Ball&, Enemy& e, float, bool, const WorldParams&, FrameEvents&) {
        e.stagger = std::max(e.stagger, cfg::role::staggerDuration);
    }
    static void onCoreBounce(World& w, Ball& b, sf::Vector2f, const WorldParams&, FrameEvents& ev) {
        if (b.isAscended(BallRole::Guardian)) w.guardianPulse(ev);
    }
};

// ==================================================================== Support
// Base: weak hits, a stronger element (World.cpp) and a mark on what it hits
// (every ball hits a marked enemy harder). Grand Support: the mark spreads.
template <> struct ClassHooks<BallRole::Support> : NoClassHooks {
    static void onHit(World& w, Ball& b, Enemy& e, float, bool, const WorldParams&, FrameEvents&) {
        e.mark = cfg::role::markDuration;
        if (!b.isAscended(BallRole::Support)) return;
        for (Enemy& o : w.enemies_)
            if (&o != &e && o.hp > 0.f && length(o.pos - e.pos) < cfg::synergy::supportSpread)
                o.mark = cfg::role::markDuration;
    }
};

// ==================================================================== Mage
// More ability slots: see abilitySlotCount (progression/Offers.hpp). Base: its
// hits feed its abilities (each one takes cfg::mage::hitRefund off every
// cooldown). Ancient Mage: every cast lets out an arcane nova (mageOnCast).
// Its signature is the Magic missile ability (a Mage ball is handed one:
// App::grantMageMissile); the missiles fly here, in worldTick.
// Its items act through casts and cooldowns, so they work on ANY ball that
// holds them (Mage class or not): World::mageCastRate / mageOnCast / mageTick
// below, called from sim/WorldAbilities.cpp.
template <> struct ClassHooks<BallRole::Mage> : NoClassHooks {
    using Missile = MageWorld::Missile;

    static void onHit(World&, Ball& b, Enemy&, float, bool, const WorldParams&, FrameEvents&) {
        if (b.ghost) return;   // copies never cast
        for (float& cd : b.abilityCd) cd = std::max(0.f, cd - cfg::mage::hitRefund);
    }

    static Enemy* findEnemy(World& w, int id) {
        for (Enemy& e : w.enemies_)
            if (e.id == id && e.hp > 0.f) return &e;
        return nullptr;
    }

    // Magic missiles: steer toward their target (a new one when it dies),
    // speed up, hit the first enemy they touch.
    static void worldTick(World& w, float dt, const WorldParams& p, FrameEvents& ev) {
        namespace A = cfg::ability;
        auto& ms = w.classWorld_.mage.missiles;
        if (ms.empty()) return;
        const float as = w.arenaScale();
        const float mr = A::missileRadius * as;
        for (Missile& m : ms) {
            m.life -= dt;
            if (m.life <= 0.f) continue;
            Enemy* t = findEnemy(w, m.target);
            if (!t) {   // lost its target: the nearest one to the missile
                const Enemy* n = w.nearestEnemy(m.pos, A::missileRetarget * as);
                m.target = n ? n->id : -1;
                t = findEnemy(w, m.target);
            }
            sf::Vector2f aim = m.pos + m.vel;
            if (t) aim = t->pos;
            else if (w.boss_.alive && w.boss_.intro <= 0.f) aim = w.boss_.pos;
            // turn toward the aim point at a capped rate, then speed up
            const float sp = std::min(length(m.vel) + A::missileAccel * as * dt, A::missileTopSpeed * as);
            const float cur = std::atan2(m.vel.y, m.vel.x);
            const sf::Vector2f to = aim - m.pos;
            float d = std::atan2(to.y, to.x) - cur;
            while (d > kPi) d -= 2.f * kPi;
            while (d < -kPi) d += 2.f * kPi;
            const float step = A::missileTurn * dt;
            const float ang = cur + std::clamp(d, -step, step);
            m.vel = sf::Vector2f{std::cos(ang), std::sin(ang)} * sp;
            m.pos += m.vel * dt;
            m.trailT -= dt;
            if (m.trailT <= 0.f) {   // a short curve behind it
                m.trailT = 0.02f;
                for (int i = std::min(m.trailN, 5); i > 0; --i) m.trail[i] = m.trail[i - 1];
                m.trail[0] = m.pos;
                m.trailN = std::min(m.trailN + 1, 6);
            }
            Enemy* hit = nullptr;
            for (Enemy& e : w.enemies_) {
                if (e.hp <= 0.f) continue;
                const sf::Vector2f q = m.pos - e.pos;
                const float rr = e.radius + mr;
                if (dot(q, q) < rr * rr) { hit = &e; break; }
            }
            if (hit) {
                w.damageEnemy(*hit, m.dmg * (hit->mark > 0.f ? p.markMul : 1.f));
                if (m.elem != 0) w.applyElement(*hit, static_cast<Element>(m.elem), m.owner, m.dmg, p, ev);
                m.life = 0.f;
            } else if (w.boss_.alive && w.boss_.intro <= 0.f && length(m.pos - w.boss_.pos) < w.boss_.radius + mr) {
                w.boss_.hp -= m.dmg * A::missileBossFrac;
                w.boss_.hitFlash = std::max(w.boss_.hitFlash, 0.4f);
                m.life = 0.f;
            }
        }
        ms.erase(std::remove_if(ms.begin(), ms.end(), [](const Missile& m) { return m.life <= 0.f; }), ms.end());
    }

    static void waveStart(World& w, const WorldParams&) {
        w.classWorld_.mage.missiles.clear();
        for (Ball& b : w.balls_) b.cls.mage.echoSlot = -1;
    }

    // Ancient Mage: an arcane nova around the ball on every cast.
    static void arcaneNova(World& w, Ball& b, const WorldParams& p, FrameEvents& ev) {
        namespace M = cfg::mage;
        const float R = M::novaRadius * w.arenaScale();
        const float dmg = w.ballDamage(b, p) * M::novaFrac * b.mods.cls.mage.power;
        const Element el = w.hitElement(b, p);
        bool any = false;
        for (Enemy& e : w.enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const sf::Vector2f d = e.pos - b.pos;
            if (length(d) > R + e.radius) continue;
            w.damageEnemy(e, dmg);
            e.vel += normalized(d, {1.f, 0.f}) * M::novaKnock * e.knockTaken;
            w.applyElement(e, el, b.owner, dmg, p, ev);
            any = true;
        }
        if (any) ev.bursts.push_back({b.pos, R, theme::classMage, nullptr});
    }
};

float World::mageCastRate(const Ball& b) const {   // "Focus", Channel, Archive; "Meditate" when still
    return 1.f + b.mods.cls.mage.focus + b.mods.cls.mage.meditate * b.cls.mage.still;
}

// `count` missiles at the nearest enemies (spread over them, nearest first),
// each for `frac` x the ball's hit. False when nothing is in range.
bool World::mageMissiles(Ball& b, int count, float frac, const WorldParams& p) {
    namespace A = cfg::ability;
    const float as = arenaScale();
    const float range = A::missileRange * as;
    std::vector<const Enemy*> near;
    for (const Enemy& e : enemies_)
        if (e.hp > 0.f && !e.orbiter && length(e.pos - b.pos) < range) near.push_back(&e);
    const bool boss = boss_.alive && boss_.intro <= 0.f && length(boss_.pos - b.pos) < range * 1.5f;
    if (near.empty() && !boss) return false;
    std::sort(near.begin(), near.end(), [&b](const Enemy* x, const Enemy* y) {
        return dot(x->pos - b.pos, x->pos - b.pos) < dot(y->pos - b.pos, y->pos - b.pos);
    });
    auto& ms = classWorld_.mage.missiles;
    const float dmg = ballDamage(b, p) * frac * b.mods.cls.mage.power * b.mods.cls.mage.missileMul;
    const Element el = hitElement(b, p);
    const sf::Vector2f head = normalized(b.vel, {0.f, -1.f});
    for (int i = 0; i < count && static_cast<int>(ms.size()) < A::maxMissiles; ++i) {
        MageWorld::Missile m;
        m.pos = b.pos;
        // fan out sideways from its heading, then curve in
        const float side = (i % 2 == 0 ? 1.f : -1.f) * (0.9f + 0.35f * static_cast<float>(i / 2));
        const float c = std::cos(side), sn = std::sin(side);
        m.vel = sf::Vector2f{head.x * c - head.y * sn, head.x * sn + head.y * c} * (A::missileSpeed * as);
        m.dmg = dmg;
        m.life = A::missileLife;
        m.elem = static_cast<int>(el);
        m.owner = b.owner;
        m.target = near.empty() ? -1 : near[static_cast<std::size_t>(i) % near.size()]->id;
        m.trail[0] = m.pos;
        m.trailN = 1;
        ms.push_back(m);
    }
    return true;
}

void World::mageOnCast(Ball& b, int slot, bool echo, const WorldParams& p, FrameEvents& ev) {
    using H = ClassHooks<BallRole::Mage>;
    const MageMods& g = b.mods.cls.mage;
    styleOnCast(b, p, ev);   // "Leyline": its runes burst
    // "Barrage": any other ability looses a magic missile too
    if (g.barrage > 0 && b.abilities[slot].id != Ability::MagicMissile) mageMissiles(b, 1, g.barrageFrac, p);
    if (b.isAscended(BallRole::Mage)) H::arcaneNova(*this, b, p, ev);
    if (echo) return;   // an echo doesn't echo again or spring the others
    if (g.twincast > 0.f && b.cls.mage.echoSlot < 0 && chance(g.twincast, p)) {   // "Twincast"
        b.cls.mage.echoSlot = slot;
        b.cls.mage.echoT = cfg::mage::echoDelay;
    }
    if (g.manaSpring > 0.f)   // "Mana spring": the other abilities get closer
        for (int j = 0; j < kMaxAbilitySlots; ++j)
            if (j != slot && b.abilities[j].id != Ability::None)
                b.abilityCd[j] = std::max(0.f, b.abilityCd[j] - abilityCooldown(b.abilities[j].id, b.abilities[j].level) * g.manaSpring);
}

void World::mageTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    MageState& s = b.cls.mage;
    if (s.echoSlot >= 0) {   // "Twincast": the echo fires
        s.echoT -= dt;
        if (s.echoT <= 0.f) {
            const int slot = s.echoSlot;
            s.echoSlot = -1;
            const AbilitySpec a = b.abilities[slot];
            if (a.id != Ability::None && fireAbility(b, a, p, ev)) {
                b.abilityFlash = 1.f;
                mageOnCast(b, slot, true, p, ev);
            }
        }
    }
}

// ==================================================================== Shooter
// Base: a volley at the nearest enemy in range every so often, faster the
// faster the ball flies. Bullets are small and weak (a share of the ball's own
// hit) and live in ShooterWorld; its items make them many, bouncy, piercing,
// elemental or reactive. Deadeye: every Nth volley is also a rail shot through
// the whole line, and every bullet hops once more.
template <> struct ClassHooks<BallRole::Shooter> : NoClassHooks {
    using Bullet = ShooterBullet;

    // Where to shoot from `from`: the nearest enemy in range (led a little),
    // or the boss. False if there's nothing to shoot at.
    static bool aimAt(const World& w, sf::Vector2f from, float range, int skipId, sf::Vector2f& at) {
        const Enemy* best = nullptr;
        float bestD2 = range * range;
        for (const Enemy& e : w.enemies_) {
            if (e.hp <= 0.f || e.orbiter || e.id == skipId) continue;
            const float d2 = dot(e.pos - from, e.pos - from);
            if (d2 < bestD2) { bestD2 = d2; best = &e; }
        }
        if (best) {
            const float t = std::sqrt(bestD2) / (cfg::shooter::bulletSpeed * w.arenaScale());
            at = best->pos + best->vel * t;
            return true;
        }
        if (skipId < 0 && w.boss_.alive && w.boss_.intro <= 0.f &&
            length(w.boss_.pos - from) < range + w.boss_.radius) {
            at = w.boss_.pos;
            return true;
        }
        return false;
    }

    static void fire(World& w, const Ball& b, sf::Vector2f dir, float dmg, const WorldParams& p) {
        auto& bullets = w.classWorld_.shooter.bullets;
        if (static_cast<int>(bullets.size()) >= cfg::shooter::maxBullets) return;
        const ShooterMods& m = b.mods.cls.shooter;
        Bullet u;
        u.pos = b.pos + dir * b.radius;
        u.vel = dir * (cfg::shooter::bulletSpeed * w.arenaScale());
        u.dmg = dmg * m.dmgMul;
        u.life = cfg::shooter::bulletLife;
        u.owner = b.owner;
        u.pierce = m.pierce;
        u.hops = m.hops + (b.isAscended(BallRole::Shooter) ? cfg::shooter::deadeyeHops : 0);
        u.hopKeep = m.hopKeep;
        if (m.tracerChance > 0.f && b.element != Element::Plain && w.chance(m.tracerChance, p))
            u.elem = static_cast<int>(b.element);
        bullets.push_back(u);
    }

    // Deadeye: a straight shot through everything from the ball to the wall.
    static void rail(World& w, const Ball& b, sf::Vector2f dir, const WorldParams& p, FrameEvents& ev) {
        namespace S = cfg::shooter;
        float t = 1e9f;
        const sf::Vector2f sz = w.size_;
        if (dir.x > 1e-4f) t = std::min(t, (sz.x - b.pos.x) / dir.x);
        if (dir.x < -1e-4f) t = std::min(t, -b.pos.x / dir.x);
        if (dir.y > 1e-4f) t = std::min(t, (sz.y - b.pos.y) / dir.y);
        if (dir.y < -1e-4f) t = std::min(t, -b.pos.y / dir.y);
        t = clampf(t, 0.f, 4000.f);
        const float dmg = w.ballDamage(b, p) * S::deadeyeRailFrac * b.mods.cls.shooter.dmgMul;
        const float hw = S::deadeyeRailWidth * w.arenaScale();
        const Element el = w.hitElement(b, p);
        for (Enemy& e : w.enemies_) {
            if (e.hp <= 0.f) continue;
            const float along = clampf(dot(e.pos - b.pos, dir), 0.f, t);
            if (length(e.pos - (b.pos + dir * along)) > hw + e.radius) continue;
            w.damageEnemy(e, dmg * (e.mark > 0.f ? p.markMul : 1.f));
            w.applyElement(e, el, b.owner, dmg, p, ev);
        }
        auto& rails = w.classWorld_.shooter.rails;
        if (rails.size() < 8) rails.push_back({b.pos, b.pos + dir * t, S::deadeyeRailLife});
    }

    static void tick(World& w, Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
        namespace S = cfg::shooter;
        ShooterState& st = b.cls.shooter;
        const ShooterMods& m = b.mods.cls.shooter;
        st.burstCd = std::max(0.f, st.burstCd - dt);
        if (!w.waveRunning_) { st.fireT = S::firstDelay; return; }
        const float still = b.cls.mage.still;
        if (m.strafe > 0.f) {   // "Strafe": fast, volleys out to both sides
            const float fast = w.styleFast(b, p);
            if (fast > 0.f && (st.strafeT -= dt * (0.5f + fast)) <= 0.f) {
                st.strafeT = cfg::style::strafeEvery;
                const sf::Vector2f h = normalized(b.vel, {1.f, 0.f});
                const sf::Vector2f side{-h.y, h.x};
                const float d = w.ballDamage(b, p) * S::bulletFrac * m.strafe;
                fire(w, b, side, d, p);
                fire(w, b, -side, d, p);
            }
        }
        float speedRate = clampf(length(b.vel) / std::max(1.f, w.ballCruise(b, p)), S::rateMin, S::rateMax);
        if (m.slug > 0.f) speedRate = std::max(speedRate, 1.f) * (1.f + m.slug * still);   // "Slug": slow = fast fire
        st.fireT -= dt * speedRate * m.rate;
        if (st.fireT > 0.f) return;
        sf::Vector2f at;
        const float range = S::range * w.arenaScale() * (1.f + 0.5f * m.slug * still);   // "Slug" reaches further
        if (!aimAt(w, b.pos, range, -1, at)) { st.fireT = 0.f; return; }   // wait, loaded
        st.fireT += S::fireInterval;
        if (st.fireT < 0.f) st.fireT = 0.f;
        const sf::Vector2f dir = normalized(at - b.pos, {1.f, 0.f});
        const float dmg = w.ballDamage(b, p) * S::bulletFrac * (m.pellets > 1 ? m.pelletFrac : 1.f);
        const float base = std::atan2(dir.y, dir.x);
        for (int i = 0; i < m.pellets; ++i) {
            const float a = base + (static_cast<float>(i) - 0.5f * static_cast<float>(m.pellets - 1)) * S::scatterSpread;
            fire(w, b, {std::cos(a), std::sin(a)}, dmg, p);
        }
        if (b.isAscended(BallRole::Shooter) && ++st.volleys % S::deadeyeEvery == 0) rail(w, b, dir, p, ev);
    }

    // "Hair trigger": a landed hit sprays a burst at the enemies around it.
    static void onHit(World& w, Ball& b, Enemy& e, float, bool, const WorldParams& p, FrameEvents&) {
        namespace S = cfg::shooter;
        const ShooterMods& m = b.mods.cls.shooter;
        if (m.burst <= 0 || b.cls.shooter.burstCd > 0.f) return;
        b.cls.shooter.burstCd = S::triggerCooldown;
        const float dmg = w.ballDamage(b, p) * S::bulletFrac * m.burstFrac;
        // Up to `burst` nearest others; any left over go out in a ring.
        const Enemy* picked[12] = {};
        const int want = std::min(m.burst, 12);
        int n = 0;
        const float R = S::range * w.arenaScale();
        for (; n < want; ++n) {
            const Enemy* best = nullptr;
            float bestD2 = R * R;
            for (const Enemy& o : w.enemies_) {
                if (&o == &e || o.hp <= 0.f || o.orbiter) continue;
                if (std::find(picked, picked + n, &o) != picked + n) continue;
                const float d2 = dot(o.pos - b.pos, o.pos - b.pos);
                if (d2 < bestD2) { bestD2 = d2; best = &o; }
            }
            if (!best) break;
            picked[n] = best;
            fire(w, b, normalized(best->pos - b.pos, {1.f, 0.f}), dmg, p);
        }
        const float spin = w.rng_.range(0.f, 2.f * kPi);
        for (int i = n; i < want; ++i) {
            const float a = spin + 2.f * kPi * static_cast<float>(i) / static_cast<float>(want);
            fire(w, b, {std::cos(a), std::sin(a)}, dmg, p);
        }
    }

    // Bullets fly, hit, pierce, hop; rails fade.
    static void worldTick(World& w, float dt, const WorldParams& p, FrameEvents& ev) {
        namespace S = cfg::shooter;
        ShooterWorld& sw = w.classWorld_.shooter;
        for (ShooterRail& r : sw.rails) r.life -= dt;
        sw.rails.erase(std::remove_if(sw.rails.begin(), sw.rails.end(), [](const ShooterRail& r) { return r.life <= 0.f; }),
                       sw.rails.end());
        if (sw.bullets.empty()) return;
        const float br = S::bulletRadius * w.arenaScale();
        const sf::Vector2f sz = w.size_;
        for (Bullet& u : sw.bullets) {
            u.life -= dt;
            u.pos += u.vel * dt;
            if (u.pos.x < 0.f || u.pos.y < 0.f || u.pos.x > sz.x || u.pos.y > sz.y) u.life = 0.f;
            if (u.life <= 0.f) continue;
            Enemy* hit = nullptr;
            for (Enemy& e : w.enemies_) {
                if (e.hp <= 0.f || e.id == u.lastHit) continue;
                const sf::Vector2f d = u.pos - e.pos;
                const float rr = e.radius + br;
                if (dot(d, d) < rr * rr) { hit = &e; break; }
            }
            if (!hit) {
                Boss& bo = w.boss_;
                if (bo.alive && bo.intro <= 0.f && length(u.pos - bo.pos) < bo.radius + br) {
                    bo.hp -= u.dmg * S::bossFrac;
                    bo.hitFlash = std::max(bo.hitFlash, 0.4f);
                    u.life = 0.f;
                }
                continue;
            }
            Enemy& e = *hit;
            if (u.pierce <= 0 && World::shieldBlocks(e, u.pos - u.vel * 0.05f, w.core_.pos)) {
                u.life = 0.f;   // the shield eats it (Drill rounds go through)
                continue;
            }
            const float flash = e.hitFlash;
            w.damageEnemy(e, u.dmg * (e.mark > 0.f ? p.markMul : 1.f));
            e.hitFlash = std::max(flash, 0.55f);   // a bullet is a tick, not a big hit
            if (u.elem != 0) w.applyElement(e, static_cast<Element>(u.elem), u.owner, u.dmg, p, ev);
            u.lastHit = e.id;
            if (u.pierce > 0) { --u.pierce; continue; }
            if (u.hops > 0) {
                sf::Vector2f at;
                if (aimAt(w, e.pos, S::reboundRange * w.arenaScale(), e.id, at)) {
                    --u.hops;
                    u.dmg *= u.hopKeep;
                    u.vel = normalized(at - e.pos, {1.f, 0.f}) * length(u.vel);
                    u.life = std::max(u.life, S::bulletLife * 0.6f);
                    continue;
                }
            }
            u.life = 0.f;
        }
        sw.bullets.erase(std::remove_if(sw.bullets.begin(), sw.bullets.end(), [](const Bullet& u) { return u.life <= 0.f; }),
                         sw.bullets.end());
    }

    static void waveStart(World& w, const WorldParams&) {
        w.classWorld_.shooter.bullets.clear();
        w.classWorld_.shooter.rails.clear();
    }
};

// ==================================================================== Assassin
// Base: a kill queues a blink; on its next tick the ball teleports next to the
// nearest enemy, aimed at it at no less than its cruise. A short cooldown (and
// dropping blinks queued too long ago) stops loops; never while held or on a
// Satellite, never into the core, a wall or another enemy. The items hang off
// the blink. Shadow Assassin: the blink first cuts through a chain of enemies,
// and comes twice as often.
template <> struct ClassHooks<BallRole::Assassin> : NoClassHooks {
    static float cooldown(const Ball& b) {
        return cfg::assassin::blinkCooldown * (b.isAscended(BallRole::Assassin) ? cfg::assassin::ascCooldownMul : 1.f);
    }

    // The live enemy nearest `from` inside the arena, not already in `skip`.
    static Enemy* pick(World& w, sf::Vector2f from, float range, const std::vector<Enemy*>& skip) {
        Enemy* best = nullptr;
        float bestD2 = range * range;
        for (Enemy& e : w.enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            if (e.pos.x < 0.f || e.pos.y < 0.f || e.pos.x > w.size_.x || e.pos.y > w.size_.y) continue;
            if (std::find(skip.begin(), skip.end(), &e) != skip.end()) continue;
            const float d2 = dot(e.pos - from, e.pos - from);
            if (d2 < bestD2) { bestD2 = d2; best = &e; }
        }
        return best;
    }

    // Where to land next to `t`: on the side facing `from`, turned a little
    // if that spot is in a wall, the core or another enemy.
    static bool landing(const World& w, const Ball& b, const Enemy& t, sf::Vector2f from, sf::Vector2f& out) {
        const float dist = t.radius + b.radius + cfg::assassin::blinkStandoff * w.arenaScale();
        const sf::Vector2f d0 = from - t.pos;
        const float base = dot(d0, d0) > 1e-6f ? std::atan2(d0.y, d0.x) : 0.f;
        for (float off : {0.f, 0.7f, -0.7f, 1.4f, -1.4f, 2.1f, -2.1f, kPi}) {
            const sf::Vector2f c = t.pos + sf::Vector2f{std::cos(base + off), std::sin(base + off)} * dist;
            if (c.x < b.radius || c.y < b.radius || c.x > w.size_.x - b.radius || c.y > w.size_.y - b.radius) continue;
            if (length(c - w.core_.pos) < w.core_.radius + b.radius + 4.f) continue;
            bool clear = true;
            for (const Enemy& e : w.enemies_)
                if (&e != &t && e.hp > 0.f && length(c - e.pos) < e.radius + b.radius) { clear = false; break; }
            if (clear) { out = c; return true; }
        }
        return false;
    }

    // Everything within reach of the segment a-c takes `dmg` (but `spare`).
    static void cut(World& w, sf::Vector2f a, sf::Vector2f c, float dmg, const Enemy* spare) {
        const float half = cfg::assassin::trailWidth * w.arenaScale();
        const sf::Vector2f ac = c - a;
        const float len2 = std::max(dot(ac, ac), 1e-6f);
        for (Enemy& e : w.enemies_) {
            if (&e == spare || e.hp <= 0.f) continue;
            const float t = clampf(dot(e.pos - a, ac) / len2, 0.f, 1.f);
            if (length(e.pos - (a + ac * t)) < half + e.radius) w.damageEnemy(e, dmg);
        }
    }

    static void addFx(World& w, sf::Vector2f a, sf::Vector2f c, float r, bool cuts) {
        std::vector<AssassinWorld::Blink>& fx = w.classWorld_.assassin.blinks;
        if (static_cast<int>(fx.size()) >= cfg::assassin::maxBlinkFx) fx.erase(fx.begin());
        fx.push_back({a, c, cfg::assassin::blinkFxLife, r, cuts});
    }

    // "Phantom": a shadow copy stays where it left and dives at another enemy.
    static void phantom(World& w, const Ball& b, Enemy& t, float life, float speed) {
        if (static_cast<int>(w.ghosts_.size() + w.pendingGhosts_.size()) >= cfg::synergy::maxGhosts) return;
        Ball g = b;
        g.ghost = true;
        g.twin = false;
        g.ghostLife = life * b.mods.copyLife;   // web "Brood"
        g.age = 0.f;
        g.held = false;
        g.trail.clear();
        g.waterTrail.clear();
        g.cls.assassin = AssassinState{};
        g.cls.assassin.cd = cooldown(b);
        const Enemy* aim = pick(w, b.pos, cfg::assassin::blinkRange * w.arenaScale(), {&t});
        g.vel = normalized((aim ? aim->pos : t.pos) - b.pos, b.vel) * speed;
        w.pendingGhosts_.push_back(g);
    }

    static void blink(World& w, Ball& b, const WorldParams& p, FrameEvents& ev) {
        namespace A = cfg::assassin;
        const AssassinMods& m = b.mods.cls.assassin;
        AssassinState& s = b.cls.assassin;
        const float range = A::blinkRange * w.arenaScale();
        const int reach = 1 + (b.isAscended(BallRole::Assassin) ? A::ascChain : 0);
        std::vector<Enemy*> chain;
        for (sf::Vector2f at = b.pos; static_cast<int>(chain.size()) < reach;) {
            Enemy* e = pick(w, at, range, chain);
            if (!e) break;
            chain.push_back(e);
            at = e->pos;
        }
        sf::Vector2f dest;
        while (!chain.empty() &&
               !landing(w, b, *chain.back(), chain.size() > 1 ? chain[chain.size() - 2]->pos : b.pos, dest))
            chain.pop_back();   // no room next to the last one: stop the chain short
        if (chain.empty()) return;
        Enemy& t = *chain.back();

        const float hit = w.ballDamage(b, p);   // before Backstab arms
        const float speed = std::min(std::max(length(b.vel), w.ballCruise(b, p) * A::blinkMinSpeed), w.ballMaxSpeed(b, p));
        if (m.phantomLife > 0.f && !b.ghost) phantom(w, b, t, m.phantomLife, speed);
        sf::Vector2f from = b.pos;
        for (std::size_t i = 0; i + 1 < chain.size(); ++i) {   // Shadow Assassin: cut through the chain
            Enemy& e = *chain[i];
            w.damageEnemy(e, hit * A::ascSlashFrac);
            w.applyElement(e, w.hitElement(b, p), b.owner, hit, p, ev);
            if (m.trailFrac > 0.f) cut(w, from, e.pos, hit * m.trailFrac, &e);
            addFx(w, from, e.pos, i == 0 ? b.radius : 0.f, true);   // the afterimage only where it left
            from = e.pos;
        }
        if (m.trailFrac > 0.f) cut(w, from, dest, hit * m.trailFrac, &t);   // "Shadow trail"
        addFx(w, from, dest, chain.size() > 1 ? 0.f : b.radius, m.trailFrac > 0.f || chain.size() > 1);

        b.pos = dest;
        b.vel = normalized(t.pos - dest, b.vel) * speed;
        b.trail.clear();
        b.waterTrail.clear();
        b.squash = 1.f;
        b.squashAxis = normalized(b.vel);

        if (m.smokeFrac > 0.f) {   // "Smoke bomb": a burst where it lands, with its element
            const float R = m.smokeRadius * w.arenaScale();
            const Element el = w.hitElement(b, p);
            for (Enemy& e : w.enemies_) {
                if (e.hp <= 0.f || length(e.pos - dest) > R + e.radius) continue;
                w.damageEnemy(e, hit * m.smokeFrac);
                w.applyElement(e, el, b.owner, hit, p, ev);
            }
            ev.bursts.push_back({dest, R, el == Element::Plain ? theme::classAssassin : elementColor(el), nullptr});
        }
        if (m.backstab > 0.f) s.armedT = A::backstabWindow;
        s.blinked = true;   // "Lurk": the next hit spends the charge
        if (m.spreePer > 0.f) {
            s.spree = std::min(s.spree + 1, m.spreeMax);
            s.spreeT = A::spreeWindow;
        }
        s.cd = cooldown(b);
    }

    static void tick(World& w, Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
        AssassinState& s = b.cls.assassin;
        s.cd = std::max(0.f, s.cd - dt);
        s.armedT = std::max(0.f, s.armedT - dt);
        if (s.spreeT > 0.f && (s.spreeT -= dt) <= 0.f) s.spree = 0;
        if (!s.pending) return;
        if (w.classWorld_.assassin.clock - s.pendingAt > cfg::assassin::blinkStale || b.held || b.mods.satellite) {
            s.pending = false;   // stale (it was held, or the cooldown ran long): drop it
            return;
        }
        if (s.cd > 0.f || !w.waveRunning_) return;
        s.pending = false;
        blink(w, b, p, ev);
    }

    static void onKill(World& w, Ball& b, Enemy&, float, const WorldParams&, FrameEvents&) {
        b.cls.assassin.pending = true;
        b.cls.assassin.pendingAt = w.classWorld_.assassin.clock;
    }

    static void onHit(World& w, Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev) {
        b.cls.assassin.armedT = 0.f;   // "Backstab" is spent
        if (b.cls.assassin.blinked) {  // "Lurk" too
            b.cls.assassin.blinked = false;
            b.cls.assassin.lurk = 0.f;
        }
        const float cull = b.mods.cls.assassin.cull;   // "Cull": finish it off (the kill blinks on)
        if (!kill && cull > 0.f && e.hp > 0.f && e.hp < e.maxHp * cull) {
            w.damageEnemy(e, e.hp);
            w.onKill(b, e, dmg, p, ev);
        }
    }

    static float damageMul(const World&, const Ball& b, const WorldParams&) {
        const AssassinMods& m = b.mods.cls.assassin;
        const AssassinState& s = b.cls.assassin;
        float k = 1.f + m.spreePer * static_cast<float>(s.spree);   // "Killing spree"
        if (s.armedT > 0.f && m.backstab > 0.f) k *= m.backstab;    // "Backstab"
        if (s.blinked && m.lurk > 0.f) k *= 1.f + m.lurk * s.lurk;  // "Lurk": charged while slow
        return k;
    }

    static void worldTick(World& w, float dt, const WorldParams&, FrameEvents&) {
        AssassinWorld& aw = w.classWorld_.assassin;
        aw.clock += dt;
        for (AssassinWorld::Blink& f : aw.blinks) f.life -= dt;
        aw.blinks.erase(std::remove_if(aw.blinks.begin(), aw.blinks.end(),
                                       [](const AssassinWorld::Blink& f) { return f.life <= 0.f; }),
                        aw.blinks.end());
    }

    static void waveStart(World& w, const WorldParams&) { w.classWorld_.assassin.blinks.clear(); }
};

// ==================================================================== Summoner
// Class: every few seconds the ball calls a SPRITELING - a small, plain,
// short-lived ball of its element, launched at the nearest enemy. Items, each
// a different helper: Turret (a wall bounce plants one), Wisps (kills near the
// ball let loose homing wisps), Totem (slows and pulses), Warden (spirits
// circling the core), Dragonling (breathes the ball's element in a cone).
// Items work on any ball; the class makes every summon hit harder and last
// longer. Archsummoner: spritelings twice as often, more of them, carrying the
// ball's items, and every summon hit leaves the element's full effect
// (freeze / poison / burn), not just the reaction primer.
//
// Everything runs from worldTick over the REAL balls (so a single Summoner item
// works on a ball without the class, and copies / twins / spritelings never
// summon). A wall bounce is read off the velocity flipping next to a wall.
template <> struct ClassHooks<BallRole::Summoner> : NoClassHooks {
    static float power(const Ball& b) {
        namespace S = cfg::summoner;
        const float cls = b.isAscended(BallRole::Summoner) ? S::ascendedPower
                        : b.hasRole(BallRole::Summoner)  ? S::rolePower
                                                         : 1.f;
        return cls * b.mods.cls.summoner.bond;   // web "Bond"
    }

    // One summon hit on an enemy: damage + its element.
    static void touch(World& w, Enemy& e, float dmg, int elem, int owner, bool full, const WorldParams& p,
                      FrameEvents& ev) {
        namespace E = cfg::element;
        w.damageEnemy(e, dmg);
        const Element el = static_cast<Element>(elem);
        if (el == Element::Plain) return;
        if (full) {   // the element's own effect, like a ball hit
            const float pot = p.elemMult[elem];
            if (el == Element::Poison) {
                e.poison = E::poisonDuration;
                e.poisonDps = std::min(e.poisonDps + E::poisonDpsPerHit * pot, E::poisonDpsMax * pot);
            } else if (el == Element::Ice) {
                e.frozen = std::max(e.frozen, cfg::summoner::dragonFreeze * pot);
            } else if (el == Element::Fire) {
                e.burn = E::burnDuration;
                e.burnDps = std::min(e.burnDps + E::burnPerHit * pot, E::burnMax * pot);
            } else if (el == Element::Water) {
                e.soak = std::max(e.soak, E::soakDuration * pot);
            } else if (el == Element::Stone) {
                w.crack(e, 1, E::crackTime * pot, E::crackMax);
            }
        }
        w.applyElement(e, el, owner, dmg, p, ev);
    }

    static void hitBoss(World& w, float dmg) {
        if (!w.boss_.alive || w.boss_.intro > 0.f) return;
        w.boss_.hp -= dmg * cfg::summoner::bossFrac;
        w.boss_.hitFlash = 1.f;
    }

    // The nearest live enemy (or the boss) to a point, within range.
    static bool target(const World& w, sf::Vector2f from, float range, sf::Vector2f& out) {
        const Enemy* e = w.nearestEnemy(from, range);
        float best = e ? length(e->pos - from) : range;
        if (e) out = e->pos;
        if (w.boss_.alive && w.boss_.intro <= 0.f && w.boss_.hp > 0.f && length(w.boss_.pos - from) < best) {
            out = w.boss_.pos;
            return true;
        }
        return e != nullptr;
    }

    static int ownedBy(const std::vector<SummonTurret>& v, int owner) {
        int n = 0;
        for (const SummonTurret& t : v) n += t.owner == owner ? 1 : 0;
        return n;
    }

    static void spawnSprite(World& w, Ball& b, const WorldParams& p) {
        namespace S = cfg::summoner;
        const bool asc = b.isAscended(BallRole::Summoner);
        if (static_cast<int>(w.ghosts_.size() + w.pendingGhosts_.size()) >= cfg::synergy::maxGhosts) return;
        int mine = 0;
        for (const Ball& g : w.ghosts_) mine += (g.cls.summoner.sprite && g.owner == b.owner) ? 1 : 0;
        if (mine >= (asc ? S::maxSpritesAscended : S::maxSprites)) return;
        Ball g = b;
        g.ghost = true;
        g.twin = false;
        g.held = false;
        g.scale = S::spriteScale;
        g.ghostLife = S::spriteLife * power(b);
        g.age = 0.f;
        g.trail.clear();
        g.waterTrail.clear();
        g.charged = g.homing = false;
        g.gluttonStacks = g.berserkStacks = 0;
        g.preyId = -1;
        g.overclockT = 0.f;
        for (AbilitySpec& a : g.abilities) a = AbilitySpec{};
        g.cls = ClassState{};
        g.cls.summoner.sprite = true;
        if (asc) {   // Archsummoner: its spritelings carry its items
            g.mods.twins = 0;
            g.mods.damageMult *= S::spriteDamage * power(b);
        } else {     // a plain little ball of its element
            g.mods = BallMods{};
            g.mods.damageMult = S::spriteDamage * power(b);
            g.roles = g.ascended = 0;
        }
        sf::Vector2f to;
        const sf::Vector2f dir = target(w, b.pos, 1e9f, to) ? normalized(to - b.pos, w.rng_.direction())
                                                            : w.rng_.direction();
        g.pos = b.pos + dir * (b.radius * 0.5f);
        g.vel = dir * w.ballCruise(b, p) * S::spriteSpeed;
        w.ghosts_.push_back(g);   // worldTick runs after the ghost loop: safe to add directly
    }

    // Per real ball: timers, plants, the dragon and the wardens.
    static void tickBall(World& w, Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
        namespace S = cfg::summoner;
        const SummonerMods& m = b.mods.cls.summoner;
        SummonerState& s = b.cls.summoner;
        const bool role = b.hasRole(BallRole::Summoner);
        const bool full = b.isAscended(BallRole::Summoner);
        const float pw = power(b);
        const int elem = static_cast<int>(b.element);
        const bool fighting = !w.enemies_.empty() || (w.boss_.alive && w.boss_.intro <= 0.f);
        SummonerWorld& sw = w.classWorld_.summoner;

        // "Kennel": the slower it moves, the sooner a wisp.
        if (m.kennel > 0.f && fighting && !b.ghost && (s.kennelT += dt * b.cls.mage.still) >= m.kennel &&
            static_cast<int>(sw.shots.size()) < S::maxShots) {
            s.kennelT = 0.f;
            SummonShot q;
            q.pos = b.pos;
            q.vel = w.rng_.direction() * S::wispSpeed * 0.6f;
            q.dmg = w.ballDamage(b, p) * m.kennelFrac * pw;
            q.life = S::wispLife * pw;
            q.elem = elem;
            q.owner = b.owner;
            q.wisp = true;
            sw.shots.push_back(q);
        }
        // "Drop turret": you let go of it here.
        if (s.dropPending) {
            s.dropPending = false;
            if (static_cast<int>(sw.turrets.size()) < S::maxTurrets) {
                SummonTurret t;
                t.pos = {std::clamp(s.dropAt.x, S::turretInset, w.size_.x - S::turretInset),
                         std::clamp(s.dropAt.y, S::turretInset, w.size_.y - S::turretInset)};
                t.life = t.maxLife = cfg::style::dropLife * pw;
                t.rate = cfg::style::dropRate;
                t.dmg = w.ballDamage(b, p) * m.drop * pw;
                t.aim = std::atan2(w.core_.pos.y - t.pos.y, w.core_.pos.x - t.pos.x);
                t.elem = elem;
                t.owner = b.owner;
                sw.turrets.push_back(t);
            }
        }

        // The class: spritelings.
        if (role && fighting && (s.spriteT -= dt) <= 0.f) {
            spawnSprite(w, b, p);
            s.spriteT = full ? S::spriteIntervalAscended : S::spriteInterval;
        }

        // Turret: a wall bounce (the heading flipped right next to a wall).
        s.turretCd = std::max(0.f, s.turretCd - dt);
        if (m.turretFrac > 0.f && !b.held && !b.mods.satellite) {
            const float slop = b.radius + length(b.vel) * dt + 2.f;
            const sf::Vector2f sz = w.size_;
            const bool flipX = b.vel.x * s.prevVel.x < 0.f && (b.pos.x < slop || b.pos.x > sz.x - slop);
            const bool flipY = b.vel.y * s.prevVel.y < 0.f && (b.pos.y < slop || b.pos.y > sz.y - slop);
            if ((flipX || flipY) && s.turretCd <= 0.f && ownedBy(sw.turrets, b.owner) < m.turretMax &&
                static_cast<int>(sw.turrets.size()) < S::maxTurrets) {
                SummonTurret t;
                t.pos = {std::clamp(b.pos.x, S::turretInset, sz.x - S::turretInset),
                         std::clamp(b.pos.y, S::turretInset, sz.y - S::turretInset)};
                t.life = t.maxLife = m.turretLife * pw;
                t.rate = m.turretRate;
                t.dmg = w.ballDamage(b, p) * m.turretFrac * pw;
                t.aim = std::atan2(w.core_.pos.y - t.pos.y, w.core_.pos.x - t.pos.x);
                t.elem = elem;
                t.owner = b.owner;
                sw.turrets.push_back(t);
                s.turretCd = S::turretCooldown;
            }
        }
        s.prevVel = b.vel;

        // Totem: planted where the ball is once enemies are close.
        if (m.totemInterval > 0.f) {
            s.totemT = std::max(0.f, s.totemT - dt);
            int mine = 0;
            for (const SummonTotem& t : sw.totems) mine += t.owner == b.owner ? 1 : 0;
            if (s.totemT <= 0.f && mine < S::totemMax && static_cast<int>(sw.totems.size()) < S::maxTotems &&
                w.nearestEnemy(b.pos, m.totemRadius)) {
                SummonTotem t;
                t.pos = b.pos;
                t.life = t.maxLife = m.totemLife * pw;
                t.radius = m.totemRadius;
                t.slow = std::min(0.8f, m.totemSlow * (full ? 1.25f : 1.f));
                t.pulseT = S::totemPulse * 0.5f;
                t.dmg = w.ballDamage(b, p) * S::totemFrac * pw;
                t.elem = elem;
                t.owner = b.owner;
                sw.totems.push_back(t);
                s.totemT = m.totemInterval;
            }
        }

        // Dragonling: hovers beside the ball, breathes at the nearest enemy.
        if (m.dragonInterval > 0.f) {
            const float side = std::atan2(b.vel.y, b.vel.x) + kPi * 0.75f;   // behind-left of the heading
            const sf::Vector2f want = b.pos + sf::Vector2f{std::cos(side), std::sin(side)} *
                                                  (b.radius + S::dragonHover);
            if (!s.dragonOut) { s.dragonPos = want; s.dragonOut = true; }
            const sf::Vector2f prev = s.dragonPos;
            s.dragonPos += (want - s.dragonPos) * (1.f - std::exp(-S::dragonFollow * dt));
            const sf::Vector2f mv = s.dragonPos - prev;
            if (length(mv) > 0.2f) s.dragonHeading = std::atan2(mv.y, mv.x);
            sf::Vector2f to;
            if ((s.dragonT -= dt) <= 0.f && target(w, s.dragonPos, S::dragonRange, to)) {
                const float dir = std::atan2(to.y - s.dragonPos.y, to.x - s.dragonPos.x);
                s.dragonHeading = dir;
                const float dmg = w.ballDamage(b, p) * m.dragonFrac * pw;
                auto inCone = [&](sf::Vector2f at, float r) {
                    const sf::Vector2f d = at - s.dragonPos;
                    const float dl = length(d);
                    if (dl > S::dragonRange + r) return false;
                    if (dl < r + 4.f) return true;
                    return std::fabs(std::remainder(std::atan2(d.y, d.x) - dir, 2.f * kPi)) <
                           m.dragonCone + std::asin(std::min(1.f, r / dl));
                };
                for (Enemy& e : w.enemies_)
                    if (e.hp > 0.f && inCone(e.pos, e.radius)) touch(w, e, dmg, elem, b.owner, true, p, ev);
                if (w.boss_.alive && inCone(w.boss_.pos, w.boss_.radius)) hitBoss(w, dmg);
                if (static_cast<int>(sw.breaths.size()) < S::maxBreaths)
                    sw.breaths.push_back({s.dragonPos, dir, m.dragonCone, S::dragonRange, S::breathLife, elem});
                s.dragonT = m.dragonInterval;
            }
        } else {
            s.dragonOut = false;
        }

        // Warden: spirits on a ring around the core.
        if (m.wardens > 0) {
            s.wardenAng += S::wardenSpin * dt;
            const float dmg = w.ballDamage(b, p) * m.wardenFrac * pw;
            for (int i = 0; i < m.wardens; ++i) {
                s.wardenRest[i] = std::max(0.f, s.wardenRest[i] - dt);
                if (s.wardenRest[i] > 0.f) continue;
                const sf::Vector2f at = wardenPos(w, b, i);
                for (Enemy& e : w.enemies_) {
                    if (e.hp <= 0.f || e.orbiter || length(e.pos - at) > e.radius + S::wardenRadius) continue;
                    touch(w, e, dmg, elem, b.owner, full, p, ev);
                    e.vel += normalized(e.pos - w.core_.pos, {1.f, 0.f}) * S::wardenKnock * e.knockTaken;
                    s.wardenRest[i] = S::wardenRest;
                    break;
                }
            }
        }
    }

    static sf::Vector2f wardenPos(const World& w, const Ball& b, int i) {
        return summonerWardenPos(w.core_.pos, b.cls.summoner.wardenAng, b.owner, i, b.mods.cls.summoner.wardens,
                                 cfg::summoner::wardenOrbit * w.arenaScale());
    }

    static void worldTick(World& w, float dt, const WorldParams& p, FrameEvents& ev) {
        namespace S = cfg::summoner;
        SummonerWorld& sw = w.classWorld_.summoner;

        // Wisps: every enemy that died this step near a Wisps ball (its own
        // kills, mostly) lets that ball's wisps loose. Checked before any
        // summon deals damage, so summon kills don't chain into more wisps.
        for (const Enemy& e : w.enemies_) {
            if (e.hp > 0.f) continue;
            const Ball* best = nullptr;
            float bestD = 0.f;
            for (const Ball& b : w.balls_) {
                if (b.mods.cls.summoner.wisps <= 0) continue;
                const float d = length(b.pos - e.pos) - b.radius - e.radius;
                if (d < 90.f && (!best || d < bestD)) { best = &b; bestD = d; }
            }
            if (!best) continue;
            const SummonerMods& m = best->mods.cls.summoner;
            for (int i = 0; i < m.wisps && static_cast<int>(sw.shots.size()) < S::maxShots; ++i) {
                SummonShot s;
                s.pos = e.pos;
                s.vel = w.rng_.direction() * S::wispSpeed * 0.6f;
                s.dmg = w.ballDamage(*best, p) * m.wispFrac * power(*best);
                s.life = S::wispLife * power(*best);
                s.elem = static_cast<int>(best->element);
                s.owner = best->owner;
                s.wisp = true;
                sw.shots.push_back(s);
            }
        }

        for (Ball& b : w.balls_) tickBall(w, b, dt, p, ev);

        // Turrets: aim and fire at the nearest enemy in range.
        for (SummonTurret& t : sw.turrets) {
            t.life -= dt;
            t.fireT -= dt;
            sf::Vector2f to;
            if (!target(w, t.pos, S::turretRange, to)) continue;
            t.aim = std::atan2(to.y - t.pos.y, to.x - t.pos.x);
            if (t.fireT > 0.f || static_cast<int>(sw.shots.size()) >= S::maxShots) continue;
            t.fireT = 1.f / std::max(0.1f, t.rate);
            SummonShot s;
            s.pos = t.pos;
            s.vel = sf::Vector2f{std::cos(t.aim), std::sin(t.aim)} * S::shotSpeed;
            s.dmg = t.dmg;
            s.life = S::shotLife;
            s.elem = t.elem;
            s.owner = t.owner;
            sw.shots.push_back(s);
        }
        sw.turrets.erase(std::remove_if(sw.turrets.begin(), sw.turrets.end(),
                                        [](const SummonTurret& t) { return t.life <= 0.f; }),
                         sw.turrets.end());

        // Shots and wisps: fly (wisps home in), hit the first enemy they touch.
        for (SummonShot& s : sw.shots) {
            s.life -= dt;
            if (s.wisp) {
                sf::Vector2f to;
                if (target(w, s.pos, 1e9f, to)) {
                    const float sp = S::wispSpeed;
                    const float cur = std::atan2(s.vel.y, s.vel.x);
                    const float want = std::atan2(to.y - s.pos.y, to.x - s.pos.x);
                    const float diff = clampf(std::remainder(want - cur, 2.f * kPi), -S::wispTurn * dt, S::wispTurn * dt);
                    const float cs = std::min(sp, length(s.vel) + sp * 2.f * dt);   // eases up to speed
                    s.vel = sf::Vector2f{std::cos(cur + diff), std::sin(cur + diff)} * cs;
                }
            }
            s.pos += s.vel * dt;
            const float r = s.wisp ? S::wispRadius : S::shotRadius;
            for (Enemy& e : w.enemies_) {
                if (e.hp <= 0.f || length(e.pos - s.pos) > e.radius + r) continue;
                bool full = false;
                if (s.owner >= 0 && s.owner < static_cast<int>(w.balls_.size()))
                    full = w.balls_[static_cast<std::size_t>(s.owner)].isAscended(BallRole::Summoner);
                touch(w, e, s.dmg, s.elem, s.owner, full, p, ev);
                s.life = 0.f;
                break;
            }
            if (s.life > 0.f && w.boss_.alive && length(w.boss_.pos - s.pos) < w.boss_.radius + r) {
                hitBoss(w, s.dmg);
                s.life = 0.f;
            }
        }
        sw.shots.erase(std::remove_if(sw.shots.begin(), sw.shots.end(),
                                      [&](const SummonShot& s) {
                                          return s.life <= 0.f || s.pos.x < -40.f || s.pos.y < -40.f ||
                                                 s.pos.x > w.size_.x + 40.f || s.pos.y > w.size_.y + 40.f;
                                      }),
                       sw.shots.end());

        // Totems: drag what walks inside and pulse a little damage.
        for (SummonTotem& t : sw.totems) {
            t.life -= dt;
            t.flash = std::max(0.f, t.flash - dt * 3.f);
            // Enemies ease toward their walking speed at rate 8 (updateEnemies);
            // a drag k holds them at 8 / (8 + k) of it.
            const float k = 8.f * t.slow / std::max(0.05f, 1.f - t.slow);
            const bool pulse = (t.pulseT -= dt) <= 0.f;
            if (pulse) { t.pulseT = S::totemPulse; t.flash = 1.f; }
            for (Enemy& e : w.enemies_) {
                if (e.hp <= 0.f || e.orbiter || length(e.pos - t.pos) > t.radius + e.radius) continue;
                e.vel *= std::exp(-k * dt);
                if (pulse) touch(w, e, t.dmg, t.elem, t.owner, false, p, ev);
            }
        }
        sw.totems.erase(std::remove_if(sw.totems.begin(), sw.totems.end(),
                                       [](const SummonTotem& t) { return t.life <= 0.f; }),
                        sw.totems.end());

        for (DragonBreath& br : sw.breaths) br.life -= dt;
        sw.breaths.erase(std::remove_if(sw.breaths.begin(), sw.breaths.end(),
                                        [](const DragonBreath& br) { return br.life <= 0.f; }),
                         sw.breaths.end());
    }

    // A new wave: the field's summons go (the ghosts do too); timers part-charged.
    static void waveStart(World& w, const WorldParams&) {
        w.classWorld_.summoner = SummonerWorld{};
        for (Ball& b : w.balls_) {
            SummonerState& s = b.cls.summoner;
            s.spriteT = cfg::summoner::spriteInterval * 0.5f;
            s.totemT = 0.f;
            s.dragonT = 0.f;
            s.turretCd = 0.f;
            for (float& r : s.wardenRest) r = 0.f;
        }
    }
};


// ==================================================================== Jester
// Plays on chance. Base: every hit rolls an outcome - normal, a double, a
// spark to the nearest other enemy, or a random element that reacts with
// anything (bands x the run's luck). Its items (coin, wild card, chaos bounce,
// jackpot) work on any ball, even without the class (ClassMods::loose).
// Grand Jester: every Jester roll is taken twice and the best kept; doubles
// triple.
template <> struct ClassHooks<BallRole::Jester> : NoClassHooks {
    enum Outcome { Normal, RandomElement, Spark, Double };   // worst -> best
    enum Trick { Crit, Echo, Zap, Bomb, Hole };              // "Wild card": borrowed procs

    // One Jester chance: through the run's luck; a Grand Jester rolls twice
    // and "Reroll" may give a miss one more try.
    static bool roll(World& w, const Ball& b, float base, const WorldParams& p) {
        if (w.chance(base, p)) return true;
        if (b.isAscended(BallRole::Jester) && w.chance(base, p)) return true;
        const float re = b.mods.cls.jester.reroll;
        return re > 0.f && w.rng_.range(0.f, 1.f) < re && w.chance(base, p);
    }

    static Outcome rollOutcome(World& w, const WorldParams& p) {
        namespace J = cfg::jester;
        float d = J::rollDouble * p.luck, s = J::rollSpark * p.luck, el = J::rollElement * p.luck;
        if (const float sum = d + s + el; sum > J::bandCap) {
            const float k = J::bandCap / sum;
            d *= k; s *= k; el *= k;
        }
        const float r = w.rng_.range(0.f, 1.f);
        return r < d ? Double : r < d + s ? Spark : r < d + s + el ? RandomElement : Normal;
    }

    static Outcome outcome(World& w, const Ball& b, const WorldParams& p) {
        Outcome o = rollOutcome(w, p);
        if (b.isAscended(BallRole::Jester)) o = std::max(o, rollOutcome(w, p));
        const float re = b.mods.cls.jester.reroll;
        if (o == Normal && re > 0.f && w.rng_.range(0.f, 1.f) < re) o = rollOutcome(w, p);
        return o;
    }

    // A small jolt to the nearest other live enemy not hit this instant, with a bolt.
    static void spark(World& w, const Enemy& from, float dmg, float range) {
        Enemy* t = nullptr;
        float best = range * range;
        for (Enemy& o : w.enemies_) {
            if (&o == &from || o.hp <= 0.f || o.hitFlash > 0.95f) continue;   // skip ones just hit
            const float d2 = dot(o.pos - from.pos, o.pos - from.pos);
            if (d2 < best) { best = d2; t = &o; }
        }
        if (!t) return;
        w.damageEnemy(*t, dmg);
        if (static_cast<int>(w.bolts_.size()) < cfg::element::maxBolts)
            w.bolts_.push_back(Bolt{from.pos, t->pos, cfg::element::boltLife, cfg::element::boltLife});
    }

    static void pop(World& w, sf::Vector2f at) {   // the outcome pip, spaced out
        JesterWorld& jw = w.classWorld_.jester;
        if (jw.popCd > 0.f || static_cast<int>(jw.pops.size()) >= cfg::jester::maxPops) return;
        jw.pops.push_back({at, cfg::jester::popLife});
        jw.popCd = cfg::jester::popSpacing;
    }

    // "Wild card": one proc borrowed at random from the items of any ball on
    // the field (its own numbers); with none around, a zap, a bomb or an echo.
    static void wildCard(World& w, Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev) {
        namespace S = cfg::synergy;
        namespace C = cfg::changer;
        struct Pick { Trick t; const BallMods* m; };
        Pick picks[24];
        int n = 0;
        auto add = [&](Trick t, const BallMods* m) { if (n < 24) picks[n++] = {t, m}; };
        for (const Ball& o : w.balls_) {
            const BallMods& m = o.mods;
            if (m.critChance > 0.f) add(Crit, &m);
            if (m.echoChance > 0.f) add(Echo, &m);
            if (m.teslaChance > 0.f) add(Zap, &m);
            if (m.bomberChance > 0.f) add(Bomb, &m);
            if (m.blackHoleChance > 0.f) add(Hole, &m);
        }
        if (n == 0) { add(Zap, nullptr); add(Bomb, nullptr); add(Echo, nullptr); }
        Pick pk = picks[w.rng_.irange(0, n - 1)];
        if (kill && (pk.t == Crit || pk.t == Echo)) pk = {Zap, nullptr};   // nothing left to hit twice
        switch (pk.t) {
            case Crit:
                w.damageEnemy(e, dmg * ((pk.m ? pk.m->critMult : cfg::combat::critMult) - 1.f));
                break;
            case Echo:
                w.damageEnemy(e, dmg);
                break;
            case Zap: {
                const int targets = pk.m ? pk.m->teslaTargets : S::teslaTargets;
                for (int i = 0; i < targets; ++i) spark(w, e, dmg * S::teslaFrac, S::teslaRange);
                break;
            }
            case Bomb: {
                const float r = pk.m ? pk.m->bombRadius : S::bombRadius;
                w.areaDamage(e.pos, r, dmg * S::bombFrac, &e);
                ev.bursts.push_back({e.pos, r, theme::elemFire, nullptr});
                break;
            }
            case Hole:
                if (static_cast<int>(w.blackHoles_.size()) < C::maxBlackHoles) {
                    BlackHole h;
                    h.pos = e.pos;
                    h.pull = pk.m->blackHolePull;
                    h.dmg = dmg * pk.m->blackHoleFrac;
                    h.elem = w.hitElement(b, p);
                    h.owner = b.owner;
                    w.blackHoles_.push_back(h);
                }
                break;
        }
    }

    static void onHit(World& w, Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev) {
        namespace J = cfg::jester;
        const JesterMods& j = b.mods.cls.jester;
        if (b.hasRole(BallRole::Jester)) {   // the role's own roll
            switch (outcome(w, b, p)) {
                case Double: {
                    const float extra = dmg * ((b.isAscended(BallRole::Jester) ? J::grandDoubleMul : 2.f) - 1.f);
                    if (kill || e.hp <= 0.f) spark(w, e, extra, J::sparkRange);   // it's down: the rest jumps on
                    else w.damageEnemy(e, extra);
                    pop(w, e.pos);
                    break;
                }
                case Spark: spark(w, e, dmg * J::sparkFrac, J::sparkRange); break;
                case RandomElement:   // no owner: it reacts with anything, the ball's own element too
                    if (!kill && e.hp > 0.f)
                        w.applyElement(e, static_cast<Element>(w.rng_.irange(1, kElementCount - 1)), -1, dmg, p, ev);
                    break;
                case Normal: break;
            }
        }
        if (j.wildChance > 0.f && roll(w, b, j.wildChance, p)) wildCard(w, b, e, dmg * j.wildPower, kill, p, ev);
        b.cls.jester.chaosArmed = false;   // "Chaos bounce": spent
        // "Coin flip": flip the next hit's coin now (damageMul reads it).
        b.cls.jester.coinMul = j.coinHeads <= 0.f ? 1.f
                             : roll(w, b, J::coinHeadsChance, p) ? j.coinHeads : J::coinTails;
    }

    static void onKill(World& w, Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev) {
        const JesterMods& j = b.mods.cls.jester;
        if (j.jackpotChance <= 0.f || !roll(w, b, j.jackpotChance, p)) return;
        w.areaDamage(e.pos, cfg::jester::jackpotRadius, dmg * j.jackpotBlast, &e);   // "Jackpot"
        ev.midasGold += j.jackpotGold;
        ev.bursts.push_back({e.pos, cfg::jester::jackpotRadius, theme::puGolden, "JACKPOT"});
    }

    // "Sleight": the slower it moves, the sooner it vanishes and reappears on
    // a random enemy, striking it.
    static void tick(World& w, Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
        const JesterMods& m = b.mods.cls.jester;
        if (m.sleight <= 0.f || b.ghost || b.held || !w.waveRunning_) return;
        JesterState& s = b.cls.jester;
        s.sleightT = std::min(m.sleightEvery, s.sleightT + dt * b.cls.mage.still);
        if (s.sleightT < m.sleightEvery) return;
        int alive = 0;
        for (const Enemy& e : w.enemies_) alive += e.hp > 0.f && !e.orbiter ? 1 : 0;
        if (alive == 0) return;
        int pick = w.rng_.irange(0, alive - 1);
        Enemy* t = nullptr;
        for (Enemy& e : w.enemies_)
            if (e.hp > 0.f && !e.orbiter && pick-- == 0) { t = &e; break; }
        if (!t) return;
        s.sleightT = 0.f;
        const sf::Vector2f from = b.pos;
        const sf::Vector2f dir = normalized(from - t->pos, {1.f, 0.f});
        b.pos = t->pos + dir * (t->radius + b.radius + 2.f);
        b.pos.x = clampf(b.pos.x, b.radius, w.size_.x - b.radius);
        b.pos.y = clampf(b.pos.y, b.radius, w.size_.y - b.radius);
        b.trail.clear();
        w.damageEnemy(*t, w.ballDamage(b, p) * m.sleight);
        pop(w, t->pos);
        ClassHooks<BallRole::Assassin>::addFx(w, from, b.pos, b.radius, false);
        ev.bursts.push_back({t->pos, t->radius * 2.f, theme::classJester, nullptr});
    }

    // "Chaos bounce": off the wall at a random angle, next hit armed.
    static void onWallBounce(World& w, Ball& b, sf::Vector2f normal, const WorldParams&, FrameEvents&) {
        if (b.mods.cls.jester.chaosHit <= 0.f) return;
        const float sp = length(b.vel);
        const float ang = std::atan2(normal.y, normal.x) +
                          w.rng_.range(-cfg::jester::chaosSpread, cfg::jester::chaosSpread);
        b.vel = sf::Vector2f{std::cos(ang), std::sin(ang)} * sp;
        b.cls.jester.chaosArmed = true;
    }

    static float damageMul(const World&, const Ball& b, const WorldParams&) {
        float m = b.cls.jester.coinMul;   // 1 without a coin
        if (b.cls.jester.chaosArmed && b.mods.cls.jester.chaosHit > 0.f) m *= b.mods.cls.jester.chaosHit;
        return m;
    }

    static void worldTick(World& w, float dt, const WorldParams&, FrameEvents&) {
        JesterWorld& jw = w.classWorld_.jester;
        jw.popCd = std::max(0.f, jw.popCd - dt);
        for (JesterWorld::Pop& q : jw.pops) q.t -= dt;
        jw.pops.erase(std::remove_if(jw.pops.begin(), jw.pops.end(), [](const JesterWorld::Pop& q) { return q.t <= 0.f; }),
                      jw.pops.end());
    }
};

// ==================================================================== Slinger
// The class of your hands. Base: catching it pays more (World::grabAt) and the
// first hit after your throw lands harder. Its items (Coil in regulateSpeed /
// releaseHeld, the rest here) work on any ball, even without the class
// (ClassMods::loose). Master Slinger: every catch recharges its abilities and
// the thrown hit is bigger.
template <> struct ClassHooks<BallRole::Slinger> : NoClassHooks {
    static void tick(World& w, Ball& b, float dt, const WorldParams& p, FrameEvents&) {
        namespace S = cfg::slinger;
        SlingerState& st = b.cls.slinger;
        const SlingerMods& m = b.mods.cls.slinger;
        st.sinceThrow += dt;
        if (st.armed && (st.armedT -= dt) <= 0.f) st.armed = false;   // the thrown hit went unused
        // "Afterburner": fire along the throw while it's still flying fast.
        if (st.burnT > 0.f) {
            st.burnT -= dt;
            st.flameT -= dt;
            if (st.flameT <= 0.f && m.burnFrac > 0.f && length(b.vel) > w.ballCruise(b, p)) {
                st.flameT = S::flameEvery;
                auto& fl = w.classWorld_.slinger.flames;
                if (static_cast<int>(fl.size()) >= S::maxFlames) fl.erase(fl.begin());
                fl.push_back({b.pos, S::flameLife, S::flameLife, w.ballDamage(b, p) * m.burnFrac, b.owner});
            }
        }
        // "Grip": close to your pointer, it bends toward it.
        if (m.gripTurn > 0.f && w.hasPointer_ && !b.held) {
            const sf::Vector2f to = w.pointer_ - b.pos;
            const float sp = length(b.vel);
            if (sp > 1e-3f && length(to) < m.gripRange * w.arenaScale()) {
                const float cur = std::atan2(b.vel.y, b.vel.x);
                const float want = std::atan2(to.y, to.x);
                const float d = clampf(std::remainder(want - cur, 2.f * kPi), -m.gripTurn * dt, m.gripTurn * dt);
                b.vel = sf::Vector2f{std::cos(cur + d), std::sin(cur + d)} * sp;
            }
        }
    }

    static void onGrab(World&, Ball& b) {
        namespace S = cfg::slinger;
        SlingerState& st = b.cls.slinger;
        const SlingerMods& m = b.mods.cls.slinger;
        const bool quick = st.sinceThrow < S::releaseWindow;   // caught back soon after your throw
        if (m.releaseMax > 0) st.stacks = quick ? std::min(st.stacks + 1, m.releaseMax) : 0;   // "Catch & release"
        if (m.doubleDown > 0.f && quick) st.doubleDown = true;                                // "Double down"
        if (b.isAscended(BallRole::Slinger))   // Master Slinger: a catch recharges its abilities
            for (int i = 0; i < kMaxAbilitySlots; ++i)
                if (b.abilities[i].id != Ability::None)
                    b.abilityCd[i] = std::max(0.f, b.abilityCd[i] - S::masterRecharge *
                                                   abilityCooldown(b.abilities[i].id, b.abilities[i].level));
    }

    static void onThrow(World&, Ball& b) {
        SlingerState& st = b.cls.slinger;
        st.sinceThrow = 0.f;
        st.armed = true;
        st.armedT = cfg::slinger::armedTime;
        st.burnT = b.mods.cls.slinger.burnTime;   // "Afterburner" (0 without it)
        st.flameT = 0.f;
    }

    static float damageMul(const World& w, const Ball& b, const WorldParams& p) {
        const SlingerMods& m = b.mods.cls.slinger;
        float k = 1.f + m.releasePer * static_cast<float>(b.cls.slinger.stacks);   // "Catch & release"
        if (m.momentum > 0.f) {   // "Momentum": every cruise over its own adds up, no cap
            const float over = length(b.vel) / std::max(1.f, w.ballCruise(b, p)) - 1.f;
            if (over > 0.f) k *= 1.f + m.momentum * over;
        }
        return k;
    }

    static float preHit(World& w, Ball& b, const Enemy& e, const WorldParams& p) {
        namespace S = cfg::slinger;
        SlingerState& st = b.cls.slinger;
        const SlingerMods& m = b.mods.cls.slinger;
        float k = 1.f;
        if (st.doubleDown) {   // "Double down": double or nothing, luck on the win
            st.doubleDown = false;
            k *= w.chance(S::doubleChance, p) ? m.doubleDown : 0.f;
        }
        if (!st.armed) return k;
        if (b.hasRole(BallRole::Slinger))   // the class: your throw's first hit lands harder
            k *= b.isAscended(BallRole::Slinger) ? S::masterThrownHit : S::thrownHit;
        if (m.execution > 0.f && e.hp >= e.maxHp) k *= m.execution;   // "Execution throw"
        return k;
    }

    static void onHit(World& w, Ball& b, Enemy& e, float dmg, bool, const WorldParams& p, FrameEvents& ev) {
        SlingerState& st = b.cls.slinger;
        if (!st.armed) return;
        st.armed = false;   // the thrown first hit is spent
        const float amb = b.mods.cls.slinger.ambush;
        if (amb <= 0.f || b.ghost) return;
        // "Ambush": blink on to the nearest other enemy and strike it.
        Enemy* t = nullptr;
        float best = cfg::slinger::ambushRange * w.arenaScale();
        best *= best;
        for (Enemy& o : w.enemies_) {
            if (&o == &e || o.hp <= 0.f) continue;
            const float d2 = dot(o.pos - b.pos, o.pos - b.pos);
            if (d2 < best) { best = d2; t = &o; }
        }
        if (!t) return;
        const sf::Vector2f from = b.pos;
        const sf::Vector2f dir = normalized(from - t->pos, {1.f, 0.f});
        b.pos = t->pos + dir * (t->radius + b.radius + 2.f);
        b.pos.x = clampf(b.pos.x, b.radius, w.size_.x - b.radius);
        b.pos.y = clampf(b.pos.y, b.radius, w.size_.y - b.radius);
        const float sp = std::max(length(b.vel), w.ballCruise(b, p));
        b.vel = dir * sp;   // it bounces off the one it hit
        b.trail.clear();
        w.damageEnemy(*t, dmg * amb);
        ClassHooks<BallRole::Assassin>::addFx(w, from, b.pos, b.radius, true);
        ev.bursts.push_back({t->pos, t->radius * 2.2f, theme::classSlinger, nullptr});
    }

    // "Afterburner": the flames burn what stands in them - the Fire element
    // for real: its reactions, Ember's burn, the element nodes' potency.
    static void worldTick(World& w, float dt, const WorldParams& p, FrameEvents& ev) {
        namespace S = cfg::slinger;
        auto& fl = w.classWorld_.slinger.flames;
        if (fl.empty()) return;
        const float r = S::flameRadius * w.arenaScale();
        const float pot = p.elemMult[static_cast<int>(Element::Fire)];
        for (Enemy& e : w.enemies_) {
            if (e.hp <= 0.f) continue;
            const SlingerWorld::Flame* in = nullptr;
            for (const SlingerWorld::Flame& f : fl)
                if (dot(e.pos - f.pos, e.pos - f.pos) < (r + e.radius) * (r + e.radius) && (!in || f.dps > in->dps)) in = &f;
            if (!in) continue;
            w.damageEnemy(e, in->dps * dt);
            {   // the same burn a fire ball leaves (it spreads on death, "Ember" heats it)
                const float k = pot * (1.f + cfg::element::emberPerLevel * static_cast<float>(p.emberLevel));
                e.burn = std::max(e.burn, cfg::element::burnDuration);
                e.burnDps = std::max(e.burnDps, cfg::element::burnPerHit * 2.f * k);
            }
            if (e.elem != Element::Fire) w.applyElement(e, Element::Fire, in->owner, in->dps, p, ev);   // reactions
        }
        for (SlingerWorld::Flame& f : fl) f.life -= dt;
        fl.erase(std::remove_if(fl.begin(), fl.end(), [](const SlingerWorld::Flame& f) { return f.life <= 0.f; }), fl.end());
    }

    static void waveStart(World& w, const WorldParams&) { w.classWorld_.slinger.flames.clear(); }
};

// ---------------------------------------------------------------- dispatch
// (no class logic below this line)

namespace {

// Call fn(ClassHooks<R>{}) for every class in the mask, in BallRole order.
template <typename Fn>
void eachClass(RoleMask m, Fn&& fn) {
    if (m & roleBit(BallRole::Striker))  fn(ClassHooks<BallRole::Striker>{});
    if (m & roleBit(BallRole::Guardian)) fn(ClassHooks<BallRole::Guardian>{});
    if (m & roleBit(BallRole::Support))  fn(ClassHooks<BallRole::Support>{});
    if (m & roleBit(BallRole::Mage))     fn(ClassHooks<BallRole::Mage>{});
    if (m & roleBit(BallRole::Shooter))  fn(ClassHooks<BallRole::Shooter>{});
    if (m & roleBit(BallRole::Assassin)) fn(ClassHooks<BallRole::Assassin>{});
    if (m & roleBit(BallRole::Summoner)) fn(ClassHooks<BallRole::Summoner>{});
    if (m & roleBit(BallRole::Jester))   fn(ClassHooks<BallRole::Jester>{});
    if (m & roleBit(BallRole::Slinger))  fn(ClassHooks<BallRole::Slinger>{});
}

constexpr RoleMask kAllClasses = ~0u;

// The ball's classes, plus any class whose single items work on their own
// (ClassMods::loose).
RoleMask hookMask(const Ball& b) { return b.roles | b.mods.cls.loose; }

}  // namespace

void World::classTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    eachClass(hookMask(b), [&](auto h) { decltype(h)::tick(*this, b, dt, p, ev); });
}

void World::classOnHit(Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev) {
    eachClass(hookMask(b), [&](auto h) { decltype(h)::onHit(*this, b, e, dmg, kill, p, ev); });
}

void World::classOnKill(Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev) {
    eachClass(hookMask(b), [&](auto h) { decltype(h)::onKill(*this, b, e, dmg, p, ev); });
}

void World::classOnWallBounce(Ball& b, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev) {
    eachClass(hookMask(b), [&](auto h) { decltype(h)::onWallBounce(*this, b, normal, p, ev); });
}

void World::classOnCoreBounce(Ball& b, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev) {
    eachClass(hookMask(b), [&](auto h) { decltype(h)::onCoreBounce(*this, b, normal, p, ev); });
}

float World::classDamageMul(const Ball& b, const WorldParams& p) const {
    float m = 1.f;
    eachClass(hookMask(b), [&](auto h) { m *= decltype(h)::damageMul(*this, b, p); });
    return m;
}

void World::classWorldTick(float dt, const WorldParams& p, FrameEvents& ev) {
    eachClass(kAllClasses, [&](auto h) { decltype(h)::worldTick(*this, dt, p, ev); });
}

void World::classWaveStart(const WorldParams& p) {
    eachClass(kAllClasses, [&](auto h) { decltype(h)::waveStart(*this, p); });
}

void World::classOnGrab(Ball& b) {
    eachClass(hookMask(b), [&](auto h) { decltype(h)::onGrab(*this, b); });
}

void World::classOnThrow(Ball& b) {
    eachClass(hookMask(b), [&](auto h) { decltype(h)::onThrow(*this, b); });
}

float World::classPreHit(Ball& b, const Enemy& e, const WorldParams& p) {
    float m = 1.f;
    eachClass(hookMask(b), [&](auto h) { m *= decltype(h)::preHit(*this, b, e, p); });
    return m;
}

}  // namespace sb
