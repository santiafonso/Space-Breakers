// The bosses' attacks (2026-10-30): every boss casts in turn from a list per
// phase, each telegraphed, so a fight has a rhythm to read instead of one long
// chip at a sponge. Several turn the player's own tools against them: Frost
// freezes the balls near it (the Hex), the Well pulls them away (a black
// hole), the Bouncer is a ball of its own that rebounds off the walls. Waves
// of the act's enemies come in with it, called by number.

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

namespace {
using M = BossMove;

// Each boss's list for its phase (0, 1, 2): the attacks it rotates through.
const std::vector<BossMove>& bossMoves(BossKind k, int stage) {
    static const std::vector<BossMove> lists[5][3] = {
        {{M::Bouncer}, {M::Bouncer, M::Volley}, {M::Nova, M::Bouncer, M::Volley}},              // Charger
        {{M::Brood, M::Volley}, {M::Brood, M::Volley}, {M::Brood, M::Nova, M::Brood, M::Well}},  // Hive
        {{M::Volley}, {M::Frost, M::Volley}, {M::Frost, M::Nova, M::Bouncer}},                  // Warden
        {{M::Bouncer}, {M::Well, M::Bouncer}, {M::Well, M::Nova, M::Bouncer}},                  // Dasher
        {{M::Nova}, {M::Well, M::Nova}, {M::Frost, M::Nova, M::Well, M::Bouncer}},              // Orbital
    };
    return lists[static_cast<int>(k)][std::clamp(stage, 0, 2)];
}
}  // namespace

void World::fireRing(sf::Vector2f from, int n, float turn, int bounces) {
    const float base = rng_.range(0.f, 2.f * kPi);
    for (int i = 0; i < n; ++i) {
        const float a = base + 2.f * kPi * static_cast<float>(i) / static_cast<float>(n);
        Shot s;
        s.pos = from;
        s.vel = sf::Vector2f{std::cos(a), std::sin(a)} * cfg::boss::shotSpeed * arenaScale();
        s.turn = turn;
        s.bounces = bounces;
        shots_.push_back(s);
    }
}

// The rotation: count down, telegraph, cast. It holds while the boss comes
// in or pulls back, and a stun breaks a cast in progress.
void World::updateBossMoves(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace B = cfg::boss;
    if (!boss_.alive || boss_.hp <= 0.f || runOver_) return;
    if (boss_.intro > 0.f || boss_.retreat > 0.f) return;
    if (boss_.stun > 0.f) { boss_.warnT = 0.f; return; }
    const float k = boss_.pace() * (p.timeDilation ? cfg::changer::timeDilation : 1.f);
    if (boss_.warnT > 0.f) {
        if ((boss_.warnT -= dt * k) <= 0.f) {
            boss_.warnT = 0.f;
            castBossMove(boss_.warnMove, ev);
        }
        return;
    }
    if ((boss_.moveT -= dt * k) > 0.f) return;
    boss_.moveT = B::moveEvery;
    const std::vector<BossMove>& list = bossMoves(boss_.kind, boss_.stage);
    boss_.warnMove = list[static_cast<std::size_t>(boss_.moveIdx++) % list.size()];
    boss_.warnT = B::moveWarn;
    if (boss_.warnMove == BossMove::Frost) boss_.frostR = B::frostRadius * arenaScale();
    if (boss_.warnMove == BossMove::Well) {
        // Across the core from the boss: it drags the balls away from it.
        const float side = std::min(size_.x, size_.y);
        const sf::Vector2f away = normalized(core_.pos - boss_.pos, {-1.f, 0.f});
        const float a = std::atan2(away.y, away.x) + rng_.range(-0.8f, 0.8f);
        const sf::Vector2f at = core_.pos + sf::Vector2f{std::cos(a), std::sin(a)} * side * rng_.range(0.3f, 0.4f);
        const float m = 80.f;
        boss_.warnAt = {clampf(at.x, m, size_.x - m), clampf(at.y, m, size_.y - m)};
        boss_.frostR = B::wellRadius * arenaScale();
    }
}

