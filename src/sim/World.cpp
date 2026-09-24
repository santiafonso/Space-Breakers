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
    if (effect_ && effect_->kind == PowerUp::Surge)
        c *= 1.f + (cfg::powerup::surgeCruiseMul - 1.f) * effStrength(p);   // fades out under "Afterglow"
    return c;
}

float World::ballBaseCruise(const Ball& b, const WorldParams& p) const {
    return cruiseBase(p) * b.mods.cruiseMult *
           (b.role == BallRole::Guardian ? cfg::role::guardianCruiseMul : 1.f);
}

float World::ballCruise(const Ball& b, const WorldParams& p) const {
    float c = cruiseSpeed(p) * b.mods.cruiseMult *
              (b.role == BallRole::Guardian ? cfg::role::guardianCruiseMul : 1.f);
    if (b.mods.warmUp) {   // "Warm-up": cruise climbs over the course of a wave
        const float t = clampf(waveClock_ / cfg::combat::warmUpTime, 0.f, 1.f);
        c *= 1.f + t * cfg::combat::warmUpBonus;
    }
    return c;
}

float World::ballMaxSpeed(const Ball& b, const WorldParams& p) const {
    return std::min(cruiseBase(p) * cfg::ball::maxSpeedCruiseMul * b.mods.maxSpeedMult,
                    cfg::ball::hardSpeedCap);
}

float World::ballRadius(const Ball& b, const WorldParams& p) const {
    return cfg::ball::radius * p.ballRadiusMult * b.mods.radiusMult *
           (b.role == BallRole::Guardian ? cfg::role::guardianRadiusMul : 1.f);
}

float World::elemPotency(const Ball& b, const WorldParams& p) const {
    return p.elemMult[static_cast<int>(b.element)] *
           (b.role == BallRole::Support ? cfg::role::supportElemMul : 1.f);
}

void World::boostSpeed(Ball& b, float mult, const WorldParams& p) {
    if (mult <= 1.f) return;
    const float sp = length(b.vel);
    if (sp > 1e-3f) b.vel *= std::min(sp * mult, ballMaxSpeed(b, p)) / sp;
}

float World::fastestBall() const {
    float m = 0.f;
    for (const Ball& b : balls_) m = std::max(m, length(b.vel));
    return m;
}

// ---------------------------------------------------------------- lifecycle

void World::spawnBall(const BallSpec& spec, const WorldParams& p) {
    Ball b;
    b.role = spec.role;
    b.element = spec.element;
    b.mods = spec.mods;
    b.radius = ballRadius(b, p);
    const float a = rng_.range(0.f, 2.f * kPi);
    b.pos = core_.pos + sf::Vector2f{std::cos(a), std::sin(a)} * (core_.radius + b.radius + 20.f);
    b.vel = rng_.direction() * ballBaseCruise(b, p);  // straight line, random heading
    b.color = b.element == Element::Plain
                  ? theme::speedColor(length(b.vel), cruiseBase(p))
                  : theme::elementSpeedColor(elementColor(b.element), length(b.vel), cruiseBase(p));
    b.cooldown = rng_.range(0.f, 0.6f);
    balls_.push_back(b);
}

