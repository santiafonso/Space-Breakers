#include "ui/Screens.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

bool isKey(const sf::Event& e, sf::Keyboard::Key k) {
    return e.type == sf::Event::KeyPressed && e.key.code == k;
}
bool isLeftClick(const sf::Event& e) {
    return e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left;
}
bool isDismiss(const sf::Event& e) {
    return isLeftClick(e) || isKey(e, sf::Keyboard::Escape) || isKey(e, sf::Keyboard::Enter) ||
           isKey(e, sf::Keyboard::Space);
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

constexpr float kCardW = 252.f;
constexpr float kCardH = 176.f;
constexpr float kCardGap = 22.f;

sf::Vector2f cardCenter(sf::Vector2f size, int i) {
    const float total = kChoiceCount * kCardW + (kChoiceCount - 1) * kCardGap;
    const float startX = size.x * 0.5f - total * 0.5f;
    return {startX + kCardW * 0.5f + static_cast<float>(i) * (kCardW + kCardGap), size.y * 0.52f};
}

// "12 cores" plus "  ·  3 prisms" once the player has any.
std::string currencyLine(const MetaState& m) {
    std::string s = std::to_string(m.cores) + " cores";
    if (m.prisms > 0)
        s += "   -   " + std::to_string(m.prisms) + (m.prisms == 1 ? " prism" : " prisms");
    return s;
}

// ---- skill-web layout (the Loadout screen) -------------------------------
// Polar layout: a node's (gx,gy) is a direction plus a ring index
// (ring = max(|gx|,|gy|)), so every node sits exactly on a background ring.
// Kept small on purpose - the web will gain many more nodes later.
// The web is drawn small on purpose: it will grow to many more nodes and levels,
// so every element (ring gap, node radius, label offsets, hit radius) is kept
// tight to fit the graph into the middle of the screen.
constexpr float kRingGap = 36.f;      // pixels between concentric rings
constexpr float kNodeR = 7.f;         // branch node radius
constexpr float kRootR = 10.f;        // centre node radius
constexpr float kWebCenterY = 0.5f;   // * size.y
constexpr int   kBackRings = 13;      // faint rings drawn behind the web (room to grow)

const sf::Color kPrismColor = theme::puSurge;   // violet - distinct from the core-blue accent

sf::Color branchColor(MetaBranch b) {
    switch (b) {
        case MetaBranch::Base:    return theme::core;
        case MetaBranch::Combat:  return theme::ballFast;
        case MetaBranch::Eco:     return theme::accent;
        case MetaBranch::Special: return theme::elemFire;
        case MetaBranch::Pickups: return theme::puPoints;
        case MetaBranch::Root:    return theme::textHi;
    }
    return theme::textHi;
}

// Ring index of a node (0 centre, 1, 2, ...) - drives the intro stagger.
float nodeRing(int i) {
    const MetaUnlockDef& d = metaUnlockDef(i);
    return std::max(std::fabs(d.gx), std::fabs(d.gy));
}

const char* branchLabel(MetaBranch b) {
    switch (b) {
        case MetaBranch::Base:    return "Base";
        case MetaBranch::Combat:  return "Ball combat";
        case MetaBranch::Eco:     return "Economy";
        case MetaBranch::Special: return "Special balls";
        case MetaBranch::Pickups: return "Power-ups";
        case MetaBranch::Root:    return "Core";
    }
    return "";
}

void drawLink(sf::RenderWindow& w, sf::Vector2f a, sf::Vector2f b, float thick, sf::Color c) {
    const sf::Vector2f d = b - a;
    sf::RectangleShape bar({length(d), thick});
    bar.setOrigin(0.f, thick * 0.5f);
    bar.setPosition(a);
    bar.setRotation(std::atan2(d.y, d.x) * 180.f / kPi);
    bar.setFillColor(c);
    w.draw(bar);
}

void drawDot(sf::RenderWindow& w, sf::Vector2f p, float r, sf::Color c) {
    sf::CircleShape d(r);
    d.setOrigin(r, r);
    d.setPosition(p);
    d.setFillColor(c);
    w.draw(d);
}

// ---- ball loadout panels (Choice target mode + the Tab overlay) -----------
constexpr float kPanelW = 190.f;
constexpr float kPanelH = 250.f;
constexpr float kPanelGap = 16.f;
constexpr float kSlotH = 24.f;
constexpr float kSlotStep = 29.f;

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

}  // namespace

// ================================================================ Menu

void MenuScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    menu_.init(app.font(), theme::fsItem, s.y * 0.072f);
    menu_.setItems({{"Play", true},
                    {"Stats", true},
                    {"How to Play", true},
                    {resetArm_ > 0.f ? "Reset progress - click again to confirm" : "Reset progress",
                     true},
                    {"Quit", true}});
    menu_.layout({s.x * 0.5f, s.y * 0.44f});
}

void MenuScreen::onEnter(App& app) {
    rebuild(app);
    backdrop_.init(app.size());
}

void MenuScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Enter) || isKey(e, sf::Keyboard::Space)) { app.openLoadout(); return; }
    if (!isLeftClick(e)) return;
    switch (menu_.clickIndex(mouse)) {
        case 0: app.openLoadout(); break;
        case 1: app.openStats(); break;
        case 2: app.openHowTo(); break;
        case 3:
            if (resetArm_ > 0.f) { app.wipeSave(); return; }  // wipeSave rebuilds the menu
            resetArm_ = 4.f;
            rebuild(app);
            break;
        case 4: app.quit(); break;
        default: break;
    }
}

void MenuScreen::update(App& app, float dt, sf::Vector2f mouse) {
    backdrop_.update(dt);
    menu_.update(dt, mouse);
    if (resetArm_ > 0.f) {
        resetArm_ -= dt;
        if (resetArm_ <= 0.f) { resetArm_ = 0.f; rebuild(app); }  // un-arm: restore the label
    }
}

void MenuScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    backdrop_.draw(w, clampf(introPop(it, 0.f, 0.9f), 0.f, 1.f));
    drawCenteredPop(w, app.font(), "Space-Breakers", theme::fsTitle, {s.x * 0.5f, s.y * 0.2f},
                    theme::textHi, introPop(it, 0.f, 0.34f));
    drawCenteredPop(w, app.font(), currencyLine(app.data().meta), theme::fsHeading,
                    {s.x * 0.5f, s.y * 0.2f + 42.f}, theme::accent, introPop(it, 0.09f));
    menu_.draw(w, it);
}

// ================================================================ Loadout

void LoadoutScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    menu_.init(app.font(), theme::fsItem, s.y * 0.050f);
    menu_.setItems({{"Start run", true}, {"Back", true}});
    menu_.layout({s.x * 0.5f, s.y * 0.85f});
}

void LoadoutScreen::onEnter(App& app) {
    rebuild(app);
    selNode_ = 0;
    hoverNode_ = -1;
    selUsed_ = false;
    keyNav_ = false;
    lastMouse_ = {-1.f, -1.f};
    for (int i = 0; i < MetaUnlockCount; ++i) glow_[i] = 0.f;
}

sf::Vector2f LoadoutScreen::nodePos(App& app, int i) const {
    const sf::Vector2f s = app.size();
    const MetaUnlockDef& d = metaUnlockDef(i);
    const sf::Vector2f centre{s.x * 0.5f, s.y * kWebCenterY};
    const float ring = std::max(std::fabs(d.gx), std::fabs(d.gy));
    if (ring < 0.01f) return centre;
    return centre + normalized({d.gx, d.gy}) * (ring * kRingGap);
}

int LoadoutScreen::nodeAt(App& app, sf::Vector2f mouse) const {
    for (int i = 0; i < MetaUnlockCount; ++i) {
        const float r = (i == 0 ? kRootR : kNodeR) + 9.f;   // generous but < half the ring gap
        const sf::Vector2f d = mouse - nodePos(app, i);
        if (d.x * d.x + d.y * d.y <= r * r) return i;
    }
    return -1;
}

// Jump the selection to the nearest node roughly in the (dx,dy) direction.
void LoadoutScreen::moveSelection(App& app, int dx, int dy) {
    const sf::Vector2f cur = nodePos(app, selNode_);
    int best = -1;
    float bestScore = 1e9f;
    for (int i = 0; i < MetaUnlockCount; ++i) {
        if (i == selNode_) continue;
        const sf::Vector2f d = nodePos(app, i) - cur;
        const float along = d.x * static_cast<float>(dx) + d.y * static_cast<float>(dy);
        if (along <= 4.f) continue;
        const float perp = std::fabs(d.x * static_cast<float>(dy) - d.y * static_cast<float>(dx));
        const float score = perp * 2.f + along;
        if (score < bestScore) { bestScore = score; best = i; }
    }
    if (best >= 0) { selNode_ = best; selUsed_ = true; keyNav_ = true; }
}

void LoadoutScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape)) { app.back(); return; }
    if (isKey(e, sf::Keyboard::Enter) || isKey(e, sf::Keyboard::Space)) { app.newRun(); return; }
    if (e.type == sf::Event::KeyPressed) {
        switch (e.key.code) {
            case sf::Keyboard::Left:  moveSelection(app, -1, 0); return;
            case sf::Keyboard::Right: moveSelection(app, 1, 0);  return;
            case sf::Keyboard::Up:    moveSelection(app, 0, -1); return;
            case sf::Keyboard::Down:  moveSelection(app, 0, 1);  return;
            case sf::Keyboard::E:     app.buyMetaUnlock(selNode_); return;
            case sf::Keyboard::C:     if (app.devMode()) { app.devGrantCurrency(); return; } break;
            default: break;
        }
    }
    if (!isLeftClick(e)) return;
    const int n = nodeAt(app, mouse);
    if (n >= 0) { selNode_ = n; app.buyMetaUnlock(n); return; }
    switch (menu_.clickIndex(mouse)) {
        case 0: app.newRun(); break;
        case 1: app.back(); break;
        default: break;
    }
}

void LoadoutScreen::update(App& app, float dt, sf::Vector2f mouse) {
    menu_.update(dt, mouse);
    if (length(mouse - lastMouse_) > 0.5f) { keyNav_ = false; lastMouse_ = mouse; }
    hoverNode_ = nodeAt(app, mouse);
    if (hoverNode_ >= 0) { selNode_ = hoverNode_; selUsed_ = true; }   // hover drives card + E key
    const int active = hoverNode_ >= 0 ? hoverNode_ : (keyNav_ ? selNode_ : -1);
    const float k = 1.f - std::exp(-16.f * dt);
    for (int i = 0; i < MetaUnlockCount; ++i)
        glow_[i] = lerpf(glow_[i], (i == active) ? 1.f : 0.f, k);
}

