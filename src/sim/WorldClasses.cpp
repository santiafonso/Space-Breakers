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
// Fires bullets.
template <> struct ClassHooks<BallRole::Shooter> : NoClassHooks {
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
        g.ghostLife = life;
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
