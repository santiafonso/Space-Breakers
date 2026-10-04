// The commons (2026-10-04): one small item more per class, each one line, each
// built to combine with other classes too. They work on any ball that carries
// them - no class needed - so they hang off the World's own hooks (strike,
// onKill, the bounces, the clack, a cast, a reaction, grab and throw) rather
// than the class dispatch. Numbers: cfg::common; folded in core/ClassSpec.cpp.

#include <algorithm>
#include <cmath>

#include "core/Config.hpp"
#include "sim/World.hpp"

namespace sb {

// ---------------------------------------------------------------- a hit

float World::commonPreHit(Ball& b, Enemy& e, const WorldParams&) {
    const CommonMods& c = b.mods.cls.common;
    CommonState& s = b.cls.common;
    float k = 1.f;
    if (c.reagent > 0.f && e.elemT > 0.f && e.elem != Element::Plain && e.elemOwner != b.owner)   // "Reagent"
        k *= 1.f + c.reagent;
    if (c.opportunist > 0.f && (e.frozen > 0.f || e.stagger > 0.f)) k *= 1.f + c.opportunist;   // "Opportunist"
    if (e.spotT > 0.f && e.spotOwner != b.owner) k *= e.spotMul;   // another ball's "Spotter"
    if (e.doomT > 0.f) k *= e.doomMul;                             // "Mark for death"
    if (c.spellblade > 0.f && s.spellT > 0.f) {                    // "Spellblade": spent
        k *= 1.f + c.spellblade;
        s.spellT = 0.f;
    }
    if (c.hotStreak > 0.f) k *= 1.f + c.hotStreak * static_cast<float>(s.streak);   // "Hot streak"
    if (c.firstBlood > 0.f && s.firstBlood) {                      // "First blood": spent
        k *= c.firstBlood;
        s.firstBlood = false;
    }
    if (c.comboHit > 0.f && s.lastHitId >= 0 && s.lastHitId != e.id && s.lastHitT < cfg::common::comboWindow)
        k *= 1.f + c.comboHit;                                     // "Combo hit"
    if (s.pull > 0.f) { k *= 1.f + s.pull; s.pull = 0.f; }         // "Long pull": the throw's first hit
    if (s.handOff > 0.f) { k *= 1.f + s.handOff; s.handOff = 0.f; }   // another ball's "Hand-off"
    if (s.venom) k *= c.venomBlade;                                 // "Venom blade" (its element: commonOnHit)
    if (s.bankArmed) {                                              // "Bank throw": spent
        k *= 1.f + c.bankThrow * static_cast<float>(s.banks);
        s.bankArmed = false;
        s.banks = 0;
    }
    if (c.bully > 0.f && e.radius > b.radius) k *= 1.f + c.bully;   // "Bully"
    if (s.caromT > 0.f) { k *= 1.f + c.carom; s.caromT = 0.f; }   // "Carom"
    if (c.highRoller > 0.f) k *= cfg::common::highRollerPlain;     // "High roller": every hit a little less...
    if (c.luckySeven > 0 && ++s.hits7 >= c.luckySeven) {           // "Lucky seven": a sure double
        s.hits7 = 0;
        s.forcedDouble = true;
        k *= 2.f;
    }
    if (s.predArmed) { k *= 1.f + c.predator; s.predArmed = false; }   // "Predator"
    return k;
}

void World::commonOnHit(Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev) {
    namespace C = cfg::common;
    const CommonMods& c = b.mods.cls.common;
    CommonState& s = b.cls.common;
    s.lastHitId = e.id;   // "Combo hit"
    s.lastHitT = 0.f;
    if (c.hotStreak > 0.f) s.streak = s.doubled ? std::min(s.streak + 1, C::hotStreakMax) : 0;   // "Hot streak"
    if (c.confetti > 0 && s.doubled) {   // "Confetti": sparks at the nearest few
        std::vector<const Enemy*> hit{&e};
        for (int n = 0; n < c.confetti; ++n) {
            Enemy* t = nullptr;
            float best = C::confettiRange * arenaScale();
            best *= best;
            for (Enemy& o : enemies_) {
                if (o.hp <= 0.f || std::find(hit.begin(), hit.end(), &o) != hit.end()) continue;
                const float d2 = dot(o.pos - e.pos, o.pos - e.pos);
                if (d2 < best) { best = d2; t = &o; }
            }
            if (!t) break;
            damageEnemy(*t, dmg * c.confettiFrac);
            hit.push_back(t);
            if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                bolts_.push_back(Bolt{e.pos, t->pos, cfg::element::boltLife, cfg::element::boltLife});
        }
    }
    if (s.doubled) {
        if (c.highRoller > 0.f && !kill && e.hp > 0.f) damageEnemy(e, dmg * c.highRoller);   // ...a double far more
        if (c.fortune > 0.f) s.fortune = std::min(s.fortune + 1, C::fortuneMax);         // "Fortune"
    }
    s.doubled = false;
    if (c.pickpocket > 0.f && !b.ghost && chance(c.pickpocket, p)) ++ev.midasGold;   // "Pickpocket"
    if (c.luckyBreak > 0.f && chance(c.luckyBreak, p)) {   // "Lucky break": every ability ready
        bool any = false;
        for (int i = 0; i < kMaxAbilitySlots; ++i)
            if (b.abilities[i].id != Ability::None) { b.abilityCd[i] = 0.f; any = true; }
        if (any) ev.bursts.push_back({b.pos, b.radius * 2.2f, theme::classJester, nullptr});
    }
    const bool venom = s.venom;
    s.venom = false;
    if (kill || e.hp <= 0.f) return;
    if (venom && b.element != Element::Plain)   // "Venom blade": its element's touch once more
        touchElement(e, b.element, elemPotency(b, p), b.mods, p);
    if (c.mirror > 0.f && chance(c.mirror, p)) {   // "Mirror": the nearest other ball copies the hit
        Ball* o = nullptr;
        float bd = 1e18f;
        for (Ball& x : balls_) {
            if (x.owner == b.owner || x.held) continue;
            const float d2 = dot(x.pos - e.pos, x.pos - e.pos);
            if (d2 < bd) { bd = d2; o = &x; }
        }
        if (o) {
            const float d = ballDamage(*o, p);
            damageEnemy(e, d);
            applyElement(e, hitElement(*o, p), o->owner, d, p, ev);
            if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                bolts_.push_back(Bolt{o->pos, e.pos, cfg::element::boltLife, cfg::element::boltLife});
        }
    }
    if (c.shieldBash > 0.f &&   // "Shield bash": too close to the core - off you go
        length(e.pos - core_.pos) < C::bashRange * arenaScale() + core_.radius) {
        e.vel += normalized(e.pos - core_.pos, {1.f, 0.f}) * c.shieldBash * arenaScale() * e.knockTaken;
        e.stagger = std::max(e.stagger, c.bashStun);
    }
    if (c.bleed > 0.f) {   // "Bleed": the hit again, spread over a couple of seconds
        e.bleed = C::bleedTime;
        e.bleedDps = std::max(e.bleedDps, dmg * c.bleed / C::bleedTime);
    }
    if (c.brace > 0.f) {   // "Brace"
        e.braceT = C::braceTime;
        e.braceSlow = std::max(e.braceSlow, c.brace);
    }
    if (c.spotter > 0.f) {   // "Spotter": the others hit it harder
        e.spotT = C::spotTime;
        e.spotMul = 1.f + c.spotter;
        e.spotOwner = b.owner;
    }
}

void World::commonOnKill(Ball& b, Enemy& e, const WorldParams& p) {
    namespace C = cfg::common;
    const CommonMods& c = b.mods.cls.common;
    CommonState& s = b.cls.common;
    if (c.insight > 0.f)   // "Insight"
        for (int i = 0; i < kMaxAbilitySlots; ++i)
            if (b.abilities[i].id != Ability::None) b.abilityCd[i] = std::max(0.f, b.abilityCd[i] - c.insight);
    if (c.adrenaline > 0.f && s.adrenaline < C::adrenalineMax) ++s.adrenaline;   // "Adrenaline" (ballCruise reads it)
    if (c.chainKill > 0.f && s.chainT > 0.f) {   // "Chain kill": the blink is ready again
        b.cls.assassin.cd = 0.f;
        s.chainT = 0.f;
    }
    if (c.handOff > 0.f) {   // "Hand-off": the nearest other ball's next hit
        Ball* best = nullptr;
        float bd = 1e18f;
        for (Ball& o : balls_) {
            if (o.owner == b.owner) continue;
            const float d2 = dot(o.pos - e.pos, o.pos - e.pos);
            if (d2 < bd) { bd = d2; best = &o; }
        }
        if (best) best->cls.common.handOff = std::max(best->cls.common.handOff, c.handOff);
    }
    (void)p;
}

// Multipliers that sit on the ball's damage itself (ballDamage).
float World::commonDamageMul(const Ball& b) const {
    namespace C = cfg::common;
    const CommonMods& c = b.mods.cls.common;
    float k = 1.f;
    if (c.packLeader > 0.f) {   // "Pack leader": its summons alive
        const SummonerWorld& sw = classWorld_.summoner;
        int n = 0;
        for (const SummonTurret& t : sw.turrets) n += t.owner == b.owner ? 1 : 0;
        for (const SummonTotem& t : sw.totems) n += t.owner == b.owner ? 1 : 0;
        for (const SummonShot& t : sw.shots) n += t.wisp && t.owner == b.owner ? 1 : 0;
        n += b.mods.cls.summoner.wardens;
        for (const CommonWorld::Golem& g : classWorld_.common.golems) n += g.owner == b.owner ? 1 : 0;
        k *= 1.f + c.packLeader * static_cast<float>(std::min(n, C::packLeaderMax));
    }
    if (c.sentinel > 0.f) {   // "Sentinel": close to the core
        const float d = std::max(0.f, length(b.pos - core_.pos) - core_.radius - b.radius);
        k *= 1.f + c.sentinel * clampf(1.f - d / (C::sentinelRange * arenaScale()), 0.f, 1.f);
    }
    if (classWorld_.common.elixirT > 0.f) k *= 1.f + classWorld_.common.elixir;   // someone's "Elixir"
    for (const Ball& o : balls_)   // other balls' "Anthem"
        if (o.owner != b.owner && o.mods.cls.common.anthem > 0.f) k *= 1.f + o.mods.cls.common.anthem;
    return k;
}

// ---------------------------------------------------------------- over time

void World::commonTick(Ball& b, float dt, const WorldParams& p, FrameEvents&) {
    CommonState& s = b.cls.common;
    s.spellT = std::max(0.f, s.spellT - dt);
    s.lastHitT += dt;
    s.medicCd = std::max(0.f, s.medicCd - dt);
    s.landingCd = std::max(0.f, s.landingCd - dt);
    s.chainT = std::max(0.f, s.chainT - dt);
    s.caromT = std::max(0.f, s.caromT - dt);
    s.transT = std::max(0.f, s.transT - dt);
    if (s.coverPending) {
        s.coverPending = false;
        if (!b.held && waveRunning_) commonVolley(b, p, b.mods.cls.common.cover, core_.pos);
    }
}

void World::commonWorldTick(float dt, const WorldParams& p, FrameEvents& ev) {
    auto& ps = classWorld_.common.puddles;
    for (auto& pd : ps) {   // "Seep": what walks through takes the element
        pd.life -= dt;
        if ((pd.tickT -= dt) > 0.f) continue;
        pd.tickT = cfg::common::seepTick;
        const Element el = static_cast<Element>(pd.elem);
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - pd.pos) > pd.r + e.radius) continue;
            if (pd.prime) {   // "Residue": only primes - the next hit there reacts
                if (e.elemT <= 0.f) { e.elem = el; e.elemOwner = -1; e.elemT = cfg::synergy::reactWindow; }
                continue;
            }
            touchElement(e, el, 1.f, BallMods{}, p);
            applyElement(e, el, pd.owner, pd.dmg, p, ev);
        }
    }
    ps.erase(std::remove_if(ps.begin(), ps.end(), [](const CommonWorld::Puddle& pd) { return pd.life <= 0.f; }),
             ps.end());

    CommonWorld& cw = classWorld_.common;
    if (cw.elixirT > 0.f && (cw.elixirT -= dt) <= 0.f) cw.elixir = 0.f;   // "Elixir" wears off
    for (auto& g : cw.glyphs) {   // "Sigil": another ball crossing it sets it off
        g.life -= dt;
        for (const Ball& o : balls_) {
            if (g.life <= 0.f || o.owner == g.owner || length(o.pos - g.pos) > g.r + o.radius) continue;
            const float R = cfg::common::sigilNova * arenaScale();
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || length(e.pos - o.pos) > R + e.radius) continue;
                damageEnemy(e, g.dmg);
                applyElement(e, static_cast<Element>(g.elem), g.owner, g.dmg, p, ev);
            }
            ev.bursts.push_back({o.pos, R, theme::classMage, nullptr});
            g.life = 0.f;
        }
    }
    cw.glyphs.erase(std::remove_if(cw.glyphs.begin(), cw.glyphs.end(),
                                   [](const CommonWorld::Glyph& g) { return g.life <= 0.f; }),
                    cw.glyphs.end());
}

