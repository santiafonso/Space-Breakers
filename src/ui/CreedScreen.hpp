#pragma once

#include <SFML/Graphics.hpp>

#include "progression/Creeds.hpp"
#include "ui/Screen.hpp"

namespace sb {

// The creed choice (Fase O): after the act-1 boss, or at the run start with the
// "Covenant" web node. Three (four with "Oath") big cards, each a rule for the
// rest of the run with its cost; or refuse them all for a little gold.
class CreedScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    sf::FloatRect cardRect(App& app, int i) const;
    sf::FloatRect refuseRect(App& app) const;
    int cardAt(App& app, sf::Vector2f mouse) const;
    float hover_[4] = {};
    float refuseHover_ = 0.f;
    sf::Vector2f mouse_;
    float clock_ = 0.f;
};

// Archetype colour: the creed cards, chips and the web's Creeds branch share it.
sf::Color creedColor(CreedArchetype a);

// A row of chips naming the run's creeds at `pos` (its left edge, or its centre
// when `centered`). Hovering a chip shows what the creed does when `tips`.
// Returns true if a chip is under the pointer.
bool drawCreedStrip(App& app, sf::RenderWindow& w, sf::Vector2f pos, bool centered, sf::Vector2f mouse, bool tips);

// In a fight: the creed chips bottom-left (+ the Nova charge), UI view.
void drawCreedHud(App& app, sf::RenderWindow& w, sf::Vector2f mouse, bool tips);

// In a fight, world view: "Hunters" tethers to each ball's prey and the
// "Living Core" overcharge ring.
void drawCreedWorld(App& app, sf::RenderWindow& w);

}  // namespace sb
