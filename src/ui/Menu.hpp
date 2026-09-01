#pragma once

#include <string>
#include <vector>

#include "core/Math.hpp"

namespace sb {

// A vertical list of selectable rows with a subtle hover animation.
class Menu {
public:
    struct Item {
        std::string label;
        bool enabled = true;
    };

    Menu() = default;
    void init(const sf::Font& font, unsigned fontSize, float rowGap);
    void setItems(std::vector<Item> items);
    void layout(sf::Vector2f firstRowCenter);
    void update(float dt, sf::Vector2f mouse);

    int hovered() const { return hovered_; }
    // Returns the index of the enabled row under `mouse`, or -1.
    int clickIndex(sf::Vector2f mouse) const;
    void draw(sf::RenderWindow& window);

private:
    const sf::Font* font_ = nullptr;
    unsigned fontSize_ = 28;
    float rowGap_ = 56.f;
    sf::Vector2f first_{0.f, 0.f};
    std::vector<Item> items_;
    std::vector<float> hover_;
    int hovered_ = -1;
};

}  // namespace sb
