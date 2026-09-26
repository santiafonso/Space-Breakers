// The between-wave run screens: the path map, the shop and the ball / slot
// picker (Equip) shared by Choice cards, shop buys and the forge.

#include <algorithm>
#include <cmath>
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
    draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::bg, col, 0.22f + 0.2f * hover), a),
              withAlpha(lerpColor(theme::bg, col, 0.06f + 0.1f * hover), a),
              withAlpha(col, (0.4f + 0.45f * hover) * a), 1.5f);
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
    hover_ = nodeAt(app, mouse);
    info_ = nodeAt(app, mouse, false);
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
        float rad = n.type == MapNodeType::Boss ? 25.f : 15.f;
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
    const MapNodeType legend[] = {MapNodeType::Combat, MapNodeType::Elite,   MapNodeType::Shop,
                                  MapNodeType::Forge,  MapNodeType::Rest,    MapNodeType::Upgrade,
                                  MapNodeType::Recruit};
    float ly = s.y * 0.36f;
    int legendHover = -1;
    for (MapNodeType t : legend) {
        if (sf::FloatRect(theme::margin + 8.f, ly - 15.f, 150.f, 30.f).contains(mouse_))
            legendHover = static_cast<int>(t);
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

    // The run's pacts, under the legend (hover a chip for its rule).
    bool pactHover = false;
    if (!r.pacts.empty()) {
        sf::Text ph = makeText(app.font(), "Pacts", theme::fsSmall, theme::textLo);
        ph.setPosition(theme::margin + 12.f, ly + 44.f);
        w.draw(ph);
        pactHover = drawPactStrip(app, w, {theme::margin + 10.f, ly + 64.f}, false, mouse_, info_ < 0);
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
    } else if (!pactHover && std::fabs(mouse_.y - 82.f) < 12.f && std::fabs(mouse_.x - s.x * 0.5f) < 170.f) {
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
        drawCentered(w, f, tierName(tier), theme::fsSmall, {cx, rc.top + 64.f}, withAlpha(tierColor(tier), a));

        std::string head = upgradeCatName(cat);
        if (itemTag(k) != ItemTag::None) head += std::string("  -  ") + itemTagName(itemTag(k));
        drawCentered(w, f, head, theme::fsSmall, {cx, rc.top + 16.f},
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
