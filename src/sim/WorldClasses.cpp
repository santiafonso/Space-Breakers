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
    static void onHit(World& w, Ball& b, Enemy& e, float dmg, bool, const WorldParams& p, FrameEvents&) {
        namespace S = cfg::synergy;
        const float over = length(b.vel) / (w.ballCruise(b, p) * S::strikerShockSpeed);
        if (!b.isAscended(BallRole::Striker) || over <= 1.f) return;
        const float r = S::strikerShockRadius * std::min(over, S::strikerShockMaxScale);   // faster = wider (the hit already scales)
        w.areaDamage(e.pos, r, dmg * S::strikerShockFrac, &e);
        w.itemFx(CommonWorld::Fx::Sonic, e.pos, r, theme::ballFast, 0.35f, normalized(b.vel, {1.f, 0.f}));
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

    static void onHit(World& w, Ball& b, Enemy&, float, bool, const WorldParams&, FrameEvents&) {
        if (b.ghost) return;   // copies never cast
        w.refundAbilities(b, cfg::mage::hitRefund);
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
            if (const Enemy* f = w.focusEnemy()) m.target = f->id;   // the player's target
            Enemy* t = findEnemy(w, m.target);
            if (!t) {   // lost its target: the nearest one to the missile
                const Enemy* n = w.nearestEnemy(m.pos, A::missileRetarget * as);
                m.target = n ? n->id : -1;
                t = findEnemy(w, m.target);
            }
            sf::Vector2f aim = m.pos + m.vel;
            if (t) aim = t->pos;
            else if (w.boss_.alive && w.boss_.intro <= 0.f) aim = w.boss_.pos;
            if (w.focusIsBoss()) aim = w.boss_.pos;
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
        if (any) w.itemFx(CommonWorld::Fx::Rune, b.pos, R, theme::classMage, 0.45f);
    }
};

