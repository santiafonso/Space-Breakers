#pragma once

#include "sim/World.hpp"

namespace sb {

// Draws the simulation: puddles / obstacles and the core behind, then enemies,
// projectiles, pickups and balls. Stateless - hand it a window and a World.
class WorldRenderer {
public:
    void draw(sf::RenderWindow& window, const World& world) const;

private:
    void drawCore(sf::RenderWindow& window, const Core& c) const;
    void drawEnemy(sf::RenderWindow& window, const Enemy& e) const;
    void drawPuddle(sf::RenderWindow& window, const Puddle& p) const;
    void drawObstacle(sf::RenderWindow& window, const Obstacle& o) const;
    void drawProjectile(sf::RenderWindow& window, const Projectile& pr) const;
    void drawPickup(sf::RenderWindow& window, const Pickup& pu) const;
    void drawBall(sf::RenderWindow& window, const Ball& b,
                  const std::optional<ActiveEffect>& effect) const;
};

}  // namespace sb
