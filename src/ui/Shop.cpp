#include "ui/Shop.hpp"

#include <cmath>
#include <string>

#include "core/Theme.hpp"
#include "ui/Widgets.hpp"

namespace sb {

void Shop::init(const sf::Font& font, sf::Vector2f size) {
    font_ = &font;
    size_ = size;
    rowH_ = 44.f;
    const float y0 = size.y * 0.30f;
    for (int i = 0; i < UpgradeCount; ++i)
        rowY_[i] = y0 + rowH_ * static_cast<float>(i);
}

void Shop::update(float dt, sf::Vector2f mouse) {
    const int row = rowAt(mouse);
    const float k = 1.f - std::exp(-16.f * dt);
    for (int i = 0; i < UpgradeCount; ++i)
        hover_[i] = lerpf(hover_[i], row == i ? 1.f : 0.f, k);
}

int Shop::rowAt(sf::Vector2f mouse) const {
    if (std::fabs(mouse.x - size_.x * 0.5f) > 300.f) return -1;
    for (int i = 0; i < UpgradeCount; ++i)
        if (std::fabs(mouse.y - rowY_[i]) < rowH_ * 0.5f) return i;
    return -1;
}

void Shop::draw(sf::RenderWindow& window, const GameData& data) const {
    drawCentered(window, *font_, "Upgrades", theme::fsTitle, {size_.x * 0.5f, size_.y * 0.14f},
                 theme::textHi);
    drawCentered(window, *font_, std::to_string(data.points) + " points", theme::fsHeading,
                 {size_.x * 0.5f, size_.y * 0.21f}, theme::accent);

    const float nameX = size_.x * 0.5f - 230.f;
    const float levelX = size_.x * 0.5f + 70.f;
    const float costX = size_.x * 0.5f + 230.f;
    int hoveredRow = -1;

    for (int i = 0; i < UpgradeCount; ++i) {
        const UpgradeDef& def = upgradeDef(i);
        const int level = data.level[i];
        const bool maxed = upgradeMaxed(i, level);
        const std::uint32_t cost = upgradeCost(i, level);
        const bool afford = !maxed && data.points >= cost;
        const float h = hover_[i];
        const float y = rowY_[i];
        if (h > 0.5f) hoveredRow = i;

        if (h > 0.02f) {
            sf::RectangleShape hl({520.f, rowH_ - 6.f});
            hl.setOrigin(hl.getSize().x / 2.f, hl.getSize().y / 2.f);
            hl.setPosition(size_.x * 0.5f, y);
            hl.setFillColor(withAlpha(theme::accent, 0.07f * h));
            window.draw(hl);
        }

        const sf::Color nameCol =
            maxed ? theme::textLo : lerpColor(theme::textHi, theme::accent, h * 0.7f);
        sf::Text name = makeText(*font_, std::string(def.name), theme::fsItem, nameCol);
        const sf::FloatRect nb = name.getLocalBounds();
        name.setOrigin(nb.left, nb.top + nb.height / 2.f);
        name.setPosition(nameX, y);
        window.draw(name);

        drawCentered(window, *font_, maxed ? "MAX" : "Lv " + std::to_string(level), theme::fsBody,
                     {levelX, y}, theme::textLo);

        sf::Text costText = makeText(*font_, maxed ? "maxed" : std::to_string(cost), theme::fsItem,
                                     maxed ? theme::textDim : (afford ? theme::accent : theme::textDim));
        const sf::FloatRect cb = costText.getLocalBounds();
        costText.setOrigin(cb.left + cb.width, cb.top + cb.height / 2.f);
        costText.setPosition(costX, y);
        window.draw(costText);
    }

    // One shared description line for whichever row is hovered.
    const std::string desc = hoveredRow >= 0 ? std::string(upgradeDef(hoveredRow).effect) : "";
    drawCentered(window, *font_, desc, theme::fsSmall,
                 {size_.x * 0.5f, rowY_[UpgradeCount - 1] + rowH_ * 0.9f}, theme::textLo);

    drawCentered(window, *font_, "click or press 1-6 to buy    -    TAB to play", theme::fsBody,
                 {size_.x * 0.5f, size_.y * 0.9f}, theme::textLo);
}

}  // namespace sb
