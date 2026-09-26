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

// A straight bar from a to b.
void line(sf::RenderTarget& t, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color color);

// Four L-shaped corner marks just inside `r`, each arm `arm` long. `out` pushes
// them outward (used to make them snap in on reveal / breathe on hover).
void brackets(sf::RenderTarget& t, sf::FloatRect r, float arm, float thickness, sf::Color color,
              float out = 0.f);

// The house panel: dark glass lit faintly from the top, a hairline edge, a
// 1px highlight along the inside of the top edge and corner brackets in
// `edge`. `lit` (0..1) brightens fill and edge (hover / selection).
void panel(sf::RenderTarget& t, sf::FloatRect r, sf::Color edge, float alpha, float lit = 0.f);

// Darkens the screen toward its corners (radial), drawn over the backdrop.
void vignette(sf::RenderTarget& t, sf::Vector2f size, sf::Color edge, float strength);

// Radar backdrop: concentric rings every `gap` out to `maxR` plus `spokes`
// radial lines, with small ticks on the rings. Low alpha on purpose.
void radar(sf::RenderTarget& t, sf::Vector2f c, float maxR, float gap, int spokes, sf::Color color,
           float alpha);

}  // namespace sb::draw
