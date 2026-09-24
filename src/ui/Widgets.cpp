#include "ui/Widgets.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

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

// Greedy word-wrap: break `str` into lines no wider than `maxW` at `size`.
std::vector<std::string> wrapText(const sf::Font& font, const std::string& str, unsigned size,
                                  float maxW) {
    std::vector<std::string> lines;
    std::string line;
    std::size_t i = 0;
    while (i < str.size()) {
        std::size_t sp = str.find(' ', i);
        const std::string word = str.substr(i, sp == std::string::npos ? std::string::npos : sp - i);
        const std::string trial = line.empty() ? word : line + " " + word;
        if (!line.empty() && makeText(font, trial, size, theme::textLo).getLocalBounds().width > maxW) {
            lines.push_back(line);
            line = word;
        } else {
            line = trial;
        }
        if (sp == std::string::npos) break;
        i = sp + 1;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

// ---- ball loadout panels (Equip picker, Tab overlay) -----------------------


sf::Vector2f panelCenter(sf::Vector2f size, int i, int n, float cy) {
    const float total = static_cast<float>(n) * kPanelW + static_cast<float>(n - 1) * kPanelGap;
    const float x0 = size.x * 0.5f - total * 0.5f + kPanelW * 0.5f;
    return {x0 + static_cast<float>(i) * (kPanelW + kPanelGap), cy};
}

sf::FloatRect slotRect(sf::Vector2f c, int slot) {
    const float wd = kPanelW - 24.f;
    const float cy = c.y - kPanelH * 0.5f + 100.f + static_cast<float>(slot) * kSlotStep;
    return {c.x - wd * 0.5f, cy - kSlotH * 0.5f, wd, kSlotH};
}

// "DMG 2  SPD 1" - a ball's stacked modifiers, compact.
std::string modifierLine(const BallLoadout& L) {
    static const char* kShort[kModifierCount] = {"DMG", "SIZE", "SPD", "TOP", "KNOCK", "FLING"};
    std::string out;
    for (int i = 0; i < kModifierCount; ++i) {
        if (L.mods[i] <= 0) continue;
        if (!out.empty()) out += "  ";
        out += std::string(kShort[i]) + " " + std::to_string(L.mods[i]);
    }
    return out;
}

sf::Color catColor(UpgradeCat c) {
    switch (c) {
        case UpgradeCat::NewBall:  return theme::core;
        case UpgradeCat::Role:     return theme::puOverdrive;
        case UpgradeCat::Element:  return theme::elemFire;
        case UpgradeCat::Item:     return theme::accent;
        case UpgradeCat::Modifier: return theme::ballMid;
        case UpgradeCat::Relic:    return theme::puGolden;
    }
    return theme::accent;
}

// One ball of the loadout: its look (element colour + role mark, same as in the
// arena), role / element name, its item slots and its stacked modifiers.
void drawLoadoutPanel(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f c,
                      const BallLoadout& L, float alpha, float hover, int hoverSlot, bool dim) {
    const float a = alpha * (dim ? 0.35f : 1.f);
    sf::RectangleShape box({kPanelW, kPanelH});
    box.setOrigin(kPanelW * 0.5f, kPanelH * 0.5f);
    box.setPosition(c);
    box.setFillColor(withAlpha(theme::accent, (0.06f + 0.14f * hover) * a));
    box.setOutlineThickness(1.5f);
    box.setOutlineColor(withAlpha(theme::accent, (0.30f + 0.5f * hover) * a));
    w.draw(box);

    const Element el = L.element();
    const sf::Color ec = el == Element::Plain ? theme::textLo : elementColor(el);
    const float r = L.role == BallRole::Guardian ? 17.f : 13.f;
    const sf::Vector2f bp{c.x, c.y - kPanelH * 0.5f + 30.f};
    sf::CircleShape ball(r, 32);
    ball.setOrigin(r, r);
    ball.setPosition(bp);
    ball.setFillColor(withAlpha(ec, a));
    ball.setOutlineThickness(L.role == BallRole::Guardian ? 3.5f : 1.5f);
    ball.setOutlineColor(withAlpha(sf::Color::White, (L.role == BallRole::Guardian ? 0.55f : 0.2f) * a));
    w.draw(ball);
    if (L.role == BallRole::Support) {
        sf::CircleShape ring(r * 0.5f, 24);
        ring.setOrigin(r * 0.5f, r * 0.5f);
        ring.setPosition(bp);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(withAlpha(sf::Color::White, 0.7f * a));
        w.draw(ring);
    } else if (L.role == BallRole::Striker) {
        sf::CircleShape dot(r * 0.28f, 16);
        dot.setOrigin(r * 0.28f, r * 0.28f);
        dot.setPosition(bp);
        dot.setFillColor(withAlpha(sf::Color::White, 0.8f * a));
        w.draw(dot);
    }

    std::string name = roleName(L.role);
    if (el != Element::Plain) name = std::string(elementName(el)) + " " + name;
    drawCentered(w, font, name, theme::fsBody, {c.x, c.y - kPanelH * 0.5f + 64.f}, withAlpha(theme::textHi, a));

    for (int i = 0; i < kBallSlots; ++i) {
        const sf::FloatRect sr = slotRect(c, i);
        const bool hot = hoverSlot == i;
        sf::RectangleShape sb({sr.width, sr.height});
        sb.setPosition(sr.left, sr.top);
        sb.setFillColor(withAlpha(theme::textLo, (hot ? 0.22f : 0.06f) * a));
        sb.setOutlineThickness(1.f);
        sb.setOutlineColor(withAlpha(theme::accent, (hot ? 0.8f : 0.2f) * a));
        w.draw(sb);
        std::string t = "empty slot";
        sf::Color tc = theme::textDim;
        if (L.gear[i] >= 0) {
            t = upgradeInfo(static_cast<UpgradeKind>(L.gear[i])).title;
            if (L.gearLvl[i] > 1) t += "  Lv" + std::to_string(L.gearLvl[i]);
            tc = theme::textLo;
        }
        drawCentered(w, font, t, theme::fsSmall, {c.x, sr.top + sr.height * 0.5f - 1.f}, withAlpha(tc, a));
    }
    const std::string ml = modifierLine(L);
    drawCentered(w, font, ml.empty() ? "no modifiers" : ml, theme::fsSmall,
                 {c.x, c.y + kPanelH * 0.5f - 18.f}, withAlpha(ml.empty() ? theme::textDim : theme::ballMid, a));
}

}  // namespace sb
