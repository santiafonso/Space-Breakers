// Pact hooks (Fase O). Everything a pact does inside the simulation lives here,
// kept apart from World.cpp: each function is a no-op unless its pact is on
// (WorldParams::pact), and World.cpp only calls into them from one line each.

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

namespace {

// A plain enemy's HP on this wave: the core's own attacks scale with it so
// they stay relevant from wave 1 to 20 (same curve as World.cpp).
float plainEnemyHp(int wave) {
    return cfg::wave::hpBase * std::pow(cfg::wave::hpGrowth, static_cast<float>(std::max(1, wave) - 1));
}

}  // namespace

// "Duet": the balls past the first n leave the arena (App already folded
// their gear into the survivors). Their Gemini twins go with them.
void World::trimBalls(int n) {
    n = std::max(0, n);
    if (static_cast<int>(balls_.size()) <= n) return;
    if (grabbed_ == Grabbed::Ball && heldIndex_ >= n) forceRelease();
    balls_.erase(balls_.begin() + n, balls_.end());
    ghosts_.erase(std::remove_if(ghosts_.begin(), ghosts_.end(),
                                 [n](const Ball& g) { return g.owner >= n; }),
                  ghosts_.end());
}

// "Nova": every ball is thrown out of the core at once, fanned evenly around
// it, and the core shoves back whatever was pressing on it.
void World::pactNova(const WorldParams& p) {
    forceRelease();
    std::vector<Ball*> movers;
    for (Ball& b : balls_)
        if (!b.mods.satellite) movers.push_back(&b);
    const float off = rng_.range(0.f, 2.f * kPi);
    const int n = static_cast<int>(movers.size());
    for (int i = 0; i < n; ++i) {
        Ball& b = *movers[static_cast<std::size_t>(i)];
        const float a = off + 2.f * kPi * static_cast<float>(i) / static_cast<float>(std::max(1, n));
        const sf::Vector2f dir{std::cos(a), std::sin(a)};
        b.pos = core_.pos + dir * (core_.radius + b.radius + 6.f);
        b.vel = dir * std::min(ballCruise(b, p) * cfg::pact::novaSpeed, ballMaxSpeed(b, p));
        b.trail.clear();
        b.squash = 1.f;
        b.squashAxis = dir;
    }
    const float R = cfg::pact::novaRadius * arenaScale();
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const sf::Vector2f d = e.pos - core_.pos;
        if (length(d) > R + e.radius) continue;
        e.vel += normalized(d, {1.f, 0.f}) * cfg::pact::novaKnock * e.knockTaken;
        e.stagger = std::max(e.stagger, 0.6f);
    }
}

// "Hunters": every ball keeps its own prey - preferably one nobody else is
// chasing, closest to the core - and bends hard toward it until it dies. It
// bounces off on contact and swings straight back in, pecking at it.
void World::updateHunters(float dt, const WorldParams& p) {
    if (!p.pact.hunters) {
        huntPrey_.clear();
        huntHas_.clear();
        return;
    }
    const std::size_t n = balls_.size();
    huntPrey_.resize(n, {0.f, 0.f});
    huntHas_.resize(n, 0);
    const float keep = cfg::pact::huntKeep * arenaScale();
    const bool bossOpen = boss_.alive && boss_.intro <= 0.f;

    // Follow each prey from where it was last step (enemies don't have ids;
    // they move a few px per step, so the nearest one is the same enemy).
    for (std::size_t i = 0; i < n; ++i) {
        if (!huntHas_[i]) continue;
        float best = keep * keep;
        bool found = false;
        sf::Vector2f at;
        for (const Enemy& e : enemies_) {
            if (e.hp <= 0.f) continue;
            const float d2 = dot(e.pos - huntPrey_[i], e.pos - huntPrey_[i]);
            if (d2 < best) { best = d2; at = e.pos; found = true; }
        }
        if (bossOpen && dot(boss_.pos - huntPrey_[i], boss_.pos - huntPrey_[i]) < best) { at = boss_.pos; found = true; }
        huntHas_[i] = found ? 1 : 0;
        if (found) huntPrey_[i] = at;
    }
    // Free balls pick new prey: the enemy nearest the core that the fewest
    // other balls are already on.
    for (std::size_t i = 0; i < n; ++i) {
        if (huntHas_[i]) continue;
        const Enemy* pick = nullptr;
        float bestScore = 1e18f;
        for (const Enemy& e : enemies_) {
            if (e.hp <= 0.f) continue;
            int chasers = 0;
            for (std::size_t j = 0; j < n; ++j)
                if (j != i && huntHas_[j] && length(huntPrey_[j] - e.pos) < keep) ++chasers;
            const float score = length(e.pos - core_.pos) + static_cast<float>(chasers) * 2000.f;
            if (score < bestScore) { bestScore = score; pick = &e; }
        }
        if (pick) { huntPrey_[i] = pick->pos; huntHas_[i] = 1; }
        else if (bossOpen) { huntPrey_[i] = boss_.pos; huntHas_[i] = 1; }
    }
    // Steer.
    for (std::size_t i = 0; i < n; ++i) {
        Ball& b = balls_[i];
        if (!huntHas_[i] || b.held || b.mods.satellite) continue;
        float sp = length(b.vel);
        if (sp < 1e-3f) continue;
        const float cur = std::atan2(b.vel.y, b.vel.x);
        const float want = std::atan2(huntPrey_[i].y - b.pos.y, huntPrey_[i].x - b.pos.x);
        const float maxTurn = cfg::pact::huntTurn * dt;
        const float diff = clampf(std::remainder(want - cur, 2.f * kPi), -maxTurn, maxTurn);
        sp = std::min(std::max(sp, ballCruise(b, p) * cfg::pact::huntMinSpeed), ballMaxSpeed(b, p));
        b.vel = sf::Vector2f{std::cos(cur + diff), std::sin(cur + diff)} * sp;
    }
}

