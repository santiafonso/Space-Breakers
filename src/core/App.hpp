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

enum class ScreenId { Hub, Play, Choice, RunSummary, Pause, Stats, HowTo };

// Top-level application: owns the window, subsystems and the screen stack, runs
// the loop (fixed-step simulation, per-frame render) and wires the roguelite
// flow: Hub -> Play (waves) -> Choice -> ... -> RunSummary -> Hub.
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

    bool hasRunInProgress() const { return data_.run.active; }
    const std::array<OfferKind, 3>& offers() const { return offers_; }
    int lastRunWave() const { return lastRunWave_; }
    int lastRunCores() const { return lastRunCores_; }

    void newRun();
    void continueRun();
    void applyOffer(OfferKind k);
    void skipChoice();
    void abandonRun();
    void finishSummary();
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

    float startCoreHp() const;
    void startNextWave();
    void openChoice();
    void endRun();

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
    const std::string savePath_ = "saves/save.txt";

    std::vector<std::unique_ptr<Screen>> stack_;
    std::array<OfferKind, 3> offers_{};
    int lastRunWave_ = 0;
    int lastRunCores_ = 0;

    float fade_ = 0.f;
    float worldAccum_ = 0.f;
    float autosaveTimer_ = 20.f;
};

}  // namespace sb
