// The between-wave run screens: the path map, the shop and the ball / slot
// picker (Equip) shared by Choice cards, shop buys and the forge.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "render/Draw.hpp"
#include "ui/PactScreen.hpp"
#include "ui/Screens.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

bool isKey(const sf::Event& e, sf::Keyboard::Key k) {
    return e.type == sf::Event::KeyPressed && e.key.code == k;
}
bool isLeftClick(const sf::Event& e) {
    return e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left;
}
bool isRightClick(const sf::Event& e) {
    return e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Right;
}

void drawButton(sf::RenderWindow& w, const sf::Font& font, sf::FloatRect r, const std::string& label,
                sf::Color col, float hover, bool enabled) {
    const float a = enabled ? 1.f : 0.35f;
    draw::panel(w, r, col, a, hover);
    drawLabel(w, font, label, 12, {r.left + r.width * 0.5f, r.top + r.height * 0.5f},
              withAlpha(lerpColor(theme::textLo, theme::textHi, 0.5f + 0.5f * hover), a));
}

// Gold + core line shown on the map and in the shop.
void drawRunStatus(App& app, sf::RenderWindow& w, float y) {
    const sf::Vector2f s = app.size();
    const Core& c = app.world().core();
    const std::string gold = std::to_string(app.gold());
    const std::string core = std::to_string(static_cast<int>(std::ceil(c.hp))) + " / " +
                             std::to_string(static_cast<int>(std::ceil(c.maxHp)));
    const sf::Color coreCol = lerpColor(theme::coreLow, theme::core, c.maxHp > 0.f ? c.hp / c.maxHp : 1.f);
    // "GOLD 187      CORE 80 / 80", centred as one line.
    const float capW1 = makeLabel(app.font(), "gold", 11, theme::textDim).getLocalBounds().width;
    const float capW2 = makeLabel(app.font(), "core", 11, theme::textDim).getLocalBounds().width;
    const float v1 = makeText(app.font(), gold, theme::fsBody, theme::puGolden).getLocalBounds().width;
    const float v2 = makeText(app.font(), core, theme::fsBody, coreCol).getLocalBounds().width;
    const float gap = 8.f, sep = 34.f;
    float x = s.x * 0.5f - (capW1 + gap + v1 + sep + capW2 + gap + v2) * 0.5f;
    auto value = [&](const std::string& str, sf::Color col) {
        sf::Text t = makeText(app.font(), str, theme::fsBody, col);
        const sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left, b.top + b.height * 0.5f);
        t.setPosition(std::round(x), std::round(y));
        w.draw(t);
        x += b.width;
    };
    drawLabel(w, app.font(), "gold", 11, {x, y}, withAlpha(theme::puGolden, 0.6f), -1);
    x += capW1 + gap;
    value(gold, theme::puGolden);
    x += sep;
    drawLabel(w, app.font(), "core", 11, {x, y}, withAlpha(coreCol, 0.6f), -1);
    x += capW2 + gap;
    value(core, coreCol);
}

sf::Color nodeColor(MapNodeType t) {
    switch (t) {
        case MapNodeType::Combat:  return theme::enemy;
        case MapNodeType::Elite:   return theme::coreLow;
        case MapNodeType::Shop:    return theme::puGolden;
        case MapNodeType::Forge:   return theme::accent;
        case MapNodeType::Rest:    return theme::core;
        case MapNodeType::Upgrade: return theme::puSurge;
        case MapNodeType::Recruit: return theme::ballMid;
        case MapNodeType::Boss:    return theme::coreLow;
    }
    return theme::textLo;
}

const char* nodeGlyph(MapNodeType t) {
    switch (t) {
        case MapNodeType::Combat:  return "";
        case MapNodeType::Elite:   return "!";
        case MapNodeType::Shop:    return "$";
        case MapNodeType::Forge:   return "^";
        case MapNodeType::Rest:    return "+";
        case MapNodeType::Upgrade: return "?";
        case MapNodeType::Recruit: return "o";
        case MapNodeType::Boss:    return "B";
    }
    return "";
}

