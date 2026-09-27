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
    return cfg::ball::baseCruise * p.cruiseMult * arenaScale();
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
    return cruiseSpeed(p) * b.mods.cruiseMult *
           (b.role == BallRole::Guardian ? cfg::role::guardianCruiseMul : 1.f);
}

float World::ballMaxSpeed(const Ball& b, const WorldParams& p) const {
    return std::min(cruiseBase(p) * cfg::ball::maxSpeedCruiseMul * b.mods.maxSpeedMult * p.pact.speedCeilMul,
                    cfg::ball::hardSpeedCap * arenaScale());
}

float World::ballRadius(const Ball& b, const WorldParams& p) const {
    // Every size bonus (web, Big ball, Giant, Bumper, Guardian, Glutton) stacks,
    // but never past maxRadiusMult - a ball must not fill the arena.
    float mult = p.ballRadiusMult * b.mods.radiusMult *
                 (b.role == BallRole::Guardian ? cfg::role::guardianRadiusMul : 1.f) *
                 (1.f + cfg::changer::gluttonRadius * static_cast<float>(b.gluttonStacks));
    mult = std::min(mult, cfg::changer::maxRadiusMult);
    return cfg::ball::radius * mult * b.scale * (1.f + (arenaScale() - 1.f) * cfg::boss::ballRadiusArenaFrac);
}

float World::elemPotency(const Ball& b, const WorldParams& p) const {
    return p.elemMult[static_cast<int>(b.element)] * b.mods.elemMult *   // web level x item level
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
    return m / arenaScale();   // on-screen speed, comparable across arenas
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
    b.owner = static_cast<int>(balls_.size());
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
        b.owner = static_cast<int>(i);
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

void World::devSpawn(EnemyKind k, int n) {
    for (int i = 0; i < n; ++i) {
        spawnEnemy();
        setEnemyKind(enemies_.back(), k, waveEnemyHp(std::max(1, wave_)) * waveHpMul_,
                     waveEnemySpeed(std::max(1, wave_)));
    }
    waveRunning_ = true;
}

void World::devKillAll() {
    for (Enemy& e : enemies_)
        if (!e.orbiter) e.hp = 0.f;   // swept (as kills) on the next step
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
    ghosts_.clear();
    pendingGhosts_.clear();
    enemies_.clear();
    bolts_.clear();
    obstacles_.clear();
    blackHoles_.clear();
    tethers_.clear();
    pickups_.clear();
    effect_.reset();
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
    huntPrey_.clear();
    huntHas_.clear();
    coreZapT_ = 0.f;
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
        b.gluttonStacks = 0;   // "Glutton" grows per wave
        b.preyId = -1;
        b.homing = false;
    }
    ghosts_.clear();   // "Split shot" / "Mitosis" copies don't outlive their wave
    pendingGhosts_.clear();
    blackHoles_.clear();
    tethers_.clear();
    heldGrabOffset_ = {0.f, 0.f};
    grabbed_ = Grabbed::None;
    heldIndex_ = -1;
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
    e.id = nextEnemyId_++;
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
    e.id = nextEnemyId_++;
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
        if (balls_[i].mods.satellite) continue;   // an orbiting Satellite can't be grabbed
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
    if (b.mods.cometFling > 0.f) throwVel *= b.mods.cometFling;                 // "Comet"
    b.homing = false;
    const float s = length(throwVel);
    if (s < cfg::ball::minThrowSpeed) b.vel = rng_.direction() * cfg::ball::nudgeSpeed;
    else if (s > cfg::ball::hardSpeedCap * arenaScale())
        b.vel = throwVel * (cfg::ball::hardSpeedCap * arenaScale() / s);
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

std::optional<sf::Vector2f> World::nearestTarget(sf::Vector2f from) const {
    std::optional<sf::Vector2f> best;
    float bestD2 = 1e18f;
    for (const Enemy& e : enemies_) {
        if (e.hp <= 0.f) continue;   // dying: not worth a throw
        const float d2 = dot(e.pos - from, e.pos - from);
        if (d2 < bestD2) { bestD2 = d2; best = e.pos; }
    }
    if (boss_.alive && boss_.hp > 0.f && dot(boss_.pos - from, boss_.pos - from) < bestD2) best = boss_.pos;
    return best;
}

// Auto-throw option: now and then, launch the ball that's closest to plain
// cruising (the one doing least) at the enemy nearest the core.
void World::updateAutoFling(float dt, const WorldParams& p, FrameEvents& ev) {
    const bool clockwork = p.pact.autoFlingEvery > 0.f;   // "Clockwork" pact: always on, faster
    if ((!p.autoFling && !clockwork) || !waveRunning_) return;
    autoFlingTimer_ -= dt;
    if (autoFlingTimer_ > 0.f) return;
    autoFlingTimer_ = clockwork ? p.pact.autoFlingEvery : cfg::combat::autoFlingInterval;

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
        if (b.mods.satellite) continue;
        const float ratio = length(b.vel) / std::max(1.f, ballCruise(b, p));
        if (ratio < slowest) { slowest = ratio; pick = &b; }
    }
    if (!pick || (!clockwork && slowest > 1.3f)) return;   // everyone is already flying hot
    const sf::Vector2f d = normalized(aim - pick->pos, {1.f, 0.f});
    const float mul = clockwork ? p.pact.autoFlingSpeed : cfg::combat::autoFlingSpeedMul;
    pick->vel = d * std::min(ballCruise(*pick, p) * mul, ballMaxSpeed(*pick, p));
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

void World::advanceCombo(float dt, const WorldParams& p) {
    comboCapTier_ = cfg::combo::baseCapTier * (p.overcharge ? cfg::changer::overchargeMul : 1);   // "Overcharge"
    comboExtra_ = p.pact.bloodlust ? 1 : 0;   // "Bloodlust" pact: climbs twice as fast...
    sinceHit_ += dt;
    if (!p.pact.bloodlust && sinceHit_ > cfg::combo::decayWindow && comboStreak_ > 0) {   // ...and never cools
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

    if (countHit) addComboHit();
}

void World::addComboHit() {
    comboStreak_ += (effect_ && effect_->kind == PowerUp::Golden && effect_->remaining > 0.f)
                        ? cfg::powerup::goldenComboRate : 1;   // GOLDEN BOUNCE climbs faster
    comboStreak_ += comboExtra_;   // "Bloodlust" pact
    sinceHit_ = 0.f;
}

// Re-aim a Guardian's bounce at the enemy nearest the core (ignoring `skip`,
// the one it just hit, and anything already staggered and drifting away). The
// new heading must still leave the surface it hit, or the plain bounce stands.
void World::aimBounce(Ball& b, sf::Vector2f normal, const Enemy* skip, bool force) {
    if (!force && (!cfg::role::guardianAimsBounces || b.role != BallRole::Guardian)) return;
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

// ---------------------------------------------------------------- game-changers

// "Satellite": no bouncing - the ball rides a fixed orbit around the core at
// cruise speed and grinds whatever comes near (each enemy on a short cooldown).
void World::advanceSatellite(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    const float R = cfg::changer::satelliteRadius * arenaScale() + core_.radius;
    const float sp = ballCruise(b, p) * cfg::changer::satelliteSpeed;
    b.orbitAng += sp / R * dt;
    const sf::Vector2f dir{std::cos(b.orbitAng), std::sin(b.orbitAng)};
    b.pos = core_.pos + dir * R;
    b.vel = sf::Vector2f{-dir.y, dir.x} * sp;   // tangent: the speed damage and colour read
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.pierceCd > 0.f) continue;
        const sf::Vector2f d = b.pos - e.pos;
        if (length(d) > b.radius + e.radius) continue;
        e.pierceCd = cfg::changer::pierceCooldown;
        const sf::Vector2f n = normalized(d, {1.f, 0.f});
        strike(b, e, n, p, ev);
        BounceFx fx;
        fx.pos = e.pos + n * e.radius;
        fx.normal = n;
        fx.speed = sp / arenaScale();
        fx.color = b.color;
        ev.bounces.push_back(fx);
    }
    if (boss_.alive && boss_.intro <= 0.f && boss_.hitCd <= 0.f &&
        length(b.pos - boss_.pos) < b.radius + boss_.radius) {
        boss_.hp -= ballDamage(b, p);
        boss_.hitFlash = 1.f;
        boss_.hitCd = cfg::boss::hitCooldown * 3.f;   // an orbit grinds, it shouldn't melt a boss
        ev.bossHit = true;
    }
    emitElement(b, dt, p, ev);
    b.color = b.element == Element::Plain
                  ? theme::speedColor(sp, cruiseBase(p))
                  : theme::elementSpeedColor(elementColor(b.element), sp, cruiseBase(p));
    b.squash *= std::exp(-cfg::ball::squashDecay * dt);
    updateTrail(b);
}

// "Railgun": a wall bounce fires a straight beam along the new heading to the
// far wall; everything it crosses takes the ball's hit (and its element).
void World::fireRail(Ball& b, const WorldParams& p, FrameEvents& ev) {
    const sf::Vector2f dir = normalized(b.vel, {1.f, 0.f});
    float t = 1e9f;
    if (dir.x > 1e-4f) t = std::min(t, (size_.x - b.pos.x) / dir.x);
    if (dir.x < -1e-4f) t = std::min(t, -b.pos.x / dir.x);
    if (dir.y > 1e-4f) t = std::min(t, (size_.y - b.pos.y) / dir.y);
    if (dir.y < -1e-4f) t = std::min(t, -b.pos.y / dir.y);
    t = std::max(0.f, std::min(t, 4000.f));
    const sf::Vector2f end = b.pos + dir * t;
    const float dmg = ballDamage(b, p) * b.mods.railFrac;
    const float w = b.mods.railWidth * arenaScale();
    const Element el = hitElement(b, p);
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f) continue;
        const float along = clampf(dot(e.pos - b.pos, dir), 0.f, t);
        if (length(e.pos - (b.pos + dir * along)) > w + e.radius) continue;
        damageEnemy(e, dmg);
        applyElement(e, el, b.owner, dmg, p, ev);
    }
    Bolt beam{b.pos, end, 0.22f, 0.22f};
    beam.beam = true;
    bolts_.push_back(beam);
}

