#include "core/App.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "platform/Paths.hpp"
#include "platform/Save.hpp"
#include "ui/Screens.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

sf::Vector2f kLogical() { return {1280.f, 800.f}; }

bool envDevMode() {
    const char* v = std::getenv("SB_DEV");
    return v && *v && *v != '0';
}

int envInt(const char* key, int fallback) {
    const char* v = std::getenv(key);
    if (!v || !*v) return fallback;
    try {
        return std::stoi(v);
    } catch (...) {
        return fallback;
    }
}

bool loadFont(sf::Font& font) {
    // Next to the binary first (packaged build), then the project-root layouts
    // used when running straight from a build tree.
    const std::filesystem::path dir = exeDir();
    const std::filesystem::path candidates[] = {
        dir / "assets" / "arial.ttf", dir / "arial.ttf",
        dir / ".." / "assets" / "arial.ttf",
        "assets/arial.ttf", "../assets/arial.ttf", "arial.ttf",
    };
    for (const std::filesystem::path& path : candidates)
        if (std::filesystem::exists(path) && font.loadFromFile(path.string())) return true;
    return false;
}

}  // namespace

App::App() : window_(kLogical()), world_(kLogical()) {
    savePath_ = (exeDir() / "saves" / "save.txt").string();

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
    replaceStack(ScreenId::Menu);
}

WorldParams App::params() const {
    const RunMods& m = data_.run.mods;
    WorldParams p;
    p.damageMult = 1.f + cfg::combat::heavyImpactPerPick * static_cast<float>(m.heavyImpact);
    p.wave = std::max(1, data_.run.wave);
    p.ballRadiusMult = 1.f + cfg::combat::bigBallPerPick * static_cast<float>(m.bigBall);
    p.coreBounceBoost = m.spring ? cfg::combat::springBoost : 1.f;
    p.flingDecayMult = m.flingMomentum ? cfg::combat::flingDecayMult : 1.f;
    p.retaliate = m.retaliate;
    p.strayBolt = m.strayBolt;
    p.secondChanceAvail = m.secondChance && !m.secondChanceUsed;
    p.powerUpsUnlocked = powerUpsUnlocked();
    return p;
}

int App::startBallCount() const {
    return cfg::run::startBalls + data_.meta.unlock[MetaStartBalls];
}

float App::startCoreHp() const {
    return cfg::core::baseHp +
           cfg::core::hpPerBulwark * static_cast<float>(data_.meta.unlock[MetaCoreHp]);
}

int App::fireCap() const { return 1 + data_.meta.unlock[MetaFireCap]; }

int App::powerUpsUnlocked() const { return 2 + data_.meta.unlock[MetaPowerups]; }

// ---------------------------------------------------------------- screen stack

