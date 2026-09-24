// The between-wave run screens: the path map, the shop and the ball / slot
// picker (Equip) shared by Choice cards, shop buys and the forge.

#include <algorithm>
#include <cmath>
#include <string>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
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
    sf::RectangleShape b({r.width, r.height});
    b.setPosition(r.left, r.top);
    b.setFillColor(withAlpha(col, (0.10f + 0.16f * hover) * a));
    b.setOutlineThickness(1.5f);
    b.setOutlineColor(withAlpha(col, (0.4f + 0.45f * hover) * a));
    w.draw(b);
    drawCentered(w, font, label, theme::fsSmall, {r.left + r.width * 0.5f, r.top + r.height * 0.5f - 1.f},
                 withAlpha(theme::textHi, a));
}

// Gold + core line shown on the map and in the shop.
void drawRunStatus(App& app, sf::RenderWindow& w, float y) {
    const sf::Vector2f s = app.size();
    const Core& c = app.world().core();
    const std::string line = std::to_string(app.gold()) + " gold      core " +
                             std::to_string(static_cast<int>(std::ceil(c.hp))) + " / " +
                             std::to_string(static_cast<int>(std::ceil(c.maxHp)));
    drawCentered(w, app.font(), line, theme::fsBody, {s.x * 0.5f, y}, theme::puGolden);
}

sf::Color nodeColor(MapNodeType t) {
    switch (t) {
        case MapNodeType::Combat:  return theme::enemy;
        case MapNodeType::Elite:   return theme::coreLow;
        case MapNodeType::Shop:    return theme::puGolden;
        case MapNodeType::Forge:   return theme::accent;
        case MapNodeType::Rest:    return theme::core;
        case MapNodeType::Upgrade: return theme::puSurge;
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
        case MapNodeType::Boss:    return "B";
    }
    return "";
}

void drawNode(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f p, MapNodeType t, float r,
              float alpha, bool filled) {
    const sf::Color col = nodeColor(t);
    sf::CircleShape c(r, 32);
    c.setOrigin(r, r);
    c.setPosition(p);
    c.setFillColor(withAlpha(col, (filled ? 0.85f : 0.16f) * alpha));
    c.setOutlineThickness(2.f);
    c.setOutlineColor(withAlpha(col, 0.9f * alpha));
    w.draw(c);
    if (t == MapNodeType::Combat) {   // a plain fight: just an enemy dot
        sf::CircleShape d(r * 0.32f, 16);
        d.setOrigin(r * 0.32f, r * 0.32f);
        d.setPosition(p);
        d.setFillColor(withAlpha(filled ? theme::bg : col, alpha));
        w.draw(d);
    } else {
        drawCentered(w, font, nodeGlyph(t), t == MapNodeType::Boss ? theme::fsHeading : theme::fsBody,
                     {p.x, p.y - 2.f}, withAlpha(filled ? theme::bg : theme::textHi, alpha));
    }
}

}  // namespace

// ================================================================ Equip

void EquipScreen::targetAt(App& app, sf::Vector2f mouse, int& ball, int& slot) const {
    ball = slot = -1;
    const sf::Vector2f s = app.size();
    const int n = app.runBallCount();
    const bool forge = app.equipSource() == EquipSource::Forge;
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
        // element always takes over the ball's current element slot. Roles and
        // modifiers just pick the ball.
        if (!upgradeTakesSlot(k)) return;
        slot = defaultSlot(k, L);
        if (elementItemSlot(k) < 0 || L.elementSlot() < 0)
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
    targetAt(app, mouse, hoverBall_, hoverSlot_);
}

void EquipScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const bool forge = app.equipSource() == EquipSource::Forge;
    const UpgradeKind k = app.equipKind();
    const UpgradeInfo info = upgradeInfo(k);

    std::string title, sub, hint;
    if (forge) {
        title = "Forge - pick an item to level up";
        sub = "each level adds half of the item's bonus again (max level " +
              std::to_string(cfg::gold::maxItemLevel) + ")";
        hint = "click an item   -   Esc: leave the forge";
    } else {
        switch (upgradeCat(k)) {
            case UpgradeCat::Role:     title = std::string("Make a ball a ") + info.title; break;
            case UpgradeCat::Modifier: title = std::string(info.title) + " - pick a ball"; break;
            default:                   title = std::string("Equip ") + info.title + " - pick a ball and slot"; break;
        }
        sub = info.desc;
        if (app.equipSource() == EquipSource::Shop) sub += "   (" + std::to_string(app.shopPrice(k)) + " gold)";
        hint = elementItemSlot(k) >= 0 ? "click a ball (one element per ball: it replaces the current one)   -   Esc: back"
             : upgradeTakesSlot(k)     ? "click a slot (a full one gets replaced)   -   Esc / right-click: back"
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
}

// ================================================================ Map

sf::Vector2f MapScreen::nodePos(App& app, int node) const {
    const sf::Vector2f s = app.size();
    const MapNode& n = app.data().run.map.nodes[static_cast<std::size_t>(node)];
    // Top to bottom: row 1 just under the title, the boss at the bottom.
    const float top = 128.f, bottom = s.y - 62.f;
    const float step = (bottom - top) / static_cast<float>(cfg::map::rows);   // rows 1..9 + boss
    const float y = top + static_cast<float>(n.row - 1) * step;
    const float laneGap = 150.f;
    const float left = s.x * 0.5f - laneGap * 0.5f * static_cast<float>(cfg::map::lanes - 1);
    const float x = n.lane < 0 ? s.x * 0.5f : left + laneGap * static_cast<float>(n.lane);
    return {x, y};
}

int MapScreen::nodeAt(App& app, sf::Vector2f mouse) const {
    const int count = static_cast<int>(app.data().run.map.nodes.size());
    for (int i = 0; i < count; ++i)
        if (length(nodePos(app, i) - mouse) < 30.f && app.mapNodeOpen(i)) return i;
    return -1;
}

void MapScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
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
    hover_ = nodeAt(app, mouse);
}

void MapScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const RunState& r = app.data().run;
    const auto& nodes = r.map.nodes;
    const int count = static_cast<int>(nodes.size());

    drawCenteredPop(w, app.font(), "Act " + std::to_string(r.map.act) + "  -  choose your path",
                    theme::fsTitle, {s.x * 0.5f, 42.f}, theme::textHi, introPop(it, 0.f, 0.3f));
    drawRunStatus(app, w, 82.f);

    // Links first. The path you walked is bright, the ways open to you are lit,
    // everything else stays faint.
    for (int i = 0; i < count; ++i) {
        const MapNode& a = nodes[static_cast<std::size_t>(i)];
        for (int j : a.next) {
            const MapNode& b = nodes[static_cast<std::size_t>(j)];
            const bool walked = a.visited && b.visited;
            const bool open = i == r.mapNode && app.mapNodeOpen(j);
            const sf::Vector2f pa = nodePos(app, i), pb = nodePos(app, j);
            const sf::Vector2f d = pb - pa;
            sf::RectangleShape bar({length(d), walked || open ? 3.f : 1.5f});
            bar.setOrigin(0.f, bar.getSize().y * 0.5f);
            bar.setPosition(pa);
            bar.setRotation(std::atan2(d.y, d.x) * 180.f / kPi);
            bar.setFillColor(withAlpha(walked ? theme::textHi : theme::accent,
                                       walked ? 0.55f : (open ? 0.6f : 0.12f)));
            w.draw(bar);
        }
    }

    for (int i = 0; i < count; ++i) {
        const MapNode& n = nodes[static_cast<std::size_t>(i)];
        const float cp = clampf(introPop(it, 0.05f + 0.025f * static_cast<float>(n.row), 0.3f), 0.f, 1.f);
        if (cp <= 0.001f) continue;
        const bool open = app.mapNodeOpen(i);
        const bool here = i == r.mapNode;
        const bool past = n.row <= r.mapRow && !here;
        float rad = n.type == MapNodeType::Boss ? 30.f : 19.f;
        if (open) rad *= 1.f + 0.08f * std::sin(clock_ * 5.f) + (hover_ == i ? 0.18f : 0.f);
        const float a = cp * (open || here ? 1.f : (past ? (n.visited ? 0.7f : 0.2f) : 0.45f));
        drawNode(w, app.font(), nodePos(app, i), n.type, rad, a, n.visited || hover_ == i);
        if (here) {   // you are here
            sf::CircleShape ring(rad + 7.f, 32);
            ring.setOrigin(rad + 7.f, rad + 7.f);
            ring.setPosition(nodePos(app, i));
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(2.f);
            ring.setOutlineColor(withAlpha(theme::textHi, 0.8f));
            w.draw(ring);
        }
    }

    // Left column: the legend. Right column: the hovered node's name + what it does.
    const MapNodeType legend[] = {MapNodeType::Combat, MapNodeType::Elite, MapNodeType::Shop,
                                  MapNodeType::Forge,  MapNodeType::Rest,  MapNodeType::Upgrade};
    float ly = s.y * 0.36f;
    for (MapNodeType t : legend) {
        drawNode(w, app.font(), {theme::margin + 24.f, ly}, t, 11.f, 0.9f, false);
        sf::Text tx = makeText(app.font(), mapNodeName(t), theme::fsSmall, theme::textLo);
        const sf::FloatRect b = tx.getLocalBounds();
        tx.setOrigin(b.left, b.top + b.height * 0.5f);
        tx.setPosition(theme::margin + 44.f, ly);
        w.draw(tx);
        ly += 34.f;
    }
    sf::Text keys = makeText(app.font(), "click a lit node (or 1-4)", theme::fsSmall, theme::textDim);
    keys.setPosition(theme::margin + 12.f, ly + 10.f);
    w.draw(keys);

    if (hover_ >= 0) {
        const MapNodeType t = nodes[static_cast<std::size_t>(hover_)].type;
        const float cx = s.x - 150.f;
        drawCentered(w, app.font(), mapNodeName(t), theme::fsHeading, {cx, s.y * 0.42f}, nodeColor(t));
        float dy = s.y * 0.42f + 30.f;
        for (const std::string& l : wrapText(app.font(), mapNodeDesc(t), theme::fsSmall, 230.f)) {
            drawCentered(w, app.font(), l, theme::fsSmall, {cx, dy}, theme::textLo);
            dy += 18.f;
        }
    }
}

// ================================================================ Shop

namespace {
constexpr float kOfferW = 200.f;
constexpr float kOfferH = 178.f;
constexpr float kOfferGap = 16.f;
}  // namespace

sf::FloatRect ShopScreen::offerRect(App& app, int i) const {
    const sf::Vector2f s = app.size();
    const int n = static_cast<int>(app.data().run.shopOffers.size());
    const float total = static_cast<float>(n) * kOfferW + static_cast<float>(n - 1) * kOfferGap;
    const float x = s.x * 0.5f - total * 0.5f + static_cast<float>(i) * (kOfferW + kOfferGap);
    return {x, s.y * 0.30f, kOfferW, kOfferH};
}

sf::FloatRect ShopScreen::repairRect(App& app) const {
    const sf::Vector2f s = app.size();
    return {s.x * 0.5f - 290.f, s.y * 0.30f + kOfferH + 40.f, 280.f, 36.f};
}

sf::FloatRect ShopScreen::leaveRect(App& app) const {
    const sf::Vector2f s = app.size();
    return {s.x * 0.5f + 10.f, s.y * 0.30f + kOfferH + 40.f, 280.f, 36.f};
}

void ShopScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape)) { app.leaveShop(); return; }
    if (!isLeftClick(e)) return;
    const int n = static_cast<int>(app.data().run.shopOffers.size());
    for (int i = 0; i < n; ++i)
        if (offerRect(app, i).contains(mouse)) { app.buyShopOffer(i); return; }
    if (repairRect(app).contains(mouse)) { app.buyRepair(); return; }
    if (leaveRect(app).contains(mouse)) app.leaveShop();
}

void ShopScreen::update(App& app, float, sf::Vector2f mouse) {
    hover_ = -1;
    const int n = static_cast<int>(app.data().run.shopOffers.size());
    for (int i = 0; i < n; ++i)
        if (offerRect(app, i).contains(mouse)) hover_ = i;
    if (repairRect(app).contains(mouse)) hover_ = 100;
    if (leaveRect(app).contains(mouse)) hover_ = 101;
}

void ShopScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const RunState& r = app.data().run;

    drawCenteredPop(w, app.font(), "Shop", theme::fsTitle, {s.x * 0.5f, s.y * 0.12f}, theme::puGolden,
                    introPop(it, 0.f, 0.3f));
    drawRunStatus(app, w, s.y * 0.12f + 42.f);

    const int n = static_cast<int>(r.shopOffers.size());
    for (int i = 0; i < n; ++i) {
        const auto k = static_cast<UpgradeKind>(r.shopOffers[static_cast<std::size_t>(i)]);
        const UpgradeInfo info = upgradeInfo(k);
        const UpgradeCat cat = upgradeCat(k);
        const bool sold = r.shopSold[static_cast<std::size_t>(i)];
        const int price = app.shopPrice(k);
        const bool afford = app.gold() >= price;
        const float cp = clampf(introPop(it, 0.08f + 0.06f * static_cast<float>(i), 0.3f), 0.f, 1.f);
        const float a = cp * (sold ? 0.3f : 1.f);
        const float h = (hover_ == i && !sold) ? 1.f : 0.f;
        const sf::FloatRect rc = offerRect(app, i);
        const float cx = rc.left + rc.width * 0.5f;

        sf::RectangleShape card({rc.width, rc.height});
        card.setPosition(rc.left, rc.top);
        card.setFillColor(withAlpha(theme::accent, (0.08f + 0.14f * h) * a));
        card.setOutlineThickness(1.5f);
        card.setOutlineColor(withAlpha(theme::accent, (0.35f + 0.5f * h) * a));
        w.draw(card);

        drawCentered(w, app.font(), upgradeCatName(cat), theme::fsSmall, {cx, rc.top + 16.f},
                     withAlpha(catColor(cat), a));
        const int es = elementItemSlot(k);
        drawCentered(w, app.font(), info.title, theme::fsHeading, {cx, rc.top + 46.f},
                     withAlpha(es >= 0 ? elementColor(static_cast<Element>(es + 1)) : theme::textHi, a));
        const auto lines = wrapText(app.font(), info.desc, theme::fsSmall, rc.width - 22.f);
        float y = rc.top + 78.f;
        for (const std::string& l : lines) {
            drawCentered(w, app.font(), l, theme::fsSmall, {cx, y}, withAlpha(theme::textLo, a));
            y += 17.f;
        }
        drawCentered(w, app.font(), sold ? "SOLD" : std::to_string(price) + " gold", theme::fsBody,
                     {cx, rc.top + rc.height - 18.f},
                     withAlpha(sold ? theme::textDim : (afford ? theme::puGolden : theme::coreLow), a));
    }

    const Core& c = app.world().core();
    const bool hurt = c.hp < c.maxHp - 0.5f;
    drawButton(w, app.font(), repairRect(app),
               "Repair core +" + std::to_string(app.repairAmount()) + "  -  " +
                   std::to_string(cfg::gold::priceRepair) + " gold",
               theme::core, hover_ == 100 ? 1.f : 0.f, hurt && app.gold() >= cfg::gold::priceRepair);
    drawButton(w, app.font(), leaveRect(app), "Leave (Esc)", theme::accent, hover_ == 101 ? 1.f : 0.f, true);
}

}  // namespace sb
