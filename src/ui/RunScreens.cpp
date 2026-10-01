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
#include "ui/CreedScreen.hpp"
#include "ui/Screens.hpp"
#include "ui/UiSound.hpp"
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

// Gold + core + luck line shown on the map and in the shop. Returns the x
// where the line ends.
float drawRunStatus(App& app, sf::RenderWindow& w, float y) {
    const sf::Vector2f s = app.size();
    const Core& c = app.world().core();
    const std::string gold = std::to_string(app.gold());
    const std::string core = std::to_string(static_cast<int>(std::ceil(c.hp))) + " / " +
                             std::to_string(static_cast<int>(std::ceil(c.maxHp)));
    const sf::Color coreCol = lerpColor(theme::coreLow, theme::core, c.maxHp > 0.f ? c.hp / c.maxHp : 1.f);
    const std::string luck = std::to_string(app.luck());
    const sf::Color luckCol = app.luck() > 0 ? theme::puSurge : theme::textLo;
    // "GOLD 187      CORE 80 / 80      LUCK 6", centred as one line.
    const float capW1 = makeLabel(app.font(), "gold", 11, theme::textDim).getLocalBounds().width;
    const float capW2 = makeLabel(app.font(), "core", 11, theme::textDim).getLocalBounds().width;
    const float capW3 = makeLabel(app.font(), "luck", 11, theme::textDim).getLocalBounds().width;
    const float v1 = makeText(app.font(), gold, theme::fsBody, theme::puGolden).getLocalBounds().width;
    const float v2 = makeText(app.font(), core, theme::fsBody, coreCol).getLocalBounds().width;
    const float v3 = makeText(app.font(), luck, theme::fsBody, luckCol).getLocalBounds().width;
    const float gap = 8.f, sep = 34.f;
    float x = s.x * 0.5f - (capW1 + gap + v1 + sep + capW2 + gap + v2 + sep + capW3 + gap + v3) * 0.5f;
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
    x += sep;
    drawLabel(w, app.font(), "luck", 11, {x, y}, withAlpha(luckCol, 0.6f), -1);
    x += capW3 + gap;
    value(luck, luckCol);
    return x;
}

sf::Color nodeColor(MapNodeType t) {
    switch (t) {
        case MapNodeType::Combat:  return theme::enemy;
        case MapNodeType::Elite:   return theme::ember;   // its own warm orange: the fights that pay items
        case MapNodeType::Shop:    return theme::puGolden;
        case MapNodeType::Forge:   return theme::accent;
        case MapNodeType::Rest:    return theme::core;
        case MapNodeType::Upgrade: return theme::puSurge;
        case MapNodeType::Recruit: return theme::ballMid;
        case MapNodeType::Boss:    return theme::coreLow;
        case MapNodeType::Altar:   return theme::pact;
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
        case MapNodeType::Altar:   return "*";
    }
    return "";
}

