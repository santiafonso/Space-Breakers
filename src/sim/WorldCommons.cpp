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
    if (c.bully > 0.f && e.radius > b.radius) k *= 1.f + c.bully;   // "Bully"
    if (s.caromT > 0.f) { k *= 1.f + c.carom; s.caromT = 0.f; }   // "Carom"
    if (c.highRoller > 0.f) k *= cfg::common::highRollerPlain;     // "High roller": every hit a little less...
    if (c.luckySeven > 0 && ++s.hits7 >= c.luckySeven) {           // "Lucky seven": a sure double
        s.hits7 = 0;
        s.forcedDouble = true;
        k *= 2.f;
    }
    if (s.predArmed) { k *= 1.f + c.predator; s.predArmed = false; }   // "Predator"
    if (s.perfect) k *= c.perfect;                                 // "Perfect release": until a wall
    if (s.empower > 0.f) {                                         // "Empower" (ability): spent
        k *= s.empower;
        s.empower = 0.f;
        s.empowerHit = true;
    }
    if (s.frenzyT > 0.f) k *= cfg::common::slotFrenzyDamage;       // "777": a frenzy
    if (s.luckyN > 5)                                               // "Lucky number": a high roll hits harder...
        k *= 1.f + cfg::common::luckyDamagePer * static_cast<float>(s.luckyN - 5);
    if (s.luckyN == 10) k *= cfg::common::luckyTenCrit;             // ...a 10 crits every hit
    if (s.luckyArmed) { k *= cfg::common::luckyOneHit; s.luckyArmed = false; }   // ...a 1's comeback hit
    return k;
}

void World::commonOnHit(Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev) {
    if (b.mods.cls.common.plague > 0.f && !kill && e.hp > 0.f && e.plagueT <= 0.f && !e.orbiter &&   // "Black plague"
        chance(b.mods.cls.common.plague, p))
        infect(e, ballDamage(b, p) * b.mods.cls.common.plagueDps);
    namespace C = cfg::common;
    const CommonMods& c = b.mods.cls.common;
    CommonState& s = b.cls.common;
    if (!kill && e.hp > 0.f && e.sentenceT > 0.f && e.hp < e.maxHp * e.sentence) {   // a "Death sentence" carried out
        damageEnemy(e, e.hp);
        for (Ball& o : balls_)
            if (o.owner == e.sentenceOwner && !o.ghost) {   // ...and the sentencing ball blinks on
                o.cls.assassin.pending = true;
                o.cls.assassin.pendingAt = classWorld_.assassin.clock;
                break;
            }
        itemFx(CommonWorld::Fx::Slash, e.pos, e.radius * 2.5f, theme::classAssassin, 0.4f);   // the sentence: one clean cut
        kill = true;
    }
    if (c.contract > 0.f && !kill && e.hp > 0.f && e.id == classWorld_.common.contractId) {   // "Contract"
        damageEnemy(e, e.maxHp * c.contract);
        kill = e.hp <= 0.f;
    }
    if (!b.held) commonGunship(b, e, p);   // someone's "Gunship": a bullet off this hit
    if (s.empowerHit) {   // "Empower" (ability): the hit sends out a wave
        s.empowerHit = false;
        areaDamage(e.pos, s.empowerR, dmg * cfg::ability::empowerWave, &e);
        ev.bursts.push_back({e.pos, s.empowerR, b.color, nullptr});
        CommonWorld::Fx f;   // a second, sharper ring and a star of spikes
        f.kind = CommonWorld::Fx::Shock;
        f.a = e.pos;
        f.r = s.empowerR;
        f.life = f.maxLife = 0.4f;
        f.color = b.color;
        addFx(f);
        ev.kick = std::max(ev.kick, 4.f);
    }
    if (c.sentence > 0.f && !kill && e.hp > 0.f) {   // "Death sentence": passed
        e.sentenceT = C::sentenceTime;
        e.sentence = std::max(e.sentenceT > 0.f ? e.sentence : 0.f, c.sentence);
        e.sentenceOwner = b.owner;
    }
    if (s.homeArmed) { s.homeArmed = false; s.homeT = C::homeTime; }   // "Homecoming": now curve home
    if (c.lightspeed > 0.f && length(b.vel) > ballCruise(b, p) * C::lightspeedAt) {   // "Lightspeed": chain
        std::vector<const Enemy*> hit{&e};
        sf::Vector2f from = e.pos;
        for (int n = 0; n < c.lightChains; ++n) {
            Enemy* t = nullptr;
            float best = C::lightRange * arenaScale();
            best *= best;
            for (Enemy& o : enemies_) {
                if (o.hp <= 0.f || std::find(hit.begin(), hit.end(), &o) != hit.end()) continue;
                const float d2 = dot(o.pos - from, o.pos - from);
                if (d2 < best) { best = d2; t = &o; }
            }
            if (!t) break;
            damageEnemy(*t, dmg * c.lightspeed);
            itemFx(CommonWorld::Fx::Streak, from, b.radius, b.color, 0.25f, t->pos);   // a straight white-hot streak, not a jagged bolt
            hit.push_back(t);
            from = t->pos;
        }
    }
    if (c.wildCd > 0.f && s.doubled && s.wildT <= 0.f) {   // "Wild magic": a random ability from the run
        itemFx(CommonWorld::Fx::Wild, b.pos, b.radius * 2.5f, theme::classJester, 0.5f);
        std::vector<AbilitySpec> pool;
        for (const Ball& o : balls_)
            for (int i = 0; i < kMaxAbilitySlots; ++i)
                if (o.abilities[i].id != Ability::None) pool.push_back(o.abilities[i]);
        if (!pool.empty()) {
            const AbilitySpec a = pool[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(pool.size()) - 1))];
            castBall_ = &b;
            if (fireAbility(b, a, p, ev)) s.wildT = c.wildCd;
            castBall_ = nullptr;
        }
    }
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
        if (any) itemFx(CommonWorld::Fx::Clover, b.pos, b.radius * 2.2f, theme::classJester, 0.5f);
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
    if (s.luckyN > 0 && s.luckyN < 5) {   // "Lucky number": a low roll is heavy - it shoves and stuns
        const float lo = static_cast<float>(5 - s.luckyN);
        e.vel += normalized(e.pos - b.pos, {1.f, 0.f}) * cfg::combat::knockback * C::luckyKnockPer * lo * e.knockTaken;
        e.stagger = std::max(e.stagger, C::luckyStunPer * lo);
    }
    if (c.shrapnel > 0) commonRing(b, p, e.pos, c.shrapnel);   // "Shrapnel"
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
    const float grown = b.mods.radiusMult * (1.f + b.cls.common.titan);   // "Titan" grows it too
    if (c.juggernaut > 0.f && grown > 1.f) k *= 1.f + c.juggernaut * (grown - 1.f);   // "Juggernaut"
    k *= 1.f + C::titanDamage * b.cls.common.titan;                                  // "Titan"
    k *= 1.f + c.showtime * static_cast<float>(b.cls.common.showStacks);              // "Showtime"
    for (const Ball& o : balls_)   // other balls' "Anthem"
        if (o.owner != b.owner && o.mods.cls.common.anthem > 0.f) k *= 1.f + o.mods.cls.common.anthem;
    return k;
}

