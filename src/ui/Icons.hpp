#pragma once

#include "progression/UpgradeKind.hpp"

#include <SFML/Graphics.hpp>

// Item / relic / ability / element icons: the kind's mark as a frame (square
// item, diamond ability, hexagon element, disc relic, triangle modifier) in
// its class colour, and a glyph inside for what it does - so a slot or a
// card reads at a glance before its text does.
namespace sb {

enum class Glyph { Plus, Flame, Bubbles, Drop, Frost, Rock, Bolt, Speed, Blast, Twin, Shield, Burst, Meteor, Missile,
                   Size, Crit, Sling, Flask, Combo, Heal, Recharge, Bounce, Pierce, Skull, Shove, Eye, Beam, Vortex,
                   Magnet, Orbit, Coin, Rune, Bullets, Blade, Turret, Ghost, Dice, Card, Star, Clock, Claw, Planet };
inline constexpr int kGlyphCount = static_cast<int>(Glyph::Planet) + 1;

// What it does, as one of the glyphs (a table in ui/Icons.cpp).
Glyph glyphOf(UpgradeKind k);
// The icon's colour: the class's, else the element's, else the kind's.
sf::Color iconColor(UpgradeKind k);

// Just the glyph, centred on `c`, `r` from centre to edge.
void drawGlyph(sf::RenderTarget& t, Glyph g, sf::Vector2f c, float r, sf::Color col);
// The whole icon: frame + glyph. `r` is the frame's half-size.
void drawIcon(sf::RenderTarget& t, UpgradeKind k, sf::Vector2f c, float r, float alpha = 1.f);

}  // namespace sb