// "Storm": every so often, zap everything in a ring around the ball.
void World::updateStorm(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    b.stormT -= dt;
    if (b.stormT > 0.f) return;
    b.stormT = b.mods.stormInterval;
    const float R = cfg::changer::stormRadius * arenaScale();
    const float dmg = ballDamage(b, p) * b.mods.stormFrac;
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || length(e.pos - b.pos) > R + e.radius) continue;
        damageEnemy(e, dmg);
        applyElement(e, hitElement(b, p), b.owner, dmg, p, ev);
        if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
            bolts_.push_back(Bolt{b.pos, e.pos, cfg::element::boltLife, cfg::element::boltLife});
    }
}

// "Gemini": every ball carrying it keeps its ghost twins (1, 2 at Lv3, 3 at
// Lv5) that copy its items; they go when the item does. Twins spread evenly
// around the parent: opposite heading / the other side of a Satellite orbit.
void World::updateTwins(const WorldParams& p) {
    for (Ball& g : ghosts_) {
        if (!g.twin) continue;
        const Ball* parent = (g.owner >= 0 && g.owner < static_cast<int>(balls_.size())) ? &balls_[g.owner] : nullptr;
        if (!parent || g.twinIdx >= parent->mods.twins) { g.ghostLife = 0.f; continue; }
        g.mods = parent->mods;      // keep up with new items
        g.role = parent->role;
        g.element = parent->element;
        g.ghostLife = 1e9f;
    }
    for (const Ball& b : balls_) {
        for (int idx = 0; idx < b.mods.twins; ++idx) {
            bool has = false;
            for (const std::vector<Ball>* set : {&ghosts_, &pendingGhosts_})
                for (const Ball& g : *set)
                    if (g.twin && g.owner == b.owner && g.twinIdx == idx && g.ghostLife > 0.f) has = true;
            if (has) continue;
            const float turn = 2.f * kPi * static_cast<float>(idx + 1) / static_cast<float>(b.mods.twins + 1);
            Ball t = b;
            t.ghost = true;
            t.twin = true;
            t.twinIdx = idx;
            t.ghostLife = 1e9f;
            t.held = false;
            t.trail.clear();
            t.waterTrail.clear();
            t.orbitAng = b.orbitAng + turn;
            const float c = std::cos(turn), sn = std::sin(turn);
            t.vel = {b.vel.x * c - b.vel.y * sn, b.vel.x * sn + b.vel.y * c};
            if (length(t.vel) < 1e-3f) t.vel = rng_.direction() * cruiseBase(p);
            pendingGhosts_.push_back(t);
        }
    }
}

