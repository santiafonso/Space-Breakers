#include "ui/Widgets.hpp"

#include "render/Draw.hpp"

#include <algorithm>
#include <array>
#include <cctype>
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

namespace {
const sf::Font* gTitleFont = nullptr;
}

void setTitleFont(const sf::Font* font) { gTitleFont = font; }

sf::Text makeText(const sf::Font& font, const std::string& str, unsigned size, sf::Color color) {
    sf::Text t(str, (gTitleFont && size >= theme::fsHeading) ? *gTitleFont : font, size);
    t.setFillColor(color);
    return t;
}

sf::Text makeLabel(const sf::Font& font, const std::string& str, unsigned size, sf::Color color) {
    std::string up = str;
    for (char& ch : up) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    sf::Text t(up, font, size);   // always the UI face: labels stay light, never Black
    t.setLetterSpacing(theme::tracking);
    t.setFillColor(color);
    return t;
}

void drawLabel(sf::RenderTarget& target, const sf::Font& font, const std::string& str, unsigned size,
               sf::Vector2f pos, sf::Color color, int align) {
    sf::Text t = makeLabel(font, str, size, color);
    const sf::FloatRect b = t.getLocalBounds();
    const float ox = align == 0 ? b.left + b.width * 0.5f : (align > 0 ? b.left + b.width : b.left);
    t.setOrigin(ox, b.top + b.height * 0.5f);
    t.setPosition(std::round(pos.x), std::round(pos.y));
    target.draw(t);
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

        // caption on the left, value on the right, a hairline between rows
        drawLabel(window, font, rows[i].first, 12, {labelX, y + 11.f}, withAlpha(theme::textLo, a), -1);
        draw::line(window, {labelX, y + gap * 0.5f + 12.f}, {valueX, y + gap * 0.5f + 12.f}, 1.f,
                   withAlpha(theme::arenaEdge, 0.6f * a));

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

sf::Color tagColor(ItemTag t) {
    switch (t) {
        case ItemTag::Striker:  return theme::ballFast;
        case ItemTag::Guardian: return theme::core;
        case ItemTag::Support:  return theme::puSurge;
        case ItemTag::None:     return theme::textLo;
    }
    return theme::textLo;
}

sf::Color tierColor(Tier t) {
    switch (t) {
        case Tier::Common:    return theme::textLo;
        case Tier::Uncommon:  return theme::ballMid;
        case Tier::Rare:      return theme::accent;
        case Tier::Epic:      return theme::puSurge;
        case Tier::Legendary: return theme::puGolden;
    }
    return theme::textLo;
}

void drawTierFrame(sf::RenderWindow& w, sf::FloatRect r, Tier t, float hover, float alpha, float time,
                   float reveal) {
    const sf::Color col = tierColor(t);
    const int rank = static_cast<int>(t);
    const float snap = (1.f - clampf(reveal, 0.f, 1.f)) * 16.f;   // brackets fly in from outside
    if (rank >= static_cast<int>(Tier::Epic)) {   // a slow breathing second frame round the rare ones
        const float pulse = 0.5f + 0.5f * std::sin(time * (rank == 4 ? 3.2f : 2.4f));
        draw::brackets(w, r, 16.f, 2.f, withAlpha(col, (0.35f + 0.4f * pulse) * alpha), 5.f + 3.f * pulse + snap);
    }
    // Glass lit from the top by the tier colour; brighter under the pointer.
    const float lit = 0.10f + 0.035f * static_cast<float>(rank) + 0.12f * hover;
    draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::glassTop, col, lit), 0.97f * alpha),
              withAlpha(lerpColor(theme::glassBottom, col, lit * 0.25f), 0.97f * alpha),
              withAlpha(col, (0.30f + 0.4f * hover) * alpha), 1.f);
    draw::box(w, {r.left, r.top, r.width, 3.f}, 0.f, withAlpha(col, 0.85f * alpha), withAlpha(col, 0.85f * alpha));
    draw::brackets(w, r, theme::bracket + 3.f, 2.f, withAlpha(col, (0.7f + 0.3f * hover) * alpha), snap);
}