void drawNode(sf::RenderWindow& w, const sf::Font& font, sf::Vector2f p, MapNodeType t, float r,
              float alpha, bool filled) {
    // Map nodes are diamonds - straight-edged like the rest of the console -
    // and the boss an octagon, the same silhouette as in the arena.
    const sf::Color col = nodeColor(t);
    const bool boss = t == MapNodeType::Boss;
    const bool altar = t == MapNodeType::Altar;   // a hexagon: it isn't a stop like the others
    const int sides = boss ? 8 : (altar ? 6 : 4);
    const float rot = boss ? kPi / 8.f : (altar ? kPi / 6.f : 0.f);
    const bool elite = t == MapNodeType::Elite;
    const float rr = boss ? r : r * (elite ? 1.4f : 1.2f);   // a diamond needs a longer radius to match a disc's weight
    if (elite || altar)   // an elite wears a second, outer frame; an Altar too
        draw::polygonOutline(w, p, rr + 6.f, sides, rot, 1.5f, withAlpha(col, 0.6f * alpha));
    if (filled) {
        draw::polygon(w, p, rr, sides, rot, withAlpha(lerpColor(col, sf::Color::White, 0.15f), 0.9f * alpha),
                      withAlpha(col, 0.8f * alpha));
    } else {
        draw::polygon(w, p, rr, sides, rot, withAlpha(lerpColor(theme::glassTop, col, 0.2f), 0.95f * alpha),
                      withAlpha(theme::glassBottom, 0.95f * alpha));
    }
    draw::polygonOutline(w, p, rr, sides, rot, 1.5f, withAlpha(col, 0.95f * alpha));
    if (altar) {   // an Altar: a small hexagon inside the frame
        draw::polygon(w, p, r * 0.42f, 6, kPi / 6.f, withAlpha(filled ? theme::bg : col, alpha),
                      withAlpha(filled ? theme::bg : col, alpha));
    } else if (t == MapNodeType::Combat) {   // a plain fight: just a small enemy pip
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

namespace {
// The equip picker's panels are magnified to fill the screen (few balls: big
// panels); they lay out in UI units / this.
float equipZoom(App& app) { return panelRowZoom(app.size(), app.runBallCount(), 0.f, 1.15f); }
float equipRowY(App& app) { return app.size().y * 0.58f / equipZoom(app); }
}  // namespace

void EquipScreen::targetAt(App& app, sf::Vector2f mouse, int& ball, int& slot) const {
    ball = slot = -1;
    const float zoom = equipZoom(app);
    const sf::Vector2f s = app.size() / zoom;
    mouse /= zoom;
    const int n = app.runBallCount();
    // Forge, the shop's forge and selling all point at an item already on a ball.
    const EquipSource src = app.equipSource();
    const bool forge = src == EquipSource::Forge || src == EquipSource::ShopForge || src == EquipSource::Sell;
    const UpgradeKind k = app.equipKind();
    for (int i = 0; i < n; ++i) {
        const sf::Vector2f c = panelCenter(s, i, n, equipRowY(app));
        if (std::fabs(mouse.x - c.x) > kPanelW * 0.5f || std::fabs(mouse.y - c.y) > kPanelH * 0.5f) continue;
        if (!app.equipFitsBall(i)) return;
        ball = i;
        const BallLoadout& L = app.data().run.balls[static_cast<std::size_t>(i)];
        const int shown = kSlotAbility + abilityBoxes(L);   // slots drawn on the panel
        if (forge) {
            // The slot under the pointer, else the first one that can level.
            for (int sl = 0; sl < shown; ++sl)
                if (slotRect(c, sl, L).contains(mouse) && app.equipFitsSlot(i, sl)) slot = sl;
            for (int sl = 0; sl < kLoadoutSlots && slot < 0; ++sl)
                if (app.equipFitsSlot(i, sl)) slot = sl;
            return;
        }
        // Picks land in the slot under the pointer if it takes them (items: an
        // item slot, elements: the type slot, abilities: an open ability
        // slot), else their default slot; a duplicate levels up its own slot.
        // Modifiers just pick the ball.
        if (!upgradeTakesSlot(k)) return;
        slot = defaultSlot(k, L);
        if (!L.has(k))
            for (int sl = 0; sl < shown; ++sl)
                if (slotRect(c, sl, L).contains(mouse) && slotAccepts(k, sl, L)) slot = sl;
        return;
    }
}

sf::FloatRect EquipScreen::backRect(sf::Vector2f s) {
    const float wd = 260.f, ht = 34.f;
    return {s.x * 0.5f - wd * 0.5f, s.y - 64.f, wd, ht};
}

// What "Back" returns to: a card you took but haven't placed goes back on the
// table (nothing is spent), the shop, or out of the forge.
static const char* equipBackLabel(EquipSource src) {
    switch (src) {
        case EquipSource::Choice: return "Back to the cards";
        case EquipSource::Forge:  return "Leave the forge";
        default:                  return "Back to the shop";
    }
}

void EquipScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isRightClick(e) || (isLeftClick(e) && backRect(app.size()).contains(mouse))) {
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
    const bool onBack = backRect(app.size()).contains(mouse);
    uisound::hover(this, onBack ? 999 : hoverBall_ >= 0 ? hoverBall_ * 16 + hoverSlot_ + 1 : -1);
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
        hint = src == EquipSource::ShopForge ? "click an item   -   Back: the shop"
                                             : "click an item   -   Back: leave the forge";
    } else if (sell) {
        title = "Sell - pick an item to sell";
        sub = "it pays " + std::to_string(static_cast<int>(cfg::gold::sellFrac * 100.f)) +
              "% of its tier price per forge level; its slot is freed";
        hint = "click an item   -   Back: the shop";
    } else {
        switch (upgradeCat(k)) {
            case UpgradeCat::Modifier: title = std::string(info.title) + " - pick a ball"; break;
            default:                   title = std::string("Equip ") + info.title + " - pick a ball and slot"; break;
        }
        sub = info.desc;
        // Hovering a ball that already has it: this pick levels it up. One
        // whose slot is taken: it swaps what's there.
        if (hoverBall_ >= 0 && hoverBall_ < app.runBallCount()) {
            const BallLoadout& L = app.data().run.balls[static_cast<std::size_t>(hoverBall_)];
            if (upgradeLevelsUp(k, L)) {
                const int lv = L.levelOf(k);
                title = std::string(info.title) + "   Lv " + std::to_string(lv) + " -> " + std::to_string(lv + 1);
                sub = std::string("level up: ") + upgradeLevelDesc(k);
            } else if (hoverSlot_ >= 0 && L.kindAt(hoverSlot_) >= 0) {
                title = std::string("Swap ") + upgradeInfo(static_cast<UpgradeKind>(L.kindAt(hoverSlot_))).title +
                        "  ->  " + info.title;
            }
        }
        if (app.equipSource() == EquipSource::Shop) sub += "   (" + std::to_string(app.shopPrice(k)) + " gold)";
        switch (upgradeCat(k)) {
            case UpgradeCat::Element:
                hint = "click a ball: it goes in the type slot (one element per ball - a new one swaps it)   -   Back / right-click";
                break;
            case UpgradeCat::Ability:
                hint = "click an ability slot (a full one gets swapped; a ball that has it levels it up)   -   Back / right-click";
                break;
            case UpgradeCat::Item:
                hint = "click an item slot (a full one gets replaced; a ball that has it levels it up)   -   Back / right-click";
                break;
            default:
                hint = "click a ball   -   Back / right-click";
                break;
        }
    }

    drawCenteredPop(w, app.font(), title, theme::fsHeading, {s.x * 0.5f, s.y * 0.20f}, theme::textHi,
                    introPop(it, 0.f, 0.25f));
    drawCentered(w, app.font(), sub, theme::fsSmall, {s.x * 0.5f, s.y * 0.20f + 30.f}, theme::textLo);

    (void)hint;   // how to click is on hover (the slot tooltips), not a line of text

    const auto& balls = app.data().run.balls;
    const int n = static_cast<int>(balls.size());
    const float zoom = equipZoom(app);
    app.useUiZoom(zoom);
    const sf::Vector2f zs = s / zoom, zm = mouse_ / zoom;
    const float rowY = equipRowY(app);
    for (int i = 0; i < n; ++i) {
        const float cp = clampf(introPop(it, 0.04f * static_cast<float>(i), 0.3f), 0.f, 1.f);
        const sf::Vector2f c = panelCenter(zs, i, n, rowY) + sf::Vector2f(0.f, (1.f - cp) * 30.f);
        const bool hot = hoverBall_ == i;
        drawLoadoutPanel(w, app.font(), c, balls[static_cast<std::size_t>(i)], cp, hot ? 1.f : 0.f,
                         hot ? hoverSlot_ : -1, !app.equipFitsBall(i),
                         forge || sell || !upgradeTakesSlot(k) ? -1 : static_cast<int>(k));
    }

    for (int i = 0; i < n; ++i) {   // what's in the slot / on the ball under the pointer
        const int part = panelPartAt(panelCenter(zs, i, n, rowY), zm, balls[static_cast<std::size_t>(i)]);
        std::string tt, td;
        if (part >= 0 && loadoutTooltip(balls[static_cast<std::size_t>(i)], part, tt, td)) {
            if (sell && isItemSlot(part) && balls[static_cast<std::size_t>(i)].gear[part] >= 0)
                tt = "Sell " + tt + "  for " + std::to_string(app.sellValue(i, part)) + " gold";
            drawTooltip(w, app.font(), zm, zs, tt, td);
        }
    }
    app.useUiView();

    // "Back": change your mind before the pick is placed (right-click too).
    const sf::FloatRect br = backRect(s);
    const float ba = clampf(introPop(it, 0.15f), 0.f, 1.f);
    const bool onBack = br.contains(mouse_);
    draw::panel(w, br, theme::textLo, ba, onBack ? 1.f : 0.f);
    drawCentered(w, app.font(), equipBackLabel(src), theme::fsSmall, {s.x * 0.5f, br.top + br.height * 0.5f - 1.f},
                 withAlpha(onBack ? theme::textHi : theme::textLo, ba));
}

