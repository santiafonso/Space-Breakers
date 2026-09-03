#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "platform/Audio.hpp"
#include "platform/Window.hpp"
#include "progression/GameData.hpp"
#include "progression/Offers.hpp"
#include "render/Effects.hpp"
#include "sim/World.hpp"
#include "ui/Hud.hpp"
#include "ui/Screen.hpp"

namespace sb {

enum class ScreenId { Menu, Loadout, Play, Choice, Pause, Stats, HowTo };

// Top-level application: owns the window, subsystems and the screen stack, runs
// the loop (fixed-step simulation, per-frame render) and wires the flow:
// Menu -> Loadout (spend cores) -> Play (10 waves, Choice between each) -> Loadout.
class App {
public:
    App();
    int run();

    // ---- screen API ------------------------------------------------------
    World& world() { return world_; }
    const World& world() const { return world_; }
    GameData& data() { return data_; }
    Audio& audio() { return audio_; }
    Effects& effects() { return effects_; }
    Hud& hud() { return hud_; }
    const sf::Font& font() const { return font_; }
    sf::Vector2f size() const { return window_.logicalSize(); }
    WorldParams params() const;

    int runBallCount() const { return static_cast<int>(data_.run.balls.size()); }
    int lastRunWave() const { return lastRunWave_; }
    int lastRunCores() const { return lastRunCores_; }
    bool lastRunWon() const { return lastRunWon_; }
    const std::array<UpgradeKind, kChoiceCount>& choices() const { return choices_; }

    void openLoadout();     // Menu -> the game menu
    void newRun();          // Loadout "Start" -> a fresh run
    void applyUpgrade(int idx);   // Choice: pick one of the four
    void abandonRun();

    // ---- dev tools: enabled by the SB_DEV env var, no-ops otherwise -----
    bool devMode() const;
    void devWinWave();
    void devGrantCores(int n);
    void devHealCore();
    void devToggleInvuln();
    void devAddBall();
    void devCycleGrant();  // grant the "next" upgrade in the pool
    void openPause();
    void openStats();
    void openHowTo();
    void back();
    void quit();

    void buyMetaUnlock(int unlock);
    void toggleSound();
    void toggleFullscreen();
    void save();

private:
    std::unique_ptr<Screen> makeScreen(ScreenId id);
    void replaceStack(ScreenId id);
    void push(ScreenId id);
    bool simulating() const;

    int startBallCount() const;
    float startCoreHp() const;
    int fireCap() const;
    int powerUpsUnlocked() const;
    void startNextWave();
    void openChoice();
    void rollChoices();
    void applyUpgradeKind(UpgradeKind k);
    void endRun(bool won);

    void handleEvent(const sf::Event& e);
    void update(float frameDt);
    void render();
    void processEvents(const FrameEvents& ev);

    Window window_;
    sf::Font font_;
    Audio audio_;
    World world_;
    Effects effects_;
    Hud hud_;

    GameData data_;
    Rng rng_;
    std::string savePath_;  // set in the ctor: <exe dir>/saves/save.txt

    std::vector<std::unique_ptr<Screen>> stack_;
    std::array<UpgradeKind, kChoiceCount> choices_{};
    int lastRunWave_ = 0;
    int lastRunCores_ = 0;
    bool lastRunWon_ = false;
    int devGrantNext_ = 0;

    float fade_ = 0.f;
    float worldAccum_ = 0.f;
    float autosaveTimer_ = 20.f;
};

}  // namespace sb
