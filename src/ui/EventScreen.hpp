#pragma once

#include <SFML/Graphics.hpp>

#include "ui/Screen.hpp"

namespace sb {

// A "?" stop's event (2026-10-01): a couple of deals for gold (rule cards, in
// the "?" colour). Take one or walk away. App::openEvent / takeEventDeal.
class EventScreen : public Screen {
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