// ---------------------------------------------------------------- over time

void World::commonTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    CommonState& s = b.cls.common;
    s.spellT = std::max(0.f, s.spellT - dt);
    s.lastHitT += dt;
    s.medicCd = std::max(0.f, s.medicCd - dt);
    s.landingCd = std::max(0.f, s.landingCd - dt);
    s.chainT = std::max(0.f, s.chainT - dt);
    s.caromT = std::max(0.f, s.caromT - dt);
    s.transT = std::max(0.f, s.transT - dt);
    s.wildT = std::max(0.f, s.wildT - dt);
    s.frenzyT = std::max(0.f, s.frenzyT - dt);   // "777"
    s.rainT = std::max(0.f, s.rainT - dt);
    s.luckyStopT = std::max(0.f, s.luckyStopT - dt);   // "Lucky number"
    b.cls.slinger.beckonT = std::max(0.f, b.cls.slinger.beckonT - dt);   // "Beckon"
    b.regrabT = std::max(0.f, b.regrabT - dt);                            // a throw's no-grab moment
    if ((s.spinIdle += dt) > cfg::common::minigunIdle) s.spin = 0.f;   // "Minigun": spun down
    const CommonMods& c = b.mods.cls.common;
    const bool live = waveRunning_ && !b.held && !b.ghost;
    if (c.maelstrom > 0.f && live && (s.maelT -= dt) <= 0.f) {   // "Maelstrom": a ring of all its elements
        s.maelT = c.maelstrom;
        const float R = c.maelstromR * arenaScale();
        const float hit = ballDamage(b, p) * 0.5f;
        const float pot = elemPotency(b, p);
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - b.pos) > R + e.radius) continue;
            for (int i = 0; i < b.elemN; ++i) {
                touchElement(e, b.elems[i], pot, b.mods, p);
                applyElement(e, b.elems[i], i == 0 ? b.owner : -1, hit, p, ev);   // the rest react with it
            }
        }
        if (b.elemN > 0)
            itemFx(CommonWorld::Fx::Petals, b.pos, R, elementColor(b.elems[0]), 0.5f, {0.f, 0.f}, elementColor(b.elems[b.elemN - 1]));
    }
    if (c.slotEvery > 0.f && live && s.spinT < 0.f && (s.slotT -= dt) <= 0.f) {   // "777": pull the lever
        namespace C = cfg::common;
        s.slotT = c.slotEvery;
        s.spinT = 0.f;
        // The result is decided now (luck helps the triple); the reels just take their time showing it.
        const int roll = rng_.irange(0, 99);
        const int base = b.isAscended(BallRole::Jester) ? C::slotTripleGrand : b.hasRole(BallRole::Jester) ? C::slotTripleJester : C::slotTriplePct;
        const float tripleQ = static_cast<float>(base) * p.luck * (classWorld_.common.luckT > 0.f ? 1.5f : 1.f);
        auto pickTriple = [&] {
            int w = rng_.irange(0, C::slotFrenzyW + C::slotLuckW + C::slotRainW - 1);
            return w < C::slotFrenzyW ? 0 : (w -= C::slotFrenzyW) < C::slotLuckW ? 1 : 2;
        };
        if (static_cast<float>(roll) < tripleQ) {
            const int sym = pickTriple();
            s.reel[0] = s.reel[1] = s.reel[2] = sym;
        } else if (roll < static_cast<int>(tripleQ) + C::slotPairPct) {
            const int sym = rng_.irange(0, 2), other = (sym + 1 + rng_.irange(0, 1)) % 3, odd = rng_.irange(0, 2);
            for (int i = 0; i < 3; ++i) s.reel[i] = i == odd ? other : sym;
        } else {
            s.reel[0] = 0; s.reel[1] = 1; s.reel[2] = 2;
            for (int i = 2; i > 0; --i) std::swap(s.reel[i], s.reel[rng_.irange(0, i)]);
        }
    }
    if (s.spinT >= 0.f && (s.spinT += dt) >= cfg::common::slotSpin) {   // the last reel stops: pay out
        namespace C = cfg::common;
        s.spinT = -1.f;
        const bool triple = s.reel[0] == s.reel[1] && s.reel[1] == s.reel[2];
        const int pair = triple ? -1 : s.reel[0] == s.reel[1] || s.reel[0] == s.reel[2] ? s.reel[0] : s.reel[1] == s.reel[2] ? s.reel[1] : -1;
        const int sym = triple ? s.reel[0] : pair;
        const float len = triple ? c.slotBuff : c.slotBuff * C::slotPairBuff;
        static const char* kWin[3] = {"FRENZY", "LUCKY", "COIN RAIN"};
        if (sym == 0) s.frenzyT = std::max(s.frenzyT, len);
        else if (sym == 1) classWorld_.common.luckT = std::max(classWorld_.common.luckT, len);
        else if (sym == 2) s.rainT = std::max(s.rainT, len * C::slotRainLong);
        if (triple) {
            ev.bursts.push_back({b.pos, 0.f, sym == 2 ? theme::puGolden : theme::classJester, kWin[sym]});
            itemFx(sym == 2 ? CommonWorld::Fx::Coins : CommonWorld::Fx::Confetti, b.pos, 120.f * arenaScale(), theme::puGolden, 0.9f);
            ev.kick = std::max(ev.kick, 5.f);
        } else if (sym >= 0) {
            itemFx(CommonWorld::Fx::Reels, b.pos, b.radius, theme::classJester, 0.6f);
        }
    }
    if (s.frenzyT > 0.f && !b.held) {   // "777" frenzy: it lurches about
        if (rng_.range(0.f, 1.f) < dt * 4.f) {
            const float sp = length(b.vel), a = std::atan2(b.vel.y, b.vel.x) + rng_.range(-1.f, 1.f) * cfg::common::slotFrenzyJolt;
            if (sp > 1.f) b.vel = sf::Vector2f{std::cos(a), std::sin(a)} * sp;
        }
    }
    if (s.rainT > 0.f && (s.rainDropT -= dt) <= 0.f) {   // "777" coins: they fall from the top, anywhere
        s.rainDropT = cfg::common::slotRainEvery;
        auto& coins = classWorld_.common.coins;
        if (coins.size() < 140) {
            CommonWorld::Coin co;
            co.pos = {rng_.range(20.f, size_.x - 20.f), -10.f};
            co.vel = {rng_.range(-30.f, 30.f), rng_.range(380.f, 520.f) * arenaScale()};
            co.dmg = ballDamage(b, p) * cfg::common::slotRainFrac;
            co.spin = rng_.range(0.f, 6.f);
            co.owner = b.owner;
            coins.push_back(co);
        }
    }
    if (c.toxic > 0.f && live && b.elemN > 0 && (s.toxicT -= dt) <= 0.f) {   // "Toxic trail": a puff behind it
        s.toxicT = cfg::common::toxicEvery;
        auto& tr = classWorld_.common.trail;
        if (static_cast<int>(tr.size()) >= cfg::common::toxicMax) tr.erase(tr.begin());
        CommonWorld::Cloud cl;
        cl.pos = b.pos;
        cl.life = cl.maxLife = c.toxicLife;
        cl.r = cfg::common::toxicRadius * arenaScale();
        cl.dmg = ballDamage(b, p) * c.toxic;
        cl.elem = static_cast<int>(b.elems[s.toxicTurn++ % b.elemN]);
        cl.tickT = cfg::common::toxicTick;
        tr.push_back(cl);
    }
    if (c.primal > 0.f && live && b.elemN > 0) primalTick(b, dt, p, ev);   // "Primal"
    if (c.luckyEvery > 0.f && live && (s.luckyT -= dt) <= 0.f) {   // "Lucky number": roll the d10
        s.luckyT = c.luckyEvery;
        s.luckyN = rng_.irange(1, 10);
        if (s.luckyN == 1) {   // stopped dead - and the next hit x3
            s.luckyStopT = cfg::common::luckyStop;
            s.luckyArmed = true;
            b.vel *= 0.05f;
        }
    }
    if (s.homeT > 0.f) {   // "Homecoming": curve back toward your pointer
        s.homeT -= dt;
        if (hasPointer_ && !b.held && length(b.vel) > 1.f) {
            const float cur = std::atan2(b.vel.y, b.vel.x);
            const sf::Vector2f to = pointer_ - b.pos;
            const float want = std::atan2(to.y, to.x);
            const float turn = clampf(std::remainder(want - cur, 2.f * kPi), -cfg::common::homeTurn * dt,
                                      cfg::common::homeTurn * dt);
            b.vel = sf::Vector2f{std::cos(cur + turn), std::sin(cur + turn)} * length(b.vel);
        }
    }
    if (s.homePulse) {   // ...caught on the way home: a shockwave
        s.homePulse = false;
        const float R = cfg::common::homeRadius * arenaScale();
        areaDamage(b.pos, R, ballDamage(b, p) * c.homecoming, nullptr);
        itemFx(CommonWorld::Fx::Boomerang, b.pos, R, theme::classSlinger, 0.45f);
    }
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
    ev.midasGold += midasPending_;   // gold the commons earned off the events path (Slot machine)
    midasPending_ = 0;
    if (cw.elixirT > 0.f && (cw.elixirT -= dt) <= 0.f) cw.elixir = 0.f;   // "Elixir" wears off
    if (cw.chronoT > 0.f && (cw.chronoT -= dt) <= 0.f) cw.chrono = 0.f;   // "Chronomancy" wears off
    cw.stopT = std::max(0.f, cw.stopT - dt);                               // "Time stop"
    abilityWorldTick(dt, p, ev);   // mines, vortices, clouds, portals (sim/WorldAbilities.cpp)
    cw.rigged = false;                                                     // "Rigged" (World::chance)
    bool contract = false;
    for (const Ball& b : balls_) {
        cw.rigged = cw.rigged || b.mods.cls.common.rigged;
        contract = contract || b.mods.cls.common.contract > 0.f;
    }
    // "Contract": the boss, else the strongest enemy (a Brute first) - a new
    // one when the last falls.
    cw.contractBoss = contract && boss_.alive && boss_.intro <= 0.f;
    if (!contract || cw.contractBoss) {
        cw.contractId = -1;
    } else {
        bool alive = false;
        for (const Enemy& e : enemies_) alive = alive || (e.id == cw.contractId && e.hp > 0.f);
        if (!alive) {
            cw.contractId = -1;
            float best = -1.f;
            for (const Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || e.pos.x < 0.f || e.pos.y < 0.f || e.pos.x > size_.x || e.pos.y > size_.y)
                    continue;
                const float score = e.maxHp * (e.kind == EnemyKind::Brute ? 100.f : 1.f);
                if (score > best) { best = score; cw.contractId = e.id; }
            }
        }
    }
    cw.wardCd = std::max(0.f, cw.wardCd - dt);
    cw.wardOn = false;
    for (const Ball& b : balls_) cw.wardOn = cw.wardOn || b.mods.cls.common.wardEvery > 0.f;
    for (auto& wr : cw.wires) {   // "Razor wire": what crosses it gets cut
        wr.life -= dt;
        if ((wr.tickT -= dt) > 0.f) continue;
        wr.tickT = cfg::common::razorTick;
        const sf::Vector2f ab = wr.b - wr.a;
        const float L2 = std::max(1.f, dot(ab, ab));
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f) continue;
            const float t = clampf(dot(e.pos - wr.a, ab) / L2, 0.f, 1.f);
            if (length(e.pos - (wr.a + ab * t)) <= cfg::common::razorWidth * arenaScale() + e.radius) damageEnemy(e, wr.dmg);
        }
    }
    cw.wires.erase(std::remove_if(cw.wires.begin(), cw.wires.end(), [](const CommonWorld::Wire& w) { return w.life <= 0.f; }),
                   cw.wires.end());
    if (!cw.lastLineUsed && waveRunning_ && core_.hp > 0.f && core_.hp < core_.maxHp * cfg::common::lastLineAt) {
        const Ball* best = nullptr;   // "Last line": the strongest one carrying it
        for (const Ball& b : balls_)
            if (!b.ghost && b.mods.cls.common.lastLine > 0.f && (!best || b.mods.cls.common.lastLine > best->mods.cls.common.lastLine))
                best = &b;
        if (best) {
            cw.lastLineUsed = true;
            const float dmg = ballDamage(*best, p) * best->mods.cls.common.lastLine;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                damageEnemy(e, dmg);
                e.vel += normalized(e.pos - core_.pos, {1.f, 0.f}) * cfg::common::lastLineKnock * arenaScale() * e.knockTaken;
                e.stagger = std::max(e.stagger, best->mods.cls.common.lastLineStun);
            }
            ev.bursts.push_back({core_.pos, 0.f, theme::core, "LAST LINE"});
            itemFx(CommonWorld::Fx::Bastion, core_.pos, std::max(size_.x, size_.y) * 0.5f, theme::core, 0.7f);
        }
    }
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
            itemFx(CommonWorld::Fx::Glyph, o.pos, R, theme::classMage, 0.45f);
            g.life = 0.f;
        }
    }
    cw.glyphs.erase(std::remove_if(cw.glyphs.begin(), cw.glyphs.end(),
                                   [](const CommonWorld::Glyph& g) { return g.life <= 0.f; }),
                    cw.glyphs.end());
}