sf::Color catColor(UpgradeCat c) {
    switch (c) {
        case UpgradeCat::NewBall:  return theme::core;
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
    // Solid, so the arena never shows through; lit a touch from the top.
    const sf::FloatRect pr{c.x - kPanelW * 0.5f, c.y - kPanelH * 0.5f, kPanelW, kPanelH};
    draw::panel(w, pr, dim ? theme::textDim : theme::accent, alpha, hover);

    const Element el = L.element();
    const sf::Color ec = el == Element::Plain ? theme::textLo : elementColor(el);
    const float r = L.role() == BallRole::Guardian ? 17.f : 13.f;
    const sf::Vector2f bp{c.x, c.y - kPanelH * 0.5f + 30.f};
    // The same look as in the arena: glow, shaded body, role mark, highlight.
    const bool guardian = L.role() == BallRole::Guardian;
    draw::glow(w, bp, r * 1.6f, ec, 0.06f * a);
    draw::disc(w, bp, r, withAlpha(lerpColor(ec, sf::Color::White, 0.2f), a),
               withAlpha(lerpColor(ec, theme::bg, 0.2f), a));
    draw::ring(w, bp, r, guardian ? 3.5f : 1.5f, withAlpha(sf::Color::White, (guardian ? 0.55f : 0.22f) * a));
    if (L.role() == BallRole::Support)
        draw::ring(w, bp, r * 0.45f, 2.f, withAlpha(sf::Color::White, 0.7f * a));
    else if (L.role() == BallRole::Striker)
        draw::disc(w, bp, r * 0.26f, withAlpha(sf::Color::White, 0.85f * a), withAlpha(sf::Color::White, 0.6f * a));
    if (L.mastery()) draw::ring(w, bp, r + 5.f, 1.5f, withAlpha(lerpColor(ec, sf::Color::White, 0.5f), 0.7f * a));
    draw::disc(w, bp + sf::Vector2f{-0.34f, -0.38f} * r, r * 0.26f, withAlpha(sf::Color::White, 0.22f * a),
               withAlpha(sf::Color::White, 0.f), {1.f, 0.8f}, 16);

    std::string name = roleName(L.role());
    if (L.mastery()) name += "+";   // mastery: 4 items of its tag
    if (el != Element::Plain) name = std::string(elementName(el)) + " " + name;
    drawCentered(w, font, name, theme::fsBody, {c.x, c.y - kPanelH * 0.5f + 64.f}, withAlpha(theme::textHi, a));

    for (int i = 0; i < kBallSlots; ++i) {
        const sf::FloatRect sr = slotRect(c, i);
        const bool hot = hoverSlot == i;
        draw::box(w, sr, theme::corner, withAlpha(theme::bgDeep, (hot ? 0.5f : 0.7f) * a),
                  withAlpha(theme::bgDeep, (hot ? 0.3f : 0.55f) * a),
                  withAlpha(theme::accent, (hot ? 0.8f : 0.14f) * a), 1.f);
        if (hot) draw::box(w, sr, 0.f, withAlpha(theme::accent, 0.14f * a), withAlpha(theme::accent, 0.04f * a));
        std::string t = "empty";
        sf::Color tc = theme::textDim;
        if (L.gear[i] >= 0) {
            const auto k = static_cast<UpgradeKind>(L.gear[i]);
            t = upgradeInfo(k).title;
            if (L.gearLvl[i] > 1) t += "  Lv" + std::to_string(L.gearLvl[i]);
            tc = tagColor(itemTag(k));   // the tag shows which role it pushes toward
            // a tier tick on the left edge of a filled slot
            draw::box(w, {sr.left, sr.top, 3.f, sr.height}, 0.f, withAlpha(tierColor(upgradeTier(k)), a),
                      withAlpha(tierColor(upgradeTier(k)), a));
            drawCentered(w, font, t, theme::fsSmall, {c.x, sr.top + sr.height * 0.5f - 1.f}, withAlpha(tc, a));
        } else {
            drawLabel(w, font, t, 10, {c.x, sr.top + sr.height * 0.5f}, withAlpha(tc, a));
        }
    }
    const std::string ml = modifierLine(L);
    draw::line(w, {c.x - kPanelW * 0.5f + 14.f, c.y + kPanelH * 0.5f - 34.f},
               {c.x + kPanelW * 0.5f - 14.f, c.y + kPanelH * 0.5f - 34.f}, 1.f, withAlpha(theme::arenaEdge, 0.8f * a));
    drawLabel(w, font, ml.empty() ? "no modifiers" : ml, 11, {c.x, c.y + kPanelH * 0.5f - 18.f},
              withAlpha(ml.empty() ? theme::textDim : theme::ballMid, a));
}

int panelPartAt(sf::Vector2f c, sf::Vector2f mouse) {
    if (std::fabs(mouse.x - c.x) > kPanelW * 0.5f || std::fabs(mouse.y - c.y) > kPanelH * 0.5f) return -1;
    for (int i = 0; i < kBallSlots; ++i)
        if (slotRect(c, i).contains(mouse)) return i;
    const float top = c.y - kPanelH * 0.5f;
    if (mouse.y < top + 76.f) return kPanelPartBall;
    if (mouse.y > c.y + kPanelH * 0.5f - 32.f) return kPanelPartMods;
    return -1;
}

bool loadoutTooltip(const BallLoadout& L, int part, std::string& title, std::string& desc) {
    if (part >= 0 && part < kBallSlots) {
        if (L.gear[part] < 0) {
            title = "Empty slot";
            desc = "items and elements go here (4 per ball)";
            return true;
        }
        const auto k = static_cast<UpgradeKind>(L.gear[part]);
        const UpgradeInfo info = upgradeInfo(k);
        title = info.title;
        if (L.gearLvl[part] > 1) title += "  (level " + std::to_string(L.gearLvl[part]) + ")";
        desc = info.desc;
        desc += std::string("  [") + tierName(upgradeTier(k));
        if (itemTag(k) != ItemTag::None) desc += std::string(", ") + itemTagName(itemTag(k));
        desc += "]";
        return true;
    }
    if (part == kPanelPartBall) {
        title = roleName(L.role());
        if (L.mastery()) title += " (mastery)";
        desc = roleDesc(L.role());
        desc += std::string(".  Roles come from item tags: 2 of a tag = that role, 4 = mastery. Now: ") +
                std::to_string(L.tagCount(ItemTag::Striker)) + " Striker, " +
                std::to_string(L.tagCount(ItemTag::Guardian)) + " Guardian, " +
                std::to_string(L.tagCount(ItemTag::Support)) + " Support";
        const Element e = L.element();
        if (e != Element::Plain) {
            title = std::string(elementName(e)) + " " + title;
            desc += std::string(".  ") +
                    upgradeInfo(static_cast<UpgradeKind>(static_cast<int>(UpgradeKind::ElemFire) +
                                                         static_cast<int>(e) - 1)).desc;
        }
        return true;
    }
    if (part == kPanelPartMods) {
        title = "Modifiers";
        desc.clear();
        for (int i = 0; i < kModifierCount; ++i) {
            if (L.mods[i] <= 0) continue;
            if (!desc.empty()) desc += ",  ";
            desc += std::string(upgradeInfo(static_cast<UpgradeKind>(static_cast<int>(UpgradeKind::HeavyImpact) + i)).title) +
                    " x" + std::to_string(L.mods[i]);
        }
        if (desc.empty()) desc = "stat bumps that stack on this ball - none yet";
        return true;
    }
    return false;
}

void drawTooltip(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f mouse, sf::Vector2f screen,
                 const std::string& title, const std::string& desc, sf::Color titleColor) {
    const float wd = 270.f, pad = 10.f, lineH = 17.f;
    const std::vector<std::string> lines = wrapText(font, desc, theme::fsSmall, wd - 2.f * pad);
    const float ht = pad * 2.f + 20.f + lineH * static_cast<float>(lines.size());
    float x = mouse.x + 16.f, y = mouse.y + 18.f;
    if (x + wd > screen.x - 4.f) x = mouse.x - wd - 12.f;
    if (y + ht > screen.y - 4.f) y = mouse.y - ht - 10.f;
    x = std::max(4.f, x);
    y = std::max(4.f, y);

    draw::box(w, {x, y, wd, ht}, theme::corner, withAlpha(theme::glassTop, 0.96f), withAlpha(theme::glassBottom, 0.96f),
              withAlpha(titleColor, 0.3f), 1.f);
    draw::box(w, {x, y, 2.f, ht}, 0.f, titleColor, titleColor);   // a colour spine on the left edge
    draw::brackets(w, {x, y, wd, ht}, 7.f, 1.5f, withAlpha(titleColor, 0.7f));

    sf::Text t = makeText(font, title, theme::fsBody, titleColor);
    t.setPosition(std::round(x + pad), std::round(y + pad - 2.f));
    w.draw(t);
    float ly = y + pad + 22.f;
    for (const std::string& l : lines) {
        sf::Text d = makeText(font, l, theme::fsSmall, theme::textLo);
        d.setPosition(std::round(x + pad), std::round(ly));
        w.draw(d);
        ly += lineH;
    }
}

}  // namespace sb