void drawNode(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f p, MapNodeType t, float r,
              float alpha, bool filled) {
    // Map nodes are diamonds - straight-edged like the rest of the console -
    // and the boss an octagon, the same silhouette as in the arena.
    const sf::Color col = nodeColor(t);
    const bool boss = t == MapNodeType::Boss;
    const int sides = boss ? 8 : 4;
    const float rot = boss ? kPi / 8.f : 0.f;
    const float rr = boss ? r : r * 1.2f;   // a diamond needs a longer radius to match a disc's weight
    if (filled) {
        draw::polygon(w, p, rr, sides, rot, withAlpha(lerpColor(col, sf::Color::White, 0.15f), 0.9f * alpha),
                      withAlpha(col, 0.8f * alpha));
    } else {
        draw::polygon(w, p, rr, sides, rot, withAlpha(lerpColor(theme::glassTop, col, 0.2f), 0.95f * alpha),
                      withAlpha(theme::glassBottom, 0.95f * alpha));
    }
    draw::polygonOutline(w, p, rr, sides, rot, 1.5f, withAlpha(col, 0.95f * alpha));
    if (t == MapNodeType::Combat) {   // a plain fight: just a small enemy pip
        const float d = r * 0.28f;
        draw::polygon(w, p, d * 1.3f, 4, 0.f, withAlpha(filled ? theme::bg : col, alpha),
                      withAlpha(filled ? theme::bg : col, alpha));
    } else {
        drawCentered(w, font, nodeGlyph(t), boss ? theme::fsHeading : theme::fsSmall + 1u,
                     {p.x, p.y - 1.f}, withAlpha(filled ? theme::bg : theme::textHi, alpha));
    }
}

}  // namespace

// ================================================================ Equip

void EquipScreen::targetAt(App& app, sf::Vector2f mouse, int& ball, int& slot) const {
    ball = slot = -1;
    const sf::Vector2f s = app.size();
    const int n = app.runBallCount();
    // Forge, the shop's forge and selling all point at an item already on a ball.
    const EquipSource src = app.equipSource();
    const bool forge = src == EquipSource::Forge || src == EquipSource::ShopForge || src == EquipSource::Sell;
    const UpgradeKind k = app.equipKind();
    for (int i = 0; i < n; ++i) {
        const sf::Vector2f c = panelCenter(s, i, n, s.y * 0.52f);
        if (std::fabs(mouse.x - c.x) > kPanelW * 0.5f || std::fabs(mouse.y - c.y) > kPanelH * 0.5f) continue;
        if (!app.equipFitsBall(i)) return;
        ball = i;
        const BallLoadout& L = app.data().run.balls[static_cast<std::size_t>(i)];
        if (forge) {
            // The item under the pointer, else the first one that can level.
            for (int sl = 0; sl < kBallSlots; ++sl)
                if (slotRect(c, sl).contains(mouse) && app.equipFitsSlot(i, sl)) slot = sl;
            for (int sl = 0; sl < kBallSlots && slot < 0; ++sl)
                if (app.equipFitsSlot(i, sl)) slot = sl;
            return;
        }
        // Items land in the slot under the pointer, else their default slot. An
        // element always takes over the ball's current element slot, and a
        // duplicate levels up its own slot. Modifiers just pick the ball.
        if (!upgradeTakesSlot(k)) return;
        slot = defaultSlot(k, L);
        if (!L.has(k) && (elementItemSlot(k) < 0 || L.elementSlot() < 0))
            for (int sl = 0; sl < kBallSlots; ++sl)
                if (slotRect(c, sl).contains(mouse)) slot = sl;
        return;
    }
}

void EquipScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape) || isRightClick(e)) {
        app.cancelEquip();
        return;
    }
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code < sf::Keyboard::Num1 + app.runBallCount()) {
        app.confirmEquip(e.key.code - sf::Keyboard::Num1, -1);   // -1: default slot
        return;
    }
    if (!isLeftClick(e)) return;
    int b, sl;
    targetAt(app, mouse, b, sl);
    if (b >= 0) app.confirmEquip(b, sl);
}

void EquipScreen::update(App& app, float, sf::Vector2f mouse) {
    mouse_ = mouse;
    targetAt(app, mouse, hoverBall_, hoverSlot_);
}

void EquipScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const EquipSource src = app.equipSource();
    const bool forge = src == EquipSource::Forge || src == EquipSource::ShopForge;
    const bool sell = src == EquipSource::Sell;
    const UpgradeKind k = app.equipKind();
    const UpgradeInfo info = upgradeInfo(k);

    std::string title, sub, hint;
    if (forge) {
        title = src == EquipSource::ShopForge
            ? "Forge - pick an item to level up (" + std::to_string(cfg::gold::forgeServicePrice) + " gold)"
            : "Forge - pick an item to level up";
        sub = "each level: the item's bonus or chance grows by half, and the ball hits 10% harder (max level " +
              std::to_string(app.forgeCap()) + ")";
        hint = src == EquipSource::ShopForge ? "click an item   -   Esc: back to the shop"
                                             : "click an item   -   Esc: leave the forge";
    } else if (sell) {
        title = "Sell - pick an item to sell";
        sub = "it pays " + std::to_string(static_cast<int>(cfg::gold::sellFrac * 100.f)) +
              "% of its tier price per forge level; its slot is freed";
        hint = "click an item   -   Esc: back to the shop";
    } else {
        switch (upgradeCat(k)) {
            case UpgradeCat::Modifier: title = std::string(info.title) + " - pick a ball"; break;
            default:                   title = std::string("Equip ") + info.title + " - pick a ball and slot"; break;
        }
        sub = info.desc;
        // Hovering a ball that already has it: this pick levels it up.
        if (hoverBall_ >= 0 && hoverBall_ < app.runBallCount()) {
            const BallLoadout& L = app.data().run.balls[static_cast<std::size_t>(hoverBall_)];
            if (upgradeLevelsUp(k, L)) {
                const int lv = L.levelOf(k);
                title = std::string(info.title) + "   Lv " + std::to_string(lv) + " -> " + std::to_string(lv + 1);
                sub = std::string("level up: ") + upgradeLevelDesc(k);
            }
        }
        if (app.equipSource() == EquipSource::Shop) sub += "   (" + std::to_string(app.shopPrice(k)) + " gold)";
        hint = elementItemSlot(k) >= 0 ? "click a ball (one element per ball: it replaces the current one)   -   Esc: back"
             : upgradeTakesSlot(k)     ? "click a slot (a full one gets replaced; a ball that has it levels it up)   -   Esc: back"
                                       : "click a ball   -   Esc / right-click: back";
    }

    drawCenteredPop(w, app.font(), title, theme::fsHeading, {s.x * 0.5f, s.y * 0.20f}, theme::textHi,
                    introPop(it, 0.f, 0.25f));
    drawCentered(w, app.font(), sub, theme::fsSmall, {s.x * 0.5f, s.y * 0.20f + 30.f}, theme::textLo);

    const auto& balls = app.data().run.balls;
    const int n = static_cast<int>(balls.size());
    for (int i = 0; i < n; ++i) {
        const float cp = clampf(introPop(it, 0.04f * static_cast<float>(i), 0.3f), 0.f, 1.f);
        const sf::Vector2f c = panelCenter(s, i, n, s.y * 0.52f) + sf::Vector2f(0.f, (1.f - cp) * 30.f);
        const bool hot = hoverBall_ == i;
        drawLoadoutPanel(w, app.font(), c, balls[static_cast<std::size_t>(i)], cp, hot ? 1.f : 0.f,
                         hot ? hoverSlot_ : -1, !app.equipFitsBall(i));
        drawCentered(w, app.font(), std::to_string(i + 1), theme::fsSmall,
                     {c.x, c.y + kPanelH * 0.5f + 14.f}, withAlpha(theme::textDim, cp));
    }
    drawCentered(w, app.font(), hint, theme::fsSmall, {s.x * 0.5f, s.y * 0.52f + kPanelH * 0.5f + 44.f},
                 theme::textDim);

    for (int i = 0; i < n; ++i) {   // what's in the slot / on the ball under the pointer
        const int part = panelPartAt(panelCenter(s, i, n, s.y * 0.52f), mouse_);
        std::string tt, td;
        if (part >= 0 && loadoutTooltip(balls[static_cast<std::size_t>(i)], part, tt, td)) {
            if (sell && part < kBallSlots && balls[static_cast<std::size_t>(i)].gear[part] >= 0)
                tt = "Sell " + tt + "  for " + std::to_string(app.sellValue(i, part)) + " gold";
            drawTooltip(w, app.font(), mouse_, s, tt, td);
        }
    }
}

// ================================================================ Map

sf::Vector2f MapScreen::nodePos(App& app, int node) const {
    const sf::Vector2f s = app.size();
    const MapNode& n = app.data().run.map.nodes[static_cast<std::size_t>(node)];
    // Bottom to top, like a tree: row 1 at the base, the boss at the crown
    // just under the title.
    const float top = 140.f, bottom = s.y - 50.f;
    const float step = (bottom - top) / static_cast<float>(cfg::map::rows);   // rows 1..9 + boss
    const float y = bottom - static_cast<float>(n.row - 1) * step;
    const float laneGap = 150.f;
    const float left = s.x * 0.5f - laneGap * 0.5f * static_cast<float>(cfg::map::lanes - 1);
    const float x = n.lane < 0 ? s.x * 0.5f : left + laneGap * static_cast<float>(n.lane);
    return {x, y};
}

