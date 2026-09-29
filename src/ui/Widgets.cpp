#include "ui/Widgets.hpp"

#include "render/ClassRender.hpp"
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
    // Captions were set at 9-12px and read as specks once the canvas is
    // scaled up: lift the small ones two steps, never below 12.
    if (size < 13) size = std::max(12u, size + 2u);
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

sf::Vector2f keyCapSize(const sf::Font& font, const std::string& key) {
    const float lw = makeLabel(font, key, 12, theme::textLo).getLocalBounds().width;
    return {std::max(26.f, std::round(lw + 16.f)), 22.f};
}

void drawKeyCap(sf::RenderWindow& w, const sf::Font& font, sf::FloatRect cap, const std::string& key,
                const std::string& caption, bool hot, bool captionRight) {
    const sf::Color c = hot ? theme::textLo : theme::textDim;
    draw::box(w, cap, 0.f, withAlpha(c, 0.18f), withAlpha(c, hot ? 0.12f : 0.05f), withAlpha(c, 0.8f), 1.f);
    const float cy = cap.top + cap.height * 0.5f;
    drawLabel(w, font, key, 12, {cap.left + cap.width * 0.5f + 1.f, cy}, hot ? theme::textHi : theme::textLo);
    if (hot && !caption.empty())
        drawLabel(w, font, caption, 12, {captionRight ? cap.left + cap.width + 10.f : cap.left - 10.f, cy},
                  theme::textLo, captionRight ? -1 : 1);
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

float panelRowZoom(sf::Vector2f size, int n, float reserve, float maxZoom) {
    const float fn = static_cast<float>(std::max(1, n));
    const float need = fn * kPanelW + (fn - 1.f) * kPanelGap + reserve + 2.f * theme::margin + 40.f;
    return clampf(std::min(size.x / need, size.y * 0.66f / kPanelH), 0.75f, maxZoom);
}

sf::Vector2f panelCenter(sf::Vector2f size, int i, int n, float cy, float cx) {
    if (cx < 0.f) cx = size.x * 0.5f;
    const float total = static_cast<float>(n) * kPanelW + static_cast<float>(n - 1) * kPanelGap;
    const float x0 = cx - total * 0.5f + kPanelW * 0.5f;
    return {x0 + static_cast<float>(i) * (kPanelW + kPanelGap), cy};
}

int abilityBoxes(const BallLoadout& L) {
    int n = std::max(1, std::min(abilitySlotCount(L), kMaxAbilitySlots));
    for (int i = n; i < kMaxAbilitySlots; ++i)
        if (L.ability[i] >= 0) n = i + 1;   // a filled slot that closed still shows (asleep)
    return n;
}

sf::FloatRect slotRect(sf::Vector2f c, int slot, const BallLoadout& L) {
    const float wd = kPanelW - 24.f;
    const float top = c.y - kPanelH * 0.5f;
    if (isItemSlot(slot)) {
        const float cy = top + kPanelItemsTop + static_cast<float>(slot) * kSlotStep;
        return {c.x - wd * 0.5f, cy - kSlotH * 0.5f, wd, kSlotH};
    }
    const float hy = top + kPanelHeadY;
    if (slot == kSlotType) {   // a circle left of the ball
        const float r = 17.f;
        return {c.x - 58.f - r, hy - r, 2.f * r, 2.f * r};
    }
    if (isAbilitySlot(slot)) {   // diamonds right of the ball: 1 big, 2 or 3 smaller side by side
        const int n = abilityBoxes(L);
        const int i = slot - kSlotAbility;
        const float r = n == 1 ? 16.f : (n == 2 ? 12.5f : 10.f);
        const float step = n == 1 ? 0.f : (n == 2 ? 27.f : 21.f);
        const float x = c.x + 58.f + (static_cast<float>(i) - 0.5f * static_cast<float>(n - 1)) * step;
        return {x - r, hy - r, 2.f * r, 2.f * r};
    }
    return {};
}

// "DMG 2  SPD 1" - a ball's stacked modifiers, compact.
std::string modifierLine(const BallLoadout& L) {
    static const char* kShort[kModifierCount] = {"DMG", "SIZE", "SPD"};
    std::string out;
    for (int i = 0; i < kModifierCount; ++i) {
        if (L.mods[i] <= 0) continue;
        if (!out.empty()) out += "  ";
        out += std::string(kShort[i]) + " " + std::to_string(L.mods[i]);
    }
    return out;
}

sf::Color tagColor(ItemTag t) { return roleColor(tagRole(t)); }   // ItemTag::None -> Normal: grey

// Rarity is a brightness ramp, not a hue - dim grey up to white, and only a
// Legendary turns pale gold - so it can never be mistaken for a class colour.
sf::Color tierColor(Tier t) {
    switch (t) {
        case Tier::Common:    return {104, 108, 122};
        case Tier::Uncommon:  return {150, 156, 170};
        case Tier::Rare:      return {196, 202, 214};
        case Tier::Epic:      return {240, 242, 250};
        case Tier::Legendary: return {240, 208, 140};
    }
    return theme::textLo;
}

void drawTierFrame(sf::RenderWindow& w, sf::FloatRect r, Tier t, float hover, float alpha, float time,
                   float reveal) {
    // Rarity lives on the frame only - its edge, a thin top band and the
    // brackets - so the card's fill is left to its class (drawClassCardMark).
    const sf::Color col = tierColor(t);
    const int rank = static_cast<int>(t);
    const float snap = (1.f - clampf(reveal, 0.f, 1.f)) * 16.f;   // brackets fly in from outside
    if (rank >= static_cast<int>(Tier::Epic)) {   // a slow breathing second frame round the rare ones
        const float pulse = 0.5f + 0.5f * std::sin(time * (rank == 4 ? 2.6f : 2.0f));
        draw::brackets(w, r, 16.f, 2.f, withAlpha(col, (0.25f + 0.3f * pulse) * alpha), 5.f + 2.f * pulse + snap);
    }
    const float lit = 0.05f * hover;
    draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::glassTop, theme::textHi, lit), 0.97f * alpha),
              withAlpha(theme::glassBottom, 0.97f * alpha),
              withAlpha(lerpColor(theme::arenaEdge, col, 0.45f + 0.1f * static_cast<float>(rank)),
                        (0.55f + 0.4f * hover) * alpha), 1.f);
    // the top edge is the rarity's place: a band that thickens with the tier
    const float band = 2.f + static_cast<float>(rank);
    draw::box(w, {r.left, r.top, r.width, band}, 0.f, withAlpha(col, 0.9f * alpha), withAlpha(col, 0.7f * alpha));
    draw::brackets(w, r, theme::bracket + 3.f, 2.f, withAlpha(col, (0.45f + 0.3f * hover) * alpha), snap);
}

