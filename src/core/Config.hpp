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

// Between-wave "items" that touch the simulation. Heft / Mass (meta web) stack
// onto the same per-pick counters, so the maxPicks caps are the combined limit.
inline constexpr float springBoost = 1.6f;        // "Spring": ball speed x this on a core bounce
inline constexpr float flingDecayMult = 0.45f;    // "Reflexes": fling speed decays this much slower
inline constexpr float heavyImpactPerPick = 0.08f;// "Heavy impact" / "Heft": +this contact damage each
inline constexpr int   heavyImpactMaxPicks = 3;
inline constexpr float bigBallPerPick = 0.10f;    // "Big ball" / "Mass": +this radius fraction each
inline constexpr int   bigBallMaxPicks = 3;
inline constexpr float slowFieldRadius = 210.f;   // "Slow field": zone around the core...
inline constexpr float slowFieldMul = 0.55f;      // ...enemies inside move at this fraction of speed
}  // namespace combat

// Run score: arcade points, shown in the HUD and kept as a lifetime best.
namespace score {
inline constexpr int perKill = 100;   // per enemy killed; x2 while DOUBLE POINTS is up; boss gives 0
}  // namespace score

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
inline constexpr float baseHp = 60.f;
inline constexpr float hpPerBulwark = 20.f;    // "Bulwark" meta unlock, per level
inline constexpr float enemyDamage = 8.f;      // hp lost per enemy that reaches the core
inline constexpr float waveHeal = 9.f;         // core repaired this much on a wave clear
inline constexpr float mendPerLevel = 3.f;     // "Mend" meta node: + this to waveHeal per level
}  // namespace core

// A run is a fixed sprint: survive to the final wave and you win.
namespace run {
inline constexpr int startBalls = 1;   // before the "Squad" meta unlock
inline constexpr int bossWave = 10;    // the miniboss duel
inline constexpr int finalWave = 20;   // last wave once "Continue" past the boss is unlocked
inline constexpr float coreSlideTime = 1.4f;  // core eases left -> arena centre entering wave 11
}  // namespace run

// Wave 10 is a miniboss duel in a wider arena.
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
inline constexpr float hitCooldown = 0.1f;      // i-frames: a ball trapped against a boss can't melt it
inline constexpr float minHitCruiseFrac = 0.9f; // a ball must be at ~cruise speed to hurt a boss, so a
                                                // just-released "nudge" ball dropped on it does nothing
                                                // (kills the "put the cursor on the boss and spam-click" cheese)
}  // namespace boss

// Wave 20: a smaller boss that spirals in toward the core while a spinning ring
// of enemies shields it from the balls. Kill it and the ring breaks loose and
// rushes the core - clear them to win.
namespace finalBoss {
inline constexpr float radius = 46.f;           // a touch smaller than the wave-10 boss (58)
inline constexpr float hp = 64.f;               // takes real work - shouldn't fall to one burst
inline constexpr float introTime = 1.2f;        // slides in from the left edge, invulnerable, before it spirals
inline constexpr float startAngle = 3.14159265f; // enters from the left of the centred core
inline constexpr float spiralOmega = 1.0f;      // rad/s around the core (more loops = longer path)
inline constexpr float spiralShrink = 18.f;     // px/s pulled toward the core (lower = longer descent)
inline constexpr float spiralStartDist = 1200.f; // clamped to the arena edge - starts as far out as it fits
inline constexpr int   shieldCount = 6;         // orbiters kept alive around the boss
inline constexpr float shieldRadius = 118.f;    // orbiter ring radius around the boss
inline constexpr float shieldOmega = 1.5f;      // rad/s the ring spins
inline constexpr float shieldRespawn = 2.0f;    // seconds to replace a downed orbiter
inline constexpr float shieldHp = 6.f;
inline constexpr float deathBurst = 260.f;      // outward shove on the ring when the boss dies
inline constexpr float addInterval = 1.4f;      // enemies pour in from the screen edges
inline constexpr int   addCap = 16;             // concurrent edge adds (orbiters not counted)
inline constexpr float addHp = 10.f;            // softer than a plain wave-20 enemy would be
inline constexpr float addSpeed = 95.f;
}  // namespace finalBoss

