#include "ui/Widgets.hpp"
#include "ui/Icons.hpp"
#include "ui/CreedScreen.hpp"

#include "render/ClassRender.hpp"
#include "render/Draw.hpp"
#include "core/ConfigClasses.hpp"

#include <algorithm>
#include <unordered_map>
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

// ---- rich text (2026-10-22): what each word in a description is about, by colour

namespace {

struct Keyword { const char* word; sf::Color color; };

// One colour per idea, the same on every card, tooltip and web node. Class names
// take their class colour (keywordColor); numbers are lit white.
const sf::Color kwCrit{240, 96, 104}, kwDamage{255, 156, 90}, kwSpeed{240, 206, 96}, kwSize{96, 156, 255},
    kwControl{186, 160, 255}, kwSummon{150, 226, 120};

const std::vector<Keyword>& keywords() {
    static const std::vector<Keyword> k = [] {
        std::vector<Keyword> v;
        auto add = [&](sf::Color c, std::initializer_list<const char*> ws) {
            for (const char* w : ws) v.push_back({w, c});
        };
        add(kwCrit, {"crit", "crits", "critical", "critically"});
        add(kwDamage, {"damage", "hit", "hits", "harder", "strike", "strikes", "stab", "stabs", "slam", "slams",
                       "burst", "bursts", "blast", "blasts", "explodes", "explosion", "pierce", "pierces", "piercing"});
        add(kwSpeed, {"speed", "fast", "faster", "fastest", "quick", "quicker", "cruise", "swift", "velocity"});
        add(kwSize, {"size", "big", "bigger", "biggest", "large", "larger", "grow", "grows", "growth", "wide", "wider",
                     "radius", "huge", "giant", "reach"});
        add(kwControl, {"slow", "slows", "slowed", "slower", "stun", "stuns", "stunned", "stagger", "staggers",
                        "staggered", "knock", "knocks", "knockback", "shove", "shoves", "pull", "pulls", "push",
                        "pushes", "mark", "marks", "marked"});
        add(theme::ability, {"ability", "abilities", "cast", "casts", "cooldown", "cooldowns", "recharge",
                             "recharges", "charge", "charges"});
        add(kwSummon, {"summon", "summons", "sprite", "spriteling", "spritelings", "turret", "turrets", "totem",
                       "totems", "golem", "golems", "wisp", "wisps", "dragon", "dragons", "minion", "minions"});
        add(theme::core, {"core", "heal", "heals", "healed", "repair", "repairs", "health", "hp", "shield",
                          "shields", "regen"});
        add(theme::puGolden, {"gold", "coin", "coins", "cores", "price", "prices", "sell"});
        add(theme::puOverdrive, {"combo"});
        add(lerpColor(theme::elemFire, sf::Color::White, 0.2f), {"fire", "burn", "burns", "burning", "flame", "flames", "lava"});
        add(theme::venom, {"poison", "poisoned", "poisons", "venom", "toxic"});
        add(lerpColor(theme::elemWater, sf::Color::White, 0.2f), {"water", "wave", "waves", "wet", "rain"});
        add(lerpColor(theme::elemIce, sf::Color::White, 0.2f), {"ice", "frost", "freeze", "freezes", "frozen", "chill", "chilled"});
        add(lerpColor(theme::elemStone, sf::Color::White, 0.2f), {"stone", "boulder", "boulders", "quake", "earth", "mud"});
        add(lerpColor(theme::elemElectric, sf::Color::White, 0.25f), {"electric", "zap", "zaps", "lightning", "shock", "chain", "chains"});
        add(theme::plagueEdge, {"plague", "infect", "infects", "infected"});
        return v;
    }();
    return k;
}

// "Split shot" / "Hand-off" / "777" -> "split shot" / "hand off" / "777": lower
// case, words split on anything that isn't a letter or a digit.
std::string normName(const std::string& s) {
    std::string out;
    bool gap = false;
    for (char ch : s) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalnum(c)) {
            if (gap && !out.empty()) out += ' ';
            out += static_cast<char>(std::tolower(c));
            gap = false;
        } else {
            gap = true;
        }
    }
    return out;
}

