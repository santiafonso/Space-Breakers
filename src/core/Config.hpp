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
inline constexpr float radius = 13.f;
inline constexpr float baseCruise = 330.f;       // px/s at speed level 0
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
inline constexpr int bouncesPerTier = 8;
inline constexpr int baseCapTier = 6;              // + 1 per Combo Hold level
inline constexpr float decayWindow = 2.6f;         // s without a bounce before a tier drops
inline constexpr float decayWindowPerLevel = 0.9f; // + per Combo Hold level
inline constexpr float multiplierPerTier = 0.5f;
}  // namespace combo

namespace wall {
inline constexpr int maxWalls = 6;
inline constexpr float driftCap = 150.f;           // px/s ceiling for a drifting wall
inline constexpr float throwToDrift = 0.22f;       // fraction of throw speed kept as drift
inline constexpr float minThrowToDrift = 60.f;     // slower throw just parks the wall
inline constexpr float rightClickDrift = 70.f;     // px/s given by a right-click nudge
inline constexpr float grabPadding = 6.f;          // px of slack around a wall for grabbing
}  // namespace wall

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