// "Hunter": lock onto the biggest threat (toughest, weighted toward the core)
// and bend hard toward it until it dies; with nothing left, go for the boss.
void World::steerHunter(Ball& b, float dt) {
    const Enemy* prey = nullptr;
    for (const Enemy& e : enemies_)
        if (e.id == b.preyId && e.hp > 0.f) prey = &e;
    if (!prey) {
        float best = -1.f;
        for (const Enemy& e : enemies_) {
            if (e.hp <= 0.f) continue;
            const float threat = e.hp / (1.f + length(e.pos - core_.pos) / (cfg::changer::hunterCoreBias * arenaScale()));
            if (threat > best) { best = threat; prey = &e; }
        }
        b.preyId = prey ? prey->id : -1;
    }
    sf::Vector2f target;
    if (prey) target = prey->pos;
    else if (boss_.alive && boss_.intro <= 0.f) target = boss_.pos;
    else return;
    const float sp = length(b.vel);
    if (sp < 1e-3f) return;
    const float cur = std::atan2(b.vel.y, b.vel.x);
    const float want = std::atan2(target.y - b.pos.y, target.x - b.pos.x);
    const float maxTurn = b.mods.hunterTurn * dt;
    const float diff = clampf(std::remainder(want - cur, 2.f * kPi), -maxTurn, maxTurn);
    b.vel = sf::Vector2f{std::cos(cur + diff), std::sin(cur + diff)} * sp;
}

// "Tether": every ball carrying it burns a laser to the nearest other ball
// (twins and copies count). Damage ticks on whatever touches the line, and
// carries the ball's element - two tethers of different elements react.
void World::updateTethers(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace C = cfg::changer;
    tethers_.clear();
    for (std::vector<Ball>* set : {&balls_, &ghosts_})
        for (Ball& b : *set) {
            if (b.mods.tetherFrac <= 0.f) continue;
            const Ball* partner = nullptr;
            float best = C::tetherMaxLen * arenaScale();
            best *= best;
            for (const std::vector<Ball>* s2 : {&balls_, &ghosts_})
                for (const Ball& o : *s2) {
                    if (&o == &b) continue;
                    const float d2 = dot(o.pos - b.pos, o.pos - b.pos);
                    if (d2 < best) { best = d2; partner = &o; }
                }
            if (!partner) continue;
            const float w = b.mods.tetherWidth * arenaScale();
            tethers_.push_back({b.pos, partner->pos, w, b.color});

            b.tetherT -= dt;
            if (b.tetherT > 0.f) continue;
            b.tetherT = C::tetherTick;
            const float tick = ballDamage(b, p) * b.mods.tetherFrac * C::tetherTick;
            const Element el = hitElement(b, p);
            const sf::Vector2f a = b.pos, seg = partner->pos - b.pos;
            const float len2 = std::max(dot(seg, seg), 1e-4f);
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f) continue;
                const float t = clampf(dot(e.pos - a, seg) / len2, 0.f, 1.f);
                if (length(e.pos - (a + seg * t)) > w + e.radius) continue;
                damageEnemy(e, tick * (e.mark > 0.f ? cfg::role::markDamageMul : 1.f));
                e.hitFlash = std::max(0.f, e.hitFlash - 0.6f);   // a beam doesn't flash like a hit
                applyElement(e, el, b.owner, tick, p, ev);
            }
        }
}

// "Black hole": pull everything near it in, then burst with the element of
// the ball that left it.
void World::updateBlackHoles(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace C = cfg::changer;
    const float R = C::blackHoleRadius * arenaScale();
    for (auto it = blackHoles_.begin(); it != blackHoles_.end();) {
        BlackHole& h = *it;
        h.life -= dt;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter) continue;
            const sf::Vector2f to = h.pos - e.pos;
            const float d = length(to);
            if (d > R || d < 1e-3f) continue;
            const float step = C::blackHolePull * h.pull * arenaScale() * (1.f - d / R) * dt;
            e.pos += to / d * std::min(step, d);
        }
        if (h.life > 0.f) { ++it; continue; }
        const float br = C::blackHoleBurst * arenaScale();
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || length(e.pos - h.pos) > br + e.radius) continue;
            damageEnemy(e, h.dmg);
            applyElement(e, h.elem, h.owner, h.dmg, p, ev);
        }
        ev.bursts.push_back({h.pos, br, h.elem == Element::Plain ? theme::accent : elementColor(h.elem), nullptr});
        it = blackHoles_.erase(it);
    }
}

// "Resonance": a hit arcs to every other ball of the same element (twins and
// copies included); each of them zaps the enemy nearest to it.
void World::resonate(Ball& b, float dmg, const WorldParams& p, FrameEvents& ev) {
    if (b.resonanceT > 0.f) return;
    b.resonanceT = b.mods.resonanceCd;
    const float range = cfg::changer::resonanceRange * arenaScale();
    for (std::vector<Ball>* set : {&balls_, &ghosts_})
        for (Ball& o : *set) {
            if (&o == &b || o.element != b.element) continue;
            if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                bolts_.push_back(Bolt{b.pos, o.pos, cfg::element::boltLife, cfg::element::boltLife});
            Enemy* t = nullptr;
            float best = range * range;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f) continue;
                const float d2 = dot(e.pos - o.pos, e.pos - o.pos);
                if (d2 < best) { best = d2; t = &e; }
            }
            if (!t) continue;
            const float zap = dmg * b.mods.resonanceFrac;
            damageEnemy(*t, zap);
            applyElement(*t, hitElement(o, p), o.owner, zap, p, ev);
            if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                bolts_.push_back(Bolt{o.pos, t->pos, cfg::element::boltLife, cfg::element::boltLife});
        }
}

// ---------------------------------------------------------------- synergies

bool World::chance(float base, const WorldParams& p) {
    return rng_.range(0.f, 1.f) < std::min(base * p.luck, cfg::synergy::chanceCap);
}

void World::damageEnemy(Enemy& e, float dmg) {
    e.hp -= dmg * (e.brittle > 0.f ? cfg::synergy::brittleMul : 1.f);
    e.hitFlash = 1.f;
}

void World::areaDamage(sf::Vector2f at, float radius, float dmg, const Enemy* skip) {
    for (Enemy& o : enemies_) {
        if (&o == skip || o.hp <= 0.f) continue;
        if (length(o.pos - at) < radius + o.radius) damageEnemy(o, dmg);
    }
}

