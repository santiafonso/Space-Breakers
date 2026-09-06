#pragma once

#include <string>
#include <vector>

#include "core/Math.hpp"
#include "core/Theme.hpp"

namespace sb {

// The visual feedback layer: a single expanding ring per bounce, brief glow on
// the arena edge that was hit, a soft full-screen tint on power-up pickup, and
// the occasional floating label. Deliberately sparse - no particle sprays.
class Effects {
public:
    void init(const sf::Font& font, sf::Vector2f size);

    void addRing(sf::Vector2f pos, float speed, sf::Color color);
    void edgeHit(sf::Vector2f normal);
    void addLabel(const std::string& text, sf::Vector2f pos, sf::Color color,
                  unsigned size, float life);
    void flash(sf::Color color, float strength);
    void clear();

    void update(float dt);
    void drawBorder(sf::RenderWindow& window) const;  // behind the balls
    void drawRings(sf::RenderWindow& window) const;    // above the balls
    void drawOverlay(sf::RenderWindow& window) const;  // labels + tint, above HUD

private:
    struct Ring {
        sf::Vector2f pos;
        float age = 0.f;
        float life = 0.4f;
        float r0 = 0.f;
        float r1 = 0.f;
        sf::Color color;
    };
    struct Label {
        sf::Text text;
        float age = 0.f;
        float life = 1.f;
        sf::Vector2f vel;
    };

    const sf::Font* font_ = nullptr;
    sf::Vector2f size_;
    std::vector<Ring> rings_;
    std::vector<Label> labels_;
    float edge_[4] = {0.f, 0.f, 0.f, 0.f};  // left, right, top, bottom
    float flash_ = 0.f;
    sf::Color flashColor_ = theme::accent;
};

}  // namespace sb