// Per enemy, every step (frozen or not): "Bleed" ticks, the marks wear off.
void World::commonEnemyTick(Enemy& e, float dt) {
    if (e.plagueT > 0.f) {   // "Black plague": it sickens while it runs mad
        e.plagueT -= dt;
        e.hp -= e.plagueDps * dt;
    }
    if (e.bleed > 0.f) {
        e.bleed -= dt;
        e.hp -= e.bleedDps * dt;
        if (e.bleed <= 0.f) e.bleedDps = 0.f;
    }
    e.spotT = std::max(0.f, e.spotT - dt);
    e.doomT = std::max(0.f, e.doomT - dt);
    e.shrinkPop = std::max(0.f, e.shrinkPop - dt * 4.f);
    if (e.shrinkT > 0.f && (e.shrinkT -= dt) <= 0.f) e.radius = e.shrinkR;   // "Shrink" wears off
}

// Per walking enemy, per step: "Brace" slows, "Decoy" pulls.
void World::commonEnemyDrag(Enemy& e, float& edt, float dt) {
    if (classWorld_.common.chronoT > 0.f) edt *= 1.f - classWorld_.common.chrono;   // "Chronomancy"
    if (classWorld_.common.stopT > 0.f) edt = 0.f;                                  // "Time stop"
    if (e.shrinkT > 0.f) edt *= cfg::ability::shrinkSlow;                           // "Shrink" (ability)
    if (classWorld_.common.rainT > 0.f) edt *= cfg::common::rainSlow;               // "Primal" water: rain
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
    if (b.cls.common.pingLeft > 0) {   // "Ping-pong": off the wall, straight at the nearest enemy
        sf::Vector2f aim;
        bool has = false;
        if (const auto f = focusPos()) { aim = *f; has = true; }
        else if (const Enemy* t = nearestEnemy(b.pos, 1e9f)) { aim = t->pos; has = true; }
        else if (boss_.alive && boss_.intro <= 0.f) { aim = boss_.pos; has = true; }
        if (has) {
            --b.cls.common.pingLeft;
            const sf::Vector2f d = normalized(aim - b.pos, normalized(b.vel, {1.f, 0.f}));
            b.vel = d * length(b.vel);
            itemFx(CommonWorld::Fx::Ping, b.pos, b.radius, b.color, 0.35f, d);
        }
    }
    b.cls.common.perfect = false;                        // "Perfect release": the wall ends it
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
    b.cls.common.pingLeft = 0;       // "Ping-pong": home
    b.cls.common.perfect = false;
    if (c.titan > 0.f) {   // "Titan": it grows on every landing; full grown, the next one quakes
        CommonState& s = b.cls.common;
        if (s.titan >= 1.f) {
            s.titan = 0.f;
            const float dmg = ballDamage(b, p) * c.titanQuake;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                damageEnemy(e, dmg);
                e.stagger = std::max(e.stagger, c.titanStun);
            }
            ev.bursts.push_back({core_.pos, 0.f, theme::core, "QUAKE"});
            itemFx(CommonWorld::Fx::Cracks, core_.pos, std::max(size_.x, size_.y) * 0.45f, theme::core, 0.8f);
            ev.kick = std::max(ev.kick, 9.f);
        } else {
            s.titan = std::min(1.f, s.titan + c.titan);
        }
    }
    if (c.heavyLanding <= 0.f || b.cls.common.landingCd > 0.f) return;
    b.cls.common.landingCd = cfg::common::landingCd;
    const float R = cfg::common::landingRadius * arenaScale();
    areaDamage(b.pos, R, ballDamage(b, p) * c.heavyLanding, nullptr);
    itemFx(CommonWorld::Fx::Dust, b.pos, R, theme::core, 0.5f);
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