// ================================================================ Map

namespace {
// The map is taller than the screen: a strip of rows you scroll through, the
// status line pinned on top. Rows are kMapStep apart, row 1 sits at the
// bottom of the view when scroll = 0.
constexpr float kMapStep = 104.f;
constexpr float kMapLaneGap = 190.f;
constexpr float kMapTop = 104.f;      // the view's top edge (under the status line)
constexpr float kMapBottomPad = 70.f; // row 1's distance from the bottom edge at scroll 0
constexpr float kNodeR = 21.f;
constexpr float kBossR = 36.f;
constexpr float kStatusY = 52.f;
constexpr float kTravelTime = 0.42f;   // the spark's trip to a chosen node
constexpr float kIntroSpeed = 1.35f;   // the map's opening plays this much faster than a screen's usual pace
constexpr float kIntroDone = 0.55f;    // ...and nodes can be picked once it's this far in (the rows in view are up)
constexpr float kScrollRate = 12.f;    // how fast the view glides to where you stand
constexpr float kRevealDelay = 0.5f;   // the hidden Altar: a beat, then the path draws itself...
constexpr float kRevealDraw = 1.3f;    // ...over this long, then the Altar lights up
constexpr float kRevealPop = 0.6f;
}  // namespace

float MapScreen::scrollMax(App& app) const {
    const sf::Vector2f s = app.size();
    return std::max(0.f, static_cast<float>(mapRows(app.data().run.map.act)) * kMapStep - (s.y - kMapBottomPad - kMapTop - 60.f));
}

float MapScreen::scrollFor(App& app, int row) const {
    const sf::Vector2f s = app.size();
    // Where you stand sits low in the view so the rows ahead fill it.
    const float want = s.y * 0.74f;
    const float atZero = s.y - kMapBottomPad - static_cast<float>(std::max(row, 1) - 1) * kMapStep;
    return clampf(want - atZero, 0.f, scrollMax(app));
}

sf::Vector2f MapScreen::nodePos(App& app, int node) const {
    const sf::Vector2f s = app.size();
    const MapNode& n = app.data().run.map.nodes[static_cast<std::size_t>(node)];
    // Bottom to top, like a tree: row 1 at the base, the boss at the crown.
    const float y = s.y - kMapBottomPad - static_cast<float>(n.row - 1) * kMapStep + scroll_;
    const float left = s.x * 0.5f - kMapLaneGap * 0.5f * static_cast<float>(cfg::map::lanes - 1);
    const float x = n.lane < 0 ? s.x * 0.5f : left + kMapLaneGap * static_cast<float>(n.lane);
    return {x, y};
}

sf::Vector2f MapScreen::travelFrom(App& app) const {
    const RunState& r = app.data().run;
    if (r.mapNode >= 0) return nodePos(app, r.mapNode);
    // The act's start: just under the trunk (row 1).
    for (int i = 0; i < static_cast<int>(r.map.nodes.size()); ++i)
        if (r.map.nodes[static_cast<std::size_t>(i)].row == 1) return nodePos(app, i) + sf::Vector2f{0.f, kMapStep * 0.7f};
    return {app.size().x * 0.5f, app.size().y};
}

int MapScreen::nodeAt(App& app, sf::Vector2f mouse, bool openOnly) const {
    if (mouse.y < kMapTop) return -1;   // under the pinned status line
    const int count = static_cast<int>(app.data().run.map.nodes.size());
    int best = -1;
    float bestD = kNodeR + 12.f;
    for (int i = 0; i < count; ++i) {
        const float d = length(nodePos(app, i) - mouse);
        if (d < bestD && (!openOnly || app.mapNodeOpen(i))) { bestD = d; best = i; }
    }
    return best;
}

bool MapScreen::ready() const {   // the opening has played: a pick now is on purpose
    return intro() * kIntroSpeed >= kIntroDone && std::fabs(scroll_ - scrollTarget_) < 3.f && revealT_ < 0.f;
}

void MapScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (travelTo_ >= 0) return;   // on the way: the trip plays out
    auto go = [&](int node) {     // a spark runs along the link, then the stop opens (update)
        if (!ready() || !app.mapNodeOpen(node)) return;   // not while the map is still opening
        travelTo_ = node;
        travelT_ = 0.f;
    };
    if (peek_.handle(e)) return;   // TAB loadout peek: the map waits under it
    if (peek_.open) { loadoutDragEvent(app, peek_, e); return; }
    if (isKey(e, sf::Keyboard::Escape)) { app.openPause(); return; }
    // Scroll: wheel, arrows / W S; Space jumps back to where you stand.
    if (e.type == sf::Event::MouseWheelScrolled) {
        scrollTarget_ = clampf(scrollTarget_ + e.mouseWheelScroll.delta * kMapStep * 0.8f, 0.f, scrollMax(app));
        return;
    }
    if (e.type == sf::Event::KeyPressed) {
        const auto k = e.key.code;
        if (k == sf::Keyboard::Up || k == sf::Keyboard::W || k == sf::Keyboard::Down || k == sf::Keyboard::S) {
            const float dir = (k == sf::Keyboard::Up || k == sf::Keyboard::W) ? 1.f : -1.f;
            scrollTarget_ = clampf(scrollTarget_ + dir * kMapStep * 2.f, 0.f, scrollMax(app));
            return;
        }
        if (k == sf::Keyboard::Space) {
            scrollTarget_ = scrollFor(app, app.data().run.mapRow + 1);
            return;
        }
    }
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code <= sf::Keyboard::Num4) {
        // 1-4: the open nodes, left to right
        std::vector<int> open;
        for (int i = 0; i < static_cast<int>(app.data().run.map.nodes.size()); ++i)
            if (app.mapNodeOpen(i)) open.push_back(i);
        const int k = e.key.code - sf::Keyboard::Num1;
        if (k < static_cast<int>(open.size())) go(open[static_cast<std::size_t>(k)]);
        return;
    }
    // Drag the map up / down; a press on a lit node travels instead.
    if (dragging_ && e.type == sf::Event::MouseMoved) {
        scrollTarget_ = scroll_ = clampf(dragScroll0_ + (mouse.y - dragY0_), 0.f, scrollMax(app));
        return;
    }
    if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
        dragging_ = false;
        return;
    }
    if (!isLeftClick(e)) return;
    const int n = nodeAt(app, mouse);
    if (n >= 0) {
        go(n);
        return;
    }
    if (mouse.y > kMapTop) {
        dragging_ = true;
        dragY0_ = mouse.y;
        dragScroll0_ = scroll_;
    }
}

void MapScreen::update(App& app, float dt, sf::Vector2f mouse) {
    clock_ += dt;
    mouse_ = mouse;
    peek_.update(dt);
    if (!scrollInit_) {   // open on where you stand: the next row just above centre
        scrollInit_ = true;
        const float from = scrollFor(app, app.data().run.mapRow);
        scroll_ = from;
        scrollTarget_ = scrollFor(app, app.data().run.mapRow + 1);
    }
    if (app.consumeAltarReveal()) {   // the hidden Altar: show the top of the map and draw its path in
        const auto& nodes = app.data().run.map.nodes;
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
            if (nodes[static_cast<std::size_t>(i)].type == MapNodeType::Altar &&
                nodes[static_cast<std::size_t>(i)].row == app.data().run.map.bossRow())
                altarNode_ = i;
        revealT_ = 0.f;
        scrollTarget_ = scrollMax(app);
    }
    if (revealT_ >= 0.f) {
        const float before = revealT_;
        revealT_ += dt;
        const float lit = kRevealDelay + kRevealDraw;
        if (before < lit && revealT_ >= lit && altarNode_ >= 0) {   // the Altar lights up
            app.effects().flash(theme::pact, 0.35f);
            const sf::Vector2f ap = nodePos(app, altarNode_);   // beside it, on the side away from the boss
            app.effects().addLabel("a hidden path opens", ap + sf::Vector2f{ap.x < app.size().x * 0.5f ? -150.f : 150.f, 0.f},
                                   theme::pact, 22, 2.2f);
            app.audio().bossAppear();
        }
        if (revealT_ > lit + kRevealPop + 0.2f) revealT_ = -1.f;
    }
    if (travelTo_ >= 0) {   // the spark reaches the node: the stop opens
        travelT_ += dt;
        if (travelT_ >= kTravelTime) {
            const int n = travelTo_;
            travelTo_ = -1;
            app.travelTo(n);
            return;   // this screen is closed
        }
    }
    scroll_ += (scrollTarget_ - scroll_) * (1.f - std::exp(-kScrollRate * dt));
    hover_ = peek_.open || travelTo_ >= 0 || !ready() ? -1 : nodeAt(app, mouse);
    info_ = peek_.open ? -1 : nodeAt(app, mouse, false);
    uisound::hover(this, hover_);
    // The lit way follows the pointer and eases in; a trip keeps it on its node.
    const int want = travelTo_ >= 0 ? travelTo_ : hover_;
    if (want >= 0 && want != glowNode_) { glowNode_ = want; hoverGlow_ = 0.f; }
    hoverGlow_ = clampf(hoverGlow_ + dt * (want >= 0 ? 5.f : -5.f), 0.f, 1.f);
    if (hoverGlow_ <= 0.f) glowNode_ = -1;
}

void MapScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro() * kIntroSpeed;
    const RunState& r = app.data().run;
    const auto& nodes = r.map.nodes;
    const int count = static_cast<int>(nodes.size());

    // Nodes fade out toward the top and bottom edges of the view rather than
    // being cut off.
    auto edgeFade = [&](float y) {
        return clampf((y - kMapTop) / 70.f, 0.f, 1.f) * clampf((s.y - 8.f - y) / 50.f, 0.f, 1.f);
    };

    // The hidden Altar drawing itself in: how much of its path shows, and how
    // lit the Altar is (both 1 once it's done).
    const float revealDraw = revealT_ < 0.f ? 1.f : clampf((revealT_ - kRevealDelay) / kRevealDraw, 0.f, 1.f);
    const float altarLit = revealT_ < 0.f ? 1.f : clampf((revealT_ - kRevealDelay - kRevealDraw) / kRevealPop, 0.f, 1.f);
    const float glow = hoverGlow_ * hoverGlow_ * (3.f - 2.f * hoverGlow_);   // eased

    // Links first. The path you walked is bright, the ways open to you are
    // clear lines, everything else stays faint. The way under the pointer
    // lights up and flows toward its node.
    for (int i = 0; i < count; ++i) {
        const MapNode& a = nodes[static_cast<std::size_t>(i)];
        for (int j : a.next) {
            const MapNode& b = nodes[static_cast<std::size_t>(j)];
            const bool walked = a.visited && b.visited;
            const bool open = i == r.mapNode && app.mapNodeOpen(j);
            const sf::Vector2f pa = nodePos(app, i), pb = nodePos(app, j);
            const float fa = std::min(edgeFade(pa.y), edgeFade(pb.y)) * 0.5f +
                             0.5f * std::max(edgeFade(pa.y), edgeFade(pb.y));
            if (fa <= 0.01f) continue;
            // stop short of the nodes so a line never runs through an icon
            const sf::Vector2f d = pb - pa;
            const float len = length(d);
            const sf::Vector2f u = d / len;
            const float ra = a.type == MapNodeType::Boss ? kBossR : kNodeR * 1.25f;
            const float rb = b.type == MapNodeType::Boss ? kBossR : kNodeR * 1.25f;
            const sf::Vector2f p0 = pa + u * (ra + 6.f), p1 = pb - u * (rb + 6.f);
            const bool toAltar = j == altarNode_, fromAltar = i == altarNode_;
            if (toAltar || fromAltar) {   // the hidden path: in the pact colour, drawn in on its reveal
                const float k = toAltar ? revealDraw : altarLit;
                if (k <= 0.f) continue;
                const sf::Vector2f end = p0 + (p1 - p0) * k;
                const bool lit = open || walked;
                draw::line(w, p0, end, 6.f, withAlpha(theme::pact, (0.12f + 0.2f * (1.f - altarLit)) * fa));
                draw::line(w, p0, end, lit ? 2.5f : 1.5f, withAlpha(theme::pact, (lit ? 0.85f : 0.45f) * fa));
                if (toAltar && k < 1.f) draw::disc(w, end, 4.f, theme::textHi, theme::pact, {1.f, 1.f}, 12);   // the tip
                if (!(open && j == glowNode_)) continue;
            }
            if (open && j == glowNode_ && glow > 0.f) {   // the way you're about to take
                const sf::Color c = lerpColor(theme::accent, theme::textHi, 0.25f);
                draw::line(w, p0, p1, 2.f + 5.f * glow, withAlpha(theme::accent, 0.16f * glow * fa));
                draw::line(w, p0, p1, 2.f + 1.f * glow, withAlpha(c, (0.7f + 0.3f * glow) * fa));
                for (int k = 0; k < 3; ++k) {   // sparks flowing toward the node
                    const float t = std::fmod(clock_ * 0.9f + static_cast<float>(k) / 3.f, 1.f);
                    draw::disc(w, p0 + (p1 - p0) * t, 2.5f, theme::textHi, c, {1.f, 1.f}, 10);
                }
            } else if (open) draw::line(w, p0, p1, 2.f, withAlpha(theme::accent, (0.7f - 0.35f * glow) * fa));
            else draw::line(w, p0, p1, walked ? 2.5f : 1.f,
                            withAlpha(walked ? theme::textLo : theme::grid, (walked ? 0.7f : 0.22f) * fa));
        }
    }

    for (int i = 0; i < count; ++i) {
        const MapNode& n = nodes[static_cast<std::size_t>(i)];
        const sf::Vector2f p = nodePos(app, i);
        const float fade = edgeFade(p.y);
        const float cp = clampf(introPop(it, 0.05f + 0.02f * static_cast<float>(std::abs(n.row - r.mapRow)), 0.3f),
                                0.f, 1.f);
        if (cp * fade <= 0.001f) continue;
        const bool open = app.mapNodeOpen(i);
        const bool here = i == r.mapNode;
        const bool past = n.row <= r.mapRow && !here;
        float rad = n.type == MapNodeType::Boss ? kBossR : kNodeR;
        if (open && glowNode_ == i) rad *= 1.f + 0.14f * glow;
        // Ahead: plain but readable. Open: full. Behind: only the path you took.
        float a = cp * fade * (open || here ? 1.f : (past ? (n.visited ? 0.55f : 0.14f) : 0.5f));
        if (i == altarNode_) {   // the hidden Altar: pops in at the end of its path, then breathes
            if (altarLit <= 0.f) continue;
            a *= altarLit;
            rad *= 1.f + 0.5f * (1.f - altarLit) * altarLit;   // a swell as it lights
            const float br = 0.5f + 0.5f * std::sin(clock_ * 2.2f);
            draw::polygonOutline(w, p, rad * 1.2f + 12.f + 4.f * br, 6, kPi / 6.f, 1.5f,
                                 withAlpha(theme::pact, (0.2f + 0.2f * br) * a));
            if (altarLit < 1.f)   // the flare as it lights
                draw::ring(w, p, rad * (1.5f + 3.f * altarLit), 2.f, withAlpha(theme::pact, 0.8f * (1.f - altarLit)));
        }
        drawNode(w, app.font(), p, n.type, rad, a, open && glowNode_ == i && glow > 0.5f);
        if (here) {   // you are here: still brackets around the node
            const float h = rad * 1.25f + 10.f;
            draw::brackets(w, {p.x - h, p.y - h, 2.f * h, 2.f * h}, 8.f, 2.f, withAlpha(theme::textHi, 0.8f * cp * fade));
        }
    }

    // A trip: a spark runs from where you stand to the chosen node, which
    // flares as it arrives.
    if (travelTo_ >= 0) {
        const float t = clampf(travelT_ / kTravelTime, 0.f, 1.f);
        const float e = t * t * (3.f - 2.f * t);
        const sf::Vector2f a = travelFrom(app), b = nodePos(app, travelTo_);
        const sf::Color c = nodeColor(nodes[static_cast<std::size_t>(travelTo_)].type);
        for (int k = 1; k <= 4; ++k) {   // a short fading tail
            const float te = std::max(0.f, e - 0.05f * static_cast<float>(k));
            draw::disc(w, a + (b - a) * te, 6.f - static_cast<float>(k), withAlpha(theme::textHi, 0.5f - 0.1f * static_cast<float>(k)),
                       withAlpha(c, 0.4f - 0.08f * static_cast<float>(k)), {1.f, 1.f}, 12);
        }
        draw::disc(w, a + (b - a) * e, 7.f, theme::textHi, c, {1.f, 1.f}, 16);
        if (t > 0.6f) {
            const float f2 = (t - 0.6f) / 0.4f;
            draw::ring(w, b, kNodeR * (1.3f + 1.2f * f2), 2.f, withAlpha(c, 0.9f * (1.f - f2)));
        }
    }

    // Pinned header: a band over the scrolled map with the act and the run's
    // status line.
    {
        sf::RectangleShape band({s.x, kMapTop});
        band.setFillColor(withAlpha(theme::bg, 0.92f));
        w.draw(band);
        draw::line(w, {0.f, kMapTop}, {s.x, kMapTop}, 1.f, withAlpha(theme::arenaEdge, 0.5f));
    }
    const float ha = clampf(introPop(it, 0.f), 0.f, 1.f);
    drawLabel(w, app.font(), "act " + std::to_string(r.map.act), 14, {theme::margin, kStatusY},
              withAlpha(theme::accent, ha), -1);
    const float statusEnd = drawRunStatus(app, w, kStatusY);
    const float ironX = statusEnd + 34.f;
    if (app.ironCoreAlive())   // "Iron core": a quiet marker while no repair has been made this act
        drawLabel(w, app.font(), "iron core", 12, {ironX, kStatusY}, withAlpha(theme::core, 0.55f * ha), -1);

    // Scroll cue: a small caret at an edge while there's more map that way.
    if (scroll_ < scrollMax(app) - 4.f)
        draw::polygon(w, {s.x * 0.5f, kMapTop + 16.f}, 6.f, 3, -kPi / 2.f, withAlpha(theme::textDim, 0.8f),
                      withAlpha(theme::textDim, 0.8f));
    if (scroll_ > 4.f)
        draw::polygon(w, {s.x * 0.5f, s.y - 16.f}, 6.f, 3, kPi / 2.f, withAlpha(theme::textDim, 0.8f),
                      withAlpha(theme::textDim, 0.8f));

    drawTabHint(app, w, {theme::margin, s.y - theme::margin - keyCapSize(app.font(), "tab").y});

    // The run's creeds, top-right under the header (hover a chip for its rule).
    bool creedHover = false;
    if (!r.creeds.empty() || !r.pacts.empty())
        creedHover = drawCreedStrip(app, w, {s.x - theme::margin - creedStripWidth(app), kMapTop + 24.f}, false, mouse_,
                                  info_ < 0 && !peek_.open);

    if (peek_.open) {   // the loadout peek covers the map; its own hover help only
        drawLoadoutOverlay(app, w, false, peek_);
        return;
    }

    if (info_ >= 0) {
        const MapNode& n = nodes[static_cast<std::size_t>(info_)];
        std::string d = mapNodeDesc(n.type);
        if (n.type == MapNodeType::Rest && app.ironCoreAlive()) d += " (ends this act's Iron core)";
        if (!app.mapNodeOpen(info_) && info_ != r.mapNode)
            d += n.row <= r.mapRow ? "  (behind you)" : "  (not reachable from here yet)";
        drawTooltip(w, app.font(), mouse_, s, mapNodeName(n.type), d, nodeColor(n.type));
    } else if (!creedHover && app.ironCoreAlive() && std::fabs(mouse_.y - kStatusY) < 10.f &&
               mouse_.x > ironX - 4.f && mouse_.x < ironX + 90.f) {
        drawTooltip(w, app.font(), mouse_, s, "Iron core",
                    "no repairs yet this act. Beat the boss without resting, buying a repair or skipping a pick "
                    "to repair, and the run banks +" + std::to_string(cfg::meta::ironCoreCores) +
                        " cores. The heal before each fight doesn't count.", theme::core);
    } else if (!creedHover && std::fabs(mouse_.y - kStatusY) < 12.f && std::fabs(mouse_.x - s.x * 0.5f) < 220.f) {
        drawTooltip(w, app.font(), mouse_, s, "Gold, core and luck",
                    "gold buys things in shops; the core must survive - rests and shops repair it. Luck (" +
                        std::to_string(app.luck()) + "): each point makes every chance " +
                        std::to_string(static_cast<int>(cfg::luck::chancePerPoint * 100.f + 0.5f)) +
                        "% likelier and cards a little rarer. From Lucky clover, Lucky star, Loaded Dice, Lucky charm (Jester).");
    }
}

