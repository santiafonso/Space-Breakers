#pragma once

#include <string>

#include "core/Math.hpp"
#include "core/Theme.hpp"
#include "progression/GameData.hpp"

namespace sb {

// Small shared drawing helpers used across every screen.
sf::Text makeText(const sf::Font& font, const std::string& str, unsigned size, sf::Color color);
void centerOrigin(sf::Text& t);

// Draw a string centred on `pos`.
void drawCentered(sf::RenderWindow& window, const sf::Font& font, const std::string& str,
                  unsigned size, sf::Vector2f pos, sf::Color color);

// Full-screen dim, used behind pause / shop overlays.
void drawDim(sf::RenderWindow& window, sf::Vector2f size, float alpha);

// The lifetime records panel (shared by the Stats screen).
void drawStatsPanel(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f size,
                    const Stats& stats);

}  // namespace sb