// Per enemy, every step (frozen or not): "Bleed" ticks, the marks wear off.
void World::commonEnemyTick(Enemy& e, float dt) {
    if (e.bleed > 0.f) {
        e.bleed -= dt;
        e.hp -= e.bleedDps * dt;
        if (e.bleed <= 0.f) e.bleedDps = 0.f;
    }
    e.spotT = std::max(0.f, e.spotT - dt);
    e.doomT = std::max(0.f, e.doomT - dt);
}

// Per walking enemy, per step: "Brace" slows, "Decoy" pulls.
void World::commonEnemyDrag(Enemy& e, float& edt, float dt) {
    if (e.braceT > 0.f) {
        e.braceT -= dt;
        edt *= 1.f - e.braceSlow;
        if (e.braceT <= 0.f) e.braceSlow = 0.f;
    }
    const SummonerWorld& sw = classWorld_.summoner;
    if (sw.turrets.empty() && sw.totems.empty()) return;
    const float R = cfg::common::decoyRadius * arenaScale();
    auto pull = [&](sf::Vector2f at, int owner) {
        const sf::Vector2f to = at - e.pos;
        const float d = length(to);
        if (d > R + e.radius || d < 1e-3f) return;
        for (const Ball& b : balls_)
            if (b.owner == owner && b.mods.cls.common.decoy > 0.f) {
                e.pos += to / d * b.mods.cls.common.decoy * arenaScale() * dt;
                return;
            }
    };
    for (const SummonTurret& t : sw.turrets) pull(t.pos, t.owner);
    for (const SummonTotem& t : sw.totems) pull(t.pos, t.owner);
}

