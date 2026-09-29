#pragma once

#include "sim/World.hpp"

namespace sb {

// Draws the simulation: water trails / obstacles and the core behind, then enemies,
// bolts, pickups and balls. Stateless - hand it a window and a World.
class WorldRenderer {
public:
    void draw(sf::RenderWindow& window, const World& world) const;

private:
    void drawCore(sf::RenderWindow& window, const Core& c) const;
    void drawBoss(sf::RenderWindow& window, const Boss& b, sf::Vector2f corePos) const;
    void drawEnemy(sf::RenderWindow& window, const Enemy& e, sf::Vector2f corePos) const;
    void drawWave(sf::RenderWindow& window, const Wave& w, float arenaScale) const;
    void drawObstacle(sf::RenderWindow& window, const Obstacle& o) const;
    void drawBolt(sf::RenderWindow& window, const Bolt& bo) const;
    void drawPickup(sf::RenderWindow& window, const Pickup& pu) const;
    void drawBall(sf::RenderWindow& window, const Ball& b,
                  const std::optional<ActiveEffect>& effect) const;
};

}  // namespace sb
