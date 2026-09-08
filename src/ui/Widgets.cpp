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

float introPop(float elapsed, float delay, float dur) {
    const float x = clampf((elapsed - delay) / (dur > 1e-3f ? dur : 1e-3f), 0.f, 1.f);
    const float c1 = 1.70158f;          // easeOutBack: settles with a small overshoot
    const float c3 = c1 + 1.f;
    const float u = x - 1.f;
    return 1.f + c3 * u * u * u + c1 * u * u;
}

void drawCenteredPop(sf::RenderWindow& window, const sf::Font& font, const std::string& str,
                     unsigned size, sf::Vector2f pos, sf::Color color, float pop) {
    if (pop <= 0.001f) return;
    const float a = clampf(pop, 0.f, 1.f);
    sf::Text t = makeText(font, str, size, withAlpha(color, a));
    centerOrigin(t);
    const float sc = 0.70f + 0.30f * pop;               // springs a touch past 1
    t.setScale(sc, sc);
    t.setPosition(std::round(pos.x), std::round(pos.y + (1.f - a) * 10.f));
    window.draw(t);
}

void drawDim(sf::RenderWindow& window, sf::Vector2f size, float alpha) {
    sf::RectangleShape r(size);
    r.setFillColor(withAlpha(theme::panel, alpha));
    window.draw(r);
}

void drawStatsPanel(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f size,
                    const Stats& s, float intro) {
    drawCenteredPop(window, font, "Stats", theme::fsTitle, {size.x * 0.5f, size.y * 0.16f},
                    theme::textHi, introPop(intro, 0.f));

    const std::array<std::pair<std::string, std::string>, 9> rows = {{
        {"Best wave", std::to_string(s.bestWave)},
        {"Best score", std::to_string(s.bestScore)},
        {"Runs", std::to_string(s.runs)},
        {"Wins", std::to_string(s.wins)},
        {"Enemies defeated", std::to_string(s.enemiesKilled)},
        {"Cores earned", std::to_string(s.coresEarned)},
        {"Best damage streak", std::to_string(s.bestCombo) + " hits"},
        {"Top speed", std::to_string(static_cast<long>(s.maxSpeed)) + " px/s"},
        {"Time played", formatTime(s.timePlayed)},
    }};

    const float y0 = size.y * 0.30f;
    const float gap = 38.f;
    const float labelX = size.x * 0.5f - 200.f;
    const float valueX = size.x * 0.5f + 200.f;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const float pop = introPop(intro, 0.10f + 0.045f * static_cast<float>(i), 0.26f);
        if (pop <= 0.001f) continue;
        const float a = clampf(pop, 0.f, 1.f);
        const float y = y0 + gap * static_cast<float>(i) - 12.f + (1.f - a) * 8.f;

        sf::Text label = makeText(font, rows[i].first, 18, withAlpha(theme::textLo, a));
        label.setPosition(labelX, y);
        window.draw(label);

        sf::Text value = makeText(font, rows[i].second, 18, withAlpha(theme::textHi, a));
        const sf::FloatRect vb = value.getLocalBounds();
        value.setOrigin(vb.left + vb.width, vb.top);
        value.setPosition(valueX, y);
        window.draw(value);
    }
}

}  // namespace sb
