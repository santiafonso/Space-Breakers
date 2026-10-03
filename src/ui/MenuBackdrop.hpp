#pragma once

#include <SFML/Graphics.hpp>

#include <vector>

#include "core/Math.hpp"

namespace sb {

// A calm layer of translucent balls drifting and bouncing off the screen edges,
// drawn behind the main menu so the start screen isn't dead-still. Kept dim and
// slow on purpose - the real ball is still the focus.
class MenuBackdrop {
public:
    void init(sf::Vector2f size);
    void update(float dt);
    void draw(sf::RenderWindow& window, float alpha = 1.f) const;

private:
    struct Orb {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float radius = 0.f;
        sf::Color color;
    };

    sf::Vector2f size_{1280.f, 800.f};
    std::vector<Orb> orbs_;
    Rng rng_;
};

// A few planets on slow tilted orbits around `centre`, each with its faint
// orbit, some with a ring or a moon - the far background of the skill web and
// the run map. `t` = seconds (drives the orbits), `scale` sizes the system.
void drawOrbitingPlanets(sf::RenderWindow& window, sf::Vector2f centre, float t, float scale, float alpha);

}  // namespace sb
