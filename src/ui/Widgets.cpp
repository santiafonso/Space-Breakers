#include "ui/Widgets.hpp"

#include <array>
#include <cmath>
#include <cstdio>

namespace sb {

namespace {

std::string formatTime(double seconds) {
    const long total = static_cast<long>(seconds);
    const long h = total / 3600;
    const long m = (total % 3600) / 60;
    const long s = total % 60;
    char buf[32];
    if (h > 0) std::snprintf(buf, sizeof(buf), "%ldh %02ldm", h, m);
    else std::snprintf(buf, sizeof(buf), "%ldm %02lds", m, s);
    return buf;
}

}  // namespace

sf::Text makeText(const sf::Font& font, const std::string& str, unsigned size, sf::Color color) {
    sf::Text t(str, font, size);
    t.setFillColor(color);
    return t;
}

void centerOrigin(sf::Text& t) {
    const sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
}

void drawCentered(sf::RenderWindow& window, const sf::Font& font, const std::string& str,
                  unsigned size, sf::Vector2f pos, sf::Color color) {
    sf::Text t = makeText(font, str, size, color);
    centerOrigin(t);
    t.setPosition(std::round(pos.x), std::round(pos.y));
    window.draw(t);
}

void drawDim(sf::RenderWindow& window, sf::Vector2f size, float alpha) {
    sf::RectangleShape r(size);
    r.setFillColor(withAlpha(theme::panel, alpha));
    window.draw(r);
}

void drawStatsPanel(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f size,
                    const Stats& s) {
    drawCentered(window, font, "Stats", theme::fsTitle, {size.x * 0.5f, size.y * 0.16f},
                 theme::textHi);

    const std::array<std::pair<std::string, std::string>, 7> rows = {{
        {"Best wave", std::to_string(s.bestWave)},
        {"Runs", std::to_string(s.runs)},
        {"Enemies defeated", std::to_string(s.enemiesKilled)},
        {"Lifetime scrap", std::to_string(s.lifetimeScrap)},
        {"Best damage streak", std::to_string(s.bestCombo) + " hits"},
        {"Top speed", std::to_string(static_cast<long>(s.maxSpeed)) + " px/s"},
        {"Time played", formatTime(s.timePlayed)},
    }};

    const float y0 = size.y * 0.30f;
    const float gap = 42.f;
    const float labelX = size.x * 0.5f - 200.f;
    const float valueX = size.x * 0.5f + 200.f;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const float y = y0 + gap * static_cast<float>(i);
        sf::Text label = makeText(font, rows[i].first, 18, theme::textLo);
        label.setPosition(labelX, y - 12.f);
        window.draw(label);

        sf::Text value = makeText(font, rows[i].second, 18, theme::textHi);
        const sf::FloatRect vb = value.getLocalBounds();
        value.setOrigin(vb.left + vb.width, vb.top);
        value.setPosition(valueX, y - 12.f);
        window.draw(value);
    }
}

}  // namespace sb
