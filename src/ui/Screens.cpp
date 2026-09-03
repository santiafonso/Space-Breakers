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

constexpr float kUnlockTop = 0.30f;   // * size.y
constexpr float kUnlockGap = 48.f;
constexpr float kUnlockRowW = 540.f;
constexpr float kUnlockRowH = 42.f;

}  // namespace

// ================================================================ Menu

void MenuScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    menu_.init(app.font(), theme::fsItem, s.y * 0.072f);
    menu_.setItems({{"Play", true}, {"Stats", true}, {"How to Play", true}, {"Quit", true}});
    menu_.layout({s.x * 0.5f, s.y * 0.46f});
}

void MenuScreen::onEnter(App& app) { rebuild(app); }

void MenuScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Enter) || isKey(e, sf::Keyboard::Space)) { app.openLoadout(); return; }
    if (!isLeftClick(e)) return;
    switch (menu_.clickIndex(mouse)) {
        case 0: app.openLoadout(); break;
        case 1: app.openStats(); break;
        case 2: app.openHowTo(); break;
        case 3: app.quit(); break;
        default: break;
    }
}

void MenuScreen::update(App&, float dt, sf::Vector2f mouse) { menu_.update(dt, mouse); }

void MenuScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    drawCentered(w, app.font(), "Space-Breakers", theme::fsTitle, {s.x * 0.5f, s.y * 0.2f},
                 theme::textHi);
    drawCentered(w, app.font(), std::to_string(app.data().meta.cores) + " cores", theme::fsHeading,
                 {s.x * 0.5f, s.y * 0.2f + 42.f}, theme::accent);
    menu_.draw(w);
}

// ================================================================ Loadout

void LoadoutScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    menu_.init(app.font(), theme::fsItem, s.y * 0.066f);
    menu_.setItems({{"Start run", true}, {"Back", true}});
    menu_.layout({s.x * 0.5f, s.y * 0.72f});
}

void LoadoutScreen::onEnter(App& app) { rebuild(app); }

sf::Vector2f LoadoutScreen::unlockRowCenter(App& app, int i) const {
    const sf::Vector2f s = app.size();
    return {s.x * 0.5f, s.y * kUnlockTop + static_cast<float>(i) * kUnlockGap};
}

int LoadoutScreen::unlockRowAt(App& app, sf::Vector2f mouse) const {
    for (int i = 0; i < MetaUnlockCount; ++i) {
        const sf::Vector2f c = unlockRowCenter(app, i);
        if (std::fabs(mouse.x - c.x) < kUnlockRowW * 0.5f &&
            std::fabs(mouse.y - c.y) < kUnlockRowH * 0.5f)
            return i;
    }
    return -1;
}

void LoadoutScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape)) { app.back(); return; }
    if (isKey(e, sf::Keyboard::Enter) || isKey(e, sf::Keyboard::Space)) { app.newRun(); return; }
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code < sf::Keyboard::Num1 + MetaUnlockCount) {
        app.buyMetaUnlock(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (!isLeftClick(e)) return;
    const int row = unlockRowAt(app, mouse);
    if (row >= 0) { app.buyMetaUnlock(row); return; }
    switch (menu_.clickIndex(mouse)) {
        case 0: app.newRun(); break;
        case 1: app.back(); break;
        default: break;
    }
}

void LoadoutScreen::update(App& app, float dt, sf::Vector2f mouse) {
    menu_.update(dt, mouse);
    const int row = unlockRowAt(app, mouse);
    const float k = 1.f - std::exp(-16.f * dt);
    for (int i = 0; i < MetaUnlockCount; ++i)
        hover_[i] = lerpf(hover_[i], row == i ? 1.f : 0.f, k);
}

void LoadoutScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const MetaState& m = app.data().meta;

    drawCentered(w, app.font(), "Game menu", theme::fsTitle, {s.x * 0.5f, s.y * 0.11f}, theme::textHi);
    drawCentered(w, app.font(), std::to_string(m.cores) + " cores", theme::fsHeading,
                 {s.x * 0.5f, s.y * 0.11f + 38.f}, theme::accent);

    if (app.lastRunWave() > 0) {
        char line[96];
        std::snprintf(line, sizeof(line), "Last run: %s on wave %d   +%d cores",
                      app.lastRunWon() ? "won" : "lost", app.lastRunWave(), app.lastRunCores());
        drawCentered(w, app.font(), line, theme::fsBody, {s.x * 0.5f, s.y * 0.11f + 70.f},
                     app.lastRunWon() ? theme::core : theme::textLo);
    }

    for (int i = 0; i < MetaUnlockCount; ++i) {
        const MetaUnlockDef& d = metaUnlockDef(i);
        const int lvl = m.unlock[i];
        const bool maxed = metaUnlockMaxed(i, lvl);
        const std::string cost = maxed ? "MAX" : (std::to_string(metaUnlockCost(i, lvl)) + " cores");
        const bool afford = !maxed && m.cores >= metaUnlockCost(i, lvl);
        const sf::Vector2f c = unlockRowCenter(app, i);

        sf::RectangleShape box({kUnlockRowW, kUnlockRowH});
        box.setOrigin(kUnlockRowW * 0.5f, kUnlockRowH * 0.5f);
        box.setPosition(c);
        box.setFillColor(withAlpha(theme::accent, 0.05f + 0.13f * hover_[i]));
        box.setOutlineThickness(1.f);
        box.setOutlineColor(withAlpha(theme::accent, afford ? 0.25f + 0.45f * hover_[i] : 0.12f));
        w.draw(box);

        char head[96];
        std::snprintf(head, sizeof(head), "%d.  %s   Lv %d", i + 1, d.name, lvl);
        sf::Text ht = makeText(app.font(), head, theme::fsBody,
                               maxed ? theme::textDim : (afford ? theme::textHi : theme::textLo));
        ht.setPosition(c.x - kUnlockRowW * 0.5f + 14.f, c.y - kUnlockRowH * 0.5f + 3.f);
        w.draw(ht);

        sf::Text ct = makeText(app.font(), cost, theme::fsBody,
                               maxed ? theme::textDim : (afford ? theme::accent : theme::textLo));
        const sf::FloatRect cb = ct.getLocalBounds();
        ct.setOrigin(cb.left + cb.width, cb.top);
        ct.setPosition(c.x + kUnlockRowW * 0.5f - 14.f, c.y - kUnlockRowH * 0.5f + 3.f);
        w.draw(ct);

        sf::Text et = makeText(app.font(), d.effect, theme::fsSmall, theme::textDim);
        et.setPosition(c.x - kUnlockRowW * 0.5f + 14.f, c.y + 2.f);
        w.draw(et);
    }

    menu_.draw(w);
    drawCentered(w, app.font(), "click a row or press 1-5      Enter starts the run", theme::fsSmall,
                 {s.x * 0.5f, s.y * 0.93f}, theme::textDim);
    if (app.devMode())
        drawCentered(w, app.font(),
                     "DEV: env SB_WAVE / SB_BALLS / SB_UPGRADES apply on Start   -   in-run keys shown on screen",
                     theme::fsSmall, {s.x * 0.5f, s.y * 0.96f}, theme::accent);
}

// ================================================================ Play

void PlayScreen::onEnter(App&) {
    dragging_ = false;
    showPicks_ = false;
    clock_ = 0.f;
    samples_.clear();
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
    const sf::Vector2f v = pointerVelocity() * cfg::app::throwVelScale;
    app.world().releaseHeld(v);
    if (app.world().grabbedKind() == Grabbed::None) app.audio().thrown(clampf(length(v) / 900.f, 0.f, 1.f));
    dragging_ = false;
    samples_.clear();
}

void PlayScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape)) { app.openPause(); return; }
    if (isKey(e, sf::Keyboard::M)) { app.toggleSound(); return; }
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
    if (dragging_ && !sf::Mouse::isButtonPressed(sf::Mouse::Left)) release(app);
    if (dragging_ && app.world().hasHeld()) {
        samples_.push_back({clock_, mouse});
        while (samples_.size() > 2 &&
               clock_ - samples_.front().first > cfg::app::pointerSampleWindow)
            samples_.pop_front();
        app.world().moveHeld(mouse);
    }
}