bool World::strike(Ball& b, Enemy& e, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev,
                   bool allowEcho) {
    namespace S = cfg::synergy;
    const BallMods& m = b.mods;
    float dmg = ballDamage(b, p);   // fire / ricochet / Glutton / role bonuses are baked in
    const bool afflicted = e.poison > 0.f || e.frozen > 0.f || e.burn > 0.f;
    if (p.primed && afflicted) dmg *= cfg::combat::primedMult;             // "Primed"
    if (m.shatterMult > 0.f && e.frozen > 0.f) dmg *= m.shatterMult;       // "Shatter"
    if (e.mark > 0.f) dmg *= cfg::role::markDamageMul;                     // marked by a Support
    if (e.brittle > 0.f) dmg *= S::brittleMul;                             // Superconductor
    if (m.critChance > 0.f && chance(m.critChance, p)) dmg *= m.critMult;  // "Keen eye"
    if (m.executeThreshold > 0.f && e.hp < e.maxHp * m.executeThreshold)   // "Executioner"
        dmg *= m.executeMult;
    if (m.hunterMult > 0.f && e.id == b.preyId) dmg *= m.hunterMult;       // "Hunter": its prey
    if (b.charged) {                                                       // "Boomerang": the trip home paid off
        if (m.boomerangHit > 0.f) dmg *= m.boomerangHit;
        b.charged = false;
    }
    if (m.berserkPerHit > 0.f) {   // "Berserk": hits in a row stack up until a wall
        dmg *= 1.f + m.berserkPerHit * static_cast<float>(b.berserkStacks);
        b.berserkStacks = std::min(b.berserkStacks + 1, m.berserkMax);
    }
    // "Cleave" Lv2+: a hit that leaves it nearly dead finishes it (and cuts through).
    if (m.cleaveExec > 0.f && e.hp - dmg < e.maxHp * m.cleaveExec) dmg = std::max(dmg, e.hp);

    const bool kill = dmg >= e.hp;
    if (m.overkillFrac > 0.f && kill) {   // "Overkill": leftover damage splashes to the nearest few
        const float leftover = (dmg - e.hp) * m.overkillFrac;
        std::vector<const Enemy*> hit{&e};
        for (int n = 0; n < m.overkillTargets && leftover > 0.f; ++n) {
            Enemy* nb = nullptr;
            float best = cfg::combat::overkillRange * cfg::combat::overkillRange;
            for (Enemy& o : enemies_) {
                if (o.hp <= 0.f || std::find(hit.begin(), hit.end(), &o) != hit.end()) continue;
                const float d2 = dot(o.pos - e.pos, o.pos - e.pos);
                if (d2 < best) { best = d2; nb = &o; }
            }
            if (!nb) break;
            damageEnemy(*nb, leftover);
            hit.push_back(nb);
        }
    }

    e.hp -= dmg;
    e.hitFlash = 1.f;
    const float knock = cfg::combat::knockback * m.knockMult *   // Big ball / Bumper
                        (b.role == BallRole::Guardian ? cfg::role::guardianKnockMul : 1.f) *
                        (m.rampartKnock > 0.f ? m.rampartKnock : 1.f);    // "Rampart"
    e.vel += -normal * knock * e.knockTaken;
    if (b.role == BallRole::Support) {
        e.mark = cfg::role::markDuration;
        if (m.mastery)   // Support mastery: the mark spreads
            for (Enemy& o : enemies_)
                if (&o != &e && o.hp > 0.f && length(o.pos - e.pos) < S::supportSpread)
                    o.mark = cfg::role::markDuration;
    }
    if (b.role == BallRole::Guardian) e.stagger = std::max(e.stagger, cfg::role::staggerDuration);
    if (m.rampartStagger > 0.f) e.stagger = std::max(e.stagger, cfg::role::staggerDuration * m.rampartStagger);

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
                    (1.f + cfg::element::burnPerEmberLevel * static_cast<float>(p.emberLevel - 1));
    }
    applyElement(e, hitElement(b, p), b.owner, dmg, p, ev);   // a different ball's element waiting -> reaction

    // ---- procs
    if (m.teslaChance > 0.f && chance(m.teslaChance, p)) {   // "Tesla": zap the nearest few
        for (int n = 0; n < m.teslaTargets; ++n) {
            Enemy* t = nullptr;
            float best = S::teslaRange * S::teslaRange;
            for (Enemy& o : enemies_) {
                if (&o == &e || o.hp <= 0.f || o.hitFlash > 0.95f) continue;   // skip ones just hit
                const float d2 = dot(o.pos - e.pos, o.pos - e.pos);
                if (d2 < best) { best = d2; t = &o; }
            }
            if (!t) break;
            damageEnemy(*t, dmg * S::teslaFrac);
            if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                bolts_.push_back(Bolt{e.pos, t->pos, cfg::element::boltLife, cfg::element::boltLife});
        }
    }
    if (b.role == BallRole::Striker && m.mastery &&   // Striker mastery: shockwave on fast hits
        length(b.vel) > ballCruise(b, p) * S::strikerShockSpeed) {
        areaDamage(e.pos, S::strikerShockRadius, dmg * S::strikerShockFrac, &e);
        ev.bursts.push_back({e.pos, S::strikerShockRadius, theme::ballFast, nullptr});
    }
    if (m.resonanceFrac > 0.f) resonate(b, dmg, p, ev);   // "Resonance"
    if (kill) onKill(b, e, dmg, p, ev);
    if (!kill && allowEcho && m.echoChance > 0.f && e.hp > 0.f && chance(m.echoChance, p))   // "Echo"
        return strike(b, e, normal, p, ev, false);
    return kill;
}

