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

// Eased "pop in" factor for an element that starts appearing `delay` seconds
// into a screen's intro and settles over `dur`. Overshoots slightly past 1 for
// a springy feel; clamp to [0,1] when driving alpha.
float introPop(float elapsed, float delay, float dur = 0.30f);

// Like drawCentered, but the text springs up to size and fades in as `pop`
// goes 0 -> ~1.1 (pass an introPop(...) value). A pop <= 0 draws nothing.
void drawCenteredPop(sf::RenderWindow& window, const sf::Font& font, const std::string& str,
                     unsigned size, sf::Vector2f pos, sf::Color color, float pop);

// Full-screen dim, used behind pause / shop overlays.
void drawDim(sf::RenderWindow& window, sf::Vector2f size, float alpha);

// The lifetime records panel (shared by the Stats screen). `intro` staggers the
// rows popping in; pass a large value for no animation.
void drawStatsPanel(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f size,
                    const Stats& stats, float intro = 1e6f);

}  // namespace sb
