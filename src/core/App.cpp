#include "core/App.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "platform/Save.hpp"
#include "ui/Screens.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

// Fixed internal resolution. The window can be any size; the letterboxed view
// maps this logical space onto it, so every layout number is resolution-free.
sf::Vector2f kLogical() { return {1280.f, 800.f}; }

bool loadFont(sf::Font& font) {
    for (const char* path : {"arial.ttf", "assets/arial.ttf", "../arial.ttf"})
        if (std::filesystem::exists(path) && font.loadFromFile(path)) return true;
    return false;
}

}  // namespace

App::App() : window_(kLogical()), world_(kLogical()) {
    if (!loadFont(font_)) {
        std::cerr << "Space-Breakers: could not load arial.ttf (looked in . and ./assets)\n";
        std::exit(1);
    }
    if (!audio_.init())
        std::cerr << "Space-Breakers: audio unavailable, continuing without sound\n";

    loadGame(savePath_, data_);
    audio_.setEnabled(data_.soundOn);
    window_.applyVideoMode(data_.fullscreen);

    effects_.init(font_, size());
    hud_.init(font_, size());
    world_.reset(data_.level[UpgMultiball], params(), data_.walls);

    autosaveTimer_ = cfg::app::autosaveInterval;
    replaceStack(ScreenId::MainMenu);
}

WorldParams App::params() const {
    return {data_.level[UpgSpeed], data_.level[UpgPoints], data_.level[UpgWalls],
            data_.level[UpgCombo], data_.level[UpgLuck]};
}

bool App::hasSave() const { return hasSavedGame(savePath_); }

// ---------------------------------------------------------------- screen stack

std::unique_ptr<Screen> App::makeScreen(ScreenId id) {
    switch (id) {
        case ScreenId::MainMenu: return std::make_unique<MainMenuScreen>();
        case ScreenId::Play:     return std::make_unique<PlayScreen>();
        case ScreenId::Shop:     return std::make_unique<ShopScreen>();
        case ScreenId::Pause:    return std::make_unique<PauseScreen>();
        case ScreenId::Stats:    return std::make_unique<StatsScreen>();
        case ScreenId::HowTo:    return std::make_unique<HowToScreen>();
    }
    return std::make_unique<MainMenuScreen>();
}

void App::replaceStack(ScreenId id) {
    stack_.clear();
    stack_.push_back(makeScreen(id));
    fade_ = 1.f;
    stack_.back()->onEnter(*this);
}

void App::push(ScreenId id) {
    stack_.push_back(makeScreen(id));
    fade_ = 1.f;
    stack_.back()->onEnter(*this);
}

void App::back() {
    if (stack_.size() > 1) stack_.pop_back();
    fade_ = 1.f;
}

bool App::simulating() const {
    return !stack_.empty() && stack_.back()->simulates();
}

// ---------------------------------------------------------------- commands

void App::newRun() {
    data_.points = 0;
    for (int i = 0; i < UpgradeCount; ++i) data_.level[i] = 0;
    data_.walls.clear();
    world_.reset(0, params(), data_.walls);
    effects_.clear();
    runStartBest_ = data_.stats.bestScore;
    announcedBest_ = false;
    autosaveTimer_ = cfg::app::autosaveInterval;
    save();
    replaceStack(ScreenId::Play);
}

void App::continueRun() {
    loadGame(savePath_, data_);
    audio_.setEnabled(data_.soundOn);
    world_.reset(data_.level[UpgMultiball], params(), data_.walls);
    effects_.clear();
    runStartBest_ = data_.stats.bestScore;
    announcedBest_ = false;
    autosaveTimer_ = cfg::app::autosaveInterval;
    replaceStack(ScreenId::Play);
}

void App::toMenu() {
    world_.forceRelease();
    save();
    replaceStack(ScreenId::MainMenu);
}

void App::openShop() {
    world_.forceRelease();
    push(ScreenId::Shop);
}

void App::openPause() {
    world_.forceRelease();
    push(ScreenId::Pause);
}

void App::openStats() { push(ScreenId::Stats); }
void App::openHowTo() { push(ScreenId::HowTo); }

void App::quit() {
    save();
    window_.close();
}

void App::toggleSound() {
    data_.soundOn = !data_.soundOn;
    audio_.setEnabled(data_.soundOn);
    save();
}

void App::toggleFullscreen() {
    data_.fullscreen = !data_.fullscreen;
    window_.applyVideoMode(data_.fullscreen);
    save();
}

void App::buyUpgrade(int u) {
    if (u < 0 || u >= UpgradeCount) return;
    if (upgradeMaxed(u, data_.level[u])) return;
    const std::uint32_t cost = upgradeCost(u, data_.level[u]);
    if (data_.points < cost) return;

    data_.points -= cost;
    ++data_.level[u];
    if (u == UpgMultiball) world_.setMultiball(data_.level[UpgMultiball], params());
    if (u == UpgWalls) world_.syncWallCount(data_.level[UpgWalls]);
    audio_.purchase();
    effects_.flash(theme::accent, 0.5f);
}

