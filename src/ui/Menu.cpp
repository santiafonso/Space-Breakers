#include "ui/Menu.hpp"

#include <cmath>

#include "core/Theme.hpp"
#include "ui/Widgets.hpp"

namespace sb {

void Menu::init(const sf::Font& font, unsigned fontSize, float rowGap) {
    font_ = &font;
    fontSize_ = fontSize;
    rowGap_ = rowGap;
}

void Menu::setItems(std::vector<Item> items) {
    items_ = std::move(items);
    hover_.assign(items_.size(), 0.f);
}

void Menu::layout(sf::Vector2f firstRowCenter) { first_ = firstRowCenter; }

void Menu::update(float dt, sf::Vector2f mouse) {
    hovered_ = -1;
    const float k = 1.f - std::exp(-16.f * dt);
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const sf::Vector2f c = first_ + sf::Vector2f(0.f, rowGap_ * static_cast<float>(i));
        const bool over = items_[i].enabled &&
                          std::fabs(mouse.x - c.x) < 240.f &&
                          std::fabs(mouse.y - c.y) < rowGap_ * 0.45f;
        if (over) hovered_ = static_cast<int>(i);
        hover_[i] = lerpf(hover_[i], over ? 1.f : 0.f, k);
    }
}

int Menu::clickIndex(sf::Vector2f mouse) const {
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const sf::Vector2f c = first_ + sf::Vector2f(0.f, rowGap_ * static_cast<float>(i));
        if (items_[i].enabled && std::fabs(mouse.x - c.x) < 240.f &&
            std::fabs(mouse.y - c.y) < rowGap_ * 0.45f)
            return static_cast<int>(i);
    }
    return -1;
}

void Menu::draw(sf::RenderWindow& window) {
    if (!font_) return;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const float h = hover_[i];
        const sf::Vector2f c = first_ + sf::Vector2f(0.f, rowGap_ * static_cast<float>(i));
        sf::Color color = items_[i].enabled
                              ? lerpColor(theme::textLo, theme::textHi, 0.55f + 0.45f * h)
                              : theme::textDim;
        if (items_[i].enabled) color = lerpColor(color, theme::accent, h * 0.8f);

        sf::Text t = makeText(*font_, items_[i].label, fontSize_, color);
        centerOrigin(t);
        t.setPosition(std::round(c.x + h * 6.f), std::round(c.y));
        window.draw(t);

        if (h > 0.02f) {
            const sf::FloatRect b = t.getGlobalBounds();
            sf::RectangleShape bar({3.f, static_cast<float>(fontSize_) * 0.9f});
            bar.setOrigin(0.f, bar.getSize().y / 2.f);
            bar.setPosition(b.left - 16.f, c.y);
            bar.setFillColor(withAlpha(theme::accent, h));
            window.draw(bar);
        }
    }
}

}  // namespace sb