std::unique_ptr<Screen> App::makeScreen(ScreenId id) {
    switch (id) {
        case ScreenId::Menu:    return std::make_unique<MenuScreen>();
        case ScreenId::Loadout: return std::make_unique<LoadoutScreen>();
        case ScreenId::Play:    return std::make_unique<PlayScreen>();
        case ScreenId::Choice:  return std::make_unique<ChoiceScreen>();
        case ScreenId::Pause:   return std::make_unique<PauseScreen>();
        case ScreenId::Stats:   return std::make_unique<StatsScreen>();
        case ScreenId::HowTo:   return std::make_unique<HowToScreen>();
    }
    return std::make_unique<MenuScreen>();
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

void App::openLoadout() { push(ScreenId::Loadout); }

void App::newRun() {
    RunState& r = data_.run;
    r = RunState{};
    r.active = true;
    r.wave = 0;
    r.coreMaxHp = startCoreHp();
    r.coreHp = r.coreMaxHp;
    r.balls.assign(static_cast<std::size_t>(startBallCount()), static_cast<int>(Element::Plain));
    ++data_.meta.stats.runs;

    // Dev overrides: SB_BALLS / SB_WAVE / SB_UPGRADES=Name,Name,...
    int startWave = 1;
    if (devMode()) {
        const int nb = envInt("SB_BALLS", 0);
        if (nb > 0)
            r.balls.assign(static_cast<std::size_t>(std::min(nb, cfg::ball::maxBalls)),
                           static_cast<int>(Element::Plain));
        startWave = std::clamp(envInt("SB_WAVE", 1), 1, cfg::run::finalWave);
    }

    world_.startRun(params(), r.balls, r.coreHp, r.coreMaxHp);
    effects_.clear();

    if (devMode()) {
        if (const char* up = std::getenv("SB_UPGRADES")) {
            std::string s(up), tok;
            std::stringstream ss(s);
            while (std::getline(ss, tok, ',')) {
                for (int i = 0; i < kUpgradeKindCount; ++i) {
                    const auto k = static_cast<UpgradeKind>(i);
                    if (tok == upgradeKindId(k)) { applyUpgradeKind(k); break; }
                }
            }
        }
    }

    r.wave = startWave - 1;
    startNextWave();
    replaceStack(ScreenId::Play);
    save();
}

void App::startNextWave() {
    data_.run.wave += 1;
    world_.repairCore(cfg::core::waveHeal);
    if (data_.run.wave >= cfg::run::finalWave)
        world_.startBossWave(params());        // wide arena + miniboss
    else
        world_.startWave(data_.run.wave, params());
    data_.meta.stats.bestWave =
        std::max(data_.meta.stats.bestWave, static_cast<std::uint32_t>(data_.run.wave));
}

void App::rollChoices() {
    const RunState& r = data_.run;
    int fireBalls = 0;
    for (int e : r.balls)
        if (e == static_cast<int>(Element::Fire)) ++fireBalls;

    UpgradeCtx c;
    c.ballCount = runBallCount();
    c.maxBalls = cfg::ball::maxBalls;
    c.fireBalls = fireBalls;
    c.fireCap = fireCap();
    c.fireUnlocked = data_.meta.unlock[MetaFireBall] > 0;
    c.coreFull = world_.core().hp >= world_.core().maxHp - 0.5f;
    c.bigBallPicks = r.mods.bigBall;
    c.spring = r.mods.spring;
    c.retaliate = r.mods.retaliate;
    c.flingMomentum = r.mods.flingMomentum;
    c.strayBolt = r.mods.strayBolt;
    c.loot = r.mods.loot;
    c.secondChance = r.mods.secondChance;

    std::vector<UpgradeKind> pool;
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        if (upgradeEligible(k, c)) pool.push_back(k);
    }
    for (std::size_t i = pool.size(); i > 1; --i)
        std::swap(pool[i - 1], pool[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(i) - 1))]);

    for (int i = 0; i < kChoiceCount; ++i)
        choices_[i] = pool.empty() ? UpgradeKind::CoreRepair
                                   : pool[static_cast<std::size_t>(i) % pool.size()];
}

void App::openChoice() {
    rollChoices();
    push(ScreenId::Choice);
}

void App::applyUpgradeKind(UpgradeKind k) {
    RunState& r = data_.run;
    RunMods& m = r.mods;
    r.picks.push_back(static_cast<int>(k));
    switch (k) {
        case UpgradeKind::AddBall:
            world_.addBall(Element::Plain, params());
            r.balls.push_back(static_cast<int>(Element::Plain));
            break;
        case UpgradeKind::BallToFire:
            for (int& e : r.balls)
                if (e == static_cast<int>(Element::Plain)) { e = static_cast<int>(Element::Fire); break; }
            world_.convertOneBall(Element::Plain, Element::Fire);
            break;
        case UpgradeKind::CoreArmor:
            ++m.coreArmor;
            world_.addCoreMaxHp(cfg::combat::coreArmorHp);
            break;
        case UpgradeKind::CoreRepair:    world_.repairCore(1e9f); break;
        case UpgradeKind::CoreSpring:    m.spring = true; break;
        case UpgradeKind::CoreRetaliate: m.retaliate = true; break;
        case UpgradeKind::FlingMomentum: m.flingMomentum = true; break;
        case UpgradeKind::HeavyImpact:   ++m.heavyImpact; break;
        case UpgradeKind::BigBall:       ++m.bigBall; break;
        case UpgradeKind::StrayBolt:     m.strayBolt = true; break;
        case UpgradeKind::Loot:          m.loot = true; break;
        case UpgradeKind::SecondChance:  m.secondChance = true; break;
    }
}

void App::applyUpgrade(int idx) {
    if (idx < 0 || idx >= kChoiceCount) return;
    applyUpgradeKind(choices_[idx]);
    audio_.purchase();
    effects_.flash(theme::accent, 0.4f);
    back();
    startNextWave();
}

