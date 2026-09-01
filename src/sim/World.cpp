#include "sim/World.hpp"

#include <algorithm>
#include <cmath>

#include "sim/Collision.hpp"

namespace sb {

namespace {

Wall defaultWall(int i, sf::Vector2f s) {
    const float cx = s.x * 0.5f;
    const float cy = s.y * 0.5f;
    switch (i % cfg::wall::maxWalls) {
        case 0: return {{s.x * 0.30f, cy}, {7.f, s.y * 0.24f}};
        case 1: return {{s.x * 0.70f, cy}, {7.f, s.y * 0.24f}};
        case 2: return {{cx, s.y * 0.28f}, {s.x * 0.20f, 7.f}};
        case 3: return {{cx, s.y * 0.72f}, {s.x * 0.20f, 7.f}};
        case 4: return {{cx, cy}, {32.f, 32.f}};
        default: return {{cx, cy}, {58.f, 12.f}};
    }
}

float luckMultiplier(int luckLevel) {
    return std::pow(cfg::pickup::luckFactor, static_cast<float>(luckLevel));
}

}  // namespace

World::World(sf::Vector2f size) : size_(size) {}

// ---------------------------------------------------------------- speeds

float World::cruiseBase(const WorldParams& p) const {
    return cfg::ball::baseCruise *
           std::pow(cfg::ball::cruiseGrowth, static_cast<float>(p.speedLevel));
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

// ---------------------------------------------------------------- walls

void World::configureWalls(int wallLevel, const std::vector<WallSnapshot>& saved) {
    walls_.clear();
    const int want = std::clamp(wallLevel, 0, cfg::wall::maxWalls);
    for (int i = 0; i < want; ++i) {
        if (i < static_cast<int>(saved.size()) && saved[i].hx > 0.f && saved[i].hy > 0.f) {
            const WallSnapshot& s = saved[i];
            walls_.push_back(Wall{{s.cx, s.cy}, {s.hx, s.hy}, {s.vx, s.vy}});
        } else {
            walls_.push_back(defaultWall(i, size_));
        }
    }
}

void World::syncWallCount(int wallLevel) {
    const int want = std::clamp(wallLevel, 0, cfg::wall::maxWalls);
    while (static_cast<int>(walls_.size()) < want)
        walls_.push_back(defaultWall(static_cast<int>(walls_.size()), size_));
    while (static_cast<int>(walls_.size()) > want) walls_.pop_back();
}

std::vector<WallSnapshot> World::wallSnapshot() const {
    std::vector<WallSnapshot> out;
    out.reserve(walls_.size());
    for (const Wall& w : walls_)
        out.push_back({w.pos.x, w.pos.y, w.half.x, w.half.y, w.vel.x, w.vel.y});
    return out;
}

bool World::wallHit(const Wall& w, sf::Vector2f point) const {
    return std::fabs(point.x - w.pos.x) <= w.half.x + cfg::wall::grabPadding &&
           std::fabs(point.y - w.pos.y) <= w.half.y + cfg::wall::grabPadding;
}

void World::updateWalls(float dt) {
    for (Wall& w : walls_) {
        if (w.held || !w.drifting()) continue;
        const float s = length(w.vel);
        if (s > cfg::wall::driftCap) w.vel *= cfg::wall::driftCap / s;
        w.pos += w.vel * dt;
        if (w.pos.x - w.half.x < 0.f) { w.pos.x = w.half.x; w.vel.x = std::fabs(w.vel.x); }
        else if (w.pos.x + w.half.x > size_.x) { w.pos.x = size_.x - w.half.x; w.vel.x = -std::fabs(w.vel.x); }
        if (w.pos.y - w.half.y < 0.f) { w.pos.y = w.half.y; w.vel.y = std::fabs(w.vel.y); }
        else if (w.pos.y + w.half.y > size_.y) { w.pos.y = size_.y - w.half.y; w.vel.y = -std::fabs(w.vel.y); }
    }
}

// ---------------------------------------------------------------- lifecycle

void World::spawnBall(const WorldParams& p) {
    Ball b;
    const sf::Vector2f jitter{rng_.range(-40.f, 40.f), rng_.range(-40.f, 40.f)};
    b.pos = size_ * 0.5f + jitter;
    b.vel = rng_.direction() * cruiseBase(p);
    b.color = theme::speedColor(length(b.vel), cruiseBase(p));
    balls_.push_back(b);
}

void World::setMultiball(int multiballLevel, const WorldParams& p) {
    const int want = std::min(1 + std::max(0, multiballLevel), cfg::ball::maxBalls);
    while (static_cast<int>(balls_.size()) < want) spawnBall(p);
}

void World::reset(int multiballLevel, const WorldParams& p,
                  const std::vector<WallSnapshot>& walls) {
    balls_.clear();
    pickups_.clear();
    effect_.reset();
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
    comboStreak_ = 0;
    comboCapTier_ = cfg::combo::baseCapTier + p.comboLevel;
    reportedTier_ = 0;
    sinceBounce_ = 0.f;
    pickupTimer_ =
        rng_.range(cfg::pickup::firstSpawnMin, cfg::pickup::firstSpawnMax) * luckMultiplier(p.luckLevel);
    configureWalls(p.wallLevel, walls);
    setMultiball(multiballLevel, p);
}

// ---------------------------------------------------------------- grab / throw

bool World::grabAt(sf::Vector2f point, float catchRadius) {
    // Walls take priority (larger, visually on top). Search topmost first.
    for (int i = static_cast<int>(walls_.size()) - 1; i >= 0; --i) {
        if (wallHit(walls_[i], point)) {
            grabbed_ = Grabbed::Wall;
            heldIndex_ = i;
            walls_[i].held = true;
            walls_[i].vel = {0.f, 0.f};
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
    if (grabbed_ == Grabbed::Wall) {
        Wall& w = walls_[heldIndex_];
        w.pos.x = clampf(target.x, w.half.x, size_.x - w.half.x);
        w.pos.y = clampf(target.y, w.half.y, size_.y - w.half.y);
        w.vel = {0.f, 0.f};
        return;
    }
    if (grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[heldIndex_];
    b.pos.x = clampf(target.x, b.radius, size_.x - b.radius);
    b.pos.y = clampf(target.y, b.radius, size_.y - b.radius);
    b.vel = {0.f, 0.f};
    for (const Wall& w : walls_) collision::pushOutOfWall(b, w);
}

void World::releaseHeld(sf::Vector2f throwVel) {
    const float s = length(throwVel);
    if (grabbed_ == Grabbed::Wall) {
        Wall& w = walls_[heldIndex_];
        w.held = false;
        w.vel = (s < cfg::wall::minThrowToDrift)
                    ? sf::Vector2f{0.f, 0.f}
                    : normalized(throwVel) * std::min(s * cfg::wall::throwToDrift, cfg::wall::driftCap);
        grabbed_ = Grabbed::None;
        heldIndex_ = -1;
        return;
    }
    if (grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[heldIndex_];
    b.held = false;
    if (s < cfg::ball::minThrowSpeed) b.vel = rng_.direction() * cfg::ball::nudgeSpeed;
    else if (s > cfg::ball::hardSpeedCap) b.vel = throwVel * (cfg::ball::hardSpeedCap / s);
    else b.vel = throwVel;
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

void World::forceRelease() {
    if (grabbed_ == Grabbed::Wall) {
        walls_[heldIndex_].held = false;
    } else if (grabbed_ == Grabbed::Ball) {
        Ball& b = balls_[heldIndex_];
        b.held = false;
        b.vel = rng_.direction() * cfg::ball::forceReleaseSpeed;
    }
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

bool World::toggleDriftAt(sf::Vector2f point) {
    for (int i = static_cast<int>(walls_.size()) - 1; i >= 0; --i) {
        Wall& w = walls_[i];
        if (w.held || !wallHit(w, point)) continue;
        w.vel = w.drifting() ? sf::Vector2f{0.f, 0.f}
                             : rng_.direction() * cfg::wall::rightClickDrift;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------- per-step parts

void World::advanceCombo(float dt, const WorldParams& p) {
    comboCapTier_ = cfg::combo::baseCapTier + p.comboLevel;
    const float window =
        cfg::combo::decayWindow + cfg::combo::decayWindowPerLevel * static_cast<float>(p.comboLevel);
    sinceBounce_ += dt;
    if (sinceBounce_ > window && comboStreak_ > 0) {
        comboStreak_ = std::max(0, comboStreak_ - cfg::combo::bouncesPerTier);
        sinceBounce_ = 0.f;
    }
}

void World::afterBounce(Ball& b, sf::Vector2f normal) {
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

    ++comboStreak_;
    sinceBounce_ = 0.f;
}

int World::awardPoints(const WorldParams& p) const {
    const int per = 1 + p.pointsLevel;
    const int tier = std::min(comboStreak_ / cfg::combo::bouncesPerTier, comboCapTier_);
    int gained = static_cast<int>(
        std::lround(per * (1.f + tier * cfg::combo::multiplierPerTier)));
    if (effect_) {
        if (effect_->kind == PowerUp::Points2x) gained *= cfg::powerup::points2xMul;
        else if (effect_->kind == PowerUp::Frenzy) gained *= cfg::powerup::frenzyMul;
        else if (effect_->kind == PowerUp::Golden)
            gained += cfg::powerup::goldenBonusBase +
                      cfg::powerup::goldenBonusPerPoints * p.pointsLevel;
    }
    return gained;
}

void World::regulateSpeed(Ball& b, float dt, const WorldParams& p) {
    // Cruise is a floor the ball climbs back to quickly and a target it eases
    // down to slowly, so a throw stays fast for seconds.
    const float cruiseS = cruiseSpeed(p);
    const float vMax = maxSpeed(p);
    const bool slow = effect_ && effect_->kind == PowerUp::SlowMo;

    const float sp = length(b.vel);
    if (sp < 1e-3f) {
        b.vel = rng_.direction() * cruiseS;
        return;
    }
    const float up = 1.f - std::exp(-cfg::ball::regainRate * dt);
    const float down = 1.f - std::exp((slow ? -cfg::ball::decayRateSlowMo : -cfg::ball::decayRate) * dt);
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
    const bool phasing = effect_ && effect_->kind == PowerUp::Ghost;

    const float speed = length(b.vel);
    const int steps = std::clamp(
        static_cast<int>(std::ceil(speed * dt / (b.radius * cfg::ball::substepPerRadius))),
        1, cfg::ball::maxSubsteps);
    const float h = dt / static_cast<float>(steps);

    auto record = [&](const collision::Contact& c) {
        afterBounce(b, c.normal);
        BounceFx fx;
        fx.normal = c.normal;
        fx.pos = c.point;
        fx.color = b.color;
        fx.speed = length(b.vel);
        ev.bounces.push_back(fx);
        ev.pointsGained += awardPoints(p);
    };

    for (int s = 0; s < steps; ++s) {
        b.pos += b.vel * h;
        if (collision::Contact c = collision::circleVsBounds(b, size_); c.hit) record(c);
        if (!phasing)
            for (const Wall& w : walls_)
                if (collision::Contact c = collision::circleVsWall(b, w); c.hit) record(c);
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

void World::updatePickups(float dt, const WorldParams& p, FrameEvents& ev) {
    const float luck = luckMultiplier(p.luckLevel);

    if (!effect_ && pickups_.empty()) {
        pickupTimer_ -= dt;
        if (pickupTimer_ <= 0.f) {
            Pickup pu;
            pu.kind = static_cast<PowerUp>(rng_.irange(0, kPowerUpCount - 1));
            pu.pos = {rng_.range(size_.x * 0.15f, size_.x * 0.85f),
                      rng_.range(size_.y * 0.15f, size_.y * 0.85f)};
            pu.vel = rng_.direction() * rng_.range(cfg::pickup::driftMin, cfg::pickup::driftMax);
            pickups_.push_back(pu);
            pickupTimer_ = rng_.range(cfg::pickup::spawnMin, cfg::pickup::spawnMax) * luck;
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
                pickupTimer_ = rng_.range(cfg::pickup::spawnMin, cfg::pickup::spawnMax) * luck;
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
    if (dt <= 0.f) return ev;

    advanceCombo(dt, p);
    updateWalls(dt);

    for (std::size_t i = 0; i < balls_.size(); ++i) {
        Ball& b = balls_[i];
        if (grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_) {
            b.squash *= std::exp(-cfg::ball::squashDecay * dt);
            continue;
        }
        advanceBall(b, dt, p, ev);
    }

    resolveBallPairs();
    updatePickups(dt, p, ev);
    advanceEffect(dt);

    const int tier = comboTier();
    ev.comboTier = tier;
    if (tier > reportedTier_) ev.comboTierUp = true;
    reportedTier_ = tier;
    return ev;
}

}  // namespace sb
