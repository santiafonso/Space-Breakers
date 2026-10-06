#pragma once

#include <SFML/Graphics.hpp>

namespace sb {

class App;

// One full-screen state (menu, play, shop, ...). The App keeps a stack of them:
//   - input goes only to the top screen;
//   - drawing walks the stack from the topmost `opaque()` screen upward, so an
//     overlay (pause, shop) can render on top of the still-visible game;
//   - the simulation ticks only while the top screen `simulates()` and isn't
//     `frozen()` (a fight screen holding its clock still, e.g. the TAB peek).
class Screen {
public:
    virtual ~Screen() = default;

    virtual void onEnter(App&) {}
    virtual void handleEvent(App&, const sf::Event&, sf::Vector2f mouse) { (void)mouse; }
    virtual void update(App&, float dt, sf::Vector2f mouse) { (void)dt; (void)mouse; }
    virtual void draw(App&, sf::RenderWindow&) {}

    virtual bool opaque() const { return true; }
    virtual bool simulates() const { return false; }
    virtual bool frozen() const { return false; }
    // The screen runs its own TAB peek (the fight, the map). On every other
    // screen of a live run the App opens the same peek over it.
    virtual bool ownsTab() const { return false; }

    // Seconds since this screen last became the active one. Screens read it to
    // stagger their contents popping in on entry; App resets and advances it.
    void beginIntro() { intro_ = 0.f; }
    void advanceIntro(float dt) { intro_ += dt; }
    float intro() const { return intro_; }

private:
    float intro_ = 0.f;
};

// The TAB loadout peek: the fight and the map run their own, the App one over
// every other screen of a live run. Hold TAB to look and
// let go to close; a quick tap leaves it open until TAB (or Esc) again.
// While it's open a slot can be dragged onto another ball (loadoutDragEvent);
// letting go of TAB mid-drag keeps it open until the drop.
struct TabPeek {
    bool open = false;
    bool down = false;     // TAB is physically held
    float held = 0.f;      // seconds since this press
    int dragBall = -1;     // the slot being dragged (ball, slot), -1 = none
    int dragSlot = -1;
    bool closeOnDrop = false;   // TAB was let go mid-drag: close once it lands
    bool handle(const sf::Event& e);   // true if it consumed the event
    void update(float dt) { if (down) held += dt; }
    void close() { open = down = closeOnDrop = false; held = 0.f; dragBall = dragSlot = -1; }
    bool latched() const { return open && !down && !closeOnDrop; }
    bool dragging() const { return open && dragBall >= 0; }
};

}  // namespace sb