void World::castBossMove(BossMove m, FrameEvents& ev) {
    namespace B = cfg::boss;
    const int stage = boss_.stage;
    switch (m) {
        case BossMove::Volley:
            fireShots(boss_.pos, B::volleyShots + stage, B::volleySpread);
            break;
        case BossMove::Nova:
            fireRing(boss_.pos, B::novaShots + 2 * stage, B::novaTurn, 1);
            break;
        case BossMove::Bouncer: {
            // Its own ball, thrown off the line to the core so it rebounds around first.
            const int n = boss_.enraged ? 2 : 1;
            const float base = std::atan2(core_.pos.y - boss_.pos.y, core_.pos.x - boss_.pos.x);
            for (int i = 0; i < n; ++i) {
                const float a = base + (i == 0 ? 1.f : -1.f) * rng_.range(0.5f, 1.1f);
                Shot s;
                s.rogue = true;
                s.radius = B::rogueRadius * fieldRadiusMul();
                s.pos = boss_.pos + sf::Vector2f{std::cos(a), std::sin(a)} * (boss_.radius + s.radius);
                s.vel = sf::Vector2f{std::cos(a), std::sin(a)} * B::shotSpeed * B::rogueSpeed * arenaScale();
                s.bounces = B::rogueBounces;
                s.damage = B::rogueDamage;
                shots_.push_back(s);
            }
            break;
        }
        case BossMove::Brood: {
            const int n = B::broodEggs + (boss_.enraged ? 1 : 0);
            const float base = std::atan2(core_.pos.y - boss_.pos.y, core_.pos.x - boss_.pos.x);
            const float r = B::eggRadius * fieldRadiusMul();
            for (int i = 0; i < n; ++i) {
                const float a = base + (static_cast<float>(i) / static_cast<float>(std::max(1, n - 1)) - 0.5f) * 2.2f;
                Enemy e;
                e.id = nextEnemyId_++;
                e.radius = r;
                e.maxHp = e.hp = enemyHp(wave_) * B::eggHp;
                e.speed = 0.f;
                e.knockTaken = 0.f;
                e.coreDamage = 0.f;
                e.hatchT = B::eggHatch * rng_.range(0.9f, 1.1f);
                const sf::Vector2f at = boss_.pos + sf::Vector2f{std::cos(a), std::sin(a)} * (boss_.radius + r + rng_.range(50.f, 110.f));
                e.home = e.pos = {clampf(at.x, r, size_.x - r), clampf(at.y, r, size_.y - r)};
                enemies_.push_back(e);
                ev.bursts.push_back({e.pos, r * 2.f, theme::coreLow, nullptr});
            }
            break;
        }
        case BossMove::Frost:
            // The Hex, on every ball near it: frozen where it is, can't be grabbed.
            // Like the Hex, never all of them: the farthest one from the boss stays free.
        {
            std::vector<std::size_t> hit;
            bool anyFree = false;
            for (std::size_t i = 0; i < balls_.size(); ++i) {
                Ball& b = balls_[i];
                const bool inHand = b.held || (grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_);
                if (b.hexT > 0.f || b.snaredBy >= 0) continue;
                if (inHand || length(b.pos - boss_.pos) > boss_.frostR + b.radius) { anyFree = true; continue; }
                hit.push_back(i);
            }
            if (!anyFree && !hit.empty()) {
                auto far = std::max_element(hit.begin(), hit.end(), [&](std::size_t a, std::size_t c) {
                    return length(balls_[a].pos - boss_.pos) < length(balls_[c].pos - boss_.pos);
                });
                hit.erase(far);
            }
            for (std::size_t i : hit) {
                Ball& b = balls_[i];
                b.hexT = b.hexMax = B::frostTime;
                b.vel = {0.f, 0.f};
                b.trail.clear();
            }
        }
            ev.bossFrost = true;
            ev.bursts.push_back({boss_.pos, boss_.frostR, theme::puSurge, nullptr});
            break;
        case BossMove::Well: {
            Well w;
            w.pos = boss_.warnAt;
            w.life = w.maxLife = B::wellLife;
            wells_.push_back(w);
            ev.bursts.push_back({w.pos, B::wellRadius * arenaScale() * 0.4f, theme::pact, nullptr});
            break;
        }
    }
}

