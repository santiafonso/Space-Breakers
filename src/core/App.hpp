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

enum class ScreenId { Menu, Loadout, Play, Choice, Pause, Stats, HowTo, BossWin };

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
    void useWorldView() { window_.useWorldView(); }
    void useUiView() { window_.useUiView(); }
    WorldParams params() const;

    int runBallCount() const { return static_cast<int>(data_.run.balls.size()); }
    int rerollsLeft() const { return data_.run.rerollsLeft; }
    int lastRunWave() const { return lastRunWave_; }
    int lastRunCores() const { return lastRunCores_; }
    int lastRunPrisms() const { return lastRunPrisms_; }
    bool lastRunWon() const { return lastRunWon_; }
    const std::array<UpgradeKind, kChoiceCount>& choices() const { return choices_; }

    void openLoadout();     // Menu -> the game menu
    void newRun();          // Loadout "Start" -> a fresh run
    void applyUpgrade(int idx);   // Choice: pick one of the four
    void rerollChoice(int idx);   // Choice: swap card `idx` for another item (costs a Foresight charge)
    void repairCoreSkipItem();    // Choice: heal the core to full instead of taking an item
    void useReserve();            // Play: fire the "Stockpile" reserve power-up (key Q)
    void leaveBossWin();    // BossWin card "Back to menu" -> game menu (banks the run)
    void continuePastBoss();  // BossWin card "Continue" -> resume at wave 11
    bool bossWinCanContinue() const;  // true when the BossWin card should offer "Continue"
    void abandonRun();
    void wipeSave();        // Game menu "Reset progress" -> erase all saved data

    // ---- dev tools: enabled by the SB_DEV env var, no-ops otherwise -----
    bool devMode() const;
    void devWinWave();
    void devGrantCores(int n);
    void devGrantCurrency();   // top cores + prisms up to a huge pile (game-menu web testing)
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
    unsigned powerUpMask() const;
    void startNextWave();
    void openChoice();
    UpgradeCtx buildUpgradeCtx() const;   // shared by rollChoices / rerollChoice
    void rollChoices();
    void applyUpgradeKind(UpgradeKind k);
    void bankRun(bool won);   // pay out cores/prisms/stats for the run; no navigation
    void finishToMenu();      // clear the run and go back to the game menu

    void handleEvent(const sf::Event& e);
    void update(float frameDt);
    void render();
    void drawDevOverlay(sf::RenderWindow& w) const;   // dev key cheat-sheet, always top-right in SB_DEV
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
    int lastRunPrisms_ = 0;
    bool lastRunWon_ = false;
    bool continueUnlocked_ = false;  // snapshot at newRun: has a run ever been won before?
    bool runBanked_ = false;         // this run's cores/prisms have been paid out
    int devGrantNext_ = 0;

    float fade_ = 0.f;
    float waveIntro_ = 0.f;   // >0 while a new wave eases in (sim runs slow -> full)
    float worldAccum_ = 0.f;
    float autosaveTimer_ = 20.f;
    float hitstop_ = 0.f;     // >0 freezes the simulation for a beat after an impact

    sf::Vector2f camSize_{1280.f, 800.f};
    sf::Vector2f camCenter_{640.f, 400.f};
    float camKick_ = 0.f;             // camera-shake magnitude, decays out after a hit
    sf::Vector2f camShake_{0.f, 0.f}; // this frame's shake offset
};

}  // namespace sb
