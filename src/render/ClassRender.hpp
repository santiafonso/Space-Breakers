#pragma once

#include <SFML/Graphics.hpp>

#include "sim/World.hpp"

// Class visuals (the class framework, 2026-09-26), one section per class in
// render/ClassRender.cpp. Kept minimal: the ball stays the focus.
namespace sb {

// A class's mark on a ball body of radius `r` (world view and the UI loadout
// panels). `heading` = the ball's direction of travel (radians), `alpha` 0..1.
// A ball with two classes draws both marks, so marks must layer cleanly.
void drawClassMark(sf::RenderTarget& t, BallRole role, sf::Vector2f pos, float r, float heading, float alpha);

// Per-class things on the field (bullets, turrets, summons...), drawn over
// the enemies and under the balls. World view.
void drawClassWorld(sf::RenderTarget& t, const World& world);

}  // namespace sb
