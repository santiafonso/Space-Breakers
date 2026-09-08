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

}  // namespace

World::World(sf::Vector2f size) : baseSize_(size), size_(size) { core_.pos = size_ * 0.5f; }

// ---------------------------------------------------------------- speeds

float World::cruiseBase(const WorldParams& p) const {
    return cfg::ball::baseCruise * p.cruiseMult;
}

float World::cruiseSpeed(const WorldParams& p) const {
    float c = cruiseBase(p);
    if (effect_ && effect_->kind == PowerUp::Surge) c *= cfg::powerup::surgeCruiseMul;
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
    b.radius = cfg::ball::radius * p.ballRadiusMult;
    const float a = rng_.range(0.f, 2.f * kPi);
    b.pos = core_.pos + sf::Vector2f{std::cos(a), std::sin(a)} * (core_.radius + b.radius + 20.f);
    b.vel = rng_.direction() * cruiseBase(p);  // straight line, random heading
    b.color = theme::speedColor(length(b.vel), cruiseBase(p));
    b.cooldown = rng_.range(0.f, 0.6f);
    balls_.push_back(b);
}

void World::addBall(Element e, const WorldParams& p) {
    if (static_cast<int>(balls_.size()) >= cfg::ball::maxBalls) return;
    spawnBall(e, p);
}

void World::convertOneBall(Element from, Element to) {
    for (Ball& b : balls_) {
        if (b.element == from) {
            b.element = to;
            b.cooldown = 0.25f;
            return;
        }
    }
}

void World::repairCore(float amount) {
    core_.hp = std::min(core_.maxHp, core_.hp + amount);
}

void World::addCoreMaxHp(float delta) {
    core_.maxHp += delta;
    core_.hp = std::min(core_.maxHp, core_.hp + delta);
}

