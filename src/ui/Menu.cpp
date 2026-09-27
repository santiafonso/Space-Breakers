#include "ui/Menu.hpp"

#include <cmath>

#include "core/Theme.hpp"
#include "render/Draw.hpp"
#include "ui/UiSound.hpp"
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
    uisound::hover(this, hovered_);
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

void Menu::draw(sf::RenderWindow& window, float intro) {
    if (!font_) return;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const float pop = introPop(intro, 0.05f + 0.055f * static_cast<float>(i), 0.26f);
        if (pop <= 0.001f) continue;
        const float ia = clampf(pop, 0.f, 1.f);

        const float h = hover_[i];
        const sf::Vector2f c = first_ + sf::Vector2f(0.f, rowGap_ * static_cast<float>(i));
        sf::Color color = items_[i].enabled
                              ? lerpColor(theme::textLo, theme::textHi, 0.55f + 0.45f * h)
                              : theme::textDim;
        if (items_[i].enabled) color = lerpColor(color, theme::accent, h * 0.8f);

        sf::Text t = makeText(*font_, items_[i].label, fontSize_, withAlpha(color, ia));
        centerOrigin(t);
        const float sc = 0.82f + 0.18f * pop;
        t.setScale(sc, sc);
        t.setPosition(std::round(c.x), std::round(c.y + (1.f - ia) * 9.f));
        window.draw(t);

        if (h > 0.02f) {   // corner brackets close in on the row under the pointer
            const sf::FloatRect b = t.getGlobalBounds();
            const float px = 18.f + 12.f * (1.f - h), py = 8.f + 5.f * (1.f - h);
            draw::box(window, {b.left - px, b.top - py, b.width + 2.f * px, b.height + 2.f * py}, 0.f,
                      withAlpha(theme::accent, 0.07f * h * ia), withAlpha(theme::accent, 0.f));
            draw::brackets(window, {b.left - px, b.top - py, b.width + 2.f * px, b.height + 2.f * py}, 8.f, 2.f,
                           withAlpha(theme::accent, h * ia));
        }
    }
}

}  // namespace sb
