#pragma once

#include <optional>
#include <vector>

#include "core/Config.hpp"
#include "sim/Entities.hpp"

namespace sb {

enum class Grabbed { None, Ball };

class World;

// A class's simulation hooks (sim/WorldClasses.cpp): one specialisation per
// class, each a set of static functions that get the World (they're friends,
// so they reach everything a member would). The defaults do nothing, so a
// class only writes the hooks it needs.
struct NoClassHooks {
    // Every step, for every ball (and ghost) with the class, before it moves.
    static void tick(World&, Ball&, float /*dt*/, const WorldParams&, FrameEvents&) {}
    // It landed a hit on an enemy (after damage, statuses and procs).
    static void onHit(World&, Ball&, Enemy&, float /*dmg*/, bool /*kill*/, const WorldParams&, FrameEvents&) {}
    // Its hit killed an enemy (after the on-kill items).
    static void onKill(World&, Ball&, Enemy&, float /*dmg*/, const WorldParams&, FrameEvents&) {}
    static void onWallBounce(World&, Ball&, sf::Vector2f /*normal*/, const WorldParams&, FrameEvents&) {}
    static void onCoreBounce(World&, Ball&, sf::Vector2f /*normal*/, const WorldParams&, FrameEvents&) {}
    // Contact-damage multiplier for a ball with the class (World::ballDamage).
    static float damageMul(const World&, const Ball&, const WorldParams&) { return 1.f; }
    // Once per step, whatever the balls are: the class's own world state
    // (bullets, turrets, summons...) in ClassWorldState.
    static void worldTick(World&, float /*dt*/, const WorldParams&, FrameEvents&) {}
    // A new wave starts (the balls carry over; ghosts are gone).
    static void waveStart(World&, const WorldParams&) {}
    // You picked it up / threw it (a real throw, click or slingshot).
    static void onGrab(World&, Ball&) {}
    static void onThrow(World&, Ball&) {}
    // Right before one of its hits lands on `e`: a multiplier for that hit
    // (it may roll or spend state - it runs once per hit).
    static float preHit(World&, Ball&, const Enemy&, const WorldParams&) { return 1.f; }
};
template <BallRole R> struct ClassHooks;

// The simulation: elemental balls orbiting a central core, clearing waves of
// enemies that march on it. Knows nothing about rendering, input or progression
// storage.
class World {
public:
    explicit World(sf::Vector2f size);

    // ---- run / wave lifecycle -----------------------------------------
    void startRun(const WorldParams& p, const std::vector<BallSpec>& balls,
                  float coreHp, float coreMaxHp);
    // elite: an Elite map node - tougher and more enemies (cfg::map).
    void startWave(int wave, const WorldParams& p, bool elite = false);
    void startBossWave(int wave, const WorldParams& p);   // an act's boss (wave 10, 20, ... 50)
    void startPostBossWave(int wave, const WorldParams& p, bool elite = false);  // past act 1: wide arena, core slides to centre
    // Match the balls to the run loadout: refresh role / element / gear of the
    // existing ones in place (they keep flying) and spawn any new ones.
    void syncBalls(const std::vector<BallSpec>& specs, const WorldParams& p);
    // Ball `idx` just gained a class (or its ascended form): it flares in its
    // class colour the next time the fight runs (Ball::classPulse).
    void pulseClass(int idx, bool ascended);
    void pulseElement(int idx);   // it just took an element: rings in its colour when the fight runs
    void repairCore(float amount);
    void addCoreMaxHp(float delta);                  // raise the core's max HP mid-run

    // ---- creeds (sim/WorldCreeds.cpp) -----------------------------------
    void trimBalls(int n);                     // "Duet": drop every ball past the first n
    void creedNova(const WorldParams& p);       // "Nova": burst every ball out of the core in a ring
    // "Hunters": each ball's current prey (valid where huntHas()[i] is set).
    const std::vector<sf::Vector2f>& huntPrey() const { return huntPrey_; }
    const std::vector<char>& huntHas() const { return huntHas_; }

