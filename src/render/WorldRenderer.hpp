#pragma once

#include "sim/World.hpp"

namespace sb {

// Draws the simulation: field structures and the core behind, then enemies,
// pickups and balls. Stateless - hand it a window and a World.
class WorldRenderer {
public:
    void draw(sf::RenderWindow& window, const World& world) const;

private:
    void drawField(sf::RenderWindow& window, const FieldObject& f) const;
    void drawCore(sf::RenderWindow& window, const Core& c) const;
    void drawEnemy(sf::RenderWindow& window, const Enemy& e) const;
    void drawPickup(sf::RenderWindow& window, const Pickup& pu) const;
    void drawBall(sf::RenderWindow& window, const Ball& b,
                  const std::optional<ActiveEffect>& effect) const;
};

}  // namespace sb
