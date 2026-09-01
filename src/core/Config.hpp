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
inline constexpr float radius = 14.f;
inline constexpr float baseCruise = 300.f;       // px/s at speed level 0
inline constexpr float cruiseGrowth = 1.16f;     // x per Speed upgrade level
inline constexpr float hardSpeedCap = 2600.f;
inline constexpr float maxSpeedCruiseMul = 4.0f; // ceiling = cruise * this (capped by hardSpeedCap)
inline constexpr int maxBalls = 6;

// Speed regulation: cruise is a floor the ball climbs back to quickly and a
// target it eases down to slowly, so a throw stays fast for seconds.
inline constexpr float regainRate = 3.5f;        // toward cruise from below
inline constexpr float decayRate = 0.30f;        // toward cruise from above
inline constexpr float decayRateSlowMo = 1.8f;   // faster settle under SLOW MOTION

inline constexpr float minThrowSpeed = 45.f;     // below this a "throw" is just a nudge
inline constexpr float nudgeSpeed = 150.f;        // random speed given to a dropped ball
inline constexpr float forceReleaseSpeed = 200.f; // speed when a grab is torn away

inline constexpr float squashDecay = 9.f;
inline constexpr float bounceAngleJitter = 0.13f;  // rad, added each bounce
inline constexpr float minAxisFraction = 0.22f;    // never a flat ping-pong
inline constexpr float substepPerRadius = 0.5f;    // advance <= radius*this per sub-step
inline constexpr int maxSubsteps = 8;
}  // namespace ball

namespace combo {
// Combo is now a DAMAGE multiplier: it climbs when balls hit enemies in quick
// succession and decays on time since the last hit.
inline constexpr int bouncesPerTier = 6;           // enemy hits per tier
inline constexpr int baseCapTier = 8;
inline constexpr float decayWindow = 2.4f;         // s without an enemy hit before a tier drops
inline constexpr float decayWindowPerLevel = 0.0f;
inline constexpr float multiplierPerTier = 0.5f;
}  // namespace combo

namespace combat {
inline constexpr float contactDamageBase = 1.0f;   // damage at zero speed
inline constexpr float contactDamagePerCruise = 1.7f;  // + this * (speed / baseCruise)
inline constexpr float knockback = 210.f;          // px/s pushed into the enemy on a hit
inline constexpr float hitRebound = 0.85f;         // how much the ball reflects off an enemy (1 = full)
// Baseline pull toward the nearest enemy so a ball reliably finds work even with
// no structures placed. Field structures still dominate the routing.
inline constexpr float homingAccel = 1150.f;       // px/s^2 toward the nearest enemy
inline constexpr float homingRange = 760.f;        // only within this distance
}  // namespace combat

namespace field {
inline constexpr int startSlots = 2;               // black holes you may have placed at once
inline constexpr float blackHoleStrength = 3.2e5f; // accel = strength / dist^2
inline constexpr float blackHoleRadius = 260.f;    // influence radius
inline constexpr float maxAccel = 2200.f;          // clamp so it never yanks the ball insanely
inline constexpr float minDist = 26.f;             // distance floor for the 1/d^2 term
inline constexpr float grabPadding = 14.f;         // slack around a structure for grabbing
}  // namespace field

namespace core {
inline constexpr float radius = 34.f;
inline constexpr float baseHp = 140.f;
inline constexpr float hpPlusBonus = 70.f;         // "reinforced core" meta unlock
inline constexpr float enemyDamage = 8.f;          // hp lost per enemy that reaches the core
}  // namespace core

namespace wave {
inline constexpr int baseCount = 4;
inline constexpr float countGrowth = 1.26f;        // enemies = baseCount * growth^(wave-1)
inline constexpr int maxCount = 60;
inline constexpr float spawnInterval = 0.95f;      // seconds between spawns
inline constexpr float hpBase = 3.f;
inline constexpr float hpGrowth = 1.15f;
inline constexpr float speedBase = 34.f;
inline constexpr float speedGrowth = 1.05f;
inline constexpr float speedMax = 120.f;
inline constexpr float enemyRadius = 19.f;
}  // namespace wave

namespace meta {
inline constexpr int coresPerWave = 1;             // meta currency earned per wave survived
inline constexpr int coresPerBoss = 5;
}  // namespace meta

namespace pickup {
inline constexpr float radius = 12.f;
inline constexpr float ttl = 14.f;                 // seconds on screen before it fades
inline constexpr float firstSpawnMin = 16.f;
inline constexpr float firstSpawnMax = 26.f;
inline constexpr float spawnMin = 22.f;
inline constexpr float spawnMax = 40.f;
inline constexpr float luckFactor = 0.72f;         // spawn interval x this per Lucky Orbs level
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
inline constexpr int frenzyMul = 3;
inline constexpr int points2xMul = 2;
inline constexpr int goldenBonusBase = 6;          // + goldenBonusPerPoints * pointsLevel
inline constexpr int goldenBonusPerPoints = 2;
}  // namespace powerup

namespace app {
inline constexpr float autosaveInterval = 20.f;
inline constexpr float throwVelScale = 1.15f;      // pointer velocity -> ball velocity
inline constexpr float pointerSampleWindow = 0.09f;
inline constexpr float catchRadius = 95.f;         // how close a click must be to grab a ball
inline constexpr float fadeRate = 14.f;            // screen-transition dim decay
}  // namespace app

}  // namespace sb::cfg
