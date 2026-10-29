// Pact hooks (2026-09-28). Everything a pact does inside the simulation lives
// here; each function is a no-op unless its pact is on (pact_, refreshed from
// WorldParams::pact every step and at each wave start), and World.cpp calls
// into them from one line each.

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

float World::pactDamageMul(const Ball& b, const WorldParams& p) const {
    namespace P = cfg::pact;
    const PactRules& r = p.pact;
    float k = 1.f;
    if (r.juggler) k *= 1.f + P::jugglePer * static_cast<float>(b.juggle);           // "Juggler"
    if (r.rubberWalls) k *= 1.f + P::rubberPer * static_cast<float>(b.rubber);       // "Rubber walls"
    if (r.starfallCost && !b.starfall) k *= P::starfallOthers;                      // "Starfall": the rest
    return k;
}

// "Void Walls": a ball whose centre leaves the arena comes back in through the
// opposite edge, heading the same way.
bool World::pactWrap(Ball& b) {
    if (!pact_.voidWalls) return false;
    bool moved = false;
    if (b.pos.x < 0.f) { b.pos.x += size_.x; moved = true; }
    else if (b.pos.x > size_.x) { b.pos.x -= size_.x; moved = true; }
    if (b.pos.y < 0.f) { b.pos.y += size_.y; moved = true; }
    else if (b.pos.y > size_.y) { b.pos.y -= size_.y; moved = true; }
    if (moved) {   // no streak across the arena
        b.trail.clear();
        b.waterTrail.clear();
    }
    return true;   // walls never bounce under this pact
}

// "Anchor Walls": a wall soaks up nearly all of a ball's speed. "Rubber
// walls": every bounce stacks on the ball until it's caught.
void World::pactWallBump(Ball& b) {
    if (pact_.anchorWalls) b.vel *= cfg::pact::anchorKeep;
    if (pact_.rubberWalls) b.rubber = std::min(b.rubber + 1, cfg::pact::rubberMax);
}

// "Starfall": a meteor's first hit lands harder and blasts everything round it.
void World::pactStrike(Ball& b, Enemy& e, float& dmg, FrameEvents& ev) {
    if (!b.starfall) return;
    namespace P = cfg::pact;
    b.starfall = false;
    dmg *= P::starfallMul;
    const float R = P::starfallRadius * arenaScale();
    areaDamage(e.pos, R, dmg * P::starfallBlast, &e);
    ev.bursts.push_back({e.pos, R, theme::ember, nullptr});
    ev.kick = std::max(ev.kick, 0.5f);
}

// "Billiards": the enemy a ball kills flies on along the ball's path and
// slams whatever is behind it.
void World::pactKill(const Ball& b, const Enemy& e, float dmg, FrameEvents& ev) {
    if (!pact_.billiards || b.ghost) return;
    namespace P = cfg::pact;
    const sf::Vector2f dir = normalized(e.pos - b.pos, normalized(b.vel, {1.f, 0.f}));   // knocked away from the ball
    const float reach = e.radius * P::billiardsReach;
    bool any = false;
    for (Enemy& o : enemies_) {
        if (&o == &e || o.hp <= 0.f || o.orbiter) continue;
        const float along = dot(o.pos - e.pos, dir);
        if (along <= 0.f || along > reach) continue;
        if (length(o.pos - (e.pos + dir * along)) > e.radius + o.radius) continue;
        o.lastBy = b.owner;
        damageEnemy(o, dmg * P::billiardsFrac);
        o.vel += dir * cfg::combat::knockback;
        ev.bursts.push_back({o.pos, o.radius * 1.8f, theme::textHi, nullptr});
        any = true;
    }
    if (any) ev.bursts.push_back({e.pos + dir * (reach * 0.5f), e.radius * 1.4f, theme::textLo, nullptr});
}

// "Powder keg": every death blasts the ones around it (and they can chain).
void World::pactOnDeath(const Enemy& dead, FrameEvents& ev) {
    if (!pact_.powderKeg) return;
    namespace P = cfg::pact;
    const float R = P::kegRadius * arenaScale();
    const float blast = dead.maxHp * P::kegFrac;
    for (Enemy& o : enemies_) {
        if (&o == &dead || o.hp <= 0.f || o.orbiter || length(o.pos - dead.pos) > R + o.radius) continue;
        o.lastBy = dead.lastBy;
        damageEnemy(o, blast);
    }
    ev.bursts.push_back({dead.pos, R, theme::ember, nullptr});
    if (pact_.kegChip && length(dead.pos - core_.pos) < R + core_.radius) {   // too close: the core feels it
        core_.hp = std::max(1.f, core_.hp - core_.maxHp * P::kegCoreChip);
        core_.hitFlash = 1.f;
    }
}

