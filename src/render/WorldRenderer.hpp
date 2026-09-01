#pragma once

#include "sim/World.hpp"

namespace sb {

// Draws the simulation: walls first, then pickups, then balls (with their trails
// and squash). Stateless - hand it a window and a World.
class WorldRenderer {
public:
    void draw(sf::RenderWindow& window, const World& world) const;

private:
    void drawWall(sf::RenderWindow& window, const Wall& w) const;
    void drawPickup(sf::RenderWindow& window, const Pickup& pu) const;
    void drawBall(sf::RenderWindow& window, const Ball& b,
                  const std::optional<ActiveEffect>& effect) const;
};

}  // namespace sb