void World::commonOnCast(Ball& b, const WorldParams& p, FrameEvents& ev) {
    const CommonMods& c = b.mods.cls.common;
    if (c.archmage > 0.f) ++b.cls.common.archStacks;   // "Archmage": the next ones hit harder
    if (c.timeStop > 0.f && ++b.cls.common.stopCasts % cfg::common::timeStopEvery == 0) {   // "Time stop"
        CommonWorld& cw = classWorld_.common;
        cw.stopT = std::max(cw.stopT, c.timeStop);
        for (Enemy& e : enemies_)
            if (e.hp > 0.f) e.stagger = std::max(e.stagger, c.timeStop);   // stopped counts as staggered (Opportunist)
        ev.bursts.push_back({b.pos, 0.f, theme::classMage, "TIME STOP"});
        itemFx(CommonWorld::Fx::Clock, b.pos, 70.f * arenaScale(), theme::classMage, std::max(0.6f, c.timeStop));
    }
    if (c.chrono > 0.f) {   // "Chronomancy": everything slows a moment
        itemFx(CommonWorld::Fx::Chrono, b.pos, std::max(size_.x, size_.y) * 0.6f, theme::classMage, 0.9f);
        CommonWorld& cw = classWorld_.common;
        cw.chronoT = cfg::common::chronoTime;
        cw.chrono = std::max(cw.chrono, c.chrono);
    }
    if (c.chorus > 0 && ++b.cls.common.casts % c.chorus == 0) {   // "Spell chorus": every ball casts
        for (Ball& o : balls_) {
            if (o.owner == b.owner || o.abilities[0].id == Ability::None) continue;
            castBall_ = &o;
            if (fireAbility(o, o.abilities[0], p, ev)) o.abilityFlash = 1.f;
            castBall_ = nullptr;
        }
        itemFx(CommonWorld::Fx::Echo, b.pos, b.radius * 3.f, theme::classMage, 0.6f);
    }
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

void World::commonOnReaction(Ball* rb, Element x, Element y, sf::Vector2f at, float hitDmg, const WorldParams& p,
                             FrameEvents& ev) {
    if (!rb) return;
    const CommonMods& c = rb->mods.cls.common;
    if (c.saturate > 0.f) {   // "Saturate": both elements' full effects on what the blast caught
        const float R = c.saturate * arenaScale();
        itemFx(CommonWorld::Fx::Saturate, at, R, elementColor(x), 0.5f, {0.f, 0.f}, elementColor(y));
        const float pot = elemPotency(*rb, p);
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - at) > R + e.radius) continue;
            touchElement(e, x, pot, rb->mods, p);
            touchElement(e, y, pot, rb->mods, p);
        }
    }
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

