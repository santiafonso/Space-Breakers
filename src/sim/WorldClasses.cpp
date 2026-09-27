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
// Its items act through casts and cooldowns, so they work on ANY ball that
// holds them (Mage class or not): World::mageCastRate / mageOnCast / mageTick
// below, called from sim/WorldAbilities.cpp.
template <> struct ClassHooks<BallRole::Mage> : NoClassHooks {
    static void onHit(World&, Ball& b, Enemy&, float, bool, const WorldParams&, FrameEvents&) {
        if (b.ghost) return;   // copies never cast
        for (float& cd : b.abilityCd) cd = std::max(0.f, cd - cfg::mage::hitRefund);
    }
    static void worldTick(World& w, float dt, const WorldParams&, FrameEvents&) {
        auto& s = w.classWorld_.mage.streaks;
        for (auto& k : s) k.life -= dt;
        s.erase(std::remove_if(s.begin(), s.end(), [](const MageWorld::Streak& k) { return k.life <= 0.f; }), s.end());
    }
    static void waveStart(World& w, const WorldParams&) {
        w.classWorld_.mage.streaks.clear();
        for (Ball& b : w.balls_) {
            b.cls.mage.echoSlot = -1;
            b.cls.mage.missileT = b.mods.cls.mage.missileEvery * 0.5f;
        }
    }

    // "Arcane missile": up to missileTargets of the nearest enemies. False if none in reach.
    static bool missileVolley(World& w, Ball& b, const WorldParams& p, FrameEvents& ev) {
        const MageMods& g = b.mods.cls.mage;
        const float range = cfg::mage::missileRange * w.arenaScale();
        std::vector<Enemy*> near;
        for (Enemy& e : w.enemies_)
            if (e.hp > 0.f && !e.orbiter && length(e.pos - b.pos) < range) near.push_back(&e);
        if (near.empty()) return false;
        const std::size_t n = std::min(near.size(), static_cast<std::size_t>(std::max(1, g.missileTargets)));
        std::partial_sort(near.begin(), near.begin() + static_cast<std::ptrdiff_t>(n), near.end(),
                          [&b](const Enemy* x, const Enemy* y) {
                              return dot(x->pos - b.pos, x->pos - b.pos) < dot(y->pos - b.pos, y->pos - b.pos);
                          });
        const float dmg = w.ballDamage(b, p) * g.missileFrac * g.power;
        const Element el = w.hitElement(b, p);
        auto& streaks = w.classWorld_.mage.streaks;
        for (std::size_t i = 0; i < n; ++i) {
            Enemy& e = *near[i];
            w.damageEnemy(e, dmg);
            w.applyElement(e, el, b.owner, dmg, p, ev);
            if (static_cast<int>(streaks.size()) < cfg::mage::maxStreaks)
                streaks.push_back({b.pos, e.pos, cfg::mage::missileLife});
        }
        return true;
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

float World::mageCastRate(const Ball& b) const { return 1.f + b.mods.cls.mage.focus; }   // "Focus"

void World::mageOnCast(Ball& b, int slot, bool echo, const WorldParams& p, FrameEvents& ev) {
    using H = ClassHooks<BallRole::Mage>;
    const MageMods& g = b.mods.cls.mage;
    if (g.missileFrac > 0.f) H::missileVolley(*this, b, p, ev);   // "Arcane missile": one per cast
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
    using H = ClassHooks<BallRole::Mage>;
    const MageMods& g = b.mods.cls.mage;
    MageState& s = b.cls.mage;
    if (g.missileFrac > 0.f) {   // "Arcane missile": the timed volley
        s.missileT -= dt;
        if (s.missileT <= 0.f) s.missileT = H::missileVolley(*this, b, p, ev) ? g.missileEvery : 0.2f;
    }
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
// Fires bullets.
template <> struct ClassHooks<BallRole::Shooter> : NoClassHooks {
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