// Every named thing a description may point at, in its own colour: items (their
// class), elements, abilities, relics, modifiers, ascended forms, creeds, pacts.
const std::unordered_map<std::string, sf::Color>& namedThings() {
    static const std::unordered_map<std::string, sf::Color> m = [] {
        std::unordered_map<std::string, sf::Color> v;
        for (int i = 0; i < kUpgradeKindCount; ++i) {
            const auto k = static_cast<UpgradeKind>(i);
            const std::string n = normName(upgradeInfo(k).title);
            if (!n.empty() && upgradeCat(k) != UpgradeCat::NewBall) v.emplace(n, nameColor(k));
        }
        for (int i = 1; i < kBallRoleCount; ++i)
            if (const char* a = ascendedName(static_cast<BallRole>(i)); a && *a)
                v.emplace(normName(a), tagColor(static_cast<ItemTag>(i)));
        for (int i = 0; i < kCreedCount; ++i) {
            const CreedDef& d = creedDef(static_cast<CreedId>(i));
            v.emplace(normName(d.name), lerpColor(creedColor(d.archetype), theme::textHi, 0.2f));
        }
        for (int i = 0; i < kPactCount; ++i)
            v.emplace(normName(pactDef(static_cast<PactId>(i)).name), lerpColor(theme::pact, theme::textHi, 0.25f));
        return v;
    }();
    return m;
}

bool isWordChar(const std::string& s, std::size_t i) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (std::isalnum(c) || c == '%' || c == '\'') return true;
    if (c == '+' || c == '-') return i + 1 < s.size() && std::isdigit(static_cast<unsigned char>(s[i + 1]));   // "+25%", "-10%"
    if (c == '.') return i > 0 && i + 1 < s.size() && std::isdigit(static_cast<unsigned char>(s[i - 1])) &&
                         std::isdigit(static_cast<unsigned char>(s[i + 1]));                                    // "1.5"
    return false;
}

}  // namespace

sf::Color nameColor(UpgradeKind k) {
    switch (upgradeCat(k)) {
        case UpgradeCat::Relic:    return theme::puGolden;
        case UpgradeCat::Modifier: return lerpColor(theme::ballMid, theme::textHi, 0.15f);
        case UpgradeCat::Item:     return lerpColor(iconColor(k), theme::textHi, 0.2f);
        default:                   return iconColor(k);
    }
}

