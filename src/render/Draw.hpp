#pragma once

#include <SFML/Graphics.hpp>

// A small shape kit shared by the world renderer and the UI: soft glows,
// radial-gradient discs, regular polygons and rounded (optionally gradient)
// boxes. Everything takes a RenderTarget so it works on windows and textures.
namespace sb::draw {

// A soft radial glow: `color` at the centre fading to nothing at `radius`.
// Additive, so overlapping glows brighten instead of muddying.
void glow(sf::RenderTarget& t, sf::Vector2f pos, float radius, sf::Color color, float alpha);

// A filled disc shaded from `inner` at the centre to `outer` at the rim,
// optionally squashed along an axis (scale.x / scale.y) around `pos`.
void disc(sf::RenderTarget& t, sf::Vector2f pos, float radius, sf::Color inner, sf::Color outer,
          sf::Vector2f scale = {1.f, 1.f}, int points = 40);

// Same, for a regular polygon with `sides` corners rotated by `rot` radians.
void polygon(sf::RenderTarget& t, sf::Vector2f pos, float radius, int sides, float rot,
             sf::Color inner, sf::Color outer);
// Just the outline of that polygon.
void polygonOutline(sf::RenderTarget& t, sf::Vector2f pos, float radius, int sides, float rot,
                    float thickness, sf::Color color);

// A ring (circle outline) of `thickness`, optionally only an arc from `a0` to
// `a1` radians (clockwise from +x).
void ring(sf::RenderTarget& t, sf::Vector2f pos, float radius, float thickness, sf::Color color,
          float a0 = 0.f, float a1 = 6.2831853f, int segments = 48);

// A rounded box with a vertical gradient fill (top -> bottom) and an outline.
void box(sf::RenderTarget& t, sf::FloatRect r, float corner, sf::Color top, sf::Color bottom,
         sf::Color outline = sf::Color::Transparent, float thickness = 0.f);

}  // namespace sb::draw
