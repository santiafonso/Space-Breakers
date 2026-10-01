#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "platform/Audio.hpp"
#include "platform/Window.hpp"
#include "progression/Events.hpp"
#include "progression/GameData.hpp"
#include "progression/Offers.hpp"
#include "render/Effects.hpp"
#include "sim/World.hpp"
#include "ui/Hud.hpp"
#include "ui/Screen.hpp"

namespace sb {

enum class ScreenId { Menu, Loadout, Play, Choice, Pause, Stats, HowTo, BossWin, Map, Shop, Equip, Dev, Creed, Sound, AbilityPick, Altar, Event };

// Who opened the ball / slot picker, and so what confirming it does.
// ShopForge / Sell are the shop's paid forge and its "sell an item" counter.
enum class EquipSource { Choice, Shop, Forge, ShopForge, Sell };

// Where a creed choice comes from: the run start ("Covenant") or the act-1 boss.
enum class CreedSource { Start, Boss };

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
    bool hasFocus() { return window_.handle().hasFocus(); }
    void useUiZoom(float k) { window_.useUiZoom(k); }
    sf::Vector2f uiMouse() const {   // pointer in UI units, any screen (photo mode can pin it)
        return snapMouseOn_ ? snapMouse_ : window_.uiMousePosition();
    }
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
    int choiceCount() const { return choiceCount_; }   // cards on the table (an Elite deals 3)

    void openLoadout();     // Menu -> the game menu
    void newRun();          // Loadout "Start" -> a fresh run (drops a saved one)
    bool hasSavedRun() const;   // a run in progress on disk (saves/run.txt)
    bool resumeRun();           // Menu "Continue run": back on its map; false if it couldn't be read
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

    // ---- TAB peek: drag a slot onto another ball / slot ----
    // Where slot (fromBall, fromSlot) lands when dropped on toBall's slot
    // toSlot (-1 = on the ball's panel: its first free slot of that kind);
    // -1 = the drop is refused. moveSlot does it (a filled target swaps).
    int slotMoveTarget(int fromBall, int fromSlot, int toBall, int toSlot) const;
    bool moveSlot(int fromBall, int fromSlot, int toBall, int toSlot);

    // ---- shop ----
    int shopPrice(UpgradeKind k) const;
    void buyShopOffer(int i);
    int repairAmount() const;           // HP a repair restores
    void leaveShop();
    // Shop extras (Fase O): a sale, a mystery box, a paid reroll, selling an
    // item back and the forge as a paid service.
    int shopOfferPrice(int i) const;    // what offer i costs right now (sale / prepaid applied)
    int shopRerollPrice() const;
    int shopFreeRerolls() const;        // free rerolls this visit still has ("Merchant": 1 per level)
    int shopSellsLeft() const;          // items this visit may still buy back (1, +1 per "Haggler" level)
    void rerollShop();
    int saleOffPercent() const;
    void beginSell();
    int sellValue(int ball, int slot) const;
    int forgeCap() const;               // max forge level ("Duet" raises it)
    void rerollChoice(int idx);   // Choice: swap card `idx` for another item (costs a Foresight charge)
    void repairCoreSkipItem();    // Choice: heal the core to full instead of taking an item
    void skipChoice();            // Choice: take nothing
    int ballCap() const;          // balls the arena holds now (more after act 2's boss; "Duet": two)
    // F, Repulse (from act 2): the core shoves nearby enemies away.
    bool repulseOpen() const;
    float repulseCooldown() const { return repulseCd_; }
    void repulse();
    // "?" stop events (progression/Events.hpp): deals for gold, or walk away.
    const std::vector<EventKind>& eventDeals() const { return eventDeals_; }
    int eventGoldOf(EventKind k) const { return eventGold(k, data_.run.map.act); }
    bool eventDealOk(EventKind k) const;   // affordable and something to gain
    void takeEventDeal(int idx);
    void leaveEvent();
    void playerRepair(float amount);   // a deliberate repair: heals and ends this act's "Iron core"
    // The player's own abilities: Q = Mark (always), E = bullet time (act 2 on, held).
    void playerMark();   // Q: the volley
    float markCooldown() const { return markCd_; }
    bool bulletOpen() const;
    void setBulletHeld(bool on) { bulletHeld_ = on; }
    float bulletGauge() const { return bulletGauge_; }   // 0..1
    bool bulletActive() const { return bulletOn_; }
    void leaveBossWin();    // BossWin card "Back to menu" -> game menu (banks the run)
    void continuePastBoss();  // BossWin card "Continue" -> resume at wave 11
    bool bossWinCanContinue() const;  // true when the BossWin card should offer "Continue"

