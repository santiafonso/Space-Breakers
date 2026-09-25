#pragma once

#include <string>
#include <vector>

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

// Greedy word-wrap: break `str` into lines no wider than `maxW` at `size`.
std::vector<std::string> wrapText(const sf::Font& font, const std::string& str, unsigned size,
                                  float maxW);

// ---- ball loadout panels: one ball's look, role, 4 item slots, modifiers ----
inline constexpr float kPanelW = 190.f;
inline constexpr float kPanelH = 250.f;
inline constexpr float kPanelGap = 16.f;
inline constexpr float kSlotH = 24.f;
inline constexpr float kSlotStep = 29.f;

sf::Vector2f panelCenter(sf::Vector2f size, int i, int n, float cy);   // i of n, in a row
sf::FloatRect slotRect(sf::Vector2f panelCentre, int slot);
std::string modifierLine(const BallLoadout& L);
sf::Color catColor(UpgradeCat c);
sf::Color tagColor(ItemTag t);   // Striker / Guardian / Support item colour
sf::Color tierColor(Tier t);     // Common grey .. Legendary gold
// A pick's card frame: tier-coloured fill + outline; Epic / Legendary get a
// pulsing halo so the rare ones jump out. `time` drives the pulse.
void drawTierFrame(sf::RenderWindow& w, sf::FloatRect r, Tier t, float hover, float alpha, float time);

// What part of a loadout panel centred at `c` the pointer is on: 0..3 = item
// slot, kPanelPartBall = the ball / role name, kPanelPartMods = the modifier
// line, -1 = nothing.
inline constexpr int kPanelPartBall = 10;
inline constexpr int kPanelPartMods = 11;
int panelPartAt(sf::Vector2f c, sf::Vector2f mouse);
// Hover text for that part. False when there's nothing to say.
bool loadoutTooltip(const BallLoadout& L, int part, std::string& title, std::string& desc);

// A small hover box by the pointer: a title and a short wrapped description,
// kept on screen.
void drawTooltip(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f mouse, sf::Vector2f screen,
                 const std::string& title, const std::string& desc, sf::Color titleColor = theme::textHi);
void drawLoadoutPanel(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f c,
                      const BallLoadout& L, float alpha, float hover, int hoverSlot, bool dim);

}  // namespace sb
