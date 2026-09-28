#pragma once

#include <SFML/Graphics.hpp>

#include "progression/Pacts.hpp"
#include "ui/Screen.hpp"

namespace sb {

// The Altar (2026-09-28): three pacts, each a gift with a price. Take one or
// walk away with nothing.
class PactScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    sf::FloatRect cardRect(App& app, int i) const;
    sf::FloatRect leaveRect(App& app) const;
    int cardAt(App& app, sf::Vector2f mouse) const;
    float hover_[4] = {};
    float leaveHover_ = 0.f;
    sf::Vector2f mouse_;
    float clock_ = 0.f;
};

}  // namespace sb