bool keywordColor(const std::string& word, sf::Color& out) {
    std::string w;
    bool digit = false;
    for (char ch : word) {
        digit = digit || std::isdigit(static_cast<unsigned char>(ch));
        w += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (digit) { out = theme::textHi; return true; }   // "+25%", "x2", "3 s"
    if (w.size() > 2 && w.compare(w.size() - 2, 2, "'s") == 0) w.resize(w.size() - 2);   // "Striker's"
    for (const Keyword& k : keywords())
        if (w == k.word) { out = k.color; return true; }
    for (int i = 1; i < kBallRoleCount; ++i) {   // a class by name: its colour
        std::string name = roleName(static_cast<BallRole>(i));
        for (char& ch : name) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (w == name || w == name + "s") { out = tagColor(static_cast<ItemTag>(i)); return true; }
    }
    return false;
}

void drawRichText(sf::RenderTarget& t, const sf::Font& font, const std::string& line, unsigned size,
                  sf::Vector2f pos, sf::Color base, int align) {
    sf::Text full = makeText(font, line, size, base);
    sf::Vector2f at = pos;
    if (align == 0) {   // as centerOrigin would
        const sf::FloatRect b = full.getLocalBounds();
        at -= sf::Vector2f{b.left + b.width / 2.f, b.top + b.height / 2.f};
    }
    full.setPosition(std::round(at.x), std::round(at.y));
    // runs of one colour: a keyword, or everything between keywords
    std::size_t i = 0, runStart = 0;
    sf::Color runCol = base;
    auto flush = [&](std::size_t end) {
        if (end <= runStart) return;
        sf::Text seg = makeText(font, line.substr(runStart, end - runStart), size, runCol);
        const sf::Vector2f p = full.findCharacterPos(runStart);
        seg.setPosition(std::round(p.x), std::round(full.getPosition().y));
        t.draw(seg);
    };
    // the words, then: a name (up to 4 words, capitalised in the text) before a keyword
    struct Tok { std::size_t a, b; };
    std::vector<Tok> toks;
    while (i < line.size()) {
        std::size_t j = i;
        while (j < line.size() && isWordChar(line, j)) ++j;
        if (j == i) { ++i; continue; }
        toks.push_back({i, j});
        i = j;
    }
    const auto& names = namedThings();
    auto paint = [&](std::size_t a, sf::Color c) {
        const sf::Color want = withAlpha(c, static_cast<float>(base.a) / 255.f);
        if (want == runCol) return;
        flush(a);
        runStart = a;
        runCol = want;
    };
    for (std::size_t ti = 0; ti < toks.size();) {
        const unsigned char first = static_cast<unsigned char>(line[toks[ti].a]);
        std::size_t used = 0;
        if (std::isupper(first) || std::isdigit(first)) {
            for (std::size_t n = std::min<std::size_t>(4, toks.size() - ti); n >= 1 && used == 0; --n) {
                bool joined = true;   // the words of a name sit a space or a hyphen apart
                for (std::size_t q = ti; q + 1 < ti + n && joined; ++q)
                    joined = toks[q + 1].a - toks[q].b == 1 && (line[toks[q].b] == ' ' || line[toks[q].b] == '-');
                if (!joined) continue;
                const auto it = names.find(normName(line.substr(toks[ti].a, toks[ti + n - 1].b - toks[ti].a)));
                if (it == names.end()) continue;
                paint(toks[ti].a, it->second);
                paint(toks[ti + n - 1].b, base);
                used = n;
            }
        }
        if (used > 0) { ti += used; continue; }
        sf::Color kc;
        paint(toks[ti].a, keywordColor(line.substr(toks[ti].a, toks[ti].b - toks[ti].a), kc) ? kc : base);
        paint(toks[ti].b, base);
        ++ti;
    }
    flush(line.size());
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
    if (const std::size_t nl = str.find('\n'); nl != std::string::npos) {   // paragraphs: "" = a gap
        std::vector<std::string> lines = wrapText(font, str.substr(0, nl), size, maxW);
        if (nl == 0) lines.push_back("");
        for (const std::string& l : wrapText(font, str.substr(nl + 1), size, maxW)) lines.push_back(l);
        return lines;
    }
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
    return clampf(std::min(size.x / need, size.y * 0.66f / kPanelH), 0.6f, maxZoom);
}

sf::Vector2f panelCenter(sf::Vector2f size, int i, int n, float cy, float cx) {
    if (cx < 0.f) cx = size.x * 0.5f;
    const float total = static_cast<float>(n) * kPanelW + static_cast<float>(n - 1) * kPanelGap;
    const float x0 = cx - total * 0.5f + kPanelW * 0.5f;
    return {x0 + static_cast<float>(i) * (kPanelW + kPanelGap), cy};
}

// An Alchemist's other elements: small hexagons in a row under the type slot,
// where its name would be (inside the panel, so the pointer reaches them).
sf::Vector2f extraElementPos(sf::Vector2f typeCentre, float typeR, int i, int n) {
    return {typeCentre.x + (static_cast<float>(i - 1) - static_cast<float>(n - 1) * 0.5f) * kExtraStep,
            typeCentre.y + typeR + 12.f};
}

bool extraElementAt(sf::Vector2f c, sf::Vector2f m, const BallLoadout& L) {
    Element els[kMaxElements];
    const int ne = L.elements(els);
    if (ne < 2) return false;
    const sf::FloatRect tr = slotRect(c, kSlotType, L);
    const sf::Vector2f pc{tr.left + tr.width * 0.5f, tr.top + tr.height * 0.5f};
    for (int i = 1; i < ne; ++i)
        if (length(m - extraElementPos(pc, tr.width * 0.5f, i, ne - 1)) < 9.f) return true;
    return false;
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
        if (slot >= L.itemSlots) return {};
        if (L.itemSlots > kBaseItemSlots) {   // expanded (6): two columns of three in the same space
            const float step = kSlotStep * static_cast<float>(kBaseItemSlots - 1) / 2.f;
            const float cw = (wd - 6.f) * 0.5f;
            const float cy = top + kPanelItemsTop + static_cast<float>(slot / 2) * step;
            return {c.x - wd * 0.5f + static_cast<float>(slot % 2) * (cw + 6.f), cy - kSlotH * 0.5f, cw, kSlotH};
        }
        const float cy = top + kPanelItemsTop + static_cast<float>(slot) * kSlotStep;
        return {c.x - wd * 0.5f, cy - kSlotH * 0.5f, wd, kSlotH};
    }
    const float hy = top + kPanelHeadY;
    if (slot == kSlotType) {   // a circle left of the ball
        const float r = 17.f;
        return {c.x - 58.f - r, hy - r, 2.f * r, 2.f * r};
    }
    if (isAbilitySlot(slot)) {   // a diamond right of the ball; a Mage's other abilities small beside it
        const int i = slot - kSlotAbility;
        if (i == 0) {
            const float r = 16.f;
            return {c.x + 58.f - r, hy - r, 2.f * r, 2.f * r};
        }
        // in a row under the diamond, where its name would be (as an Alchemist's elements)
        const float r = 6.f;
        const int n = abilityBoxes(L) - 1;
        const sf::Vector2f q{c.x + 58.f + (static_cast<float>(i - 1) - static_cast<float>(n - 1) * 0.5f) * kExtraStep,
                             hy + 16.f * 1.15f + 11.f};
        return {q.x - r, q.y - r, 2.f * r, 2.f * r};
    }
    return {};
}