void LoadoutScreen::drawInfoCard(App& app, sf::RenderWindow& w, int node) const {
    if (node < 0 || node >= MetaUnlockCount) return;
    const sf::Vector2f s = app.size();
    const MetaState& m = app.data().meta;
    const MetaUnlockDef& d = metaUnlockDef(node);
    const int lvl = m.unlock[node];
    const bool maxed = metaUnlockMaxed(node, lvl);
    const bool avail = metaUnlockAvailable(node, m.unlock);
    const bool isPrism = d.currency == MetaCurrency::Prisms;
    const std::uint32_t cost = metaUnlockCost(node, lvl);
    const sf::Color col = branchColor(d.branch);

    const float cw = 300.f, ch = 116.f;
    const sf::Vector2f o(theme::margin, s.y * 0.15f);

    sf::RectangleShape card({cw, ch});
    card.setPosition(o);
    card.setFillColor(withAlpha(theme::panel, 0.85f));
    card.setOutlineThickness(1.f);
    card.setOutlineColor(withAlpha(col, 0.5f));
    w.draw(card);

    sf::RectangleShape stripe({3.f, ch});
    stripe.setPosition(o);
    stripe.setFillColor(col);
    w.draw(stripe);

    sf::Text name = makeText(app.font(), d.name, theme::fsItem, theme::textHi);
    name.setPosition(o.x + 14.f, o.y + 8.f);
    w.draw(name);

    sf::Text br = makeText(app.font(), branchLabel(d.branch), theme::fsSmall, col);
    const sf::FloatRect brb = br.getLocalBounds();
    br.setOrigin(brb.left + brb.width, brb.top);
    br.setPosition(o.x + cw - 14.f, o.y + 14.f);
    w.draw(br);

    char lv[48];
    std::snprintf(lv, sizeof(lv), "Level %d / %d", lvl, d.maxLevel);
    sf::Text lvt = makeText(app.font(), lv, theme::fsSmall, theme::textLo);
    lvt.setPosition(o.x + 14.f, o.y + 38.f);
    w.draw(lvt);

    float y = o.y + 58.f;
    for (const std::string& dl : wrapText(app.font(), d.effect, theme::fsSmall, cw - 28.f)) {
        sf::Text t = makeText(app.font(), dl, theme::fsSmall, theme::textLo);
        t.setPosition(o.x + 14.f, y);
        w.draw(t);
        y += 16.f;
    }

    std::string foot;
    sf::Color footCol = theme::textDim;
    if (maxed) {
        foot = "fully unlocked";
    } else if (!avail) {
        foot = std::string("locked - get ") + metaUnlockDef(d.parent).name + " first";
    } else {
        foot = "cost  " + std::to_string(cost) + (isPrism ? " prisms" : " cores");
        const bool afford = isPrism ? m.prisms >= cost : m.cores >= cost;
        footCol = afford ? (isPrism ? kPrismColor : theme::accent) : theme::coreLow;
    }
    sf::Text ft = makeText(app.font(), foot, theme::fsSmall, footCol);
    ft.setPosition(o.x + 14.f, o.y + ch - 22.f);
    w.draw(ft);
}

void LoadoutScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const MetaState& m = app.data().meta;
    const float it = intro();

    drawCenteredPop(w, app.font(), "Skill web", theme::fsTitle, {s.x * 0.5f, s.y * 0.055f},
                    theme::textHi, introPop(it, 0.f, 0.32f));
    drawCenteredPop(w, app.font(), "click a node to unlock      arrows move, E unlocks",
                    theme::fsSmall, {s.x * 0.5f, s.y * 0.10f}, theme::textDim, introPop(it, 0.06f));

    {
        const float ca = clampf(introPop(it, 0.05f), 0.f, 1.f);
        sf::Text ct = makeText(app.font(), currencyLine(m), theme::fsHeading,
                               withAlpha(theme::accent, ca));
        const sf::FloatRect cb = ct.getLocalBounds();
        ct.setOrigin(cb.left + cb.width, cb.top);
        ct.setPosition(s.x - theme::margin, theme::margin + (1.f - ca) * 8.f);
        w.draw(ct);
    }

    if (app.lastRunWave() > 0) {
        const float la = clampf(introPop(it, 0.12f), 0.f, 1.f);
        char line[96];
        std::snprintf(line, sizeof(line), "Last run: %s on wave %d   +%d cores",
                      app.lastRunWon() ? "won" : "lost", app.lastRunWave(), app.lastRunCores());
        sf::Text lt = makeText(app.font(), line, theme::fsSmall,
                               withAlpha(app.lastRunWon() ? theme::core : theme::textLo, la));
        lt.setPosition(theme::margin, theme::margin + (1.f - la) * 8.f);
        w.draw(lt);
    }

    // Faint concentric rings behind the web - every node sits on one of them.
    const sf::Vector2f centre = nodePos(app, 0);
    const float ringsA = clampf(introPop(it, 0.10f, 0.4f), 0.f, 1.f);
    for (int ring = 1; ring <= kBackRings; ++ring) {
        const float rad = static_cast<float>(ring) * kRingGap;
        sf::CircleShape halo(rad);
        halo.setOrigin(rad, rad);
        halo.setPosition(centre);
        halo.setPointCount(96);
        halo.setFillColor(sf::Color::Transparent);
        halo.setOutlineThickness(1.f);
        halo.setOutlineColor(withAlpha(theme::arenaEdge,
                                       std::max(0.04f, 0.26f - 0.03f * static_cast<float>(ring - 1)) * ringsA));
        w.draw(halo);
    }

    // Links under the nodes - quiet unless both ends (or the parent) are earned.
    for (int i = 0; i < MetaUnlockCount; ++i) {
        const MetaUnlockDef& d = metaUnlockDef(i);
        if (d.parent < 0) continue;
        const float la = clampf(introPop(it, 0.16f + 0.05f * nodeRing(i), 0.3f), 0.f, 1.f);
        if (la <= 0.001f) continue;
        const sf::Vector2f a = nodePos(app, d.parent);
        const sf::Vector2f b = nodePos(app, i);
        const sf::Color col = branchColor(d.branch);
        const bool lit = m.unlock[i] > 0;
        const bool open = !lit && m.unlock[d.parent] > 0;
        drawLink(w, a, b, lit ? 2.5f : 1.5f,
                 lit ? withAlpha(col, 0.5f * la)
                     : withAlpha(open ? col : theme::arenaEdge, 0.22f * la));
    }

    // Nodes. Only the one under the cursor / keyboard selection lights up.
    for (int i = 0; i < MetaUnlockCount; ++i) {
        const MetaUnlockDef& d = metaUnlockDef(i);
        const int lvl = m.unlock[i];
        const bool owned = lvl > 0;
        const bool maxed = metaUnlockMaxed(i, lvl);
        const bool avail = metaUnlockAvailable(i, m.unlock);
        const bool isPrism = d.currency == MetaCurrency::Prisms;
        const std::uint32_t cost = metaUnlockCost(i, lvl);
        const bool afford = avail && !maxed && (isPrism ? m.prisms >= cost : m.cores >= cost);
        const sf::Color col = branchColor(d.branch);
        const sf::Vector2f p = nodePos(app, i);
        const float baseR = (i == 0 ? kRootR : kNodeR);
        const float g = glow_[i];                 // 0 = idle, 1 = lit
        const float pop = introPop(it, 0.12f + 0.06f * nodeRing(i), 0.34f);
        if (pop <= 0.001f) continue;
        const float na = clampf(pop, 0.f, 1.f);                 // intro alpha
        const float r = baseR * clampf(pop, 0.f, 1.12f) * (1.f + 0.45f * g);

        if (g > 0.01f) {
            const float gr = r + 4.f + 12.f * g;
            sf::CircleShape halo(gr);
            halo.setOrigin(gr, gr);
            halo.setPosition(p);
            halo.setFillColor(withAlpha(col, 0.20f * g * na));
            w.draw(halo);
        }

        sf::CircleShape body(r);
        body.setOrigin(r, r);
        body.setPosition(p);
        body.setPointCount(40);
        body.setOutlineThickness(2.f);
        if (owned) {
            body.setFillColor(withAlpha(col, 0.85f * na));
            body.setOutlineColor(withAlpha(col, 0.9f * na));
        } else if (avail) {
            body.setFillColor(withAlpha(col, (0.10f + 0.30f * g) * na));
            body.setOutlineColor(withAlpha(col, ((afford ? 0.42f : 0.24f) + 0.5f * g) * na));
        } else {
            body.setFillColor(withAlpha(theme::arenaEdge, 0.14f * na));
            body.setOutlineColor(withAlpha(theme::arenaEdge, (0.34f + 0.4f * g) * na));
        }
        w.draw(body);

        // a small pip marks an owned node while it is idle
        if (owned && g < 0.6f)
            drawDot(w, p, baseR * 0.36f, withAlpha(col, 0.9f * na));

        // name / level / cost only while lit
        if (g > 0.03f) {
            const float a = clampf(g * 1.5f, 0.f, 1.f);
            drawCentered(w, app.font(), d.name, theme::fsSmall, {p.x, p.y - baseR - 13.f},
                         withAlpha(theme::textHi, a));

            std::string tag;
            sf::Color tagCol = theme::textLo;
            if (maxed) {
                tag = "MAX";
            } else if (owned) {
                tag = "Lv " + std::to_string(lvl) + "  -  " + std::to_string(cost) +
                      (isPrism ? " pr" : "");
            } else if (!avail) {
                tag = "locked";
                tagCol = theme::textDim;
            } else {
                tag = std::to_string(cost) + (isPrism ? " prisms" : " cores");
                tagCol = afford ? (isPrism ? kPrismColor : theme::accent) : theme::textDim;
            }
            drawCentered(w, app.font(), tag, theme::fsSmall, {p.x, p.y + baseR + 13.f},
                         withAlpha(tagCol, a));

            if (d.maxLevel > 1) {
                const float span = static_cast<float>(d.maxLevel - 1) * 7.f;
                for (int k = 0; k < d.maxLevel; ++k) {
                    const bool got = k < lvl;
                    drawDot(w,
                            {p.x - span * 0.5f + static_cast<float>(k) * 7.f, p.y + baseR + 25.f},
                            2.2f, withAlpha(got ? col : theme::textLo, a * (got ? 1.f : 0.45f)));
                }
            }
        }
    }

    if (hoverNode_ >= 0 || selUsed_) drawInfoCard(app, w, selNode_);

    menu_.draw(w, it);
    drawCenteredPop(w, app.font(), "Enter starts the run      Esc goes back", theme::fsSmall,
                    {s.x * 0.5f, s.y * 0.95f}, theme::textDim, introPop(it, 0.28f));
}