void App::endRun(bool won) {
    RunState& r = data_.run;
    lastRunWave_ = r.wave;
    lastRunWon_ = won;

    int cores = r.wave * cfg::meta::coresPerWave + (won ? cfg::meta::winBonus : 0);
    if (r.mods.loot) cores = cores * (100 + cfg::combat::lootBonusPct) / 100;
    lastRunCores_ = cores;

    data_.meta.cores += static_cast<std::uint32_t>(cores);
    data_.meta.stats.coresEarned += static_cast<std::uint32_t>(cores);
    data_.meta.stats.bestWave =
        std::max(data_.meta.stats.bestWave, static_cast<std::uint32_t>(r.wave));
    if (won) ++data_.meta.stats.wins;

    data_.run = RunState{};
    save();
    replaceStack(ScreenId::Menu);
    push(ScreenId::Loadout);
}

void App::abandonRun() {
    data_.run = RunState{};
    save();
    replaceStack(ScreenId::Menu);
    push(ScreenId::Loadout);
}

// ---------------------------------------------------------------- dev tools

bool App::devMode() const {
    static const bool on = envDevMode();
    return on;
}

void App::devWinWave() {
    if (!devMode() || !data_.run.active) return;
    world_.devWinWave();
}

void App::devGrantCores(int n) {
    if (!devMode()) return;
    data_.meta.cores += static_cast<std::uint32_t>(std::max(0, n));
    effects_.flash(theme::accent, 0.3f);
}

void App::devHealCore() {
    if (!devMode() || !data_.run.active) return;
    world_.repairCore(1e9f);
    effects_.flash(theme::core, 0.3f);
}

void App::devToggleInvuln() {
    if (!devMode() || !data_.run.active) return;
    world_.devSetInvuln(!world_.devInvuln());
    effects_.flash(world_.devInvuln() ? theme::core : theme::coreLow, 0.3f);
}

void App::devAddBall() {
    if (!devMode() || !data_.run.active) return;
    if (runBallCount() >= cfg::ball::maxBalls) return;
    world_.addBall(Element::Plain, params());
    data_.run.balls.push_back(static_cast<int>(Element::Plain));
}

void App::devCycleGrant() {
    if (!devMode() || !data_.run.active) return;
    const auto k = static_cast<UpgradeKind>(devGrantNext_ % kUpgradeKindCount);
    devGrantNext_ = (devGrantNext_ + 1) % kUpgradeKindCount;
    applyUpgradeKind(k);
    effects_.addLabel(std::string("+ ") + upgradeInfo(k).title, {size().x * 0.5f, size().y * 0.4f},
                      theme::accent, 22, 1.0f);
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

void App::save() { saveGame(savePath_, data_); }

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
    if (ev.corePulsed) {
        effects_.addRing(ev.corePulsePos, 900.f, theme::core);
        effects_.flash(theme::core, 0.4f);
        audio_.comboUp(2);
    }
    if (ev.secondChanceUsed) {
        data_.run.mods.secondChanceUsed = true;
        effects_.addLabel("SECOND CHANCE", {size().x * 0.5f, size().y * 0.34f}, theme::core, 24, 1.2f);
        effects_.flash(theme::core, 0.8f);
        audio_.pickup();
    }

    data_.meta.stats.bestCombo =
        std::max(data_.meta.stats.bestCombo, static_cast<std::uint32_t>(world_.comboStreak()));
    data_.meta.stats.maxSpeed = std::max(data_.meta.stats.maxSpeed, world_.fastestBall());

    if (ev.runOver) {
        endRun(false);
        return;
    }
    if (ev.waveCleared) {
        if (data_.run.wave >= cfg::run::finalWave) {
            endRun(true);
            return;
        }
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

    // Camera: ease toward the area the world wants framed (wider on the boss
    // wave), snap back to the fixed view whenever we are not in a live run.
    sf::Vector2f tgtSize = kLogical();
    sf::Vector2f tgtCenter = kLogical() * 0.5f;
    bool snap = true;
    if (simulating() && data_.run.active) {
        tgtSize = world_.viewSize();
        tgtCenter = world_.viewCenter();
        snap = false;
    }
    if (snap) {
        camSize_ = tgtSize;
        camCenter_ = tgtCenter;
    } else {
        const float k = 1.f - std::exp(-cfg::boss::camEase * frameDt);
        camSize_ += (tgtSize - camSize_) * k;
        camCenter_ += (tgtCenter - camCenter_) * k;
    }
    window_.setWorldView(camSize_, camCenter_);

    const Core& c = world_.core();
    hud_.update(frameDt, world_.wave(), cfg::run::finalWave, world_.enemiesLeft(),
                c.maxHp > 0.f ? c.hp / c.maxHp : 0.f, world_.comboMultiplier(), world_.effect(),
                world_.bossWave());
}

void App::render() {
    sf::RenderWindow& w = window_.handle();
    window_.useUiView();
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
