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
    if (r.stillness > 0.f) {   // "Stillness": the slower, the harder (full bonus at a standstill)
        const float slow = 1.f - length(b.vel) / std::max(1.f, ballCruise(b, p));
        k *= 1.f + r.stillness * clampf(slow, 0.f, 1.f);
    }
    if (r.idlePenalty && b.sinceThrow > P::quickIdleAfter) k *= P::quickIdleDamage;   // "Quick Hands"
    if (r.juggler) k *= 1.f + P::jugglePer * static_cast<float>(b.juggle);          // "Juggler"
    if (r.lastBreath && core_.hp < core_.maxHp * P::lastBreathAt) k *= P::lastBreathMul;   // "Last Breath"
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

// "Anchor Walls": a wall soaks up nearly all of a ball's speed.
void World::pactWallBump(Ball& b) {
    if (pact_.anchorWalls) b.vel *= cfg::pact::anchorKeep;
}

// "Juggler": a juggled ball touching the core loses its streak and chips it.
void World::pactCoreBounce(Ball& b) {
    if (!pact_.juggler || b.juggle <= 0 || b.ghost) return;
    b.juggle = 0;
    core_.hp = std::max(1.f, core_.hp - core_.maxHp * cfg::pact::juggleCoreChip);
    core_.hitFlash = 1.f;
}

void World::pactOnGrab(Ball& b) {
    if (pact_.frenzy) comboStreak_ = 0;   // "Frenzy": your hands break the combo
    if (pact_.juggler) b.juggle = std::min(b.juggle + 1, cfg::pact::juggleMax);   // "Juggler": one more catch
}

void World::pactOnThrow(Ball& b) {
    b.sinceThrow = 0.f;   // "Quick Hands"
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
    if (!pact_.hotPotato || heldT_ < cfg::pact::potatoMax || grabbed_ != Grabbed::Ball) return;
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