    // ---- dev tools (no-ops unless the caller is in dev mode) ---------
    void devWinWave();                    // clear the current wave now
    void devSpawn(EnemyKind k, int n);    // drop n enemies of a kind in from the edges
    void devKillAll();                    // everything on the field dies (counts as kills)
    void devSetInvuln(bool on) { invuln_ = on; }
    void setPhoenix(int charges) { phoenixLeft_ = charges; }   // "Phoenix" relic: saves left this act
    bool devInvuln() const { return invuln_; }

    FrameEvents step(float dt, const WorldParams& p);

    // ---- grab / throw: knock a ball off its orbit -------------------
    bool grabAt(sf::Vector2f point, float catchRadius);
    void setPointer(sf::Vector2f p) { pointer_ = p; hasPointer_ = true; }   // the play screen, every frame
    float heldCatch() const { return heldCatch_; }   // the grab just made: its catch reward (0 = none)
    bool hasHeld() const { return grabbed_ != Grabbed::None; }
    Grabbed grabbedKind() const { return grabbed_; }
    void moveHeld(sf::Vector2f target, float dt);
    void releaseHeld(sf::Vector2f throwVel);
    void cancelHeld();                 // let go without a throw: the ball resumes its old velocity
    int repulse();   // F: shove every enemy near the core away and stagger it; how many it hit
    // The player's target: click an enemy (or the boss) and every throw, ability,
    // dash and automatic aim goes for it while it lives (nearestTarget, Dash,
    // missiles, Shooter, Assassin, Summoner, Slinger, Seeker, Hunter, Clockwork,
    // aimed bounces). False = nothing under the point; true toggles it.
    bool focusAt(sf::Vector2f point);
    std::optional<sf::Vector2f> focusPos() const;
    const Enemy* focusEnemy() const;   // the target if it's a live enemy (not the boss)
    bool focusIsBoss() const { return focusBoss_ && focusPos().has_value(); }
    // Q, Volley: every free ball is thrown at the live enemy (or boss) nearest
    // to it, at `speed`. False = nothing to aim at.
    bool volley(float speed);
    // Quick throw: the live enemy (or the boss) nearest to a point, if any.
    std::optional<sf::Vector2f> nearestTarget(sf::Vector2f from) const;
    const Ball* heldBall() const {
        return grabbed_ == Grabbed::Ball ? &balls_[static_cast<std::size_t>(heldIndex_)] : nullptr;
    }
    void forceRelease();

    // ---- read-only views --------------------------------------------
    const std::vector<Ball>& balls() const { return balls_; }
    const std::vector<Ball>& ghosts() const { return ghosts_; }   // "Split shot" copies
    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<Bolt>& bolts() const { return bolts_; }
    const std::vector<Obstacle>& obstacles() const { return obstacles_; }
    const std::vector<Pool>& pools() const { return pools_; }
    const std::vector<Wave>& waves() const { return waves_; }
    const std::vector<BlackHole>& blackHoles() const { return blackHoles_; }   // "Black hole"
    const ClassWorldState& classWorld() const { return classWorld_; }          // per-class world state (bullets, summons...)
    const std::vector<TetherBeam>& tethers() const { return tethers_; }        // "Tether" lasers, this step
    const std::vector<Pickup>& pickups() const { return pickups_; }
    const Core& core() const { return core_; }
    const Boss& boss() const { return boss_; }
    const std::optional<ActiveEffect>& effect() const { return effect_; }
    bool coreCleanWave() const { return !coreHitThisWave_; } // "Interest": no core damage this wave
    sf::Vector2f size() const { return size_; }

    // Area the camera should frame (grows for the boss wave).
    sf::Vector2f viewSize() const { return size_; }
    // How much bigger the current arena is than the normal one (1 on waves
    // 1-9). Ball speeds scale by it so they look the same on screen.
    float arenaScale() const { return size_.x / baseSize_.x; }
    sf::Vector2f viewCenter() const { return size_ * 0.5f; }