// A boss's eggs: they stand (updateEnemies holds them) and hatch into runners
// unless a ball breaks them first.
void World::updateEggs(float dt) {
    namespace B = cfg::boss;
    std::vector<sf::Vector2f> hatched;
    for (Enemy& e : enemies_) {
        if (e.hatchT <= 0.f || e.hp <= 0.f) continue;
        if ((e.hatchT -= dt) > 0.f) continue;
        e.hatchT = 0.f;
        hatched.push_back(e.pos);
        // The egg itself becomes the first runner.
        setEnemyKind(e, EnemyKind::Runner, enemyHp(wave_) * B::addHpMul, enemySpeed(wave_));
        e.vel = normalized(core_.pos - e.pos, {-1.f, 0.f}) * e.speed;
    }
    for (const sf::Vector2f& at : hatched) {
        for (int i = 1; i < B::eggRunners; ++i) {
            spawnEnemy(EnemyKind::Runner);
            Enemy& r = enemies_.back();
            r.maxHp = r.hp = r.hp * B::addHpMul;
            r.pos = at + rng_.direction() * rng_.range(10.f, 30.f);
            r.vel = normalized(core_.pos - r.pos, {-1.f, 0.f}) * r.speed;
        }
    }
}

// Gravity wells pull the balls (not a held one, nor a frozen or snared one),
// then collapse into a ring of orbs.
void World::updateWells(float dt, FrameEvents& ev) {
    namespace B = cfg::boss;
    const float R = B::wellRadius * arenaScale();
    if (!boss_.alive) { wells_.clear(); return; }   // the boss is down: its wells close quietly
    for (auto it = wells_.begin(); it != wells_.end();) {
        Well& w = *it;
        w.life -= dt;
        for (std::vector<Ball>* set : {&balls_, &ghosts_})
            for (std::size_t i = 0; i < set->size(); ++i) {
                Ball& b = (*set)[i];
                if (b.held || b.hexT > 0.f || b.snaredBy >= 0) continue;
                if (set == &balls_ && grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_) continue;
                const sf::Vector2f d = w.pos - b.pos;
                const float dl = length(d);
                if (dl > R || dl < 1.f) continue;
                b.vel += d / dl * B::wellPull * arenaScale() * (1.f - dl / R) * dt;
            }
        if (w.life <= 0.f) {
            fireRing(w.pos, B::wellShots, B::novaTurn, 1);
            ev.bursts.push_back({w.pos, R * 0.5f, theme::pact, nullptr});
            it = wells_.erase(it);
        } else {
            ++it;
        }
    }
}

// A wave of the act's enemies, all from one side, a touch soft.
void World::bossWaveIn(FrameEvents& ev) {
    namespace B = cfg::boss;
    const int n = B::packBase + B::packPerAct * (cfg::run::actOfWave(wave_) - 1);
    const int cap = B::maxAdds + (boss_.kind == BossKind::Orbital ? cfg::finalBoss::shieldCount : 0);
    sf::Vector2f at;
    int made = 0;
    for (int i = 0; i < n && static_cast<int>(enemies_.size()) < cap; ++i) {
        spawnEnemy(rollEnemyKind());
        Enemy& e = enemies_.back();
        e.maxHp = e.hp = e.maxHp * B::addHpMul;
        rollGait(e);
        if (made++ == 0) {
            at = e.pos;
        } else {   // bunched up behind the first, outside the arena
            const bool side = at.x < 0.f || at.x > size_.x;   // came in through a side wall, or top/bottom
            const sf::Vector2f out = side ? sf::Vector2f{at.x < 0.f ? -1.f : 1.f, 0.f}
                                          : sf::Vector2f{0.f, at.y < size_.y * 0.5f ? -1.f : 1.f};
            e.pos = at + out * rng_.range(0.f, 70.f) + sf::Vector2f{-out.y, out.x} * rng_.range(-140.f, 140.f);
            e.vel = normalized(core_.pos - e.pos, {-1.f, 0.f}) * e.speed;
        }
    }
    if (made > 0) ev.bossWave = ++boss_.waves;
}

}  // namespace sb