void drawKindMark(sf::RenderTarget& w, UpgradeCat cat, sf::Vector2f pos, float size, sf::Color color) {
    switch (cat) {
        case UpgradeCat::Item:
            draw::polygon(w, pos, size * 1.2f, 4, kPi / 4.f, color, color);   // a square
            break;
        case UpgradeCat::Ability:  draw::polygon(w, pos, size * 1.25f, 4, 0.f, color, color); break;
        case UpgradeCat::Element:  draw::polygon(w, pos, size * 1.1f, 6, kPi / 6.f, color, color); break;
        case UpgradeCat::Relic:    draw::disc(w, pos, size, color, color, {1.f, 1.f}, 20); break;
        case UpgradeCat::Modifier: draw::polygon(w, pos + sf::Vector2f{0.f, size * 0.15f}, size * 1.25f, 3, -kPi / 2.f, color, color); break;
        case UpgradeCat::NewBall:  draw::ring(w, pos, size, 2.f, color); break;
    }
}

void drawPickCard(sf::RenderWindow& w, const sf::Font& font, sf::FloatRect r, UpgradeKind k,
                  const std::vector<BallLoadout>& balls, const PickCardStyle& st) {
    const float a = st.alpha;
    if (a <= 0.001f) return;
    const Tier tier = upgradeTier(k);
    const UpgradeCat cat = upgradeCat(k);
    const ItemTag tag = itemTag(k);
    const UpgradeInfo info = upgradeInfo(k);
    drawTierFrame(w, r, tier, st.hover, a, st.time, st.reveal);
    if (st.classMark) drawClassCardMark(w, font, r, k, balls, a);

    const float pad = 16.f, top = r.top + 21.f;
    // No class: a faint wash of the kind's colour instead, so a shelf of
    // relics, abilities and modifiers doesn't read as one grey row.
    if (tag == ItemTag::None) {
        const sf::Color kc = catColor(cat);
        draw::box(w, {r.left + 1.f, r.top + 3.f, r.width - 2.f, r.height * 0.5f}, 0.f, withAlpha(kc, 0.07f * a),
                  withAlpha(kc, 0.f));
    }
    // What it is.
    // What it is: its kind's mark (and name, unless a shelf header says it).
    const std::string what = upgradeCatName(cat);
    const sf::Color kindCol = withAlpha(lerpColor(theme::textLo, catColor(cat), 0.6f), a);
    float whatX = r.left + pad;
    if (st.showWhat) {
        drawKindMark(w, cat, {whatX + 4.f, top}, 4.5f, kindCol);
        drawLabel(w, font, what, 12, {whatX + 14.f, top}, kindCol, -1);
    }
    // How rare: pips, filled up to the tier, and the tier's name when it fits.
    const sf::Color tc = tierColor(tier);
    const int rank = static_cast<int>(tier);
    constexpr float kPip = 10.f;
    float x = r.left + r.width - pad - 4.f;
    for (int p = 4; p >= 0; --p) {
        const bool on = p <= rank;
        const sf::Color pc = on ? tc : theme::arenaEdge;
        draw::polygon(w, {x, top}, on ? 4.5f : 3.5f, 4, 0.f, withAlpha(pc, a), withAlpha(pc, a));
        x -= kPip;
    }
    if (!st.showWhat) {
        drawLabel(w, font, tierName(tier), 12, {r.left + pad, top}, withAlpha(tc, a), -1);
    } else {
        const float whatW = makeLabel(font, what, 12, theme::textLo).getLocalBounds().width + 14.f;
        const float wordW = makeLabel(font, tierName(tier), 12, tc).getLocalBounds().width;
        if (whatW + wordW + 5.f * kPip + 2.f * pad + 12.f < r.width)
            drawLabel(w, font, tierName(tier), 12, {x - 2.f, top}, withAlpha(tc, a), 1);
    }

    // The name.
    const int es = elementItemSlot(k);
    const unsigned ts = r.width > 230.f ? theme::fsItem : theme::fsHeading;
    sf::Color titleCol = theme::textHi;
    if (es >= 0) titleCol = elementColor(static_cast<Element>(es + 1));
    if (cat == UpgradeCat::Ability) titleCol = theme::ability;
    sf::Text title = makeText(font, info.title, ts, withAlpha(titleCol, a));
    centerOrigin(title);
    const float sc = clampf(st.pop, 0.f, 1.1f);
    title.setScale(sc, sc);
    title.setPosition(std::round(r.left + r.width * 0.5f), std::round(r.top + 54.f));
    // The kind's shape round the name, as on the ball's panel: an ability sits
    // in cyan brackets, an element in a pill of its colour.
    if (cat == UpgradeCat::Ability || cat == UpgradeCat::Element) {
        const sf::FloatRect tb = title.getGlobalBounds();
        const sf::FloatRect box{tb.left - 14.f, tb.top - 8.f, tb.width + 28.f, tb.height + 16.f};
        if (cat == UpgradeCat::Ability) {
            draw::brackets(w, box, 8.f, 2.f, withAlpha(theme::ability, 0.85f * a));
        } else {
            const sf::Color pc = elementColor(static_cast<Element>(es + 1));
            draw::box(w, box, box.height * 0.5f, withAlpha(pc, 0.16f * a), withAlpha(pc, 0.06f * a),
                      withAlpha(pc, 0.8f * a), 1.5f);
        }
    }
    w.draw(title);

    // Which class: a solid badge.
    float y = r.top + 86.f;
    const float cx = r.left + r.width * 0.5f;
    if (tag != ItemTag::None) {
        const sf::Color cc = tagColor(tag);
        const std::string name = itemTagName(tag);
        const float bw = makeLabel(font, name, 12, cc).getLocalBounds().width + 22.f;
        draw::box(w, {cx - bw * 0.5f, y - 11.f, bw, 22.f}, 0.f, withAlpha(cc, 0.9f * a), withAlpha(lerpColor(cc, theme::bg, 0.15f), 0.9f * a));
        drawLabel(w, font, name, 12, {cx + 1.f, y}, withAlpha(theme::bgDeep, a));
        y += 26.f;
    } else if (cat == UpgradeCat::Ability || cat == UpgradeCat::Element) {
        y += 4.f;   // (its shape round the name says what it is)
    }
    if (!st.note.empty()) {
        drawLabel(w, font, st.note, 12, {cx, y}, withAlpha(theme::accent, a));
        y += 22.f;
    }

    // What it does.
    const float lineH = 18.f;
    y += 4.f;
    const float bottom = r.top + r.height - st.bottomReserve - 6.f;
    for (const std::string& l : wrapText(font, info.desc, theme::fsSmall, r.width - 28.f)) {
        if (y > bottom) break;
        sf::Text t = makeText(font, l, theme::fsSmall, withAlpha(theme::textLo, a));
        centerOrigin(t);
        t.setPosition(std::round(cx), std::round(y));
        w.draw(t);
        y += lineH;
    }
}