    bool waveRunning() const { return waveRunning_; }
    bool launching() const { return launchT_ > 0.f; }   // the fight-opening whirl is on
    bool bossWave() const { return bossWave_; }
    bool runOver() const { return runOver_; }
    int wave() const { return wave_; }
    // What's left of the fight for the HUD: still to come + alive, not
    // counting a Splitter's shards (it counts as one, done once it splits -
    // so the count never climbs back up).
    int enemiesLeft() const {
        int n = toSpawn_;
        for (const Enemy& e : enemies_) n += e.kind == EnemyKind::Shard ? 0 : 1;
        return n;
    }

    float cruiseBase(const WorldParams& p) const;
    // A ball's body colour at a speed: its lead class's hue (richer ascended),
    // or the neutral speed grey without a class. The element isn't in it.
    sf::Color ballTint(const Ball& b, float speed, const WorldParams& p) const;
    float cruiseSpeed(const WorldParams& p) const;
    int comboStreak() const { return comboStreak_; }
    int comboTier() const {
        return std::min(comboStreak_ / cfg::combo::bouncesPerTier, comboCapTier_);
    }
    float comboMultiplier() const {
        return 1.f + static_cast<float>(comboTier()) * cfg::combo::multiplierPerTier;
    }
    float fastestBall() const;

private:
    template <BallRole R> friend struct ClassHooks;