void App::save() {
    data_.walls = world_.wallSnapshot();
    saveGame(savePath_, data_);
}

// ---------------------------------------------------------------- loop

void App::handleEvent(const sf::Event& e) {
    if (e.type == sf::Event::Closed) {
        quit();
        return;
    }
    if (e.type == sf::Event::Resized) {
        window_.applyLetterbox(e.size.width, e.size.height);
        return;
    }
    if (e.type == sf::Event::KeyPressed &&
        (e.key.code == sf::Keyboard::F11 || e.key.code == sf::Keyboard::F)) {
        toggleFullscreen();
        return;
    }
    if (!stack_.empty())
        stack_.back()->handleEvent(*this, e, window_.mousePosition());
}

void App::processEvents(const FrameEvents& ev) {
    if (ev.pointsGained > 0) {
        data_.points += static_cast<std::uint32_t>(ev.pointsGained);
        data_.stats.lifetimePoints += static_cast<std::uint32_t>(ev.pointsGained);
    }
    for (const BounceFx& b : ev.bounces) {
        ++data_.stats.lifetimeBounces;
        effects_.addRing(b.pos, b.speed, b.color);
        effects_.edgeHit(b.normal);
        audio_.bounce(clampf(b.speed / 900.f, 0.f, 1.f));
    }
    if (ev.comboTierUp) {
        hud_.pulseCombo();
        audio_.comboUp(ev.comboTier);
    }
    if (ev.gotPickup) {
        const sf::Color c = powerUpColor(ev.pickupKind);
        effects_.addLabel(powerUpName(ev.pickupKind), {size().x * 0.5f, size().y * 0.38f}, c, 24, 1.1f);
        effects_.flash(c, 0.7f);
        audio_.pickup();
    }

    data_.stats.bestScore = std::max(data_.stats.bestScore, data_.points);
    data_.stats.bestCombo =
        std::max(data_.stats.bestCombo, static_cast<std::uint32_t>(world_.comboStreak()));
    data_.stats.maxSpeed = std::max(data_.stats.maxSpeed, world_.fastestBall());

    if (!announcedBest_ && runStartBest_ > 0 && data_.points > runStartBest_) {
        announcedBest_ = true;
        effects_.addLabel("NEW BEST", {size().x * 0.5f, size().y * 0.46f}, theme::accent, 18, 1.3f);
    }
}

void App::update(float frameDt) {
    const sf::Vector2f mouse = window_.mousePosition();
    effects_.update(frameDt);
    fade_ *= std::exp(-cfg::app::fadeRate * frameDt);

    if (!stack_.empty()) stack_.back()->update(*this, frameDt, mouse);

    if (simulating()) {
        worldAccum_ += frameDt;
        int steps = 0;
        while (worldAccum_ >= cfg::loop::fixedDt && steps < cfg::loop::maxSteps) {
            processEvents(world_.step(cfg::loop::fixedDt, params()));
            worldAccum_ -= cfg::loop::fixedDt;
            ++steps;
        }
        if (steps == cfg::loop::maxSteps) worldAccum_ = 0.f;  // drop an unrecoverable backlog

        data_.stats.timePlayed += frameDt;
        autosaveTimer_ -= frameDt;
        if (autosaveTimer_ <= 0.f) {
            save();
            autosaveTimer_ = cfg::app::autosaveInterval;
        }
    } else {
        worldAccum_ = 0.f;
    }

    hud_.update(frameDt, data_.points, data_.stats.bestScore, world_.comboMultiplier(),
                world_.effect());
}

void App::render() {
    sf::RenderWindow& w = window_.handle();
    w.clear(theme::bg);
    effects_.drawBorder(w);

    std::size_t start = 0;
    for (std::size_t i = stack_.size(); i-- > 0;) {
        if (stack_[i]->opaque()) {
            start = i;
            break;
        }
    }
    for (std::size_t i = start; i < stack_.size(); ++i) stack_[i]->draw(*this, w);

    effects_.drawOverlay(w);
    if (fade_ > 0.01f) drawDim(w, size(), fade_ * 0.5f);
    w.display();
}

int App::run() {
    sf::Clock clock;
    while (window_.isOpen()) {
        sf::Event e;
        while (window_.handle().pollEvent(e)) handleEvent(e);
        if (!window_.isOpen()) break;

        const float frameDt = std::min(clock.restart().asSeconds(), cfg::loop::maxFrame);
        update(frameDt);
        render();
    }
    save();
    return 0;
}

}  // namespace sb
