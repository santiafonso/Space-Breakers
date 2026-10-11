// The gods (2026-10-06): Hermes (the Striker's) and Odin (the Mage's). Each
// is a class a ball ascends into at a Shrine; one of their items is enough
// for the class. Few items, each a big one. (Atlas lives in WorldPlanets.cpp.)
//
// Hermes - speed. The class: faster, a much higher top speed, its hits grow
// with its speed. Talaria (wall bounces speed it up), Caduceus (fast hits
// strike the 2 nearest too), Messenger (there and back to the farthest enemy),
// Petasos (fast: golden, through enemies, sonic booms), Quicksilver (fast:
// every enemy slower).
//
// Odin - abilities that show. The class: every 2 s it casts all of its
// abilities at once; 3 ability slots, abilities stronger and wider, faster
// recharge. Huginn (everything three times running), Muninn (an arcane blast
// on the thickest pack with every cast), Valknut (recharge, the all-cast
// sooner), Gungnir (a spear: 10x its hit through the line to the strongest),
// Draupnir (abilities a level up, stronger and wider).

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

namespace {

// Abilities that move the ball: a raven can't cast those from where it is.
bool movesBall(Ability a) {
    return a == Ability::Dash || a == Ability::Warp || a == Ability::Swap || a == Ability::Leash;
}

}  // namespace

bool World::hermesFast(const Ball& b, const WorldParams& p) const {
    return b.hasRole(BallRole::Hermes) && !b.held &&
           length(b.vel) > ballCruise(b, p) * cfg::hermes::fastAt;
}

bool World::hermesGolden(const Ball& b, const WorldParams& p) const {
    return b.mods.cls.hermes.petasos > 0.f && hermesFast(b, p);
}

// An ability cast again from somewhere else (a raven) at a share of its power,
// outside its cooldown. The ball stays where it is.
bool World::recastAbility(Ball& b, Ability id, int level, float power, sf::Vector2f from, const WorldParams& p,
                          FrameEvents& ev) {
    if (id == Ability::None || (movesBall(id) && length(from - b.pos) > 1.f)) return false;
    const AbilitySpec a{id, std::max(1, level)};
    const sf::Vector2f pos = b.pos, vel = b.vel;
    const float pw = b.mods.cls.mage.power;
    b.mods.cls.mage.power *= power;
    b.pos = from;
    castBall_ = &b;
    godRecast_ = true;
    const bool ok = fireAbility(b, a, p, ev);
    godRecast_ = false;
    castBall_ = nullptr;
    b.mods.cls.mage.power = pw;
    if (!movesBall(id)) { b.pos = pos; b.vel = vel; }
    return ok;
}

