#pragma once

#include <SFML/Graphics.hpp>

#include "ui/Screen.hpp"

namespace sb {

// Options (main menu, pause, or O / the corner button anywhere), on two tabs.
// General: the sound (master / music / effects, each a volume and an on-off
// switch), the display (fullscreen) and the cards (simple / detailed
// descriptions). Sound mix: for each category of sound a volume and a
// synthesised style (Soft / Bright / Retro / Off). Picking a style or letting
// go of a slider previews the sound. Saved on the way out. (2026-10-19)
class SoundScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    // Hit ids: 0..2 the global sliders, kMute + 0..2 their on-off switches
    // (all sound / music / effects), 100 + c a category slider,
    // 200 + c * 4 + s a style chip, and the buttons below.
    static constexpr int kMute = 10;
    static constexpr int kBack = 20;
    static constexpr int kFull = 21;      // the fullscreen switch
    static constexpr int kSimple = 22;    // descriptions: simple...
    static constexpr int kDetailed = 23;  // ...or detailed
    static constexpr int kTab = 30;       // kTab + 0 General, kTab + 1 Sound mix
    int itemAt(App& app, sf::Vector2f mouse) const;
    void setSlider(App& app, int id, float x);
    void close(App& app);

    bool advanced_ = false;   // the Sound mix tab (the per-category table); off, General
    int hover_ = -1;
    int drag_ = -1;     // slider being dragged
    sf::Vector2f mouse_;
};

}  // namespace sb
