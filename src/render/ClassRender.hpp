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

// Everything that says who a ball is, drawn over its body (whose colour is
// its lead class - see World::ballTint): the element as a thin muted rim just
// outside the edge, a dual role's second class as an accent arc along the
// lower inside of the rim, every class's mark, and the ascended form's halo
// (class-coloured, with four slow ticks) with its mark at full strength. A
// classless, element-less ball keeps a plain faint rim. Arena + TAB panels.
struct BallLook {
    RoleMask roles = 0;
    RoleMask ascended = 0;
    BallRole lead = BallRole::Normal;
    BallRole second = BallRole::Normal;
    Element element = Element::Plain;
    RoleMask legend = 0;   // a Legendary of each of these classes: its cosmetic (hat, visor, dagger...)
};
BallLook ballLook(const Ball& b);
void drawBallIdentity(sf::RenderTarget& t, const BallLook& look, sf::Vector2f pos, float r, float heading,
                      float alpha, bool held = false);

// The class-gain flare (Ball::classPulse, 1 -> 0): a ring in the lead class's
// colour that swells off the ball; the ascended one is bigger with a second
// ring behind it.
void drawClassPulse(sf::RenderTarget& t, BallRole lead, bool ascended, sf::Vector2f pos, float r, float k);
void drawElementPulse(sf::RenderTarget& t, Element el, sf::Vector2f pos, float r, float k);   // element-gain rings

// Per-class things on the field (bullets, turrets, summons...), drawn over
// the enemies and under the balls. World view.
void drawClassWorld(sf::RenderTarget& t, const World& world);

// A dragon in the Elder dragon's look, in one of its shapes (0 classic, 1 wyvern,
// 2 four wings, 3 serpent) - Tiamat's dragons, the web sky's.
void drawDragonForm(sf::RenderTarget& t, int form, sf::Vector2f pos, sf::Vector2f face, sf::Vector2f headDir, float S, float phase,
                    sf::Color c, float glowA, const sf::Vector2f* trail, int trailN, float k, int heads = 1,
                    const sf::Vector2f* headDirs = nullptr);   // (several heads: where each looks, else fanned out)

// Dev (SB_FXSHEET=<dir>): a contact sheet of every item / ability flourish,
// each at three moments, so they can be judged side by side. `page` 0..;
// returns false past the last page.
bool drawFxSheet(sf::RenderTarget& t, int page, const sf::Font& font);

}  // namespace sb
