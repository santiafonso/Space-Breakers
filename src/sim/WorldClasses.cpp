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
// More ability slots: see abilitySlotCount (progression/Offers.hpp).
template <> struct ClassHooks<BallRole::Mage> : NoClassHooks {
};

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
            w.damageEnemy(e, dmg * (e.mark > 0.f ? cfg::role::markDamageMul : 1.f));
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
        const float speedRate = clampf(length(b.vel) / std::max(1.f, w.ballCruise(b, p)), S::rateMin, S::rateMax);
        st.fireT -= dt * speedRate * m.rate;
        if (st.fireT > 0.f) return;
        sf::Vector2f at;
        if (!aimAt(w, b.pos, S::range * w.arenaScale(), -1, at)) { st.fireT = 0.f; return; }   // wait, loaded
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
            w.damageEnemy(e, u.dmg * (e.mark > 0.f ? cfg::role::markDamageMul : 1.f));
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
// Teleports to the nearest enemy on a kill.
template <> struct ClassHooks<BallRole::Assassin> : NoClassHooks {
};

// ==================================================================== Summoner
// Summons things: short-lived balls, turrets, a small dragon...
template <> struct ClassHooks<BallRole::Summoner> : NoClassHooks {
};

// ==================================================================== Jester
// Plays on chance.
template <> struct ClassHooks<BallRole::Jester> : NoClassHooks {
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
}

constexpr RoleMask kAllClasses = ~0u;

}  // namespace

void World::classTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    eachClass(b.roles, [&](auto h) { decltype(h)::tick(*this, b, dt, p, ev); });
}

void World::classOnHit(Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev) {
    eachClass(b.roles, [&](auto h) { decltype(h)::onHit(*this, b, e, dmg, kill, p, ev); });
}

void World::classOnKill(Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev) {
    eachClass(b.roles, [&](auto h) { decltype(h)::onKill(*this, b, e, dmg, p, ev); });
}

void World::classOnWallBounce(Ball& b, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev) {
    eachClass(b.roles, [&](auto h) { decltype(h)::onWallBounce(*this, b, normal, p, ev); });
}

void World::classOnCoreBounce(Ball& b, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev) {
    eachClass(b.roles, [&](auto h) { decltype(h)::onCoreBounce(*this, b, normal, p, ev); });
}

float World::classDamageMul(const Ball& b, const WorldParams& p) const {
    float m = 1.f;
    eachClass(b.roles, [&](auto h) { m *= decltype(h)::damageMul(*this, b, p); });
    return m;
}

void World::classWorldTick(float dt, const WorldParams& p, FrameEvents& ev) {
    eachClass(kAllClasses, [&](auto h) { decltype(h)::worldTick(*this, dt, p, ev); });
}

void World::classWaveStart(const WorldParams& p) {
    eachClass(kAllClasses, [&](auto h) { decltype(h)::waveStart(*this, p); });
}

}  // namespace sb
