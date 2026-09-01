#pragma once

#include <optional>
#include <vector>

#include "core/Config.hpp"
#include "sim/Entities.hpp"

namespace sb {

enum class Grabbed { None, Ball };

// The simulation: elemental balls orbiting a central core, clearing waves of
// enemies that march on it. Knows nothing about rendering, input or progression
// storage.
class World {
public:
    explicit World(sf::Vector2f size);

    // ---- run / wave lifecycle -----------------------------------------
    void startRun(const WorldParams& p, const std::vector<int>& ballElements,
                  float coreHp, float coreMaxHp);
    void startWave(int wave, const WorldParams& p);
    void addBall(Element e, const WorldParams& p);
    void repairCore(float amount);

    FrameEvents step(float dt, const WorldParams& p);

    // ---- grab / throw: knock a ball off its orbit -------------------
    bool grabAt(sf::Vector2f point, float catchRadius);
    bool hasHeld() const { return grabbed_ != Grabbed::None; }
    Grabbed grabbedKind() const { return grabbed_; }
    void moveHeld(sf::Vector2f target);
    void releaseHeld(sf::Vector2f throwVel);
    void forceRelease();

    // ---- read-only views --------------------------------------------
    const std::vector<Ball>& balls() const { return balls_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<Projectile>& projectiles() const { return projectiles_; }
    const std::vector<Puddle>& puddles() const { return puddles_; }
    const std::vector<Obstacle>& obstacles() const { return obstacles_; }
    const std::vector<Pickup>& pickups() const { return pickups_; }
    const Core& core() const { return core_; }
    const std::optional<ActiveEffect>& effect() const { return effect_; }
    sf::Vector2f size() const { return size_; }

    bool waveRunning() const { return waveRunning_; }
    bool runOver() const { return runOver_; }
    int wave() const { return wave_; }
    int enemiesLeft() const { return static_cast<int>(enemies_.size()) + toSpawn_; }

    float cruiseBase(const WorldParams& p) const;
    float cruiseSpeed(const WorldParams& p) const;
    float maxSpeed(const WorldParams& p) const;
    int comboStreak() const { return comboStreak_; }
    int comboTier() const {
        return std::min(comboStreak_ / cfg::combo::bouncesPerTier, comboCapTier_);
    }
    float comboMultiplier() const {
        return 1.f + static_cast<float>(comboTier()) * cfg::combo::multiplierPerTier;
    }
    float fastestBall() const;

private:
    void spawnBall(Element e, const WorldParams& p);
    void spawnEnemy();
    void advanceCombo(float dt);
    void advanceBall(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    void orbitAssist(Ball& b, float dt, const WorldParams& p) const;
    void emitElement(Ball& b, float dt, const WorldParams& p);
    void resolveBallPairs();
    void updateProjectiles(float dt);
    void updatePuddles(float dt);
    void updateObstacles(float dt);
    void updateEnemies(float dt, FrameEvents& ev);
    void sweepDeadEnemies(const WorldParams& p, FrameEvents& ev);
    void updateWaveSpawner(float dt, FrameEvents& ev);
    void updatePickups(float dt, FrameEvents& ev);
    void advanceEffect(float dt);
    void afterBounce(Ball& b, sf::Vector2f normal, bool countHit);
    void regulateSpeed(Ball& b, const WorldParams& p);
    void updateTrail(Ball& b);
    float ballDamage(const Ball& b, const WorldParams& p) const;

    sf::Vector2f size_;
    std::vector<Ball> balls_;
    std::vector<Enemy> enemies_;
    std::vector<Projectile> projectiles_;
    std::vector<Puddle> puddles_;
    std::vector<Obstacle> obstacles_;
    std::vector<Pickup> pickups_;
    Core core_;
    std::optional<ActiveEffect> effect_;
    Rng rng_;

    Grabbed grabbed_ = Grabbed::None;
    int heldIndex_ = -1;

    int comboStreak_ = 0;
    int comboCapTier_ = cfg::combo::baseCapTier;
    float sinceHit_ = 0.f;
    int reportedTier_ = 0;

    int wave_ = 0;
    bool waveRunning_ = false;
    bool runOver_ = false;
    int toSpawn_ = 0;
    float spawnTimer_ = 0.f;

    float pickupTimer_ = cfg::pickup::spawnMin;
};

}  // namespace sb