// ================================================================ Play

void PlayScreen::onEnter(App&) {
    dragging_ = false;
    showPicks_ = false;
    clock_ = 0.f;
    samples_.clear();
    sceneIn_ = 0.f;        // run just started: fade the arena up from black
    bannerWave_ = 0;       // let the first update fire the "Wave 1" banner
    bannerT_ = 999.f;
}

sf::Vector2f PlayScreen::pointerVelocity() const {
    if (samples_.size() < 2) return {0.f, 0.f};
    const auto& first = samples_.front();
    const auto& last = samples_.back();
    const float span = last.first - first.first;
    if (span < 1e-3f) return {0.f, 0.f};
    return (last.second - first.second) / span;
}

void PlayScreen::grab(App& app, sf::Vector2f mouse) {
    if (app.world().grabAt(mouse, cfg::app::catchRadius)) {
        dragging_ = true;
        samples_.clear();
        samples_.push_back({clock_, mouse});
    }
}

void PlayScreen::release(App& app) {
    if (!dragging_) return;
    const float power = app.data().run.mods.strongArm ? cfg::combat::flingPowerBoost : 1.f;
    const sf::Vector2f v = pointerVelocity() * cfg::app::throwVelScale * power;
    app.world().releaseHeld(v);
    if (app.world().grabbedKind() == Grabbed::None) app.audio().thrown(clampf(length(v) / 900.f, 0.f, 1.f));
    dragging_ = false;
    samples_.clear();
}

void PlayScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape)) { app.openPause(); return; }
    if (isKey(e, sf::Keyboard::M)) { app.toggleSound(); return; }
    if (isKey(e, sf::Keyboard::Q)) { app.useReserve(); return; }   // "Stockpile" reserve power-up
    if (isKey(e, sf::Keyboard::Tab)) { showPicks_ = true; return; }
    if (e.type == sf::Event::KeyReleased && e.key.code == sf::Keyboard::Tab) {
        showPicks_ = false;
        return;
    }
    if (app.devMode() && e.type == sf::Event::KeyPressed) {
        switch (e.key.code) {
            case sf::Keyboard::N:        app.devWinWave(); return;
            case sf::Keyboard::H:        app.devHealCore(); return;
            case sf::Keyboard::G:        app.devToggleInvuln(); return;
            case sf::Keyboard::B:        app.devAddBall(); return;
            case sf::Keyboard::U:        app.devCycleGrant(); return;
            case sf::Keyboard::C:        app.devGrantCores(25); return;
            default: break;
        }
    }
    if (e.type == sf::Event::LostFocus) { release(app); showPicks_ = false; return; }
    if (isLeftClick(e)) { grab(app, mouse); return; }
    if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left)
        release(app);
}