// Per ball, per step.
void World::godsTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    namespace H = cfg::hermes;
    namespace O = cfg::odin;
    const float k = arenaScale();
    // ---- Hermes
    const HermesMods& hm = b.mods.cls.hermes;
    HermesWorld& hw = classWorld_.hermes;
    b.cls.hermes.fast = hermesFast(b, p);
    b.cls.hermes.golden = hermesGolden(b, p);
    if (hm.quicksilver > 0.f && hermesFast(b, p)) {   // time bends round it
        hw.slowT = 0.15f;
        hw.slow = std::max(hw.slow, std::min(0.7f, hm.quicksilver));
    }
    if (hm.messenger > 0.f && waveRunning_ && !b.held && !b.ghost && (b.cls.hermes.messengerT -= dt) <= 0.f) {
        // There and back: to the enemy farthest from the core, a hit, and back where it was.
        Enemy* far = nullptr;
        float fd = -1.f;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const float d = length(e.pos - core_.pos);
            if (d > fd) { fd = d; far = &e; }
        }
        if (far) {
            b.cls.hermes.messengerT = hm.messenger;
            const float dmg = ballDamage(b, p) * hm.messengerHit;
            dmgOwner_ = b.owner;
            damageEnemy(*far, dmg);
            applyElement(*far, hitElement(b, p), b.owner, dmg, p, ev);
            dmgOwner_ = -1;
            far->vel += normalized(far->pos - b.pos, {1.f, 0.f}) * (300.f * k * far->knockTaken);
            hw.fx.push_back({0, b.pos, far->pos, 0.45f, 0.45f, far->radius});
            ev.bursts.push_back({far->pos, far->radius * 2.4f, theme::classHermes, nullptr});
        } else {
            b.cls.hermes.messengerT = 0.5f;   // nobody out there yet: soon
        }
    }

    // The class's own flight: shy of the core, leaning toward the enemies; flying fast loads its message.
    if (b.hasRole(BallRole::Hermes) && !b.held && !b.ghost && waveRunning_ && length(b.vel) > 1.f) {
        const float sp = length(b.vel);
        float cur = std::atan2(b.vel.y, b.vel.x);
        const sf::Vector2f r = b.pos - core_.pos;
        const float d = length(r);
        const float near = core_.radius + H::avoidRange * k;
        bool shy = false;
        const bool deliver = b.cls.hermes.bounces >= H::deliverAfter;   // its message is due: straight for the core
        if (deliver) {
            const float want = std::atan2(-r.y, -r.x);
            cur += clampf(std::remainder(want - cur, 2.f * kPi), -6.f * dt, 6.f * dt);
            shy = true;
        } else if (d < near && dot(b.vel, r) < 0.f) {   // heading in: veer off along the side it's already passing
            const sf::Vector2f rh = r / std::max(1.f, d);
            const sf::Vector2f tn{-rh.y, rh.x};
            const sf::Vector2f side = dot(b.vel, tn) >= 0.f ? tn : -tn;
            const float want = std::atan2(side.y, side.x);
            const float strength = 1.f - (d - core_.radius) / std::max(1.f, near - core_.radius);
            cur += clampf(std::remainder(want - cur, 2.f * kPi), -H::avoidTurn * strength * dt, H::avoidTurn * strength * dt);
            // about to touch it anyway: swerve right off (it only reaches the core when its message is due)
            if (d < core_.radius + b.radius * 2.5f + sp * 0.05f && dot(sf::Vector2f{std::cos(cur), std::sin(cur)}, rh) < 0.f) cur = want;
            shy = true;
        }
        if (!shy) {   // lean toward the player's target, else the nearest enemy
            const Enemy* t = focusEnemy();
            if (!t) t = nearestEnemy(b.pos, H::seekRange * k);
            if (t) {
                const float want = std::atan2(t->pos.y - b.pos.y, t->pos.x - b.pos.x);
                cur += clampf(std::remainder(want - cur, 2.f * kPi), -H::seekTurn * dt, H::seekTurn * dt);
            }
        }
        b.vel = sf::Vector2f{std::cos(cur), std::sin(cur)} * sp;
        if (b.cls.hermes.fast) b.cls.hermes.charge = std::min(H::chargeMax, b.cls.hermes.charge + H::chargePerSecFast * dt);
    }

    // ---- Odin: every few s all of its abilities at once; Huginn three times running; Gungnir.
    const OdinMods& om = b.mods.cls.odin;
    OdinState& os = b.cls.odin;
    for (int i = 0; i < 2; ++i) {   // the ravens circle it (drawn), and flash when they act
        const float an = b.age * (1.6f + 0.5f * static_cast<float>(i)) + static_cast<float>(i) * kPi;
        os.raven[i] = b.pos + sf::Vector2f{std::cos(an), std::sin(an) * 0.7f} * (b.radius * (2.6f + 0.6f * static_cast<float>(i)));
        os.ravenFlash[i] = std::max(0.f, os.ravenFlash[i] - dt * 2.f);
    }
    os.circle = std::max(0.f, os.circle - dt * 1.8f);
    if (!waveRunning_ || b.held || b.ghost || !b.hasRole(BallRole::Odin)) return;
    // Odin casts only in volleys (its abilities don't fire one by one: updateAbilities):
    // everything at once, every allEvery s at its recharge rate (diminishing).
    if ((os.allT -= dt) <= 0.f) {
        os.allT = O::allEvery / mageCastRate(b);
        if (odinCastAll(b, p, ev)) {
            os.circle = 1.f;
            ++os.volleys;
            if (om.huginn > 0.f && os.volleys % O::huginnEvery == 0) {   // Huginn: this volley twice more
                os.burst = O::huginnCasts - 1;
                os.burstT = O::burstGap;
            }
        }
    }
    if (os.burst > 0 && (os.burstT -= dt) <= 0.f) {
        os.burstT = O::burstGap;
        --os.burst;
        if (odinCastAll(b, p, ev)) { os.ravenFlash[0] = 1.f; os.circle = 1.f; }
    }
    if (om.gungnir > 0.f && (os.gungnirT -= dt) <= 0.f) {   // Gungnir: at the strongest, through the line, and back
        // the marks, strongest first (the boss before all); one more spear per spell cast since the last throw
        std::vector<sf::Vector2f> marks;
        if (boss_.targetable()) marks.push_back(boss_.pos);
        std::vector<const Enemy*> by;
        for (const Enemy& e : enemies_)
            if (e.hp > 0.f && !e.orbiter) by.push_back(&e);
        std::sort(by.begin(), by.end(), [](const Enemy* x, const Enemy* y) { return x->maxHp > y->maxHp; });
        for (const Enemy* e : by) marks.push_back(e->pos);
        if (!marks.empty()) {
            os.gungnirT = om.gungnirEvery / mageCastRate(b);   // Gungnir scales with its cooldowns
            const int n = 1 + std::min(O::spearsMax, os.spells);
            os.spells = 0;
            for (int i = 0; i < n; ++i) {
                const sf::Vector2f to = marks[static_cast<std::size_t>(i) % marks.size()];
                const sf::Vector2f dir = normalized(to - b.pos, {1.f, 0.f});
                OdinWorld::Spear s;
                s.from = s.pos = b.pos;
                s.to = to + dir * (120.f * k);
                s.dmg = ballDamage(b, p) * om.gungnir * (i == 0 ? 1.f : O::spearExtra);
                s.owner = b.owner;
                s.delay = O::spearGap * static_cast<float>(i);
                classWorld_.odin.spears.push_back(std::move(s));
            }
        } else {
            os.gungnirT = 0.5f;
        }
    }
}

