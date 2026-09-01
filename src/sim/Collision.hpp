#pragma once

#include "sim/Entities.hpp"

// Pure collision resolution. These helpers mutate the entity they are handed
// (position pushed out of penetration, velocity reflected) and report the
// contact so the caller can spawn feedback. They know nothing about the World.
namespace sb::collision {

struct Contact {
    bool hit = false;
    sf::Vector2f normal;  // unit surface normal, pointing toward the ball
    sf::Vector2f point;   // where the hit happened, for ring / edge feedback
};

// Circle against the inside of the arena rectangle spanning [0, size].
Contact circleVsBounds(Ball& b, sf::Vector2f size);

// Circle against one (possibly moving) axis-aligned wall. Reflection happens in
// the wall's reference frame, so a drifting wall imparts a kick.
Contact circleVsWall(Ball& b, const Wall& w);

// Push a held / parked ball out of a wall it overlaps. No velocity change.
void pushOutOfWall(Ball& b, const Wall& w);

// Equal-mass elastic response for a pair of overlapping balls: separate them,
// then exchange the velocity component along the contact normal.
void resolveBallPair(Ball& a, Ball& b);

}  // namespace sb::collision