void PlayScreen::update(App& app, float dt, sf::Vector2f mouse) {
    clock_ += dt;
    sceneIn_ += dt;
    bannerT_ += dt;
    if (const int wv = app.data().run.wave; wv > 0 && wv != bannerWave_) {
        bannerWave_ = wv;   // a new wave began (run start, or back from the upgrade cards)
        bannerT_ = 0.f;
    }
    if (dragging_ && !sf::Mouse::isButtonPressed(sf::Mouse::Left)) release(app);
    if (dragging_ && app.world().hasHeld()) {
        samples_.push_back({clock_, mouse});
        while (samples_.size() > 2 &&
               clock_ - samples_.front().first > cfg::app::pointerSampleWindow)
            samples_.pop_front();
        app.world().moveHeld(mouse, dt);
    }
}

void PlayScreen::draw(App& app, sf::RenderWindow& w) {
    app.useWorldView();
    renderer_.draw(w, app.world());
    app.effects().drawRings(w);
    app.useUiView();

    app.hud().draw(w);

    // Ball tally, bottom-left: how many are in play and of what element.
    const std::vector<Ball>& balls = app.world().balls();
    const sf::Vector2f s = app.size();
    const std::string n = std::to_string(balls.size());
    sf::Text tally = makeText(app.font(), n + (balls.size() == 1 ? " ball" : " balls"),
                              theme::fsBody, theme::textLo);
    tally.setPosition(theme::margin, s.y - theme::margin - 40.f);
    w.draw(tally);

    float dx = theme::margin + 5.f;
    const float dy = s.y - theme::margin - 12.f;
    for (const Ball& b : balls) {
        sf::CircleShape dot(5.f);
        dot.setOrigin(5.f, 5.f);
        dot.setPosition(dx, dy);
        dot.setFillColor(b.element == Element::Plain ? theme::textLo : elementColor(b.element));
        w.draw(dot);
        dx += 15.f;
    }

    drawCentered(w, app.font(), "hold TAB for your balls", theme::fsSmall,
                 {theme::margin + 60.f, s.y - theme::margin - 56.f}, theme::textDim);

    if (showPicks_) drawPicks(app, w);

    drawWaveBanner(app, w);

    // Run start: the whole scene fades up from black (holds dark, then clears).
    const float x = clampf(sceneIn_ / 0.55f, 0.f, 1.f);
    const float black = (1.f - x) * (1.f - x);   // ease-out: lingers, then lifts fast
    if (black > 0.001f) drawDim(w, s, black);
}

void PlayScreen::drawWaveBanner(App& app, sf::RenderWindow& w) const {
    if (bannerT_ > 1.6f) return;
    const sf::Vector2f s = app.size();

    const float in = clampf(introPop(bannerT_, 0.f, 0.34f), 0.f, 1.15f);   // springs in
    const float out = 1.f - clampf((bannerT_ - 1.05f) / 0.5f, 0.f, 1.f);   // then fades away
    const float a = clampf(in, 0.f, 1.f) * out;
    if (a <= 0.01f) return;

    std::string label = "Wave " + std::to_string(bannerWave_);
    if (bannerWave_ == cfg::run::bossWave) label = "Miniboss";
    else if (bannerWave_ == cfg::run::finalWave) label = "Final boss";

    const sf::Vector2f c{s.x * 0.5f, s.y * 0.40f - (1.f - out) * 16.f};

    sf::Text t = makeText(app.font(), label, theme::fsTitle + 8u, withAlpha(theme::textHi, a));
    centerOrigin(t);
    const float sc = 0.6f + 0.4f * in;
    t.setScale(sc, sc);
    t.setPosition(std::round(c.x), std::round(c.y));
    w.draw(t);

    // accent underline that wipes open from the centre
    const float uw = t.getGlobalBounds().width;
    sf::RectangleShape bar({uw, 2.f});
    bar.setOrigin(uw * 0.5f, 1.f);
    bar.setPosition(c.x, c.y + static_cast<float>(theme::fsTitle) * 0.55f + 10.f);
    bar.setFillColor(withAlpha(theme::accent, a));
    w.draw(bar);
}

void PlayScreen::drawPicks(App& app, sf::RenderWindow& w) const {
    const sf::Vector2f s = app.size();
    const RunState& r = app.data().run;
    drawDim(w, s, 0.7f);
    drawCentered(w, app.font(), "Your balls", theme::fsHeading, {s.x * 0.5f, s.y * 0.26f}, theme::textHi);

    const int n = static_cast<int>(r.balls.size());
    for (int i = 0; i < n; ++i)
        drawLoadoutPanel(w, app.font(), panelCenter(s, i, n, s.y * 0.48f), r.balls[i], 1.f, 0.f, -1, false);

    std::string relics;
    const std::pair<bool, UpgradeKind> list[] = {
        {r.mods.spring, UpgradeKind::CoreSpring},   {r.mods.slowField, UpgradeKind::CoreSlowField},
        {r.mods.strongArm, UpgradeKind::StrongArm}, {r.mods.contagion, UpgradeKind::Contagion},
        {r.mods.primed, UpgradeKind::Primed}};
    for (const auto& [on, k] : list) {
        if (!on) continue;
        if (!relics.empty()) relics += "   -   ";
        relics += upgradeInfo(k).title;
    }
    drawCentered(w, app.font(), relics.empty() ? "no relics yet" : "Relics:  " + relics, theme::fsSmall,
                 {s.x * 0.5f, s.y * 0.48f + kPanelH * 0.5f + 30.f},
                 relics.empty() ? theme::textDim : theme::puGolden);
}

// ================================================================ Choice

int ChoiceScreen::cardAt(App& app, sf::Vector2f mouse) const {
    const sf::Vector2f s = app.size();
    for (int i = 0; i < kChoiceCount; ++i) {
        const sf::Vector2f c = cardCenter(s, i);
        if (std::fabs(mouse.x - c.x) < kCardW * 0.5f && std::fabs(mouse.y - c.y) < kCardH * 0.5f)
            return i;
    }
    return -1;
}

