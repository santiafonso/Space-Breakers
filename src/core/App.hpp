#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "platform/Audio.hpp"
#include "platform/Window.hpp"
#include "progression/GameData.hpp"
#include "render/Effects.hpp"
#include "sim/World.hpp"
#include "ui/Hud.hpp"
#include "ui/Screen.hpp"

namespace sb {

enum class ScreenId { MainMenu, Play, Shop, Pause, Stats, HowTo };

// Top-level application: owns the window, subsystems and the screen stack, runs
// the loop (fixed-step simulation, per-frame render) and wires physics events to
// audio-visual feedback. Screens drive transitions through the small command
// surface in the "screen API" section.
class App {
public:
    App();
    int run();

    // ---- screen API ---------------------------------------------------------
    World& world() { return world_; }
    const World& world() const { return world_; }
    GameData& data() { return data_; }
    Audio& audio() { return audio_; }
    Effects& effects() { return effects_; }
    Hud& hud() { return hud_; }
    const sf::Font& font() const { return font_; }
    sf::Vector2f size() const { return window_.logicalSize(); }
    WorldParams params() const;
    bool hasSave() const;

    void newRun();
    void continueRun();
    void toMenu();
    void openShop();
    void openPause();
    void openStats();
    void openHowTo();
    void back();
    void quit();

    void toggleSound();
    void toggleFullscreen();
    void buyUpgrade(int upgrade);
    void save();

private:
    std::unique_ptr<Screen> makeScreen(ScreenId id);
    void replaceStack(ScreenId id);
    void push(ScreenId id);
    bool simulating() const;

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
    const std::string savePath_ = "saves/save.txt";

    std::vector<std::unique_ptr<Screen>> stack_;
    float fade_ = 0.f;
    float worldAccum_ = 0.f;
    float autosaveTimer_ = 20.f;

    std::uint32_t runStartBest_ = 0;
    bool announcedBest_ = false;
};

}  // namespace sb