// ---------------------------------------------------------------- events

void World::commonWallBounce(Ball& b, const WorldParams& p) {
    const CommonMods& c = b.mods.cls.common;
    if (c.arcaneCharge > 0.f)   // "Arcane charge"
        for (int i = 0; i < kMaxAbilitySlots; ++i)
            if (b.abilities[i].id != Ability::None) b.abilityCd[i] = std::max(0.f, b.abilityCd[i] - c.arcaneCharge);
    if (b.cls.common.bankArmed) ++b.cls.common.banks;   // "Bank throw"
    if (c.ammoFeed > 0 && waveRunning_) commonVolley(b, p, c.ammoFeed, b.pos);   // "Ammo feed"
    // "Seep": a puddle of its element
    if (c.seepLife <= 0.f || b.element == Element::Plain) return;
    auto& ps = classWorld_.common.puddles;
    if (static_cast<int>(ps.size()) >= cfg::common::maxPuddles) ps.erase(ps.begin());
    CommonWorld::Puddle pd;
    pd.pos = b.pos;
    pd.r = c.seepRadius * arenaScale();
    pd.life = pd.maxLife = c.seepLife;
    pd.dmg = ballDamage(b, p) * cfg::common::seepFrac;
    pd.elem = static_cast<int>(b.element);
    pd.owner = b.owner;
    ps.push_back(pd);
}