// A ball's stacked modifiers, compact: "DMG 2  SPD 1". The more kinds it
// has, the shorter each gets - word, then three letters, then one - and if
// even that won't fit `maxW`, the rest go as "+N" (the hover lists them all).
std::string modifierLine(const BallLoadout& L, const sf::Font* font, unsigned size, float maxW) {
    static const char* kWord[kModifierCount] = {"DMG", "SIZE", "SPD", "HEAVY", "CRIT", "REACH",
                                                "ELEM", "SPIN", "LEECH", "MIND", "THROW", "BOUNCE"};
    static const char* kMid[kModifierCount] = {"DMG", "SIZ", "SPD", "BAL", "CRT", "RCH",
                                               "ELM", "SPN", "LCH", "MND", "THR", "BNC"};
    static const char* kOne[kModifierCount] = {"D", "Z", "S", "W", "C", "R", "E", "N", "L", "M", "T", "B"};
    auto build = [&](const char* const* names, const char* gap, const char* sep, int limit) {
        std::string out;
        int shown = 0, hidden = 0;
        for (int i = 0; i < kModifierCount; ++i) {
            if (L.mods[i] <= 0) continue;
            if (shown >= limit) { ++hidden; continue; }
            if (!out.empty()) out += sep;
            out += std::string(names[i]) + gap + std::to_string(L.mods[i]);
            ++shown;
        }
        if (hidden > 0) out += std::string(sep) + "+" + std::to_string(hidden);
        return out;
    };
    auto fits = [&](const std::string& str) {
        return !font || makeLabel(*font, str, size, theme::textHi).getLocalBounds().width <= maxW;
    };
    std::string line = build(kWord, " ", "  ", kModifierCount);
    if (fits(line)) return line;
    line = build(kMid, "", " ", kModifierCount);
    if (fits(line)) return line;
    for (int limit = kModifierCount; limit >= 1; --limit) {
        line = build(kOne, "", " ", limit);
        if (fits(line)) return line;
    }
    return line;
}

// A class's items are a touch muted; a god's items (the same hue) shine
// brighter - that's how an ascended item reads at a glance (2026-10-06).
sf::Color tagColor(ItemTag t) {
    const sf::Color c = roleColor(tagRole(t));   // ItemTag::None -> Normal: grey
    if (t == ItemTag::None) return c;
    return isGod(t) ? lerpColor(c, sf::Color::White, 0.3f) : lerpColor(c, sf::Color(110, 114, 128), 0.3f);
}

sf::Color traitColor(Trait t) {
    switch (t) {
        case Trait::Impact:    return theme::ember;
        case Trait::Swift:     return theme::accent;
        case Trait::Arcane:    return theme::puSurge;
        case Trait::Elemental: return theme::venom;
        case Trait::Squad:     return theme::core;
    }
    return theme::textLo;
}

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