    // ---- creeds (Fase O) ----
    const std::vector<CreedId>& creedChoices() const { return creedChoices_; }
    CreedSource creedSource() const { return creedSrc_; }
    void chooseCreed(int idx);          // Creed screen: take card idx
    void refuseCreeds();                // Creed screen: turn them all down for gold
    bool hasCreed(CreedId id) const { return data_.run.hasCreed(id); }
    int luck() const;   // the run's luck, in points (Lucky clover, Lucky star, Loaded Dice, Lucky charm)
    bool canGrab() const;              // "Hunters" / "Clockwork" take the balls out of your hands
    float flingPower() const;          // throw speed multiplier (Strong arm, Hot Hands, Pinball)
    void useCreedAbility();             // "Nova" (SPACE / right-click in a fight)
    // ---- pacts (2026-09-28, core/AppPacts.cpp) ----
    const std::vector<PactId>& pactChoices() const { return pactChoices_; }
    void choosePact(int idx);           // Altar screen: take card idx
    void refusePacts();                 // Altar screen: walk away
    bool hasPact(PactId id) const { return data_.run.hasPact(id); }
    bool aimSlows() const { return !hasPact(PactId::HeavyArm); }   // "Heavy Arm": no bullet time
    bool aimGuide() const { return !hasPact(PactId::Blind); }      // "Blind": no dotted guide
    float quickThrowMul() const;        // "Stillness": weaker quick throws
    bool consumeAltarReveal();          // the map: play the "a path opens" animation once
    void devTogglePact(PactId id);      // SB_DEV: grant it (or drop the rule)
    float novaCooldown() const { return novaCd_; }
    std::string choiceTitle() const;   // Choice screen heading
    // ---- "Calling": the starting ball's class, picked in the run intro ----
    const std::vector<UpgradeKind>& abilityChoices() const { return abilityChoices_; }
    void chooseAbility(int idx);       // AbilityPick screen: take card idx
    void chooseAbilityCard(int idx);   // ...give ball 0 that ability (no screen change)
    void abandonRun();
    void wipeSave();        // Game menu "Reset progress" -> erase all saved data

    // ---- dev tools: enabled by the SB_DEV env var, no-ops otherwise -----
    bool devMode() const;
    void devWinWave();
    void devGrantCurrency();   // top cores + prisms up to a huge pile (game-menu web testing)
    void devHealCore();
    void devToggleInvuln();
    void devAddBall();
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
    enum class DevOpen { Shop, Forge, Upgrade, Elite, BossTreasure, Recruit, JumpToBoss, CreedBoss, CreedStart, AbilityPick, PostFight, Altar };
    void devOpen(DevOpen what);
    void devToggleCreed(CreedId id);   // grant it (or drop it, if the run has it)
    void openPause();
    void openStats();
    void openSound();       // the Sound settings screen (main menu / pause)
    void openHowTo();
    void back();
    void quit();

    void buyMetaUnlock(int unlock);
    // SB_DEV on the web: F1 = +1000 cores and +10 prisms, F2 = lock every node
    // again and pay back what it cost (to test unlocking from scratch).
    void devGiveCurrency();
    void devRelockWeb();
    void devUnlockWeb();   // F3: every node to its max level, free
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
    // Where a pick comes from, which decides what it may be:
    //  Normal (Upgrade / Recruit nodes): anything but items;
    //  PostFight (after a plain fight): modifiers only;
    //  Elite: items only; Boss (treasure): anything; Shop: anything but a ball.
    // A new ball is always a long shot (cfg::run::newBallCardWeight).
    enum class RollSource { Normal, Elite, Boss, Shop, PostFight };
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
    // Run intro (newRun): Covenant creed -> Calling class pick -> Quartermaster starter pick -> map.
    void advanceRunIntro();
    bool openStarterChoice();
    std::vector<UpgradeKind> starterPool(Tier want);          // Starter kit candidates, nearest tier first
    void postFight();
    void afterFightPick();   // an Elite's pick, else the map
    bool openAbilityChoice();                                 // the run's first ability (false = nothing to pick)
    void grantMageMissiles();                                 // a Mage ball gets Magic missile in a free slot
    // Creeds.
    void foldCreeds(WorldParams& p) const;  // the run's creeds into the sim params
    bool openCreedChoice(CreedSource src);   // false = nothing to offer (caller moves on)
    float coinRadius(int comboTier) const;   // a kill's gold coin (UI px)
    void continueAfterCreed();
    void grantCreed(CreedId id);
    // Pacts.
    void foldPacts(WorldParams& p) const;
    bool openPactChoice(bool fromMap = true);   // false = nothing to offer (caller moves on)
    void grantPact(PactId id);
    void notePactFight(bool flawless);  // the hidden Altar path's streak
    void revealAltarPath();             // on the pre-boss row with the streak: the hidden Altar appears
    int colossusBall() const;           // "Colossus": the most built-up ball (-1 = none)
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
    struct KnownClasses { RoleMask roles = 0; ItemTag ascended = ItemTag::None; Element element = Element::Plain; };
    std::vector<KnownClasses> knownClasses_;
    void rememberClasses();   // take the current loadout as the baseline (run start)
    void announceClassGains();
    void bankRun(bool won);   // pay out cores/prisms/stats for the run; no navigation
    void finishToMenu();      // clear the run and go back to the game menu