// ================================================================ Shop

namespace {
constexpr float kOfferH = 236.f;
constexpr float kOfferGap = 14.f;
constexpr float kGroupGap = 30.f;    // extra room between the shelf's groups (items | abilities | relics ...)
constexpr float kOfferTop = 0.27f;   // * size.y
constexpr float kBtnW = 204.f, kBtnH = 36.f, kBtnGap = 12.f;
}  // namespace

int ShopScreen::cardCount(App& app) const {
    const RunState& r = app.data().run;
    return static_cast<int>(r.shopOffers.size());
}

namespace {
// The shelf is sorted into groups by what a pick is, rarest first inside a
// group; the mystery box closes the row in a group of its own.
int shelfGroup(UpgradeCat c) {
    switch (c) {
        case UpgradeCat::Item:     return 0;
        case UpgradeCat::Ability:  return 1;
        case UpgradeCat::Element:  return 2;
        case UpgradeCat::Relic:    return 3;
        case UpgradeCat::Modifier: return 4;
        case UpgradeCat::NewBall:  return 5;
    }
    return 6;
}
constexpr int kMysteryGroup = 7;

const char* shelfName(int g) {
    switch (g) {
        case 0: return "items";
        case 1: return "abilities";
        case 2: return "elements";
        case 3: return "relics";
        case 4: return "modifiers";
        case 5: return "new ball";
    }
    return "";
}
sf::Color shelfColor(int g) {
    switch (g) {
        case 0: return catColor(UpgradeCat::Item);
        case 1: return catColor(UpgradeCat::Ability);
        case 2: return catColor(UpgradeCat::Element);
        case 3: return catColor(UpgradeCat::Relic);
        case 4: return catColor(UpgradeCat::Modifier);
        case 5: return catColor(UpgradeCat::NewBall);
    }
    return tierColor(Tier::Epic);
}

// Card i of the shop (offers first, then the mystery box) -> its group, and
// the cards in shelf order.
struct Shelf {
    std::vector<int> order;    // card indices left to right
    std::vector<int> group;    // per card index
};
Shelf shelf(App& app, int cards) {
    const RunState& r = app.data().run;
    Shelf sh;
    const int offers = static_cast<int>(r.shopOffers.size());
    for (int i = 0; i < cards; ++i) {
        sh.order.push_back(i);
        sh.group.push_back(i < offers ? shelfGroup(upgradeCat(static_cast<UpgradeKind>(r.shopOffers[static_cast<std::size_t>(i)])))
                                      : kMysteryGroup);
    }
    auto tier = [&](int i) {
        return i < offers ? static_cast<int>(upgradeTier(static_cast<UpgradeKind>(r.shopOffers[static_cast<std::size_t>(i)]))) : 0;
    };
    std::stable_sort(sh.order.begin(), sh.order.end(), [&](int x, int y) {
        const int gx = sh.group[static_cast<std::size_t>(x)], gy = sh.group[static_cast<std::size_t>(y)];
        return gx != gy ? gx < gy : tier(x) > tier(y);
    });
    return sh;
}
}  // namespace

