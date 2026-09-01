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

sf::Vector2f kLogical() { return {1280.f, 800.f}; }

bool loadFont(sf::Font& font) {
    for (const char* path : {"assets/arial.ttf", "../assets/arial.ttf", "arial.ttf"})
        if (std::filesystem::exists(path) && font.loadFromFile(path)) return true;
    return false;
}

}  // namespace

App::App() : window_(kLogical()), world_(kLogical()) {
    if (!loadFont(font_)) {
        std::cerr << "Space-Breakers: could not load assets/arial.ttf (run from the project root)\n";
        std::exit(1);
    }
    if (!audio_.init())
        std::cerr << "Space-Breakers: audio unavailable, continuing without sound\n";

    loadGame(savePath_, data_);
    audio_.setEnabled(data_.meta.soundOn);
    window_.applyVideoMode(data_.meta.fullscreen);

    effects_.init(font_, size());
    hud_.init(font_, size());
    autosaveTimer_ = cfg::app::autosaveInterval;

    // Idle world so the renderer always has a valid core to draw.
    world_.startRun(params(), {}, cfg::core::baseHp, cfg::core::baseHp);
    replaceStack(ScreenId::Hub);
}

WorldParams App::params() const {
    return {data_.run.damageMult, 1.f, std::max(1, data_.run.wave)};
}

float App::startCoreHp() const {
    return cfg::core::baseHp +
           (data_.meta.unlock[MetaCoreHp] > 0 ? cfg::core::hpPlusBonus : 0.f);
}

// ---------------------------------------------------------------- screen stack