void PlayScreen::draw(App& app, sf::RenderWindow& w) {
    renderer_.draw(w, app.world());
    app.effects().drawRings(w);
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

    drawCentered(w, app.font(), "hold TAB for upgrades", theme::fsSmall,
                 {theme::margin + 60.f, s.y - theme::margin - 56.f}, theme::textDim);

    if (app.devMode()) drawDevKeys(app, w);
    if (showPicks_) drawPicks(app, w);
}

void PlayScreen::drawDevKeys(App& app, sf::RenderWindow& w) const {
    const sf::Vector2f s = app.size();
    const bool invuln = app.world().devInvuln();
    const std::array<std::string, 8> lines = {{
        "- DEV -",
        "N   win wave",
        "H   heal core",
        std::string("G   invuln: ") + (invuln ? "ON" : "off"),
        "B   add ball",
        "U   grant next upgrade",
        "C   +25 cores",
        "TAB   upgrades taken",
    }};
    const float right = s.x - theme::margin;
    float y = theme::margin + 74.f;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const sf::Color col = i == 0 ? theme::accent
                                     : (invuln && i == 3 ? theme::core : theme::textDim);
        sf::Text t = makeText(app.font(), lines[i], theme::fsSmall, col);
        const sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left + b.width, b.top);
        t.setPosition(right, y);
        w.draw(t);
        y += 17.f;
    }
}

void PlayScreen::drawPicks(App& app, sf::RenderWindow& w) const {
    const sf::Vector2f s = app.size();
    const std::vector<int>& picks = app.data().run.picks;

    // Tally repeats so "Heavy impact" taken 3x shows as one "x3" row.
    std::array<int, kUpgradeKindCount> count{};
    std::array<int, kUpgradeKindCount> firstSeen{};
    int order = 0;
    for (int i = 0; i < kUpgradeKindCount; ++i) firstSeen[i] = 1000;
    for (int p : picks) {
        if (p < 0 || p >= kUpgradeKindCount) continue;
        if (count[p] == 0) firstSeen[p] = order++;
        ++count[p];
    }

    std::vector<int> kinds;
    for (int i = 0; i < kUpgradeKindCount; ++i)
        if (count[i] > 0) kinds.push_back(i);
    std::sort(kinds.begin(), kinds.end(),
              [&](int a, int b) { return firstSeen[a] < firstSeen[b]; });

    const float panelW = 320.f;
    const float rowH = 26.f;
    const float panelH = 56.f + rowH * static_cast<float>(std::max<std::size_t>(kinds.size(), 1));
    const float px = s.x * 0.5f - panelW * 0.5f;
    const float py = s.y * 0.5f - panelH * 0.5f;

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition(px, py);
    panel.setFillColor(withAlpha(theme::panel, 0.92f));
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(withAlpha(theme::accent, 0.35f));
    w.draw(panel);

    drawCentered(w, app.font(), "Upgrades this run", theme::fsBody, {px + panelW * 0.5f, py + 18.f},
                 theme::textHi);

    if (kinds.empty()) {
        drawCentered(w, app.font(), "none yet", theme::fsSmall, {px + panelW * 0.5f, py + 46.f},
                     theme::textDim);
        return;
    }

    float y = py + 44.f;
    for (int k : kinds) {
        sf::Text t = makeText(app.font(), upgradeInfo(static_cast<UpgradeKind>(k)).title, theme::fsBody,
                              theme::textLo);
        t.setPosition(px + 18.f, y);
        w.draw(t);
        if (count[k] > 1) {
            sf::Text c = makeText(app.font(), "x" + std::to_string(count[k]), theme::fsBody,
                                  theme::accent);
            const sf::FloatRect cb = c.getLocalBounds();
            c.setOrigin(cb.left + cb.width, cb.top);
            c.setPosition(px + panelW - 18.f, y);
            w.draw(c);
        }
        y += rowH;
    }
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

void ChoiceScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code < sf::Keyboard::Num1 + kChoiceCount) {
        app.applyUpgrade(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (!isLeftClick(e)) return;
    const int c = cardAt(app, mouse);
    if (c >= 0) app.applyUpgrade(c);
}

void ChoiceScreen::update(App& app, float dt, sf::Vector2f mouse) {
    const float k = 1.f - std::exp(-16.f * dt);
    const int c = cardAt(app, mouse);
    for (int i = 0; i < kChoiceCount; ++i) hover_[i] = lerpf(hover_[i], c == i ? 1.f : 0.f, k);
}

void ChoiceScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();

    drawDim(w, s, 0.82f);
    drawCentered(w, app.font(), "Wave cleared - choose an upgrade", theme::fsTitle,
                 {s.x * 0.5f, s.y * 0.26f}, theme::textHi);

    for (int i = 0; i < kChoiceCount; ++i) {
        const UpgradeInfo info = upgradeInfo(app.choices()[i]);
        const sf::Vector2f c = cardCenter(s, i);
        const float h = hover_[i];

        sf::RectangleShape card({kCardW, kCardH});
        card.setOrigin(kCardW * 0.5f, kCardH * 0.5f);
        card.setPosition(c);
        card.setFillColor(withAlpha(theme::accent, 0.10f + 0.16f * h));
        card.setOutlineThickness(2.f);
        card.setOutlineColor(withAlpha(theme::accent, 0.4f + 0.5f * h));
        w.draw(card);

        drawCentered(w, app.font(), std::to_string(i + 1), theme::fsSmall,
                     {c.x, c.y - kCardH * 0.5f + 16.f}, theme::textDim);
        drawCentered(w, app.font(), info.title, theme::fsItem,
                     {c.x, c.y - kCardH * 0.5f + 52.f}, theme::textHi);

        const std::vector<std::string> desc =
            wrapText(app.font(), info.desc, theme::fsSmall, kCardW - 28.f);
        const float lineH = 18.f;
        float dy = c.y + 24.f - lineH * 0.5f * static_cast<float>(desc.size() - 1);
        for (const std::string& dl : desc) {
            drawCentered(w, app.font(), dl, theme::fsSmall, {c.x, dy}, theme::textLo);
            dy += lineH;
        }
    }

    drawCentered(w, app.font(), "click a card or press 1-4", theme::fsSmall,
                 {s.x * 0.5f, s.y * 0.52f + kCardH * 0.5f + 40.f}, theme::textDim);
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
    drawDim(w, s, 0.72f);
    drawCentered(w, app.font(), "Paused", theme::fsTitle, {s.x * 0.5f, s.y * 0.2f}, theme::textHi);
    menu_.draw(w);
}