// Every step: "Lodestone" pulls enemies toward a held ball; "Scorch" sets
// the ones a ball's trail crosses on fire.
void World::pactTick(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace P = cfg::pact;
    (void)ev;
    if (pact_.lodestone && grabbed_ == Grabbed::Ball) {
        const Ball& h = balls_[static_cast<std::size_t>(heldIndex_)];
        const float R = P::lodestoneRadius * arenaScale();
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const sf::Vector2f d = h.pos - e.pos;
            const float dist = length(d);
            const float gap = dist - h.radius - e.radius;
            if (dist > R || gap <= 2.f) continue;
            const float pull = P::lodestonePull * arenaScale() * (0.4f + 0.6f * (1.f - dist / R)) * dt;
            e.pos += d / dist * std::min(pull, gap);
        }
    }
    if (pact_.scorch) {
        for (const Ball& b : balls_) {
            if (b.held || b.trail.size() < 2) continue;
            const float dps = ballDamage(b, p) * P::scorchDps;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                const float reach = e.radius + b.radius * 0.6f;
                bool crossed = false;
                for (std::size_t i = 0; i + 1 < b.trail.size() && !crossed; i += 2)
                    crossed = length(e.pos - b.trail[i]) < reach;
                if (!crossed) continue;
                e.burn = std::max(e.burn, P::scorchTime);   // a burn like fire's (it counts as an element)
                e.burnDps = std::max(e.burnDps, dps);
                e.lastBy = b.owner;
            }
        }
    }
}

// "Juggler": a juggled ball touching the core loses its streak and chips it.
void World::pactCoreBounce(Ball& b) {
    if (!pact_.juggler || !pact_.juggleChip || b.juggle <= 0 || b.ghost) return;
    b.juggle = 0;
    core_.hp = std::max(1.f, core_.hp - core_.maxHp * cfg::pact::juggleCoreChip);
    core_.hitFlash = 1.f;
}

void World::pactOnGrab(Ball& b) {
    if (pact_.frenzyBreak) comboStreak_ = 0;   // "Frenzy": your hands break the combo
    if (pact_.juggler) b.juggle = std::min(b.juggle + 1, cfg::pact::juggleMax);   // "Juggler": one more catch
    b.rubber = 0;   // "Rubber walls": caught, the stacks go
    // "Afterimage": a ball caught on the fly leaves a ghost of itself flying on.
    if (pact_.afterimage && !b.ghost &&
        length(b.vel) > cfg::pact::afterimageFrom * cfg::ball::baseCruise * arenaScale() &&
        static_cast<int>(ghosts_.size() + pendingGhosts_.size()) < cfg::synergy::maxGhosts) {
        Ball g = b;
        g.ghost = true;
        g.twin = false;
        g.held = false;
        g.classPulse = 0.f;
        g.catchBonus = 0.f;
        g.starfall = false;
        g.ghostLife = cfg::pact::afterimageLife;
        g.trail.clear();
        g.waterTrail.clear();
        pendingGhosts_.push_back(g);
    }
}

void World::pactOnThrow(Ball& b) {
    b.sinceThrow = 0.f;
    if (pact_.starfall && ++pactThrows_ % cfg::pact::starfallEvery == 0) b.starfall = true;   // "Starfall": a meteor
    if (pact_.hotPotato && heldT_ > 0.f) {   // "Hot Potato": the time you held it rides on the first hit
        b.catchBonus += cfg::pact::potatoPerSec * std::min(heldT_, cfg::pact::potatoMax);
        b.catchT = cfg::combat::catchWindow;
    }
    heldT_ = 0.f;
    if (pact_.mirror && static_cast<int>(ghosts_.size() + pendingGhosts_.size()) < cfg::synergy::maxGhosts) {
        Ball g = b;   // "Mirror": a ghost of the throw, the opposite way
        g.ghost = true;
        g.twin = false;
        g.classPulse = 0.f;
        g.held = false;
        g.catchBonus = 0.f;
        g.ghostLife = cfg::synergy::ghostLife * b.mods.copyLife;
        g.vel = -b.vel;
        g.trail.clear();
        g.waterTrail.clear();
        pendingGhosts_.push_back(g);
    }
}

// "Hot Potato": held past its limit, the ball slips out of your hand with no
// throw behind it (App / PlayScreen see the grab end).
void World::pactHeldTick(float dt) {
    heldT_ += dt;
    if (!pact_.potatoSlip || heldT_ < cfg::pact::potatoMax || grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[static_cast<std::size_t>(heldIndex_)];
    b.held = false;
    b.vel = rng_.direction() * cfg::ball::nudgeSpeed;
    heldCatch_ = 0.f;
    heldT_ = 0.f;
    heldGrabOffset_ = {0.f, 0.f};
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

}  // namespace sb
