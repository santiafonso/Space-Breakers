#include "ui/Screens.hpp"

#include <array>
#include <cmath>

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
bool isRightClick(const sf::Event& e) {
    return e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Right;
}
bool isDismiss(const sf::Event& e) {
    return isLeftClick(e) || isKey(e, sf::Keyboard::Escape) || isKey(e, sf::Keyboard::Enter) ||
           isKey(e, sf::Keyboard::Space);
}

}  // namespace

// ---------------------------------------------------------------- MainMenu

void MainMenuScreen::onEnter(App& app) {
    const sf::Vector2f s = app.size();
    menu_.init(app.font(), theme::fsItem, s.y * 0.072f);
    menu_.setItems({{"New Game", true},
                    {"Continue", app.hasSave()},
                    {"Stats", true},
                    {"How to Play", true},
                    {"Quit", true}});
    menu_.layout({s.x * 0.5f, s.y * 0.42f});
}

void MainMenuScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (!isLeftClick(e)) return;
    switch (menu_.clickIndex(mouse)) {
        case 0: app.newRun(); break;
        case 1: app.continueRun(); break;
        case 2: app.openStats(); break;
        case 3: app.openHowTo(); break;
        case 4: app.quit(); break;
        default: break;
    }
}

void MainMenuScreen::update(App&, float dt, sf::Vector2f mouse) { menu_.update(dt, mouse); }

void MainMenuScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    drawCentered(w, app.font(), "Space-Breakers", theme::fsTitle, {s.x * 0.5f, s.y * 0.22f},
                 theme::textHi);
    drawCentered(w, app.font(), "an idle game about a bouncing ball", theme::fsBody,
                 {s.x * 0.5f, s.y * 0.22f + 42.f}, theme::textLo);
    menu_.draw(w);
}

// ---------------------------------------------------------------- Play

void PlayScreen::onEnter(App&) {
    dragging_ = false;
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
    app.audio().thrown(clampf(length(v) / 900.f, 0.f, 1.f));
    dragging_ = false;
    samples_.clear();
}

void PlayScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Tab)) { app.openShop(); return; }
    if (isKey(e, sf::Keyboard::Escape)) { app.openPause(); return; }
    if (isKey(e, sf::Keyboard::M)) { app.toggleSound(); return; }
    if (e.type == sf::Event::LostFocus) { release(app); return; }
    if (isLeftClick(e)) { grab(app, mouse); return; }
    if (isRightClick(e)) {
        if (app.world().toggleDriftAt(mouse)) {
            app.audio().thrown(0.2f);
            app.effects().flash(theme::accent, 0.25f);
        }
        return;
    }
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
}

// ---------------------------------------------------------------- Shop

void ShopScreen::onEnter(App& app) { shop_.init(app.font(), app.size()); }

void ShopScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Tab) || isKey(e, sf::Keyboard::Escape)) { app.back(); return; }
    if (isKey(e, sf::Keyboard::M)) { app.toggleSound(); return; }
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code <= sf::Keyboard::Num6) {
        app.buyUpgrade(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (isLeftClick(e)) app.buyUpgrade(shop_.rowAt(mouse));
}

void ShopScreen::update(App&, float dt, sf::Vector2f mouse) { shop_.update(dt, mouse); }

void ShopScreen::draw(App& app, sf::RenderWindow& w) {
    drawDim(w, app.size(), 0.72f);
    shop_.draw(w, app.data());
}

// ---------------------------------------------------------------- Pause

void PauseScreen::rebuild(App& app) {
    const sf::Vector2f s = app.size();
    lastSound_ = app.data().soundOn;
    menu_.init(app.font(), theme::fsItem, s.y * 0.064f);
    menu_.setItems({{"Resume", true},
                    {"Stats", true},
                    {"How to Play", true},
                    {lastSound_ ? "Sound: On" : "Sound: Off", true},
                    {"Main Menu", true},
                    {"Quit", true}});
    menu_.layout({s.x * 0.5f, s.y * 0.36f});
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
        case 4: app.toMenu(); break;
        case 5: app.quit(); break;
        default: break;
    }
}

void PauseScreen::update(App& app, float dt, sf::Vector2f mouse) {
    if (app.data().soundOn != lastSound_) rebuild(app);
    menu_.update(dt, mouse);
}

void PauseScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    drawDim(w, s, 0.72f);
    drawCentered(w, app.font(), "Paused", theme::fsTitle, {s.x * 0.5f, s.y * 0.2f}, theme::textHi);
    menu_.draw(w);
}

// ---------------------------------------------------------------- Stats

void StatsScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f) {
    if (isDismiss(e)) app.back();
}

void StatsScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    drawStatsPanel(w, app.font(), s, app.data().stats);
    drawCentered(w, app.font(), "press ESC or click to go back", theme::fsSmall,
                 {s.x * 0.5f, s.y * 0.86f}, theme::textDim);
}

// ---------------------------------------------------------------- HowTo

void HowToScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f) {
    if (isDismiss(e)) app.back();
}

void HowToScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    drawCentered(w, app.font(), "How to Play", theme::fsTitle, {s.x * 0.5f, s.y * 0.18f},
                 theme::textHi);

    const std::array<const char*, 6> lines = {{
        "Grab the ball and fling it. A faster ball scores more on every bounce.",
        "Keep it bouncing without a pause to build a combo multiplier.",
        "Steer the ball into a glowing orb to trigger a power-up.",
        "Drag your walls to place them. Fling a wall (or right-click it) to set it drifting.",
        "Right-click a drifting wall to stop it again.",
        "TAB  upgrades      ESC  pause      F  fullscreen      M  sound",
    }};
    const float y0 = s.y * 0.32f;
    for (std::size_t i = 0; i < lines.size(); ++i)
        drawCentered(w, app.font(), lines[i], theme::fsBody,
                     {s.x * 0.5f, y0 + 42.f * static_cast<float>(i)},
                     i + 1 == lines.size() ? theme::textHi : theme::textLo);

    drawCentered(w, app.font(), "press ESC or click to go back", theme::fsSmall,
                 {s.x * 0.5f, s.y * 0.84f}, theme::textDim);
}

}  // namespace sb