void drawClassCardMark(sf::RenderWindow& w, const sf::Font& font, sf::FloatRect r, UpgradeKind k,
                       const std::vector<BallLoadout>& balls, float alpha) {
    const ItemTag t = itemTag(k);
    if (t == ItemTag::None || alpha <= 0.01f) return;
    const sf::Color col = tagColor(t);
    // the left edge is the class's place: a thick spine and a wash of its colour
    draw::box(w, {r.left + 1.f, r.top + 6.f, r.width - 2.f, r.height - 7.f}, 0.f, withAlpha(col, 0.16f * alpha),
              withAlpha(col, 0.03f * alpha));
    draw::box(w, {r.left, r.top + 6.f, 7.f, r.height - 6.f}, 0.f, withAlpha(col, 0.95f * alpha),
              withAlpha(col, 0.7f * alpha));

    // Would it make (2nd item of the tag) or ascend (4th) a ball? A copy it
    // already has only levels up, so that doesn't count.
    int best = 0;   // 1 = makes the class, 2 = ascends
    for (const BallLoadout& L : balls) {
        if (!upgradeFitsBall(k, L) || upgradeLevelsUp(k, L)) continue;
        bool free = false;
        for (int i = 0; i < kBallSlots; ++i) free = free || L.gear[i] < 0;
        if (!free) continue;
        const int n = L.tagCount(t);
        if (n == 3) best = 2;
        else if (n == 1) best = std::max(best, 1);
    }
    if (best == 0) return;
    const BallRole role = tagRole(t);
    const std::string text = best == 2 ? std::string("ascends: ") + ascendedName(role)
                                       : std::string("makes a ") + roleName(role);
    const sf::Text probe = makeLabel(font, text, 10, col);
    const float cw = probe.getLocalBounds().width + 22.f;
    const sf::FloatRect chip{r.left + r.width * 0.5f - cw * 0.5f, r.top + r.height - 6.f, cw, 19.f};
    draw::box(w, chip, theme::corner, withAlpha(lerpColor(theme::bgDeep, col, 0.25f), 0.97f * alpha),
              withAlpha(theme::bgDeep, 0.97f * alpha), withAlpha(col, 0.85f * alpha), 1.f);
    drawLabel(w, font, text, 10, {chip.left + chip.width * 0.5f, chip.top + chip.height * 0.5f},
              withAlpha(lerpColor(col, sf::Color::White, 0.15f), alpha));
}