void World::devWinWave() {
    toSpawn_ = 0;
    enemies_.clear();
    projectiles_.clear();
    if (bossWave_ && boss_.alive) boss_.hp = 0.f;  // updateBoss clears it -> waveCleared
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
    invuln_ = false;
    bossWave_ = false;
    boss_ = Boss{};
    coreSlideT_ = 0.f;

    size_ = baseSize_;
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

void World::carryBalls(const WorldParams& p) {
    // A wave change no longer teleports the balls: they stay exactly where the
    // last wave left them, keeping their heading. Only a held ball is let go and
    // any ball that had stopped is woken back up to cruise speed.
    for (Ball& b : balls_) {
        b.held = false;
        if (length(b.vel) < cfg::ball::minThrowSpeed)
            b.vel = rng_.direction() * cruiseBase(p);
    }
    heldGrabOffset_ = {0.f, 0.f};
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

void World::startWave(int wave, const WorldParams& p) {
    bossWave_ = false;
    boss_ = Boss{};
    coreSlideT_ = 0.f;
    size_ = baseSize_;
    core_.pos = size_ * 0.5f;

    wave_ = wave;
    toSpawn_ = waveEnemyCount(wave);
    spawnTimer_ = cfg::wave::introDelay;
    waveRunning_ = true;
    projectiles_.clear();
    carryBalls(p);
}

// The pulled-back arena, sized to the base view's aspect ratio so the zoomed-out
// camera frames it with no head-room (balls bounce at the visible screen edge,
// not an invisible wall part-way up). Used from the boss wave onward.
sf::Vector2f World::wideArenaSize() const {
    sf::Vector2f s = {baseSize_.x * cfg::boss::arenaScaleX, baseSize_.y * cfg::boss::arenaScaleY};
    const float viewRatio = baseSize_.x / baseSize_.y;
    if (s.x / s.y > viewRatio) s.y = s.x / viewRatio;
    else                       s.x = s.y * viewRatio;
    return s;
}

// Waves 11..20: same wide arena and pulled-back camera as the boss, but a normal
// (hard) wave. The core eases from the boss's far-left spot back to the centre.
void World::startPostBossWave(int wave, const WorldParams& p) {
    bossWave_ = false;
    boss_ = Boss{};
    size_ = wideArenaSize();

    coreSlideFrom_ = core_.pos;          // wherever the boss wave left it (far left)
    coreSlideTo_ = size_ * 0.5f;
    coreSlideT_ = cfg::run::coreSlideTime;

    wave_ = wave;
    toSpawn_ = waveEnemyCount(wave);
    spawnTimer_ = cfg::wave::introDelay;
    waveRunning_ = true;
    projectiles_.clear();
    enemies_.clear();
    carryBalls(p);
}

void World::updateCoreSlide(float dt) {
    if (coreSlideT_ <= 0.f) return;
    coreSlideT_ = std::max(0.f, coreSlideT_ - dt);
    float u = 1.f - coreSlideT_ / cfg::run::coreSlideTime;   // 0 -> 1
    u = u * u * (3.f - 2.f * u);                             // smoothstep
    core_.pos = coreSlideFrom_ + (coreSlideTo_ - coreSlideFrom_) * u;
}

void World::startBossWave(const WorldParams& p) {
    bossWave_ = true;
    wave_ = cfg::run::bossWave;
    waveRunning_ = true;
    toSpawn_ = 0;
    spawnTimer_ = 1.0f;
    enemies_.clear();
    projectiles_.clear();
    coreSlideT_ = 0.f;

    // Wider arena, core shoved to the far left.
    size_ = wideArenaSize();
    core_.pos = {core_.radius + cfg::boss::coreMarginX, size_.y * 0.5f};

    boss_ = Boss{};
    boss_.alive = true;
    boss_.kind = BossKind::Charger;
    boss_.hp = boss_.maxHp = cfg::boss::hp;
    boss_.pos = {size_.x - boss_.radius - 4.f, size_.y * 0.5f};
    boss_.vel = {-cfg::boss::speed, 0.f};

    carryBalls(p);
}

// Wave 20: the Orbital boss. Wide arena, core centred (it is already there from
// waves 11-19). The boss starts near the arena edge and spirals inward; a ring
// of shield enemies spins around it and is topped up while it lives.
void World::startFinalBossWave(const WorldParams& p) {
    bossWave_ = true;
    wave_ = cfg::run::finalWave;
    waveRunning_ = true;
    toSpawn_ = 0;
    spawnTimer_ = cfg::finalBoss::addInterval;   // first edge add after a short beat
    enemies_.clear();
    projectiles_.clear();
    coreSlideT_ = 0.f;

    size_ = wideArenaSize();
    core_.pos = size_ * 0.5f;

    boss_ = Boss{};
    boss_.alive = true;
    boss_.kind = BossKind::Orbital;
    boss_.radius = cfg::finalBoss::radius;
    boss_.hp = boss_.maxHp = cfg::finalBoss::hp;

    // Spiral has to fit inside the arena, so cap the start radius on the shorter
    // axis. The boss slides in from off the left edge to that spiral-start point
    // during the intro, then winds inward - so the player sees where it comes
    // from and reads the path (renderer draws a guide arc).
    const float maxDist = std::min(size_.x, size_.y) * 0.5f - boss_.radius - 40.f;
    boss_.dist = std::min(cfg::finalBoss::spiralStartDist, maxDist);
    boss_.ang = cfg::finalBoss::startAngle;
    boss_.pos = {-boss_.radius, core_.pos.y};   // just off the left edge
    boss_.intro = cfg::finalBoss::introTime;
    boss_.shieldTimer = cfg::finalBoss::shieldRespawn;

    for (int i = 0; i < cfg::finalBoss::shieldCount; ++i)
        spawnOrbiter(static_cast<float>(i) / cfg::finalBoss::shieldCount * 2.f * kPi);

    carryBalls(p);
}

void World::spawnOrbiter(float phase) {
    Enemy e;
    e.orbiter = true;
    e.orbitPhase = phase;
    e.radius = cfg::wave::enemyRadius;
    e.maxHp = e.hp = cfg::finalBoss::shieldHp;
    e.speed = 0.f;
    const float a = boss_.ringAng + phase;
    e.pos = boss_.pos + sf::Vector2f{std::cos(a), std::sin(a)} * cfg::finalBoss::shieldRadius;
    enemies_.push_back(e);
}

void World::spawnEnemy() {
    const float r = cfg::wave::enemyRadius;
    const bool chargerWave = bossWave_ && boss_.kind == BossKind::Charger;
    const bool orbitalWave = bossWave_ && boss_.kind == BossKind::Orbital;

    sf::Vector2f pos;
    if (chargerWave) {
        // Charger: adds only from the right half - never from behind the far-left
        // core or the flanks near it.
        const float xLo = size_.x * 0.5f;
        switch (rng_.irange(0, 2)) {
            case 0:  pos = {rng_.range(xLo, size_.x), -r}; break;            // top, right half
            case 1:  pos = {rng_.range(xLo, size_.x), size_.y + r}; break;   // bottom, right half
            default: pos = {size_.x + r, rng_.range(0.f, size_.y)}; break;   // right edge
        }
    } else {
        // Normal waves and the Orbital boss: any of the four screen edges.
        switch (rng_.irange(0, 3)) {
            case 0:  pos = {rng_.range(0.f, size_.x), -r}; break;
            case 1:  pos = {rng_.range(0.f, size_.x), size_.y + r}; break;
            case 2:  pos = {-r, rng_.range(0.f, size_.y)}; break;
            default: pos = {size_.x + r, rng_.range(0.f, size_.y)}; break;
        }
    }

    Enemy e;
    e.pos = pos;
    e.radius = cfg::wave::enemyRadius;
    if (orbitalWave) {   // softer than a plain wave-20 enemy - the shield is the fight
        e.maxHp = e.hp = cfg::finalBoss::addHp;
        e.speed = cfg::finalBoss::addSpeed;
    } else {
        e.maxHp = e.hp = waveEnemyHp(wave_);
        e.speed = waveEnemySpeed(wave_);
    }
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
    // Remember where the ball sat relative to the cursor so it doesn't snap to
    // the pointer on grab - moveHeld eases this offset out.
    heldGrabOffset_ = b.pos - point;
    b.held = true;
    b.vel = {0.f, 0.f};
    b.trail.clear();
    return true;
}

void World::moveHeld(sf::Vector2f target, float dt) {
    if (grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[heldIndex_];
    heldGrabOffset_ *= std::exp(-cfg::app::grabSettle * dt);
    if (length(heldGrabOffset_) < 1.f) heldGrabOffset_ = {0.f, 0.f};
    const sf::Vector2f p = target + heldGrabOffset_;
    b.pos.x = clampf(p.x, b.radius, size_.x - b.radius);
    b.pos.y = clampf(p.y, b.radius, size_.y - b.radius);
    b.vel = {0.f, 0.f};
}

void World::releaseHeld(sf::Vector2f throwVel) {
    if (grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[heldIndex_];
    b.held = false;
    heldGrabOffset_ = {0.f, 0.f};
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
    heldGrabOffset_ = {0.f, 0.f};
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

    // Nudge the reflected heading and keep it off the axes so the ball never
    // settles into a flat horizontal / vertical ping-pong.
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
        comboStreak_ += (effect_ && effect_->kind == PowerUp::Golden)
                            ? cfg::powerup::goldenComboRate : 1;   // GOLDEN BOUNCE climbs faster
        sinceHit_ = 0.f;
    }
}

float World::ballDamage(const Ball& b, const WorldParams& p) const {
    const float ratio = length(b.vel) / cfg::ball::baseCruise;
    float dmg = (cfg::combat::contactDamageBase + cfg::combat::contactDamagePerCruise * ratio) *
                comboMultiplier() * p.damageMult;
    if (effect_ && effect_->kind == PowerUp::Overdrive) dmg *= cfg::powerup::overdriveDamageMul;
    return dmg;
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

void World::regulateSpeed(Ball& b, float dt, const WorldParams& p) {
    // Cruise is a floor the ball climbs back to fast and a target it eases down
    // to slowly, so a fling stays fast for a moment.
    const float cruiseS = cruiseSpeed(p);
    const float vMax = maxSpeed(p);

    const float sp = length(b.vel);
    if (sp < 1e-3f) {
        b.vel = rng_.direction() * cruiseS;
        return;
    }
    const float up = 1.f - std::exp(-cfg::ball::regainRate * dt);
    const float decayRate = cfg::ball::decayRate * p.flingDecayMult;
    const float down = 1.f - std::exp(-decayRate * dt);
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
    b.radius = cfg::ball::radius * p.ballRadiusMult;  // "Big ball" upgrade

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
        // The core is solid: balls bounce off it (no damage to the core).
        if (collision::Contact c =
                collision::circleVsSolidCircle(b, core_.pos, core_.radius, 1.f);
            c.hit) {
            afterBounce(b, c.normal, false);
            if (p.coreBounceBoost > 1.f) {  // "Spring core" upgrade
                const float sp = length(b.vel);
                if (sp > 1e-3f)
                    b.vel *= std::min(sp * p.coreBounceBoost, maxSpeed(p)) / sp;
            }
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

        // The boss is solid. Balls always bounce off it, but damage only lands
        // when it isn't in i-frames, isn't still sliding in (intro), and the
        // ball is genuinely moving - so a ball wedged in the ring can't melt it
        // in place, and neither can dropping a ball on it and spam-clicking.
        if (boss_.alive) {
            if (collision::Contact c = collision::circleVsSolidCircle(
                    b, boss_.pos, boss_.radius, cfg::combat::hitRebound);
                c.hit) {
                if (boss_.intro <= 0.f && boss_.hitCd <= 0.f &&
                    length(b.vel) >= cruiseBase(p) * cfg::boss::minHitCruiseFrac) {
                    boss_.hp -= ballDamage(b, p);
                    boss_.hitFlash = 1.f;
                    boss_.hitCd = cfg::boss::hitCooldown;
                    ev.bossHit = true;
                }
                afterBounce(b, c.normal, true);
                pushFx(c);
            }
        }
    }

    emitElement(b, dt, p);
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
        if (!hit && boss_.alive && length(boss_.pos - it->pos) < boss_.radius + 4.f) {
            boss_.hp -= it->damage;
            boss_.hitFlash = 1.f;
            hit = true;
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

void World::updateEnemies(float dt, const WorldParams& p, FrameEvents& ev) {
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        Enemy& e = *it;

        if (e.burn > 0.f) {
            e.burn -= dt;
            e.hp -= e.burnDps * dt;
        }

        // Wave-20 shield orbiters are locked rigidly onto the spinning ring
        // around the boss - position set outright every frame so the ring stays
        // centred on the boss no matter how fast it moves (no trailing lag).
        // They only block balls (hits still damage them) and never touch the
        // core; only the boss reaching the core loses. Once the boss dies they
        // drop orbiter and become ordinary core-seekers (handled below).
        if (e.orbiter && boss_.alive) {
            const float a = boss_.ringAng + e.orbitPhase;
            const sf::Vector2f slot =
                boss_.pos + sf::Vector2f{std::cos(a), std::sin(a)} * cfg::finalBoss::shieldRadius;
            e.vel = (slot - e.pos) / std::max(dt, 1e-4f);   // this frame's motion, for fx
            e.pos = slot;
            e.hitFlash *= std::exp(-6.f * dt);
            ++it;
            continue;
        }

        {
            const sf::Vector2f d = core_.pos - e.pos;
            const float dl = length(d);
            const sf::Vector2f steer = (dl > 1e-3f ? d / dl : sf::Vector2f{0.f, 1.f}) * e.speed;

            // SLOW MOTION drags every enemy; the "Slow field" item drags only
            // those close to the core. Both just scale this enemy's time step.
            float edt = dt;
            if (effect_ && effect_->kind == PowerUp::SlowMo) edt *= cfg::powerup::slowMoEnemyMul;
            if (p.slowField && dl < cfg::combat::slowFieldRadius) edt *= cfg::combat::slowFieldMul;

            e.vel += (steer - e.vel) * (1.f - std::exp(-8.f * edt));
            e.pos += e.vel * edt;

            // Pushed out of any rubble in the way.
            for (const Obstacle& o : obstacles_) {
                const sf::Vector2f od = e.pos - o.pos;
                const float sum = o.radius + e.radius;
                const float dd = length(od);
                if (dd < sum && dd > 1e-3f) e.pos += (od / dd) * (sum - dd);
            }
        }
        e.hitFlash *= std::exp(-6.f * dt);

        const float dist = length(core_.pos - e.pos);
        if (dist <= core_.radius + e.radius) {
            if (!invuln_) core_.hp -= cfg::core::enemyDamage;
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

void World::sweepDeadEnemies(FrameEvents& ev) {
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        if (it->hp <= 0.f) {
            ev.kills.push_back(it->pos);
            it = enemies_.erase(it);
        } else {
            ++it;
        }
    }
}

void World::updateBoss(float dt, const WorldParams& p, FrameEvents& ev) {
    (void)p;
    if (!bossWave_ || !boss_.alive) return;

    boss_.hitCd = std::max(0.f, boss_.hitCd - dt);
    boss_.hitFlash *= std::exp(-6.f * dt);

    if (boss_.kind == BossKind::Orbital) {
        boss_.ringAng += cfg::finalBoss::shieldOmega * dt;

        if (boss_.intro > 0.f) {
            // Slide in from off the left edge to the spiral-start point.
            boss_.intro = std::max(0.f, boss_.intro - dt);
            float k = 1.f - boss_.intro / cfg::finalBoss::introTime;   // 0 -> 1
            k = k * k * (3.f - 2.f * k);
            const sf::Vector2f entry = {-boss_.radius, core_.pos.y};
            const sf::Vector2f spiralStart =
                core_.pos + sf::Vector2f{std::cos(boss_.ang), std::sin(boss_.ang)} * boss_.dist;
            const sf::Vector2f prev = boss_.pos;
            boss_.pos = entry + (spiralStart - entry) * k;
            boss_.vel = (boss_.pos - prev) / std::max(dt, 1e-4f);
        } else {
            boss_.ang += cfg::finalBoss::spiralOmega * dt;
            boss_.dist = std::max(0.f, boss_.dist - cfg::finalBoss::spiralShrink * dt);
            const sf::Vector2f prev = boss_.pos;
            boss_.pos = core_.pos +
                        sf::Vector2f{std::cos(boss_.ang), std::sin(boss_.ang)} * boss_.dist;
            boss_.vel = (boss_.pos - prev) / std::max(dt, 1e-4f);
        }

        // Keep the shield ring stocked, one orbiter at a time.
        int live = 0;
        for (const Enemy& e : enemies_)
            if (e.orbiter) ++live;
        if (live >= cfg::finalBoss::shieldCount) {
            boss_.shieldTimer = cfg::finalBoss::shieldRespawn;
        } else {
            boss_.shieldTimer -= dt;
            if (boss_.shieldTimer <= 0.f) {
                spawnOrbiter(rng_.range(0.f, 2.f * kPi));
                boss_.shieldTimer = cfg::finalBoss::shieldRespawn;
            }
        }
    } else {
        boss_.pos += boss_.vel * dt;   // Charger: dead straight, no knockback / steering
    }

    if (boss_.hp <= 0.f) {
        boss_.alive = false;
        boss_.hitFlash = 0.f;
        ev.kills.push_back(boss_.pos);
        // The wave is NOT over yet. The Orbital ring breaks loose: its orbiters
        // become ordinary core-seekers, flung outward first so the player gets a
        // beat. updateWaveSpawner ends the wave once every enemy is gone.
        for (Enemy& e : enemies_) {
            if (!e.orbiter) continue;
            e.orbiter = false;
            e.speed = cfg::finalBoss::addSpeed;
            e.vel = normalized(e.pos - boss_.pos, {1.f, 0.f}) * cfg::finalBoss::deathBurst;
        }
        return;
    }

    if (boss_.intro <= 0.f &&
        length(core_.pos - boss_.pos) <= core_.radius + boss_.radius) {
        boss_.alive = false;
        core_.hp = 0.f;
        core_.hitFlash = 1.f;
        ev.coreHit = true;
        runOver_ = true;   // reaching the core loses the run outright
    }
}

void World::updateWaveSpawner(float dt, FrameEvents& ev) {
    if (!waveRunning_) return;

    if (bossWave_) {
        // Both bosses trickle adds from the edges while alive (the Orbital boss
        // also runs its shield ring in updateBoss, which isn't counted here).
        // The wave ends only once the boss is down and every enemy it left
        // behind is cleared.
        if (boss_.alive) {
            spawnTimer_ -= dt;
            if (spawnTimer_ <= 0.f) {
                if (boss_.kind == BossKind::Charger) {
                    if (static_cast<int>(enemies_.size()) < cfg::boss::maxAdds) {
                        spawnEnemy();
                        spawnTimer_ = cfg::boss::addInterval;
                    }
                } else {
                    int adds = 0;
                    for (const Enemy& e : enemies_)
                        if (!e.orbiter) ++adds;
                    if (adds < cfg::finalBoss::addCap) {
                        spawnEnemy();
                        spawnTimer_ = cfg::finalBoss::addInterval;
                    }
                }
            }
        } else if (enemies_.empty()) {
            waveRunning_ = false;
            ev.waveCleared = true;
        }
        return;
    }

    if (toSpawn_ > 0) {
        spawnTimer_ -= dt;
        if (spawnTimer_ <= 0.f) {
            spawnEnemy();
            --toSpawn_;
            // Later waves spawn denser: ease the cadence down toward the final wave.
            const float t = clampf(static_cast<float>(wave_ - 1) /
                                       static_cast<float>(cfg::run::finalWave - 1),
                                   0.f, 1.f);
            spawnTimer_ = lerpf(cfg::wave::spawnInterval, cfg::wave::spawnIntervalMin, t);
        }
    }
    if (toSpawn_ == 0 && enemies_.empty()) {
        waveRunning_ = false;
        ev.waveCleared = true;
    }
}

void World::updatePickups(float dt, const WorldParams& p, FrameEvents& ev) {
    if (!effect_ && pickups_.empty()) {
        pickupTimer_ -= dt;
        if (pickupTimer_ <= 0.f) {
            int enabled[kPowerUpCount];
            int n = 0;
            for (int i = 0; i < kPowerUpCount; ++i)
                if (p.powerUpMask & (1u << i)) enabled[n++] = i;
            if (n > 0) {
                Pickup pu;
                pu.kind = static_cast<PowerUp>(enabled[rng_.irange(0, n - 1)]);
                pu.pos = {rng_.range(size_.x * 0.15f, size_.x * 0.85f),
                          rng_.range(size_.y * 0.15f, size_.y * 0.85f)};
                pu.vel = rng_.direction() * rng_.range(cfg::pickup::driftMin, cfg::pickup::driftMax);
                pickups_.push_back(pu);
            }
            pickupTimer_ = rng_.range(cfg::pickup::spawnMin, cfg::pickup::spawnMax) * p.pickupSpawnMult;
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
                const float dur = powerUpDuration(pu.kind) * p.pickupDurMult;
                effect_ = ActiveEffect{pu.kind, dur, dur};
                ev.gotPickup = true;
                ev.pickupKind = pu.kind;
                pickupTimer_ = rng_.range(cfg::pickup::spawnMin, cfg::pickup::spawnMax) * p.pickupSpawnMult;
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
    updateCoreSlide(dt);

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
    updateEnemies(dt, p, ev);
    sweepDeadEnemies(ev);
    updateBoss(dt, p, ev);
    updateWaveSpawner(dt, ev);
    updatePickups(dt, p, ev);
    advanceEffect(dt);

    const int tier = comboTier();
    ev.comboTier = tier;
    if (tier > reportedTier_) ev.comboTierUp = true;
    reportedTier_ = tier;
    ev.runOver = runOver_;
    return ev;
}

}  // namespace sb