namespace {

// Would this card make (2nd item of the tag) or ascend (4th) a ball? A copy it
// already has only levels up, so that doesn't count. "" when neither.
std::string classMarkText(UpgradeKind k, const std::vector<BallLoadout>& balls) {
    const ItemTag t = itemTag(k);
    if (t == ItemTag::None) return "";
    int best = 0;   // 1 = makes the class, 2 = ascends
    for (int b = 0; b < static_cast<int>(balls.size()); ++b) {
        const BallLoadout& L = balls[static_cast<std::size_t>(b)];
        if (!upgradeFitsTeam(k, balls, b) || upgradeLevelsUp(k, L)) continue;
        bool free = false;
        for (int i = 0; i < L.itemSlots; ++i) free = free || L.gear[i] < 0;
        if (!free) continue;
        const int n = L.tagCount(t);
        if (n == 3) best = 2;
        else if (n == 1) best = std::max(best, 1);
    }
    if (best == 0) return "";
    const BallRole role = tagRole(t);
    return best == 2 ? std::string("ascends: ") + ascendedName(role) : std::string("makes a ") + roleName(role);
}

}  // namespace

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
    // Resting, a card is its icon, name and class; under the pointer the icon
    // gives way and what it does takes its place.
    const float hv = clampf(st.hover, 0.f, 1.f);
    const float h = hv * hv * (3.f - 2.f * hv);
    // How rare: its name and pips, filled up to the tier.
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
    drawLabel(w, font, tierName(tier), 12, {r.left + pad, top}, withAlpha(tc, a), -1);

    const float cx = r.left + r.width * 0.5f;
    if (h < 0.99f) drawIcon(w, k, {cx, r.top + 68.f - 18.f * h}, 25.f * clampf(st.pop, 0.f, 1.1f), a * (1.f - h));

    // The name.
    const unsigned ts = r.width > 230.f ? theme::fsItem : theme::fsHeading;
    sf::Color titleCol = theme::textHi;
    if (cat == UpgradeCat::Element || cat == UpgradeCat::Ability) titleCol = iconColor(k);
    else if (tag != ItemTag::None || cat == UpgradeCat::Relic) titleCol = lerpColor(nameColor(k), theme::textHi, 0.3f);
    sf::Text title = makeText(font, info.title, ts, withAlpha(titleCol, a));
    centerOrigin(title);
    const float sc = clampf(st.pop, 0.f, 1.1f);
    title.setScale(sc, sc);
    title.setPosition(std::round(cx), std::round(r.top + 116.f - 62.f * h));
    w.draw(title);

    // Which class: a solid badge.
    float y = r.top + 146.f - 60.f * h;
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
    // The squad's class cap: no ball can take it (2026-10-22).
    if (cat == UpgradeCat::Item && tag != ItemTag::None && !isGod(tag) && !balls.empty()) {
        bool any = false;
        for (int b = 0; b < static_cast<int>(balls.size()) && !any; ++b) any = classCapFits(k, balls, b);
        if (!any) {
            drawLabel(w, font, std::string(itemTagName(tag)) + " is on " + std::to_string(cfg::team::classCap) + " balls already",
                      10, {cx, y}, withAlpha(theme::coreLow, a));
            y += 20.f;
        }
    }
    // Would it make or ascend a class? An outlined chip in the class colour,
    // inside the card under the badge (it used to hang off the bottom edge).
    if (const std::string mark = st.classMark ? classMarkText(k, balls) : std::string(); !mark.empty()) {
        const sf::Color col = tagColor(tag);
        const float cw = makeLabel(font, mark, 10, col).getLocalBounds().width + 20.f;
        const sf::FloatRect chip{cx - cw * 0.5f, y - 9.f, cw, 18.f};
        draw::box(w, chip, theme::corner, withAlpha(lerpColor(theme::bgDeep, col, 0.25f), 0.97f * a),
                  withAlpha(theme::bgDeep, 0.97f * a), withAlpha(col, 0.85f * a), 1.f);
        drawLabel(w, font, mark, 10, {cx, y}, withAlpha(lerpColor(col, sf::Color::White, 0.15f), a));
        y += 24.f;
    }

    // What it does: only under the pointer - first the class's squad traits.
    if (h < 0.01f) return;
    const float lineH = 18.f;
    y += 4.f;
    if (Trait ta, tb; tag != ItemTag::None && traitsOf(isGod(tag) ? baseOf(tag) : tag, ta, tb)) {
        const std::string tl = std::string(traitName(ta)) + "  +  " + traitName(tb);
        drawLabel(w, font, tl, 10, {cx, y}, withAlpha(theme::textLo, 0.8f * a * h));
        y += lineH;
    }
    const float bottom = r.top + r.height - st.bottomReserve - 6.f;
    for (const std::string& l : wrapText(font, info.desc, theme::fsSmall, r.width - 28.f)) {
        if (y > bottom) break;
        drawRichText(w, font, l, theme::fsSmall, {cx, y}, withAlpha(theme::textLo, a * h), 0);
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
    (void)font;   // (its "makes a" chip moved inside the card: drawPickCard)
    (void)balls;
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
    ItemTag roles[3];
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
    look.legend = look.ascended;   // all 4 items of a class: its cosmetic, as in the arena
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
        const unsigned size = sr.width > 120.f ? theme::fsSmall : sr.width > 90.f ? 12u : 11u;

        if (s == kSlotType) {   // the type: a hexagon of the element's colour, its name under it
            const sf::Vector2f pc{sr.left + sr.width * 0.5f, cy};
            const float r = sr.width * 0.5f;
            constexpr float kHexRot = kPi / 6.f;   // flat top
            if (takes) draw::polygonOutline(w, pc, r + 5.f, 6, kHexRot, 1.5f, withAlpha(theme::accent, 0.7f * sa));
            const bool plague = L.element() == Element::Plague;   // Nergal's own: always there, black
            if (kind < 0 && !plague) {
                draw::polygonOutline(w, pc, r, 6, kHexRot, 1.5f, withAlpha(theme::textDim, (0.45f + 0.4f * lit) * sa));
                return;
            }
            const sf::Color ecol = elementColor(L.element());
            if (plague) {   // solid black, a bruise-violet edge so it reads on the dark panel
                draw::polygon(w, pc, r, 6, kHexRot, withAlpha(ecol, sa), withAlpha(ecol, sa));
                draw::polygonOutline(w, pc, r, 6, kHexRot, 2.f, withAlpha(theme::plagueEdge, (0.9f + 0.1f * lit) * sa));
            } else {
                draw::polygon(w, pc, r, 6, kHexRot, withAlpha(lerpColor(ecol, theme::bgDeep, 0.35f), 0.95f * sa),
                              withAlpha(lerpColor(ecol, theme::bgDeep, 0.6f), 0.95f * sa));
                draw::polygonOutline(w, pc, r, 6, kHexRot, 2.f,
                                     withAlpha(lerpColor(ecol, sf::Color::White, 0.2f), (0.9f + 0.1f * lit) * sa));
            }
            {   // what the element does, drawn; its level small at the corner
                const sf::Color ink = withAlpha(lerpColor(ecol, sf::Color::White, 0.55f), sa);
                const Glyph g = plague ? Glyph::Skull
                                       : glyphOf(static_cast<UpgradeKind>(static_cast<int>(UpgradeKind::ElemFire) +
                                                                          static_cast<int>(L.element()) - 1));
                drawGlyph(w, g, pc, r * 0.55f, plague ? withAlpha(theme::plagueEdge, sa) : ink);
            }
            if (L.levelAt(s) > 1)
                drawCentered(w, font, std::to_string(L.levelAt(s)), 10, {pc.x + r * 0.95f, pc.y - r * 0.8f}, withAlpha(theme::textHi, sa));
            Element els[kMaxElements];   // an Alchemist's other elements: small hexagons under it, in its name's place
            const int ne = L.elements(els);
            if (ne < 2) {
                sf::Text nm = makeText(font, elementName(L.element()), 12, withAlpha(plague ? theme::plagueEdge : ecol, sa));
                centerOrigin(nm);
                nm.setPosition(std::round(pc.x), std::round(pc.y + r + 12.f));
                w.draw(nm);
            }
            for (int i = 1; i < ne; ++i) {
                const sf::Color c2 = elementColor(els[i]);
                const sf::Vector2f q = extraElementPos(pc, r, i, ne - 1);
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
            drawGlyph(w, glyphOf(static_cast<UpgradeKind>(kind)), pc, r * 0.6f,
                      withAlpha(lerpColor(theme::ability, sf::Color::White, 0.55f), sa));
            if (L.levelAt(s) > 1 && r > 8.f)
                drawCentered(w, font, std::to_string(L.levelAt(s)), 10u, {pc.x + r * 1.05f, pc.y - r * 0.85f}, withAlpha(theme::textHi, sa));
            // The main one's name under it - a long one ("Magic missile") in smaller type, kept
            // inside the panel; a Mage's other abilities take that row instead.
            if (s == kSlotAbility && abilityBoxes(L) < 2) {
                for (unsigned fs : {12u, 11u, 10u, 9u}) {
                    sf::Text nm = makeText(font, upgradeInfo(static_cast<UpgradeKind>(kind)).title, fs,
                                           withAlpha(theme::ability, sa));
                    if (nm.getLocalBounds().width >= 2.f * (kPanelW * 0.5f - 58.f) - 6.f && fs > 9u) continue;
                    centerOrigin(nm);
                    nm.setPosition(std::round(pc.x), std::round(pc.y + r * 1.15f + 11.f));
                    w.draw(nm);
                    break;
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
        if (isGod(itemTag(k))) {   // a god's item: a bright edge and a sheen sweeping across it
            static sf::Clock godClock;
            const float tt = godClock.getElapsedTime().asSeconds();
            draw::box(w, sr, 0.f, sf::Color::Transparent, sf::Color::Transparent, withAlpha(cc, 0.9f * sa), 1.5f);
            const float u = std::fmod(tt * 0.6f + sr.left * 0.003f, 1.6f) - 0.3f;   // a pass, then a rest
            if (u >= 0.f && u <= 1.f) {
                const float x = sr.left + sr.width * u;
                draw::line(w, {x - 6.f, sr.top + sr.height}, {x + 6.f, sr.top}, 3.f, withAlpha(sf::Color::White, 0.35f * sa));
            }
        }
        // rarity: pips at the right end, the brightness ramp
        const int rank = static_cast<int>(upgradeTier(k));
        const sf::Color tcol = tierColor(upgradeTier(k));
        const bool narrow = sr.width < 120.f;   // an expanded ball's two columns: a tighter rarity mark
        float px = sr.left + sr.width - (narrow ? 5.f : 7.f);
        for (int p = 0; p <= rank; ++p) {   // (narrow: along the bottom edge, clear of the name)
            draw::polygon(w, {px, narrow ? sr.top + sr.height - 4.f : cy}, narrow ? 1.8f : 2.6f, 4, 0.f, withAlpha(tcol, sa), withAlpha(tcol, sa));
            px -= narrow ? 4.5f : 6.f;
        }
        if (narrow) px = sr.left + sr.width - 3.f;   // (the pips sit in the corner: the name gets the width)
        std::string t = upgradeInfo(k).title;
        if (L.levelAt(s) > 1) t += (sr.width > 90.f ? "  Lv" : " ") + std::to_string(L.levelAt(s));
        // what it does, drawn, just past the spine
        const float gr = std::min(sr.height * 0.3f, narrow ? 5.5f : 9.f);
        drawGlyph(w, glyphOf(k), {sr.left + 6.f + gr + 2.f, cy}, gr, withAlpha(lerpColor(cc, sf::Color::White, 0.45f), sa));
        const float textL = sr.left + 6.f + 2.f * gr + 4.f, textR = px + 2.f;   // between the glyph and the pips
        // the name, as large as fits between them - never over the glyph
        unsigned fs = size;
        while (fs > 9u && makeText(font, t, fs, theme::textHi).getLocalBounds().width > textR - textL - 2.f) --fs;
        while (t.size() > 3 && makeText(font, t, fs, theme::textHi).getLocalBounds().width > textR - textL - 2.f)
            t = t.substr(0, t.size() - 2) + ".";
        drawCentered(w, font, t, fs, {(textL + textR) * 0.5f, cy - 1.f}, withAlpha(theme::textHi, sa));
    };
    for (int i = 0; i < L.itemSlots; ++i) slotBox(i, "empty", false);
    slotBox(kSlotType, "type", false);
    const int open = abilitySlotCount(L);
    for (int i = 0; i < abilityBoxes(L); ++i) slotBox(kSlotAbility + i, "ability", i >= open);

    const std::string ml = modifierLine(L, &font, 11, kPanelW - 28.f);
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
        if (part == kSlotType && L.element() == Element::Plague) {   // Nergal's own element
            title = "Black plague";
            desc = "Nergal's own element, and his only - no other element goes on him. It reacts with every element your other balls bring, "
                   "each its own way: fire a pyre, ice cold death, poison pestilence, electric fever, water a miasma, stone a mass grave.";
            return true;
        }
        if (kind < 0) {
            if (isItemSlot(part)) {
                title = "Empty item slot";
                desc = "Items go here, 4 per ball (6 once a Shrine expands it).\n\n2 items of a tag: gets that class.\n4 items of a tag: its full form - and a Shrine can ascend it.";
            } else if (part == kSlotType) {
                title = "Type slot";
                desc = "the ball's element goes here - one per ball (an Alchemist holds 2: click the small one to make it the first); a new one swaps it. It doesn't count toward a class.";
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
        if (k == UpgradeKind::Cocoon)   // its level counts the fights left
            title += "  hatches in " + std::to_string(L.levelAt(part)) + (L.levelAt(part) == 1 ? " fight" : " fights");
        else
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
        ItemTag roles[3];
        const int n = L.roles(roles);
        const ItemTag asc = L.ascended();
        title.clear();
        desc.clear();
        for (int i = 0; i < n; ++i) {   // per class: what it does, then its two steps
            const BallRole r = tagRole(roles[i]);
            const int c = L.tagCount(roles[i]);
            if (i > 0) { title += " / "; desc += "\n\n"; }
            title += roles[i] == asc ? ascendedName(r) : roleName(r);
            std::string what = roleDesc(r);
            if (!what.empty()) what[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(what[0])));
            desc += what + ".\n";
            if (std::string tr = roleTraits(r); !tr.empty()) {
                tr[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(tr[0])));
                desc += tr + ".\n";
            }
            desc += "\n";
            desc += std::string("2 items: ") + roleName(r) + "  (done)\n";
            desc += std::string("4 items: ") + ascendedName(r) + (c >= 4 ? "  (done)" : "  (" + std::to_string(c) + "/4)");
            std::string more = ascendedDesc(r);
            if (!more.empty()) more[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(more[0])));
            desc += "\n" + more + ".";
        }
        if (n == 0) {   // no class yet: what it's building toward
            title = roleName(BallRole::Normal);
            desc = "No class yet.\n\n2 items of a tag: gets that class.\n4 items of a tag: ascends.";
            std::string counts;
            for (int i = 0; i < kClassCount; ++i) {
                const int c = L.tagCount(classTag(i));
                if (c <= 0) continue;
                counts += std::string("\n") + itemTagName(classTag(i)) + ": " + std::to_string(c) + "/2";
            }
            if (!counts.empty()) desc += "\n" + counts;
        }
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

// A Tiamat dragon's item: how grown its dragon is.
float loadoutGrowth(const BallLoadout& L, int part, std::string& stage) {
    namespace T = cfg::tiamat;
    if (part == kPanelPartBall && L.ascended() == ItemTag::Tiamat) {   // her mount: always ridden, a hatchling at first
        const float xp = L.dragonXp[5];
        stage = std::string("Mount: ") + (xp >= T::stageXp[2] ? "Adult - fully grown" : xp >= T::stageXp[1] ? "Young" : "Hatchling");
        return std::clamp(xp / T::stageXp[2], 0.f, 1.f);
    }
    if (!isItemSlot(part)) return -1.f;
    int kind = -1;
    switch (static_cast<UpgradeKind>(L.gear[part])) {
        case UpgradeKind::FrostWyrm: kind = 0; break;
        case UpgradeKind::StormDrake: kind = 1; break;
        case UpgradeKind::AncientGold: kind = 2; break;
        case UpgradeKind::Devourer: kind = 3; break;
        case UpgradeKind::Wyrmling: kind = 4; break;
        default: return -1.f;
    }
    const float xp = L.dragonXp[kind];
    const char* names[4] = {"Egg", "Hatchling", "Young", "Adult - fully grown"};
    int st = 0;
    while (st < 3 && xp >= T::stageXp[st]) ++st;
    stage = names[st];
    return std::clamp(xp / T::stageXp[2], 0.f, 1.f);
}

void drawTooltip(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f mouse, sf::Vector2f screen,
                 const std::string& title, const std::string& desc, sf::Color titleColor, float meter, const std::string& meterLabel) {
    const float wd = 270.f, pad = 10.f, lineH = 17.f;
    const std::vector<std::string> lines = wrapText(font, desc, theme::fsSmall, wd - 2.f * pad);
    float textH = 0.f;   // an empty line is a half-height gap between paragraphs
    for (const std::string& l : lines) textH += l.empty() ? lineH * 0.5f : lineH;
    const float meterH = meter >= 0.f ? 30.f : 0.f;
    const float ht = pad * 2.f + 20.f + textH + meterH;
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
        if (l.empty()) { ly += lineH * 0.5f; continue; }
        drawRichText(w, font, l, theme::fsSmall, {x + pad, ly}, theme::textLo);
        ly += lineH;
    }
    if (meter >= 0.f) {   // growth: its stage, and a bar to adult with a notch at each stage
        ly += 4.f;
        sf::Text m = makeText(font, meterLabel, theme::fsSmall, meter >= 1.f ? titleColor : theme::textHi);
        m.setPosition(std::round(x + pad), std::round(ly));
        w.draw(m);
        const float bx = x + pad, by = ly + 19.f, bw = wd - 2.f * pad;
        draw::box(w, {bx, by, bw, 4.f}, 2.f, withAlpha(theme::textDim, 0.35f), withAlpha(theme::textDim, 0.35f));
        if (meter > 0.f) draw::box(w, {bx, by, bw * meter, 4.f}, 2.f, titleColor, titleColor);
        for (int i = 0; i < 2; ++i) {
            const float nx = bx + bw * cfg::tiamat::stageXp[i] / cfg::tiamat::stageXp[2];
            draw::box(w, {std::round(nx), by - 2.f, 1.f, 8.f}, 0.f, theme::textLo, theme::textLo);
        }
    }
}

}  // namespace sb