sf::Color catColor(UpgradeCat c) {
    switch (c) {
        case UpgradeCat::NewBall:  return theme::core;
        case UpgradeCat::Element:  return theme::elemFire;   // muted: an element is the lesser pick
        case UpgradeCat::Ability:  return theme::ability;
        case UpgradeCat::Item:     return theme::accent;
        case UpgradeCat::Modifier: return theme::ballMid;
        case UpgradeCat::Relic:    return theme::puGolden;
    }
    return theme::accent;
}

// One ball of the loadout: its look (class-coloured body, element rim, class
// marks - same as in the arena), its classes, 4 item slots, its type slot, its ability slot(s)
// and its stacked modifiers.
void drawLoadoutPanel(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f c,
                      const BallLoadout& L, float alpha, float hover, int hoverSlot, bool dim, int placing) {
    const float a = alpha * (dim ? 0.35f : 1.f);
    // Solid, so the arena never shows through; lit a touch from the top.
    const sf::FloatRect pr{c.x - kPanelW * 0.5f, c.y - kPanelH * 0.5f, kPanelW, kPanelH};
    draw::panel(w, pr, dim ? theme::textDim : theme::accent, alpha, hover);
    const float top = c.y - kPanelH * 0.5f;

    // The ball, as in the arena: its lead class's colour (grey without one),
    // the element as a muted rim, class marks, ascended halo, highlight.
    const Element el = L.element();
    const sf::Color ec = el == Element::Plain ? theme::textLo : elementColor(el);
    ItemTag roles[2];
    const int nRoles = L.roles(roles);
    const ItemTag asc = L.ascended();
    const float r = L.hasRole(ItemTag::Guardian) ? 16.f : 13.f;
    const sf::Vector2f bp{c.x, top + kPanelHeadY};
    BallLook look;
    look.roles = L.roleMask();
    look.ascended = roleBit(tagRole(asc));
    look.lead = nRoles > 0 ? tagRole(roles[0]) : BallRole::Normal;
    look.second = nRoles > 1 ? tagRole(roles[1]) : BallRole::Normal;
    look.element = el;
    const sf::Color bc = nRoles > 0 ? theme::hueSpeedColor(tagColor(roles[0]), 1.f, 1.f, roles[0] == asc)
                                    : theme::speedColor(1.f, 1.f);
    draw::glow(w, bp, r * (asc != ItemTag::None ? 2.f : 1.6f), bc, (asc != ItemTag::None ? 0.11f : 0.06f) * a);
    draw::disc(w, bp, r, withAlpha(lerpColor(bc, sf::Color::White, 0.2f), a),
               withAlpha(lerpColor(bc, theme::bg, 0.18f), a));
    drawBallIdentity(w, look, bp, r, 0.f, a);
    draw::disc(w, bp + sf::Vector2f{-0.34f, -0.38f} * r, r * 0.26f, withAlpha(sf::Color::White, 0.22f * a),
               withAlpha(sf::Color::White, 0.f), {1.f, 0.8f}, 16);

    // "Striker / Jester": each class in its own colour - an ascended one by
    // its ascended name, brighter.
    std::vector<std::pair<std::string, sf::Color>> parts;   // classes only: the element has its own pill row
    for (int i = 0; i < nRoles; ++i) {
        if (i > 0) parts.push_back({" / ", theme::textDim});
        const bool up = roles[i] == asc;
        parts.push_back({up ? ascendedName(tagRole(roles[i])) : roleName(tagRole(roles[i])),
                         up ? lerpColor(tagColor(roles[i]), sf::Color::White, 0.35f) : tagColor(roles[i])});
    }
    if (nRoles == 0) parts.push_back({"Normal", theme::textLo});
    auto advance = [&](const std::string& str, unsigned size) {   // pen advance, trailing spaces included
        const sf::Text t = makeText(font, str, size, theme::textHi);
        return t.findCharacterPos(str.size()).x - t.findCharacterPos(0).x;
    };
    auto widthAt = [&](unsigned size) {
        float wd = 0.f;
        for (const auto& pt : parts) wd += advance(pt.first, size);
        return wd;
    };
    unsigned fs = theme::fsBody;
    if (widthAt(fs) > kPanelW - 14.f) fs = theme::fsSmall;
    float x = c.x - widthAt(fs) * 0.5f;
    for (const auto& [str, col] : parts) {
        sf::Text t = makeText(font, str, fs, withAlpha(col, a));
        const sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left, b.top + b.height * 0.5f);
        t.setPosition(std::round(x), std::round(top + 84.f));
        w.draw(t);
        x += advance(str, fs);
    }

    // One slot, its kind told by shape, colour and place, never by words:
    //  item    = a square chip tinted in its class colour with a solid class
    //            spine on the left and its rarity as pips on the right (below);
    //  type    = a hexagon of the element's colour (header, left of the ball);
    //  ability = a cyan diamond (header, right of the ball).
    // Empty slots keep the shape, faint. While a pick is being placed, the
    // slots that take it are lit.
    auto slotBox = [&](int s, const std::string& emptyLabel, bool asleep) {
        const sf::FloatRect sr = slotRect(c, s, L);
        const bool hot = hoverSlot == s;
        const float sa = a * (asleep ? 0.45f : 1.f);
        const bool takes = placing >= 0 && !dim && slotAccepts(static_cast<UpgradeKind>(placing), s, L);
        const int kind = L.kindAt(s);
        const float cy = sr.top + sr.height * 0.5f;
        const float lit = hot ? 1.f : (takes ? 0.6f : 0.f);
        const unsigned size = sr.width > 90.f ? theme::fsSmall : 11u;

        if (s == kSlotType) {   // the type: a hexagon of the element's colour, its name under it
            const sf::Vector2f pc{sr.left + sr.width * 0.5f, cy};
            const float r = sr.width * 0.5f;
            constexpr float kHexRot = kPi / 6.f;   // flat top
            if (takes) draw::polygonOutline(w, pc, r + 5.f, 6, kHexRot, 1.5f, withAlpha(theme::accent, 0.7f * sa));
            if (kind < 0) {
                draw::polygonOutline(w, pc, r, 6, kHexRot, 1.5f, withAlpha(theme::textDim, (0.45f + 0.4f * lit) * sa));
                return;
            }
            const sf::Color ecol = elementColor(L.element());
            draw::polygon(w, pc, r, 6, kHexRot, withAlpha(lerpColor(ecol, theme::bgDeep, 0.35f), 0.95f * sa),
                          withAlpha(lerpColor(ecol, theme::bgDeep, 0.6f), 0.95f * sa));
            draw::polygonOutline(w, pc, r, 6, kHexRot, 2.f,
                                 withAlpha(lerpColor(ecol, sf::Color::White, 0.2f), (0.9f + 0.1f * lit) * sa));
            if (L.levelAt(s) > 1)
                drawCentered(w, font, std::to_string(L.levelAt(s)), 12, {pc.x, pc.y - 1.f}, withAlpha(theme::textHi, sa));
            sf::Text nm = makeText(font, elementName(L.element()), 12, withAlpha(ecol, sa));
            centerOrigin(nm);
            nm.setPosition(std::round(pc.x), std::round(pc.y + r + 12.f));
            w.draw(nm);
            Element els[kMaxElements];   // an Alchemist's other elements: small hexagons beside it
            const int ne = L.elements(els);
            for (int i = 1; i < ne; ++i) {
                const sf::Color c2 = elementColor(els[i]);
                const sf::Vector2f q{pc.x - r - 4.f - 11.f * static_cast<float>(i), pc.y - r * 0.55f + 13.f * static_cast<float>(i - 1)};
                draw::polygon(w, q, 6.f, 6, kHexRot, withAlpha(lerpColor(c2, theme::bgDeep, 0.3f), 0.95f * sa),
                              withAlpha(lerpColor(c2, theme::bgDeep, 0.55f), 0.95f * sa));
                draw::polygonOutline(w, q, 6.f, 6, kHexRot, 1.2f, withAlpha(c2, 0.95f * sa));
            }
            return;
        }
        if (isAbilitySlot(s)) {   // an ability: a cyan diamond; its name under it when there's one
            const sf::Vector2f pc{sr.left + sr.width * 0.5f, cy};
            const float r = sr.width * 0.5f;
            if (takes) draw::polygonOutline(w, pc, r * 1.15f + 5.f, 4, 0.f, 1.5f, withAlpha(theme::accent, 0.7f * sa));
            if (kind < 0) {
                draw::polygonOutline(w, pc, r * 1.15f, 4, 0.f, 1.5f,
                                     withAlpha(theme::ability, (0.3f + 0.4f * lit) * sa));
                return;
            }
            draw::polygon(w, pc, r * 1.15f, 4, 0.f, withAlpha(lerpColor(theme::ability, theme::bgDeep, 0.4f), 0.95f * sa),
                          withAlpha(lerpColor(theme::ability, theme::bgDeep, 0.65f), 0.95f * sa));
            draw::polygonOutline(w, pc, r * 1.15f, 4, 0.f, 2.f,
                                 withAlpha(lerpColor(theme::ability, sf::Color::White, 0.25f), (0.9f + 0.1f * lit) * sa));
            if (L.levelAt(s) > 1)
                drawCentered(w, font, std::to_string(L.levelAt(s)), r > 12.f ? 12u : 11u, {pc.x, pc.y - 1.f},
                             withAlpha(theme::textHi, sa));
            if (abilityBoxes(L) == 1) {
                sf::Text nm = makeText(font, upgradeInfo(static_cast<UpgradeKind>(kind)).title, 12,
                                       withAlpha(theme::ability, sa));
                if (nm.getLocalBounds().width < 70.f) {
                    centerOrigin(nm);
                    nm.setPosition(std::round(pc.x), std::round(pc.y + r * 1.15f + 11.f));
                    w.draw(nm);
                }
            }
            return;
        }
        // an item slot
        if (kind < 0) {
            draw::box(w, sr, 0.f, withAlpha(theme::bgDeep, 0.6f * sa), withAlpha(theme::bgDeep, 0.5f * sa),
                      withAlpha(theme::accent, (0.12f + 0.5f * lit) * sa), 1.f);
            drawLabel(w, font, emptyLabel, 9, {sr.left + sr.width * 0.5f, cy}, withAlpha(theme::textDim, sa));
            return;
        }
        const auto k = static_cast<UpgradeKind>(kind);
        const sf::Color cc = tagColor(itemTag(k));
        draw::box(w, sr, 0.f, withAlpha(lerpColor(cc, theme::bgDeep, 0.62f - 0.1f * lit), 0.95f * sa),
                  withAlpha(lerpColor(cc, theme::bgDeep, 0.78f), 0.95f * sa), withAlpha(cc, (0.35f + 0.4f * lit) * sa), 1.f);
        draw::box(w, {sr.left, sr.top, 5.f, sr.height}, 0.f, withAlpha(cc, sa), withAlpha(cc, sa));   // the class spine
        // rarity: pips at the right end, the brightness ramp
        const int rank = static_cast<int>(upgradeTier(k));
        const sf::Color tcol = tierColor(upgradeTier(k));
        float px = sr.left + sr.width - 7.f;
        for (int p = 0; p <= rank; ++p) {
            draw::polygon(w, {px, cy}, 2.6f, 4, 0.f, withAlpha(tcol, sa), withAlpha(tcol, sa));
            px -= 6.f;
        }
        std::string t = upgradeInfo(k).title;
        if (L.levelAt(s) > 1) t += (sr.width > 90.f ? "  Lv" : " ") + std::to_string(L.levelAt(s));
        const float textL = sr.left + 5.f, textR = px + 2.f;   // between the spine and the pips
        drawCentered(w, font, t, size, {(textL + textR) * 0.5f, cy - 1.f}, withAlpha(theme::textHi, sa));
    };
    for (int i = 0; i < kBallSlots; ++i) slotBox(i, "empty", false);
    slotBox(kSlotType, "type", false);
    const int open = abilitySlotCount(L);
    for (int i = 0; i < abilityBoxes(L); ++i) slotBox(kSlotAbility + i, "ability", i >= open);

    const std::string ml = modifierLine(L);
    draw::line(w, {c.x - kPanelW * 0.5f + 14.f, c.y + kPanelH * 0.5f - 34.f},
               {c.x + kPanelW * 0.5f - 14.f, c.y + kPanelH * 0.5f - 34.f}, 1.f, withAlpha(theme::arenaEdge, 0.8f * a));
    drawLabel(w, font, ml.empty() ? "no modifiers" : ml, 11, {c.x, c.y + kPanelH * 0.5f - 18.f},
              withAlpha(ml.empty() ? theme::textDim : theme::ballMid, a));
}