void World::commonCoreBounce(Ball& b, const WorldParams& p, FrameEvents& ev) {   // "Heavy landing"
    const CommonMods& c = b.mods.cls.common;
    b.cls.common.skipping = false;   // "Skipping stone": home
    if (c.heavyLanding <= 0.f || b.cls.common.landingCd > 0.f) return;
    b.cls.common.landingCd = cfg::common::landingCd;
    const float R = cfg::common::landingRadius * arenaScale();
    areaDamage(b.pos, R, ballDamage(b, p) * c.heavyLanding, nullptr);
    ev.bursts.push_back({b.pos, R, theme::core, nullptr});
}

// Two balls clacked: `a`'s items act on `o`.
void World::commonClack(Ball& a, Ball& o) {
    const CommonMods& c = a.mods.cls.common;
    if (c.carom > 0.f) a.cls.common.caromT = cfg::common::caromWindow;   // "Carom"
    if (c.tincture > 0.f && a.element != Element::Plain) {   // "Tincture": like a Pass charge, element only
        SupportState& os = o.cls.support;
        os.beaconT = cfg::style::beaconTime;
        os.beaconMul = std::max(os.beaconT > 0.f ? os.beaconMul : 1.f, c.tincture);
        os.beaconElem = static_cast<int>(a.element);
        os.beaconOwner = a.owner;
    }
    if (c.medic > 0.f && !a.ghost && a.cls.common.medicCd <= 0.f) {   // "Medic"
        a.cls.common.medicCd = cfg::common::medicCd;
        core_.hp = std::min(core_.maxHp, core_.hp + c.medic);
    }
}

