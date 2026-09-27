#pragma once

#include <SFML/Graphics.hpp>

#include "ui/Screen.hpp"

namespace sb {

// Sound settings (main menu "Sound", pause "Sound"): master / music / effects
// volume, mute, and for each category of sound a volume and a synthesised
// style (Soft / Bright / Retro / Off). Picking a style or letting go of a
// slider previews the sound. Saved on the way out.
class SoundScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    // Hit ids: 0..2 the global sliders, kMute, 100 + c a category slider,
    // 200 + c * 4 + s a style chip, kBack.
    static constexpr int kMute = 10;
    static constexpr int kBack = 20;
    int itemAt(App& app, sf::Vector2f mouse) const;
    void setSlider(App& app, int id, float x);
    void close(App& app);

    int hover_ = -1;
    int drag_ = -1;     // slider being dragged
    sf::Vector2f mouse_;
};

}  // namespace sb
