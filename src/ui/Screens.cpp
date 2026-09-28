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
#include "render/Draw.hpp"
#include "ui/CreedScreen.hpp"
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
bool isDismiss(const sf::Event& e) {
    return isLeftClick(e) || isKey(e, sf::Keyboard::Escape) || isKey(e, sf::Keyboard::Enter) ||
           isKey(e, sf::Keyboard::Space);
}


constexpr float kCardW = 252.f;
constexpr float kCardH = 214.f;
constexpr float kCardGap = 22.f;

sf::Vector2f cardCenter(sf::Vector2f size, int i, int n) {
    const float total = static_cast<float>(n) * kCardW + static_cast<float>(n - 1) * kCardGap;
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
// Polar layout: a node sits at (ang, ring) - a direction (degrees clockwise
// from up) and a distance in rings. Each class route owns a wedge around the
// centre (Offers.hpp); the web is stretched sideways (kStretchX) into an
// ellipse so the long side routes use the screen's width.
constexpr float kRingGap = 56.f;      // pixels between concentric rings
constexpr float kStretchX = 1.4f;     // horizontal stretch of the whole web
constexpr float kInnerPad = 0.6f;     // ring r sits at (r + this) gaps: room for the 9 routes around the centre
constexpr float kNodeR = 11.f;        // branch node radius
constexpr float kClassR = 14.f;       // a class node: bigger, with an outer ring
constexpr float kRootR = 16.f;        // centre node radius
constexpr float kWebCenterY = 0.53f;  // * size.y
constexpr int   kBackRings = 7;       // faint rings drawn behind the web
constexpr float kZoomMin = 0.45f, kZoomMax = 1.9f;
// The legend lists the routes clockwise from the top, like the web.
constexpr MetaBranch kLegend[] = {MetaBranch::Striker,  MetaBranch::Slinger, MetaBranch::Shooter, MetaBranch::Jester,
                                  MetaBranch::Assassin, MetaBranch::Creeds,   MetaBranch::Summoner,
                                  MetaBranch::Support,  MetaBranch::Mage,    MetaBranch::Guardian};
constexpr int kLegendCount = 10;
constexpr float kLegendRow = 21.f;

const sf::Color kPrismColor = theme::puSurge;   // violet - distinct from the core-blue accent

// A route takes its class's colour; Creeds stay a pale bone (a bargain, no class).
sf::Color branchColor(MetaBranch b) {
    switch (b) {
        case MetaBranch::Root:  return theme::textHi;
        case MetaBranch::Creeds: return sf::Color(206, 192, 170);
        default:                return tagColor(metaBranchTag(b));
    }
}

// Ring index of a node (0 centre, 1, 2, ...) - drives the intro stagger.
float nodeRing(int i) { return metaUnlockDef(i).ring; }

// The node that unlocks a class (drawn bigger, its name always shown).
bool isClassNode(int i) {
    return (i >= MetaClassSupport && i <= MetaClassJester) || i == MetaClassSlinger || i == MetaClassStriker;
}

const char* branchLabel(MetaBranch b) {
    switch (b) {
        case MetaBranch::Root:  return "Core";
        case MetaBranch::Creeds: return "Creeds";
        default:                return itemTagName(metaBranchTag(b));
    }
}

// A frontier node's name, pushed outward from the web's centre so neighbours
// on the same ring don't stack their labels.
void drawOutward(sf::RenderWindow& w, const sf::Font& font, const std::string& str, sf::Vector2f p,
                 sf::Vector2f fromCentre, float r, sf::Color c) {
    sf::Text t = makeText(font, str, theme::fsSmall, c);
    const sf::FloatRect b = t.getLocalBounds();
    const sf::Vector2f d = normalized(fromCentre, {0.f, 1.f});
    const sf::Vector2f half{b.width * 0.5f + 3.f, b.height * 0.5f + 3.f};
    // distance from the node centre to the label centre along d, so the box clears the node
    const float reach = r + 3.f + std::min(std::fabs(d.x) > 1e-3f ? half.x / std::fabs(d.x) : 1e9f,
                                           std::fabs(d.y) > 1e-3f ? half.y / std::fabs(d.y) : 1e9f);
    const sf::Vector2f at = p + d * reach;
    t.setOrigin(b.left + b.width * 0.5f, b.top + b.height * 0.5f);
    t.setPosition(std::round(at.x), std::round(at.y));
    w.draw(t);
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

}  // namespace

// ================================================================ Menu

void MenuScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    menu_.init(app.font(), theme::fsItem, s.y * 0.072f);
    menu_.setItems({{"Play", true},
                    {"Stats", true},
                    {"How to Play", true},
                    {"Options", true},
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
        case 3: app.openSound(); break;
        case 4:
            if (resetArm_ > 0.f) { app.wipeSave(); return; }  // wipeSave rebuilds the menu
            resetArm_ = 4.f;
            rebuild(app);
            break;
        case 5: app.quit(); break;
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
    // The title: heavy, wide-tracked caps over a hairline rule that wipes open
    // from an accent centre mark - the console's nameplate.
    const float tp = introPop(it, 0.f, 0.4f);
    const sf::Vector2f tc{s.x * 0.5f, s.y * 0.19f};
    if (tp > 0.001f) {
        const float a = clampf(tp, 0.f, 1.f);
        sf::Text t = makeText(app.font(), "SPACE-BREAKERS", 64, withAlpha(theme::textHi, a));
        t.setLetterSpacing(1.5f);
        centerOrigin(t);
        const float sc = 0.8f + 0.2f * tp;
        t.setScale(sc, sc);
        t.setPosition(std::round(tc.x), std::round(tc.y + (1.f - a) * 10.f));
        w.draw(t);
        const float rw = t.getGlobalBounds().width * clampf(introPop(it, 0.12f, 0.5f), 0.f, 1.f);
        const float ry = tc.y + 50.f;
        draw::line(w, {tc.x - rw * 0.5f, ry}, {tc.x + rw * 0.5f, ry}, 1.f, withAlpha(theme::arenaEdge, a));
        draw::line(w, {tc.x - 28.f, ry}, {tc.x + 28.f, ry}, 3.f, withAlpha(theme::accent, a));
        draw::line(w, {tc.x - rw * 0.5f, ry - 4.f}, {tc.x - rw * 0.5f, ry + 4.f}, 1.f, withAlpha(theme::arenaEdge, a));
        draw::line(w, {tc.x + rw * 0.5f, ry - 4.f}, {tc.x + rw * 0.5f, ry + 4.f}, 1.f, withAlpha(theme::arenaEdge, a));
    }
    drawLabel(w, app.font(), currencyLine(app.data().meta), 14, {tc.x, tc.y + 76.f},
              withAlpha(theme::accent, clampf(introPop(it, 0.09f), 0.f, 1.f)));
    menu_.draw(w, it);
}

// ================================================================ Loadout

void LoadoutScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    menu_.init(app.font(), theme::fsItem, s.y * 0.050f);
    menu_.setItems({{"Start run", true},
                    {app.data().meta.hardMode ? "Mode: Hard" : "Mode: Normal", true},
                    {"Back", true}});
    menu_.layout({s.x - 150.f, s.y * 0.80f});   // bottom-right, clear of the web
}

void LoadoutScreen::onEnter(App& app) {
    rebuild(app);
    selNode_ = 0;
    hoverNode_ = -1;
    selUsed_ = false;
    keyNav_ = false;
    lastMouse_ = {-1.f, -1.f};
    for (int i = 0; i < MetaUnlockCount; ++i) glow_[i] = 0.f;
    // Start zoomed to fit the whole web between the title and the bottom edge
    // (and clear of the side panels).
    const sf::Vector2f s = app.size();
    const float up = s.y * kWebCenterY - 112.f, down = s.y * (1.f - kWebCenterY) - 30.f, side = s.x * 0.5f - 60.f;
    float fit = 1.f;
    for (int i = 0; i < MetaUnlockCount; ++i) {
        const sf::Vector2f o = nodeOffset(i) * kRingGap;   // at zoom 1
        if (o.y < -1.f) fit = std::min(fit, up / -o.y);
        if (o.y > 1.f) fit = std::min(fit, down / o.y);
        if (std::fabs(o.x) > 1.f) fit = std::min(fit, side / std::fabs(o.x));
    }
    zoom_ = clampf(fit, kZoomMin, 1.f);
    pan_ = {0.f, 0.f};
    panning_ = false;
    legendHover_ = -1;
}

sf::Vector2f LoadoutScreen::webCentre(App& app) const {
    const sf::Vector2f s = app.size();
    return sf::Vector2f{s.x * 0.5f, s.y * kWebCenterY} + pan_;
}

// A node's offset from the centre in rings (stretched sideways).
sf::Vector2f LoadoutScreen::nodeOffset(int i) {
    const MetaUnlockDef& d = metaUnlockDef(i);
    const float a = d.ang * kPi / 180.f;
    const float r = d.ring > 0.01f ? d.ring + kInnerPad : 0.f;
    return {std::sin(a) * r * kStretchX, -std::cos(a) * r};
}

sf::Vector2f LoadoutScreen::nodePos(App& app, int i) const {
    return webCentre(app) + nodeOffset(i) * (kRingGap * zoom_);
}

// Zoom by `factor`, keeping the web point under the pointer where it is.
void LoadoutScreen::zoomAt(App& app, sf::Vector2f mouse, float factor) {
    const float nz = clampf(zoom_ * factor, kZoomMin, kZoomMax);
    const sf::Vector2f c = webCentre(app);
    pan_ += (mouse - c) * (1.f - nz / zoom_);
    zoom_ = nz;
}

int LoadoutScreen::legendAt(App& app, sf::Vector2f mouse) const {
    const sf::Vector2f s = app.size();
    const float top = s.y - theme::margin - static_cast<float>(kLegendCount) * kLegendRow;
    for (int i = 0; i < kLegendCount; ++i)
        if (sf::FloatRect(theme::margin, top + static_cast<float>(i) * kLegendRow - 2.f, 170.f, kLegendRow).contains(mouse))
            return static_cast<int>(kLegend[i]);
    return -1;
}

int LoadoutScreen::nodeAt(App& app, sf::Vector2f mouse) const {
    for (int i = 0; i < MetaUnlockCount; ++i) {
        const float r = ((i == 0 ? kRootR : isClassNode(i) ? kClassR : kNodeR) + 7.f) * clampf(zoom_, 0.8f, 1.3f);   // generous but < half the ring gap
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
    // Pan / zoom: wheel zooms at the pointer, a drag on empty space (or with
    // the right / middle button anywhere) moves the web, +/- and 0 by key.
    if (e.type == sf::Event::MouseWheelScrolled) {
        zoomAt(app, mouse, std::pow(1.12f, e.mouseWheelScroll.delta));
        return;
    }
    if (e.type == sf::Event::MouseButtonReleased) { panning_ = false; return; }
    if (e.type == sf::Event::MouseButtonPressed &&
        (e.mouseButton.button == sf::Mouse::Right || e.mouseButton.button == sf::Mouse::Middle)) {
        panning_ = true;
        panStart_ = mouse;
        panFrom_ = pan_;
        return;
    }
    if (e.type == sf::Event::KeyPressed) {
        switch (e.key.code) {
            case sf::Keyboard::Add: case sf::Keyboard::Equal:     zoomAt(app, webCentre(app), 1.15f); return;
            case sf::Keyboard::Subtract: case sf::Keyboard::Hyphen: zoomAt(app, webCentre(app), 1.f / 1.15f); return;
            case sf::Keyboard::Num0: case sf::Keyboard::Home:     onEnter(app); return;
            default: break;
        }
    }
    if (isKey(e, sf::Keyboard::Enter) || isKey(e, sf::Keyboard::Space)) { app.newRun(); return; }
    if (e.type == sf::Event::KeyPressed) {
        switch (e.key.code) {
            case sf::Keyboard::Left:  moveSelection(app, -1, 0); return;
            case sf::Keyboard::Right: moveSelection(app, 1, 0);  return;
            case sf::Keyboard::Up:    moveSelection(app, 0, -1); return;
            case sf::Keyboard::Down:  moveSelection(app, 0, 1);  return;
            case sf::Keyboard::E:     app.buyMetaUnlock(selNode_); return;
            default: break;
        }
    }
    if (!isLeftClick(e)) return;
    const int n = nodeAt(app, mouse);
    if (n >= 0) { selNode_ = n; app.buyMetaUnlock(n); return; }
    switch (menu_.clickIndex(mouse)) {
        case 0: app.newRun(); return;
        case 1:   // hard mode on / off for the next run (saved)
            app.data().meta.hardMode = !app.data().meta.hardMode;
            app.save();
            rebuild(app);
            return;
        case 2: app.back(); return;
        default: break;
    }
    if (legendAt(app, mouse) >= 0) return;
    panning_ = true;   // a left drag on empty space moves the web
    panStart_ = mouse;
    panFrom_ = pan_;
}

void LoadoutScreen::update(App& app, float dt, sf::Vector2f mouse) {
    menu_.update(dt, mouse);
    if (panning_) {
        const sf::Vector2f s = app.size();
        pan_ = panFrom_ + (mouse - panStart_);
        pan_.x = clampf(pan_.x, -s.x * 0.6f, s.x * 0.6f);   // never lose the web off-screen
        pan_.y = clampf(pan_.y, -s.y * 0.6f, s.y * 0.6f);
    }
    legendHover_ = legendAt(app, mouse);
    if (length(mouse - lastMouse_) > 0.5f) { keyNav_ = false; lastMouse_ = mouse; }
    hoverNode_ = nodeAt(app, mouse);
    if (hoverNode_ >= 0) { selNode_ = hoverNode_; selUsed_ = true; }   // hover drives card + E key
    uisound::hover(this, hoverNode_);
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

    draw::panel(w, {o.x, o.y, cw, ch}, col, 1.f, 0.3f);
    draw::box(w, {o.x, o.y, 3.f, ch}, 0.f, col, col);   // branch colour spine

    sf::Text name = makeText(app.font(), d.name, theme::fsItem, theme::textHi);
    name.setPosition(o.x + 14.f, o.y + 8.f);
    w.draw(name);

    drawLabel(w, app.font(), d.branch == MetaBranch::Root || d.branch == MetaBranch::Creeds
                                 ? std::string(branchLabel(d.branch))
                                 : std::string(branchLabel(d.branch)) + " route",
              10, {o.x + cw - 14.f, o.y + 46.f}, col, 1);   // on the level line: long names need the width

    char lv[48];
    std::snprintf(lv, sizeof(lv), "Level %d / %d", lvl, d.maxLevel);
    drawLabel(w, app.font(), lv, 10, {o.x + 14.f, o.y + 46.f}, theme::textLo, -1);

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
    if (std::fabs(app.uiMouse().x - s.x * 0.5f) < 140.f && app.uiMouse().y < s.y * 0.11f)   // controls: on the title's hover
        drawCentered(w, app.font(), "click a node to unlock   -   drag to move, wheel to zoom, 0 resets   -   arrows move, E unlocks",
                     theme::fsSmall, {s.x * 0.5f, s.y * 0.10f}, theme::textLo);

    {   // wallet, top-right: "CORES 480" and, once you have any, "PRISMS 6" under it
        const float ca = clampf(introPop(it, 0.05f), 0.f, 1.f);
        const float right = s.x - theme::margin, y0 = theme::margin + 10.f + (1.f - ca) * 8.f;
        auto row = [&](const char* cap, std::uint32_t v, sf::Color col, float y) {
            sf::Text t = makeText(app.font(), std::to_string(v), theme::fsHeading, withAlpha(col, ca));
            const sf::FloatRect b = t.getLocalBounds();
            t.setOrigin(b.left + b.width, b.top + b.height * 0.5f);
            t.setPosition(std::round(right), std::round(y));
            w.draw(t);
            drawLabel(w, app.font(), cap, 11, {right - b.width - 10.f, y}, withAlpha(col, 0.65f * ca), 1);
        };
        row("cores", m.cores, theme::accent, y0);
        if (m.prisms > 0) row("prisms", m.prisms, kPrismColor, y0 + 30.f);
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
        const float rad = (static_cast<float>(ring) + kInnerPad) * kRingGap * zoom_;
        sf::CircleShape halo(rad);
        halo.setOrigin(rad, rad);
        halo.setPosition(centre);
        halo.setScale(kStretchX, 1.f);
        halo.setPointCount(96);
        halo.setFillColor(sf::Color::Transparent);
        halo.setOutlineThickness(1.f / kStretchX);
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
        const float focus = (legendHover_ < 0 || static_cast<int>(d.branch) == legendHover_) ? 1.f : 0.25f;
        drawLink(w, a, b, lit ? 2.5f : 1.5f,
                 lit ? withAlpha(col, 0.5f * la * focus)
                     : withAlpha(open ? col : theme::arenaEdge, 0.22f * la * focus));
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
        const bool classNode = isClassNode(i);
        const float baseR = (i == 0 ? kRootR : classNode ? kClassR : kNodeR);
        const float g = glow_[i];                 // 0 = idle, 1 = lit
        const float pop = introPop(it, 0.12f + 0.06f * nodeRing(i), 0.34f);
        if (pop <= 0.001f) continue;
        const bool inFocus = legendHover_ < 0 || static_cast<int>(d.branch) == legendHover_ || i == 0;
        const float na = clampf(pop, 0.f, 1.f) * (inFocus ? 1.f : 0.25f);   // intro alpha, legend focus
        const float r = baseR * clampf(zoom_, 0.75f, 1.3f) * clampf(pop, 0.f, 1.12f) *
                        (1.f + (legendHover_ >= 0 && inFocus && i != 0 ? 0.2f : 0.f) + 0.45f * g);

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
        if (classNode)   // a class: an outer ring in its colour, brighter once unlocked
            draw::ring(w, p, r + 4.f, 1.5f, withAlpha(owned || avail ? col : theme::arenaEdge, (owned ? 0.8f : 0.45f) * na));

        // a small pip marks an owned node while it is idle
        if (owned && g < 0.6f)
            drawDot(w, p, baseR * 0.22f, withAlpha(theme::bg, 0.55f * na));

        // Idle labels: the purchase frontier (open, not bought yet) shows its
        // name, so you can read your options without hovering every node;
        // owned multi-level nodes show their level pips.
        if (g < 0.03f && avail && !owned && i != 0 && inFocus && !classNode)
            drawOutward(w, app.font(), d.name, p, p - centre, baseR,
                        withAlpha(afford ? theme::textLo : theme::textDim, 0.9f * na));
        // a class node always names its class (the goal at the end of the route)
        if (g < 0.03f && classNode)
            drawLabel(w, app.font(), d.name, 10, {p.x, p.y + baseR + 14.f},
                      withAlpha(owned ? col : avail ? lerpColor(col, theme::textLo, 0.4f) : theme::textDim, na), 0);
        if (g < 0.03f && owned && d.maxLevel > 1) {
            const float span = static_cast<float>(d.maxLevel - 1) * 6.f;
            for (int k = 0; k < d.maxLevel; ++k)
                drawDot(w, {p.x - span * 0.5f + static_cast<float>(k) * 6.f, p.y + baseR + 7.f}, 2.f,
                        withAlpha(k < lvl ? col : theme::textDim, na));
        }

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

    // Route legend, bottom-left: one row per class route (and the Creeds).
    // Hovering a row lights that route alone.
    {
        const float la = clampf(introPop(it, 0.2f), 0.f, 1.f);
        float ly = s.y - theme::margin - static_cast<float>(kLegendCount) * kLegendRow;
        drawLabel(w, app.font(), "routes", 10, {theme::margin + 2.f, ly - 12.f}, withAlpha(theme::textDim, la), -1);
        for (MetaBranch b : kLegend) {
            const bool hot = legendHover_ == static_cast<int>(b);
            int owned = 0, total = 0;
            for (int i = 0; i < MetaUnlockCount; ++i)
                if (metaUnlockDef(i).branch == b) { ++total; owned += m.unlock[i] > 0 ? 1 : 0; }
            drawDot(w, {theme::margin + 8.f, ly + 8.f}, hot ? 7.5f : 6.f, withAlpha(branchColor(b), la));
            sf::Text t = makeText(app.font(), std::string(branchLabel(b)) + "   " + std::to_string(owned) + "/" +
                                  std::to_string(total), theme::fsSmall, withAlpha(hot ? theme::textHi : theme::textLo, la));
            t.setPosition(theme::margin + 22.f, ly);
            w.draw(t);
            ly += kLegendRow;
        }
    }

    menu_.draw(w, it);
    if (menu_.hovered() == 1)   // what hard mode means, on hover only
        drawTooltip(w, app.font(), lastMouse_, s, "Hard mode",
                    "enemies tougher, faster and more of them, bosses much tougher, Brutes everywhere, "
                    "enemies hit the core harder and no free repair between fights. Pays x1.75 cores. Click to toggle.",
                    theme::coreLow);
}

// ================================================================ Play

void PlayScreen::onEnter(App&) {
    dragging_ = false;
    aimCommitted_ = false;
    peek_.close();
    clock_ = 0.f;
    sceneIn_ = 0.f;        // run just started: fade the arena up from black
    bannerWave_ = 0;       // let the first update fire the "Stage 1" banner
    bannerT_ = 999.f;
}

void PlayScreen::grab(App& app, sf::Vector2f mouse) {
    if (!app.canGrab()) return;   // "Hunters" / "Clockwork" creeds: hands off
    // The wide arena is framed by a pulled-back camera: scale the reach with it
    // so a grab covers the same on screen as in act 1.
    if (app.world().grabAt(mouse, cfg::app::catchRadius * app.world().arenaScale())) {
        app.audio().grab();
        if (const float c = app.world().heldCatch(); c > 0.f)   // caught in flight: a flash that grows with the reward
            if (const Ball* b = app.world().heldBall())
                app.effects().addBurst(b->pos, b->radius * (1.5f + 3.f * c), theme::textHi);
        dragging_ = true;
        aimCommitted_ = false;   // a click until the pointer moves (see update)
        pressPos_ = mouse;
        if (const Ball* b = app.world().heldBall()) anchor_ = b->pos;
    }
}

void PlayScreen::commitAim(App& app) {
    aimCommitted_ = true;
    app.setAiming(true);
}

void PlayScreen::dropHeld(App& app) {
    if (!dragging_) return;
    app.world().cancelHeld();
    app.setAiming(false);
    dragging_ = false;
    aimCommitted_ = false;
}

// Two ways to throw, no options: click a ball (let go without moving) and it
// flies at the nearest enemy; press and pull to aim it with the slingshot.
void PlayScreen::release(App& app) {
    if (!dragging_) return;
    const float power = app.flingPower();   // Strong arm, Hot Hands / Pinball creeds
    const float k = app.world().arenaScale();
    if (!aimCommitted_ && length(worldMouse_ - pressPos_) > cfg::app::quickThrowSlop * k)
        commitAim(app);   // a fast pull that ended between frames still aims
    dragging_ = false;
    if (!aimCommitted_) {
        // A click: the ball goes at the enemy nearest to it, through the same
        // release as a hand-aimed throw. Nothing to hit: let it carry on.
        const Ball* b = app.world().heldBall();
        const std::optional<sf::Vector2f> target = b ? app.world().nearestTarget(b->pos) : std::nullopt;
        if (b && target && length(*target - b->pos) > 1e-3f) {
            const float speed = lerpf(cfg::app::slingMinSpeed, cfg::app::slingMaxSpeed, cfg::app::quickThrowPower) *
                                app.quickThrowMul();   // "Stillness" pact
            const sf::Vector2f v = normalized(*target - b->pos, {1.f, 0.f}) * speed * k * power;
            app.world().releaseHeld(v);
            app.audio().thrown(clampf(length(v) / 900.f, 0.f, 1.f));
        } else {
            app.world().cancelHeld();
        }
        return;
    }
    app.setAiming(false);
    // Pull back, let go: the ball flies away from the pointer, harder the
    // further you pulled. A tiny pull cancels and the ball carries on.
    // Pull and speed are in arena units, so they scale with the wide arena.
    const sf::Vector2f pull = anchor_ - worldMouse_;
    const float len = length(pull);
    if (len < cfg::app::slingDeadzone * k) {
        app.world().cancelHeld();
        app.audio().letGo();
        return;
    }
    const float pw = clampf(len / (cfg::app::slingMaxPull * k), 0.f, 1.f);
    const sf::Vector2f v = pull / len * lerpf(cfg::app::slingMinSpeed, cfg::app::slingMaxSpeed, pw) * k * power;
    app.world().releaseHeld(v);
    if (app.world().grabbedKind() == Grabbed::None) app.audio().thrown(clampf(length(v) / 900.f, 0.f, 1.f));
}

void PlayScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    // TAB peek: the fight stands still under it, so a held ball is set down
    // (not thrown) and nothing else acts until it closes.
    if (peek_.handle(e)) {
        if (peek_.open) dropHeld(app);
        return;
    }
    if (isKey(e, sf::Keyboard::M)) { app.toggleSound(); return; }
    if (peek_.open) {
        loadoutDragEvent(app, peek_, e);   // drag slots between the balls
        return;
    }
    if (isKey(e, sf::Keyboard::Escape)) { app.openPause(); return; }
    if (isKey(e, sf::Keyboard::Q)) { app.useReserve(); return; }   // "Stockpile" reserve power-up
    if (isKey(e, sf::Keyboard::Space) ||                               // "Nova" creed
        (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Right)) {
        app.useCreedAbility();
        return;
    }
    if (app.devMode() && isKey(e, sf::Keyboard::F1)) { app.devOpenPanel(); return; }   // SB_DEV: everything is in the panel
    if (e.type == sf::Event::LostFocus) {   // don't fire a throw on alt-tab
        dropHeld(app);
        return;
    }
    if (isLeftClick(e)) { grab(app, mouse); return; }
    if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left)
        release(app);
}

void PlayScreen::update(App& app, float dt, sf::Vector2f mouse) {
    worldMouse_ = mouse;
    app.world().setPointer(mouse);   // "Grip" bends balls toward it
    sceneIn_ += dt;
    peek_.update(dt);
    if (peek_.open) return;   // paused: the stage banner and drag sampling wait too
    clock_ += dt;
    bannerT_ += dt;
    // A new stage's fight began: key it on act + map row (several rows can
    // share a difficulty wave).
    if (const int key = app.data().run.map.act * 100 + app.data().run.mapRow;
        app.data().run.mapRow > 0 && key != bannerWave_) {
        bannerWave_ = key;
        bannerT_ = 0.f;
    }
    // The pointer moved off the ball while pressed: aim with the slingshot.
    if (dragging_ && !aimCommitted_ && sf::Mouse::isButtonPressed(sf::Mouse::Left) &&
        length(mouse - pressPos_) > cfg::app::quickThrowSlop * app.world().arenaScale())
        commitAim(app);
    if (dragging_ && !sf::Mouse::isButtonPressed(sf::Mouse::Left)) release(app);
    if (dragging_ && !app.world().heldBall()) {   // "Hot Potato": it slipped out of your hand
        dragging_ = false;
        aimCommitted_ = false;
        app.setAiming(false);
        app.audio().letGo();
    }
}

// Slingshot aim: a band from the ball to the pointer and a dotted line along
// the launch direction up to the first wall, brighter with more power.
void PlayScreen::drawAim(App& app, sf::RenderWindow& w) const {
    const Ball* b = app.world().heldBall();
    if (!b) return;
    const sf::Vector2f pull = anchor_ - worldMouse_;
    const float len = length(pull);
    const float as = app.world().arenaScale();
    const bool live = len >= cfg::app::slingDeadzone * as;
    const float k = clampf(len / (cfg::app::slingMaxPull * as), 0.f, 1.f);

    auto seg = [&](sf::Vector2f a, sf::Vector2f c, float thick, sf::Color col) {
        const sf::Vector2f d = c - a;
        sf::RectangleShape r({length(d), thick});
        r.setOrigin(0.f, thick * 0.5f);
        r.setPosition(a);
        r.setRotation(std::atan2(d.y, d.x) * 180.f / kPi);
        r.setFillColor(col);
        w.draw(r);
    };
    seg(b->pos, worldMouse_, 2.f * as, withAlpha(theme::textLo, live ? 0.5f : 0.25f));   // the band
    if (!live || !app.aimGuide()) return;   // "Blind" pact: the band only

    const sf::Vector2f dir = pull / len;
    const sf::Vector2f sz = app.world().size();
    float t = 1e9f;   // distance to the first wall along dir
    if (dir.x > 1e-4f) t = std::min(t, (sz.x - b->radius - b->pos.x) / dir.x);
    if (dir.x < -1e-4f) t = std::min(t, (b->radius - b->pos.x) / dir.x);
    if (dir.y > 1e-4f) t = std::min(t, (sz.y - b->radius - b->pos.y) / dir.y);
    if (dir.y < -1e-4f) t = std::min(t, (b->radius - b->pos.y) / dir.y);
    t = std::max(0.f, std::min(t, 2000.f));
    const sf::Color col = withAlpha(lerpColor(theme::accent, theme::puGolden, k), 0.35f + 0.5f * k);
    for (float s = b->radius + 6.f * as; s < t; s += 18.f * as)
        seg(b->pos + dir * s, b->pos + dir * std::min(s + 9.f * as, t), 3.f * as, col);
}

void PlayScreen::draw(App& app, sf::RenderWindow& w) {
    app.useWorldView();
    renderer_.draw(w, app.world());
    drawCreedWorld(app, w);   // Hunters tethers, Living Core overcharge
    app.effects().drawRings(w);
    if (dragging_ && aimCommitted_) drawAim(app, w);
    app.useUiView();

    app.hud().draw(w);

    // Ball tally, bottom-left: one chip per ball in its class colour, the
    // [TAB] cap for the loadout above them. Words only on hover.
    const std::vector<Ball>& balls = app.world().balls();
    const sf::Vector2f s = app.size();
    const sf::FloatRect tally = ballTallyRect(app, static_cast<int>(balls.size()));
    float dx = tally.left + 9.f;
    const float dy = tally.top + tally.height - 9.f;
    for (const Ball& b : balls) {
        const sf::Color ec = ballHue(b);   // its class, not its element
        draw::disc(w, {dx, dy}, 7.f, lerpColor(ec, sf::Color::White, 0.2f), ec, {1.f, 1.f}, 20);
        dx += 22.f;
    }
    drawTabHint(app, w, {theme::margin, tally.top});

    drawCreedHud(app, w, app.uiMouse(), !peek_.open && !dragging_);

    if (peek_.open) {
        drawLoadoutOverlay(app, w, true, peek_);
    } else if (!dragging_) {
        // Hover help for the HUD and the ball tally (pointer in UI units).
        const sf::Vector2f um = app.uiMouse();
        std::string tt, td;
        sf::Color tc = theme::textHi;
        const Enemy* hovered = nullptr;
        for (const Enemy& e : app.world().enemies())
            if (length(e.pos - worldMouse_) < e.radius + 6.f) hovered = &e;
        if (app.hud().tooltipAt(um, tt, td, tc)) {
            drawTooltip(w, app.font(), um, s, tt, td, tc);
        } else if (hovered) {
            drawTooltip(w, app.font(), um, s, enemyName(hovered->kind), enemyDesc(hovered->kind), theme::enemy);
        } else if (sf::FloatRect(tally.left - 6.f, tally.top + 26.f, tally.width + 12.f, tally.height - 20.f).contains(um)) {
            drawTooltip(w, app.font(), um, s, "Your balls",
                        "click one to throw it at the nearest enemy, or press and pull back to aim. TAB shows each ball's classes, items, type, abilities and modifiers (the fight pauses).");
        }
    }

    if (!peek_.open) drawWaveBanner(app, w);

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

    const int act = bannerWave_ / 100, row = bannerWave_ % 100;
    std::string label = "Stage " + std::to_string(row);
    const bool elite = app.data().run.eliteWave && row <= mapRows(act);
    if (elite) label = "Elite";
    if (row > mapRows(act)) label = bossName(bossOfAct(act));
    const sf::Color frame = elite ? theme::ember : theme::accent;   // an elite's banner burns orange
    drawLabel(w, app.font(), elite ? "act " + std::to_string(act) + "   -   item spoils" : "act " + std::to_string(act), 12,
              {s.x * 0.5f, s.y * 0.40f - 46.f - (1.f - out) * 16.f}, withAlpha(frame, a));

    const sf::Vector2f c{s.x * 0.5f, s.y * 0.40f - (1.f - out) * 16.f};

    sf::Text t = makeText(app.font(), label, theme::fsTitle + 8u, withAlpha(theme::textHi, a));
    centerOrigin(t);
    const float sc = 0.6f + 0.4f * in;
    t.setScale(sc, sc);
    t.setPosition(std::round(c.x), std::round(c.y));
    w.draw(t);

    // corner brackets that close in on the title as it lands
    const sf::FloatRect tb = t.getGlobalBounds();
    const float pad = 18.f + 30.f * (1.f - clampf(in, 0.f, 1.f));
    draw::brackets(w, {tb.left - pad, tb.top - pad * 0.6f, tb.width + 2.f * pad, tb.height + 1.2f * pad}, 12.f, 2.f,
                   withAlpha(frame, 0.8f * a));
}

// ================================================================ TAB peek

namespace {
constexpr float kTabTapTime = 0.25f;   // a press shorter than this keeps the peek open
}

bool TabPeek::handle(const sf::Event& e) {
    if (e.type == sf::Event::LostFocus) { close(); return false; }
    if (isKey(e, sf::Keyboard::Tab)) {
        if (latched()) { close(); return true; }   // tapped open earlier: this press closes it
        open = down = true;
        held = 0.f;
        closeOnDrop = false;
        return true;
    }
    if (e.type == sf::Event::KeyReleased && e.key.code == sf::Keyboard::Tab) {
        if (!down) return true;
        down = false;
        if (held >= kTabTapTime) {   // held to look: letting go closes it (mid-drag: once it lands)
            if (dragBall >= 0) closeOnDrop = true;
            else close();
        }
        return true;
    }
    if (open && isKey(e, sf::Keyboard::Escape)) { close(); return true; }
    return false;
}

sf::FloatRect ballTallyRect(App& app, int balls) {
    const sf::Vector2f s = app.size();
    return {theme::margin, s.y - theme::margin - 50.f, std::max(60.f, 22.f * static_cast<float>(balls)), 50.f};
}

void drawTabHint(App& app, sf::RenderWindow& w, sf::Vector2f topLeft) {
    const sf::Vector2f k = keyCapSize(app.font(), "tab");
    const sf::FloatRect cap{topLeft.x, topLeft.y, k.x, k.y};
    drawKeyCap(w, app.font(), cap, "tab", "loadout", cap.contains(app.uiMouse()));
}

// Every relic the run has picked: the whole-run passives, listed beside the
// balls in the TAB peek.
std::vector<UpgradeKind> runRelics(const RunMods& m) {
    const std::pair<bool, UpgradeKind> list[] = {
        {m.spring, UpgradeKind::CoreSpring},       {m.slowField, UpgradeKind::CoreSlowField},
        {m.strongArm, UpgradeKind::StrongArm},     {m.contagion, UpgradeKind::Contagion},
        {m.primed, UpgradeKind::Primed},           {m.catalyst, UpgradeKind::Catalyst},
        {m.chainReaction, UpgradeKind::ChainReaction}, {m.luckyClover, UpgradeKind::LuckyClover},
        {m.glassCannon, UpgradeKind::GlassCannon}, {m.magneticCore, UpgradeKind::MagneticCore},
        {m.prismCore, UpgradeKind::PrismCore},     {m.phoenix, UpgradeKind::Phoenix},
        {m.timeDilation, UpgradeKind::TimeDilation}, {m.overcharge, UpgradeKind::Overcharge}};
    std::vector<UpgradeKind> out;
    for (const auto& [on, k] : list)
        if (on) out.push_back(k);
    return out;
}

namespace {
constexpr float kPeekSideW = 210.f;   // the relics / creeds column on the right of the peek

// Where the peek's pieces go, in its zoomed units: the balls' row, and the
// relics / creeds column on the right - only when the run has any.
struct PeekLayout {
    sf::Vector2f s;   // the zoomed canvas
    bool side;
    float sideX, rowCx, rowY;
};

bool peekHasSide(App& app) {
    return !runRelics(app.data().run.mods).empty() || !app.data().run.creeds.empty() || !app.data().run.pacts.empty();
}

PeekLayout peekLayout(App& app) {
    PeekLayout L;
    L.s = app.size() / loadoutZoom(app);
    L.side = peekHasSide(app);
    L.sideX = L.s.x - theme::margin - kPeekSideW;
    L.rowCx = L.side ? (theme::margin + L.sideX - 16.f) * 0.5f : L.s.x * 0.5f;
    L.rowY = L.s.y * 0.54f;
    return L;
}

// The (ball, slot) of the peek under the pointer: slot -1 = on a ball's panel
// but not on one of its slots; ball -1 = on no panel.
void peekSlotAt(App& app, sf::Vector2f m, int& ball, int& slot) {
    ball = slot = -1;
    const std::vector<BallLoadout>& balls = app.data().run.balls;
    for (int i = 0; i < static_cast<int>(balls.size()); ++i) {
        const sf::Vector2f c = loadoutPanelCenter(app, i);
        if (std::fabs(m.x - c.x) > kPanelW * 0.5f || std::fabs(m.y - c.y) > kPanelH * 0.5f) continue;
        ball = i;
        const int part = panelPartAt(c, m, balls[static_cast<std::size_t>(i)]);
        if (part >= 0 && part < kLoadoutSlots) slot = part;
        return;
    }
}

// A pick's text colour on a slot: its class tag, its element, or the ability blue.
sf::Color pickColor(UpgradeKind k) {
    if (upgradeCat(k) == UpgradeCat::Element) return elementColor(static_cast<Element>(elementItemSlot(k) + 1));
    if (upgradeCat(k) == UpgradeCat::Ability) return theme::ability;
    return tagColor(itemTag(k));
}
}  // namespace

float loadoutZoom(App& app) {
    return panelRowZoom(app.size(), app.runBallCount(), peekHasSide(app) ? kPeekSideW + 48.f : 0.f);
}

sf::Vector2f loadoutPanelCenter(App& app, int i) {
    const PeekLayout L = peekLayout(app);
    return panelCenter(L.s, i, app.runBallCount(), L.rowY, L.rowCx);
}

bool loadoutDragEvent(App& app, TabPeek& peek, const sf::Event& e) {
    if (!peek.open) return false;
    const bool left = (e.type == sf::Event::MouseButtonPressed || e.type == sf::Event::MouseButtonReleased) &&
                      e.mouseButton.button == sf::Mouse::Left;
    if (!left) return false;
    int ball, slot;
    peekSlotAt(app, app.uiMouse() / loadoutZoom(app), ball, slot);
    if (e.type == sf::Event::MouseButtonPressed) {   // pick up a filled slot
        peek.dragBall = peek.dragSlot = -1;
        if (ball >= 0 && slot >= 0 && app.data().run.balls[static_cast<std::size_t>(ball)].kindAt(slot) >= 0) {
            peek.dragBall = ball;
            peek.dragSlot = slot;
            app.audio().grab();
        }
        return true;
    }
    if (peek.dragBall < 0) return true;
    // Drop: on a slot, or on a panel (its first free slot of that kind).
    // Anything the move refuses snaps back.
    const bool home = ball == peek.dragBall && (slot == peek.dragSlot || slot < 0);
    if (ball >= 0 && app.moveSlot(peek.dragBall, peek.dragSlot, ball, slot)) app.audio().uiClick();
    else if (!home) app.audio().letGo();
    peek.dragBall = peek.dragSlot = -1;
    if (peek.closeOnDrop) peek.close();
    return true;
}

void drawLoadoutOverlay(App& app, sf::RenderWindow& w, bool paused, const TabPeek& peek) {
    (void)paused;
    drawDim(w, app.size(), 0.7f);
    // Everything below lays out in the zoomed canvas; back to plain UI after.
    const float zoom = loadoutZoom(app);
    app.useUiZoom(zoom);
    struct ViewReset {
        App& app;
        ~ViewReset() { app.useUiView(); }
    } viewReset{app};
    const PeekLayout lay = peekLayout(app);
    const sf::Vector2f s = lay.s;
    const RunState& r = app.data().run;
    const sf::Vector2f um = app.uiMouse() / zoom;

    // The balls in a row; the run-wide passives (relics, creeds) in a column on
    // the right when there are any. How to drag / close is on hover only.
    constexpr float kSideW = kPeekSideW;
    const float sideX = lay.sideX;
    const float rowCx = lay.rowCx;
    const float rowY = lay.rowY;
    drawCentered(w, app.font(), "Your balls", theme::fsHeading, {rowCx, rowY - kPanelH * 0.5f - 44.f},
                 theme::textHi);

    const int n = static_cast<int>(r.balls.size());
    for (int i = 0; i < n; ++i)
        drawLoadoutPanel(w, app.font(), panelCenter(s, i, n, rowY, rowCx), r.balls[static_cast<std::size_t>(i)],
                         1.f, 0.f, -1, false);

    // A slot in hand: its spot on the panel empties, the slots it may land on
    // get a faint edge (the one under the pointer lit), and a chip follows the
    // pointer.
    const bool dragging = peek.dragging() && peek.dragBall < n &&
                          r.balls[static_cast<std::size_t>(peek.dragBall)].kindAt(peek.dragSlot) >= 0;
    int land = -1;   // where the slot in hand would land, -1 = nowhere
    if (dragging) {
        int hb, hs;
        peekSlotAt(app, um, hb, hs);
        land = hb >= 0 ? app.slotMoveTarget(peek.dragBall, peek.dragSlot, hb, hs) : -1;
        for (int i = 0; i < n; ++i) {
            const BallLoadout& L = r.balls[static_cast<std::size_t>(i)];
            const sf::Vector2f c = panelCenter(s, i, n, rowY, rowCx);
            for (int sl = 0; sl < kSlotAbility + abilityBoxes(L); ++sl) {
                const sf::FloatRect sr = slotRect(c, sl, L);
                if (i == peek.dragBall && sl == peek.dragSlot) {
                    draw::box(w, sr, theme::corner, withAlpha(theme::bgDeep, 0.8f), withAlpha(theme::bgDeep, 0.8f),
                              withAlpha(theme::accent, 0.3f), 1.f);
                    continue;
                }
                if (app.slotMoveTarget(peek.dragBall, peek.dragSlot, i, sl) != sl) continue;
                const bool hot = i == hb && sl == land;
                draw::box(w, sr, theme::corner, withAlpha(theme::accent, hot ? 0.16f : 0.f),
                          withAlpha(theme::accent, hot ? 0.06f : 0.f), withAlpha(theme::accent, hot ? 0.85f : 0.42f),
                          1.f);
            }
        }
    }

    // ---- side column: relics, then creeds - each section only when it has any
    const std::vector<UpgradeKind> relics = runRelics(r.mods);
    const UpgradeKind* hotRelic = nullptr;
    bool creedTip = false;
    if (lay.side) {
        const float colTop = rowY - kPanelH * 0.5f;
        draw::panel(w, {sideX, colTop, kSideW, kPanelH}, theme::puGolden, 0.9f);
        float y = colTop + 20.f;
        constexpr float kRelicStep = 22.f;
        if (!relics.empty()) {
            drawLabel(w, app.font(), "relics", 12, {sideX + 14.f, y}, withAlpha(theme::puGolden, 0.8f), -1);
            y += 26.f;
        }
        for (const UpgradeKind& k : relics) {
            const sf::FloatRect row{sideX + 8.f, y - kRelicStep * 0.5f, kSideW - 16.f, kRelicStep};
            const bool hot = !dragging && row.contains(um);
            if (hot) {
                hotRelic = &k;
                draw::box(w, row, 0.f, withAlpha(theme::puGolden, 0.10f), withAlpha(theme::puGolden, 0.03f));
            }
            drawKindMark(w, UpgradeCat::Relic, {sideX + 14.f, y}, 3.5f, tierColor(upgradeTier(k)));   // relic = circle, lit by rarity
            sf::Text t = makeText(app.font(), upgradeInfo(k).title, theme::fsSmall, hot ? theme::textHi : theme::puGolden);
            const sf::FloatRect tb = t.getLocalBounds();
            t.setOrigin(tb.left, tb.top + tb.height * 0.5f);
            t.setPosition(std::round(sideX + 22.f), std::round(y));
            w.draw(t);
            y += kRelicStep;
        }
        if (!r.creeds.empty()) {
            if (!relics.empty()) y += 12.f;
            drawLabel(w, app.font(), "creeds", 12, {sideX + 14.f, y}, withAlpha(sf::Color(226, 70, 84), 0.9f), -1);
            y += 18.f;
            creedTip = drawCreedStrip(app, w, {sideX + 12.f, y}, false, um, !dragging);
        }
    }
    if (dragging) {   // the chip in hand, over everything; no hover help meanwhile
        const BallLoadout& L = r.balls[static_cast<std::size_t>(peek.dragBall)];
        const auto k = static_cast<UpgradeKind>(L.kindAt(peek.dragSlot));
        std::string t = upgradeInfo(k).title;
        if (L.levelAt(peek.dragSlot) > 1) t += "  Lv" + std::to_string(L.levelAt(peek.dragSlot));
        const sf::FloatRect chip{um.x + 10.f, um.y + 8.f, 128.f, kSlotH};   // beside the pointer, off the target
        draw::box(w, chip, theme::corner, withAlpha(theme::bgDeep, 0.92f), withAlpha(theme::bgDeep, 0.85f),
                  withAlpha(land >= 0 ? theme::accent : theme::textDim, 0.7f), 1.f);
        const sf::Color tc = tierColor(upgradeTier(k));
        draw::box(w, {chip.left, chip.top, 3.f, chip.height}, 0.f, tc, tc);
        drawCentered(w, app.font(), t, theme::fsSmall, {chip.left + chip.width * 0.5f, chip.top + chip.height * 0.5f - 1.f},
                     pickColor(k));
        return;
    }
    if (creedTip) return;

    // Hover help on the panels and the relics.
    for (int i = 0; i < n; ++i) {
        const BallLoadout& L = r.balls[static_cast<std::size_t>(i)];
        const int part = panelPartAt(panelCenter(s, i, n, rowY, rowCx), um, L);
        std::string tt, td;
        if (part >= 0 && loadoutTooltip(L, part, tt, td)) {
            drawTooltip(w, app.font(), um, s, tt, td);
            return;
        }
    }
    if (hotRelic)
        drawTooltip(w, app.font(), um, s, std::string(upgradeInfo(*hotRelic).title) + "  -  relic",
                    std::string(upgradeInfo(*hotRelic).desc) + "  [" + tierName(upgradeTier(*hotRelic)) + ", every ball]",
                    theme::puGolden);
}

// ================================================================ Choice

int ChoiceScreen::cardAt(App& app, sf::Vector2f mouse) const {
    const sf::Vector2f s = app.size();
    for (int i = 0; i < app.choiceCount(); ++i) {
        const sf::Vector2f c = cardCenter(s, i, app.choiceCount());
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
sf::FloatRect ChoiceScreen::rerollRect(sf::Vector2f s, int i, int n) const {
    const sf::Vector2f c = cardCenter(s, i, n);
    const float wd = kCardW - 28.f, ht = 22.f;
    const float cy = c.y + kCardH * 0.5f - 15.f;
    return {c.x - wd * 0.5f, cy - ht * 0.5f, wd, ht};
}

bool ChoiceScreen::coreHurt(App& app) const {
    const Core& c = app.world().core();
    return c.hp < c.maxHp - 0.5f;
}

void ChoiceScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code < sf::Keyboard::Num1 + app.choiceCount()) {
        app.applyUpgrade(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (!isLeftClick(e)) return;
    if (app.rerollsLeft() > 0) {
        for (int i = 0; i < app.choiceCount(); ++i)
            if (rerollRect(app.size(), i, app.choiceCount()).contains(mouse)) { app.rerollChoice(i); return; }
    }
    if (coreHurt(app) && healRect(app.size()).contains(mouse)) {
        app.repairCoreSkipItem();
        return;
    }
    const int c = cardAt(app, mouse);
    if (c >= 0) app.applyUpgrade(c);
}

void ChoiceScreen::update(App& app, float dt, sf::Vector2f mouse) {
    mouse_ = mouse;
    const float k = 1.f - std::exp(-16.f * dt);
    const int c = cardAt(app, mouse);
    const bool canReroll = app.rerollsLeft() > 0;
    for (int i = 0; i < app.choiceCount(); ++i) {
        hover_[i] = lerpf(hover_[i], c == i ? 1.f : 0.f, k);
        const bool onR = canReroll && rerollRect(app.size(), i, app.choiceCount()).contains(mouse);
        rerollHover_[i] = lerpf(rerollHover_[i], onR ? 1.f : 0.f, k);
    }
    const bool onHeal = coreHurt(app) && healRect(app.size()).contains(mouse);
    healHover_ = lerpf(healHover_, onHeal ? 1.f : 0.f, k);
    int hot = c;   // card, its reroll strip (10 + i) or the repair button (20)
    for (int i = 0; i < app.choiceCount() && hot < 0; ++i)
        if (canReroll && rerollRect(app.size(), i, app.choiceCount()).contains(mouse)) hot = 10 + i;
    if (hot < 0 && onHeal) hot = 20;
    uisound::hover(this, hot);
}

void ChoiceScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();

    drawDim(w, s, 0.82f * clampf(introPop(it, 0.f, 0.2f), 0.f, 1.f));
    drawCenteredPop(w, app.font(), app.choiceTitle(),
                    theme::fsTitle,
                    {s.x * 0.5f, s.y * 0.26f}, theme::textHi, introPop(it, 0.04f, 0.3f));

    for (int i = 0; i < app.choiceCount(); ++i) {
        const UpgradeInfo info = upgradeInfo(app.choices()[i]);
        const sf::Vector2f c0 = cardCenter(s, i, app.choiceCount());
        const float h = hover_[i];
        const float cp = introPop(it, 0.10f + 0.09f * static_cast<float>(i), 0.40f);
        if (cp <= 0.001f) continue;
        const float ca = clampf(cp, 0.f, 1.f);
        // rises up into place, and lifts a little more under the pointer
        const sf::Vector2f c = c0 + sf::Vector2f(0.f, (1.f - ca) * 46.f - 10.f * h);

        const UpgradeKind kind = app.choices()[i];
        const float sc = 0.55f + 0.45f * clampf(cp, 0.f, 1.05f);            // springs open
        const sf::FloatRect cardR{c.x - kCardW * 0.5f * sc, c.y - kCardH * 0.5f * sc, kCardW * sc, kCardH * sc};
        PickCardStyle st;
        st.hover = h;
        st.alpha = ca;
        st.time = it;
        st.reveal = ca;
        st.pop = cp;
        st.bottomReserve = app.rerollsLeft() > 0 ? 30.f : 0.f;
        // A ball that already has it would level it up: say so.
        for (const BallLoadout& L : app.data().run.balls)
            if (upgradeLevelsUp(kind, L) && L.levelOf(kind) < maxLevelOf(kind)) {
                st.note = "Lv " + std::to_string(L.levelOf(kind)) + " -> " + std::to_string(L.levelOf(kind) + 1);
                break;
            }
        if (sc > 0.98f) drawPickCard(w, app.font(), cardR, kind, app.data().run.balls, st);
        else drawTierFrame(w, cardR, upgradeTier(kind), h, ca, it, ca);   // still springing open: just the frame

        if (app.rerollsLeft() > 0) {   // "reroll this card" strip along the card's bottom edge
            const float wd = kCardW - 28.f, ht = 22.f;
            const float rh = rerollHover_[i];
            const float ry = c.y + kCardH * 0.5f - 15.f;
            draw::box(w, {c.x - wd * 0.5f, ry - ht * 0.5f, wd, ht}, theme::corner,
                      withAlpha(theme::textLo, (0.10f + 0.14f * rh) * ca), withAlpha(theme::textLo, (0.04f + 0.08f * rh) * ca),
                      withAlpha(theme::accent, (0.22f + 0.4f * rh) * ca), 1.f);
            drawLabel(w, app.font(), "reroll  " + std::to_string(app.rerollsLeft()), 12, {c.x, ry},
                      withAlpha(theme::textLo, ca));
        }
    }

    const float hintPop = introPop(it, 0.10f + 0.07f * app.choiceCount());
    if (coreHurt(app)) {
        const sf::FloatRect r = healRect(s);
        const float a = clampf(hintPop, 0.f, 1.f);
        draw::panel(w, r, theme::core, a, healHover_);
        drawCenteredPop(w, app.font(), "Repair the core instead", theme::fsSmall,
                        {s.x * 0.5f, r.top + r.height * 0.5f - 1.f}, theme::textHi, hintPop);
    }

    // Hover help: what the card's kind means, the reroll strip, the repair skip.
    if (app.rerollsLeft() > 0)
        for (int i = 0; i < app.choiceCount(); ++i)
            if (rerollRect(s, i, app.choiceCount()).contains(mouse_)) {
                drawTooltip(w, app.font(), mouse_, s, "Reroll",
                            "swap this card for a different pick (" + std::to_string(app.rerollsLeft()) +
                                " left this run)");
                return;
            }
    if (const int c = cardAt(app, mouse_); c >= 0) {
        const UpgradeCat cat = upgradeCat(app.choices()[c]);
        drawTooltip(w, app.font(), mouse_, s, upgradeCatName(cat), upgradeCatDesc(cat), catColor(cat));
    } else if (coreHurt(app) && healRect(s).contains(mouse_)) {
        drawTooltip(w, app.font(), mouse_, s, "Skip the pick",
                    std::string("repair the core to full instead of taking a card") +
                        (app.ironCoreAlive() ? "  Ends this act's Iron core." : ""),
                    theme::core);
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
                    {lastSound_ ? "Options" : "Options (muted)", true},
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
        case 3: app.openSound(); break;
        case 4: app.abandonRun(); break;
        case 5: app.quit(); break;
        default: break;
    }
}

void PauseScreen::update(App& app, float dt, sf::Vector2f mouse) {
    mouse_ = mouse;
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
        "Click a ball to throw it at the nearest enemy; press and pull back to aim it.",
        "Items carry a class tag: 2 of a tag give a ball that class (it can have two),",
        "4 its ascended form. Two balls' elements on one enemy set off a reaction.",
        "Between fights, pick your path on the map: fights pay gold, elites add a pick,",
        "shops / forges / rests / upgrades build your balls (4 item slots each).",
        "Five acts, a boss at the end of each. Beat the last one to finish the run.",
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
    const std::string downed = std::string(bossName(bossOfAct(cfg::run::actOfWave(app.data().run.wave)))) + " defeated";
    drawCenteredPop(w, app.font(), runEnd ? "Run complete" : downed, theme::fsTitle,
                    {s.x * 0.5f, s.y * 0.28f}, theme::core, introPop(it, 0.05f, 0.34f));

    if (goingOn) {
        const int next = cfg::run::actOfWave(app.data().run.wave) + 1;
        const std::string on = next == 2 ? "the run goes on: seal a creed, take the boss treasure, on to act 2"
                                         : "the run goes on: take the boss treasure, on to act " + std::to_string(next) +
                                               " of " + std::to_string(cfg::run::acts);
        drawCenteredPop(w, app.font(), on, theme::fsBody,
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

    // Clean-play bonuses this boss earned: one quiet line under the result.
    std::string bonus;
    if (goingOn && app.bossFlawlessGold() > 0) bonus = "FLAWLESS  +" + std::to_string(app.bossFlawlessGold()) + " gold";
    if (app.bossIronCores() > 0) {
        if (!bonus.empty()) bonus += "      ";
        bonus += "IRON CORE  +" + std::to_string(app.bossIronCores()) + " cores";
    }
    if (!bonus.empty())
        drawCenteredPop(w, app.font(), bonus, theme::fsBody, {s.x * 0.5f, s.y * 0.28f + (goingOn ? 92.f : 112.f)},
                        theme::core, introPop(it, 0.3f));

    menu_.draw(w, it);
}

}  // namespace sb