int MapScreen::nodeAt(App& app, sf::Vector2f mouse, bool openOnly) const {
    // Nearest node within reach (rows sit close together on a long map).
    const int count = static_cast<int>(app.data().run.map.nodes.size());
    int best = -1;
    float bestD = 22.f;
    for (int i = 0; i < count; ++i) {
        const float d = length(nodePos(app, i) - mouse);
        if (d < bestD && (!openOnly || app.mapNodeOpen(i))) { bestD = d; best = i; }
    }
    return best;
}

void MapScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (peek_.handle(e) || peek_.open) return;   // TAB loadout peek: the map waits under it
    if (isKey(e, sf::Keyboard::Escape)) { app.openPause(); return; }
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code <= sf::Keyboard::Num4) {
        // 1-4: the open nodes, left to right
        std::vector<int> open;
        for (int i = 0; i < static_cast<int>(app.data().run.map.nodes.size()); ++i)
            if (app.mapNodeOpen(i)) open.push_back(i);
        const int k = e.key.code - sf::Keyboard::Num1;
        if (k < static_cast<int>(open.size())) app.travelTo(open[static_cast<std::size_t>(k)]);
        return;
    }
    if (!isLeftClick(e)) return;
    const int n = nodeAt(app, mouse);
    if (n >= 0) app.travelTo(n);
}

void MapScreen::update(App& app, float dt, sf::Vector2f mouse) {
    clock_ += dt;
    mouse_ = mouse;
    peek_.update(dt);
    hover_ = peek_.open ? -1 : nodeAt(app, mouse);
    info_ = peek_.open ? -1 : nodeAt(app, mouse, false);
}

void MapScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const RunState& r = app.data().run;
    const auto& nodes = r.map.nodes;
    const int count = static_cast<int>(nodes.size());

    drawLabel(w, app.font(), "act " + std::to_string(r.map.act), 12, {s.x * 0.5f, 16.f},
              withAlpha(theme::accent, clampf(introPop(it, 0.f), 0.f, 1.f)));
    drawCenteredPop(w, app.font(), "Choose your path", theme::fsTitle, {s.x * 0.5f, 46.f}, theme::textHi,
                    introPop(it, 0.f, 0.3f));
    drawRunStatus(app, w, 84.f);

    // Row guides: a faint rule per stage and its number down the left edge of
    // the map, the row you stand on picked out - the map reads like a scale.
    {
        const float ga = clampf(introPop(it, 0.05f, 0.4f), 0.f, 1.f);
        const float x0 = nodePos(app, 0).x - 150.f * 0.5f * static_cast<float>(cfg::map::lanes - 1) - 40.f;
        const float x1 = s.x - x0;
        for (int row = 1; row <= cfg::map::rows; ++row) {
            float y = 0.f;
            for (const MapNode& n : nodes)
                if (n.row == row) { y = nodePos(app, static_cast<int>(&n - nodes.data())).y; break; }
            if (y <= 0.f) continue;
            const bool cur = row == r.mapRow;
            draw::line(w, {x0, y}, {x1, y}, 1.f, withAlpha(cur ? theme::accent : theme::grid, (cur ? 0.18f : 0.06f) * ga));
            char num[8];
            std::snprintf(num, sizeof(num), "%02d", row);
            drawLabel(w, app.font(), num, 10, {x0 - 8.f, y}, withAlpha(cur ? theme::accent : theme::textDim, ga), 1);
        }
    }

    // Links first. The path you walked is bright, the ways open to you are lit,
    // everything else stays faint.
    for (int i = 0; i < count; ++i) {
        const MapNode& a = nodes[static_cast<std::size_t>(i)];
        for (int j : a.next) {
            const MapNode& b = nodes[static_cast<std::size_t>(j)];
            const bool walked = a.visited && b.visited;
            const bool open = i == r.mapNode && app.mapNodeOpen(j);
            const sf::Vector2f pa = nodePos(app, i), pb = nodePos(app, j);
            if (open) {   // the ways open to you: dashes marching toward the next node
                const sf::Vector2f d = pb - pa;
                const float len = length(d);
                const sf::Vector2f u = d / len;
                const float dash = 7.f, period = 13.f, off = std::fmod(clock_ * 22.f, period);
                for (float t = off - period; t < len; t += period) {
                    const float a0 = std::max(0.f, t), a1 = std::min(len, t + dash);
                    if (a1 > a0) draw::line(w, pa + u * a0, pa + u * a1, 2.f, withAlpha(theme::accent, 0.8f));
                }
            } else {
                draw::line(w, pa, pb, walked ? 2.5f : 1.f,
                           withAlpha(walked ? theme::textHi : theme::grid, walked ? 0.6f : 0.16f));
            }
        }
    }

    for (int i = 0; i < count; ++i) {
        const MapNode& n = nodes[static_cast<std::size_t>(i)];
        const float cp = clampf(introPop(it, 0.05f + 0.025f * static_cast<float>(n.row), 0.3f), 0.f, 1.f);
        if (cp <= 0.001f) continue;
        const bool open = app.mapNodeOpen(i);
        const bool here = i == r.mapNode;
        const bool past = n.row <= r.mapRow && !here;
        float rad = n.type == MapNodeType::Boss ? 25.f : 15.f;
        if (open) rad *= 1.f + 0.08f * std::sin(clock_ * 5.f) + (hover_ == i ? 0.18f : 0.f);
        const float a = cp * (open || here ? 1.f : (past ? (n.visited ? 0.7f : 0.2f) : 0.45f));
        drawNode(w, app.font(), nodePos(app, i), n.type, rad, a, n.visited || hover_ == i);
        if (here) {   // you are here: a target lock that breathes
            const sf::Vector2f p = nodePos(app, i);
            const float h = rad + 9.f + 2.f * std::sin(clock_ * 3.f);
            draw::brackets(w, {p.x - h, p.y - h, 2.f * h, 2.f * h}, 7.f, 2.f, withAlpha(theme::textHi, 0.9f * cp));
            drawLabel(w, app.font(), "you", 10, {p.x + h + 8.f, p.y}, withAlpha(theme::textHi, 0.8f * cp), -1);
        }
    }

    // Left column: the legend. Right column: the hovered node's name + what it does.
    const MapNodeType legend[] = {MapNodeType::Combat, MapNodeType::Elite,   MapNodeType::Shop,
                                  MapNodeType::Forge,  MapNodeType::Rest,    MapNodeType::Upgrade,
                                  MapNodeType::Recruit};
    float ly = s.y * 0.36f;
    drawLabel(w, app.font(), "legend", 10, {theme::margin + 8.f, ly - 32.f}, theme::textDim, -1);
    draw::line(w, {theme::margin + 8.f, ly - 22.f}, {theme::margin + 150.f, ly - 22.f}, 1.f,
               withAlpha(theme::arenaEdge, 0.8f));
    int legendHover = -1;
    for (MapNodeType t : legend) {
        if (sf::FloatRect(theme::margin + 8.f, ly - 15.f, 150.f, 30.f).contains(mouse_))
            legendHover = static_cast<int>(t);
        drawNode(w, app.font(), {theme::margin + 24.f, ly}, t, 10.f, 0.9f, false);
        drawLabel(w, app.font(), mapNodeName(t), 11, {theme::margin + 44.f, ly}, theme::textLo, -1);
        ly += 34.f;
    }
    sf::Text keys = makeText(app.font(), "click a lit node (or 1-4)", theme::fsSmall, theme::textDim);
    keys.setPosition(theme::margin + 8.f, ly + 6.f);
    w.draw(keys);
    drawTabHint(app, w, {theme::margin + 8.f, ly + 32.f});

    // The run's pacts, under the legend (hover a chip for its rule).
    bool pactHover = false;
    if (!r.pacts.empty()) {
        sf::Text ph = makeText(app.font(), "Pacts", theme::fsSmall, theme::textLo);
        ph.setPosition(theme::margin + 12.f, ly + 66.f);
        w.draw(ph);
        pactHover = drawPactStrip(app, w, {theme::margin + 10.f, ly + 86.f}, false, mouse_,
                                  info_ < 0 && !peek_.open);
    }

    if (peek_.open) {   // the loadout peek covers the map; its own hover help only
        drawLoadoutOverlay(app, w, false, peek_.latched());
        return;
    }

    if (info_ >= 0) {
        const MapNode& n = nodes[static_cast<std::size_t>(info_)];
        std::string d = mapNodeDesc(n.type);
        if (!app.mapNodeOpen(info_) && info_ != r.mapNode)
            d += n.row <= r.mapRow ? "  (behind you)" : "  (not reachable from here yet)";
        drawTooltip(w, app.font(), mouse_, s, mapNodeName(n.type), d, nodeColor(n.type));
    } else if (legendHover >= 0) {
        const auto t = static_cast<MapNodeType>(legendHover);
        drawTooltip(w, app.font(), mouse_, s, mapNodeName(t), mapNodeDesc(t), nodeColor(t));
    } else if (!pactHover && std::fabs(mouse_.y - 84.f) < 12.f && std::fabs(mouse_.x - s.x * 0.5f) < 170.f) {
        drawTooltip(w, app.font(), mouse_, s, "Gold and core",
                    "gold buys things in shops; the core must survive - rests and shops repair it");
    }
}