    void spawnBall(const BallSpec& spec, const WorldParams& p);
    void applySpec(Ball& b, const BallSpec& spec);   // roles / element / gear / abilities onto a ball
    float ballBaseCruise(const Ball& b, const WorldParams& p) const;   // role-scaled cruise, no buffs
    float ballCruise(const Ball& b, const WorldParams& p) const;       // + Surge / Warm-up
    float ballMaxSpeed(const Ball& b, const WorldParams& p) const;
    float ballRadius(const Ball& b, const WorldParams& p) const;
    float elemPotency(const Ball& b, const WorldParams& p) const;      // web level x Support bonus
    void boostSpeed(Ball& b, float mult, const WorldParams& p);        // bounce boosts, capped
    void spawnEnemy(std::optional<EnemyKind> force = std::nullopt);   // force: this kind, at the wave's stats
    void spawnPack();                 // a bunch of runners from one spot (act 2+)
    void spawnOrbiter(float phase);   // one shield enemy on the Orbital's ring
    void beginWave(int wave, bool elite);          // a normal wave's spawn plan
    void startChargerWave(const WorldParams& p);   // act 1's boss: core far left
    void startFinalBossWave(const WorldParams& p); // act 5's Orbital + shield ring
    float bossHp(float grunts, int wave) const;
    float enemyHp(int wave) const;      // the wave's plain enemy HP (hard mode on top)
    float enemySpeed(int wave) const;
    void updateHive(float dt);                     // sway in, burst runners
    void updateWarden(float dt, const WorldParams& p);
    void updateDasher(float dt, const WorldParams& p);
    // A ball landed on the boss: damage (unless i-frames / too slow / the
    // Warden's shield), Dasher knockback. cdMul stretches the i-frames.
    void ballHitsBoss(Ball& b, float cdMul, const WorldParams& p, FrameEvents& ev, bool speedGate = true);
    bool wardenBlocks(sf::Vector2f from) const;
    EnemyKind rollEnemyKind();        // weighted by wave (kinds phase in) and elite
    static void setEnemyKind(Enemy& e, EnemyKind k, float hp, float speed);
    static bool shieldBlocks(const Enemy& e, sf::Vector2f from, sf::Vector2f corePos);
    void carryBalls(const WorldParams& p);   // keep balls in place across a wave change
    sf::Vector2f wideArenaSize() const;      // the pulled-back arena used from the boss wave on
    void updateCoreSlide(float dt);          // ease the core left -> centre entering wave 11
    void advanceCombo(float dt, const WorldParams& p);
    void advanceSatellite(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);   // "Satellite": orbit the core
    void fireRail(Ball& b, const WorldParams& p, FrameEvents& ev);                    // "Railgun" beam
    void updateStorm(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);       // "Storm" zaps
    void updateTwins(const WorldParams& p);                                            // "Gemini" twins
    void steerHunter(Ball& b, float dt);                                               // "Hunter": chase the prey
    void updateTethers(float dt, const WorldParams& p, FrameEvents& ev);              // "Tether" lasers
    void updateBlackHoles(float dt, const WorldParams& p, FrameEvents& ev);           // "Black hole" pull + burst
    void resonate(Ball& b, float dmg, const WorldParams& p, FrameEvents& ev);         // "Resonance" arcs
    void onKill(Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev); // on-kill items
    void updateAutoFling(float dt, const WorldParams& p, FrameEvents& ev);
    void advanceBall(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    void emitElement(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    void resolveBallPairs(FrameEvents& ev, const WorldParams& p);
    void updateBolts(float dt);
    void updateWaves(float dt, const WorldParams& p, FrameEvents& ev);
    bool inWave(const Enemy& e) const;       // under a rolling wave's crest (Electrocution)
    void updateObstacles(float dt);
    void updatePools(float dt);
    void addPool(sf::Vector2f at, float radius, float power, PoolKind kind);
    float poolSlow(const Enemy& e) const;    // mud underfoot: its time step x this
    void crack(Enemy& e, int n, float time, int cap);   // stone: n more cracks
    void updateEnemies(float dt, const WorldParams& p, FrameEvents& ev);
    void updateBoss(float dt, const WorldParams& p, FrameEvents& ev);
    void sweepDeadEnemies(FrameEvents& ev, const WorldParams& p);
    void updateWaveSpawner(float dt, FrameEvents& ev);
    void updatePickups(float dt, const WorldParams& p, FrameEvents& ev);
    void advanceEffect(float dt, const WorldParams& p);
    void activateEffect(PowerUp k, const WorldParams& p);   // start an effect (Charged applies here)
    float effStrength(const WorldParams& p) const;          // 1 while live, ramps to 0 over the Afterglow tail
    void afterBounce(Ball& b, sf::Vector2f normal, bool countHit);
    void addComboHit();   // a hit that doesn't bounce (Cleave, Comet) still feeds the combo
    // Guardian (or any ball when `force`): bounce toward the threat.
    void aimBounce(Ball& b, sf::Vector2f normal, const Enemy* skip, bool force = false);

    // ---- creed hooks (sim/WorldCreeds.cpp): each is a no-op without its creed ----
    void updateHunters(float dt, const WorldParams& p);                         // "Hunters" homing
    void updateCoreZap(float dt, const WorldParams& p, FrameEvents& ev);        // "Living Core" zaps
    void creedWallBump(Ball& b, sf::Vector2f at, const WorldParams& p, FrameEvents& ev);   // "Pinball"
    void creedClack(Ball& a, Ball& b, sf::Vector2f at, const WorldParams& p, FrameEvents& ev);  // "Legion"
    void creedCoreBounce(Ball& b, const WorldParams& p, FrameEvents& ev);
    // Speed items of the older classes (sim/WorldStyle.cpp).
    // Where an orbit around the core wants to be: the enemy closest to the core
    // (clamped to [minR, usual x orbitMax]), else its usual radius.
    float orbitTarget(float usual, float minR) const;
    float styleStill(const Ball& b, const WorldParams& p) const;   // 0 at cruise .. 1 stopped
    float styleFast(const Ball& b, const WorldParams& p) const;    // 0 at cruise .. 1 well past it
    bool styleBlurring(const Ball& b, const WorldParams& p) const; // "Blur": passing through right now
    void styleTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    void styleWorldTick(float dt, const WorldParams& p);
    void styleEnemyDrag(Enemy& e, float& edt, float dt);          // "Anchor"
    float stylePreHit(Ball& b, Enemy& e, const WorldParams& p, FrameEvents& ev);   // Beacon / Pass charge
    void styleOnHit(Ball& b, Enemy& e, const WorldParams& p);      // "Blur" marks
    void stylePass(const Ball& a, Ball& o, const WorldParams& p);  // "Pass"
    void styleOnCast(Ball& b, const WorldParams& p, FrameEvents& ev);   // "Leyline"
    // Pacts (sim/WorldPacts.cpp).
    float pactDamageMul(const Ball& b, const WorldParams& p) const;   // Lead / Stillness / Quick Hands / Juggler / Last Breath
    bool pactWrap(Ball& b);                                           // "Void Walls": true = it went through an edge
    void pactWallBump(Ball& b);                                       // "Anchor Walls"
    void pactCoreBounce(Ball& b);                                     // "Juggler": the streak ends on the core
    void pactOnGrab(Ball& b);                                         // "Frenzy" / "Juggler"
    void pactOnThrow(Ball& b);
    void rollGait(Enemy& e);                       // straight / weave / spiral (spawnEnemy)
    bool trySnare(Ball& b, Enemy& e);              // a Snare catches the ball that hit it
    void updateSnared(Ball& b, const WorldParams& p);   // hangs on its Snare, or is let go                                        // "Hot Potato" / "Mirror"
    void pactHeldTick(float dt);                                      // "Hot Potato": the ball slips        // "Living Core" overcharge
    void creedCoreHit(const WorldParams& p, FrameEvents& ev);                    // "Fortress" / "Bloodlust"

    // ---- class hooks (sim/WorldClasses.cpp): run every class the ball has ----
    void classTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    void classOnHit(Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev);
    void classOnKill(Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev);
    void classOnWallBounce(Ball& b, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev);
    void classOnCoreBounce(Ball& b, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev);
    float classDamageMul(const Ball& b, const WorldParams& p) const;
    void classWorldTick(float dt, const WorldParams& p, FrameEvents& ev);
    void classWaveStart(const WorldParams& p);
    void classOnGrab(Ball& b);
    void classOnThrow(Ball& b);
    float classPreHit(Ball& b, const Enemy& e, const WorldParams& p);

    // ---- abilities (sim/WorldAbilities.cpp) ----
    void updateAbilities(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    bool fireAbility(Ball& b, const AbilitySpec& a, const WorldParams& p, FrameEvents& ev);   // false = nothing to act on yet
    void resetAbilityCooldowns(Ball& b);   // a wave starts: every ability part-charged
    // Mage (defined in its section of sim/WorldClasses.cpp): its items act through casts.
    float mageCastRate(const Ball& b) const;   // cooldowns tick this much faster ("Focus")
    void mageOnCast(Ball& b, int slot, bool echo, const WorldParams& p, FrameEvents& ev);   // an ability just fired
    void mageTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);   // Twincast echoes
    bool mageMissiles(Ball& b, int count, float frac, const WorldParams& p);   // loose homing magic missiles (false: no target)

    // ---- hits, procs and reactions (Fase I) ----
    // One ball landing on one enemy: damage, statuses, procs, reactions.
    // Returns true if it killed it. No bounce - the caller handles that.
    bool strike(Ball& b, Enemy& e, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev,
                bool allowEcho = true);
    void damageEnemy(Enemy& e, float dmg);            // any damage source; applies "brittle"
    void areaDamage(sf::Vector2f at, float radius, float dmg, const Enemy* skip);
    void applyElement(Enemy& e, Element el, int owner, float hitDmg, const WorldParams& p, FrameEvents& ev);
    void touchElement(Enemy& e, Element el, float pot, const BallMods& m, const WorldParams& p);
    Element hitElement(const Ball& b, const WorldParams& p);   // its element, or a random one under "Prism core"
    void triggerReaction(Element x, Element y, Enemy& e, float hitDmg, const WorldParams& p,
                         FrameEvents& ev, int depth);
    bool chance(float base, const WorldParams& p);    // roll a proc through the run's luck
    void spawnGhost(const Ball& parent);
    void spawnMitosis(const Ball& parent, const WorldParams& p);
    void guardianPulse(FrameEvents& ev);
    const Enemy* nearestEnemy(sf::Vector2f from, float maxDist) const;   // live, not an orbiter; null if none
    void regulateSpeed(Ball& b, float dt, const WorldParams& p);
    void updateTrail(Ball& b);
    float ballDamage(const Ball& b, const WorldParams& p) const;

    sf::Vector2f baseSize_;   // the normal arena
    sf::Vector2f size_;       // current arena (== baseSize_ except on the boss wave)
    std::vector<Ball> balls_;
    std::vector<Ball> ghosts_;          // "Split shot" copies (never synced to the loadout)
    std::vector<Ball> pendingGhosts_;   // spawned mid-step, added after the ball loop
    std::vector<Enemy> enemies_;
    std::vector<Bolt> bolts_;
    std::vector<Obstacle> obstacles_;
    std::vector<Pool> pools_;                // stone reactions: lava, mud, toxic dust
    std::vector<Wave> waves_;                // water balls' waves
    std::vector<BlackHole> blackHoles_;
    std::vector<TetherBeam> tethers_;
    ClassWorldState classWorld_;        // per-class world state (sim/Classes.hpp)
    std::vector<Pickup> pickups_;
    Core core_;
    Boss boss_;
    std::optional<ActiveEffect> effect_;
    Rng rng_;

    Grabbed grabbed_ = Grabbed::None;
    int heldIndex_ = -1;
    sf::Vector2f heldGrabOffset_{0.f, 0.f};  // ball pos - cursor at grab, eased to zero
    sf::Vector2f heldPrevVel_{0.f, 0.f};     // velocity before the grab (cancelHeld restores it)
    int focusId_ = -1;         // the player's target: an enemy id...
    bool focusBoss_ = false;   // ...or the boss
    float heldCatch_ = 0.f;                  // catch reward earned by this grab (0..catchBonusMax)
    float heldT_ = 0.f;                      // seconds the held ball has been held ("Hot Potato")
    sf::Vector2f pointer_{0.f, 0.f};         // your pointer in the arena ("Grip")
    bool hasPointer_ = false;
    float trickLuck_ = 1.f;                  // "Trick shot": every chance x this while its hit resolves
    Ball* reactBall_ = nullptr;              // the ball whose hit is resolving (Alchemist reactions)
    PactRules pact_;                         // the run's pacts, from WorldParams (each step / wave start)
    float autoFlingTimer_ = 1.f;   // "Clockwork": time to its next throw

    int comboStreak_ = 0;
    int comboCapTier_ = cfg::combo::baseCapTier;
    float sinceHit_ = 0.f;
    int reportedTier_ = 0;

    int wave_ = 0;
    bool waveRunning_ = false;
    bool bossWave_ = false;
    bool runOver_ = false;
    float coreSlideT_ = 0.f;   // >0 while the core is easing to the wide-arena centre
    sf::Vector2f coreSlideFrom_{0.f, 0.f};
    sf::Vector2f coreSlideTo_{0.f, 0.f};
    int toSpawn_ = 0;
    float spawnTimer_ = 0.f;
    int nextEnemyId_ = 1;     // Enemy::id source
    float waveHpMul_ = 1.f;   // Elite wave: enemy HP multiplier
    float launchT_ = 0.f;                    // >0: the balls whirl around the core (cfg::ball::launch*)
    float launchAng_ = 0.f;                  // the whirl's current angle
    sf::Vector2f launchCentre_{0.f, 0.f};    // the ring's centre (the core, nudged off a wall)
    std::vector<sf::Vector2f> launchFrom_;   // where each ball was when it started (eases onto the ring)
    void updateLaunch(float dt, const WorldParams& p, FrameEvents& ev);
    void chargerShock(FrameEvents& ev);
    bool hard_ = false;       // hard mode (cfg::hard), from WorldParams at each wave start
    std::vector<int> bruteSlots_;   // toSpawn_ values at which a Brute (miniboss) spawns instead
    bool eliteWave_ = false;

    float pickupTimer_ = cfg::pickup::spawnMin;
    bool invuln_ = false;  // dev: core takes no damage
    int phoenixLeft_ = 0;  // "Phoenix": times the core can still come back this act

    int aegisChargesLeft_ = 0;      // "Aegis": hits the core still soaks this wave
    bool coreHitThisWave_ = false;  // "Interest": did anything reach the core this wave

    // creeds
    std::vector<sf::Vector2f> huntPrey_;   // "Hunters": per ball, where its prey was last seen
    std::vector<char> huntHas_;            // ...and whether it has one
    float coreZapT_ = 0.f;                 // "Living Core": time to the next zap
    int comboExtra_ = 0;                   // "Bloodlust": extra combo steps per hit (set each step)
};

}  // namespace sb