// Recharge speed: "Focus", Channel, Archive, "Meditate" when still, "Archmage"
// - their bonuses add up with diminishing returns, to x(1 + rateCap) at most.
float World::mageCastRate(const Ball& b) const {
    const float bonus = b.mods.cls.mage.focus + b.mods.cls.mage.meditate * b.cls.mage.still +
                        std::max(0.f, b.mods.cls.common.archmage - 1.f) +
                        (b.hasRole(BallRole::Odin) ? cfg::odin::rate : 0.f) + b.mods.cls.odin.valknut;   // Odin's, in the same sum
    const float cap = cfg::ability::rateCap;
    return 1.f + cap * bonus / (cap + bonus);   // diminishing: x(1 + 2B / (2 + B)), x3 at most
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
    const float dmg = ballDamage(b, p) * frac * b.mods.cls.mage.power * b.mods.cls.mage.missileMul *
                      (1.f + b.mods.cls.common.archStack * static_cast<float>(b.cls.common.archStacks));   // "Archmage"
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
    if (g.manaSpring > 0.f) {   // "Mana spring": the other abilities get closer
        itemFx(CommonWorld::Fx::Mana, b.pos, b.radius * 3.f, theme::classMage, 0.5f);
        for (int j = 0; j < kMaxAbilitySlots; ++j)
            if (j != slot) refundAbility(b, j, 0.f, g.manaSpring);
    }
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
        if (const auto f = w.focusPos(); f && dot(*f - from, *f - from) <= range * range &&
                                         !(w.focusEnemy() && w.focusEnemy()->id == skipId)) {
            at = *f;   // the player's target, when it's in range
            return true;
        }
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

    static void fire(World& w, Ball& b, sf::Vector2f dir, float dmg, const WorldParams& p) {
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
        u.banks = b.mods.cls.common.bankShots;   // "Bank shot"
        u.hollow = b.mods.cls.common.hollow;     // "Hollow point"
        u.suppress = b.mods.cls.common.suppress; // "Suppressing fire"
        u.frag = b.mods.cls.common.frag;         // "Frag rounds"
        u.smart = b.mods.cls.common.smart;       // "Smart rounds"
        if (const CommonMods& cm = b.mods.cls.common; cm.ion > 0.f) {   // "Ion cannon": the faster it fires, the further it jumps
            const float over = std::max(0.f, b.cls.common.fireRateNow - 1.f);
            u.chain = std::min(cfg::common::ionJumpsMax, cm.ionJumps + static_cast<int>(over * cfg::common::ionRateJumps));
            u.chainKeep = cm.ion;
        }
        if (const int mk = b.mods.cls.common.marksman; mk > 0 && ++b.cls.common.shots % mk == 0) {   // "Marksman"
            u.dmg *= cfg::common::marksmanMul;
            u.mark = true;
        }
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
        CommonState& cs = b.cls.common;   // "Minigun": spun up, it fires faster
        const CommonMods& cm = b.mods.cls.common;
        if (cm.leadStorm > 0.f) {   // "Lead storm": now and then, a spiral of bullets all around
            if (cs.stormOn > 0.f) {
                cs.stormOn -= dt;
                cs.stormFireT -= dt * m.rate * cfg::common::leadStormRate;
                const float d = w.ballDamage(b, p) * S::bulletFrac;
                for (int guard = 0; cs.stormFireT <= 0.f && guard < 8; ++guard) {
                    cs.stormFireT += S::fireInterval;
                    cs.stormAng += cfg::common::leadStormTurn;
                    fire(w, b, {std::cos(cs.stormAng), std::sin(cs.stormAng)}, d, p);
                }
            } else if ((cs.stormT -= dt) <= 0.f) {
                cs.stormT = cm.leadStorm;
                cs.stormOn = cfg::common::leadStormTime;
                cs.stormFireT = 0.f;
                w.itemFx(CommonWorld::Fx::Spokes, b.pos, b.radius * 3.f, theme::classShooter, 0.4f);
            }
        }
        cs.fireRateNow = speedRate * m.rate * (1.f + cfg::common::minigunMax * cs.spin);   // ("Ion cannon" reads it)
        st.fireT -= dt * cs.fireRateNow;
        if (st.fireT > 0.f) return;
        sf::Vector2f at;
        const float range = S::range * w.arenaScale() * (1.f + 0.5f * m.slug * still);   // "Slug" reaches further
        if (!aimAt(w, b.pos, range, -1, at)) { st.fireT = 0.f; return; }   // wait, loaded
        st.fireT += S::fireInterval;
        if (st.fireT < 0.f) st.fireT = 0.f;
        if (b.mods.cls.common.minigun > 0.f) {   // "Minigun": one more volley spins it up
            cs.spin = std::min(1.f, cs.spin + b.mods.cls.common.minigun);
            cs.spinIdle = 0.f;
        }
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
            if (u.smart > 0.f) {   // "Smart rounds": curve after the nearest enemy
                sf::Vector2f at;
                if (aimAt(w, u.pos, S::range * w.arenaScale(), u.lastHit, at)) {
                    const float cur = std::atan2(u.vel.y, u.vel.x);
                    const float want = std::atan2(at.y - u.pos.y, at.x - u.pos.x);
                    const float turn = clampf(std::remainder(want - cur, 2.f * kPi), -u.smart * dt, u.smart * dt);
                    u.vel = sf::Vector2f{std::cos(cur + turn), std::sin(cur + turn)} * length(u.vel);
                }
            }
            u.pos += u.vel * dt;
            if (u.pos.x < 0.f || u.pos.y < 0.f || u.pos.x > sz.x || u.pos.y > sz.y) {
                if (u.banks > 0) {   // "Bank shot": off the wall instead
                    --u.banks;
                    if (u.pos.x < 0.f || u.pos.x > sz.x) u.vel.x = -u.vel.x;
                    if (u.pos.y < 0.f || u.pos.y > sz.y) u.vel.y = -u.vel.y;
                    u.pos.x = clampf(u.pos.x, 0.f, sz.x);
                    u.pos.y = clampf(u.pos.y, 0.f, sz.y);
                    u.lastHit = -1;
                    u.life = std::max(u.life, S::bulletLife * 0.6f);
                } else {
                    u.life = 0.f;
                }
            }
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
            const bool hollow = u.hollow > 0.f && (e.burn > 0.f || e.poison > 0.f || e.frozen > 0.f || e.soak > 0.f ||
                                                   e.cracks > 0 || (e.elemT > 0.f && e.elem != Element::Plain));
            e.lastBy = u.owner;   // kill credit
            w.damageEnemy(e, u.dmg * (e.mark > 0.f ? p.markMul : 1.f) * (hollow ? 1.f + u.hollow : 1.f));   // "Hollow point"
            e.hitFlash = std::max(flash, 0.55f);   // a bullet is a tick, not a big hit
            if (u.frag > 0.f) {   // "Frag rounds": a little burst
                const float R = cfg::common::fragRadius * w.arenaScale();
                w.areaDamage(e.pos, R, u.dmg * u.frag, &e);
                w.itemFx(CommonWorld::Fx::Pellets, e.pos, R, theme::classShooter, 0.25f);
            }
            if (u.mark) e.mark = std::max(e.mark, cfg::role::markDuration);   // "Marksman"
            if (u.suppress > 0.f) {   // "Suppressing fire": each one slows it a little more
                e.braceT = std::max(e.braceT, cfg::common::braceTime);
                e.braceSlow = std::min(cfg::common::suppressMax, e.braceSlow + u.suppress);
            }
            if (u.elem != 0) w.applyElement(e, static_cast<Element>(u.elem), u.owner, u.dmg, p, ev);
            u.lastHit = e.id;
            if (u.chain > 0) {   // "Ion cannon": the round turns into a bolt, jumping enemy to enemy
                std::vector<const Enemy*> struck{&e};
                const Enemy* from = &e;
                float d = u.dmg;
                for (int n = 0; n < u.chain; ++n) {
                    Enemy* nx = nullptr;
                    float bd = cfg::common::ionRange * w.arenaScale();
                    bd *= bd;
                    for (Enemy& o : w.enemies_) {
                        if (o.hp <= 0.f || o.orbiter || std::find(struck.begin(), struck.end(), &o) != struck.end()) continue;
                        const float d2 = dot(o.pos - from->pos, o.pos - from->pos);
                        if (d2 < bd) { bd = d2; nx = &o; }
                    }
                    if (!nx) break;
                    d *= u.chainKeep;
                    nx->lastBy = u.owner;
                    w.damageEnemy(*nx, d * (nx->mark > 0.f ? p.markMul : 1.f));
                    nx->hitFlash = std::max(nx->hitFlash, 0.55f);
                    if (u.elem != 0) w.applyElement(*nx, static_cast<Element>(u.elem), u.owner, d, p, ev);
                    w.itemFx(CommonWorld::Fx::Ion, from->pos, 0.f, theme::classShooter, 0.22f, nx->pos);
                    struck.push_back(nx);
                    from = nx;
                }
                u.life = 0.f;
                continue;
            }
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
    // "Predator": the live enemy in reach with the least health left (by share).
    static Enemy* weakest(World& w, sf::Vector2f from, float range, const std::vector<Enemy*>& skip) {
        Enemy* best = nullptr;
        float bestF = 2.f;
        for (Enemy& e : w.enemies_) {
            if (e.hp <= 0.f || e.orbiter || dot(e.pos - from, e.pos - from) > range * range) continue;
            if (e.pos.x < 0.f || e.pos.y < 0.f || e.pos.x > w.size_.x || e.pos.y > w.size_.y) continue;
            if (std::find(skip.begin(), skip.end(), &e) != skip.end()) continue;
            const float f = e.hp / std::max(1e-3f, e.maxHp);
            if (f < bestF) { bestF = f; best = &e; }
        }
        return best;
    }

    static Enemy* pick(World& w, sf::Vector2f from, float range, const std::vector<Enemy*>& skip) {
        if (const Enemy* f = w.focusEnemy();   // the player's target, when it's in range
            f && dot(f->pos - from, f->pos - from) <= range * range &&
            std::find(skip.begin(), skip.end(), f) == skip.end())
            return const_cast<Enemy*>(f);
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
        const bool predator = b.mods.cls.common.predator > 0.f;   // "Predator": further, weakest first
        const float range = A::blinkRange * w.arenaScale() * (predator ? cfg::common::predatorRange : 1.f);
        const int reach = 1 + (b.isAscended(BallRole::Assassin) ? A::ascChain : 0);
        std::vector<Enemy*> chain;
        for (sf::Vector2f at = b.pos; static_cast<int>(chain.size()) < reach;) {
            Enemy* e = predator ? weakest(w, at, range, chain) : pick(w, at, range, chain);
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
        const sf::Vector2f start = b.pos;   // ("Razor wire")
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
            w.itemFx(CommonWorld::Fx::Smoke, dest, R, el == Element::Plain ? theme::classAssassin : elementColor(el), 0.8f);
        }
        if (m.backstab > 0.f) s.armedT = A::backstabWindow;
        s.blinked = true;   // "Lurk": the next hit spends the charge
        if (m.spreePer > 0.f) {
            s.spree = std::min(s.spree + 1, m.spreeMax);
            s.spreeT = A::spreeWindow;
        }
        s.cd = b.mods.cls.common.reaper > 0.f ? std::min(cooldown(b), b.mods.cls.common.reaper) : cooldown(b);   // "Reaper"
        w.commonOnBlink(b, t, p, start);   // Mark for death / Quickstep / Razor wire...
    }

    static void tick(World& w, Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
        AssassinState& s = b.cls.assassin;
        s.cd = std::max(0.f, s.cd - dt);
        s.armedT = std::max(0.f, s.armedT - dt);
        if (s.spreeT > 0.f && (s.spreeT -= dt) <= 0.f) s.spree = 0;
        if (!s.pending) return;
        if (w.classWorld_.assassin.clock - s.pendingAt > cfg::assassin::blinkStale || b.held) {
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
        if (b.mods.cls.common.reaper > 0.f && !b.held) {   // "Reaper": every hit queues a blink, not just kills
            w.itemFx(CommonWorld::Fx::Scythe, b.pos, b.radius * 2.2f, theme::classAssassin, 0.3f, normalized(b.vel, {1.f, 0.f}));
            b.cls.assassin.pending = true;
            b.cls.assassin.pendingAt = w.classWorld_.assassin.clock;
        }
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
        const Ball* o = ownerBall(w, owner);
        if (o) dmg *= 1.f + o->mods.cls.common.guardDog;   // "Guard dog"
        if (o && o->mods.cls.common.soulbind) full = true; // "Soulbind": the element's full effect
        const bool wasAlive = e.hp > 0.f;
        if (owner >= 0) e.lastBy = owner;   // kill credit: the summoner
        w.damageEnemy(e, dmg);
        if (o && wasAlive && e.hp <= 0.f && o->mods.cls.common.hatchery > 0)   // "Hatchery": a kill hatches wisps
            hatch(w, *o, e.pos, o->mods.cls.common.hatchery, p);
        const Element el = static_cast<Element>(elem);
        if (el == Element::Plain) return;
        if (full) {   // the element's own effect, like a ball hit
            const float pot = p.elemMult[elem];
            if (el == Element::Poison) {
                e.poison = E::poisonDuration;
                e.poisonDps = std::min(e.poisonDps + E::poisonDpsPerHit * pot, E::poisonDpsMax * pot);
            } else if (el == Element::Ice) {
                e.frozen = std::max(e.frozen, cfg::summoner::summonFreeze * pot);
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
    static const Ball* ownerBall(const World& w, int owner) {
        for (const Ball& b : w.balls_)
            if (b.owner == owner) return &b;
        return nullptr;
    }

    // Where a summon of `owner` aims: "Guard dog" = the enemy nearest the core
    // (in range of it), else the usual target().
    static bool aim(const World& w, int owner, sf::Vector2f from, float range, sf::Vector2f& out) {
        const Ball* o = ownerBall(w, owner);
        if (!o || o->mods.cls.common.guardDog <= 0.f) return target(w, from, range, out);
        if (const auto f = w.focusPos(); f && dot(*f - from, *f - from) <= range * range) {
            out = *f;   // the player's target still comes first
            return true;
        }
        const Enemy* best = nullptr;
        float bd = 1e18f;
        for (const Enemy& e : w.enemies_) {
            if (e.hp <= 0.f || e.orbiter || dot(e.pos - from, e.pos - from) > range * range) continue;
            const float d2 = dot(e.pos - w.core_.pos, e.pos - w.core_.pos);
            if (d2 < bd) { bd = d2; best = &e; }
        }
        if (!best) return target(w, from, range, out);
        out = best->pos;
        return true;
    }

    static bool target(const World& w, sf::Vector2f from, float range, sf::Vector2f& out) {
        if (const auto f = w.focusPos(); f && dot(*f - from, *f - from) <= range * range) {
            out = *f;   // the player's target, when it's in range
            return true;
        }
        const Enemy* e = w.nearestEnemy(from, range);
        float best = e ? length(e->pos - from) : range;
        if (e) out = e->pos;
        if (w.boss_.targetable() && length(w.boss_.pos - from) < best) {
            out = w.boss_.pos;
            return true;
        }
        return e != nullptr;
    }

    // "Twin call": the summon just pushed gets a twin on the far side of the core.
    static sf::Vector2f mirror(const World& w, sf::Vector2f at) {
        const sf::Vector2f m = w.core_.pos * 2.f - at;
        return {clampf(m.x, 10.f, w.size_.x - 10.f), clampf(m.y, 10.f, w.size_.y - 10.f)};
    }
    static void twin(World& w, const Ball& o, std::vector<SummonShot>& v) {
        if (!o.mods.cls.common.twinCall || v.empty() || static_cast<int>(v.size()) >= cfg::summoner::maxShots) return;
        SummonShot c = v.back();
        c.pos = mirror(w, c.pos);
        c.vel = -c.vel;
        v.push_back(c);
    }
    template <typename T>
    static void twin(World& w, const Ball& o, std::vector<T>& v) {
        if (!o.mods.cls.common.twinCall || v.empty()) return;
        T c = v.back();
        c.pos = mirror(w, c.pos);
        v.push_back(c);
    }

    // "Hatchery": n wisps loose from where a summon made a kill.
    static void hatch(World& w, const Ball& o, sf::Vector2f at, int n, const WorldParams& p) {
        namespace S = cfg::summoner;
        SummonerWorld& sw = w.classWorld_.summoner;
        for (int i = 0; i < n && static_cast<int>(sw.shots.size()) < S::maxShots; ++i) {
            SummonShot s;
            s.pos = at;
            s.vel = w.rng_.direction() * S::wispSpeed * 0.6f;
            s.dmg = w.ballDamage(o, p) * cfg::common::hatcheryFrac * power(o);
            s.life = S::wispLife * power(o) * (1.f + o.mods.cls.common.linger);
            s.elem = static_cast<int>(o.element);
            s.owner = o.owner;
            s.wisp = true;
            sw.shots.push_back(s);
            twin(w, o, sw.shots);   // "Twin call"
        }
    }

    // "Martyr": a turret / totem that runs out bursts with its element.
    static void martyr(World& w, sf::Vector2f at, int elem, int owner, const WorldParams& p, FrameEvents& ev) {
        const Ball* o = ownerBall(w, owner);
        if (!o || o->mods.cls.common.martyr <= 0.f) return;
        const float R = cfg::common::martyrRadius * w.arenaScale();
        const float dmg = w.ballDamage(*o, p) * o->mods.cls.common.martyr;
        for (Enemy& e : w.enemies_)
            if (e.hp > 0.f && length(e.pos - at) <= R + e.radius) touch(w, e, dmg, elem, owner, false, p, ev);
        w.itemFx(CommonWorld::Fx::Pillar, at, R, elem != 0 ? elementColor(static_cast<Element>(elem)) : theme::classSummoner, 0.6f);
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
        g.ghostLife = S::spriteLife * power(b) * (1.f + b.mods.cls.common.linger);   // "Lingering"
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
            q.life = S::wispLife * pw * (1.f + b.mods.cls.common.linger);
            q.elem = elem;
            q.owner = b.owner;
            q.wisp = true;
            sw.shots.push_back(q);
            twin(w, b, sw.shots);   // "Twin call"
        }
        // "Drop turret": you let go of it here.
        if (s.dropPending) {
            s.dropPending = false;
            if (static_cast<int>(sw.turrets.size()) < S::maxTurrets) {
                SummonTurret t;
                t.pos = {std::clamp(s.dropAt.x, S::turretInset, w.size_.x - S::turretInset),
                         std::clamp(s.dropAt.y, S::turretInset, w.size_.y - S::turretInset)};
                t.life = t.maxLife = cfg::style::dropLife * pw * (1.f + b.mods.cls.common.linger);
                t.rate = cfg::style::dropRate;
                t.dmg = w.ballDamage(b, p) * m.drop * pw;
                t.aim = std::atan2(w.core_.pos.y - t.pos.y, w.core_.pos.x - t.pos.x);
                t.elem = elem;
                t.owner = b.owner;
                sw.turrets.push_back(t);
                twin(w, b, sw.turrets);   // "Twin call"
            }
        }

        // The class: spritelings.
        if (role && fighting && (s.spriteT -= dt) <= 0.f) {
            spawnSprite(w, b, p);
            s.spriteT = full ? S::spriteIntervalAscended : S::spriteInterval;
        }

        // Turret: a wall bounce (the heading flipped right next to a wall).
        s.turretCd = std::max(0.f, s.turretCd - dt);
        if (m.turretFrac > 0.f && !b.held) {
            const float slop = b.radius + length(b.vel) * dt + 2.f;
            const sf::Vector2f sz = w.size_;
            const bool flipX = b.vel.x * s.prevVel.x < 0.f && (b.pos.x < slop || b.pos.x > sz.x - slop);
            const bool flipY = b.vel.y * s.prevVel.y < 0.f && (b.pos.y < slop || b.pos.y > sz.y - slop);
            if ((flipX || flipY) && s.turretCd <= 0.f && ownedBy(sw.turrets, b.owner) < m.turretMax &&
                static_cast<int>(sw.turrets.size()) < S::maxTurrets) {
                SummonTurret t;
                t.pos = {std::clamp(b.pos.x, S::turretInset, sz.x - S::turretInset),
                         std::clamp(b.pos.y, S::turretInset, sz.y - S::turretInset)};
                t.life = t.maxLife = m.turretLife * pw * (1.f + b.mods.cls.common.linger);
                t.rate = m.turretRate;
                t.dmg = w.ballDamage(b, p) * m.turretFrac * pw;
                t.aim = std::atan2(w.core_.pos.y - t.pos.y, w.core_.pos.x - t.pos.x);
                t.elem = elem;
                t.owner = b.owner;
                sw.turrets.push_back(t);
                twin(w, b, sw.turrets);   // "Twin call"
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
                t.life = t.maxLife = m.totemLife * pw * (1.f + b.mods.cls.common.linger);
                t.radius = m.totemRadius;
                t.slow = std::min(0.8f, m.totemSlow * (full ? 1.25f : 1.f));
                t.pulseT = S::totemPulse * 0.5f;
                t.dmg = w.ballDamage(b, p) * S::totemFrac * pw;
                t.elem = elem;
                t.owner = b.owner;
                sw.totems.push_back(t);
                twin(w, b, sw.totems);   // "Twin call"
                s.totemT = m.totemInterval;
            }
        }

        // "Spectral sword": they float beside the ball (one per item level); each
        // darts through the nearest enemy and back, out of step with the others.
        for (int si = 0; si < 3; ++si) {
            SummonerState::Sword& sw2 = s.swords[si];
            if (si >= m.swords || m.swordEvery <= 0.f) { sw2.out = false; continue; }
            const float k = w.arenaScale();
            const float side = std::atan2(b.vel.y, b.vel.x) + kPi * 0.6f + static_cast<float>(si) * 1.2f;   // fanned out behind the ball
            const sf::Vector2f home = b.pos + sf::Vector2f{std::cos(side), std::sin(side)} * (b.radius + S::swordHover * k);
            if (!sw2.out) { sw2.pos = home; sw2.out = true; sw2.phase = 0; sw2.t = m.swordEvery * static_cast<float>(si) / 3.f; }
            sw2.clock += dt;
            if (sw2.phase == 0) {   // floating, point toward the nearest enemy
                sw2.pos += (home - sw2.pos) * (1.f - std::exp(-S::swordFollow * dt));
                sf::Vector2f to;
                const bool seen = target(w, sw2.pos, m.swordRange * k, to);
                if (seen) {
                    const float want = std::atan2(to.y - sw2.pos.y, to.x - sw2.pos.x);
                    sw2.ang += clampf(std::remainder(want - sw2.ang, 2.f * kPi), -8.f * dt, 8.f * dt);
                }
                if ((sw2.t -= dt) <= 0.f && seen) {   // dart: through it and a bit beyond
                    sw2.phase = 1;
                    sw2.clock = 0.f;
                    sw2.from = sw2.pos;
                    const sf::Vector2f d = normalized(to - sw2.pos, {1.f, 0.f});
                    sw2.to = to + d * (S::swordOvershoot * k);
                    sw2.ang = std::atan2(d.y, d.x);
                    sw2.hits = 0;
                }
            } else if (sw2.phase == 1) {   // darting: cut everything the blade passes
                const sf::Vector2f prev = sw2.pos;
                const float dash = S::swordDash * std::max(1.f, length(sw2.to - sw2.from) / (320.f * k));   // a longer dart takes longer
                sw2.pos = sw2.from + (sw2.to - sw2.from) * clampf(sw2.clock / dash, 0.f, 1.f);
                const float dmg = w.ballDamage(b, p) * m.swordFrac * pw;
                for (Enemy& e : w.enemies_) {
                    if (e.hp <= 0.f || sw2.hits >= 8) continue;
                    bool done = false;
                    for (int h = 0; h < sw2.hits; ++h) done = done || sw2.hit[h] == e.id;
                    if (done) continue;
                    const sf::Vector2f ab = sw2.pos - prev, ap = e.pos - prev;   // distance to this step's segment
                    const float tt = clampf(dot(ap, ab) / std::max(1e-3f, dot(ab, ab)), 0.f, 1.f);
                    if (length(e.pos - (prev + ab * tt)) > e.radius + S::swordWidth * k) continue;
                    sw2.hit[sw2.hits++] = e.id;
                    touch(w, e, dmg, elem, b.owner, false, p, ev);
                }
                if (w.boss_.alive && length(w.boss_.pos - sw2.pos) < w.boss_.radius + S::swordWidth * k) hitBoss(w, dmg * 0.25f);
                if (sw2.clock >= dash) { sw2.phase = 2; sw2.clock = 0.f; sw2.from = sw2.pos; }
            } else {   // flying back, spinning once
                const float q = clampf(sw2.clock / S::swordBack, 0.f, 1.f);
                sw2.pos = sw2.from + (home - sw2.from) * (q * q * (3.f - 2.f * q));
                sw2.ang += 2.f * kPi * dt / S::swordBack;
                if (q >= 1.f) { sw2.phase = 0; sw2.t = m.swordEvery; }
            }
        }

        // Warden: spirits on a ring around the core.
        if (m.wardens > 0) {
            s.wardenAng += S::wardenSpin * dt;
            {   // the ring swings out / in to meet what comes closest to the core
                const float usual = S::wardenOrbit * w.arenaScale();
                if (s.wardenR <= 0.f) s.wardenR = usual;
                const float want = w.orbitTarget(usual, w.core_.radius + S::wardenRadius + 8.f);
                s.wardenR += (want - s.wardenR) * (1.f - std::exp(-cfg::changer::orbitEase * dt));
            }
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
        const float r = b.cls.summoner.wardenR > 0.f ? b.cls.summoner.wardenR : cfg::summoner::wardenOrbit * w.arenaScale();
        return summonerWardenPos(w.core_.pos, b.cls.summoner.wardenAng, b.owner, i, b.mods.cls.summoner.wardens, r);
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
                s.life = S::wispLife * power(*best) * (1.f + best->mods.cls.common.linger);
                s.elem = static_cast<int>(best->element);
                s.owner = best->owner;
                s.wisp = true;
                sw.shots.push_back(s);
                twin(w, *best, sw.shots);   // "Twin call"
            }
        }

        for (Ball& b : w.balls_) tickBall(w, b, dt, p, ev);

        // Turrets: aim and fire at the nearest enemy in range.
        for (SummonTurret& t : sw.turrets) {
            t.life -= dt;
            t.fireT -= dt;
            sf::Vector2f to;
            if (!aim(w, t.owner, t.pos, S::turretRange, to)) continue;
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
        for (const SummonTurret& t : sw.turrets)
            if (t.life <= 0.f) martyr(w, t.pos, t.elem, t.owner, p, ev);   // "Martyr"
        sw.turrets.erase(std::remove_if(sw.turrets.begin(), sw.turrets.end(),
                                        [](const SummonTurret& t) { return t.life <= 0.f; }),
                         sw.turrets.end());

        // Shots and wisps: fly (wisps home in), hit the first enemy they touch.
        for (SummonShot& s : sw.shots) {
            s.life -= dt;
            if (s.wisp) {
                sf::Vector2f to;
                if (aim(w, s.owner, s.pos, 1e9f, to)) {
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
                if (e.id == s.lastHit) continue;   // a great spirit passing through
                touch(w, e, s.dmg, s.elem, s.owner, full, p, ev);
                s.lastHit = e.id;
                if (s.pierce > 0) --s.pierce;   // "Spirit host": on to the next one
                else s.life = 0.f;
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
        w.commonGolems(dt, p, ev);   // "Golem" (a common's summon, same rules as these)
        w.commonDragons(dt, p, ev);  // the dragons and the swarm
        for (const SummonTotem& t : sw.totems)
            if (t.life <= 0.f) martyr(w, t.pos, t.elem, t.owner, p, ev);   // "Martyr"
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
            for (auto& sw2 : s.swords) { sw2.t = 0.f; sw2.phase = 0; sw2.out = false; }
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
        if (w.classWorld_.common.rigged) o = std::max(o, rollOutcome(w, p));   // "Rigged": the better of two
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
    static void wildCard(World& w, Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents&) {
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
                w.itemFx(CommonWorld::Fx::Fuse, e.pos, r, theme::elemFire, 0.45f);
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
                    b.cls.common.doubled = true;   // Hot streak / Confetti
                    break;
                }
                case Spark: spark(w, e, dmg * J::sparkFrac, J::sparkRange); break;
                case RandomElement:   // no owner: it reacts with anything, the ball's own element too
                    if (!kill && e.hp > 0.f)
                        w.applyElement(e, static_cast<Element>(w.rng_.irange(1, kNaturalElements - 1)), -1, dmg, p, ev);
                    break;
                case Normal: break;
            }
        }
        if (j.wildChance > 0.f && roll(w, b, j.wildChance, p)) wildCard(w, b, e, dmg * j.wildPower, kill, p, ev);
        b.cls.jester.chaosArmed = false;   // "Chaos bounce": spent
        if (j.coinHeads > 0.f)   // "Coin flip": the coin this hit used, tossed up over the enemy - gold heads, grey tails
            w.itemFx(CommonWorld::Fx::Flip, e.pos - sf::Vector2f{0.f, e.radius + 4.f}, 5.f * w.arenaScale(), theme::puGolden, 0.6f,
                     {b.cls.jester.coinMul > 1.f ? 1.f : 0.f, 0.f});
        // "Coin flip": flip the next hit's coin now (damageMul reads it).
        b.cls.jester.coinMul = j.coinHeads <= 0.f ? 1.f
                             : roll(w, b, J::coinHeadsChance, p) ? j.coinHeads : J::coinTails;
    }

    static void onKill(World& w, Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev) {
        const JesterMods& j = b.mods.cls.jester;
        if (j.jackpotChance <= 0.f || !roll(w, b, j.jackpotChance, p)) return;
        w.areaDamage(e.pos, cfg::jester::jackpotRadius, dmg * j.jackpotBlast, &e);   // "Jackpot"
        ev.midasGold += j.jackpotGold;
        ev.bursts.push_back({e.pos, 0.f, theme::puGolden, "JACKPOT"});
        w.itemFx(CommonWorld::Fx::Confetti, e.pos, cfg::jester::jackpotRadius, theme::puGolden, 0.9f);
    }

    // The chaos: now and then it draws a card. The suit says what kind of
    // trick, the rank how big; the face cards are their own.
    //   Hearts   - mends the core, and every ball quickens (Tailwind)
    //   Diamonds - gold, and a burst of bullets
    //   Spades   - blades (low) or a smite (high)
    //   Clubs    - a nova that shoves (low) or a chain zap (high)
    //   Jack: a blink   Queen: a flock of missiles   King: a launch at full tilt and a nova where it lands
    //   Ace: one of every suit
    static const char* cardName(int suit, int rank) {
        static std::string names[4][14];
        static bool built = false;
        if (!built) {
            static const char* kSuit[4] = {"HEARTS", "DIAMONDS", "SPADES", "CLUBS"};
            static const char* kRank[14] = {"", "ACE", "2", "3", "4", "5", "6", "7", "8", "9", "10", "JACK", "QUEEN", "KING"};
            for (int s2 = 0; s2 < 4; ++s2)
                for (int r = 1; r <= 13; ++r) names[s2][r] = std::string(kRank[r]) + " OF " + kSuit[s2] + "!";
            built = true;
        }
        return names[suit][rank].c_str();
    }
    static bool suitTrick(World& w, Ball& b, int suit, int rank, const WorldParams& p, FrameEvents& ev) {
        const int lvl = 1 + rank / 4;
        const bool high = rank >= 8;
        switch (suit) {
            case 0:   // hearts
                w.repairCore(w.core_.maxHp * 0.006f * static_cast<float>(rank));
                w.fireAbility(b, AbilitySpec{Ability::Tailwind, lvl}, p, ev);
                return true;
            case 1: {   // diamonds
                ev.midasGold += std::max(1, rank / 3);
                w.commonVolley(b, p, 2 + rank / 2, b.pos);
                return true;
            }
            case 2: return w.fireAbility(b, AbilitySpec{high ? Ability::Smite : Ability::Blades, lvl}, p, ev);
            default: return w.fireAbility(b, AbilitySpec{high ? Ability::Arc : Ability::Nova, lvl}, p, ev);
        }
    }
    static void chaos(World& w, Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
        namespace J = cfg::jester;
        if (!b.hasRole(BallRole::Jester) || b.ghost || b.held || !w.waveRunning_) return;
        JesterState& s = b.cls.jester;
        const int items = b.mods.cls.jester.items;
        if ((s.chaosT -= dt) > 0.f) return;
        int alive = 0;
        for (const Enemy& e : w.enemies_) alive += e.hp > 0.f && !e.orbiter ? 1 : 0;
        if (alive == 0 && !(w.boss_.alive && w.boss_.intro <= 0.f)) { s.chaosT = 0.f; return; }   // a card for when it counts
        s.chaosT = std::max(J::chaosMin, J::chaosEvery - J::chaosPerItem * static_cast<float>(items));
        const int draws = 1 + (items >= 3 ? 1 : 0) + (b.isAscended(BallRole::Jester) ? 1 : 0) + (items >= 6 ? 1 : 0);
        for (int d = 0; d < draws; ++d) {
            // the deeper the bag, the higher the cards run
            const int rank = std::clamp(w.rng_.irange(1, 13) + (w.rng_.irange(0, 99) < 10 * items ? 3 : 0), 1, 13);
            const int suit = w.rng_.irange(0, 3);
            w.castBall_ = &b;
            const int lvl = 1 + rank / 4;
            if (rank == 11) w.fireAbility(b, AbilitySpec{Ability::Warp, lvl}, p, ev);   // Jack: a blink
            else if (rank == 12)   // Queen: a flock of missiles
                w.mageMissiles(b, 3 + items / 2, cfg::ability::missileFrac + cfg::ability::missileFracPerLevel * static_cast<float>(lvl), p);
            else if (rank == 13) {   // King: flung at full tilt, a nova where it is
                if (w.fireAbility(b, AbilitySpec{Ability::Dash, lvl}, p, ev)) b.vel = normalized(b.vel, {1.f, 0.f}) * w.ballMaxSpeed(b, p);
                w.fireAbility(b, AbilitySpec{Ability::Nova, lvl}, p, ev);
            } else if (rank == 1) {   // Ace: a bit of every suit
                for (int s2 = 0; s2 < 4; ++s2) suitTrick(w, b, s2, 9, p, ev);
            } else {
                suitTrick(w, b, suit, rank, p, ev);
            }
            w.castBall_ = nullptr;
            ev.bursts.push_back({b.pos, 0.f, suit < 2 ? sf::Color(236, 70, 80) : theme::classJester, cardName(suit, rank)});
            w.itemFx(CommonWorld::Fx::Card, b.pos - sf::Vector2f{0.f, b.radius + 18.f * w.arenaScale()}, b.radius, theme::classJester, 1.1f,
                     {static_cast<float>(suit), static_cast<float>(rank)});
        }
    }

    // "Sleight": the slower it moves, the sooner it vanishes and reappears on
    // a random enemy, striking it.
    static void tick(World& w, Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
        chaos(w, b, dt, p, ev);
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
        w.itemFx(CommonWorld::Fx::Cards, t->pos, t->radius * 2.f, theme::classJester, 0.5f);
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
        if (!b.held) st.idleT += dt;   // the class's slowdown (World::roleCruise)
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

    static void onGrab(World& w, Ball& b) {
        namespace S = cfg::slinger;
        SlingerState& st = b.cls.slinger;
        const SlingerMods& m = b.mods.cls.slinger;
        const bool quick = st.sinceThrow < S::releaseWindow;   // caught back soon after your throw
        if (m.releaseMax > 0) st.stacks = quick ? std::min(st.stacks + 1, m.releaseMax) : 0;   // "Catch & release"
        if (b.isAscended(BallRole::Slinger))   // Master Slinger: a catch recharges its abilities
            w.refundAbilities(b, 0.f, S::masterRecharge);
    }

    static void onThrow(World&, Ball& b) {
        SlingerState& st = b.cls.slinger;
        st.sinceThrow = 0.f;
        st.idleT = 0.f;
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

    static float preHit(World&, Ball& b, const Enemy& e, const WorldParams&) {
        namespace S = cfg::slinger;
        SlingerState& st = b.cls.slinger;
        const SlingerMods& m = b.mods.cls.slinger;
        float k = 1.f;
        if (!st.armed) return k;
        if (b.hasRole(BallRole::Slinger))   // the class: your throw's first hit lands harder
            k *= b.isAscended(BallRole::Slinger) ? S::masterThrownHit : S::thrownHit;
        if (m.execution > 0.f && e.hp >= e.maxHp) k *= m.execution;   // "Execution throw"
        return k;
    }

    static void onHit(World& w, Ball& b, Enemy& e, float dmg, bool, const WorldParams& p, FrameEvents&) {
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
        if (const Enemy* f = w.focusEnemy(); f && f != &e && dot(f->pos - b.pos, f->pos - b.pos) <=
                                                  cfg::slinger::ambushRange * w.arenaScale() *
                                                      cfg::slinger::ambushRange * w.arenaScale())
            t = const_cast<Enemy*>(f);   // the player's target, when it's in range
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
        w.itemFx(CommonWorld::Fx::Arrow, t->pos, t->radius * 2.2f, theme::classSlinger, 0.4f, dir);
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

    static void waveStart(World& w, const WorldParams&) {
        w.classWorld_.slinger.flames.clear();
        for (Ball& b : w.balls_) b.cls.slinger.idleT = 0.f;   // the opening whirl counts as a throw
    }
};

// "Golem" (a rare, sim/WorldCommons.cpp): every so often a ball calls a stone
// golem that walks to the enemy nearest the core and slams what's around it
// away from the core. A summon like the Summoner's: Guard dog, Soulbind,
// Lingering, Martyr and Pack leader all count it.
void World::commonGolems(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace C = cfg::common;
    using H = ClassHooks<BallRole::Summoner>;
    auto& gs = classWorld_.common.golems;
    const float k = arenaScale();
    for (Ball& b : balls_) {   // the real balls call them
        const CommonMods& c = b.mods.cls.common;
        if (c.golemEvery <= 0.f || !waveRunning_ || b.held) continue;
        if ((b.cls.common.golemT -= dt) > 0.f) continue;
        b.cls.common.golemT = c.golemEvery;
        if (static_cast<int>(gs.size()) >= C::maxGolems) continue;
        CommonWorld::Golem g;
        g.pos = b.pos;
        g.life = g.maxLife = C::golemLife * H::power(b) * (1.f + c.linger);
        g.dmg = ballDamage(b, p) * c.golemFrac * H::power(b);
        g.elem = static_cast<int>(b.element);
        g.owner = b.owner;
        gs.push_back(g);
        H::twin(*this, b, gs);   // "Twin call"
        itemFx(CommonWorld::Fx::Rise, g.pos, C::golemRadius * 2.f * k, theme::classSummoner, 0.5f);
    }
    for (CommonWorld::Golem& g : gs) {
        g.life -= dt;
        g.slamT -= dt;
        g.flash = std::max(0.f, g.flash - dt * 3.f);
        const Enemy* t = nullptr;   // the enemy nearest the core
        float bd = 1e18f;
        for (const Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const float d2 = dot(e.pos - core_.pos, e.pos - core_.pos);
            if (d2 < bd) { bd = d2; t = &e; }
        }
        if (!t) continue;
        const sf::Vector2f to = t->pos - g.pos;
        const float reach = t->radius + C::golemRadius * k + 6.f;
        if (length(to) > reach) {
            g.pos += normalized(to, {1.f, 0.f}) * std::min(length(to) - reach, C::golemSpeed * k * dt);
            continue;
        }
        if (g.slamT > 0.f) continue;
        g.slamT = C::golemSlamEvery;
        g.flash = 1.f;
        const float R = C::golemSlamRadius * k;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - g.pos) > R + e.radius) continue;
            H::touch(*this, e, g.dmg, g.elem, g.owner, false, p, ev);
            e.vel += normalized(e.pos - core_.pos, {1.f, 0.f}) * C::golemKnock * k * e.knockTaken;
            e.stagger = std::max(e.stagger, 0.3f);
        }
        itemFx(CommonWorld::Fx::Slam, g.pos, R, theme::classSummoner, 0.4f);
    }
    for (const CommonWorld::Golem& g : gs)
        if (g.life <= 0.f) H::martyr(*this, g.pos, g.elem, g.owner, p, ev);   // "Martyr"

    // "Spirit host": five of a ball's wisps out merge into one great spirit -
    // and it lets wisps loose on its own now and then, so it needs no other item.
    auto& shots = classWorld_.summoner.shots;
    for (Ball& b : balls_) {
        const CommonMods& c = b.mods.cls.common;
        if (c.spiritHost <= 0.f) continue;
        if (waveRunning_ && !b.held && (b.cls.common.spiritT -= dt) <= 0.f) {
            b.cls.common.spiritT = C::spiritWispEvery;
            H::hatch(*this, b, b.pos, 1, p);
        }
        int n = 0;
        for (const SummonShot& s : shots) n += s.wisp && !s.great && s.owner == b.owner ? 1 : 0;
        if (n < C::spiritAt) continue;
        SummonShot great;
        sf::Vector2f at{0.f, 0.f};
        float dmg = 0.f;
        int taken = 0;
        for (SummonShot& s : shots)
            if (taken < C::spiritAt && s.wisp && !s.great && s.owner == b.owner && s.life > 0.f) {
                at += s.pos;
                dmg += s.dmg;
                great.elem = s.elem;
                s.life = 0.f;   // swept by the summons' own pass
                ++taken;
            }
        great.pos = at / static_cast<float>(taken);
        great.vel = {0.f, 0.f};
        great.dmg = dmg / static_cast<float>(taken) * c.spiritHost;
        great.life = cfg::summoner::wispLife * C::spiritLife * (1.f + c.linger);
        great.owner = b.owner;
        great.wisp = true;
        great.great = true;
        great.pierce = c.spiritPierce;
        shots.push_back(great);
        itemFx(CommonWorld::Fx::Spiral, great.pos, 50.f * k, theme::classSummoner, 0.6f);
    }
    gs.erase(std::remove_if(gs.begin(), gs.end(), [](const CommonWorld::Golem& g) { return g.life <= 0.f; }), gs.end());
}

// The Summoner's big ones (2026-10-05). "Baby dragon" (epic): a little dragon
// rides beside its ball and breathes a cone of its element. "Elder dragon"
// (legendary, the one): a huge dragon circles the arena all fight; its breath
// sweeps a long, wide swath. "Hive" (legendary, the many): tiny bugs hatch,
// run at the nearest enemy and nibble it. Together: the elder's roar makes
// every baby breathe (and harder), a baby's breath lights bugs (their next
// bite explodes), and the elder's swath hatches more bugs.
void World::commonDragons(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace C = cfg::common;
    using H = ClassHooks<BallRole::Summoner>;
    CommonWorld& cw = classWorld_.common;
    const float k = arenaScale();
    if (!waveRunning_) return;
    bool anyElder = false, anyHive = false;
    for (const Ball& b : balls_) {
        anyElder = anyElder || b.mods.cls.common.elderDragon > 0.f;
        anyHive = anyHive || b.mods.cls.common.hive > 0.f;
    }

    // Who should have which dragons: one baby per ball (two with Twin call), one elder.
    auto has = [&](int owner, bool elder, int side) {
        for (const auto& d : cw.dragons)
            if (d.owner == owner && d.elder == elder && d.side == side) return true;
        return false;
    };
    for (const Ball& b : balls_) {
        const CommonMods& c = b.mods.cls.common;
        const int babies = c.babyDragon > 0.f ? (c.twinCall ? 2 : 1) : 0;
        for (int side = 0; side < babies; ++side)
            if (!has(b.owner, false, side)) {
                CommonWorld::Dragon d;
                d.owner = b.owner;
                d.side = side;
                d.ang = kPi * static_cast<float>(side);
                d.pos = b.pos;
                d.breathT = c.babyEvery * 0.5f;
                cw.dragons.push_back(d);
            }
        if (c.elderDragon > 0.f && !has(b.owner, true, 0)) {
            CommonWorld::Dragon d;
            d.owner = b.owner;
            d.elder = true;
            d.ang = rng_.range(0.f, 2.f * kPi);
            d.pos = core_.pos + sf::Vector2f{std::cos(d.ang), std::sin(d.ang)} * (std::min(size_.x, size_.y) * 2.f);   // flies in from afar
            d.breathT = c.elderEvery * 0.6f;
            cw.dragons.push_back(d);
        }
    }
    cw.dragons.erase(std::remove_if(cw.dragons.begin(), cw.dragons.end(), [&](const CommonWorld::Dragon& d) {
                         const Ball* o = H::ownerBall(*this, d.owner);
                         if (!o) return true;
                         const CommonMods& c = o->mods.cls.common;
                         return d.elder ? c.elderDragon <= 0.f : (c.babyDragon <= 0.f || d.side >= (c.twinCall ? 2 : 1));
                     }),
                     cw.dragons.end());

    auto nearest = [&](sf::Vector2f from, float range) -> Enemy* {
        Enemy* t = nullptr;
        float bd = range * range;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const float d2 = dot(e.pos - from, e.pos - from);
            if (d2 < bd) { bd = d2; t = &e; }
        }
        return t;
    };
    // The body faces its flight; only the head (and its fire) swings round to a
    // mark, never further than headReach from the body, never snapping.
    auto turnHead = [&](CommonWorld::Dragon& d, sf::Vector2f want, float rate) {
        const float body = std::atan2(d.face.y, d.face.x);
        const float cur = std::atan2(d.head.y, d.head.x);
        float to = std::atan2(want.y - d.pos.y, want.x - d.pos.x);
        to = body + clampf(std::remainder(to - body, 2.f * kPi), -C::headReach, C::headReach);
        const float da = clampf(std::remainder(to - cur, 2.f * kPi), -rate * dt, rate * dt);
        d.head = {std::cos(cur + da), std::sin(cur + da)};
    };
    auto settleHead = [&](CommonWorld::Dragon& d, float rate) {   // idle: the head follows the body
        turnHead(d, d.pos + d.face * 100.f, rate);
    };
    auto faceFlight = [&](CommonWorld::Dragon& d, sf::Vector2f step, float rate) {
        if (length(step) < 1e-3f) return;
        const float cur = std::atan2(d.face.y, d.face.x), to = std::atan2(step.y, step.x);
        const float da = clampf(std::remainder(to - cur, 2.f * kPi), -rate * dt, rate * dt);
        const sf::Vector2f nf{std::cos(cur + da), std::sin(cur + da)};
        const float hd = std::atan2(d.head.y, d.head.x) + da;   // the head turns with the body
        d.face = nf;
        d.head = {std::cos(hd), std::sin(hd)};
    };
    auto byId = [&](int id) -> Enemy* {
        for (Enemy& e : enemies_)
            if (e.id == id && e.hp > 0.f) return &e;
        return nullptr;
    };
    // A mark is an enemy id, or kBossMark for the boss (2026-10-06: dragons used to ignore it).
    constexpr int kBossMark = -2;
    auto markPos = [&](int id, sf::Vector2f& at) -> bool {
        if (id == kBossMark) {
            if (!boss_.targetable()) return false;
            at = boss_.pos;
            return true;
        }
        if (Enemy* e = byId(id)) { at = e->pos; return true; }
        return false;
    };
    auto pickMark = [&](sf::Vector2f from, float range) -> int {   // the player's target, else the nearest enemy or the boss
        if (const Enemy* f = focusEnemy(); f && length(f->pos - from) - f->radius <= range) return f->id;
        if (focusIsBoss() && boss_.targetable() && length(boss_.pos - from) - boss_.radius <= range) return kBossMark;
        Enemy* e = nearest(from, range);
        const float bd = boss_.targetable() ? length(boss_.pos - from) - boss_.radius : 1e9f;
        if (bd <= range && (!e || bd < length(e->pos - from) - e->radius)) return kBossMark;
        return e ? e->id : -1;
    };
    auto mouth = [&](const CommonWorld::Dragon& d) { return d.pos + d.head * ((d.elder ? 46.f : 10.f) * k * 0.65f); };
    // One bite of a breath: a baby's cone, or the elder's long swath, out of its mouth.
    auto burn = [&](CommonWorld::Dragon& d, const Ball& o, float share) {
        const CommonMods& c = o.mods.cls.common;
        const int elem = static_cast<int>(hitElement(o, p));
        const sf::Vector2f m = mouth(d), dir = d.head;
        if (!d.elder) {
            const float dmg = ballDamage(o, p) * c.babyDragon * H::power(o) * (anyElder ? C::elderBabyMul : 1.f) * share;
            const float reach = C::babyRange * k, cosCone = std::cos(C::babyCone);
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                const sf::Vector2f to = e.pos - m;
                const float l = length(to);
                if (l > reach + e.radius || (l > 1.f && dot(to / l, dir) < cosCone)) continue;
                H::touch(*this, e, dmg, elem, o.owner, false, p, ev);
            }
            if (boss_.targetable()) {   // ...the boss too, if it's in the cone
                const sf::Vector2f to = boss_.pos - m;
                const float l = length(to);
                if (l <= reach + boss_.radius && (l < 1.f || dot(to / l, dir) >= cosCone ||
                                                  l - boss_.radius < reach * 0.3f))
                    H::hitBoss(*this, dmg);
            }
            for (auto& g : cw.bugs) {   // ...and lights any bugs it passes over
                const sf::Vector2f to = g.pos - m;
                const float l = length(to);
                if (l < reach && (l < 1.f || dot(to / l, dir) >= cosCone)) g.lit = true;
            }
            return;
        }
        const float L = C::elderLength * k, W = C::elderWidth * k;
        const float dmg = ballDamage(o, p) * c.elderDragon * H::power(o) * share;
        const sf::Vector2f side{-dir.y, dir.x};
        for (Enemy& e : enemies_) {   // everything in the swath
            if (e.hp <= 0.f || e.orbiter) continue;
            const sf::Vector2f to = e.pos - m;
            const float along = dot(to, dir);
            if (along < 0.f || along > L || std::fabs(dot(to, side)) > W * (0.4f + 0.6f * along / L) + e.radius) continue;
            H::touch(*this, e, dmg, elem, o.owner, true, p, ev);
        }
        if (boss_.targetable()) {
            const sf::Vector2f to = boss_.pos - m;
            const float along = dot(to, dir);
            if (along > 0.f && along < L + boss_.radius && std::fabs(dot(to, side)) < W + boss_.radius)
                H::hitBoss(*this, dmg);
        }
    };

    bool roared = false;
    for (auto& d : cw.dragons) {
        const Ball* o = H::ownerBall(*this, d.owner);
        if (!o) continue;
        const CommonMods& c = o->mods.cls.common;
        d.flap += dt * (d.elder ? 3.f : 9.f);
        d.roar = std::max(0.f, d.roar - dt * 1.5f);
        const float turn = d.elder ? C::elderTurn : C::babyTurn;
        const float range = d.elder ? 1e9f : C::babyRange * k;
        // Where it flies: a baby beside its ball, the elder on a slow wide circle round the core.
        sf::Vector2f step;
        if (!d.elder) {
            d.ang += dt * 2.2f;
            const sf::Vector2f want = o->pos + sf::Vector2f{std::cos(d.ang), std::sin(d.ang)} * (C::babyOrbit * k + o->radius);
            step = (want - d.pos) * std::min(1.f, dt * 10.f);
        } else {
            d.ang += C::elderSpin * dt;
            const float R = std::min(size_.x, size_.y) * C::elderOrbit;
            const sf::Vector2f want = core_.pos + sf::Vector2f{std::cos(d.ang), std::sin(d.ang)} * R;
            step = (want - d.pos) * std::min(1.f, dt * 1.5f);
        }
        d.pos += step;
        faceFlight(d, step, d.elder ? 2.5f : 8.f);
        sf::Vector2f tp;
        bool hasMark = markPos(d.target, tp);
        auto remark = [&] {   // lost its mark: the next one in reach
            if (hasMark) return;
            d.target = pickMark(d.pos, range);
            hasMark = markPos(d.target, tp);
        };
        if (d.breathing > 0.f) {   // a breath of fire: the head tracks its mark, the fire bites as it goes
            remark();
            if (hasMark) turnHead(d, tp, turn * 0.6f);
            d.breathing -= dt;
            d.roar = 1.f;
            if ((d.tickT -= dt) <= 0.f) {
                const float tick = d.elder ? C::elderTick : C::breathTick;
                d.tickT += tick;
                burn(d, *o, d.elder ? tick / C::elderBreathRef * C::elderSustain : tick / C::babyBreathTime);
            }
            if (d.breathing <= 0.f) d.breathT = d.elder ? c.elderEvery : c.babyEvery;
            continue;
        }
        if (d.charge >= 0.f) {   // drawing breath: the head turns to the mark
            remark();
            if (hasMark) turnHead(d, tp, turn);
            if ((d.charge += dt) >= (d.elder ? C::elderCharge : C::babyCharge)) {
                d.charge = -1.f;
                d.breathing = d.elder ? C::elderBreathTime : C::babyBreathTime;
                d.tickT = 0.f;
                if (d.elder) {
                    roared = true;
                    ev.kick = std::max(ev.kick, 6.f);
                    if (anyHive)   // the fire hatches bugs along its path
                        for (const Ball& hb : balls_) {
                            if (hb.mods.cls.common.hive <= 0.f) continue;
                            for (int i = 0; i < C::elderHiveBugs && static_cast<int>(cw.bugs.size()) < C::hiveMax; ++i) {
                                CommonWorld::Bug g;
                                g.pos = mouth(d) + d.head * (C::elderLength * k * (0.2f + 0.2f * static_cast<float>(i)));
                                g.vel = rng_.direction() * 40.f;
                                g.life = C::hiveLife;
                                g.dmg = ballDamage(hb, p) * hb.mods.cls.common.hive * H::power(hb);
                                g.elem = static_cast<int>(hitElement(hb, p));
                                g.owner = hb.owner;
                                cw.bugs.push_back(g);
                            }
                            break;
                        }
                }
            }
            continue;
        }
        settleHead(d, turn * 0.5f);
        if ((d.breathT -= dt) > 0.f) continue;
        d.breathT = 0.f;   // ready: it starts the moment something is in reach
        if (const int mk = pickMark(d.pos, range); mk != -1) { d.charge = 0.f; d.target = mk; }
    }
    if (roared)   // the young answer the elder's roar: they all draw breath at once
        for (auto& d : cw.dragons)
            if (!d.elder && d.charge < 0.f && d.breathing <= 0.f)
                if (const int mk = pickMark(d.pos, C::babyRange * k * 1.5f); mk != -1) { d.charge = 0.f; d.target = mk; }

    // The Hive: every ball carrying it hatches a few bugs a second.
    if (cw.hiveT.size() < balls_.size()) cw.hiveT.resize(balls_.size(), 0.f);
    for (std::size_t i = 0; i < balls_.size(); ++i) {
        const Ball& b = balls_[i];
        const CommonMods& c = b.mods.cls.common;
        if (c.hive <= 0.f || b.held) continue;
        if ((cw.hiveT[i] -= dt) > 0.f) continue;
        cw.hiveT[i] = C::hiveEvery;
        for (int n = 0; n < c.hivePer && static_cast<int>(cw.bugs.size()) < C::hiveMax; ++n) {
            CommonWorld::Bug g;
            g.pos = b.pos;
            g.vel = rng_.direction() * C::hiveSpeed * k * 0.6f;
            g.life = C::hiveLife * (1.f + c.linger);
            g.dmg = ballDamage(b, p) * c.hive * H::power(b);
            g.elem = static_cast<int>(hitElement(b, p));
            g.owner = b.owner;
            cw.bugs.push_back(g);
        }
    }
    std::vector<CommonWorld::Bug> split;   // bugs born of a kill this step
    for (auto& g : cw.bugs) {   // run at the nearest enemy (or the boss), nibble it
        g.life -= dt;
        g.biteT -= dt;
        Enemy* t = const_cast<Enemy*>(focusEnemy());   // the player's target first
        if (!t) t = nearest(g.pos, 1e9f);
        // The boss counts too (2026-10-06: it used to be ignored) when it's the target, closer or alone.
        const bool atBoss = boss_.targetable() && (focusIsBoss() || (!focusEnemy() && (!t || length(boss_.pos - g.pos) - boss_.radius <
                                                                                             length(t->pos - g.pos) - t->radius)));
        if (!t && !atBoss) { g.vel *= std::exp(-3.f * dt); g.pos += g.vel * dt; continue; }
        const sf::Vector2f tpos = atBoss ? boss_.pos : t->pos;
        const float trad = atBoss ? boss_.radius : t->radius;
        const sf::Vector2f to = tpos - g.pos;
        const float l = length(to);
        const float jit = (rng_.range(0.f, 1.f) - 0.5f) * 1.2f;   // a scurry, not a beeline
        const sf::Vector2f dir = normalized(to, {1.f, 0.f});
        const sf::Vector2f want = sf::Vector2f{dir.x - dir.y * jit, dir.y + dir.x * jit} * C::hiveSpeed * k;
        g.vel += (want - g.vel) * std::min(1.f, dt * 6.f);
        if (l > trad + 2.f) g.pos += g.vel * dt;
        if (l > trad + 4.f * k || g.biteT > 0.f) continue;
        g.biteT = C::hiveBiteEvery;
        if (atBoss) {   // a bite chips the boss like any summon (a lit one goes up on it)
            H::hitBoss(*this, g.dmg * (g.lit ? C::hiveLitBlast : 1.f));
            if (g.lit) {
                itemFx(CommonWorld::Fx::Blast, g.pos, C::hiveLitRadius * k,
                       g.elem != 0 ? elementColor(static_cast<Element>(g.elem)) : theme::elemFire, 0.35f);
                g.life = 0.f;
            }
            continue;
        }
        dmgOwner_ = g.owner;   // kill credit: the Swarm's ball (its bites carry no owner for reactions)
        if (g.lit) {   // lit by a dragon: it goes up
            const float R = C::hiveLitRadius * k;
            for (Enemy& e : enemies_)
                if (e.hp > 0.f && length(e.pos - g.pos) <= R + e.radius) H::touch(*this, e, g.dmg * C::hiveLitBlast, g.elem, -1, false, p, ev);
            if (boss_.targetable() && length(boss_.pos - g.pos) <= R + boss_.radius) H::hitBoss(*this, g.dmg * C::hiveLitBlast);
            itemFx(CommonWorld::Fx::Blast, g.pos, R, g.elem != 0 ? elementColor(static_cast<Element>(g.elem)) : theme::elemFire, 0.35f);
            g.life = 0.f;
            dmgOwner_ = -1;
            continue;
        }
        const bool alive = t->hp > 0.f;
        H::touch(*this, *t, g.dmg, g.elem, -1, false, p, ev);   // no owner: its element reacts with anything
        dmgOwner_ = -1;
        if (alive && t->hp <= 0.f && static_cast<int>(cw.bugs.size() + split.size()) < C::hiveMax) {   // a kill: it splits
            g.life = std::max(g.life, C::hiveSplitLife);
            CommonWorld::Bug twin = g;
            twin.vel = sf::Vector2f{-g.vel.y, g.vel.x};
            split.push_back(twin);
        }
    }
    for (const auto& g : split) cw.bugs.push_back(g);
    cw.bugs.erase(std::remove_if(cw.bugs.begin(), cw.bugs.end(), [](const CommonWorld::Bug& g) { return g.life <= 0.f; }),
                  cw.bugs.end());
}

// "Shrapnel" (an epic): a ring of bullets out of the enemy it just killed.
void World::commonRing(Ball& b, const WorldParams& p, sf::Vector2f at, int n) {
    if (n <= 0) return;
    auto& bullets = classWorld_.shooter.bullets;
    const float dmg = ballDamage(b, p) * cfg::shooter::bulletFrac * cfg::common::shrapnelFrac;
    const float spin = rng_.range(0.f, 2.f * kPi);
    for (int i = 0; i < n; ++i) {
        const float a = spin + 2.f * kPi * static_cast<float>(i) / static_cast<float>(n);
        const std::size_t before = bullets.size();
        ClassHooks<BallRole::Shooter>::fire(*this, b, {std::cos(a), std::sin(a)}, dmg, p);
        if (bullets.size() > before) bullets.back().pos = at;   // out of the dead enemy, not the ball
    }
}

// Ares (sim/WorldAres.cpp): a bullet from one of its guns (a cannon, a hoplite) - the Shooter's gun.
void World::aresShot(Ball& b, sf::Vector2f from, sf::Vector2f dir, float dmg, const WorldParams& p) {
    auto& bullets = classWorld_.shooter.bullets;
    const std::size_t before = bullets.size();
    ClassHooks<BallRole::Shooter>::fire(*this, b, dir, dmg, p);
    if (bullets.size() > before) bullets.back().pos = from;
}

// "Gunship" (a legendary): a ball that hits fires one bullet at another enemy,
// with the gun - and the Shooter items - of the ball carrying Gunship.
void World::commonGunship(Ball& b, const Enemy& e, const WorldParams& p) {
    Ball* g = nullptr;
    for (Ball& o : balls_)
        if (&o != &b && o.owner != b.owner && !o.ghost && o.mods.cls.common.gunship > 0.f &&
            (!g || o.mods.cls.common.gunship > g->mods.cls.common.gunship))
            g = &o;
    if (!g) return;
    const Enemy* t = nullptr;
    float best = 1e18f;
    for (const Enemy& o : enemies_) {
        if (&o == &e || o.hp <= 0.f || o.orbiter) continue;
        const float d2 = dot(o.pos - b.pos, o.pos - b.pos);
        if (d2 < best) { best = d2; t = &o; }
    }
    const sf::Vector2f dir = normalized((t ? t->pos : e.pos) - b.pos, {1.f, 0.f});
    auto& bullets = classWorld_.shooter.bullets;
    const std::size_t before = bullets.size();
    ClassHooks<BallRole::Shooter>::fire(*this, *g, dir, ballDamage(b, p) * g->mods.cls.common.gunship, p);
    if (bullets.size() > before) bullets.back().pos = b.pos + dir * b.radius;   // out of the ball that hit
}

// "Covering fire" / "Ammo feed" (commons, sim/WorldCommons.cpp): a fan at the
// enemy nearest `from` - the Shooter's gun, on any ball.
void World::commonVolley(Ball& b, const WorldParams& p, int n, sf::Vector2f from) {
    if (n <= 0) return;
    const Enemy* t = nullptr;
    float best = 1e18f;
    for (const Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const float d2 = dot(e.pos - from, e.pos - from);
        if (d2 < best) { best = d2; t = &e; }
    }
    if (!t) return;
    const sf::Vector2f dir = normalized(t->pos - b.pos, {1.f, 0.f});
    const float base = std::atan2(dir.y, dir.x);
    const float dmg = ballDamage(b, p) * cfg::shooter::bulletFrac * cfg::common::coverFrac;
    for (int i = 0; i < n; ++i) {
        const float a = base + (static_cast<float>(i) - 0.5f * static_cast<float>(n - 1)) * cfg::common::coverSpread;
        ClassHooks<BallRole::Shooter>::fire(*this, b, {std::cos(a), std::sin(a)}, dmg, p);
    }
}

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
