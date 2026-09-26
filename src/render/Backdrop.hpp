#pragma once

#include <SFML/Graphics.hpp>

namespace sb::backdrop {

// What sits behind every screen: flat ink, a faint dot grid (the "graph paper"
// of the console) and a vignette that darkens the corners. Cheap - the grid is
// built once per screen size.
void draw(sf::RenderTarget& t, sf::Vector2f size);

}  // namespace sb::backdrop
