#pragma once

#include "core/Math.hpp"
#include "progression/GameData.hpp"
#include "progression/Upgrades.hpp"

namespace sb {

// Compact single-line-per-row upgrade list. `rowAt` gives the upgrade under the
// cursor (also used for 1-N keys), or -1.
class Shop {
public:
    void init(const sf::Font& font, sf::Vector2f size);
    void update(float dt, sf::Vector2f mouse);
    int rowAt(sf::Vector2f mouse) const;  // -1 if none
    void draw(sf::RenderWindow& window, const GameData& data) const;

private:
    const sf::Font* font_ = nullptr;
    sf::Vector2f size_;
    float rowY_[UpgradeCount] = {};
    float rowH_ = 44.f;
    float hover_[UpgradeCount] = {};
};

}  // namespace sb