// Whatever a ball's items do when it lands a kill.
void World::onKill(Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev) {
    namespace C = cfg::changer;
    const BallMods& m = b.mods;
    ev.midasGold += m.midasGold;                                    // "Midas"
    if (m.bomberChance > 0.f && chance(m.bomberChance, p)) {        // "Bomber": the kill goes off
        areaDamage(e.pos, m.bombRadius, dmg * cfg::synergy::bombFrac, &e);
        ev.bursts.push_back({e.pos, m.bombRadius, theme::elemFire, nullptr});
    }
    if (b.gluttonStacks < m.gluttonMax) ++b.gluttonStacks;         // "Glutton" grows
    if (m.mitosis > 0 && b.scale >= 1.f)                            // "Mitosis": copies don't split again
        for (int i = 0; i < m.mitosis; ++i) spawnMitosis(b, p);
    if (m.blackHoleChance > 0.f && static_cast<int>(blackHoles_.size()) < C::maxBlackHoles &&
        chance(m.blackHoleChance, p)) {                             // "Black hole"
        BlackHole h;
        h.pos = e.pos;
        h.pull = m.blackHolePull;
        h.dmg = std::max(dmg, ballDamage(b, p)) * m.blackHoleFrac;
        h.elem = hitElement(b, p);
        h.owner = b.owner;
        blackHoles_.push_back(h);
    }
}

// A ball leaves its element on an enemy. If a DIFFERENT ball's different
// element is already waiting there, the two react (and the status is used up).
Element World::hitElement(const Ball& b, const WorldParams& p) {
    if (p.pact.alchemy && rng_.range(0.f, 1.f) < cfg::pact::alchemyChance) {   // "Alchemy" pact: a random extra element
        Element e = static_cast<Element>(rng_.irange(1, kElementCount - 1));
        if (e == b.element) e = static_cast<Element>(static_cast<int>(e) % (kElementCount - 1) + 1);
        return e;
    }
    if (b.element != Element::Plain || !p.prismCore) return b.element;
    return static_cast<Element>(rng_.irange(1, kElementCount - 1));   // "Prism core": any element at all
}

void World::applyElement(Enemy& e, Element el, int owner, float hitDmg, const WorldParams& p, FrameEvents& ev) {
    if (el == Element::Plain) return;
    if (e.elemT > 0.f && e.elem != Element::Plain && e.elem != el &&
        (e.elemOwner != owner || p.pact.alchemy)) {   // "Alchemy" pact: a ball can react with itself
        const Element prev = e.elem;
        e.elem = Element::Plain;
        e.elemT = 0.f;
        e.elemOwner = -1;
        triggerReaction(prev, el, e, hitDmg, p, ev, 0);
        return;
    }
    e.elem = el;
    e.elemOwner = owner;
    e.elemT = cfg::synergy::reactWindow;
    // The element splashes onto bare enemies around it, so a pack gets primed
    // and the next ball through sets them off.
    for (Enemy& o : enemies_) {
        if (&o == &e || o.hp <= 0.f || o.elemT > 0.f) continue;
        if (length(o.pos - e.pos) > cfg::synergy::elemSplash + o.radius) continue;
        o.elem = el;
        o.elemOwner = owner;
        o.elemT = cfg::synergy::reactWindow;
    }
}

void World::triggerReaction(Element x, Element y, Enemy& e, float hitDmg, const WorldParams& p,
                            FrameEvents& ev, int depth) {
    namespace S = cfg::synergy;
    auto has = [&](Element q) { return x == q || y == q; };
    const float mul = p.catalyst ? S::catalystDamage : 1.f;
    const float rm = p.catalyst ? S::catalystRadius : 1.f;
    const sf::Vector2f at = e.pos;
    BurstFx fx;
    fx.pos = at;

    if (has(Element::Fire) && has(Element::Ice)) {            // Burst: the ice explodes
        fx = {at, S::burstRadius * rm, theme::elemIce, "BURST"};
        for (Enemy& o : enemies_) {
            if (o.hp <= 0.f || length(o.pos - at) > fx.radius + o.radius) continue;
            damageEnemy(o, hitDmg * S::burstFrac * mul * (o.frozen > 0.f ? 2.f : 1.f));
        }
    } else if (has(Element::Fire) && has(Element::Poison)) {  // Combustion: the poison left goes up at once
        fx = {at, S::combustRadius * rm, theme::elemFire, "COMBUSTION"};
        const float stored = std::max(hitDmg, e.poisonDps * e.poison * S::combustPoison);
        e.poison = 0.f;
        e.poisonDps = 0.f;
        areaDamage(at, fx.radius, stored * mul, nullptr);
    } else if (has(Element::Poison) && has(Element::Electric)) {   // Plague: an arc that poisons
        fx = {at, S::plagueRange * rm * 0.5f, theme::elemPoison, "PLAGUE"};
        const Enemy* from = &e;
        std::vector<const Enemy*> hit{&e};
        for (int n = 0; n < S::plagueTargets; ++n) {
            Enemy* t = nullptr;
            float best = S::plagueRange * rm * S::plagueRange * rm;
            for (Enemy& o : enemies_) {
                if (o.hp <= 0.f || std::find(hit.begin(), hit.end(), &o) != hit.end()) continue;
                const float d2 = dot(o.pos - from->pos, o.pos - from->pos);
                if (d2 < best) { best = d2; t = &o; }
            }
            if (!t) break;
            damageEnemy(*t, hitDmg * S::plagueFrac * mul);
            t->poison = cfg::element::poisonDuration;
            t->poisonDps = std::max(t->poisonDps, cfg::element::poisonDpsPerHit * 2.f * mul);
            if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                bolts_.push_back(Bolt{from->pos, t->pos, cfg::element::boltLife, cfg::element::boltLife});
            hit.push_back(t);
            from = t;
        }
    } else if (has(Element::Water) && has(Element::Electric)) {  // Electrocution: every wake conducts
        fx = {at, S::electrocuteRadius * rm, theme::elemElectric, "ELECTROCUTE"};
        areaDamage(at, fx.radius, hitDmg * S::electrocuteFrac * mul, nullptr);
        for (const Ball& wb : balls_) {
            if (wb.element != Element::Water) continue;
            for (Enemy& o : enemies_) {
                if (o.hp <= 0.f || length(o.pos - at) < fx.radius + o.radius) continue;   // already hit
                for (const sf::Vector2f& pt : wb.waterTrail)
                    if (length(o.pos - pt) < cfg::element::waterTrailWidth * 1.5f + o.radius) {
                        damageEnemy(o, hitDmg * S::electrocuteFrac * mul);
                        break;
                    }
            }
        }
    } else if (has(Element::Water) && has(Element::Fire)) {   // Steam: a scalding cloud
        fx = {at, S::steamRadius * rm, theme::textHi, "STEAM"};
        for (Enemy& o : enemies_) {
            if (o.hp <= 0.f || length(o.pos - at) > fx.radius + o.radius) continue;
            damageEnemy(o, hitDmg * S::steamFrac * mul);
            o.stagger = std::max(o.stagger, S::steamStagger);
        }
    } else if (has(Element::Ice) && has(Element::Electric)) {   // Superconductor: brittle
        fx = {at, S::superRadius * rm, theme::elemElectric, "SUPERCONDUCT"};
        for (Enemy& o : enemies_) {
            if (o.hp <= 0.f || length(o.pos - at) > fx.radius + o.radius) continue;
            o.brittle = S::brittleTime;
            damageEnemy(o, hitDmg * S::superFrac * mul);
        }
    } else {                                                  // any other pair: a clash
        fx = {at, S::clashRadius * rm, theme::accent, "CLASH"};
        areaDamage(at, fx.radius, hitDmg * S::clashFrac * mul, nullptr);
    }
    ev.bursts.push_back(fx);

    // "Chain reaction": it can go off again on another afflicted enemy - and again.
    if (p.chainReaction && depth < S::chainMaxDepth && chance(S::chainChance, p)) {
        Enemy* next = nullptr;
        float best = S::chainRange * S::chainRange;
        for (Enemy& o : enemies_) {
            if (&o == &e || o.hp <= 0.f) continue;
            const bool afflicted = o.poison > 0.f || o.frozen > 0.f || o.burn > 0.f || o.mark > 0.f ||
                                   o.brittle > 0.f || o.elemT > 0.f;
            if (!afflicted) continue;
            const float d2 = dot(o.pos - at, o.pos - at);
            if (d2 < best) { best = d2; next = &o; }
        }
        if (next) triggerReaction(x, y, *next, hitDmg, p, ev, depth + 1);
    }
}