// Every hit of a god's ball (after its damage).
void World::godsOnHit(Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev) {
    namespace H = cfg::hermes;
    const HermesMods& hm = b.mods.cls.hermes;
    if (b.hasRole(BallRole::Hermes) && !b.ghost) ++b.cls.hermes.bounces;   // off an enemy: momentum too
    if (hm.caduceus > 0.f && hermesFast(b, p)) {   // twin serpents to the 2 nearest
        const float k = arenaScale();
        Enemy* near[2] = {nullptr, nullptr};
        float nd[2] = {1e18f, 1e18f};
        for (Enemy& o : enemies_) {
            if (&o == &e || o.hp <= 0.f || o.orbiter) continue;
            const float d = length(o.pos - e.pos);
            if (d > H::caduceusRange * k) continue;
            if (d < nd[0]) { nd[1] = nd[0]; near[1] = near[0]; nd[0] = d; near[0] = &o; }
            else if (d < nd[1]) { nd[1] = d; near[1] = &o; }
        }
        for (Enemy* o : near) {
            if (!o) continue;
            dmgOwner_ = b.owner;
            damageEnemy(*o, dmg * hm.caduceus);
            applyElement(*o, hitElement(b, p), b.owner, dmg * hm.caduceus, p, ev);
            dmgOwner_ = -1;
            classWorld_.hermes.fx.push_back({1, e.pos, o->pos, 0.35f, 0.35f, 0.f});
        }
    }
}

// A golden Petasos ball passing through an enemy: a sonic boom (a few a second at most).
void World::hermesBoom(Ball& b, sf::Vector2f at, const WorldParams& p, FrameEvents& ev) {
    namespace H = cfg::hermes;
    if (b.age - b.cls.hermes.lastBoom < H::boomEvery) return;
    b.cls.hermes.lastBoom = b.age;
    const float k = arenaScale();
    const float R = H::boomR * k * clampf(b.radius / (cfg::ball::radius * k), 0.8f, 2.f);
    const float dmg = ballDamage(b, p) * b.mods.cls.hermes.petasos;
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter || length(e.pos - at) > R + e.radius) continue;
        dmgOwner_ = b.owner;
        damageEnemy(e, dmg);
        dmgOwner_ = -1;
        e.stagger = std::max(e.stagger, 0.3f);
    }
    classWorld_.hermes.fx.push_back({2, at, at, 0.35f, 0.35f, R});
    (void)ev;
}

void World::godsWallBounce(Ball& b, const WorldParams& p) {
    if (const float t = b.mods.cls.hermes.talaria; t > 0.f) boostSpeed(b, 1.f + t, p);   // "Talaria"
    if (b.hasRole(BallRole::Hermes) && !b.ghost) {   // a wall: one more bounce of momentum, and more to tell
        HermesState& h = b.cls.hermes;
        ++h.bounces;
        h.charge = std::min(cfg::hermes::chargeMax, h.charge + cfg::hermes::chargePerWall);
    }
}