void World::commonOnBlink(Ball& b, Enemy& t, const WorldParams& p, sf::Vector2f from) {
    const CommonMods& c = b.mods.cls.common;
    if (c.razor > 0.f) {   // "Razor wire": the path stays sharp a while
        auto& ws = classWorld_.common.wires;
        if (static_cast<int>(ws.size()) >= cfg::common::maxWires) ws.erase(ws.begin());
        CommonWorld::Wire w;
        w.a = from;
        w.b = b.pos;
        w.life = w.maxLife = c.razorLife;
        w.dmg = ballDamage(b, p) * c.razor;
        ws.push_back(w);
    }
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
    if (c.showtime > 0.f)                                         // "Showtime": quick = one more, slow = half
        b.cls.common.showStacks = quick ? b.cls.common.showStacks + 1 : b.cls.common.showStacks / 2;
    if (b.cls.common.homeT > 0.f) {                               // "Homecoming": caught on the way home
        b.cls.common.homeT = 0.f;
        b.cls.common.homePulse = true;
    }
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
    b.cls.common.pingLeft = c.pingPong;   // "Ping-pong": fresh walls for this throw
    b.cls.common.skipping = c.skipping > 0.f;   // "Skipping stone": until it's home
    b.cls.common.perfect = c.perfect > 0.f && pullFrac >= cfg::common::perfectAt;   // "Perfect release"
    if (b.cls.common.perfect) b.squash = 1.f;
    b.cls.common.homeArmed = c.homecoming > 0.f;   // "Homecoming": after the first hit
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
    classWorld_.common.wires.clear();
    classWorld_.common.wardCd = 0.f;
    classWorld_.common.lastLineUsed = false;
    classWorld_.common.chronoT = classWorld_.common.chrono = 0.f;
    classWorld_.common.stopT = 0.f;
    classWorld_.common.mines.clear();   // the second batch of abilities
    classWorld_.common.vortices.clear();
    classWorld_.common.clouds.clear();
    classWorld_.common.portals.clear();
    classWorld_.common.fx.clear();
    classWorld_.common.dragons.clear();   // they come back with the fight (commonDragons)
    classWorld_.common.bugs.clear();
    classWorld_.common.hiveT.clear();
    classWorld_.common.tailT = 0.f;
    classWorld_.common.luckT = 0.f;   // "777"
    classWorld_.common.trail.clear();   // "Toxic trail"
    classWorld_.common.bottles.clear(); // "Primal"
    classWorld_.common.fields.clear();
    classWorld_.common.boulders.clear();
    classWorld_.common.rainT = 0.f;
    classWorld_.common.coins.clear();
    classWorld_.common.tailMul = 1.f;
    classWorld_.common.contractId = -1;
    classWorld_.common.elixirT = classWorld_.common.elixir = 0.f;
    for (Ball& b : balls_) {
        b.cls.common.firstBlood = b.mods.cls.common.firstBlood > 0.f;   // "First blood": armed
        b.cls.common.adrenaline = 0;                                    // "Adrenaline": a fresh fight
        b.cls.common.streak = 0;
        b.cls.common.fortune = 0;   // "Fortune": a fresh fight
        b.cls.common.flickers = 0;
        b.cls.common.spin = 0.f;
        b.cls.common.slotT = b.mods.cls.common.slotEvery * 0.5f;   // the first spin comes early
        b.cls.common.spinT = -1.f;
        b.cls.common.frenzyT = b.cls.common.rainT = 0.f;
        b.cls.common.luckyT = 0.f;   // "Lucky number": a roll right away
        b.cls.common.luckyN = 0;
        b.cls.common.luckyStopT = 0.f;
        b.cls.common.luckyArmed = false;
        b.cls.common.primalT = b.mods.cls.common.primalEvery * 0.5f;   // "Primal": the first comes early
        b.cls.common.flameT = 0.f;
        b.cls.common.strikesLeft = 0;
        b.cls.common.quakesLeft = 0;
        b.cls.common.titan = 0.f;
        b.cls.common.archStacks = b.cls.common.stopCasts = b.cls.common.showStacks = 0;
        b.cls.common.stormOn = 0.f;
        b.cls.common.empower = b.cls.common.beamT = b.cls.common.bladesT = b.cls.common.leashT = 0.f;
        b.cls.common.empowerHit = false;
        b.cls.common.stormT = b.mods.cls.common.leadStorm * 0.5f;   // the first storm comes early
    }
}