// "Split shot": a short-lived copy with the same items, veering off.
void World::spawnGhost(const Ball& parent) {
    if (static_cast<int>(ghosts_.size() + pendingGhosts_.size()) >= cfg::synergy::maxGhosts) return;
    Ball g = parent;
    g.ghost = true;
    g.ghostLife = cfg::synergy::ghostLife;
    g.held = false;
    g.trail.clear();
    g.waterTrail.clear();
    const float a = rng_.range(-1.f, 1.f) > 0.f ? cfg::synergy::ghostSpread : -cfg::synergy::ghostSpread;
    const float c = std::cos(a), sn = std::sin(a);
    g.vel = {g.vel.x * c - g.vel.y * sn, g.vel.x * sn + g.vel.y * c};
    pendingGhosts_.push_back(g);
}

// "Mitosis": a small copy with the same items splits off a kill.
void World::spawnMitosis(const Ball& parent, const WorldParams& p) {
    if (static_cast<int>(ghosts_.size() + pendingGhosts_.size()) >= cfg::synergy::maxGhosts) return;
    Ball g = parent;
    g.ghost = true;
    g.twin = false;
    g.scale = cfg::changer::mitosisScale;
    g.ghostLife = parent.mods.mitosisLife;
    g.age = 0.f;
    g.held = false;
    g.charged = false;
    g.homing = false;
    g.gluttonStacks = 0;
    g.trail.clear();
    g.waterTrail.clear();
    const float sp = std::max(length(parent.vel), ballCruise(parent, p) * cfg::changer::mitosisSpeed);
    g.vel = rng_.direction() * sp;
    pendingGhosts_.push_back(g);
}

// Guardian mastery: a core bounce throws a shockwave that shoves everything
// near the core back out and staggers it.
void World::guardianPulse(FrameEvents& ev) {
    namespace S = cfg::synergy;
    for (Enemy& e : enemies_) {
        if (e.hp <= 0.f || e.orbiter) continue;
        const sf::Vector2f d = e.pos - core_.pos;
        if (length(d) > S::guardianPulseRadius + e.radius) continue;
        e.vel += normalized(d, {1.f, 0.f}) * S::guardianPulseKnock * e.knockTaken;
        e.stagger = std::max(e.stagger, S::guardianPulseStagger);
    }
    ev.bursts.push_back({core_.pos, S::guardianPulseRadius, theme::core, nullptr});
}

float World::ballDamage(const Ball& b, const WorldParams& p) const {
    const float speed = length(b.vel);
    const float ratio = speed / (cfg::ball::baseCruise * arenaScale());   // on-screen speed, not world
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
    if (b.mods.satellite) dmg *= b.mods.satelliteDamage;   // "Satellite" grinds
    if (b.mods.piercing) dmg *= b.mods.pierceMult;         // "Piercing" levels
    dmg *= 1.f + b.mods.gluttonDamage * static_cast<float>(b.gluttonStacks);   // "Glutton"
    if (b.element == Element::Fire)   // fire is a heavier hit; the burn DoT is the "Ember" node
        dmg *= 1.f + cfg::element::fireDamageBonus * elemPotency(b, p);
    if (b.ricochetT > 0.f) dmg *= b.mods.ricochetMult;   // "Ricochet": fresh off a wall
    if (b.pactCharge > 0.f) dmg *= cfg::pact::coreChargeDamage;   // "Living Core" pact: overcharged
    return dmg;
}