// "Living Core": the core zaps the nearest enemy on a timer.
void World::updateCoreZap(float dt, const WorldParams& p, FrameEvents& ev) {
    (void)ev;
    if (!p.pact.livingCore || !waveRunning_) return;
    coreZapT_ -= dt;
    if (coreZapT_ > 0.f) return;
    Enemy* t = nullptr;
    float best = cfg::pact::coreZapRange * arenaScale();
    best *= best;
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const float d2 = dot(e.pos - core_.pos, e.pos - core_.pos);
        if (d2 < best) { best = d2; t = &e; }
    }
    if (!t) {
        coreZapT_ = 0.15f;   // nothing in reach: look again soon
        return;
    }
    damageEnemy(*t, plainEnemyHp(wave_) * cfg::pact::coreZapHpFrac * p.damageMult);
    if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
        bolts_.push_back(Bolt{core_.pos, t->pos, cfg::element::boltLife * 1.5f, cfg::element::boltLife * 1.5f});
    coreZapT_ = cfg::pact::coreZapEvery;
}

// "Pinball": a wall is a bumper - speed, combo, and a spark on anything near.
void World::pactWallBump(Ball& b, sf::Vector2f at, const WorldParams& p, FrameEvents& ev) {
    if (!p.pact.pinball) return;
    boostSpeed(b, cfg::pact::pinBoost, p);
    comboStreak_ += 1;
    sinceHit_ = 0.f;
    const float R = cfg::pact::pinSparkRadius * arenaScale();
    const float dmg = ballDamage(b, p) * cfg::pact::pinSparkFrac;
    bool any = false;
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || length(e.pos - at) > R + e.radius) continue;
        damageEnemy(e, dmg);
        any = true;
    }
    if (any) ev.bursts.push_back({at, R, b.color, nullptr});   // only when it lands - no noise on empty walls
}

// "Legion": two balls clacking together throw sparks.
void World::pactClack(Ball& a, Ball& b, sf::Vector2f at, const WorldParams& p, FrameEvents& ev) {
    if (!p.pact.legion) return;
    comboStreak_ += 2;
    sinceHit_ = 0.f;
    const float R = cfg::pact::legionSparkRadius * arenaScale();
    const float dmg = 0.5f * (ballDamage(a, p) + ballDamage(b, p)) * cfg::pact::legionSparkFrac;
    bool any = false;
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || length(e.pos - at) > R + e.radius) continue;
        damageEnemy(e, dmg);
        any = true;
    }
    if (any) ev.bursts.push_back({at, R, lerpColor(a.color, b.color, 0.5f), nullptr});
}

// "Living Core": a ball bouncing off the core leaves overcharged.
void World::pactCoreBounce(Ball& b, const WorldParams& p, FrameEvents& ev) {
    if (!p.pact.livingCore) return;
    b.pactCharge = cfg::pact::coreChargeTime;
    boostSpeed(b, cfg::pact::coreChargeBoost, p);
    ev.bursts.push_back({core_.pos, core_.radius + 22.f, theme::core, nullptr});
}

// Something reached the core. "Bloodlust" loses its combo; "Fortress" blows
// the attacker's friends away.
void World::pactCoreHit(const WorldParams& p, FrameEvents& ev) {
    if (p.pact.bloodlust) comboStreak_ = 0;
    if (!p.pact.fortress) return;
    const float R = cfg::pact::fortressBlastRadius * arenaScale();
    const float dmg = plainEnemyHp(wave_) * cfg::pact::fortressBlastHpFrac * p.damageMult;
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const sf::Vector2f d = e.pos - core_.pos;
        if (length(d) > R + e.radius) continue;
        damageEnemy(e, dmg);
        e.vel += normalized(d, {1.f, 0.f}) * cfg::pact::novaKnock * e.knockTaken;
        e.stagger = std::max(e.stagger, 0.7f);
    }
    ev.bursts.push_back({core_.pos, R, theme::core, nullptr});
}

}  // namespace sb