// "Ward ring": an enemy reaching the core is thrown back instead (true), once
// every few seconds.
bool World::commonWard(Enemy& e, FrameEvents&) {
    CommonWorld& cw = classWorld_.common;
    if (cw.wardCd > 0.f) return false;
    float every = 0.f;
    for (const Ball& b : balls_)
        if (b.mods.cls.common.wardEvery > 0.f) every = every > 0.f ? std::min(every, b.mods.cls.common.wardEvery)
                                                                   : b.mods.cls.common.wardEvery;
    if (every <= 0.f) return false;
    cw.wardCd = every;
    const sf::Vector2f out = normalized(e.pos - core_.pos, {1.f, 0.f});
    e.pos = core_.pos + out * (core_.radius + e.radius + 6.f);
    e.vel = out * cfg::common::wardKnock * arenaScale() * e.knockTaken;
    e.stagger = std::max(e.stagger, 0.6f);
    itemFx(CommonWorld::Fx::Hexshield, core_.pos, core_.radius + 14.f, theme::core, 0.5f, e.pos);
    return true;
}

float World::commonHouseEdge() const {   // "House edge" (App doubles the gold)
    float c = 0.f;
    for (const Ball& b : balls_)
        if (!b.ghost) c += b.mods.cls.common.houseEdge;
    return std::min(c, cfg::common::houseEdgeCap);
}

