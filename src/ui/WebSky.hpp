#pragma once

#include <SFML/Graphics.hpp>

namespace sb {

// The skill web's sky grows with what you've unlocked (2026-10-06). A fresh
// save looks at black; every class you unlock adds its own faint motif to the
// background (with a flare the moment it arrives) and more stars; with more
// classes come a nebula, the orbiting planets and a milky way. All of it dim
// and slow - the web stays the focus.
//   unlock: MetaState::unlock; centre: the web's centre (with parallax);
//   t: seconds (drives everything); scale: the planets' size; alpha: fade-in.
//   pan: the web camera's pan - the galaxy (2x the screen) drifts with half of it.
void drawWebSky(sf::RenderWindow& w, sf::Vector2f size, sf::Vector2f centre, sf::Vector2f pan, const int* unlock, float t,
                float scale, float alpha);

// One class's motif alone (with a few stars) - the dev / snapshot preview.
void drawWebSkyClass(sf::RenderTarget& w, sf::Vector2f size, int cls, float t);

// The whole galaxy as it is with `classes` unlocked (the snapshot preview).
void drawWebSkyStage(sf::RenderTarget& w, sf::Vector2f size, int classes, float t);

// A run's sky for its act (1..5), over `size` (the UI canvas), in the act's tint.
void drawActSky(sf::RenderTarget& w, sf::Vector2f size, int act, sf::Color tint, float t, float a);

// How many classes the web has unlocked (the sky's level).
int webSkyClasses(const int* unlock);

}  // namespace sb
