#pragma once

#include <SFML/Graphics.hpp>

#include "progression/Pacts.hpp"
#include "ui/Screen.hpp"

namespace sb {

// The pact choice (Fase O): after the act-1 boss, or at the run start with the
// "Covenant" web node. Three (four with "Oath") big cards, each a rule for the
// rest of the run with its cost; or refuse them all for a little gold.
class PactScreen : public Screen {
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

// Archetype colour: the pact cards, chips and the web's Pacts branch share it.
sf::Color pactColor(PactArchetype a);

// A row of chips naming the run's pacts at `pos` (its left edge, or its centre
// when `centered`). Hovering a chip shows what the pact does when `tips`.
// Returns true if a chip is under the pointer.
bool drawPactStrip(App& app, sf::RenderWindow& w, sf::Vector2f pos, bool centered, sf::Vector2f mouse, bool tips);

// In a fight: the pact chips bottom-left (+ the Nova charge), UI view.
void drawPactHud(App& app, sf::RenderWindow& w, sf::Vector2f mouse, bool tips);

// In a fight, world view: "Hunters" tethers to each ball's prey and the
// "Living Core" overcharge ring.
void drawPactWorld(App& app, sf::RenderWindow& w);

}  // namespace sb