void World::commonOnCast(Ball& b, const WorldParams& p) {
    const CommonMods& c = b.mods.cls.common;
    if (c.spellblade > 0.f) b.cls.common.spellT = cfg::common::spellbladeWindow;   // "Spellblade": armed
    if (c.aetherStep > 0.f) boostSpeed(b, c.aetherStep, p);                      // "Aether step"
    if (c.sigil > 0.f) {   // "Sigil": a glyph for the others to cross
        auto& gs = classWorld_.common.glyphs;
        if (static_cast<int>(gs.size()) >= cfg::common::maxGlyphs) gs.erase(gs.begin());
        gs.push_back({b.pos, cfg::common::sigilRadius * arenaScale(), cfg::common::sigilLife,
                      ballDamage(b, p) * c.sigil, static_cast<int>(hitElement(b, p)), b.owner});
    }
    if (c.conduit > 0.f) {   // "Conduit": prime the pack with the nearest other ball's element
        const Ball* o = nullptr;
        float bd = 1e18f;
        for (const Ball& x : balls_) {
            if (x.owner == b.owner || x.element == Element::Plain || x.element == b.element) continue;
            const float d2 = dot(x.pos - b.pos, x.pos - b.pos);
            if (d2 < bd) { bd = d2; o = &x; }
        }
        if (o) {
            const float R = c.conduit * arenaScale();
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || length(e.pos - b.pos) > R + e.radius) continue;
                e.elem = o->element;
                e.elemOwner = o->owner;
                e.elemT = cfg::synergy::reactWindow;
            }
        }
    }
    if (c.sync > 0.f)   // "Sync": the other balls' abilities come sooner
        for (Ball& o : balls_) {
            if (o.owner == b.owner) continue;
            for (int i = 0; i < kMaxAbilitySlots; ++i)
                if (o.abilities[i].id != Ability::None) o.abilityCd[i] = std::max(0.f, o.abilityCd[i] - c.sync);
        }
}