// Fire / poison / ice act on contact (see advanceBall); water, stone and
// electric emit into the world on a per-ball timer.
void World::emitElement(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
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
                                   (b.mods.bedrockLife > 0.f ? b.mods.bedrockLife : 1.f);   // "Bedrock"
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
            damageEnemy(*target, zap * (target->mark > 0.f ? cfg::role::markDamageMul : 1.f));
            target->hitFlash = 1.f;
            applyElement(*target, b.element, b.owner, ballDamage(b, p), p, ev);   // zaps can set off reactions too
            if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                bolts_.push_back(Bolt{b.pos, target->pos,
                                      cfg::element::boltLife, cfg::element::boltLife});

            // "Conductor": the arc jumps on, one more enemy per level.
            std::vector<const Enemy*> chain{target};
            float jump = zap;
            for (int n = 0; n < b.mods.conductorJumps; ++n) {
                const Enemy* from = chain.back();
                Enemy* next = nullptr;
                float nd2 = cfg::combat::conductorRange * cfg::combat::conductorRange;
                for (Enemy& e : enemies_) {
                    if (e.hp <= 0.f || std::find(chain.begin(), chain.end(), &e) != chain.end()) continue;
                    const float d2 = dot(e.pos - from->pos, e.pos - from->pos);
                    if (d2 < nd2) { nd2 = d2; next = &e; }
                }
                if (!next) break;
                jump *= cfg::combat::conductorFalloff;
                damageEnemy(*next, jump);
                if (static_cast<int>(bolts_.size()) < cfg::element::maxBolts)
                    bolts_.push_back(Bolt{from->pos, next->pos, cfg::element::boltLife, cfg::element::boltLife});
                chain.push_back(next);
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
    const float decayRate = cfg::ball::decayRate * b.mods.flingDecay * p.pact.flingHold *   // "Hot Hands" pact
                            (b.role == BallRole::Striker ? cfg::role::strikerFlingDecay : 1.f);
    const float down = 1.f - std::exp(-decayRate * dt);
    const float k = (sp < cruiseS) ? up : down;
    const float ns = std::min(lerpf(sp, cruiseS, k), vMax);
    b.vel *= ns / sp;
}

void World::updateTrail(Ball& b) {
    b.trail.push_back(b.pos);
    const std::size_t cap =
        static_cast<std::size_t>(clampf(4.f + length(b.vel) / (90.f * arenaScale()), 4.f, 16.f));
    while (b.trail.size() > cap) b.trail.pop_front();
}

void World::advanceBall(Ball& b, float dt, const WorldParams& p, FrameEvents& ev) {
    b.age += dt;
    b.pactCharge = std::max(0.f, b.pactCharge - dt);   // "Living Core" overcharge wears off
    b.radius = ballRadius(b, p);   // role, "Big ball" gear, "Mass" web
    b.resonanceT = std::max(0.f, b.resonanceT - dt);
    if (b.mods.stormFrac > 0.f) updateStorm(b, dt, p, ev);
    if (b.mods.satellite) {
        advanceSatellite(b, dt, p, ev);
        return;
    }
    if (b.mods.seekerTurn > 0.f) {   // "Seeker": bend the heading toward the nearest enemy, a little at a time
        const Enemy* t = nullptr;
        float best = b.mods.seekerRange * arenaScale();
        best *= best;
        for (const Enemy& e : enemies_) {
            if (e.hp <= 0.f) continue;
            const float d2 = dot(e.pos - b.pos, e.pos - b.pos);
            if (d2 < best) { best = d2; t = &e; }
        }
        const float sp = length(b.vel);
        if (t && sp > 1e-3f) {
            const float cur = std::atan2(b.vel.y, b.vel.x);
            const float want = std::atan2(t->pos.y - b.pos.y, t->pos.x - b.pos.x);
            float diff = std::remainder(want - cur, 2.f * kPi);
            const float maxTurn = b.mods.seekerTurn * dt;
            diff = clampf(diff, -maxTurn, maxTurn);
            b.vel = sf::Vector2f{std::cos(cur + diff), std::sin(cur + diff)} * sp;
        }
    }

    if (b.mods.hunterMult > 0.f) steerHunter(b, dt);   // "Hunter"
    if (b.homing) {   // "Boomerang": curve home to the core
        const float sp = length(b.vel);
        const sf::Vector2f home = core_.pos - b.pos;
        if (sp > 1e-3f) {
            const float cur = std::atan2(b.vel.y, b.vel.x);
            const float want = std::atan2(home.y, home.x);
            const float maxTurn = cfg::changer::boomerangTurn * dt;
            const float diff = clampf(std::remainder(want - cur, 2.f * kPi), -maxTurn, maxTurn);
            b.vel = sf::Vector2f{std::cos(cur + diff), std::sin(cur + diff)} * sp;
        }
    }

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
        fx.speed = length(b.vel) / arenaScale();   // sound / ring size read on-screen speed
        ev.bounces.push_back(fx);
    };

    for (int s = 0; s < steps; ++s) {
        b.pos += b.vel * h;

        if (collision::Contact c = collision::circleVsBounds(b, size_); c.hit) {
            afterBounce(b, c.normal, false);
            aimBounce(b, c.normal, nullptr);
            if (b.mods.ricochetMult > 0.f) {   // "Ricochet": a speed kick and an armed hit
                boostSpeed(b, b.mods.wallBoost, p);
                b.ricochetT = cfg::combat::ricochetWindow;
            }
            if (b.mods.splitChance > 0.f && !b.ghost && chance(b.mods.splitChance, p)) spawnGhost(b);
            b.berserkStacks = 0;   // "Berserk" resets on a wall
            if (b.mods.railFrac > 0.f) fireRail(b, p, ev);
            pactWallBump(b, c.point, p, ev);   // "Pinball" pact
            pushFx(c);
        }
        // The core is solid: balls bounce off it (no damage to the core).
        if (collision::Contact c =
                collision::circleVsSolidCircle(b, core_.pos, core_.radius, 1.f);
            c.hit) {
            afterBounce(b, c.normal, false);
            aimBounce(b, c.normal, nullptr, p.magneticCore);   // "Magnetic core": every ball aims
            boostSpeed(b, p.coreBounceBoost, p);   // "Spring core" relic
            if (b.mods.menderHeal > 0.f && !b.ghost)   // "Mender": the core patches itself up
                core_.hp = std::min(core_.maxHp, core_.hp + b.mods.menderHeal);
            if (b.role == BallRole::Guardian && b.mods.mastery) guardianPulse(ev);
            if (b.mods.boomerangHit > 0.f) {   // "Boomerang": home - charge up and fly at the threat
                b.homing = false;
                b.charged = true;
                aimBounce(b, c.normal, nullptr, true);
                boostSpeed(b, b.mods.boomerangKick, p);
            }
            pactCoreBounce(b, p, ev);   // "Living Core" pact
            pushFx(c);
        }
        if (b.mods.piercing) {   // "Piercing": no bounce - every enemy on the path takes the hit
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.pierceCd > 0.f) continue;
                const sf::Vector2f d = b.pos - e.pos;
                const float dl = length(d);
                if (dl > b.radius + e.radius) continue;
                const sf::Vector2f n = normalized(d, {1.f, 0.f});
                e.pierceCd = cfg::changer::pierceCooldown;
                if (b.role != BallRole::Guardian && shieldBlocks(e, b.pos, core_.pos)) continue;
                strike(b, e, n, p, ev);
                pushFx({true, n, e.pos + n * e.radius});
            }
        }
        for (Enemy& e : enemies_) {
            if (b.mods.piercing) break;
            if (e.hp <= 0.f) continue;   // already dead this frame (not swept yet) - don't re-hit / double-splash
            // "Comet": flying hot it plows through instead of bouncing.
            const sf::Vector2f v0 = b.vel;
            const bool plowing = b.mods.cometPlow > 0.f && length(v0) > ballCruise(b, p) * b.mods.cometPlow;
            if (plowing && e.pierceCd > 0.f) continue;   // just went through this one
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
            const bool kill = strike(b, e, c.normal, p, ev);
            if ((b.mods.cleave && kill) || plowing) {   // "Cleave" / "Comet": keep flying straight through
                b.vel = v0;
                if (plowing) e.pierceCd = cfg::changer::pierceCooldown;
                addComboHit();
                pushFx(c);
                continue;
            }
            afterBounce(b, c.normal, true);
            aimBounce(b, c.normal, &e);
            if (b.mods.boomerangHit > 0.f) b.homing = true;   // "Boomerang": head home
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
    emitElement(b, dt, p, ev);
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
            if (a.mods.satellite || b.mods.satellite) continue;   // orbits are fixed paths
            if (!collision::resolveBallPair(a, b)) continue;

            // "Bumper": the other ball is launched off it, at least this x its cruise.
            auto bump = [&](Ball& o, float mult) {
                if (mult <= 0.f) return;
                const float sp = std::max(length(o.vel), ballCruise(o, p));
                o.vel = normalized(o.vel, {1.f, 0.f}) * std::min(sp * mult, ballMaxSpeed(o, p));
            };
            bump(b, a.mods.bumperBoost);
            bump(a, b.mods.bumperBoost);

            const sf::Vector2f n = normalized(b.pos - a.pos);
            a.squash = b.squash = 1.f;
            a.squashAxis = b.squashAxis = n;

            BounceFx fx;
            fx.pos = (a.pos + b.pos) * 0.5f;
            fx.normal = n;
            fx.color = lerpColor(a.color, b.color, 0.5f);
            fx.speed = std::max(length(a.vel), length(b.vel)) / arenaScale();
            fx.ballPair = true;
            ev.bounces.push_back(fx);
            pactClack(a, b, fx.pos, p, ev);   // "Legion" pact
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
void World::updateWaterTrails(float dt, const WorldParams& p, FrameEvents& ev) {
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
                    damageEnemy(e, cfg::element::waterDps * dt);
                    e.hitFlash = std::max(0.f, e.hitFlash - 0.6f);   // a wake doesn't flash like a hit
                    applyElement(e, b.element, b.owner, ballDamage(b, p), p, ev);   // wading into a wake soaks it
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
            if (length(e.pos - it->pos) < it->radius + e.radius) {
                damageEnemy(e, cfg::element::stoneDps * dt);
                e.hitFlash = std::max(0.f, e.hitFlash - 0.6f);
            }
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
        e.age += dt;
        e.mark = std::max(0.f, e.mark - dt);
        e.brittle = std::max(0.f, e.brittle - dt);
        e.pierceCd = std::max(0.f, e.pierceCd - dt);
        if (e.elemT > 0.f && (e.elemT -= dt) <= 0.f) { e.elem = Element::Plain; e.elemOwner = -1; }

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
            if (p.timeDilation) edt *= cfg::changer::timeDilation;   // "Time dilation"
            edt *= p.pact.enemySpeedMul;                              // "Living Core" pact's cost

            e.vel += (steer - e.vel) * (1.f - std::exp(-8.f * edt));
            e.pos += e.vel * edt;

            // "Gravity well" balls drag enemies in toward them (strongest up close).
            for (const std::vector<Ball>* set : {&balls_, &ghosts_})
                for (const Ball& gb : *set) {
                    if (gb.mods.gravityMult <= 0.f) continue;
                    const sf::Vector2f to = gb.pos - e.pos;
                    const float R = cfg::changer::gravityRadius * arenaScale() *
                                    (1.f + 0.4f * (gb.mods.gravityMult - 1.f));
                    const float dd = length(to);
                    if (dd > R || dd < 1e-3f) continue;
                    e.pos += to / dd * cfg::changer::gravityPull * gb.mods.gravityMult * arenaScale() *
                             (1.f - dd / R) * edt;
                }

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
                else core_.hp -= e.coreDamage * p.pact.coreDamageMul;   // "Bloodlust" pact's cost
            }
            core_.hitFlash = 1.f;
            coreHitThisWave_ = true;   // "Interest" is off for this wave now
            ev.coreHit = true;

            it = enemies_.erase(it);
            pactCoreHit(p, ev);   // "Fortress" blast / "Bloodlust" wipe

            if (core_.hp <= 0.f && phoenixLeft_ > 0) {   // "Phoenix": back from the ashes, once an act
                --phoenixLeft_;
                core_.hp = core_.maxHp * cfg::changer::phoenixHeal;
                ev.phoenix = true;
            }
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
                    s.id = nextEnemyId_++;
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
        boss_.pos += boss_.vel * dt * (p.timeDilation ? cfg::changer::timeDilation : 1.f);   // Charger: dead straight
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

    advanceCombo(dt, p);
    updateCoreSlide(dt);
    updateAutoFling(dt, p, ev);
    updateHunters(dt, p);                 // "Hunters" pact

    for (std::size_t i = 0; i < balls_.size(); ++i) {
        Ball& b = balls_[i];
        if (grabbed_ == Grabbed::Ball && static_cast<int>(i) == heldIndex_) {
            b.squash *= std::exp(-cfg::ball::squashDecay * dt);
            continue;
        }
        advanceBall(b, dt, p, ev);
    }
    updateTwins(p);
    for (Ball& g : ghosts_) {   // "Split shot" copies fly and hit like the real thing, then fade
        advanceBall(g, dt, p, ev);
        g.ghostLife -= dt;
    }
    ghosts_.erase(std::remove_if(ghosts_.begin(), ghosts_.end(),
                                 [](const Ball& g) { return g.ghostLife <= 0.f; }),
                  ghosts_.end());
    ghosts_.insert(ghosts_.end(), pendingGhosts_.begin(), pendingGhosts_.end());
    pendingGhosts_.clear();

    resolveBallPairs(ev, p);
    updateTethers(dt, p, ev);
    updateBlackHoles(dt, p, ev);
    updateBolts(dt);
    updateWaterTrails(dt, p, ev);
    updateObstacles(dt);
    updateEnemies(dt, p, ev);
    updateCoreZap(dt, p, ev);   // "Living Core" pact
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
