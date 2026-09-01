#include "sim/World.hpp"

#include <algorithm>
#include <cmath>

#include "sim/Collision.hpp"

namespace sb {

namespace {

int waveEnemyCount(int wave) {
    const int n = static_cast<int>(std::lround(
        cfg::wave::baseCount * std::pow(cfg::wave::countGrowth, static_cast<float>(wave - 1))));
    return std::clamp(n, 1, cfg::wave::maxCount);
}
float waveEnemyHp(int wave) {
    return cfg::wave::hpBase * std::pow(cfg::wave::hpGrowth, static_cast<float>(wave - 1));
}
float waveEnemySpeed(int wave) {
    return std::min(cfg::wave::speedMax,
                    cfg::wave::speedBase * std::pow(cfg::wave::speedGrowth, static_cast<float>(wave - 1)));
}
int scrapPerKill(int wave) { return 2 + wave / 2; }

sf::Vector2f perp(sf::Vector2f v) { return {-v.y, v.x}; }

}  // namespace

World::World(sf::Vector2f size) : size_(size) { core_.pos = size_ * 0.5f; }

// ---------------------------------------------------------------- speeds

float World::cruiseBase(const WorldParams& p) const {
    return cfg::ball::baseCruise * p.cruiseMult;
}

float World::cruiseSpeed(const WorldParams& p) const {
    float c = cruiseBase(p);
    if (effect_) {
        if (effect_->kind == PowerUp::SlowMo) c *= cfg::powerup::slowMoCruiseMul;
        else if (effect_->kind == PowerUp::Surge) c *= cfg::powerup::surgeCruiseMul;
    }
    return c;
}

float World::maxSpeed(const WorldParams& p) const {
    return std::min(cruiseBase(p) * cfg::ball::maxSpeedCruiseMul, cfg::ball::hardSpeedCap);
}

float World::fastestBall() const {
    float m = 0.f;
    for (const Ball& b : balls_) m = std::max(m, length(b.vel));
    return m;
}

// ---------------------------------------------------------------- lifecycle

void World::spawnBall(Element e, const WorldParams& p) {
    Ball b;
    b.element = e;
    b.baseOrbit =
        cfg::orbit::radiusPx + rng_.range(-cfg::orbit::radiusJitter, cfg::orbit::radiusJitter);
    b.orbitRadius = b.baseOrbit;
    const float a = rng_.range(0.f, 2.f * kPi);
    const sf::Vector2f radial{std::cos(a), std::sin(a)};
    b.pos = core_.pos + radial * b.orbitRadius;
    b.vel = perp(radial) * cruiseBase(p);  // start it circling
    b.color = theme::speedColor(length(b.vel), cruiseBase(p));
    b.cooldown = rng_.range(0.f, 0.6f);
    balls_.push_back(b);
}

void World::addBall(Element e, const WorldParams& p) {
    if (static_cast<int>(balls_.size()) >= cfg::ball::maxBalls) return;
    spawnBall(e, p);
}

void World::repairCore(float amount) {
    core_.hp = std::min(core_.maxHp, core_.hp + amount);
}

void World::startRun(const WorldParams& p, const std::vector<int>& ballElements,
                     float coreHp, float coreMaxHp) {
    balls_.clear();
    enemies_.clear();
    projectiles_.clear();
    puddles_.clear();
    obstacles_.clear();
    pickups_.clear();
    effect_.reset();
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
    comboStreak_ = 0;
    comboCapTier_ = cfg::combo::baseCapTier;
    sinceHit_ = 0.f;
    reportedTier_ = 0;
    wave_ = 0;
    waveRunning_ = false;
    runOver_ = false;
    toSpawn_ = 0;
    spawnTimer_ = 0.f;

    core_.pos = size_ * 0.5f;
    core_.maxHp = coreMaxHp;
    core_.hp = std::min(coreHp, coreMaxHp);
    core_.hitFlash = 0.f;

    if (ballElements.empty()) {
        spawnBall(Element::Plain, p);
    } else {
        for (int e : ballElements) spawnBall(static_cast<Element>(std::clamp(e, 0, kElementCount - 1)), p);
    }
    pickupTimer_ = rng_.range(cfg::pickup::firstSpawnMin, cfg::pickup::firstSpawnMax);
}

void World::startWave(int wave, const WorldParams& p) {
    wave_ = wave;
    toSpawn_ = waveEnemyCount(wave);
    spawnTimer_ = 0.35f;
    waveRunning_ = true;
    projectiles_.clear();

    // Re-seed the balls onto their orbits.
    for (Ball& b : balls_) {
        const float a = rng_.range(0.f, 2.f * kPi);
        const sf::Vector2f radial{std::cos(a), std::sin(a)};
        b.pos = core_.pos + radial * b.orbitRadius;
        b.vel = perp(radial) * cruiseBase(p);
        b.trail.clear();
        b.held = false;
    }
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

void World::spawnEnemy() {
    sf::Vector2f pos;
    switch (rng_.irange(0, 3)) {
        case 0: pos = {rng_.range(0.f, size_.x), -cfg::wave::enemyRadius}; break;
        case 1: pos = {rng_.range(0.f, size_.x), size_.y + cfg::wave::enemyRadius}; break;
        case 2: pos = {-cfg::wave::enemyRadius, rng_.range(0.f, size_.y)}; break;
        default: pos = {size_.x + cfg::wave::enemyRadius, rng_.range(0.f, size_.y)}; break;
    }
    Enemy e;
    e.pos = pos;
    e.radius = cfg::wave::enemyRadius;
    e.maxHp = e.hp = waveEnemyHp(wave_);
    e.speed = waveEnemySpeed(wave_);
    e.vel = normalized(core_.pos - pos) * e.speed;
    enemies_.push_back(e);
}

// ---------------------------------------------------------------- grab / throw

bool World::grabAt(sf::Vector2f point, float catchRadius) {
    int best = -1;
    float bestDist = catchRadius;
    for (std::size_t i = 0; i < balls_.size(); ++i) {
        const float d = length(balls_[i].pos - point);
        if (d < bestDist) {
            bestDist = d;
            best = static_cast<int>(i);
        }
    }
    if (best < 0) return false;
    grabbed_ = Grabbed::Ball;
    heldIndex_ = best;
    Ball& b = balls_[best];
    b.held = true;
    b.vel = {0.f, 0.f};
    b.trail.clear();
    return true;
}

void World::moveHeld(sf::Vector2f target) {
    if (grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[heldIndex_];
    b.pos.x = clampf(target.x, b.radius, size_.x - b.radius);
    b.pos.y = clampf(target.y, b.radius, size_.y - b.radius);
    b.vel = {0.f, 0.f};
}

void World::releaseHeld(sf::Vector2f throwVel) {
    if (grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[heldIndex_];
    b.held = false;
    const float s = length(throwVel);
    if (s < cfg::ball::minThrowSpeed) b.vel = rng_.direction() * cfg::ball::nudgeSpeed;
    else if (s > cfg::ball::hardSpeedCap) b.vel = throwVel * (cfg::ball::hardSpeedCap / s);
    else b.vel = throwVel;
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

void World::forceRelease() {
    if (grabbed_ == Grabbed::Ball) {
        Ball& b = balls_[heldIndex_];
        b.held = false;
        b.vel = rng_.direction() * cfg::ball::forceReleaseSpeed;
    }
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

// ---------------------------------------------------------------- per-step parts

void World::advanceCombo(float dt) {
    comboCapTier_ = cfg::combo::baseCapTier;
    sinceHit_ += dt;
    if (sinceHit_ > cfg::combo::decayWindow && comboStreak_ > 0) {
        comboStreak_ = std::max(0, comboStreak_ - cfg::combo::bouncesPerTier);
        sinceHit_ = 0.f;
    }
}

void World::afterBounce(Ball& b, sf::Vector2f normal, bool countHit) {
    b.squash = 1.f;
    b.squashAxis = normal;
    if (countHit) {
        ++comboStreak_;
        sinceHit_ = 0.f;
    }
}

float World::ballDamage(const Ball& b, const WorldParams& p) const {
    const float ratio = length(b.vel) / cfg::ball::baseCruise;
    float dmg = (cfg::combat::contactDamageBase + cfg::combat::contactDamagePerCruise * ratio) *
                comboMultiplier() * p.damageMult;
    if (effect_) {
        if (effect_->kind == PowerUp::Points2x) dmg *= 2.f;
        else if (effect_->kind == PowerUp::Frenzy) dmg *= 3.f;
        else if (effect_->kind == PowerUp::Golden) dmg += 3.f;
    }
    return dmg;
}

void World::orbitAssist(Ball& b, float dt, const WorldParams& p) const {
    const sf::Vector2f r = b.pos - core_.pos;
    const float dist = length(r);
    const sf::Vector2f radial = dist > 1e-3f ? r / dist : sf::Vector2f{1.f, 0.f};
    sf::Vector2f tangent = perp(radial);
    if (dot(b.vel, tangent) < 0.f) tangent = -tangent;  // keep its spin direction

    const float cruise = cruiseSpeed(p);

    // Nearest enemy in range.
    const Enemy* near = nullptr;
    float bestD2 = cfg::orbit::interceptRange * cfg::orbit::interceptRange;
    for (const Enemy& e : enemies_) {
        const float d2 = dot(e.pos - b.pos, e.pos - b.pos);
        if (d2 < bestD2) { bestD2 = d2; near = &e; }
    }

    // Adapt the orbit radius: expand / shrink the ring so it passes through the
    // nearest enemy; otherwise ease back to this ball's resting radius.
    const float wantRing =
        near ? clampf(length(near->pos - core_.pos), cfg::orbit::ringMin, cfg::orbit::ringMax)
             : b.baseOrbit;
    b.orbitRadius += (wantRing - b.orbitRadius) * (1.f - std::exp(-cfg::orbit::ringAdapt * dt));

    // Target orbit velocity: cruise along the tangent + a bounded pull toward the
    // (adapted) orbit radius. Tangential -> following it traces a curve.
    const float radialCorr = clampf(-cfg::orbit::pullK * (dist - b.orbitRadius),
                                    -cfg::orbit::maxRadial, cfg::orbit::maxRadial);
    sf::Vector2f target = tangent * cruise + radial * radialCorr;

    // Lean the target partly toward the enemy (never all the way -> never a
    // straight line at it).
    if (near) {
        const sf::Vector2f toEnemy = normalized(near->pos - b.pos) * cruise;
        const float w = cfg::orbit::interceptWeight;
        target = target * (1.f - w) + toEnemy * w;
    }

    // Steer the velocity toward the target (converges regardless of current speed,
    // so a flung ball spirals back into orbit).
    const float k = 1.f - std::exp(-cfg::orbit::steerRate * dt);
    b.vel += (target - b.vel) * k;
}

void World::emitElement(Ball& b, float dt, const WorldParams& p) {
    switch (b.element) {
        case Element::Wind: {
            b.cooldown -= dt;
            if (b.cooldown > 0.f) break;
            const Enemy* target = nullptr;
            float bestD2 = cfg::element::windRange * cfg::element::windRange;
            for (const Enemy& e : enemies_) {
                const float d2 = dot(e.pos - b.pos, e.pos - b.pos);
                if (d2 < bestD2) { bestD2 = d2; target = &e; }
            }
            if (target && static_cast<int>(projectiles_.size()) < cfg::element::maxProjectiles) {
                Projectile pr;
                pr.pos = b.pos;
                pr.vel = normalized(target->pos - b.pos) * cfg::element::windSpeed;
                pr.life = cfg::element::windLife;
                pr.damage = cfg::element::windDamage * p.damageMult;
                projectiles_.push_back(pr);
                b.cooldown = cfg::element::windInterval;
            } else {
                b.cooldown = 0.25f;  // retry soon
            }
            break;
        }
        case Element::Water: {
            b.cooldown -= dt;
            if (b.cooldown > 0.f) break;
            b.cooldown = cfg::element::waterInterval;
            if (static_cast<int>(puddles_.size()) < cfg::element::maxPuddles)
                puddles_.push_back(Puddle{b.pos, cfg::element::puddleRadius,
                                          cfg::element::puddleLife, cfg::element::puddleLife});
            break;
        }
        case Element::Stone: {
            b.cooldown -= dt;
            if (b.cooldown > 0.f) break;
            b.cooldown = cfg::element::stoneInterval;
            if (static_cast<int>(obstacles_.size()) < cfg::element::maxObstacles)
                obstacles_.push_back(Obstacle{b.pos, cfg::element::obstacleRadius,
                                              cfg::element::obstacleLife, cfg::element::obstacleLife});
            break;
        }
        default:
            break;
    }
}

void World::regulateSpeed(Ball& b, const WorldParams& p) {
    const float sp = length(b.vel);
    if (sp < 1e-3f) {
        b.vel = rng_.direction() * cruiseSpeed(p);
        return;
    }
    // orbitAssist already steers speed toward cruise; here we only enforce the
    // hard ceiling for a hard fling.
    const float vMax = maxSpeed(p);
    if (sp > vMax) b.vel *= vMax / sp;
}

void World::updateTrail(Ball& b) {
    b.trail.push_back(b.pos);
    const std::size_t cap =
        static_cast<std::size_t>(clampf(4.f + length(b.vel) / 90.f, 4.f, 16.f));
    while (b.trail.size() > cap) b.trail.pop_front();
}

void World::advanceBall(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    orbitAssist(b, dt, p);

    const float speed = length(b.vel);
    const int steps = std::clamp(
        static_cast<int>(std::ceil(speed * dt / (b.radius * cfg::ball::substepPerRadius))),
        1, cfg::ball::maxSubsteps);
    const float h = dt / static_cast<float>(steps);

    auto pushFx = [&](const collision::Contact& c) {
        BounceFx fx;
        fx.normal = c.normal;
        fx.pos = c.point;
        fx.color = b.color;
        fx.speed = length(b.vel);
        ev.bounces.push_back(fx);
    };

    for (int s = 0; s < steps; ++s) {
        b.pos += b.vel * h;

        if (collision::Contact c = collision::circleVsBounds(b, size_); c.hit) {
            afterBounce(b, c.normal, false);
            pushFx(c);
        }
        for (Enemy& e : enemies_) {
            collision::Contact c =
                collision::circleVsSolidCircle(b, e.pos, e.radius, cfg::combat::hitRebound);
            if (!c.hit) continue;
            e.hp -= ballDamage(b, p);
            e.hitFlash = 1.f;
            e.vel += -c.normal * cfg::combat::knockback;
            if (b.element == Element::Fire) {
                e.burn = cfg::element::burnDuration;
                e.burnDps = cfg::element::burnDps * p.damageMult;
            }
            afterBounce(b, c.normal, true);
            pushFx(c);
        }
    }

    emitElement(b, dt, p);
    regulateSpeed(b, p);
    b.color = theme::speedColor(length(b.vel), cruiseBase(p));
    b.squash *= std::exp(-cfg::ball::squashDecay * dt);
    updateTrail(b);
}

void World::resolveBallPairs() {
    for (std::size_t i = 0; i < balls_.size(); ++i) {
        if (grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_) continue;
        for (std::size_t j = i + 1; j < balls_.size(); ++j) {
            if (grabbed_ == Grabbed::Ball && static_cast<int>(j) == heldIndex_) continue;
            collision::resolveBallPair(balls_[i], balls_[j]);
        }
    }
}

void World::updateProjectiles(float dt) {
    for (auto it = projectiles_.begin(); it != projectiles_.end();) {
        it->pos += it->vel * dt;
        it->life -= dt;
        bool hit = false;
        for (Enemy& e : enemies_) {
            if (length(e.pos - it->pos) < e.radius + 4.f) {
                e.hp -= it->damage;
                e.hitFlash = 1.f;
                hit = true;
                break;
            }
        }
        if (hit || it->life <= 0.f || it->pos.x < -20.f || it->pos.x > size_.x + 20.f ||
            it->pos.y < -20.f || it->pos.y > size_.y + 20.f)
            it = projectiles_.erase(it);
        else
            ++it;
    }
}

void World::updatePuddles(float dt) {
    for (auto it = puddles_.begin(); it != puddles_.end();) {
        it->life -= dt;
        for (Enemy& e : enemies_)
            if (length(e.pos - it->pos) < it->radius + e.radius)
                e.hp -= cfg::element::puddleDps * dt;
        if (it->life <= 0.f) it = puddles_.erase(it);
        else ++it;
    }
}

void World::updateObstacles(float dt) {
    for (auto it = obstacles_.begin(); it != obstacles_.end();) {
        it->life -= dt;
        if (it->life <= 0.f) it = obstacles_.erase(it);
        else ++it;
    }
}

void World::updateEnemies(float dt, FrameEvents& ev) {
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        Enemy& e = *it;

        if (e.burn > 0.f) {
            e.burn -= dt;
            e.hp -= e.burnDps * dt;
        }

        const sf::Vector2f d = core_.pos - e.pos;
        const float dist = length(d);
        const sf::Vector2f steer = (dist > 1e-3f ? d / dist : sf::Vector2f{0.f, 1.f}) * e.speed;
        e.vel += (steer - e.vel) * (1.f - std::exp(-8.f * dt));
        e.pos += e.vel * dt;
        e.hitFlash *= std::exp(-6.f * dt);

        // Pushed out of any rubble in the way.
        for (const Obstacle& o : obstacles_) {
            const sf::Vector2f od = e.pos - o.pos;
            const float sum = o.radius + e.radius;
            const float dd = length(od);
            if (dd < sum && dd > 1e-3f) e.pos += (od / dd) * (sum - dd);
        }

        if (dist <= core_.radius + e.radius) {
            core_.hp -= cfg::core::enemyDamage;
            core_.hitFlash = 1.f;
            ev.coreHit = true;
            it = enemies_.erase(it);
            if (core_.hp <= 0.f) {
                core_.hp = 0.f;
                runOver_ = true;
            }
        } else {
            ++it;
        }
    }
    core_.hitFlash *= std::exp(-5.f * dt);
}

void World::sweepDeadEnemies(const WorldParams& p, FrameEvents& ev) {
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        if (it->hp <= 0.f) {
            ev.kills.push_back(it->pos);
            ev.scrapGained += scrapPerKill(p.wave);
            it = enemies_.erase(it);
        } else {
            ++it;
        }
    }
}

void World::updateWaveSpawner(float dt, FrameEvents& ev) {
    if (!waveRunning_) return;
    if (toSpawn_ > 0) {
        spawnTimer_ -= dt;
        if (spawnTimer_ <= 0.f) {
            spawnEnemy();
            --toSpawn_;
            spawnTimer_ = cfg::wave::spawnInterval;
        }
    }
    if (toSpawn_ == 0 && enemies_.empty()) {
        waveRunning_ = false;
        ev.waveCleared = true;
    }
}

void World::updatePickups(float dt, FrameEvents& ev) {
    if (!effect_ && pickups_.empty()) {
        pickupTimer_ -= dt;
        if (pickupTimer_ <= 0.f) {
            Pickup pu;
            pu.kind = static_cast<PowerUp>(rng_.irange(0, kPowerUpCount - 1));
            pu.pos = {rng_.range(size_.x * 0.15f, size_.x * 0.85f),
                      rng_.range(size_.y * 0.15f, size_.y * 0.85f)};
            pu.vel = rng_.direction() * rng_.range(cfg::pickup::driftMin, cfg::pickup::driftMax);
            pickups_.push_back(pu);
            pickupTimer_ = rng_.range(cfg::pickup::spawnMin, cfg::pickup::spawnMax);
        }
    }

    for (auto it = pickups_.begin(); it != pickups_.end();) {
        Pickup& pu = *it;
        pu.age += dt;
        pu.pos += pu.vel * dt;
        if (pu.pos.x - pu.radius < 0.f || pu.pos.x + pu.radius > size_.x) {
            pu.vel.x = -pu.vel.x;
            pu.pos.x = clampf(pu.pos.x, pu.radius, size_.x - pu.radius);
        }
        if (pu.pos.y - pu.radius < 0.f || pu.pos.y + pu.radius > size_.y) {
            pu.vel.y = -pu.vel.y;
            pu.pos.y = clampf(pu.pos.y, pu.radius, size_.y - pu.radius);
        }

        bool collected = false;
        for (const Ball& b : balls_) {
            if (length(b.pos - pu.pos) < b.radius + pu.radius) {
                const float dur = powerUpDuration(pu.kind);
                effect_ = ActiveEffect{pu.kind, dur, dur};
                ev.gotPickup = true;
                ev.pickupKind = pu.kind;
                pickupTimer_ = rng_.range(cfg::pickup::spawnMin, cfg::pickup::spawnMax);
                collected = true;
                break;
            }
        }
        if (collected || pu.age > pu.ttl) it = pickups_.erase(it);
        else ++it;
    }
}

void World::advanceEffect(float dt) {
    if (!effect_) return;
    effect_->remaining -= dt;
    if (effect_->remaining <= 0.f) effect_.reset();
}

// ---------------------------------------------------------------- step

FrameEvents World::step(float dt, const WorldParams& p) {
    FrameEvents ev;
    if (dt <= 0.f || runOver_) {
        ev.runOver = runOver_;
        return ev;
    }

    advanceCombo(dt);

    for (std::size_t i = 0; i < balls_.size(); ++i) {
        Ball& b = balls_[i];
        if (grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_) {
            b.squash *= std::exp(-cfg::ball::squashDecay * dt);
            continue;
        }
        advanceBall(b, dt, p, ev);
    }

    resolveBallPairs();
    updateProjectiles(dt);
    updatePuddles(dt);
    updateObstacles(dt);
    updateEnemies(dt, ev);
    sweepDeadEnemies(p, ev);
    updateWaveSpawner(dt, ev);
    updatePickups(dt, ev);
    advanceEffect(dt);

    const int tier = comboTier();
    ev.comboTier = tier;
    if (tier > reportedTier_) ev.comboTierUp = true;
    reportedTier_ = tier;
    ev.runOver = runOver_;
    return ev;
}

}  // namespace sb