namespace wave {
inline constexpr int baseCount = 4;
inline constexpr float countGrowth = 1.24f;
inline constexpr int maxCount = 100;
inline constexpr float spawnInterval = 0.95f;      // cadence on the early waves
inline constexpr float spawnIntervalMin = 0.45f;   // cadence by the final wave (denser, not a trickle)
inline constexpr float introDelay = 1.15f;   // calm beat before the first enemy of a wave
inline constexpr float hpBase = 3.f;
inline constexpr float hpGrowth = 1.17f;
inline constexpr float speedBase = 34.f;
inline constexpr float speedGrowth = 1.06f;
inline constexpr float speedMax = 165.f;
inline constexpr float enemyRadius = 19.f;
}  // namespace wave

namespace meta {
inline constexpr int coresPerWave = 2;   // earned at the end of a run, per wave reached
inline constexpr int winBonus = 10;      // extra for clearing the final wave
inline constexpr int prismsPerWin = 1;   // "prism": special currency for clearing the final wave
inline constexpr float windfallChance = 0.20f;  // "Windfall" node: chance a won run pays a 2nd prism
inline constexpr float bountyPerKillPerLevel = 0.10f;  // "Fortune" node: cores per enemy kill, per level
}  // namespace meta

// Power-up orbs drift in and buff the balls for a few seconds. Spawn cadence is
// deliberately slow at baseline; the "Uplink" web node scales p.pickupSpawnMult
// down and "Capacitor" scales p.pickupDurMult up.
namespace pickup {
inline constexpr float radius = 12.f;
inline constexpr float ttl = 14.f;
inline constexpr float firstSpawnMin = 16.f;
inline constexpr float firstSpawnMax = 26.f;
inline constexpr float spawnMin = 46.f;
inline constexpr float spawnMax = 78.f;
inline constexpr float driftMin = 45.f;
inline constexpr float driftMax = 80.f;
}  // namespace pickup

namespace powerup {
inline constexpr float durPoints2x = 8.f;
inline constexpr float durSlowMo = 6.f;
inline constexpr float durSurge = 7.f;
inline constexpr float durGolden = 7.f;
inline constexpr float durOverdrive = 5.f;
inline constexpr float surgeCruiseMul = 2.0f;      // SPEED SURGE: ball cruise x this
inline constexpr float slowMoEnemyMul = 0.45f;     // SLOW MOTION: enemies move at this fraction
inline constexpr int   goldenComboRate = 2;        // GOLDEN BOUNCE: combo climbs this many steps per hit
inline constexpr float overdriveDamageMul = 2.0f;  // OVERDRIVE: ball contact damage x this
}  // namespace powerup

namespace app {
inline constexpr float autosaveInterval = 20.f;
// A new wave eases in: the sim runs from this fraction of speed up to full over
// this long, so the scene you ended on flows into the next one instead of snapping.
inline constexpr float waveIntroTime = 0.85f;
inline constexpr float waveIntroSlow = 0.35f;
inline constexpr float throwVelScale = 1.15f;
inline constexpr float pointerSampleWindow = 0.09f;
inline constexpr float catchRadius = 130.f;   // grab a ball from near it, not only dead-on
inline constexpr float grabSettle = 16.f;     // how fast the grab offset eases out (per s)
inline constexpr float fadeRate = 14.f;

// Impact juice: freeze the sim for a beat and kick the camera on a hit. Kept
// short so the game still feels fast.
inline constexpr float hitstopKill    = 0.035f;  // an enemy dies
inline constexpr float hitstopCoreHit = 0.075f;  // something reaches the core
inline constexpr float hitstopBossHit = 0.055f;  // a ball lands on the miniboss
inline constexpr float camKickCoreHit = 9.f;     // camera-shake amplitude (px at normal zoom)
inline constexpr float camKickBossHit = 5.f;
inline constexpr float camKickDecay   = 13.f;    // shake falloff per second
}  // namespace app

}  // namespace sb::cfg
