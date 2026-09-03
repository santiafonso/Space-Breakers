#pragma once

// Every gameplay-tuning number in one place. Pure feel / balance knobs live
// here; incidental smoothing factors (UI hover easing, trail length) stay next
// to the code that owns them. Grouped by subsystem.
namespace sb::cfg {

namespace loop {
// The simulation advances in fixed slices so physics is identical at any frame
// rate. Render still happens once per real frame.
inline constexpr float fixedDt = 1.f / 120.f;
inline constexpr float maxFrame = 0.25f;   // clamp a stalled frame (catch-up cap)
inline constexpr int maxSteps = 12;        // spiral-of-death guard per frame
}  // namespace loop

namespace ball {
inline constexpr float radius = 18.f;
inline constexpr float baseCruise = 300.f;       // px/s orbit speed at level 0
inline constexpr float hardSpeedCap = 2600.f;
inline constexpr float maxSpeedCruiseMul = 4.0f; // ceiling = cruise * this (capped by hardSpeedCap)
inline constexpr int maxBalls = 8;

// Speed regulation: cruise is a floor the ball climbs back to quickly and a
// target it eases down to slowly, so a fling stays fast for a moment.
inline constexpr float regainRate = 3.5f;
inline constexpr float decayRate = 0.55f;
inline constexpr float decayRateSlowMo = 1.8f;

inline constexpr float minThrowSpeed = 45.f;
inline constexpr float nudgeSpeed = 150.f;
inline constexpr float forceReleaseSpeed = 200.f;

inline constexpr float squashDecay = 9.f;
// Balls travel in straight lines and only turn on a bounce; the jitter and the
// min-axis floor keep those bounces lively instead of a flat ping-pong.
inline constexpr float bounceAngleJitter = 0.13f;
inline constexpr float minAxisFraction = 0.20f;
inline constexpr float substepPerRadius = 0.5f;
inline constexpr int maxSubsteps = 8;
}  // namespace ball

namespace combo {
// Combo is a DAMAGE multiplier: climbs on enemy hits, decays on time since the
// last hit.
inline constexpr int bouncesPerTier = 6;
inline constexpr int baseCapTier = 8;
inline constexpr float decayWindow = 2.4f;
inline constexpr float decayWindowPerLevel = 0.0f;
inline constexpr float multiplierPerTier = 0.5f;
}  // namespace combo

namespace combat {
inline constexpr float contactDamageBase = 1.0f;
inline constexpr float contactDamagePerCruise = 1.7f;   // + this * (speed / baseCruise)
inline constexpr float knockback = 190.f;
inline constexpr float hitRebound = 0.9f;               // the ball bounces off an enemy like a wall

// Between-wave upgrades that touch the simulation.
inline constexpr float springBoost = 1.6f;        // "Spring": ball speed x this on a core bounce
inline constexpr float flingDecayMult = 0.45f;    // "Reflexes": fling speed decays this much slower
inline constexpr float heavyImpactPerPick = 0.30f;// "Heavy impact": +this contact damage per pick
inline constexpr float bigBallPerPick = 0.25f;    // "Big ball": +this radius fraction per pick
inline constexpr int   bigBallMaxPicks = 3;
inline constexpr float coreArmorHp = 25.f;        // "Reinforced core" upgrade, per pick
inline constexpr float retaliateRadius = 190.f;   // "Retaliate": pulse when an enemy hits the core
inline constexpr float retaliateDamage = 6.f;
inline constexpr float retaliateKnockback = 320.f;
inline constexpr float secondChanceHp = 1.f;      // "Second chance": core survives a lethal hit at this
inline constexpr float secondChanceHeal = 0.30f;  // ...then heals this fraction of base HP
inline constexpr int   lootBonusPct = 20;         // "Loot": +this% cores at the end of the run
}  // namespace combat

// Per-element behaviour for the ball types bought between waves.
namespace element {
// fire: applies a burn (damage over time) on contact
inline constexpr float burnDuration = 3.0f;
inline constexpr float burnDps = 2.4f;
// wind: fires a bolt at the nearest enemy on a timer
inline constexpr float windInterval = 1.3f;
inline constexpr float windSpeed = 660.f;
inline constexpr float windLife = 1.5f;
inline constexpr float windDamage = 2.2f;
inline constexpr float windRange = 900.f;
// water: drips a damaging puddle along its path
inline constexpr float waterInterval = 0.26f;
inline constexpr float puddleRadius = 26.f;
inline constexpr float puddleLife = 2.4f;
inline constexpr float puddleDps = 3.4f;
// stone: drops blocking rubble on a timer
inline constexpr float stoneInterval = 2.0f;
inline constexpr float obstacleRadius = 17.f;
inline constexpr float obstacleLife = 5.0f;
inline constexpr int maxObstacles = 14;
inline constexpr int maxPuddles = 60;
inline constexpr int maxProjectiles = 40;
}  // namespace element

namespace core {
inline constexpr float radius = 34.f;
inline constexpr float baseHp = 140.f;
inline constexpr float hpPerBulwark = 40.f;    // "Bulwark" meta unlock, per level
inline constexpr float enemyDamage = 8.f;      // hp lost per enemy that reaches the core
inline constexpr float waveHeal = 9.f;         // core repaired this much on a wave clear
}  // namespace core

// A run is a fixed sprint: survive to the final wave and you win.
namespace run {
inline constexpr int startBalls = 1;   // before the "Squad" meta unlock
inline constexpr int finalWave = 10;
}  // namespace run

// The final wave is a miniboss duel in a wider arena.
namespace boss {
inline constexpr float arenaScaleX = 1.95f;   // boss arena vs the normal one (camera pulls way back)
inline constexpr float arenaScaleY = 1.45f;
inline constexpr float coreMarginX = 110.f;   // core sits this far from the left wall
inline constexpr float hp = 40.f;             // small bar - a handful of clean hits
inline constexpr float radius = 58.f;         // fat target - you are meant to fling at it
inline constexpr float speed = 54.f;          // px/s, dead straight at the core, no steering
inline constexpr float addInterval = 1.15f;   // infinite adds cadence while the boss lives
inline constexpr int   maxAdds = 16;          // concurrent cap so it stays fair
inline constexpr float camEase = 2.1f;        // camera zoom transition rate (lower = slower pull-back)
}  // namespace boss

namespace wave {
inline constexpr int baseCount = 3;
inline constexpr float countGrowth = 1.22f;
inline constexpr int maxCount = 60;
inline constexpr float spawnInterval = 0.95f;
inline constexpr float hpBase = 3.f;
inline constexpr float hpGrowth = 1.16f;
inline constexpr float speedBase = 34.f;
inline constexpr float speedGrowth = 1.06f;
inline constexpr float speedMax = 135.f;
inline constexpr float enemyRadius = 19.f;
}  // namespace wave

namespace meta {
inline constexpr int coresPerWave = 2;   // earned at the end of a run, per wave reached
inline constexpr int winBonus = 10;      // extra for clearing the final wave
}  // namespace meta

// Power-up orbs still drift in and buff the balls for a few seconds.
namespace pickup {
inline constexpr float radius = 12.f;
inline constexpr float ttl = 14.f;
inline constexpr float firstSpawnMin = 16.f;
inline constexpr float firstSpawnMax = 26.f;
inline constexpr float spawnMin = 22.f;
inline constexpr float spawnMax = 40.f;
inline constexpr float driftMin = 45.f;
inline constexpr float driftMax = 80.f;
}  // namespace pickup

namespace powerup {
inline constexpr float durPoints2x = 8.f;
inline constexpr float durSlowMo = 6.f;
inline constexpr float durSurge = 7.f;
inline constexpr float durGolden = 7.f;
inline constexpr float durGhost = 6.f;
inline constexpr float durFrenzy = 5.f;
inline constexpr float slowMoCruiseMul = 0.42f;
inline constexpr float surgeCruiseMul = 2.0f;
}  // namespace powerup

namespace app {
inline constexpr float autosaveInterval = 20.f;
inline constexpr float throwVelScale = 1.15f;
inline constexpr float pointerSampleWindow = 0.09f;
inline constexpr float catchRadius = 95.f;
inline constexpr float fadeRate = 14.f;
}  // namespace app

}  // namespace sb::cfg
