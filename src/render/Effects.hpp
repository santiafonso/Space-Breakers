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
    void addBurst(sf::Vector2f pos, float radius, sf::Color color);   // reaction / explosion (arena units)
    void addPop(sf::Vector2f pos, float radius, sf::Color color);     // something dies: a quick bright flash
    void edgeHit(sf::Vector2f normal);
    void addLabel(const std::string& text, sf::Vector2f pos, sf::Color color,
                  unsigned size, float life);
    void flash(sf::Color color, float strength);
    // A gold coin that pops out at `pos` (UI units) and flies to `target`.
    // Bigger coins for a bigger combo.
    void addCoin(sf::Vector2f pos, sf::Vector2f target, float radius);
    int takeArrivedCoins();   // coins that reached their target since the last call
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
        bool burst = false;
        bool pop = false;
    };
    struct Label {
        sf::Text text;
        float age = 0.f;
        float life = 1.f;
        sf::Vector2f vel;
    };

    struct Coin {
        sf::Vector2f pos;
        sf::Vector2f vel;
        sf::Vector2f target;
        float age = 0.f;
        float radius = 4.f;
    };

    std::vector<Coin> coins_;
    int arrived_ = 0;
    const sf::Font* font_ = nullptr;
    sf::Vector2f size_;
    std::vector<Ring> rings_;
    std::vector<Label> labels_;
    float edge_[4] = {0.f, 0.f, 0.f, 0.f};  // left, right, top, bottom
    float flash_ = 0.f;
    sf::Color flashColor_ = theme::accent;
};

}  // namespace sb