sf::FloatRect ShopScreen::offerRect(App& app, int i) const {
    const sf::Vector2f s = app.size();
    const int n = std::max(1, cardCount(app));
    const Shelf sh = shelf(app, n);
    int groups = 0;   // gaps between groups
    for (int k = 1; k < n; ++k)
        if (sh.group[static_cast<std::size_t>(sh.order[static_cast<std::size_t>(k)])] !=
            sh.group[static_cast<std::size_t>(sh.order[static_cast<std::size_t>(k - 1)])]) ++groups;
    const float gaps = static_cast<float>(n - 1) * kOfferGap + static_cast<float>(groups) * kGroupGap;
    const float wd = std::min(236.f, (s.x - 2.f * (theme::margin + 14.f) - gaps) / static_cast<float>(n));
    const float total = static_cast<float>(n) * wd + gaps;
    float x = s.x * 0.5f - total * 0.5f;
    for (int k = 0; k < n; ++k) {
        const int card = sh.order[static_cast<std::size_t>(k)];
        if (k > 0 && sh.group[static_cast<std::size_t>(card)] !=
                         sh.group[static_cast<std::size_t>(sh.order[static_cast<std::size_t>(k - 1)])])
            x += kGroupGap;
        if (card == i) return {x, s.y * kOfferTop, wd, kOfferH};
        x += wd + kOfferGap;
    }
    return {x, s.y * kOfferTop, wd, kOfferH};
}

namespace {
// The shop's buttons that apply right now: Sell while this visit allows one
// (and there's an item to sell), Reroll and Leave always.
std::vector<int> shopButtons(App& app) {
    bool items = false;
    for (const BallLoadout& L : app.data().run.balls)
        for (int sl = 0; sl < kBallSlots; ++sl) items = items || L.gear[sl] >= 0;
    std::vector<int> v;
    if (app.shopSellsLeft() > 0 && items) v.push_back(0);
    v.push_back(1);
    v.push_back(2);
    return v;
}
}  // namespace

sf::FloatRect ShopScreen::buttonRect(App& app, int b) const {
    const sf::Vector2f s = app.size();
    const std::vector<int> v = shopButtons(app);
    const auto it = std::find(v.begin(), v.end(), b);
    if (it == v.end()) return {};
    const float n = static_cast<float>(v.size());
    const float total = n * kBtnW + (n - 1.f) * kBtnGap;
    const float i = static_cast<float>(it - v.begin());
    return {s.x * 0.5f - total * 0.5f + i * (kBtnW + kBtnGap), s.y * kOfferTop + kOfferH + 48.f, kBtnW, kBtnH};
}

void ShopScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape)) { app.leaveShop(); return; }
    if (isKey(e, sf::Keyboard::R)) { app.rerollShop(); return; }
    if (!isLeftClick(e)) return;
    for (int i = 0; i < cardCount(app); ++i) {
        if (!offerRect(app, i).contains(mouse)) continue;
        app.buyShopOffer(i);
        return;
    }
    for (int b = 0; b < 3; ++b) {
        if (!buttonRect(app, b).contains(mouse)) continue;
        switch (b) {
            case 0: app.beginSell(); break;
            case 1: app.rerollShop(); break;
            case 2: app.leaveShop(); break;
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
    for (int b = 0; b < 3; ++b)
        if (buttonRect(app, b).contains(mouse)) hover_ = 100 + b;
    uisound::hover(this, hover_);
}

void ShopScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const RunState& r = app.data().run;
    const sf::Font& f = app.font();

    drawCenteredPop(w, f, "Shop", theme::fsTitle, {s.x * 0.5f, s.y * 0.09f}, theme::puGolden, introPop(it, 0.f, 0.3f));
    drawRunStatus(app, w, s.y * 0.09f + 40.f);
    drawCreedStrip(app, w, {s.x * 0.5f, s.y * 0.09f + 60.f}, true, mouse_, hover_ < 0);

    const int offers = static_cast<int>(r.shopOffers.size());
    {   // a coloured header over each group of the shelf
        const int n = cardCount(app);
        const Shelf sh = shelf(app, n);
        const float ha = clampf(introPop(it, 0.06f, 0.3f), 0.f, 1.f);
        for (int k = 0; k < n;) {
            const int g = sh.group[static_cast<std::size_t>(sh.order[static_cast<std::size_t>(k)])];
            int e = k;
            while (e + 1 < n && sh.group[static_cast<std::size_t>(sh.order[static_cast<std::size_t>(e + 1)])] == g) ++e;
            if (g != kMysteryGroup) {
                const sf::FloatRect a0 = offerRect(app, sh.order[static_cast<std::size_t>(k)]);
                const sf::FloatRect a1 = offerRect(app, sh.order[static_cast<std::size_t>(e)]);
                const float hy = a0.top - 34.f;
                const UpgradeCat cats[] = {UpgradeCat::Item, UpgradeCat::Ability, UpgradeCat::Element,
                                           UpgradeCat::Relic, UpgradeCat::Modifier, UpgradeCat::NewBall};
                drawKindMark(w, cats[g], {a0.left + 4.f, hy}, 4.5f, withAlpha(shelfColor(g), ha));
                drawLabel(w, f, shelfName(g), 13, {a0.left + 15.f, hy}, withAlpha(shelfColor(g), ha), -1);
                draw::line(w, {a0.left, hy + 12.f}, {a1.left + a1.width, hy + 12.f}, 1.f,
                           withAlpha(shelfColor(g), 0.35f * ha));
            }
            k = e + 1;
        }
    }
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

        (void)info;
        (void)cat;
        PickCardStyle st;
        st.hover = h;
        st.alpha = a;
        st.time = it;
        st.bottomReserve = 34.f;
        st.classMark = !sold;
        st.showWhat = false;   // the group header says it
        drawPickCard(w, f, rc, k, r.balls, st);

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

    // Services: only the ones this visit allows.
    const int g = app.gold();
    if (const sf::FloatRect br = buttonRect(app, 0); br.width > 0.f)
        drawButton(w, f, br, app.shopSellsLeft() > 1 ? "Sell an item (" + std::to_string(app.shopSellsLeft()) + ")"
                                                     : "Sell an item",
                   theme::puGolden, hover_ == 100 ? 1.f : 0.f, true);
    if (const sf::FloatRect br = buttonRect(app, 1); br.width > 0.f)
        drawButton(w, f, br, app.shopRerollPrice() == 0 ? std::string("Reroll (R)  -  free")
                                                        : "Reroll (R)  -  " + std::to_string(app.shopRerollPrice()) + "g",
                   theme::puSurge,
                   hover_ == 101 ? 1.f : 0.f, g >= app.shopRerollPrice());
    drawButton(w, f, buttonRect(app, 2), "Leave", theme::accent, hover_ == 102 ? 1.f : 0.f, true);

    // Hover help.
    if (hover_ >= 0 && hover_ < offers) {
        const auto k = static_cast<UpgradeKind>(r.shopOffers[static_cast<std::size_t>(hover_)]);
        const UpgradeCat cat = upgradeCat(k);
        std::string d = upgradeCatDesc(cat);
        const int deal = hover_ < static_cast<int>(r.shopDeal.size()) ? r.shopDeal[static_cast<std::size_t>(hover_)] : 0;
        if (deal == 1) d += "  On sale: " + std::to_string(app.saleOffPercent()) + "% off, this visit only.";
        drawTooltip(w, f, mouse_, s, upgradeCatName(cat), d, catColor(cat));
    } else if (hover_ == 100) {
        drawTooltip(w, f, mouse_, s, "Sell",
                    "sell an item back for gold (" + std::to_string(static_cast<int>(cfg::gold::sellFrac * 100.f)) +
                        "% of its tier price per level) and free its slot. " + std::to_string(app.shopSellsLeft()) +
                        " left at this shop (Haggler: +1 per level).", theme::puGolden);
    } else if (hover_ == 101) {
        drawTooltip(w, f, mouse_, s, "Reroll",
                    "replace the stock with new picks (and a new sale). As often as you like - each reroll at "
                    "this shop costs much more than the last (Merchant: the first is free, 1 per level).",
                    theme::puSurge);
    }
}

}  // namespace sb
