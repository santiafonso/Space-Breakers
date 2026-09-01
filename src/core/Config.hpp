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
inline constexpr float radius = 16.f;
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
inline constexpr float bounceAngleJitter = 0.08f;
inline constexpr float minAxisFraction = 0.16f;
inline constexpr float substepPerRadius = 0.5f;
inline constexpr int maxSubsteps = 8;
}  // namespace ball

// Balls orbit the core. A radial spring holds them near their orbit radius; a
// tangential term keeps them circling at cruise speed. Flinging a ball knocks it
// off orbit and it spirals back.
// Each ball is steered toward a target velocity every step: cruise speed along
// the tangent to a circle around the core, plus a correction toward its orbit
// radius. That target is what makes the motion curved. The steer is applied to
// the velocity directly so it converges no matter how fast the ball is going.
namespace orbit {
inline constexpr float radiusPx = 215.f;       // nominal orbit radius
inline constexpr float radiusJitter = 55.f;    // +/- per ball so they spread out
inline constexpr float pullK = 3.6f;           // radius error -> radial correction speed
inline constexpr float maxRadial = 460.f;      // clamp on that correction (px/s)
inline constexpr float steerRate = 10.0f;      // how fast velocity eases to the orbit target
// When an enemy is near, the ball's orbit ring expands / shrinks to the enemy's
// distance from the core (so the circle passes through the enemy), and the
// target velocity leans partly toward it. The path stays a curve.
inline constexpr float interceptWeight = 0.62f; // 0 = pure orbit, 1 = straight at the enemy
inline constexpr float interceptRange = 720.f;
inline constexpr float ringMin = 120.f;        // clamp on the adaptive orbit radius
inline constexpr float ringMax = 360.f;
inline constexpr float ringAdapt = 6.0f;       // how fast orbitRadius eases toward the enemy ring
}  // namespace orbit

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
inline constexpr float hitRebound = 0.35f;              // ball mostly ploughs through, orbit re-corrects
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
inline constexpr float hpPlusBonus = 70.f;     // "reinforced core" meta unlock
inline constexpr float enemyDamage = 8.f;      // hp lost per enemy that reaches the core
inline constexpr float waveHeal = 9.f;         // core repaired this much on a wave clear
}  // namespace core

namespace wave {
inline constexpr int baseCount = 3;
inline constexpr float countGrowth = 1.22f;
inline constexpr int maxCount = 60;
inline constexpr float spawnInterval = 0.95f;
inline constexpr float hpBase = 3.f;
inline constexpr float hpGrowth = 1.19f;
inline constexpr float speedBase = 34.f;
inline constexpr float speedGrowth = 1.06f;
inline constexpr float speedMax = 135.f;
inline constexpr float enemyRadius = 19.f;
}  // namespace wave

namespace meta {
inline constexpr int coresPerWave = 1;
inline constexpr int coresPerBoss = 5;
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