int panelPartAt(sf::Vector2f c, sf::Vector2f mouse, const BallLoadout& L) {
    if (std::fabs(mouse.x - c.x) > kPanelW * 0.5f || std::fabs(mouse.y - c.y) > kPanelH * 0.5f) return -1;
    for (int s = 0; s < kSlotAbility + abilityBoxes(L); ++s)
        if (slotRect(c, s, L).contains(mouse)) return s;
    const float top = c.y - kPanelH * 0.5f;
    if (mouse.y < top + 98.f) return kPanelPartBall;
    if (mouse.y > c.y + kPanelH * 0.5f - 32.f) return kPanelPartMods;
    return -1;
}

bool loadoutTooltip(const BallLoadout& L, int part, std::string& title, std::string& desc) {
    if (part >= 0 && part < kLoadoutSlots) {
        const int kind = L.kindAt(part);
        if (kind < 0) {
            if (isItemSlot(part)) {
                title = "Empty item slot";
                desc = "items go here (4 per ball); 2 items of one tag give the ball that class, 4 its ascended form";
            } else if (part == kSlotType) {
                title = "Type slot";
                desc = "the ball's element goes here - one per ball (an Alchemist holds 2, 3 ascended); a new one swaps it. It doesn't count toward a class.";
            } else if (part - kSlotAbility < abilitySlotCount(L)) {
                title = "Ability slot";
                desc = "an ability goes here: it fires by itself every few seconds. It doesn't count toward a class.";
            } else {
                title = "Closed ability slot";
                desc = "the ball can't use this ability slot right now: a Mage opens a 2nd one (2 Mage items), an Ancient Mage a 3rd (4)";
            }
            return true;
        }
        const auto k = static_cast<UpgradeKind>(kind);
        const UpgradeInfo info = upgradeInfo(k);
        title = info.title;
        if (part == kSlotType) {   // an Alchemist's other elements
            Element els[kMaxElements];
            const int ne = L.elements(els);
            for (int i = 1; i < ne; ++i) title += std::string(" + ") + elementName(els[i]);
        }
        title += "  Lv " + std::to_string(L.levelAt(part)) + "/" + std::to_string(maxLevelOf(k));
        desc = info.desc;
        if (isAbilitySlot(part) && part - kSlotAbility >= abilitySlotCount(L))
            desc = "(asleep: this slot closed when the ball lost its Mage items - it wakes up when they're back)  " + desc;
        if (L.levelAt(part) < maxLevelOf(k) && *upgradeLevelDesc(k))
            desc += std::string(".  Next level: ") + upgradeLevelDesc(k);
        desc += std::string("  [") + tierName(upgradeTier(k));
        if (itemTag(k) != ItemTag::None) desc += std::string(", ") + itemTagName(itemTag(k));
        else if (upgradeCat(k) != UpgradeCat::Item) desc += std::string(", ") + upgradeCatName(upgradeCat(k));
        desc += "]";
        return true;
    }
    if (part == kPanelPartBall) {
        ItemTag roles[2];
        const int n = L.roles(roles);
        const ItemTag asc = L.ascended();
        title.clear();
        desc.clear();
        for (int i = 0; i < n; ++i) {
            const BallRole r = tagRole(roles[i]);
            if (i > 0) { title += " / "; desc += ".  "; }
            title += roles[i] == asc ? ascendedName(r) : roleName(r);
            desc += std::string(roleName(r)) + ": " + roleDesc(r);
            if (roles[i] == asc) desc += std::string(".  Ascended: ") + ascendedDesc(r);
        }
        if (n == 0) {
            title = roleName(BallRole::Normal);
            desc = roleDesc(BallRole::Normal);
        }
        // Tag count: which classes it is building toward.
        std::string counts;
        for (int i = 0; i < kClassCount; ++i) {
            const int c = L.tagCount(classTag(i));
            if (c <= 0) continue;
            if (!counts.empty()) counts += ", ";
            counts += std::to_string(c) + " " + itemTagName(classTag(i));
        }
        desc += ".  Classes come from item tags: 2 of a tag = that class (a ball can have two), 4 = ascended.";
        if (!counts.empty()) desc += " Now: " + counts + ".";
        const Element e = L.element();
        if (e != Element::Plain) title = std::string(elementName(e)) + "  " + title;
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