std::unique_ptr<Screen> App::makeScreen(ScreenId id) {
    switch (id) {
        case ScreenId::Hub:        return std::make_unique<HubScreen>();
        case ScreenId::Play:       return std::make_unique<PlayScreen>();
        case ScreenId::Choice:     return std::make_unique<ChoiceScreen>();
        case ScreenId::RunSummary: return std::make_unique<RunSummaryScreen>();
        case ScreenId::Pause:      return std::make_unique<PauseScreen>();
        case ScreenId::Stats:      return std::make_unique<StatsScreen>();
        case ScreenId::HowTo:      return std::make_unique<HowToScreen>();
    }
    return std::make_unique<HubScreen>();
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

// ---------------------------------------------------------------- run flow

void App::newRun() {
    RunState& r = data_.run;
    r = RunState{};
    r.active = true;
    r.damageMult = 1.f;
    r.wave = 0;
    r.coreMaxHp = startCoreHp();
    r.coreHp = r.coreMaxHp;
    r.scrap = 25;  // seed so the first offer is reachable
    r.balls.assign(2 + data_.meta.unlock[MetaStartBalls], static_cast<int>(Element::Plain));
    ++data_.meta.stats.runs;

    world_.startRun(params(), r.balls, r.coreHp, r.coreMaxHp);
    effects_.clear();
    startNextWave();
    replaceStack(ScreenId::Play);
    save();
}

void App::continueRun() {
    RunState& r = data_.run;
    if (!r.active) { newRun(); return; }
    if (r.wave < 1) r.wave = 1;
    if (r.balls.empty()) r.balls.push_back(static_cast<int>(Element::Plain));
    world_.startRun(params(), r.balls, r.coreHp, r.coreMaxHp);
    world_.startWave(r.wave, params());
    effects_.clear();
    replaceStack(ScreenId::Play);
}

void App::startNextWave() {
    data_.run.wave += 1;
    world_.repairCore(cfg::core::waveHeal);
    world_.startWave(data_.run.wave, params());
    data_.meta.stats.bestWave =
        std::max(data_.meta.stats.bestWave, static_cast<std::uint32_t>(data_.run.wave));
    save();
}

void App::openChoice() { push(ScreenId::Choice); }

void App::applyOffer(OfferKind k) {
    RunState& r = data_.run;
    if (runBallCount() >= cfg::ball::maxBalls) return;
    const std::uint32_t cost = offerCost(k, runBallCount());
    if (r.scrap < cost) return;

    r.scrap -= cost;
    const Element e = offerInfo(k).element;
    world_.addBall(e, params());
    r.balls.push_back(static_cast<int>(e));
    audio_.purchase();
    effects_.flash(elementColor(e), 0.45f);
    back();
    startNextWave();
}

void App::skipChoice() {
    back();
    startNextWave();
}

void App::endRun() {
    RunState& r = data_.run;
    lastRunWave_ = r.wave;
    lastRunCores_ = r.wave * cfg::meta::coresPerWave;
    data_.meta.cores += static_cast<std::uint32_t>(lastRunCores_);
    data_.meta.stats.bestWave =
        std::max(data_.meta.stats.bestWave, static_cast<std::uint32_t>(r.wave));
    data_.run = RunState{};
    data_.run.active = false;
    save();
    push(ScreenId::RunSummary);
}

void App::abandonRun() {
    data_.run = RunState{};
    data_.run.active = false;
    save();
    replaceStack(ScreenId::Hub);
}

void App::finishSummary() { replaceStack(ScreenId::Hub); }

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

void App::buyMetaUnlock(int u) {
    if (u < 0 || u >= MetaUnlockCount) return;
    if (metaUnlockMaxed(u, data_.meta.unlock[u])) return;
    const std::uint32_t cost = metaUnlockCost(u, data_.meta.unlock[u]);
    if (data_.meta.cores < cost) return;
    data_.meta.cores -= cost;
    ++data_.meta.unlock[u];
    audio_.purchase();
    effects_.flash(theme::accent, 0.4f);
    save();
}

void App::toggleSound() {
    data_.meta.soundOn = !data_.meta.soundOn;
    audio_.setEnabled(data_.meta.soundOn);
    save();
}

void App::toggleFullscreen() {
    data_.meta.fullscreen = !data_.meta.fullscreen;
    window_.applyVideoMode(data_.meta.fullscreen);
    save();
}

void App::save() {
    if (data_.run.active) {
        data_.run.coreHp = world_.core().hp;
        data_.run.coreMaxHp = world_.core().maxHp;
        if (world_.wave() > 0) data_.run.wave = world_.wave();
    }
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
    if (ev.scrapGained > 0) {
        data_.run.scrap += static_cast<std::uint32_t>(ev.scrapGained);
        data_.meta.stats.lifetimeScrap += static_cast<std::uint32_t>(ev.scrapGained);
    }
    for (const sf::Vector2f& k : ev.kills) {
        ++data_.meta.stats.enemiesKilled;
        effects_.addRing(k, 520.f, theme::enemy);
    }
    for (const BounceFx& b : ev.bounces) {
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
        effects_.addLabel(powerUpName(ev.pickupKind), {size().x * 0.5f, size().y * 0.34f}, c, 24, 1.1f);
        effects_.flash(c, 0.7f);
        audio_.pickup();
    }
    if (ev.coreHit) {
        effects_.flash(theme::coreLow, 0.55f);
        audio_.bounce(0.15f);
    }

    data_.meta.stats.bestCombo =
        std::max(data_.meta.stats.bestCombo, static_cast<std::uint32_t>(world_.comboStreak()));
    data_.meta.stats.maxSpeed = std::max(data_.meta.stats.maxSpeed, world_.fastestBall());

    if (ev.runOver) {
        endRun();
        return;
    }
    if (ev.waveCleared) {
        audio_.purchase();
        openChoice();
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
            if (!simulating()) break;  // a screen (choice / summary) was just pushed
        }
        if (steps == cfg::loop::maxSteps) worldAccum_ = 0.f;

        data_.meta.stats.timePlayed += frameDt;
        autosaveTimer_ -= frameDt;
        if (autosaveTimer_ <= 0.f) {
            save();
            autosaveTimer_ = cfg::app::autosaveInterval;
        }
    } else {
        worldAccum_ = 0.f;
    }

    const Core& c = world_.core();
    hud_.update(frameDt, data_.run.scrap, world_.wave(), world_.enemiesLeft(),
                c.maxHp > 0.f ? c.hp / c.maxHp : 0.f, world_.comboMultiplier(), world_.effect());
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