    void handleEvent(const sf::Event& e);
    void update(float frameDt);
    void render();
    // Dev "photo mode" (SB_SNAPSHOT=<dir>): stage every screen, save a PNG of
    // each and quit - a way to look at the UI without playing.
    int runSnapshots(const std::string& dir);
    void snapFrame(const std::string& file);
    std::string capturePath_;   // non-empty: render() saves this frame here
    bool snapMouseOn_ = false;  // photo mode: uiMouse() reports snapMouse_ (a drag in the TAB peek)
    sf::Vector2f snapMouse_;
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
    std::string runPath() const;   // the run in progress: run.txt beside the save
    void saveRunNow();             // write it (the map just opened)

    std::vector<std::unique_ptr<Screen>> stack_;
    TabPeek peek_;   // TAB over every run screen that doesn't run its own (shop, cards, pickers...)
    bool onPauseMenus() const;   // pause / stats / how-to / options / dev: they own Esc
    bool onOptions() const;                  // the Options (sound) screen is on top
    std::array<UpgradeKind, kChoiceCount> choices_{};
    int choiceCount_ = 3;   // cfg::run::choiceCards until a roll sets it
    int lastRunWave_ = 0;
    int lastRunCores_ = 0;
    int lastRunPrisms_ = 0;
    bool lastRunWon_ = false;
    int bossFlawlessGold_ = 0;       // set when a boss falls, for the BossWin card
    int bossIronCores_ = 0;
    bool continueUnlocked_ = false;  // snapshot at newRun: has a run ever been won before?
    bool runBanked_ = false;         // this run's cores/prisms have been paid out
    int devBall_ = 0;              // dev panel: which ball grants go to
    float devTimeScale_ = 1.f;     // dev panel: simulation speed
    EquipSource equipSrc_ = EquipSource::Choice;
    UpgradeKind equipKind_ = UpgradeKind::AddBall;
    int equipRef_ = -1;       // Choice card / shop offer being placed
    RollSource rollSource_ = RollSource::Normal;   // what the current Choice was rolled from (rerolls keep it)
    std::string choiceTitle_;                      // custom Choice heading ("Starter kit ..."), empty = default
    std::vector<CreedId> creedChoices_;
    std::vector<PactId> pactChoices_;
    bool pactFromMap_ = true;   // the Altar was a map stop: closing it goes back to the map
    std::vector<UpgradeKind> abilityChoices_;   // the first-ability pick's cards
    CreedSource creedSrc_ = CreedSource::Boss;
    bool abilityAfterFight_ = false;   // the ability pick came from postFight: its pick follows
    int introStep_ = -1;      // >= 0 while the run intro (creed / starter pick) is still running
    float novaCd_ = 0.f;      // "Nova" creed cooldown (s)
    float repulseCd_ = 0.f;   // F cooldown (s)
    float markCd_ = 0.f;      // Q cooldown (s)
    float bulletGauge_ = 1.f; // E gauge, 0..1
    bool bulletHeld_ = false; // E is down (the Play screen polls it)
    bool bulletOn_ = false;   // ...and the slow motion is running
    float bulletIdle_ = 0.f;  // real s since it last ran (refill waits a moment)
    std::vector<EventKind> eventDeals_;   // the "?" event on screen
    bool openEvent();         // a "?" stop rolled an event: false = nothing to offer
    bool openForgePicker();   // the Forge's item picker: false = nothing to level up
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
