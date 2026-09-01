#pragma once

#include <SFML/Graphics.hpp>

namespace sb {

class App;

// One full-screen state (menu, play, shop, ...). The App keeps a stack of them:
//   - input goes only to the top screen;
//   - drawing walks the stack from the topmost `opaque()` screen upward, so an
//     overlay (pause, shop) can render on top of the still-visible game;
//   - the simulation ticks only while the top screen `simulates()`.
class Screen {
public:
    virtual ~Screen() = default;

    virtual void onEnter(App&) {}
    virtual void handleEvent(App&, const sf::Event&, sf::Vector2f mouse) { (void)mouse; }
    virtual void update(App&, float dt, sf::Vector2f mouse) { (void)dt; (void)mouse; }
    virtual void draw(App&, sf::RenderWindow&) {}

    virtual bool opaque() const { return true; }
    virtual bool simulates() const { return false; }
};

}  // namespace sb
