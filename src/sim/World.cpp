#include "sim/World.hpp"

#include <algorithm>
#include <cmath>

#include "sim/Collision.hpp"

namespace sb {

namespace {
constexpr float kFieldVisualRadius = 22.f;  // matches WorldRenderer

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
int scrapPerKill(int wave) { return 1 + wave / 3; }
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

// ---------------------------------------------------------------- field

void World::configureField(const std::vector<FieldSnapshot>& snap) {
    field_.clear();
    for (const FieldSnapshot& s : snap) {
        FieldObject f;
        f.pos = {s.x, s.y};
        f.kind = static_cast<FieldKind>(s.kind);
        f.strength = s.strength > 0.f ? s.strength : 1.f;
        f.radius = cfg::field::blackHoleRadius;
        field_.push_back(f);
    }
}

void World::addField(FieldKind kind, sf::Vector2f pos, float strength) {
    FieldObject f;
    f.kind = kind;
    f.pos = {clampf(pos.x, 40.f, size_.x - 40.f), clampf(pos.y, 40.f, size_.y - 40.f)};
    f.strength = strength > 0.f ? strength : 1.f;
    f.radius = cfg::field::blackHoleRadius;
    field_.push_back(f);
}

void World::repairCore(float amount) {
    core_.maxHp += amount;
    core_.hp = std::min(core_.maxHp, core_.hp + amount);
}

std::vector<FieldSnapshot> World::fieldSnapshot() const {
    std::vector<FieldSnapshot> out;
    out.reserve(field_.size());
    for (const FieldObject& f : field_)
        out.push_back({f.pos.x, f.pos.y, f.strength, static_cast<int>(f.kind)});
    return out;
}

bool World::fieldHit(const FieldObject& f, sf::Vector2f point) const {
    return length(point - f.pos) <= kFieldVisualRadius + cfg::field::grabPadding;
}

// ---------------------------------------------------------------- lifecycle

void World::spawnBallAtCore(const WorldParams& p) {
    Ball b;
    b.pos = core_.pos + rng_.direction() * (core_.radius + b.radius + 8.f);
    b.vel = rng_.direction() * cruiseBase(p);
    b.color = theme::speedColor(length(b.vel), cruiseBase(p));
    balls_.push_back(b);
}

void World::setBallCount(int n, const WorldParams& p) {
    n = std::clamp(n, 1, cfg::ball::maxBalls);
    while (static_cast<int>(balls_.size()) < n) spawnBallAtCore(p);
    while (static_cast<int>(balls_.size()) > n) balls_.pop_back();
}

void World::startRun(const WorldParams& p, int ballCount, float coreHp, float coreMaxHp,
                     const std::vector<FieldSnapshot>& field) {
    balls_.clear();
    enemies_.clear();
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

    configureField(field);
    setBallCount(ballCount, p);
    pickupTimer_ = rng_.range(cfg::pickup::firstSpawnMin, cfg::pickup::firstSpawnMax);
}

void World::startWave(int wave, const WorldParams& p) {
    wave_ = wave;
    toSpawn_ = waveEnemyCount(wave);
    spawnTimer_ = 0.3f;
    waveRunning_ = true;

    // Every wave re-launches the balls from the core.
    for (Ball& b : balls_) {
        b.pos = core_.pos + rng_.direction() * (core_.radius + b.radius + 8.f);
        b.vel = rng_.direction() * cruiseBase(p);
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
    for (int i = static_cast<int>(field_.size()) - 1; i >= 0; --i) {
        if (fieldHit(field_[i], point)) {
            grabbed_ = Grabbed::Field;
            heldIndex_ = i;
            field_[i].held = true;
            return true;
        }
    }

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
    if (grabbed_ == Grabbed::Field) {
        FieldObject& f = field_[heldIndex_];
        f.pos.x = clampf(target.x, 24.f, size_.x - 24.f);
        f.pos.y = clampf(target.y, 24.f, size_.y - 24.f);
        return;
    }
    if (grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[heldIndex_];
    b.pos.x = clampf(target.x, b.radius, size_.x - b.radius);
    b.pos.y = clampf(target.y, b.radius, size_.y - b.radius);
    b.vel = {0.f, 0.f};
}

void World::releaseHeld(sf::Vector2f throwVel) {
    if (grabbed_ == Grabbed::Field) {
        field_[heldIndex_].held = false;
        grabbed_ = Grabbed::None;
        heldIndex_ = -1;
        return;
    }
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
    if (grabbed_ == Grabbed::Field) {
        field_[heldIndex_].held = false;
    } else if (grabbed_ == Grabbed::Ball) {
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

    const float sp = length(b.vel);
    if (sp > 1e-3f) {
        float ang = std::atan2(b.vel.y, b.vel.x) +
                    rng_.range(-cfg::ball::bounceAngleJitter, cfg::ball::bounceAngleJitter);
        sf::Vector2f d{std::cos(ang), std::sin(ang)};
        const float f = cfg::ball::minAxisFraction;
        if (std::fabs(d.x) < f) d.x = std::copysign(f, d.x == 0.f ? rng_.unit() : d.x);
        if (std::fabs(d.y) < f) d.y = std::copysign(f, d.y == 0.f ? rng_.unit() : d.y);
        b.vel = normalized(d) * sp;
    }

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

void World::applyFieldForce(Ball& b, float dt) const {
    sf::Vector2f acc{0.f, 0.f};
    for (const FieldObject& f : field_) {
        if (f.held) continue;
        const sf::Vector2f d = f.pos - b.pos;
        const float dist = length(d);
        if (dist > f.radius || dist < 1e-3f) continue;
        const float dd = std::max(dist, cfg::field::minDist);
        float a = cfg::field::blackHoleStrength * f.strength / (dd * dd);
        a = std::min(a, cfg::field::maxAccel);
        acc += (d / dist) * a;
    }
    b.vel += acc * dt;
}

void World::applyHoming(Ball& b, float dt) const {
    float bestDist2 = cfg::combat::homingRange * cfg::combat::homingRange;
    const Enemy* target = nullptr;
    for (const Enemy& e : enemies_) {
        const sf::Vector2f d = e.pos - b.pos;
        const float d2 = dot(d, d);
        if (d2 < bestDist2) {
            bestDist2 = d2;
            target = &e;
        }
    }
    if (!target) return;
    b.vel += normalized(target->pos - b.pos) * (cfg::combat::homingAccel * dt);
}

void World::regulateSpeed(Ball& b, float dt, const WorldParams& p) {
    const float cruiseS = cruiseSpeed(p);
    const float vMax = maxSpeed(p);
    const bool slow = effect_ && effect_->kind == PowerUp::SlowMo;

    const float sp = length(b.vel);
    if (sp < 1e-3f) {
        b.vel = rng_.direction() * cruiseS;
        return;
    }
    const float up = 1.f - std::exp(-cfg::ball::regainRate * dt);
    const float down =
        1.f - std::exp((slow ? -cfg::ball::decayRateSlowMo : -cfg::ball::decayRate) * dt);
    const float k = (sp < cruiseS) ? up : down;
    const float ns = std::min(lerpf(sp, cruiseS, k), vMax);
    b.vel *= ns / sp;
}

void World::updateTrail(Ball& b) {
    b.trail.push_back(b.pos);
    const std::size_t cap =
        static_cast<std::size_t>(clampf(4.f + length(b.vel) / 90.f, 4.f, 16.f));
    while (b.trail.size() > cap) b.trail.pop_front();
}

void World::advanceBall(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    applyFieldForce(b, dt);
    applyHoming(b, dt);

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
        // The core is not solid to balls: they flow through the middle where the
        // enemies pile up. Enemies still damage the core on contact.
        for (Enemy& e : enemies_) {
            collision::Contact c =
                collision::circleVsSolidCircle(b, e.pos, e.radius, cfg::combat::hitRebound);
            if (!c.hit) continue;
            e.hp -= ballDamage(b, p);
            e.hitFlash = 1.f;
            e.vel += -c.normal * cfg::combat::knockback;
            afterBounce(b, c.normal, true);
            pushFx(c);
        }
    }

    regulateSpeed(b, dt, p);
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

void World::updateEnemies(float dt, FrameEvents& ev) {
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        const sf::Vector2f d = core_.pos - it->pos;
        const float dist = length(d);
        const sf::Vector2f steer = (dist > 1e-3f ? d / dist : sf::Vector2f{0.f, 1.f}) * it->speed;
        it->vel += (steer - it->vel) * (1.f - std::exp(-8.f * dt));
        it->pos += it->vel * dt;
        it->hitFlash *= std::exp(-6.f * dt);

        if (dist <= core_.radius + it->radius) {
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
    sweepDeadEnemies(p, ev);
    updateEnemies(dt, ev);
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
