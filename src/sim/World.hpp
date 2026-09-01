#pragma once

#include <optional>
#include <vector>

#include "core/Config.hpp"
#include "sim/Entities.hpp"

namespace sb {

enum class Grabbed { None, Ball, Field };

// The simulation: balls clearing enemies that march on a central core, their
// paths bent by placed field structures. Knows nothing about rendering, input
// devices, menus or progression storage.
class World {
public:
    explicit World(sf::Vector2f size);

    // ---- run / wave lifecycle --------------------------------------------
    void startRun(const WorldParams& p, int ballCount, float coreHp, float coreMaxHp,
                  const std::vector<FieldSnapshot>& field);
    void startWave(int wave, const WorldParams& p);
    void setBallCount(int n, const WorldParams& p);
    void addField(FieldKind kind, sf::Vector2f pos, float strength);
    void repairCore(float amount);
    std::vector<FieldSnapshot> fieldSnapshot() const;

    FrameEvents step(float dt, const WorldParams& p);

    // ---- grab / throw: a field structure (priority) or the nearest ball --
    bool grabAt(sf::Vector2f point, float catchRadius);
    bool hasHeld() const { return grabbed_ != Grabbed::None; }
    Grabbed grabbedKind() const { return grabbed_; }
    void moveHeld(sf::Vector2f target);
    void releaseHeld(sf::Vector2f throwVel);
    void forceRelease();

    // ---- read-only views -----------------------------------------------
    const std::vector<Ball>& balls() const { return balls_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<FieldObject>& field() const { return field_; }
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
    void configureField(const std::vector<FieldSnapshot>& snap);
    void spawnBallAtCore(const WorldParams& p);
    void spawnEnemy();
    void advanceCombo(float dt);
    void advanceBall(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    void applyFieldForce(Ball& b, float dt) const;
    void applyHoming(Ball& b, float dt) const;
    void resolveBallPairs();
    void sweepDeadEnemies(const WorldParams& p, FrameEvents& ev);
    void updateEnemies(float dt, FrameEvents& ev);
    void updateWaveSpawner(float dt, FrameEvents& ev);
    void updatePickups(float dt, FrameEvents& ev);
    void advanceEffect(float dt);
    void afterBounce(Ball& b, sf::Vector2f normal, bool countHit);
    void regulateSpeed(Ball& b, float dt, const WorldParams& p);
    void updateTrail(Ball& b);
    float ballDamage(const Ball& b, const WorldParams& p) const;
    bool fieldHit(const FieldObject& f, sf::Vector2f point) const;

    sf::Vector2f size_;
    std::vector<Ball> balls_;
    std::vector<Enemy> enemies_;
    std::vector<FieldObject> field_;
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