sf::FloatRect ChoiceScreen::healRect(sf::Vector2f s) const {
    const float wd = 360.f, ht = 34.f;
    const float cy = s.y * 0.52f + kCardH * 0.5f + 30.f;
    return {s.x * 0.5f - wd * 0.5f, cy - ht * 0.5f, wd, ht};
}

// A "reroll" strip along the bottom edge of card i (inside it, above the heal button).
sf::FloatRect ChoiceScreen::rerollRect(sf::Vector2f s, int i) const {
    const sf::Vector2f c = cardCenter(s, i);
    const float wd = kCardW - 28.f, ht = 22.f;
    const float cy = c.y + kCardH * 0.5f - 15.f;
    return {c.x - wd * 0.5f, cy - ht * 0.5f, wd, ht};
}

bool ChoiceScreen::coreHurt(App& app) const {
    const Core& c = app.world().core();
    return c.hp < c.maxHp - 0.5f;
}

void ChoiceScreen::pickCard(App& app, int idx) {
    if (!app.choiceNeedsTarget(idx)) {
        app.applyUpgrade(idx);
        return;
    }
    target_ = idx;
    targetT_ = 0.f;
    hoverBall_ = hoverSlot_ = -1;
}

void ChoiceScreen::targetAt(App& app, sf::Vector2f mouse, int& ball, int& slot) const {
    ball = slot = -1;
    const sf::Vector2f s = app.size();
    const int n = app.runBallCount();
    for (int i = 0; i < n; ++i) {
        const sf::Vector2f c = panelCenter(s, i, n, s.y * 0.52f);
        if (std::fabs(mouse.x - c.x) > kPanelW * 0.5f || std::fabs(mouse.y - c.y) > kPanelH * 0.5f) continue;
        if (!app.choiceFitsBall(target_, i)) return;
        ball = i;
        // Items land in the slot under the pointer, else their default slot. An
        // element always takes over the ball's current element slot. Roles and
        // modifiers just pick the ball.
        const UpgradeKind k = app.choices()[target_];
        if (!upgradeTakesSlot(k)) return;
        const BallLoadout& L = app.data().run.balls[i];
        slot = defaultSlot(k, L);
        if (elementItemSlot(k) < 0 || L.elementSlot() < 0)
            for (int sl = 0; sl < kBallSlots; ++sl)
                if (slotRect(c, sl).contains(mouse)) slot = sl;
        return;
    }
}

void ChoiceScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (target_ >= 0) {
        const bool rightClick = e.type == sf::Event::MouseButtonPressed &&
                                e.mouseButton.button == sf::Mouse::Right;
        if (isKey(e, sf::Keyboard::Escape) || rightClick) { target_ = -1; return; }
        if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
            e.key.code < sf::Keyboard::Num1 + app.runBallCount()) {
            const int b = e.key.code - sf::Keyboard::Num1;
            if (!app.choiceFitsBall(target_, b)) return;
            app.applyUpgradeTo(target_, b, -1);   // -1: default slot
            return;
        }
        if (!isLeftClick(e)) return;
        int b, sl;
        targetAt(app, mouse, b, sl);
        if (b >= 0) app.applyUpgradeTo(target_, b, sl);
        return;
    }
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code < sf::Keyboard::Num1 + kChoiceCount) {
        pickCard(app, e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (!isLeftClick(e)) return;
    if (app.rerollsLeft() > 0) {
        for (int i = 0; i < kChoiceCount; ++i)
            if (rerollRect(app.size(), i).contains(mouse)) { app.rerollChoice(i); return; }
    }
    if (coreHurt(app) && healRect(app.size()).contains(mouse)) {
        app.repairCoreSkipItem();
        return;
    }
    const int c = cardAt(app, mouse);
    if (c >= 0) pickCard(app, c);
}

void ChoiceScreen::update(App& app, float dt, sf::Vector2f mouse) {
    const float k = 1.f - std::exp(-16.f * dt);
    if (target_ >= 0) {
        targetT_ += dt;
        targetAt(app, mouse, hoverBall_, hoverSlot_);
        return;
    }
    const int c = cardAt(app, mouse);
    const bool canReroll = app.rerollsLeft() > 0;
    for (int i = 0; i < kChoiceCount; ++i) {
        hover_[i] = lerpf(hover_[i], c == i ? 1.f : 0.f, k);
        const bool onR = canReroll && rerollRect(app.size(), i).contains(mouse);
        rerollHover_[i] = lerpf(rerollHover_[i], onR ? 1.f : 0.f, k);
    }
    const bool onHeal = coreHurt(app) && healRect(app.size()).contains(mouse);
    healHover_ = lerpf(healHover_, onHeal ? 1.f : 0.f, k);
}

void ChoiceScreen::drawTargets(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const UpgradeKind k = app.choices()[target_];
    const UpgradeInfo info = upgradeInfo(k);

    drawDim(w, s, 0.82f);
    const UpgradeCat cat = upgradeCat(k);
    std::string title;
    switch (cat) {
        case UpgradeCat::Role:     title = std::string("Make a ball a ") + info.title; break;
        case UpgradeCat::Modifier: title = std::string(info.title) + " - pick a ball"; break;
        default:                   title = std::string("Equip ") + info.title + " - pick a ball and slot"; break;
    }
    drawCenteredPop(w, app.font(), title, theme::fsHeading, {s.x * 0.5f, s.y * 0.24f}, theme::textHi,
                    introPop(targetT_, 0.f, 0.25f));
    drawCentered(w, app.font(), info.desc, theme::fsSmall, {s.x * 0.5f, s.y * 0.24f + 30.f}, theme::textLo);

    const auto& balls = app.data().run.balls;
    const int n = static_cast<int>(balls.size());
    for (int i = 0; i < n; ++i) {
        const float cp = clampf(introPop(targetT_, 0.04f * static_cast<float>(i), 0.3f), 0.f, 1.f);
        const sf::Vector2f c = panelCenter(s, i, n, s.y * 0.52f) + sf::Vector2f(0.f, (1.f - cp) * 30.f);
        const bool fits = app.choiceFitsBall(target_, i);
        const bool hot = hoverBall_ == i;
        drawLoadoutPanel(w, app.font(), c, balls[i], cp, hot ? 1.f : 0.f, hot ? hoverSlot_ : -1, !fits);
        drawCentered(w, app.font(), std::to_string(i + 1), theme::fsSmall,
                     {c.x, c.y + kPanelH * 0.5f + 14.f}, withAlpha(theme::textDim, cp));
    }
    drawCentered(w, app.font(),
                 elementItemSlot(k) >= 0 ? "click a ball (one element per ball: it replaces the current one)   -   Esc: back"
                 : upgradeTakesSlot(k)   ? "click a slot (a full one gets replaced)   -   Esc / right-click: back"
                                         : "click a ball   -   Esc / right-click: back",
                 theme::fsSmall, {s.x * 0.5f, s.y * 0.52f + kPanelH * 0.5f + 46.f}, theme::textDim);
}

void ChoiceScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    if (target_ >= 0) {
        drawTargets(app, w);
        return;
    }

    drawDim(w, s, 0.82f * clampf(introPop(it, 0.f, 0.2f), 0.f, 1.f));
    drawCenteredPop(w, app.font(), "Wave cleared - choose one", theme::fsTitle,
                    {s.x * 0.5f, s.y * 0.26f}, theme::textHi, introPop(it, 0.04f, 0.3f));

    for (int i = 0; i < kChoiceCount; ++i) {
        const UpgradeInfo info = upgradeInfo(app.choices()[i]);
        const sf::Vector2f c0 = cardCenter(s, i);
        const float h = hover_[i];
        const float cp = introPop(it, 0.10f + 0.09f * static_cast<float>(i), 0.40f);
        if (cp <= 0.001f) continue;
        const float ca = clampf(cp, 0.f, 1.f);
        const sf::Vector2f c = c0 + sf::Vector2f(0.f, (1.f - ca) * 46.f);   // rises up into place

        sf::RectangleShape card({kCardW, kCardH});
        card.setOrigin(kCardW * 0.5f, kCardH * 0.5f);
        card.setPosition(c);
        const float sc = 0.55f + 0.45f * cp;                                // springs open
        card.setScale(sc, sc);
        card.setFillColor(withAlpha(theme::accent, (0.10f + 0.16f * h) * ca));
        card.setOutlineThickness(2.f);
        card.setOutlineColor(withAlpha(theme::accent, (0.4f + 0.5f * h) * ca));
        w.draw(card);

        const UpgradeKind kind = app.choices()[i];
        const UpgradeCat cat = upgradeCat(kind);
        drawCenteredPop(w, app.font(), std::to_string(i + 1) + "   " + upgradeCatName(cat), theme::fsSmall,
                        {c.x, c.y - kCardH * 0.5f + 16.f}, catColor(cat), cp);
        const int es = elementItemSlot(kind);
        drawCenteredPop(w, app.font(), info.title, theme::fsItem,
                        {c.x, c.y - kCardH * 0.5f + 52.f},
                        es >= 0 ? elementColor(static_cast<Element>(es + 1)) : theme::textHi, cp);

        const std::vector<std::string> desc =
            wrapText(app.font(), info.desc, theme::fsSmall, kCardW - 28.f);
        const float lineH = 18.f;
        float dy = c.y + 22.f - lineH * 0.5f * static_cast<float>(desc.size() - 1);
        for (const std::string& dl : desc) {
            drawCenteredPop(w, app.font(), dl, theme::fsSmall, {c.x, dy}, theme::textLo, cp);
            dy += lineH;
        }

        if (app.rerollsLeft() > 0) {   // "reroll this card" strip along the card's bottom edge
            const float wd = kCardW - 28.f, ht = 22.f;
            const float rh = rerollHover_[i];
            const float ry = c.y + kCardH * 0.5f - 15.f;
            sf::RectangleShape rb({wd, ht});
            rb.setOrigin(wd * 0.5f, ht * 0.5f);
            rb.setPosition(c.x, ry);
            rb.setFillColor(withAlpha(theme::textLo, (0.06f + 0.12f * rh) * ca));
            rb.setOutlineThickness(1.f);
            rb.setOutlineColor(withAlpha(theme::accent, (0.22f + 0.4f * rh) * ca));
            w.draw(rb);
            drawCenteredPop(w, app.font(), "reroll", theme::fsSmall, {c.x, ry - 1.f},
                            theme::textLo, cp);
        }
    }

    const float hintPop = introPop(it, 0.10f + 0.07f * kChoiceCount);
    std::string hint = "click a card or press 1-4";
    if (app.rerollsLeft() > 0)
        hint += "      rerolls left: " + std::to_string(app.rerollsLeft());

    if (coreHurt(app)) {
        const sf::FloatRect r = healRect(s);
        const float a = clampf(hintPop, 0.f, 1.f);
        sf::RectangleShape btn({r.width, r.height});
        btn.setPosition(r.left, r.top);
        btn.setFillColor(withAlpha(theme::core, (0.10f + 0.16f * healHover_) * a));
        btn.setOutlineThickness(1.5f);
        btn.setOutlineColor(withAlpha(theme::core, (0.4f + 0.45f * healHover_) * a));
        w.draw(btn);
        drawCenteredPop(w, app.font(), "Repair the core instead  -  skip this item", theme::fsSmall,
                        {s.x * 0.5f, r.top + r.height * 0.5f - 1.f}, theme::textHi, hintPop);
        drawCenteredPop(w, app.font(), hint, theme::fsSmall,
                        {s.x * 0.5f, r.top + r.height + 22.f}, theme::textDim, hintPop);
    } else {
        drawCenteredPop(w, app.font(), hint, theme::fsSmall,
                        {s.x * 0.5f, s.y * 0.52f + kCardH * 0.5f + 40.f}, theme::textDim, hintPop);
    }
}

// ================================================================ Pause

void PauseScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    lastSound_ = app.data().meta.soundOn;
    menu_.init(app.font(), theme::fsItem, s.y * 0.062f);
    menu_.setItems({{"Resume", true},
                    {"Stats", true},
                    {"How to Play", true},
                    {lastSound_ ? "Sound: On" : "Sound: Off", true},
                    {"Abandon run", true},
                    {"Quit", true}});
    menu_.layout({s.x * 0.5f, s.y * 0.34f});
}