void World::commonOnReaction(Ball* rb, Element x, Element y, sf::Vector2f at, float hitDmg, FrameEvents& ev) {
    if (!rb) return;
    const CommonMods& c = rb->mods.cls.common;
    const Element other = x == rb->element ? y : x;   // the element it met
    if (c.transmute > 0.f && other != Element::Plain) {   // "Transmute": it carries that one a while
        rb->cls.common.transT = c.transmute;
        rb->cls.common.transElem = static_cast<int>(other);
    }
    if (c.residue > 0.f && other != Element::Plain) {   // "Residue": a puddle that primes (no owner: reacts with anything)
        auto& ps = classWorld_.common.puddles;
        if (static_cast<int>(ps.size()) >= cfg::common::maxPuddles) ps.erase(ps.begin());
        CommonWorld::Puddle pd;
        pd.pos = at;
        pd.r = (cfg::common::residueRadius +
                cfg::common::residueRadiusPerLevel * (c.residue - cfg::common::residue) / cfg::common::residuePerLevel) *
               arenaScale();
        pd.life = pd.maxLife = c.residue;
        pd.dmg = hitDmg * cfg::common::seepFrac;
        pd.elem = static_cast<int>(other);
        pd.owner = -1;
        pd.prime = true;
        ps.push_back(pd);
    }
    if (c.aurum > 0)   // "Aurum": what this reaction killed pays
        for (Enemy& e : enemies_)
            if (e.hp <= 0.f && !e.aurumPaid) {
                e.aurumPaid = true;
                ev.midasGold += c.aurum;
            }
    if (rb->mods.cls.common.elixir > 0.f) {   // "Elixir": every ball, for a moment
        CommonWorld& cw = classWorld_.common;
        cw.elixirT = cfg::common::elixirTime;
        cw.elixir = std::max(cw.elixir, rb->mods.cls.common.elixir);
    }
    if (rb->mods.cls.common.distill <= 0.f) return;   // "Distill"
    if (++rb->cls.common.reactions % cfg::common::distillEvery == 0)
        core_.hp = std::min(core_.maxHp, core_.hp + rb->mods.cls.common.distill);
}

void World::commonOnBlink(Ball& b, Enemy& t, const WorldParams& p) {
    const CommonMods& c = b.mods.cls.common;
    if (c.doom > 0.f) {   // "Mark for death"
        t.doomT = c.doomTime;
        t.doomMul = 1.f + c.doom;
    }
    if (c.quickstep > 0.f) boostSpeed(b, c.quickstep, p);   // "Quickstep"
    if (c.chainKill > 0.f) b.cls.common.chainT = c.chainKill;   // "Chain kill": a kill now recharges it
    if (c.venomBlade > 0.f) b.cls.common.venom = true;          // "Venom blade"
    if (c.predator > 0.f) b.cls.common.predArmed = true;        // "Predator" (the reach and the pick: the blink)
    if (c.flicker > 0.f) {   // "Flicker": again, at once - a few times at most
        CommonState& s = b.cls.common;
        if (s.flickers < cfg::common::flickerMax && chance(c.flicker, p)) {
            ++s.flickers;
            b.cls.assassin.pending = true;
            b.cls.assassin.pendingAt = classWorld_.assassin.clock;
            b.cls.assassin.cd = 0.f;
        } else {
            s.flickers = 0;
        }
    }
}

