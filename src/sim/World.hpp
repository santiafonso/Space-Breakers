#pragma once

#include <optional>
#include <vector>

#include "core/Config.hpp"
#include "sim/Entities.hpp"

namespace sb {

enum class Grabbed { None, Ball, Wall };

// The simulation: balls bouncing in an arena, movable inner walls, drifting
// power-up orbs and the combo streak. Knows nothing about rendering, input
// devices or menus.
class World {
public:
    explicit World(sf::Vector2f size);

    void reset(int multiballLevel, const WorldParams& p, const std::vector<WallSnapshot>& walls);
    void setMultiball(int multiballLevel, const WorldParams& p);
    void configureWalls(int wallLevel, const std::vector<WallSnapshot>& saved);
    void syncWallCount(int wallLevel);
    std::vector<WallSnapshot> wallSnapshot() const;

    // Advance the simulation by one fixed slice.
    FrameEvents step(float dt, const WorldParams& p);

    // Grab / throw. One thing at a time: a wall (priority) or the nearest ball.
    bool grabAt(sf::Vector2f point, float catchRadius);
    bool hasHeld() const { return grabbed_ != Grabbed::None; }
    Grabbed grabbedKind() const { return grabbed_; }
    void moveHeld(sf::Vector2f target);
    void releaseHeld(sf::Vector2f throwVel);
    void forceRelease();
    bool toggleDriftAt(sf::Vector2f point);  // right-click: wall start / stop drifting

    const std::vector<Ball>& balls() const { return balls_; }
    const std::vector<Pickup>& pickups() const { return pickups_; }
    const std::vector<Wall>& walls() const { return walls_; }
    const std::optional<ActiveEffect>& effect() const { return effect_; }
    sf::Vector2f size() const { return size_; }

    float cruiseBase(const WorldParams& p) const;
    float cruiseSpeed(const WorldParams& p) const;  // cruiseBase with power-up buffs
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
    void spawnBall(const WorldParams& p);
    void advanceCombo(float dt, const WorldParams& p);
    void advanceBall(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    void resolveBallPairs();
    void updateWalls(float dt);
    void updatePickups(float dt, const WorldParams& p, FrameEvents& ev);
    void advanceEffect(float dt);
    void afterBounce(Ball& b, sf::Vector2f normal);
    void regulateSpeed(Ball& b, float dt, const WorldParams& p);
    void updateTrail(Ball& b);
    bool wallHit(const Wall& w, sf::Vector2f point) const;
    int awardPoints(const WorldParams& p) const;  // points for one bounce, with buffs

    sf::Vector2f size_;
    std::vector<Ball> balls_;
    std::vector<Wall> walls_;
    std::vector<Pickup> pickups_;
    std::optional<ActiveEffect> effect_;
    Rng rng_;

    Grabbed grabbed_ = Grabbed::None;
    int heldIndex_ = -1;
    int comboStreak_ = 0;
    int comboCapTier_ = cfg::combo::baseCapTier;
    float sinceBounce_ = 0.f;
    int reportedTier_ = 0;
    float pickupTimer_ = cfg::pickup::spawnMin;
};

}  // namespace sb