void PauseScreen::onEnter(App& app) { rebuild(app); }

void PauseScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape)) { app.back(); return; }
    if (!isLeftClick(e)) return;
    switch (menu_.clickIndex(mouse)) {
        case 0: app.back(); break;
        case 1: app.openStats(); break;
        case 2: app.openHowTo(); break;
        case 3: app.toggleSound(); break;
        case 4: app.abandonRun(); break;
        case 5: app.quit(); break;
        default: break;
    }
}

void PauseScreen::update(App& app, float dt, sf::Vector2f mouse) {
    if (app.data().meta.soundOn != lastSound_) rebuild(app);
    menu_.update(dt, mouse);
}

void PauseScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    drawDim(w, s, 0.72f * clampf(introPop(it, 0.f, 0.18f), 0.f, 1.f));
    drawCenteredPop(w, app.font(), "Paused", theme::fsTitle, {s.x * 0.5f, s.y * 0.2f}, theme::textHi,
                    introPop(it, 0.03f, 0.28f));
    menu_.draw(w, it);
}

// ================================================================ Stats

void StatsScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f) {
    if (isDismiss(e)) app.back();
}

void StatsScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    drawStatsPanel(w, app.font(), s, app.data().meta.stats, it);
    drawCenteredPop(w, app.font(), "press ESC or click to go back", theme::fsSmall,
                    {s.x * 0.5f, s.y * 0.86f}, theme::textDim, introPop(it, 0.5f));
}

// ================================================================ HowTo

void HowToScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f) {
    if (isDismiss(e)) app.back();
}

void HowToScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    drawCenteredPop(w, app.font(), "How to Play", theme::fsTitle, {s.x * 0.5f, s.y * 0.16f},
                    theme::textHi, introPop(it, 0.f, 0.3f));

    const std::array<const char*, 7> lines = {{
        "Enemies march on the core at the centre. Keep it alive.",
        "Balls bounce in straight lines. Grab one and fling it into the enemies.",
        "Balls start Normal. A role makes one a Striker (fling it), a Support (marks",
        "enemies) or a Guardian (bounces at the closest threat and shoves it back).",
        "After each wave pick 1 of 4: a ball, a role, an item (4 slots per ball),",
        "a modifier (stacks freely) or a relic - or skip it to repair the core.",
        "Beat the miniboss at wave 10, clear wave 20 to finish the run.",
    }};
    const float y0 = s.y * 0.32f;
    for (std::size_t i = 0; i < lines.size(); ++i)
        drawCenteredPop(w, app.font(), lines[i], theme::fsBody,
                        {s.x * 0.5f, y0 + 38.f * static_cast<float>(i)},
                        i + 1 == lines.size() ? theme::textHi : theme::textLo,
                        introPop(it, 0.08f + 0.05f * static_cast<float>(i)));

    drawCenteredPop(w, app.font(),
                    "ESC  pause      TAB  your balls      F  fullscreen      M  sound",
                    theme::fsSmall, {s.x * 0.5f, s.y * 0.72f}, theme::textLo, introPop(it, 0.36f));
    drawCenteredPop(w, app.font(), "press ESC or click to go back", theme::fsSmall,
                    {s.x * 0.5f, s.y * 0.82f}, theme::textDim, introPop(it, 0.42f));
}

// ================================================================ BossWin

void BossWinScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    menu_.init(app.font(), theme::fsItem, s.y * 0.075f);
    if (app.bossWinCanContinue())
        menu_.setItems({{"Continue", true}, {"Back to menu", true}});
    else
        menu_.setItems({{"Back to menu", true}});
    menu_.layout({s.x * 0.5f, s.y * 0.6f});
}

void BossWinScreen::onEnter(App& app) { rebuild(app); }

void BossWinScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    const bool canContinue = app.bossWinCanContinue();
    if (isKey(e, sf::Keyboard::Enter) || isKey(e, sf::Keyboard::Space)) {
        if (canContinue) app.continuePastBoss();   // Enter takes the primary action
        else app.leaveBossWin();
        return;
    }
    if (isKey(e, sf::Keyboard::Escape)) {
        app.leaveBossWin();
        return;
    }
    if (!isLeftClick(e)) return;
    const int i = menu_.clickIndex(mouse);
    if (i < 0) return;
    if (canContinue && i == 0) app.continuePastBoss();
    else app.leaveBossWin();
}

void BossWinScreen::update(App&, float dt, sf::Vector2f mouse) { menu_.update(dt, mouse); }

void BossWinScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    drawDim(w, s, 0.8f * clampf(introPop(it, 0.f, 0.25f), 0.f, 1.f));

    const bool goingOn = app.bossWinCanContinue();   // run still live: the reward isn't due yet
    const bool runEnd = !goingOn && app.lastRunWave() >= cfg::run::finalWave;
    drawCenteredPop(w, app.font(), runEnd ? "Run complete" : "Miniboss defeated", theme::fsTitle,
                    {s.x * 0.5f, s.y * 0.28f}, theme::core, introPop(it, 0.05f, 0.34f));

    if (goingOn) {
        drawCenteredPop(w, app.font(), "the run goes on - push through to wave 20", theme::fsBody,
                        {s.x * 0.5f, s.y * 0.28f + 52.f}, theme::textDim, introPop(it, 0.16f));
    } else {
        const int pr = app.lastRunPrisms();
        char line[96];
        std::snprintf(line, sizeof(line), "+%d cores      +%d %s", app.lastRunCores(), pr,
                      pr == 1 ? "prism" : "prisms");
        drawCenteredPop(w, app.font(), line, theme::fsHeading, {s.x * 0.5f, s.y * 0.28f + 46.f},
                        theme::accent, introPop(it, 0.16f));
        drawCenteredPop(w, app.font(), "run complete", theme::fsBody, {s.x * 0.5f, s.y * 0.28f + 78.f},
                        theme::textDim, introPop(it, 0.24f));
    }

    menu_.draw(w, it);
}

}  // namespace sb