void World::commonOnGrab(Ball& b) {
    const CommonMods& c = b.mods.cls.common;
    const bool quick = b.sinceThrow < cfg::common::relayWindow;   // caught soon after your throw
    if (c.secondWind > 0.f) qRefund_ += c.secondWind;            // "Second wind": App reads it
    if (c.relay <= 0.f || !quick) return;
    // "Relay": the nearest other ball flies at the enemy nearest it.
    Ball* o = nullptr;
    float bd = 1e18f;
    for (Ball& x : balls_) {
        if (&x == &b || x.held || x.mods.satellite) continue;
        const float d2 = dot(x.pos - b.pos, x.pos - b.pos);
        if (d2 < bd) { bd = d2; o = &x; }
    }
    if (!o) return;
    const Enemy* t = nullptr;
    float td = 1e18f;
    for (const Enemy& e : enemies_) {
        if (e.hp <= 0.f) continue;
        const float d2 = dot(e.pos - o->pos, e.pos - o->pos);
        if (d2 < td) { td = d2; t = &e; }
    }
    if (!t) return;
    const float sp = std::max(length(o->vel), cfg::ball::baseCruise * arenaScale() * c.relay);
    o->vel = normalized(t->pos - o->pos, {1.f, 0.f}) * std::min(sp, cfg::ball::hardSpeedCap * arenaScale());
}

void World::commonOnThrow(Ball& b, float pullFrac) {
    const CommonMods& c = b.mods.cls.common;
    if (c.longPull > 0.f) b.cls.common.pull = c.longPull * clampf(pullFrac, 0.f, 1.f);   // "Long pull"
    if (c.bankThrow > 0.f) { b.cls.common.bankArmed = true; b.cls.common.banks = 0; }   // "Bank throw"
    b.cls.common.skipping = c.skipping > 0.f;   // "Skipping stone": until it's home
    if (c.formation > 0.f && length(b.vel) > 1.f) {   // "Formation": the others follow
        const sf::Vector2f v = b.vel * c.formation;
        for (Ball& o : balls_)
            if (&o != &b && !o.held && !o.mods.satellite && o.snaredBy < 0) o.vel = v;
    }
    for (Ball& o : balls_)   // other balls' "Covering fire" (on their next tick)
        if (&o != &b && o.mods.cls.common.cover > 0) o.cls.common.coverPending = true;
    b.sinceThrow = 0.f;
}

void World::commonWaveStart() {
    classWorld_.common.puddles.clear();
    classWorld_.common.glyphs.clear();
    classWorld_.common.golems.clear();
    classWorld_.common.elixirT = classWorld_.common.elixir = 0.f;
    for (Ball& b : balls_) {
        b.cls.common.firstBlood = b.mods.cls.common.firstBlood > 0.f;   // "First blood": armed
        b.cls.common.adrenaline = 0;                                    // "Adrenaline": a fresh fight
        b.cls.common.streak = 0;
        b.cls.common.fortune = 0;   // "Fortune": a fresh fight
        b.cls.common.flickers = 0;
    }
}

float World::commonHouseEdge() const {   // "House edge" (App doubles the gold)
    float c = 0.f;
    for (const Ball& b : balls_)
        if (!b.ghost) c += b.mods.cls.common.houseEdge;
    return std::min(c, cfg::common::houseEdgeCap);
}

// An enemy just died (any cause): "Fission" spreads the element it carried.
void World::commonOnDeath(const Enemy& dead, FrameEvents& ev) {
    if (dead.elemT <= 0.f || dead.elem == Element::Plain || dead.elemOwner < 0) return;
    for (const Ball& b : balls_) {
        if (b.owner != dead.elemOwner || b.mods.cls.common.fission <= 0.f) continue;
        const float R = b.mods.cls.common.fission * arenaScale();
        for (Enemy& o : enemies_) {
            if (&o == &dead || o.hp <= 0.f || length(o.pos - dead.pos) > R + o.radius) continue;
            o.elem = dead.elem;
            o.elemOwner = dead.elemOwner;
            o.elemT = cfg::synergy::reactWindow;
        }
        ev.bursts.push_back({dead.pos, R, elementColor(dead.elem), nullptr});
        return;
    }
}

float World::commonPlating() const {
    float hp = 0.f;
    for (const Ball& b : balls_)
        if (!b.ghost) hp += b.mods.cls.common.plating;
    return hp;
}

}  // namespace sb
