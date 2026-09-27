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
// Teleports to the nearest enemy on a kill.
template <> struct ClassHooks<BallRole::Assassin> : NoClassHooks {
};

// ==================================================================== Summoner
// Summons things: short-lived balls, turrets, a small dragon...
template <> struct ClassHooks<BallRole::Summoner> : NoClassHooks {
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

}  // namespace sb
