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

enum class ScreenId { Menu, Loadout, Play, Choice, Pause, Stats, HowTo, BossWin, Map, Shop, Equip };

// Who opened the ball / slot picker, and so what confirming it does.
enum class EquipSource { Choice, Shop, Forge };

// Top-level application: owns the window, subsystems and the screen stack, runs
// the loop (fixed-step simulation, per-frame render) and wires the flow:
// Menu -> Loadout (spend cores) -> Play + Map (pick a node; fights run on the
// Play screen, shops / forges / rests / picks open over it) -> ... -> Loadout.
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
    sf::Vector2f uiMouse() const { return window_.uiMousePosition(); }   // pointer in UI units, any screen
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
    void applyUpgrade(int idx);   // Choice: take card idx (asks for a ball first when it needs one)

    // ---- path map ----
    void openMap();
    bool mapNodeOpen(int node) const;   // can you step onto this node now?
    void travelTo(int node);            // step onto it: fight, shop, forge, rest or a pick
    int gold() const { return data_.run.gold; }

    // ---- ball / slot picker (Equip screen) ----
    void beginEquip(EquipSource src, UpgradeKind k, int ref);
    EquipSource equipSource() const { return equipSrc_; }
    UpgradeKind equipKind() const { return equipKind_; }
    bool equipFitsBall(int ball) const;
    bool equipFitsSlot(int ball, int slot) const;
    void confirmEquip(int ball, int slot);   // slot -1 = default slot
    void cancelEquip();

    // ---- shop ----
    int shopPrice(UpgradeKind k) const;
    void buyShopOffer(int i);
    void buyRepair();
    int repairAmount() const;           // HP a shop repair restores
    void leaveShop();
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
    void toggleSlingshot();
    void toggleAutoFling();
    void setAiming(bool on);   // slingshot aim in progress: time slows for a moment
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
    void startWaveAt(int wave, bool elite);   // fight wave N (boss waves are picked by N)
    void openChoice();
    void rollShop();
    UpgradeCtx buildUpgradeCtx() const;   // shared by rollChoices / rerollChoice
    void rollChoices();
    void rollRecruitChoices();   // Recruit node: new ball + the three roles
    void applyUpgradeKind(UpgradeKind k, int ball = -1, int slot = -1);
    bool autoTarget(UpgradeKind k, int& ball, int& slot) const;   // first ball / free slot it fits
    void finishChoice();                                          // after a pick: fx, close, next wave
    BallSpec ballSpec(const BallLoadout& b) const;
    std::vector<BallSpec> ballSpecs() const;
    void syncWorldBalls();
    void bankRun(bool won);   // pay out cores/prisms/stats for the run; no navigation
    void finishToMenu();      // clear the run and go back to the game menu

    void handleEvent(const sf::Event& e);
    void update(float frameDt);
    void render();
    void drawDevOverlay(sf::RenderWindow& w) const;   // dev key cheat-sheet, always top-right in SB_DEV
    void processEvents(const FrameEvents& ev);
    sf::Vector2f worldToUi(sf::Vector2f p) const;   // arena point -> UI units (for coins / labels)
    sf::Vector2f goldCounterPos() const;            // where kill coins fly to (HUD gold)
    void flushMultiKill();                          // pay out a finished kill burst

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
    EquipSource equipSrc_ = EquipSource::Choice;
    UpgradeKind equipKind_ = UpgradeKind::AddBall;
    int equipRef_ = -1;       // Choice card / shop offer being placed

    float fade_ = 0.f;
    float waveIntro_ = 0.f;   // >0 while a new wave eases in (sim runs slow -> full)
    float worldAccum_ = 0.f;
    float autosaveTimer_ = 20.f;
    float hitstop_ = 0.f;     // >0 freezes the simulation for a beat after an impact
    float heat_ = 0.f;        // 0..1 backdrop warmth, follows the damage combo
    bool aiming_ = false;
    float aimT_ = 0.f;        // real seconds spent aiming (slow-mo runs out)
    int multiKillN_ = 0;      // kills in the current quick burst
    float multiKillT_ = 0.f;  // time left for the burst to keep chaining
    sf::Vector2f multiKillPos_{0.f, 0.f};   // UI position of the burst's last kill

    sf::Vector2f camSize_{1280.f, 800.f};
    sf::Vector2f camCenter_{640.f, 400.f};
    float camKick_ = 0.f;             // camera-shake magnitude, decays out after a hit
    sf::Vector2f camShake_{0.f, 0.f}; // this frame's shake offset
};

}  // namespace sb