// ================================================================ Stats

void StatsScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f) {
    if (isDismiss(e)) app.back();
}

void StatsScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    drawStatsPanel(w, app.font(), s, app.data().meta.stats);
    drawCentered(w, app.font(), "press ESC or click to go back", theme::fsSmall,
                 {s.x * 0.5f, s.y * 0.86f}, theme::textDim);
}

// ================================================================ HowTo

void HowToScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f) {
    if (isDismiss(e)) app.back();
}

void HowToScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    drawCentered(w, app.font(), "How to Play", theme::fsTitle, {s.x * 0.5f, s.y * 0.16f},
                 theme::textHi);

    const std::array<const char*, 5> lines = {{
        "Enemies march on the core at the centre. Keep it alive through 10 waves.",
        "Your ball bounces freely off the walls and the core - it tracks nothing.",
        "Grab the ball and fling it to aim it into the enemies.",
        "After each wave, pick 1 of 4 upgrades: more balls, core buffs, a fire ball...",
        "Clear wave 10 to win the run. Cores you earn buy permanent unlocks.",
    }};
    const float y0 = s.y * 0.32f;
    for (std::size_t i = 0; i < lines.size(); ++i)
        drawCentered(w, app.font(), lines[i], theme::fsBody,
                     {s.x * 0.5f, y0 + 44.f * static_cast<float>(i)},
                     i + 1 == lines.size() ? theme::textHi : theme::textLo);

    drawCentered(w, app.font(), "ESC  pause      TAB  upgrades taken      F  fullscreen      M  sound",
                 theme::fsSmall, {s.x * 0.5f, s.y * 0.72f}, theme::textLo);
    drawCentered(w, app.font(), "press ESC or click to go back", theme::fsSmall,
                 {s.x * 0.5f, s.y * 0.82f}, theme::textDim);
}

}  // namespace sb