// An enemy just died (any cause): "Fission" spreads the element it carried.
void World::commonOnDeath(const Enemy& dead, FrameEvents& ev) {
    if (dead.bounty > 0) {   // "Bounty" (ability): collected
        ev.midasGold += dead.bounty;
        ev.bounties.push_back({dead.pos, dead.bounty});
        ev.bursts.push_back({dead.pos, dead.radius * 3.f, theme::puGolden, nullptr});
    }
    if (dead.elemT <= 0.f || dead.elem == Element::Plain || dead.elemOwner < 0) return;
    for (const Ball& b : balls_) {
        if (b.owner != dead.elemOwner || b.mods.cls.common.fission <= 0.f) continue;
        const float R = b.mods.cls.common.fission * arenaScale();
        for (Enemy& o : enemies_) {
            if (&o == &dead || o.hp <= 0.f || length(o.pos - dead.pos) > R + o.radius) continue;
            o.elem = dead.elem;
            o.elemOwner = dead.elemOwner;
            o.elemT = cfg::synergy::reactWindow;
            itemFx(CommonWorld::Fx::Link, dead.pos, 0.f, elementColor(dead.elem), 0.35f, o.pos);   // the element jumps over
        }
        return;
    }
}

float World::commonPlating() const {
    float hp = 0.f;
    for (const Ball& b : balls_)
        if (!b.ghost) hp += b.mods.cls.common.plating;
    return hp;
}

// "Black plague": an enemy catches it - black, mad, sickening.
void World::infect(Enemy& e, float dps) {
    if (e.plagueT > 0.f || e.hp <= 0.f) return;
    int n = 0;
    for (const Enemy& o : enemies_) n += o.plagueT > 0.f ? 1 : 0;
    if (n >= cfg::common::plagueMax) return;
    e.plagueT = cfg::common::plagueTime;
    e.plagueDps = dps;
}

