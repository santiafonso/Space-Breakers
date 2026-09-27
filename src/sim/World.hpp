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
    void startBossWave(const WorldParams& p);   // the wave-10 miniboss duel (Charger)
    void startPostBossWave(int wave, const WorldParams& p, bool elite = false);  // waves 11..19: wide arena, core slides to centre
    void startFinalBossWave(const WorldParams& p);           // wave 20: the Orbital boss + shield ring
    // Match the balls to the run loadout: refresh role / element / gear of the
    // existing ones in place (they keep flying) and spawn any new ones.
    void syncBalls(const std::vector<BallSpec>& specs, const WorldParams& p);
    void repairCore(float amount);
    void addCoreMaxHp(float delta);                  // raise the core's max HP mid-run
    void useReserve(const WorldParams& p);           // "Stockpile": fire the held reserve power-up

    // ---- pacts (sim/WorldPacts.cpp) -----------------------------------
    void trimBalls(int n);                     // "Duet": drop every ball past the first n
    void pactNova(const WorldParams& p);       // "Nova": burst every ball out of the core in a ring
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
    bool hasHeld() const { return grabbed_ != Grabbed::None; }
    Grabbed grabbedKind() const { return grabbed_; }
    void moveHeld(sf::Vector2f target, float dt);
    void releaseHeld(sf::Vector2f throwVel);
    void cancelHeld();                 // let go without a throw: the ball resumes its old velocity
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
    const std::vector<BlackHole>& blackHoles() const { return blackHoles_; }   // "Black hole"
    const ClassWorldState& classWorld() const { return classWorld_; }          // per-class world state (bullets, summons...)
    const std::vector<TetherBeam>& tethers() const { return tethers_; }        // "Tether" lasers, this step
    const std::vector<Pickup>& pickups() const { return pickups_; }
    const Core& core() const { return core_; }
    const Boss& boss() const { return boss_; }
    const std::optional<ActiveEffect>& effect() const { return effect_; }
    bool hasReserve() const { return hasReserve_; }          // "Stockpile"
    PowerUp reservePu() const { return reservePu_; }
    bool coreCleanWave() const { return !coreHitThisWave_; } // "Interest": no core damage this wave
    sf::Vector2f size() const { return size_; }

    // Area the camera should frame (grows for the boss wave).
    sf::Vector2f viewSize() const { return size_; }
    // How much bigger the current arena is than the normal one (1 on waves
    // 1-9). Ball speeds scale by it so they look the same on screen.
    float arenaScale() const { return size_.x / baseSize_.x; }
    sf::Vector2f viewCenter() const { return size_ * 0.5f; }

    bool waveRunning() const { return waveRunning_; }
    bool bossWave() const { return bossWave_; }
    bool runOver() const { return runOver_; }
    int wave() const { return wave_; }
    int enemiesLeft() const { return static_cast<int>(enemies_.size()) + toSpawn_; }

    float cruiseBase(const WorldParams& p) const;
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
    void spawnEnemy();
    void spawnOrbiter(float phase);   // one shield enemy on the wave-20 ring
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
    void updateWaterTrails(float dt, const WorldParams& p, FrameEvents& ev);
    void updateObstacles(float dt);
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

    // ---- pact hooks (sim/WorldPacts.cpp): each is a no-op without its pact ----
    void updateHunters(float dt, const WorldParams& p);                         // "Hunters" homing
    void updateCoreZap(float dt, const WorldParams& p, FrameEvents& ev);        // "Living Core" zaps
    void pactWallBump(Ball& b, sf::Vector2f at, const WorldParams& p, FrameEvents& ev);   // "Pinball"
    void pactClack(Ball& a, Ball& b, sf::Vector2f at, const WorldParams& p, FrameEvents& ev);  // "Legion"
    void pactCoreBounce(Ball& b, const WorldParams& p, FrameEvents& ev);        // "Living Core" overcharge
    void pactCoreHit(const WorldParams& p, FrameEvents& ev);                    // "Fortress" / "Bloodlust"

    // ---- class hooks (sim/WorldClasses.cpp): run every class the ball has ----
    void classTick(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    void classOnHit(Ball& b, Enemy& e, float dmg, bool kill, const WorldParams& p, FrameEvents& ev);
    void classOnKill(Ball& b, Enemy& e, float dmg, const WorldParams& p, FrameEvents& ev);
    void classOnWallBounce(Ball& b, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev);
    void classOnCoreBounce(Ball& b, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev);
    float classDamageMul(const Ball& b, const WorldParams& p) const;
    void classWorldTick(float dt, const WorldParams& p, FrameEvents& ev);
    void classWaveStart(const WorldParams& p);

    // ---- abilities (sim/WorldAbilities.cpp) ----
    void updateAbilities(Ball& b, float dt, const WorldParams& p, FrameEvents& ev);
    bool fireAbility(Ball& b, const AbilitySpec& a, const WorldParams& p, FrameEvents& ev);   // false = nothing to act on yet
    void resetAbilityCooldowns(Ball& b);   // a wave starts: every ability part-charged

    // ---- hits, procs and reactions (Fase I) ----
    // One ball landing on one enemy: damage, statuses, procs, reactions.
    // Returns true if it killed it. No bounce - the caller handles that.
    bool strike(Ball& b, Enemy& e, sf::Vector2f normal, const WorldParams& p, FrameEvents& ev,
                bool allowEcho = true);
    void damageEnemy(Enemy& e, float dmg);            // any damage source; applies "brittle"
    void areaDamage(sf::Vector2f at, float radius, float dmg, const Enemy* skip);
    void applyElement(Enemy& e, Element el, int owner, float hitDmg, const WorldParams& p, FrameEvents& ev);
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
    bool eliteWave_ = false;

    float pickupTimer_ = cfg::pickup::spawnMin;
    bool invuln_ = false;  // dev: core takes no damage
    int phoenixLeft_ = 0;  // "Phoenix": times the core can still come back this act

    int aegisChargesLeft_ = 0;      // "Aegis": hits the core still soaks this wave
    bool coreHitThisWave_ = false;  // "Interest": did anything reach the core this wave
    PowerUp reservePu_ = PowerUp::Points2x;   // "Stockpile"
    bool hasReserve_ = false;
    float reserveTimer_ = cfg::powerup::reserveFillTime;

    // pacts
    std::vector<sf::Vector2f> huntPrey_;   // "Hunters": per ball, where its prey was last seen
    std::vector<char> huntHas_;            // ...and whether it has one
    float coreZapT_ = 0.f;                 // "Living Core": time to the next zap
    int comboExtra_ = 0;                   // "Bloodlust": extra combo steps per hit (set each step)
};

}  // namespace sb