// Hermes reaches the core: the message is delivered - the whole arena takes a
// hit for what it carried - and its momentum starts over.
void World::godsCoreBounce(Ball& b, const WorldParams& p, FrameEvents& ev) {
    namespace H = cfg::hermes;
    if (b.mods.cls.hermes.talaria > 0.f) {   // "Talaria": the core takes the run of bounces back down
        const float sp = length(b.vel), cruise = ballCruise(b, p);
        if (sp > cruise * 1.5f) b.vel *= cruise * 1.5f / sp;
    }
    if (!b.hasRole(BallRole::Hermes) || b.ghost) return;
    HermesState& h = b.cls.hermes;
    if (h.bounces < H::deliverAfter) return;   // a brush with the core before it's due: it keeps its message
    if (waveRunning_ && h.charge >= H::deliverMin) {
        const float dmg = ballDamage(b, p) * H::deliverFrac * h.charge;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            dmgOwner_ = b.owner;
            damageEnemy(e, dmg);
            dmgOwner_ = -1;
            e.stagger = std::max(e.stagger, 0.5f);
        }
        if (boss_.targetable()) {
            boss_.hp -= dmg * 0.5f;
            boss_.hitFlash = 1.f;
        }
        classWorld_.hermes.fx.push_back({3, core_.pos, core_.pos, 0.9f, 0.9f, length(size_)});
        ev.hermesDelivered = h.charge;
        ev.hermesAt = core_.pos;
        ev.kick = std::max(ev.kick, 4.f + h.charge);
    }
    h.charge = 0.f;
    h.bounces = 0;
    h.peak = 0.f;
}

// A kill credited to a god's ball: each god's passive. Odin - every cooldown
// (and its next all-at-once cast) comes a share sooner. Atlas - it grows (and
// its planets with it) for the rest of the fight. (Hermes has his flight.)
void World::godsOnKill(int owner) {
    for (Ball& b : balls_) {
        if (b.owner != owner || b.ghost) continue;
        if (b.hasRole(BallRole::Odin)) {
            b.cls.odin.allT *= 1.f - cfg::odin::killRefund;   // the next volley a little sooner
        }
        if (b.hasRole(BallRole::Astronomer) && b.cls.astronomer.growth < cfg::planet::growMax) ++b.cls.astronomer.growth;
        if (b.hasRole(BallRole::Thanatos)) {   // a soul: harder hits, and on to the next at once
            b.cls.thanatos.souls = std::min(b.cls.thanatos.souls + 1, cfg::thanatos::soulMax);
            b.cls.thanatos.blinkCd *= 0.5f;   // a kill: the next blink sooner
        }
        if (b.hasRole(BallRole::Tiamat)) tiamatHatch(b.owner, b.pos);   // a lizard hatches
        if (b.hasRole(BallRole::Loki) && rng_.range(0.f, 1.f) < cfg::loki::killClone) lokiSpawn(b);   // a clone splits off
        if (b.hasRole(BallRole::Ares)) aresOnKill(b);   // bloodlust
        if (b.hasRole(BallRole::Apolo))   // the muses linger a little longer
            classWorld_.apolo.rotT = std::min(classWorld_.apolo.rotT + cfg::apolo::killHold, cfg::apolo::rotEvery * 1.5f);
        return;
    }
}

// Every ability Odin casts on its own cooldown: Muninn's blast follows it.
void World::godsOnCast(Ball& b, const AbilitySpec& a, const WorldParams& p, FrameEvents& ev) {
    (void)a;
    if (godRecast_ || !b.hasRole(BallRole::Odin)) return;
    ++b.cls.odin.spells;
    muninnBlast(b, p, ev);
}

// All of its abilities at once (the all-father's cast, Huginn's three). True if any went off.
bool World::odinCastAll(Ball& b, const WorldParams& p, FrameEvents& ev) {
    bool any = false;
    for (const AbilitySpec& a : b.abilities) {
        if (a.id == Ability::None) continue;
        if (recastAbility(b, a.id, a.level, 1.f, b.pos, p, ev)) {
            any = true;
            ++b.cls.odin.spells;
            muninnBlast(b, p, ev);
        }
    }
    if (any) b.abilityFlash = 1.f;
    return any;
}

