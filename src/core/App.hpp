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

enum class ScreenId { Menu, Loadout, Play, Choice, Pause, Stats, HowTo, BossWin, Map, Shop, Equip, Dev, Pact, Sound, ClassPick };

// Who opened the ball / slot picker, and so what confirming it does.
// ShopForge / Sell are the shop's paid forge and its "sell an item" counter.
enum class EquipSource { Choice, Shop, Forge, ShopForge, Sell };

// Where a pact choice comes from: the run start ("Covenant") or the act-1 boss.
enum class PactSource { Start, Boss };

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
    int bossFlawlessGold() const { return bossFlawlessGold_; }   // BossWin card: flawless boss bonus (0 = none)
    int bossIronCores() const { return bossIronCores_; }         // BossWin card: "Iron core" cores (0 = none)
    bool ironCoreAlive() const { return data_.run.active && !data_.run.repairedThisAct; }
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
    // Shop extras (Fase O): a sale, a mystery box, a paid reroll, selling an
    // item back and the forge as a paid service.
    int shopOfferPrice(int i) const;    // what offer i costs right now (sale / prepaid applied)
    int mysteryPrice() const;
    void buyMystery();
    int shopRerollPrice() const;
    void rerollShop();
    int saleOffPercent() const;
    void beginShopForge();
    void beginSell();
    int sellValue(int ball, int slot) const;
    int forgeCap() const;               // max forge level ("Duet" raises it)
    void rerollChoice(int idx);   // Choice: swap card `idx` for another item (costs a Foresight charge)
    void repairCoreSkipItem();    // Choice: heal the core to full instead of taking an item
    void playerRepair(float amount);   // a deliberate repair: heals and ends this act's "Iron core"
    void useReserve();            // Play: fire the "Stockpile" reserve power-up (key Q)
    void leaveBossWin();    // BossWin card "Back to menu" -> game menu (banks the run)
    void continuePastBoss();  // BossWin card "Continue" -> resume at wave 11
    bool bossWinCanContinue() const;  // true when the BossWin card should offer "Continue"

    // ---- pacts (Fase O) ----
    const std::vector<PactId>& pactChoices() const { return pactChoices_; }
    PactSource pactSource() const { return pactSrc_; }
    void choosePact(int idx);          // Pact screen: take card idx
    void refusePacts();                // Pact screen: turn them all down for gold
    bool hasPact(PactId id) const { return data_.run.hasPact(id); }
    int luck() const;   // the run's luck, in points (Lucky clover, Lucky star, Loaded Dice, Lucky charm)
    bool canGrab() const;              // "Hunters" / "Clockwork" take the balls out of your hands
    float flingPower() const;          // throw speed multiplier (Strong arm, Hot Hands, Pinball)
    void usePactAbility();             // "Nova" (SPACE / right-click in a fight)
    float novaCooldown() const { return novaCd_; }
    std::string choiceTitle() const;   // Choice screen heading
    // ---- "Calling": the starting ball's class, picked in the run intro ----
    const std::vector<ItemTag>& classChoices() const { return classChoices_; }
    void chooseClass(int idx);         // ClassPick screen: take card idx
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
    // ---- the F1 dev panel (SB_DEV only) ----
    void devOpenPanel();
    void devGrant(UpgradeKind k);            // onto devBall()
    void devClearBall();                     // strip devBall() back to a plain ball
    void devSpawn(EnemyKind k, int n);
    void devKillAll();
    void devGold(int n);
    void devSetTimeScale(float s) { devTimeScale_ = s; }
    float devTimeScale() const { return devTimeScale_; }
    int devBall() const { return std::min(devBall_, std::max(0, runBallCount() - 1)); }
    void devSetBall(int b) { devBall_ = b; }
    enum class DevOpen { Shop, Forge, Upgrade, Elite, BossTreasure, Recruit, JumpToBoss, PactBoss, PactStart, ClassPick };
    void devOpen(DevOpen what);
    void devTogglePact(PactId id);   // grant it (or drop it, if the run has it)
    void openPause();
    void openStats();
    void openSound();       // the Sound settings screen (main menu / pause)
    void openHowTo();
    void back();
    void quit();

    void buyMetaUnlock(int unlock);
    void toggleSound();
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
    // Where a set of cards comes from - it sets the tier odds.
    enum class RollSource { Normal, Elite, Boss };
    void openChoice(RollSource src = RollSource::Normal);
    // One random eligible pick: rolls a tier by the source's odds (Lucky clover
    // shifts them up), then a pick of that tier not in `exclude`. `filter`
    // narrows the pool (e.g. one item tag).
    UpgradeKind rollPick(RollSource src, const std::vector<UpgradeKind>& exclude,
                         bool (*filter)(UpgradeKind) = nullptr);
    void rollShop();
    UpgradeCtx buildUpgradeCtx() const;   // shared by rollChoices / rerollChoice
    void rollChoices(RollSource src);
    void rollRecruitChoices();   // Recruit node: new ball + the three roles
    void applyUpgradeKind(UpgradeKind k, int ball = -1, int slot = -1);
    bool autoTarget(UpgradeKind k, int& ball, int& slot) const;   // first ball / free slot it fits
    void finishChoice();                                          // after a pick: fx, close, next wave
    void afterChoice();       // a Choice closed: back to the map, or on with the run intro
    // Run intro (newRun): Covenant pact -> Calling class pick -> Quartermaster starter pick -> map.
    void advanceRunIntro();
    bool openStarterChoice();
    std::vector<UpgradeKind> starterPool(Tier want);          // Starter kit candidates, nearest tier first
    bool openClassChoice();                                   // "Calling" (false = nothing to pick)
    std::vector<UpgradeKind> startClassPool(ItemTag t) const; // items that could start a class-t ball
    void grantStartClass(ItemTag t);                          // ball 0 takes items of class t
    // Pacts.
    void foldPacts(WorldParams& p) const;  // the run's pacts into the sim params
    bool openPactChoice(PactSource src);   // false = nothing to offer (caller moves on)
    void continueAfterPact();
    void grantPact(PactId id);
    void applyDuet();
    void applyLegion();
    int randomItemFor(const BallLoadout& b, Tier maxTier);   // a random unlocked item that fits (-1 none)
    void rollShopOffers();                                           // fresh stock (keeps prepaid reveals)
    BallSpec ballSpec(const BallLoadout& b) const;
    std::vector<BallSpec> ballSpecs() const;
    void syncWorldBalls();   // after ANY loadout change: refreshes the balls and announces class gains
    // Class-gain feedback (one central place): compare every ball's classes
    // with what they were at the last sync; a new class, a second class or
    // the ascended form gets a banner, a sound and a flare on the ball.
    struct KnownClasses { RoleMask roles = 0; ItemTag ascended = ItemTag::None; };
    std::vector<KnownClasses> knownClasses_;
    void rememberClasses();   // take the current loadout as the baseline (run start)
    void announceClassGains();
    void bankRun(bool won);   // pay out cores/prisms/stats for the run; no navigation
    void finishToMenu();      // clear the run and go back to the game menu

    void handleEvent(const sf::Event& e);
    void update(float frameDt);
    void render();
    void drawDevOverlay(sf::RenderWindow& w) const;   // dev key cheat-sheet, always top-right in SB_DEV
    // Dev "photo mode" (SB_SNAPSHOT=<dir>): stage every screen, save a PNG of
    // each and quit - a way to look at the UI without playing.
    int runSnapshots(const std::string& dir);
    void snapFrame(const std::string& file);
    std::string capturePath_;   // non-empty: render() saves this frame here
    void processEvents(const FrameEvents& ev);
    sf::Vector2f worldToUi(sf::Vector2f p) const;   // arena point -> UI units (for coins / labels)
    sf::Vector2f goldCounterPos() const;            // where kill coins fly to (HUD gold)
    void flushMultiKill();                          // pay out a finished kill burst

    Window window_;
    sf::Font font_;
    sf::Font titleFont_;   // heavy face for titles / headings (see setTitleFont)
    Audio audio_;
    World world_;
    Effects effects_;
    Hud hud_;

    GameData data_;
    Rng rng_;
    std::string savePath_;  // set in the ctor: <exe dir>/saves/save.txt

    std::vector<std::unique_ptr<Screen>> stack_;
    TabPeek peek_;   // TAB over every run screen that doesn't run its own (shop, cards, pickers...)
    bool onOptions() const;                  // the Options (sound) screen is on top
    sf::FloatRect optionsButton() const;     // the corner [O] OPTIONS button, UI units
    void drawOptionsButton(sf::RenderWindow& w) const;
    std::array<UpgradeKind, kChoiceCount> choices_{};
    int lastRunWave_ = 0;
    int lastRunCores_ = 0;
    int lastRunPrisms_ = 0;
    bool lastRunWon_ = false;
    int bossFlawlessGold_ = 0;       // set when a boss falls, for the BossWin card
    int bossIronCores_ = 0;
    bool continueUnlocked_ = false;  // snapshot at newRun: has a run ever been won before?
    bool runBanked_ = false;         // this run's cores/prisms have been paid out
    int devGrantNext_ = 0;
    int devBall_ = 0;              // dev panel: which ball grants go to
    float devTimeScale_ = 1.f;     // dev panel: simulation speed
    EquipSource equipSrc_ = EquipSource::Choice;
    UpgradeKind equipKind_ = UpgradeKind::AddBall;
    int equipRef_ = -1;       // Choice card / shop offer being placed
    RollSource rollSource_ = RollSource::Normal;   // what the current Choice was rolled from (rerolls keep it)
    std::string choiceTitle_;                      // custom Choice heading ("Starter kit ..."), empty = default
    std::vector<PactId> pactChoices_;
    std::vector<ItemTag> classChoices_;   // "Calling": the classes on offer
    PactSource pactSrc_ = PactSource::Boss;
    int introStep_ = -1;      // >= 0 while the run intro (pact / starter pick) is still running
    float novaCd_ = 0.f;      // "Nova" pact cooldown (s)