// ================================================================ Shop

namespace {
constexpr float kOfferH = 186.f;
constexpr float kOfferGap = 14.f;
constexpr float kOfferTop = 0.27f;   // * size.y
constexpr float kBtnW = 204.f, kBtnH = 36.f, kBtnGap = 12.f;
}  // namespace

int ShopScreen::cardCount(App& app) const {
    const RunState& r = app.data().run;
    return static_cast<int>(r.shopOffers.size()) + (r.shopMystery == 1 ? 1 : 0);
}

sf::FloatRect ShopScreen::offerRect(App& app, int i) const {
    const sf::Vector2f s = app.size();
    const int n = std::max(1, cardCount(app));
    const float wd = std::min(200.f, (s.x - 2.f * (theme::margin + 14.f) - static_cast<float>(n - 1) * kOfferGap) /
                                         static_cast<float>(n));   // room for the Epic halo at the edges
    const float total = static_cast<float>(n) * wd + static_cast<float>(n - 1) * kOfferGap;
    const float x = s.x * 0.5f - total * 0.5f + static_cast<float>(i) * (wd + kOfferGap);
    return {x, s.y * kOfferTop, wd, kOfferH};
}

sf::FloatRect ShopScreen::buttonRect(App& app, int b) const {
    const sf::Vector2f s = app.size();
    const float total = 5.f * kBtnW + 4.f * kBtnGap;
    return {s.x * 0.5f - total * 0.5f + static_cast<float>(b) * (kBtnW + kBtnGap), s.y * kOfferTop + kOfferH + 48.f,
            kBtnW, kBtnH};
}

void ShopScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape)) { app.leaveShop(); return; }
    if (isKey(e, sf::Keyboard::R)) { app.rerollShop(); return; }
    if (!isLeftClick(e)) return;
    const RunState& r = app.data().run;
    const int offers = static_cast<int>(r.shopOffers.size());
    for (int i = 0; i < cardCount(app); ++i) {
        if (!offerRect(app, i).contains(mouse)) continue;
        if (i < offers) app.buyShopOffer(i);
        else app.buyMystery();
        return;
    }
    for (int b = 0; b < 5; ++b) {
        if (!buttonRect(app, b).contains(mouse)) continue;
        switch (b) {
            case 0: app.buyRepair(); break;
            case 1: app.beginShopForge(); break;
            case 2: app.beginSell(); break;
            case 3: app.rerollShop(); break;
            case 4: app.leaveShop(); break;
        }
        return;
    }
}

void ShopScreen::update(App& app, float dt, sf::Vector2f mouse) {
    mouse_ = mouse;
    clock_ += dt;
    hover_ = -1;
    for (int i = 0; i < cardCount(app); ++i)
        if (offerRect(app, i).contains(mouse)) hover_ = i;
    for (int b = 0; b < 5; ++b)
        if (buttonRect(app, b).contains(mouse)) hover_ = 100 + b;
}

void ShopScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const RunState& r = app.data().run;
    const sf::Font& f = app.font();

    drawCenteredPop(w, f, "Shop", theme::fsTitle, {s.x * 0.5f, s.y * 0.09f}, theme::puGolden, introPop(it, 0.f, 0.3f));
    drawRunStatus(app, w, s.y * 0.09f + 40.f);
    drawPactStrip(app, w, {s.x * 0.5f, s.y * 0.09f + 60.f}, true, mouse_, hover_ < 0);

    const int offers = static_cast<int>(r.shopOffers.size());
    for (int i = 0; i < offers; ++i) {
        const auto k = static_cast<UpgradeKind>(r.shopOffers[static_cast<std::size_t>(i)]);
        const UpgradeInfo info = upgradeInfo(k);
        const UpgradeCat cat = upgradeCat(k);
        const bool sold = r.shopSold[static_cast<std::size_t>(i)];
        const int deal = i < static_cast<int>(r.shopDeal.size()) ? r.shopDeal[static_cast<std::size_t>(i)] : 0;
        const int price = app.shopOfferPrice(i);
        const bool afford = app.gold() >= price;
        const float cp = clampf(introPop(it, 0.08f + 0.05f * static_cast<float>(i), 0.3f), 0.f, 1.f);
        const float a = cp * (sold ? 0.3f : 1.f);
        const float h = (hover_ == i && !sold) ? 1.f : 0.f;
        const sf::FloatRect rc = offerRect(app, i);
        const float cx = rc.left + rc.width * 0.5f;

        const Tier tier = upgradeTier(k);
        drawTierFrame(w, rc, tier, h, a, it);
        drawLabel(w, app.font(), tierName(tier), 10, {cx, rc.top + 66.f}, withAlpha(tierColor(tier), a));

        std::string head = upgradeCatName(cat);
        if (itemTag(k) != ItemTag::None) head += std::string("  -  ") + itemTagName(itemTag(k));
        drawLabel(w, app.font(), head, 10, {cx, rc.top + 18.f},
                  withAlpha(itemTag(k) != ItemTag::None ? tagColor(itemTag(k)) : catColor(cat), a));
        const int es = elementItemSlot(k);
        drawCentered(w, f, info.title, theme::fsHeading, {cx, rc.top + 46.f},
                     withAlpha(es >= 0 ? elementColor(static_cast<Element>(es + 1)) : theme::textHi, a));
        float y = rc.top + 86.f;
        for (const std::string& l : wrapText(f, info.desc, theme::fsSmall, rc.width - 22.f)) {
            drawCentered(w, f, l, theme::fsSmall, {cx, y}, withAlpha(theme::textLo, a));
            y += 17.f;
        }

        // Price line: SOLD / FREE (a revealed mystery box) / sale price with the old one / price.
        std::string pl;
        sf::Color pc = afford ? theme::puGolden : theme::coreLow;
        if (sold) { pl = "SOLD"; pc = theme::textDim; }
        else if (deal == 2) { pl = "PAID - take it"; pc = theme::core; }
        else if (deal == 1) pl = std::to_string(price) + " gold  (was " + std::to_string(app.shopPrice(k)) + ")";
        else pl = std::to_string(price) + " gold";
        drawCentered(w, f, pl, theme::fsBody, {cx, rc.top + rc.height - 18.f}, withAlpha(pc, a));
        if (deal == 1 && !sold) {   // a SALE tag on the card's top edge
            const std::string tag = "SALE  -" + std::to_string(app.saleOffPercent()) + "%";
            const sf::FloatRect tr{cx - 48.f, rc.top - 11.f, 96.f, 20.f};
            draw::box(w, tr, theme::corner, withAlpha(theme::coreLow, a), withAlpha(lerpColor(theme::coreLow, theme::bg, 0.3f), a));
            drawCentered(w, f, tag, theme::fsSmall, {cx, tr.top + 9.f}, withAlpha(theme::textHi, a));
        }
    }

    if (r.shopMystery == 1) {   // the mystery box: a pulsing "?" card
        const int i = offers;
        const float cp = clampf(introPop(it, 0.08f + 0.05f * static_cast<float>(i), 0.3f), 0.f, 1.f);
        const float h = hover_ == i ? 1.f : 0.f;
        const sf::FloatRect rc = offerRect(app, i);
        const float cx = rc.left + rc.width * 0.5f;
        drawTierFrame(w, rc, Tier::Epic, h, cp, clock_);
        drawCentered(w, f, "MYSTERY BOX", theme::fsSmall, {cx, rc.top + 16.f}, withAlpha(tierColor(Tier::Epic), cp));
        const float bob = 3.f * std::sin(clock_ * 3.f);
        drawCentered(w, f, "?", theme::fsTitle + 14u, {cx, rc.top + 66.f + bob}, withAlpha(theme::textHi, cp));
        float y = rc.top + 106.f;
        for (const std::string& l : wrapText(f, "a random pick at elite odds - often Rare or better", theme::fsSmall, rc.width - 22.f)) {
            drawCentered(w, f, l, theme::fsSmall, {cx, y}, withAlpha(theme::textLo, cp));
            y += 17.f;
        }
        const bool afford = app.gold() >= app.mysteryPrice();
        drawCentered(w, f, std::to_string(app.mysteryPrice()) + " gold", theme::fsBody, {cx, rc.top + rc.height - 18.f},
                     withAlpha(afford ? theme::puGolden : theme::coreLow, cp));
    }

    // Services row.
    const Core& c = app.world().core();
    const bool hurt = c.hp < c.maxHp - 0.5f;
    bool forgeable = false, sellable = false;
    for (const BallLoadout& L : r.balls)
        for (int sl = 0; sl < kBallSlots; ++sl) {
            if (L.gear[sl] < 0) continue;
            sellable = true;
            if (L.gearLvl[sl] < app.forgeCap()) forgeable = true;
        }
    const int g = app.gold();
    drawButton(w, f, buttonRect(app, 0),
               "Repair core +" + std::to_string(app.repairAmount()) + "  -  " + std::to_string(cfg::gold::priceRepair) + "g",
               theme::core, hover_ == 100 ? 1.f : 0.f, hurt && g >= cfg::gold::priceRepair);
    drawButton(w, f, buttonRect(app, 1), "Forge an item  -  " + std::to_string(cfg::gold::forgeServicePrice) + "g",
               theme::accent, hover_ == 101 ? 1.f : 0.f, forgeable && g >= cfg::gold::forgeServicePrice);
    drawButton(w, f, buttonRect(app, 2), "Sell an item", theme::puGolden, hover_ == 102 ? 1.f : 0.f, sellable);
    drawButton(w, f, buttonRect(app, 3), "Reroll stock (R)  -  " + std::to_string(app.shopRerollPrice()) + "g",
               theme::puSurge, hover_ == 103 ? 1.f : 0.f, g >= app.shopRerollPrice());
    drawButton(w, f, buttonRect(app, 4), "Leave (Esc)", theme::accent, hover_ == 104 ? 1.f : 0.f, true);
    drawCentered(w, f, "one pick is always on sale  -  the mystery box rolls at elite odds  -  hover anything for details",
                 theme::fsSmall, {s.x * 0.5f, buttonRect(app, 0).top + kBtnH + 26.f}, theme::textDim);

    // Hover help.
    if (hover_ >= 0 && hover_ < offers) {
        const auto k = static_cast<UpgradeKind>(r.shopOffers[static_cast<std::size_t>(hover_)]);
        const UpgradeCat cat = upgradeCat(k);
        std::string d = upgradeCatDesc(cat);
        const int deal = hover_ < static_cast<int>(r.shopDeal.size()) ? r.shopDeal[static_cast<std::size_t>(hover_)] : 0;
        if (deal == 1) d += "  On sale: " + std::to_string(app.saleOffPercent()) + "% off, this visit only.";
        if (deal == 2) d += "  You already paid for it (mystery box): click to take it.";
        drawTooltip(w, f, mouse_, s, upgradeCatName(cat), d, catColor(cat));
    } else if (hover_ == offers && r.shopMystery == 1) {
        drawTooltip(w, f, mouse_, s, "Mystery box",
                    "pay now, see it after: one random pick rolled at elite odds (18% Common, 32% Uncommon, 28% Rare, "
                    "15% Epic, 7% Legendary). It stays on the shelf, paid, until you take it.", tierColor(Tier::Epic));
    } else if (hover_ == 100) {
        drawTooltip(w, f, mouse_, s, "Repair",
                    "restores " + std::to_string(app.repairAmount()) +
                        " core HP. The core also heals a little before every fight.", theme::core);
    } else if (hover_ == 101) {
        drawTooltip(w, f, mouse_, s, "Forge",
                    "level up one item a ball carries, like a Forge node: its bonus or chance grows by half and the "
                    "ball hits 10% harder. Max level " + std::to_string(app.forgeCap()) + ".", theme::accent);
    } else if (hover_ == 102) {
        drawTooltip(w, f, mouse_, s, "Sell",
                    "sell an item back for gold (" + std::to_string(static_cast<int>(cfg::gold::sellFrac * 100.f)) +
                        "% of its tier price per forge level) and free its slot. Pick the item on the next screen.",
                    theme::puGolden);
    } else if (hover_ == 103) {
        drawTooltip(w, f, mouse_, s, "Reroll",
                    "replace the stock with new picks (and a new sale). Each reroll at this shop costs " +
                        std::to_string(cfg::gold::rerollStep) + " more. The mystery box stays.", theme::puSurge);
    }
}

}  // namespace sb