// Muninn: an arcane blast on the thickest pack in reach.
void World::muninnBlast(Ball& b, const WorldParams& p, FrameEvents& ev) {
    namespace O = cfg::odin;
    const float frac = b.mods.cls.odin.muninn;
    if (frac <= 0.f) return;
    const float k = arenaScale();
    const float R = O::blastR * k * (1.f + (b.mods.cls.mage.power - 1.f) * 0.5f);
    const Enemy* best = nullptr;
    int most = 0;
    for (const Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter || length(e.pos - b.pos) > O::blastRange * k) continue;
        int c = 0;
        for (const Enemy& o : enemies_)
            if (o.hp > 0.f && length(o.pos - e.pos) < R) ++c;
        if (c > most) { most = c; best = &e; }
    }
    if (!best) return;
    const sf::Vector2f at = best->pos;
    const float dmg = ballDamage(b, p) * frac * b.mods.cls.mage.power;
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter || length(e.pos - at) > R + e.radius) continue;
        dmgOwner_ = b.owner;
        damageEnemy(e, dmg);
        applyElement(e, hitElement(b, p), b.owner, dmg, p, ev);
        dmgOwner_ = -1;
    }
    auto& bl = classWorld_.odin.blasts;
    if (bl.size() < 40) bl.push_back({at, b.cls.odin.raven[1], R, 0.5f, 0.5f});
    b.cls.odin.ravenFlash[1] = 1.f;
}

// Once a step: the gods' things in the field.
void World::updateGods(float dt, const WorldParams& p, FrameEvents& ev) {
    (void)p;
    (void)ev;
    namespace O = cfg::odin;
    const float k = arenaScale();
    HermesWorld& hw = classWorld_.hermes;
    OdinWorld& ow = classWorld_.odin;
    if (ow.wave != wave_) {   // a new fight: nothing left over
        ow.wave = wave_;
        ow.spears.clear();
        ow.blasts.clear();
        hw.fx.clear();
    }
    if (hw.slowT > 0.f && (hw.slowT -= dt) <= 0.f) hw.slow = 0.f;
    for (auto& f : hw.fx) f.life -= dt;
    hw.fx.erase(std::remove_if(hw.fx.begin(), hw.fx.end(), [](const HermesWorld::Streak& s) { return s.life <= 0.f; }), hw.fx.end());
    if (!waveRunning_) return;

    for (auto& bl : ow.blasts) bl.life -= dt;
    ow.blasts.erase(std::remove_if(ow.blasts.begin(), ow.blasts.end(), [](const OdinWorld::Blast& b) { return b.life <= 0.f; }),
                    ow.blasts.end());

    // Gungnir: out through the line, then home through it again.
    for (auto& s : ow.spears) {
        if (s.delay > 0.f) {   // still in its hand: it leaves from where the ball is now
            s.delay -= dt;
            for (const Ball& b : balls_)
                if (b.owner == s.owner && !b.ghost) {
                    s.to += b.pos - s.from;
                    s.from = s.pos = b.pos;
                }
            continue;
        }
        s.t += dt / O::spearTime;
        const sf::Vector2f prev = s.pos;
        if (!s.back) {
            s.pos = s.from + (s.to - s.from) * std::min(1.f, s.t);
            if (s.t >= 1.f) { s.back = true; s.t = 0.f; s.hit.clear(); }
        } else {
            const Ball* o = nullptr;
            for (const Ball& b : balls_)
                if (b.owner == s.owner) o = &b;
            const sf::Vector2f home = o ? o->pos : s.from;
            s.pos = s.to + (home - s.to) * std::min(1.f, s.t);
        }
        const sf::Vector2f ab = s.pos - prev;
        const float W = O::spearWidth * k;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || std::find(s.hit.begin(), s.hit.end(), e.id) != s.hit.end()) continue;
            const float tt = clampf(dot(e.pos - prev, ab) / std::max(1e-3f, dot(ab, ab)), 0.f, 1.f);
            if (length(e.pos - (prev + ab * tt)) > e.radius + W) continue;
            s.hit.push_back(e.id);
            dmgOwner_ = s.owner;
            damageEnemy(e, s.dmg);
            dmgOwner_ = -1;
            e.stagger = std::max(e.stagger, 0.4f);
        }
        if (boss_.targetable() && std::find(s.hit.begin(), s.hit.end(), -7) == s.hit.end()) {
            const float tt = clampf(dot(boss_.pos - prev, ab) / std::max(1e-3f, dot(ab, ab)), 0.f, 1.f);
            if (length(boss_.pos - (prev + ab * tt)) < boss_.radius + W) {
                s.hit.push_back(-7);   // (the boss, once each way)
                boss_.hp -= s.dmg * 0.5f;
                boss_.hitFlash = 1.f;
            }
        }
    }
    ow.spears.erase(std::remove_if(ow.spears.begin(), ow.spears.end(), [](const OdinWorld::Spear& s) { return s.back && s.t >= 1.f; }),
                    ow.spears.end());
}

}  // namespace sb
