#pragma once

#include "sim/Entities.hpp"

// Pure collision resolution. These helpers mutate the ball they are handed
// (position pushed out of penetration, velocity reflected) and report the
// contact so the caller can spawn feedback and apply damage.
namespace sb::collision {

struct Contact {
    bool hit = false;
    sf::Vector2f normal;  // unit surface normal, pointing toward the ball
    sf::Vector2f point;   // where the hit happened, for ring / edge feedback
};

// Ball against the inside of the arena rectangle spanning [0, size].
Contact circleVsBounds(Ball& b, sf::Vector2f size);

// Ball against a solid circular obstacle (an enemy or the core). Pushes the ball
// clear and reflects the radial part of its velocity, scaled by `rebound`
// (1 = elastic, 0 = the ball just grazes past).
Contact circleVsSolidCircle(Ball& b, sf::Vector2f center, float radius, float rebound);

// Equal-mass elastic response for a pair of overlapping balls.
void resolveBallPair(Ball& a, Ball& b);

}  // namespace sb::collision