void World::syncBalls(const std::vector<BallSpec>& specs, const WorldParams& p) {
    const std::size_t n = std::min<std::size_t>(specs.size(), cfg::ball::maxBalls);
    for (std::size_t i = 0; i < n; ++i) {
        if (i >= balls_.size()) {
            spawnBall(specs[i], p);
            continue;
        }
        Ball& b = balls_[i];
        if (b.element != specs[i].element) {
            b.waterTrail.clear();
            b.cooldown = 0.f;
        }
        b.role = specs[i].role;
        b.element = specs[i].element;
        b.mods = specs[i].mods;
        b.radius = ballRadius(b, p);
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
    bolts_.clear();
    if (bossWave_ && boss_.alive) boss_.hp = 0.f;  // updateBoss clears it -> waveCleared
}

void World::startRun(const WorldParams& p, const std::vector<BallSpec>& balls,
                     float coreHp, float coreMaxHp) {
    balls_.clear();
    enemies_.clear();
    bolts_.clear();
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
    waveClock_ = 0.f;
    invuln_ = false;
    aegisChargesLeft_ = 0;
    coreHitThisWave_ = false;
    hasReserve_ = false;
    reserveTimer_ = cfg::powerup::reserveFillTime;
    bossWave_ = false;
    boss_ = Boss{};
    coreSlideT_ = 0.f;

    size_ = baseSize_;
    core_.pos = size_ * 0.5f;
    core_.maxHp = coreMaxHp;
    core_.hp = std::min(coreHp, coreMaxHp);
    core_.hitFlash = 0.f;

    if (balls.empty()) spawnBall(BallSpec{}, p);
    else syncBalls(balls, p);
    pickupTimer_ = rng_.range(cfg::pickup::firstSpawnMin, cfg::pickup::firstSpawnMax);
}

void World::carryBalls(const WorldParams& p) {
    // A wave change no longer teleports the balls: they stay exactly where the
    // last wave left them, keeping their heading. Only a held ball is let go and
    // any ball that had stopped is woken back up to cruise speed.
    for (Ball& b : balls_) {
        b.held = false;
        if (length(b.vel) < cfg::ball::minThrowSpeed)
            b.vel = rng_.direction() * ballBaseCruise(b, p);
    }
    heldGrabOffset_ = {0.f, 0.f};
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
    waveClock_ = 0.f;   // "Warm-up" ramp restarts each wave
    aegisChargesLeft_ = p.aegisHits;   // "Aegis": the shield recharges each wave
    coreHitThisWave_ = false;          // "Interest": track a damage-free wave
}

void World::startWave(int wave, const WorldParams& p, bool elite) {
    bossWave_ = false;
    boss_ = Boss{};
    coreSlideT_ = 0.f;
    size_ = baseSize_;
    core_.pos = size_ * 0.5f;

    wave_ = wave;
    toSpawn_ = waveEnemyCount(wave);
    if (elite) toSpawn_ = static_cast<int>(std::lround(static_cast<float>(toSpawn_) * cfg::map::eliteCountMul));
    waveHpMul_ = elite ? cfg::map::eliteHpMul : 1.f;
    eliteWave_ = elite;
    spawnTimer_ = cfg::wave::introDelay;
    waveRunning_ = true;
    bolts_.clear();
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
void World::startPostBossWave(int wave, const WorldParams& p, bool elite) {
    bossWave_ = false;
    boss_ = Boss{};
    size_ = wideArenaSize();

    coreSlideFrom_ = core_.pos;          // wherever the boss wave left it (far left)
    coreSlideTo_ = size_ * 0.5f;
    coreSlideT_ = cfg::run::coreSlideTime;

    wave_ = wave;
    toSpawn_ = waveEnemyCount(wave);
    if (elite) toSpawn_ = static_cast<int>(std::lround(static_cast<float>(toSpawn_) * cfg::map::eliteCountMul));
    waveHpMul_ = elite ? cfg::map::eliteHpMul : 1.f;
    eliteWave_ = elite;
    spawnTimer_ = cfg::wave::introDelay;
    waveRunning_ = true;
    bolts_.clear();
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
    waveHpMul_ = 1.f;
    eliteWave_ = false;
    wave_ = cfg::run::bossWave;
    waveRunning_ = true;
    toSpawn_ = 0;
    spawnTimer_ = 1.0f;
    enemies_.clear();
    bolts_.clear();
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
    waveHpMul_ = 1.f;
    eliteWave_ = false;
    wave_ = cfg::run::finalWave;
    waveRunning_ = true;
    toSpawn_ = 0;
    spawnTimer_ = cfg::finalBoss::addInterval;   // first edge add after a short beat
    enemies_.clear();
    bolts_.clear();
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

EnemyKind World::rollEnemyKind() {
    namespace E = cfg::enemy;
    const int w = wave_;
    struct Wt { EnemyKind k; int w; };
    const Wt table[] = {
        {EnemyKind::Grunt,    E::wGrunt},
        {EnemyKind::Runner,   w >= E::runnerWave ? E::wRunner : 0},
        {EnemyKind::Splitter, w >= E::splitterWave ? E::wSplitter : 0},
        {EnemyKind::Tank,     w >= E::tankWave ? E::wTank + (eliteWave_ ? E::eliteTankBonus : 0) : 0},
        {EnemyKind::Shielded, w >= E::shieldWave ? E::wShield + (eliteWave_ ? E::eliteShieldBonus : 0) : 0},
    };
    int total = 0;
    for (const Wt& t : table) total += t.w;
    int pick = rng_.irange(0, total - 1);
    for (const Wt& t : table) {
        if (pick < t.w) return t.k;
        pick -= t.w;
    }
    return EnemyKind::Grunt;
}

// Stamp a kind's shape onto an enemy built from the wave's plain hp / speed.
void World::setEnemyKind(Enemy& e, EnemyKind k, float hp, float speed) {
    namespace E = cfg::enemy;
    float hpMul = 1.f, spMul = 1.f, rMul = 1.f;
    e.kind = k;
    e.knockTaken = 1.f;
    e.coreDamage = cfg::core::enemyDamage;
    switch (k) {
        case EnemyKind::Grunt: break;
        case EnemyKind::Runner:   hpMul = E::runnerHp; spMul = E::runnerSpeed; rMul = E::runnerRadius; break;
        case EnemyKind::Tank:
            hpMul = E::tankHp; spMul = E::tankSpeed; rMul = E::tankRadius;
            e.knockTaken = E::tankKnock;
            e.coreDamage = cfg::core::enemyDamage * E::tankCoreDamage;
            break;
        case EnemyKind::Splitter: hpMul = E::splitterHp; break;
        case EnemyKind::Shard:    hpMul = E::shardHp; spMul = E::shardSpeed; rMul = E::shardRadius; break;
        case EnemyKind::Shielded: hpMul = E::shieldHp; break;
    }
    e.maxHp = e.hp = hp * hpMul;
    e.speed = speed * spMul;
    e.radius = cfg::wave::enemyRadius * rMul;
}

// A Shielded enemy's shield faces the core: a ball arriving from within the
// arc around "straight toward the core" bounces off without doing damage.
bool World::shieldBlocks(const Enemy& e, sf::Vector2f from, sf::Vector2f corePos) {
    if (e.kind != EnemyKind::Shielded || e.frozen > 0.f) return false;   // a frozen one can't guard
    const sf::Vector2f facing = normalized(corePos - e.pos, {1.f, 0.f});
    const sf::Vector2f dir = normalized(from - e.pos, {0.f, 0.f});
    return dot(facing, dir) > std::cos(cfg::enemy::shieldArc);
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
    } else if (bossWave_) {   // Charger adds: plain grunts
        e.maxHp = e.hp = waveEnemyHp(wave_) * waveHpMul_;
        e.speed = waveEnemySpeed(wave_);
    } else {
        setEnemyKind(e, rollEnemyKind(), waveEnemyHp(wave_) * waveHpMul_, waveEnemySpeed(wave_));
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
    heldPrevVel_ = b.vel;
    b.held = true;
    b.vel = {0.f, 0.f};
    b.trail.clear();
    b.waterTrail.clear();
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
    if (b.role == BallRole::Striker) throwVel *= cfg::role::strikerFlingMult;   // built to be flung
    const float s = length(throwVel);
    if (s < cfg::ball::minThrowSpeed) b.vel = rng_.direction() * cfg::ball::nudgeSpeed;
    else if (s > cfg::ball::hardSpeedCap) b.vel = throwVel * (cfg::ball::hardSpeedCap / s);
    else b.vel = throwVel;
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

void World::cancelHeld() {
    if (grabbed_ != Grabbed::Ball) return;
    Ball& b = balls_[heldIndex_];
    b.held = false;
    b.vel = heldPrevVel_;
    heldGrabOffset_ = {0.f, 0.f};
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
}

// Auto-throw option: now and then, launch the ball that's closest to plain
// cruising (the one doing least) at the enemy nearest the core.
void World::updateAutoFling(float dt, const WorldParams& p, FrameEvents& ev) {
    if (!p.autoFling || !waveRunning_) return;
    autoFlingTimer_ -= dt;
    if (autoFlingTimer_ > 0.f) return;
    autoFlingTimer_ = cfg::combat::autoFlingInterval;

    const Enemy* target = nullptr;
    float best = 1e18f;
    for (const Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const float d2 = dot(e.pos - core_.pos, e.pos - core_.pos);
        if (d2 < best) { best = d2; target = &e; }
    }
    sf::Vector2f aim = target ? target->pos : (boss_.alive ? boss_.pos : sf::Vector2f{-1.f, -1.f});
    if (aim.x < 0.f) return;

    Ball* pick = nullptr;
    float slowest = 1e18f;
    for (std::size_t i = 0; i < balls_.size(); ++i) {
        if (grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_) continue;
        Ball& b = balls_[i];
        const float ratio = length(b.vel) / std::max(1.f, ballCruise(b, p));
        if (ratio < slowest) { slowest = ratio; pick = &b; }
    }
    if (!pick || slowest > 1.3f) return;   // everyone is already flying hot
    const sf::Vector2f d = normalized(aim - pick->pos, {1.f, 0.f});
    pick->vel = d * std::min(ballCruise(*pick, p) * cfg::combat::autoFlingSpeedMul, ballMaxSpeed(*pick, p));
    ev.autoFlung = true;
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
        comboStreak_ += (effect_ && effect_->kind == PowerUp::Golden && effect_->remaining > 0.f)
                            ? cfg::powerup::goldenComboRate : 1;   // GOLDEN BOUNCE climbs faster
        sinceHit_ = 0.f;
    }
}

// Re-aim a Guardian's bounce at the enemy nearest the core (ignoring `skip`,
// the one it just hit, and anything already staggered and drifting away). The
// new heading must still leave the surface it hit, or the plain bounce stands.
void World::aimBounce(Ball& b, sf::Vector2f normal, const Enemy* skip) {
    if (!cfg::role::guardianAimsBounces || b.role != BallRole::Guardian) return;
    const Enemy* target = nullptr;
    float best = 1e18f;
    for (const Enemy& e : enemies_) {
        if (&e == skip || e.hp <= 0.f || e.stagger > 0.f) continue;
        const float d2 = dot(e.pos - core_.pos, e.pos - core_.pos);
        if (d2 < best) { best = d2; target = &e; }
    }
    if (!target) return;
    const sf::Vector2f d = normalized(target->pos - b.pos, {0.f, 0.f});
    if (dot(d, normal) <= 0.05f) return;   // would drive back into the wall / core
    const float sp = length(b.vel);
    const float ang = std::atan2(d.y, d.x) +
                      rng_.range(-cfg::role::guardianAimJitter, cfg::role::guardianAimJitter);
    b.vel = sf::Vector2f{std::cos(ang), std::sin(ang)} * sp;
}

float World::ballDamage(const Ball& b, const WorldParams& p) const {
    const float speed = length(b.vel);
    const float ratio = speed / cfg::ball::baseCruise;
    float dmg = (cfg::combat::contactDamageBase + cfg::combat::contactDamagePerCruise * ratio) *
                comboMultiplier() * p.damageMult * b.mods.damageMult;
    switch (b.role) {
        case BallRole::Striker: {   // pays off when flung: scales hard above its cruise speed
            const float over = speed / std::max(1.f, ballCruise(b, p)) - 1.f;
            if (over > 0.f) dmg *= 1.f + cfg::role::strikerSpeedDamage * over;
            break;
        }
        case BallRole::Support:  dmg *= cfg::role::supportDamageMul; break;
        case BallRole::Guardian: dmg *= cfg::role::guardianDamageMul; break;
        case BallRole::Normal:   break;
    }
    if (effect_ && effect_->kind == PowerUp::Overdrive)
        dmg *= 1.f + (cfg::powerup::overdriveDamageMul - 1.f) * effStrength(p);   // fades under "Afterglow"
    if (b.mods.bruiser)   // "Battering": the faster it flies, the harder it hits
        dmg *= 1.f + cfg::combat::bruiserPerCruise * ratio;
    if (b.element == Element::Fire)   // fire is a heavier hit; the burn DoT is the "Ember" node
        dmg *= 1.f + cfg::element::fireDamageBonus * elemPotency(b, p);
    if (b.ricochetT > 0.f) dmg *= cfg::combat::ricochetMult;   // "Ricochet": fresh off a wall
    return dmg;
}

// Fire / poison / ice act on contact (see advanceBall); water, stone and
// electric emit into the world on a per-ball timer.
void World::emitElement(Ball& b, float dt, const WorldParams& p) {
    switch (b.element) {
        case Element::Water: {
            // Lay down another point of the "worm" wake behind the ball. The
            // tail drops off once the worm is at full length; damage is done in
            // updateWaterTrails.
            b.cooldown -= dt;
            if (b.cooldown > 0.f) break;
            b.cooldown = cfg::element::waterInterval;
            b.waterTrail.push_back(b.pos);
            while (static_cast<int>(b.waterTrail.size()) > cfg::element::waterTrailPoints)
                b.waterTrail.pop_front();
            break;
        }
        case Element::Stone: {
            b.cooldown -= dt;
            if (b.cooldown > 0.f) break;
            b.cooldown = cfg::element::stoneInterval;
            if (static_cast<int>(obstacles_.size()) < cfg::element::maxObstacles) {
                const float life = cfg::element::obstacleLife *
                                   (b.mods.bedrock ? cfg::combat::bedrockLifeMult : 1.f);   // "Bedrock"
                obstacles_.push_back(Obstacle{b.pos, cfg::element::obstacleRadius, life, life});
            }
            break;
        }
        case Element::Electric: {
            b.cooldown -= dt;
            if (b.cooldown > 0.f) break;
            Enemy* target = nullptr;
            float bestD2 = cfg::element::boltRadius * cfg::element::boltRadius;
            for (Enemy& e : enemies_) {
                const float d2 = dot(e.pos - b.pos, e.pos - b.pos);
                if (d2 < bestD2) { bestD2 = d2; target = &e; }
            }
            if (!target) {
                b.cooldown = 0.15f;   // nothing in range: check again soon
                break;
            }
            const float zap = cfg::element::boltDamage * p.damageMult * b.mods.damageMult *
                              elemPotency(b, p);
            target->hp -= zap * (target->mark > 0.f ? cfg::role::markDamageMul : 1.f);
            target->hitFlash = 1.f;
            if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                bolts_.push_back(Bolt{b.pos, target->pos,
                                      cfg::element::boltLife, cfg::element::boltLife});

            if (b.mods.conductor) {   // "Conductor": arc jumps on to a second enemy
                Enemy* next = nullptr;
                float nd2 = cfg::combat::conductorRange * cfg::combat::conductorRange;
                for (Enemy& e : enemies_) {
                    if (&e == target) continue;
                    const float d2 = dot(e.pos - target->pos, e.pos - target->pos);
                    if (d2 < nd2) { nd2 = d2; next = &e; }
                }
                if (next) {
                    next->hp -= zap * cfg::combat::conductorFalloff;
                    next->hitFlash = 1.f;
                    if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                        bolts_.push_back(Bolt{target->pos, next->pos,
                                              cfg::element::boltLife, cfg::element::boltLife});
                }
            }
            b.cooldown = cfg::element::boltInterval;
            break;
        }
        default:
            break;
    }
}

void World::regulateSpeed(Ball& b, float dt, const WorldParams& p) {
    // Cruise is a floor the ball climbs back to fast and a target it eases down
    // to slowly, so a fling stays fast for a moment.
    const float cruiseS = ballCruise(b, p);
    const float vMax = ballMaxSpeed(b, p);

    const float sp = length(b.vel);
    if (sp < 1e-3f) {
        b.vel = rng_.direction() * cruiseS;
        return;
    }
    const float up = 1.f - std::exp(-cfg::ball::regainRate * dt);
    const float decayRate = cfg::ball::decayRate * b.mods.flingDecay *
                            (b.role == BallRole::Striker ? cfg::role::strikerFlingDecay : 1.f);
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
    b.radius = ballRadius(b, p);   // role, "Big ball" gear, "Mass" web

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
            aimBounce(b, c.normal, nullptr);
            boostSpeed(b, b.mods.wallBoost, p);   // "Wall rush"
            if (b.mods.ricochet) b.ricochetT = cfg::combat::ricochetWindow;   // "Ricochet"
            pushFx(c);
        }
        // The core is solid: balls bounce off it (no damage to the core).
        if (collision::Contact c =
                collision::circleVsSolidCircle(b, core_.pos, core_.radius, 1.f);
            c.hit) {
            afterBounce(b, c.normal, false);
            aimBounce(b, c.normal, nullptr);
            boostSpeed(b, p.coreBounceBoost, p);   // "Spring core" relic
            pushFx(c);
        }
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f) continue;   // already dead this frame (not swept yet) - don't re-hit / double-splash
            collision::Contact c =
                collision::circleVsSolidCircle(b, e.pos, e.radius, cfg::combat::hitRebound);
            if (!c.hit) continue;
            // Off the shield: a bounce, no damage. A Guardian is heavy enough to
            // smash straight through it.
            if (b.role != BallRole::Guardian && shieldBlocks(e, b.pos, core_.pos)) {
                afterBounce(b, c.normal, false);
                aimBounce(b, c.normal, &e);
                ev.shieldBlock = true;
                pushFx(c);
                continue;
            }
            float dmg = ballDamage(b, p);   // fire / ricochet / Battering bonuses are baked into ballDamage
            const bool afflicted = e.poison > 0.f || e.frozen > 0.f || e.burn > 0.f;
            if (p.primed && afflicted) dmg *= cfg::combat::primedMult;   // "Primed"
            if (b.mods.shatter && e.frozen > 0.f) dmg *= cfg::combat::shatterBonus;  // "Shatter" (stacks)
            if (e.mark > 0.f) dmg *= cfg::role::markDamageMul;                     // marked by a Support
            if (b.mods.critChance > 0.f && rng_.range(0.f, 1.f) < b.mods.critChance)   // "Keen eye"
                dmg *= cfg::combat::critMult;
            if (b.mods.executioner && e.hp < e.maxHp * cfg::combat::executeThreshold)  // "Executioner"
                dmg *= cfg::combat::executeMult;

            const bool kill = dmg >= e.hp;
            if (b.mods.overkill && kill) {   // "Overkill": leftover damage splashes to a neighbour
                const float leftover = (dmg - e.hp) * cfg::combat::overkillFrac;
                if (leftover > 0.f) {
                    Enemy* nb = nullptr;
                    float best = cfg::combat::overkillRange * cfg::combat::overkillRange;
                    for (Enemy& o : enemies_) {
                        if (&o == &e || o.hp <= 0.f) continue;
                        const float d2 = dot(o.pos - e.pos, o.pos - e.pos);
                        if (d2 < best) { best = d2; nb = &o; }
                    }
                    if (nb) { nb->hp -= leftover; nb->hitFlash = 1.f; }
                }
            }

            e.hp -= dmg;
            e.hitFlash = 1.f;
            const float knock = cfg::combat::knockback * b.mods.knockMult *   // "Heavy knock"
                                (b.role == BallRole::Guardian ? cfg::role::guardianKnockMul : 1.f);
            e.vel += -c.normal * knock * e.knockTaken;
            if (b.role == BallRole::Support) e.mark = cfg::role::markDuration;
            if (b.role == BallRole::Guardian) e.stagger = cfg::role::staggerDuration;
            const float pot = elemPotency(b, p);
            if (b.element == Element::Poison) {
                e.poison = cfg::element::poisonDuration;
                e.poisonDps = std::min(e.poisonDps + cfg::element::poisonDpsPerHit * pot,
                                       cfg::element::poisonDpsMax * pot);
            } else if (b.element == Element::Ice) {
                e.frozen = std::max(e.frozen, cfg::element::freezeDuration * pot);
            } else if (b.element == Element::Fire && p.emberLevel > 0) {   // "Ember": light it up
                e.burn = cfg::element::burnDuration;
                e.burnDps = cfg::element::burnDps * pot *
                            (1.f + cfg::element::burnPerEmberLevel *
                                       static_cast<float>(p.emberLevel - 1));
            }
            if (b.mods.tempo) {   // "Tempo": snap back toward cruise faster after a hit
                const float cs = ballCruise(b, p);
                const float sp = length(b.vel);
                if (sp > 1e-3f && sp < cs) b.vel *= lerpf(sp, cs, cfg::combat::tempoRecover) / sp;
            }
            if (b.mods.cleave && kill) { pushFx(c); continue; }   // "Cleave": pass straight through
            afterBounce(b, c.normal, true);
            aimBounce(b, c.normal, &e);
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
                    length(b.vel) >= ballBaseCruise(b, p) * cfg::boss::minHitCruiseFrac) {
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

    b.ricochetT = std::max(0.f, b.ricochetT - dt);   // "Ricochet" window ticks down
    emitElement(b, dt, p);
    regulateSpeed(b, dt, p);
    b.color = b.element == Element::Plain
                  ? theme::speedColor(length(b.vel), cruiseBase(p))
                  : theme::elementSpeedColor(elementColor(b.element), length(b.vel), cruiseBase(p));
    b.squash *= std::exp(-cfg::ball::squashDecay * dt);
    updateTrail(b);
}

void World::resolveBallPairs(FrameEvents& ev, const WorldParams& p) {
    for (std::size_t i = 0; i < balls_.size(); ++i) {
        if (grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_) continue;
        for (std::size_t j = i + 1; j < balls_.size(); ++j) {
            if (grabbed_ == Grabbed::Ball && static_cast<int>(j) == heldIndex_) continue;
            Ball& a = balls_[i];
            Ball& b = balls_[j];
            if (!collision::resolveBallPair(a, b)) continue;

            boostSpeed(a, a.mods.pairBoost, p);   // "Carom"
            boostSpeed(b, b.mods.pairBoost, p);

            const sf::Vector2f n = normalized(b.pos - a.pos);
            a.squash = b.squash = 1.f;
            a.squashAxis = b.squashAxis = n;

            BounceFx fx;
            fx.pos = (a.pos + b.pos) * 0.5f;
            fx.normal = n;
            fx.color = lerpColor(a.color, b.color, 0.5f);
            fx.speed = std::max(length(a.vel), length(b.vel));
            fx.ballPair = true;
            ev.bounces.push_back(fx);
        }
    }
}

// Electric arcs are damage-on-spawn; here they only fade out.
void World::updateBolts(float dt) {
    for (auto it = bolts_.begin(); it != bolts_.end();) {
        it->life -= dt;
        if (it->life <= 0.f) it = bolts_.erase(it);
        else ++it;
    }
}

// A water ball drags a "worm" of recent positions. Enemies near any segment of
// it take damage; the worm is widest at the head (nearest the ball) and tapers
// to nothing at the tail.
void World::updateWaterTrails(float dt, const WorldParams& p) {
    for (const Ball& b : balls_) {
        if (b.element != Element::Water) continue;
        const float w0 = cfg::element::waterTrailWidth * elemPotency(b, p);
        const auto& pts = b.waterTrail;
        const int n = static_cast<int>(pts.size());
        if (n < 2) continue;
        for (Enemy& e : enemies_) {
            for (int i = 0; i + 1 < n; ++i) {
                const sf::Vector2f a = pts[i], c = pts[i + 1];   // one segment
                const sf::Vector2f seg = c - a;
                const float len2 = dot(seg, seg);
                float t = len2 > 1e-4f ? clampf(dot(e.pos - a, seg) / len2, 0.f, 1.f) : 0.f;
                const sf::Vector2f closest = a + seg * t;
                // head (i near n-1) is full width, tail (i=0) is ~0
                const float taper = static_cast<float>(i + 1) / static_cast<float>(n);
                if (length(e.pos - closest) < w0 * taper + e.radius) {
                    e.hp -= cfg::element::waterDps * dt;
                    break;   // one segment's worth of damage per enemy per step
                }
            }
        }
    }
}

void World::updateObstacles(float dt) {
    for (auto it = obstacles_.begin(); it != obstacles_.end();) {
        it->life -= dt;
        for (Enemy& e : enemies_) {   // rubble grinds anything standing in it
            if (e.orbiter) continue;
            if (length(e.pos - it->pos) < it->radius + e.radius)
                e.hp -= cfg::element::stoneDps * dt;
        }
        if (it->life <= 0.f) it = obstacles_.erase(it);
        else ++it;
    }
}

void World::updateEnemies(float dt, const WorldParams& p, FrameEvents& ev) {
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        Enemy& e = *it;

        if (e.poison > 0.f) {
            e.poison -= dt;
            e.hp -= e.poisonDps * dt;
            if (e.poison <= 0.f) e.poisonDps = 0.f;   // stacks clear when it wears off
        }
        if (e.burn > 0.f) {   // "Ember": fire burn damage-over-time
            e.burn -= dt;
            e.hp -= e.burnDps * dt;
            if (e.burn <= 0.f) e.burnDps = 0.f;
        }
        e.mark = std::max(0.f, e.mark - dt);

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

        // Frozen by an ice ball: hold still (no steering, no core damage) until
        // it thaws.
        if (e.frozen > 0.f) {
            e.frozen -= dt;
            e.vel = {0.f, 0.f};
            e.hitFlash *= std::exp(-6.f * dt);
            ++it;
            continue;
        }

        if (e.stagger > 0.f) {
            // Staggered by a Guardian: no steering - it just drifts on the
            // knockback, bleeding speed, so it gets shoved clear of the core.
            e.stagger -= dt;
            e.vel *= std::exp(-cfg::role::staggerDrag * dt);
            e.pos += e.vel * dt;
        } else {
            const sf::Vector2f d = core_.pos - e.pos;
            const float dl = length(d);
            const sf::Vector2f steer = (dl > 1e-3f ? d / dl : sf::Vector2f{0.f, 1.f}) * e.speed;

            // SLOW MOTION drags every enemy; the "Slow field" item drags only
            // those close to the core. Both just scale this enemy's time step.
            float edt = dt;
            if (effect_ && effect_->kind == PowerUp::SlowMo)
                edt *= lerpf(1.f, cfg::powerup::slowMoEnemyMul, effStrength(p));   // eases back under "Afterglow"
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
            if (!invuln_) {
                if (aegisChargesLeft_ > 0) --aegisChargesLeft_;   // "Aegis" soaks the hit
                else core_.hp -= e.coreDamage;
            }
            core_.hitFlash = 1.f;
            coreHitThisWave_ = true;   // "Interest" is off for this wave now
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
    if (p.coreRegenPerSec > 0.f && !runOver_)   // "Regen": trickle the core back up during a wave
        core_.hp = std::min(core_.maxHp, core_.hp + p.coreRegenPerSec * dt);
    core_.hitFlash *= std::exp(-5.f * dt);
}

void World::sweepDeadEnemies(FrameEvents& ev, const WorldParams& p) {
    std::vector<Enemy> shards;   // Splitters burst after the sweep (can't grow the vector mid-loop)
    for (auto it = enemies_.begin(); it != enemies_.end();) {
        if (it->hp <= 0.f) {
            if (it->kind == EnemyKind::Splitter) {
                const float baseHp = it->maxHp / cfg::enemy::splitterHp;
                const float baseSpeed = it->speed;
                for (int i = 0; i < cfg::enemy::shardCount; ++i) {
                    Enemy s;
                    setEnemyKind(s, EnemyKind::Shard, baseHp, baseSpeed);
                    const float a = rng_.range(0.f, 2.f * kPi);
                    const sf::Vector2f d{std::cos(a), std::sin(a)};
                    s.pos = it->pos + d * (it->radius * 0.6f);
                    s.vel = d * 160.f;   // flung apart, then they turn for the core
                    s.stagger = 0.25f;
                    shards.push_back(s);
                }
            }
            if (p.contagion && it->poison > 0.f) {   // "Contagion": spread the poison on death
                const float dps = cfg::element::poisonDpsPerHit *
                                  p.elemMult[static_cast<int>(Element::Poison)];
                for (Enemy& e : enemies_) {
                    if (&e == &*it) continue;
                    if (length(e.pos - it->pos) < cfg::combat::contagionRadius) {
                        e.poison = cfg::element::poisonDuration;
                        e.poisonDps = std::max(e.poisonDps, dps);
                    }
                }
            }
            ev.kills.push_back(it->pos);
            it = enemies_.erase(it);
        } else {
            ++it;
        }
    }
    for (Enemy& s : shards) enemies_.push_back(s);
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

void World::activateEffect(PowerUp k, const WorldParams& p) {
    // "Charged" pads the duration; "Capacitor" (pickupDurMult) already folded in.
    const float dur = powerUpDuration(k) * p.pickupDurMult * (1.f + p.chargedFrac);
    effect_ = ActiveEffect{k, dur, dur};
}

void World::useReserve(const WorldParams& p) {
    if (!hasReserve_) return;
    activateEffect(reservePu_, p);
    hasReserve_ = false;
    reserveTimer_ = cfg::powerup::reserveFillTime;
}

float World::effStrength(const WorldParams& p) const {
    if (!effect_) return 0.f;
    if (effect_->remaining >= 0.f) return 1.f;   // still live
    const float tail = static_cast<float>(p.afterglowLevel) * cfg::powerup::afterglowPerLevel;
    if (tail <= 0.f) return 0.f;
    return clampf(1.f + effect_->remaining / tail, 0.f, 1.f);   // remaining in [-tail, 0] -> [0, 1]
}

void World::updatePickups(float dt, const WorldParams& p, FrameEvents& ev) {
    if (p.stockpile && !hasReserve_) {   // "Stockpile": slowly refill the reserve slot at random
        reserveTimer_ -= dt;
        if (reserveTimer_ <= 0.f) {
            int enabled[kPowerUpCount];
            int n = 0;
            for (int i = 0; i < kPowerUpCount; ++i)
                if (p.powerUpMask & (1u << i)) enabled[n++] = i;
            if (n > 0) {
                reservePu_ = static_cast<PowerUp>(enabled[rng_.irange(0, n - 1)]);
                hasReserve_ = true;
            }
            reserveTimer_ = cfg::powerup::reserveFillTime;
        }
    }

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
        if (p.magnetPickups && !balls_.empty()) {   // "Magnet": steer toward the nearest ball
            const Ball* nearest = nullptr;
            float best = 1e18f;
            for (const Ball& b : balls_) {
                const float d2 = dot(b.pos - pu.pos, b.pos - pu.pos);
                if (d2 < best) { best = d2; nearest = &b; }
            }
            if (nearest) {
                pu.vel += normalized(nearest->pos - pu.pos, {1.f, 0.f}) * cfg::powerup::magnetAccel * dt;
                const float sp = length(pu.vel);
                if (sp > cfg::powerup::magnetMaxSpeed) pu.vel *= cfg::powerup::magnetMaxSpeed / sp;
            }
        }
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
                activateEffect(pu.kind, p);
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

void World::advanceEffect(float dt, const WorldParams& p) {
    if (!effect_) return;
    effect_->remaining -= dt;
    // "Afterglow": once the timer hits 0 the effect lingers, fading, for the tail.
    const float tail = static_cast<float>(p.afterglowLevel) * cfg::powerup::afterglowPerLevel;
    if (effect_->remaining <= -tail) effect_.reset();
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
    updateAutoFling(dt, p, ev);
    if (waveRunning_) waveClock_ += dt;   // "Warm-up" ramp

    for (std::size_t i = 0; i < balls_.size(); ++i) {
        Ball& b = balls_[i];
        if (grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_) {
            b.squash *= std::exp(-cfg::ball::squashDecay * dt);
            continue;
        }
        advanceBall(b, dt, p, ev);
    }

    resolveBallPairs(ev, p);
    updateBolts(dt);
    updateWaterTrails(dt, p);
    updateObstacles(dt);
    updateEnemies(dt, p, ev);
    sweepDeadEnemies(ev, p);
    updateBoss(dt, p, ev);
    updateWaveSpawner(dt, ev);
    updatePickups(dt, p, ev);
    advanceEffect(dt, p);

    const int tier = comboTier();
    ev.comboTier = tier;
    if (tier > reportedTier_) ev.comboTierUp = true;
    reportedTier_ = tier;
    ev.runOver = runOver_;
    return ev;
}

}  // namespace sb