public:
    bool choiceIsBossTreasure() const { return rollSource_ == RollSource::Boss; }
private:

    float fade_ = 0.f;
    float waveIntro_ = 0.f;   // >0 while a new wave eases in (sim runs slow -> full)
    float worldAccum_ = 0.f;
    float autosaveTimer_ = 20.f;
    float hitstop_ = 0.f;     // >0 freezes the simulation for a beat after an impact
    float heat_ = 0.f;        // 0..1 backdrop warmth, follows the damage combo
    bool aiming_ = false;
    float aimT_ = 0.f;        // real seconds spent aiming (slow-mo runs out)
    float lowCoreCd_ = 0.f;   // time to the next low-core warning cue
    float reactLabelCd_ = 0.f;   // spacing between reaction name labels
    int multiKillN_ = 0;      // kills in the current quick burst
    float multiKillT_ = 0.f;  // time left for the burst to keep chaining
    sf::Vector2f multiKillPos_{0.f, 0.f};   // UI position of the burst's last kill

    sf::Vector2f camSize_{1280.f, 800.f};
    sf::Vector2f camCenter_{640.f, 400.f};
    float camKick_ = 0.f;             // camera-shake magnitude, decays out after a hit
    sf::Vector2f camShake_{0.f, 0.f}; // this frame's shake offset
};

}  // namespace sb
