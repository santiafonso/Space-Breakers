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
        return b.isAscended(BallRole::Summoner) ? S::ascendedPower
               : b.hasRole(BallRole::Summoner)  ? S::rolePower
                                                : 1.f;
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
                e.burnDps = std::max(e.burnDps, E::burnDps * pot);
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