// "Primal" (Alchemist legendary): every few seconds its first element lets
// loose in its own way; fire and electric play out over a moment.
void World::primalTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace C = cfg::common;
    CommonState& s = b.cls.common;
    const CommonMods& c = b.mods.cls.common;
    const float k = arenaScale();
    const float hit = ballDamage(b, p) * c.primal;
    auto nearestTo = [&](sf::Vector2f from) -> Enemy* {
        Enemy* t = nullptr;
        float bd = 1e18f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const float d2 = dot(e.pos - from, e.pos - from);
            if (d2 < bd) { bd = d2; t = &e; }
        }
        return t;
    };
    if (s.flameT > 0.f) {   // fire: the flamethrower keeps burning at the nearest enemy
        s.flameT -= dt;
        if (Enemy* t = nearestTo(b.pos)) s.flameDir = normalized(t->pos - b.pos, s.flameDir);
        if ((s.flameTickT -= dt) <= 0.f) {
            s.flameTickT = C::flameTick;
            const float R = C::flameRange * k, cosC = std::cos(C::flameCone);
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                const sf::Vector2f to = e.pos - b.pos;
                const float l = length(to);
                if (l > R + e.radius || (l > 1.f && dot(to / l, s.flameDir) < cosC)) continue;
                damageEnemy(e, hit * C::flameFrac);
                applyElement(e, Element::Fire, b.owner, hit * C::flameFrac, p, ev);
            }
        }
    }
    if (s.strikesLeft > 0 && (s.strikeT -= dt) <= 0.f) {   // electric: bolts keep falling
        s.strikeT = C::strikeEvery;
        --s.strikesLeft;
        std::vector<Enemy*> live;
        for (Enemy& e : enemies_)
            if (e.hp > 0.f && !e.orbiter && e.pos.x > 0.f && e.pos.y > 0.f && e.pos.x < size_.x && e.pos.y < size_.y) live.push_back(&e);
        if (!live.empty()) {
            Enemy& e = *live[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(live.size()) - 1))];
            damageEnemy(e, hit * C::strikeFrac);
            applyElement(e, Element::Electric, b.owner, hit * C::strikeFrac, p, ev);
            itemFx(CommonWorld::Fx::Storm, e.pos, e.radius, elementColor(Element::Electric), 0.4f);
            const Enemy* from = &e;   // ...and it forks on to its neighbours
            std::vector<const Enemy*> hitList{&e};
            for (int n = 0; n < C::strikeChain; ++n) {
                Enemy* nx = nullptr;
                float bd = C::strikeChainRange * k;
                bd *= bd;
                for (Enemy& o : enemies_) {
                    if (o.hp <= 0.f || std::find(hitList.begin(), hitList.end(), &o) != hitList.end()) continue;
                    const float d2 = dot(o.pos - from->pos, o.pos - from->pos);
                    if (d2 < bd) { bd = d2; nx = &o; }
                }
                if (!nx) break;
                damageEnemy(*nx, hit * C::strikeFrac * 0.5f);
                applyElement(*nx, Element::Electric, b.owner, hit * C::strikeFrac * 0.5f, p, ev);
                itemFx(CommonWorld::Fx::Streak, from->pos, 4.f, elementColor(Element::Electric), 0.2f, nx->pos);
                hitList.push_back(nx);
                from = nx;
            }
        }
    }
    if (s.quakesLeft > 0 && (s.quakeT -= dt) <= 0.f) {   // stone: the ground keeps heaving
        s.quakeT = C::quakeEvery;
        --s.quakesLeft;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            damageEnemy(e, hit * C::quakeFrac);
            applyElement(e, Element::Stone, b.owner, hit * C::quakeFrac, p, ev);
            e.stagger = std::max(e.stagger, C::quakeStun);
            itemFx(CommonWorld::Fx::Rubble, e.pos, e.radius, elementColor(Element::Stone), 0.5f);
        }
        ev.kick = std::max(ev.kick, 8.f);
    }
    if ((s.primalT -= dt) > 0.f) return;
    if (!nearestTo(b.pos)) { s.primalT = 0.f; return; }   // nothing to hit: ready the moment there is
    s.primalT = c.primalEvery;
    for (int ei = 0; ei < b.elemN; ++ei) {   // every element it carries lets loose at once (an Alchemist: both)
        const Element el = b.elems[ei];
        const sf::Color ec = elementColor(el);
        switch (el) {
            case Element::Fire:
                s.flameT = C::flameTime;
                s.flameTickT = 0.f;
                break;
            case Element::Ice: {   // a frost cloud settles where it is: it bites and freezes a while
                CommonWorld::Field f;
                f.pos = b.pos;
                f.r = C::frostRadius * k;
                f.life = f.maxLife = C::frostTime;
                f.tick = C::frostTick;
                f.dmg = hit * C::frostFrac;
                f.elem = static_cast<int>(Element::Ice);
                f.owner = b.owner;
                classWorld_.common.fields.push_back(f);
                itemFx(CommonWorld::Fx::Frost, b.pos, f.r, ec, C::frostTime);
                break;
            }
            case Element::Water:   // rain over the arena: the first drops soak everything, then it keeps pelting
                classWorld_.common.rainT = std::max(classWorld_.common.rainT, C::rainTime);
                classWorld_.common.rainDmg = hit * C::rainDropFrac;
                classWorld_.common.rainOwner = b.owner;
                for (Enemy& e : enemies_) {
                    if (e.hp <= 0.f || e.orbiter) continue;
                    damageEnemy(e, hit * C::rainFrac);
                    applyElement(e, Element::Water, b.owner, hit * C::rainFrac, p, ev);
                }
                break;
            case Element::Stone: {   // an earthquake - shock after shock - and boulders out of the sky
                s.quakesLeft = C::quakeWaves;
                s.quakeT = 0.f;
                std::vector<const Enemy*> live;
                for (const Enemy& e : enemies_)
                    if (e.hp > 0.f && !e.orbiter) live.push_back(&e);
                for (int i = 0; i < C::boulders && !live.empty(); ++i) {
                    CommonWorld::Boulder bo;
                    bo.at = live[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(live.size()) - 1))]->pos;
                    bo.t = -0.15f * static_cast<float>(i);   // one after another
                    bo.dmg = hit * C::boulderFrac;
                    bo.owner = b.owner;
                    classWorld_.common.boulders.push_back(bo);
                }
                break;
            }
            case Element::Poison: {   // bottles lobbed at the nearest few
                std::vector<Enemy*> pick;
                for (Enemy& e : enemies_)
                    if (e.hp > 0.f && !e.orbiter) pick.push_back(&e);
                std::sort(pick.begin(), pick.end(), [&](const Enemy* x, const Enemy* y) {
                    return dot(x->pos - b.pos, x->pos - b.pos) < dot(y->pos - b.pos, y->pos - b.pos);
                });
                for (int i = 0; i < C::bottles && i < static_cast<int>(pick.size()); ++i) {
                    CommonWorld::Bottle bt;
                    bt.from = b.pos;
                    bt.to = pick[static_cast<std::size_t>(i)]->pos;
                    bt.dmg = hit * C::bottleFrac;
                    bt.owner = b.owner;
                    classWorld_.common.bottles.push_back(bt);
                }
                break;
            }
            case Element::Electric:
                s.strikesLeft = C::strikes;
                s.strikeT = 0.f;
                break;
            case Element::Plain:
                break;
        }
    }
}

}  // namespace sb
