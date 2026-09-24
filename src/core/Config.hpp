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
inline constexpr int maxBalls = 5;   // few balls, each one a built-up "character" (role + element + 2 gear slots)

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

// Between-wave picks. ITEMS sit in one of a ball's 4 slots and only affect
// that ball; an item's forge level scales its bonus by gearLevelBonus per level
// past the first. MODIFIERS are per-ball stat bumps with no slot - they stack
// without limit (the per-stack values below; radius has a physical cap).
inline constexpr float gearLevelBonus = 0.5f;
inline constexpr float springBoost = 1.6f;        // "Spring": ball speed x this on a core bounce
inline constexpr float wallBounceBoost = 1.12f;   // "Wall rush": ball speed x this on a wall bounce
inline constexpr float pairBounceBoost = 1.16f;   // "Carom": ball speed x this when it clacks another ball
inline constexpr float flingPowerBoost = 1.4f;    // "Strong arm": a flung ball leaves your hand x this faster
inline constexpr float heavyImpactPerStack = 0.15f;  // "Heavy impact": + this contact damage
inline constexpr float bigBallPerStack = 0.10f;      // "Big ball": + this radius...
inline constexpr float bigBallMaxMult = 2.0f;        // ...up to x this (so a ball can't fill the arena)
inline constexpr float swiftPerStack = 0.08f;        // "Swift": + this cruise speed
inline constexpr float ceilingPerStack = 0.20f;      // "Ceiling break": + this top speed
inline constexpr float knockPerStack = 0.5f;         // "Heavy knock": + this knockback
inline constexpr float reflexesPerStack = 0.75f;     // "Reflexes": fling decay x this per stack
inline constexpr float heftPerLevel = 0.08f;      // "Heft" web node: + this contact damage, every ball
inline constexpr float massPerLevel = 0.10f;      // "Mass" web node: + this radius, every ball
inline constexpr float slowFieldRadius = 210.f;   // "Slow field": zone around the core...
inline constexpr float slowFieldMul = 0.55f;      // ...enemies inside move at this fraction of speed

// More between-wave items (all booleans, one pick each).
inline constexpr float ricochetWindow = 0.6f;    // "Ricochet": a wall bounce arms a damage bonus for this long
inline constexpr float ricochetMult = 1.6f;      // ...contact damage x this while it's armed
inline constexpr float warmUpTime = 22.f;        // "Warm-up": cruise speed ramps up over this many seconds of a wave
inline constexpr float warmUpBonus = 0.6f;       // ...up to + this fraction of cruise by the end
inline constexpr float conductorRange = 240.f;   // "Conductor": the electric arc jumps to a 2nd enemy within this
inline constexpr float conductorFalloff = 0.6f;  // ...for this fraction of the bolt's damage
inline constexpr float shatterBonus = 1.8f;      // "Shatter": hitting a frozen enemy does x this damage
inline constexpr float contagionRadius = 90.f;   // "Contagion": a poisoned enemy dying re-poisons others within this
inline constexpr float bedrockLifeMult = 4.f;    // "Bedrock": stone rubble lasts x this long
inline constexpr float primedMult = 1.35f;       // "Primed": +damage to an enemy already under an element effect

// "Ball combat" between-wave items (Fase A). No explicit prereqs - the pick pool
// just rolls them together, so synergies fall out of what you already have:
// Keen eye / Overkill / Battering all lean on the same speed & damage picks.
inline constexpr float critChance = 0.15f;       // "Keen eye": chance a contact hit deals...
inline constexpr float critMult = 2.0f;          // ...x this damage
inline constexpr float bruiserPerCruise = 0.28f; // "Battering": + this * (speed / baseCruise) contact damage
inline constexpr float executeThreshold = 0.30f; // "Executioner": enemies below this HP fraction...
inline constexpr float executeMult = 2.6f;       // ...take x this contact damage
inline constexpr float overkillFrac = 0.5f;      // "Overkill": this fraction of a kill's leftover damage...
inline constexpr float overkillRange = 150.f;    // ...splashes onto the nearest enemy within this
inline constexpr float tempoRecover = 0.4f;      // "Tempo": ball eases this far back toward cruise on an enemy hit
}  // namespace combat

// Ball roles. Every ball is one of three; the role shapes how it wants to be
// played more than any single gear does.
namespace role {
// Striker: the one worth flinging. Above its cruise speed every hit scales up
// hard, it leaves your hand harder and holds a fling longer.
inline constexpr float strikerSpeedDamage = 0.7f;  // contact dmg x (1 + this * (speed / cruise - 1)) above cruise
inline constexpr float strikerFlingMult = 1.25f;   // throw velocity x this
inline constexpr float strikerFlingDecay = 0.6f;   // fling speed decays at this fraction of the normal rate
// Support: weak hits, but marks what it touches - every ball hits a marked
// enemy harder - and its element is stronger.
inline constexpr float supportDamageMul = 0.6f;
inline constexpr float supportElemMul = 1.4f;      // poison / freeze / wake / zap potency on a support ball
inline constexpr float markDuration = 3.0f;
inline constexpr float markDamageMul = 1.35f;      // any ball (and zaps) vs a marked enemy
// Guardian: big and heavy, keeps the core clear. Hits shove enemies back hard
// and stagger them (no advancing) for a moment.
inline constexpr float guardianRadiusMul = 1.3f;
inline constexpr float guardianDamageMul = 0.75f;
inline constexpr float guardianKnockMul = 2.0f;
inline constexpr float guardianCruiseMul = 0.9f;
inline constexpr float staggerDuration = 0.7f;     // enemy drifts on its knockback, not toward the core
inline constexpr float staggerDrag = 3.5f;         // knockback bleed-off per second while staggered
// A Guardian "reads" the threat: every bounce (wall, core, enemy) sends it off
// in a straight line toward the enemy closest to the core. Not homing - it
// never curves mid-flight; only its bounce angle is chosen.
inline constexpr bool  guardianAimsBounces = true;
inline constexpr float guardianAimJitter = 0.06f;  // rad of wobble so it isn't robotic
}  // namespace role

// Run score: arcade points, shown in the HUD and kept as a lifetime best.
namespace score {
inline constexpr int perKill = 100;   // per enemy killed; x2 while DOUBLE POINTS is up; boss gives 0
}  // namespace score

// Per-element behaviour. Elemental balls come from the between-wave "add a ball
// of element X" items. Each element has a web node: level 1 unlocks its item,
// levels 2-3 raise its potency (elemMult). Unlock order, each gating the next:
// fire -> poison -> water -> ice -> stone -> electric.
namespace element {
// fire: a heavier contact hit (used to be a burn) - scales the ball's damage
inline constexpr float fireDamageBonus = 0.60f;   // + this * elemMult on top of normal contact damage
// poison: stacking damage-over-time, refreshed and stacked on every hit
inline constexpr float poisonDuration = 3.5f;
inline constexpr float poisonDpsPerHit = 1.1f;    // each hit adds this much dps...
inline constexpr float poisonDpsMax = 7.0f;       // ...capped here
// ice: a hit freezes the enemy in place for a moment
inline constexpr float freezeDuration = 1.3f;
// "Ember" web node: fire hits also light the enemy for a short burn (fire has no
// damage-over-time on its own). Scales with elemMult[Fire] (Ignition level) and
// the Ember node level.
inline constexpr float burnDuration = 2.5f;
inline constexpr float burnDps = 2.0f;
inline constexpr float burnPerEmberLevel = 0.5f;  // + this * (emberLevel - 1) to burn dps
// water: drags a damaging "worm" wake that follows the ball's path and tapers
// from head to tail
inline constexpr float waterInterval = 0.035f;    // time between trail points laid down
inline constexpr int   waterTrailPoints = 30;     // worm length (~1s of travel)
inline constexpr float waterTrailWidth = 16.f;    // damage half-width at the head; tapers toward the tail
inline constexpr float waterDps = 3.2f;
// electric: zaps the nearest enemy inside an (invisible) radius, on a timer
inline constexpr float boltRadius = 190.f;
inline constexpr float boltInterval = 0.7f;
inline constexpr float boltDamage = 2.4f;
inline constexpr float boltLife = 0.13f;          // the arc is just a brief visual flash
inline constexpr int   maxBolts = 24;
// stone: drops rubble that blocks enemies AND grinds any standing in it
inline constexpr float stoneInterval = 1.7f;
inline constexpr float obstacleRadius = 18.f;
inline constexpr float obstacleLife = 5.0f;
inline constexpr float stoneDps = 2.2f;
inline constexpr int maxObstacles = 16;
// each web level past the first multiplies an element's potency by +this
inline constexpr float powerPerLevel = 0.35f;
}  // namespace element

namespace core {
inline constexpr float radius = 34.f;
inline constexpr float baseHp = 60.f;
inline constexpr float hpPerBulwark = 20.f;    // "Bulwark" meta unlock, per level
inline constexpr float enemyDamage = 8.f;      // hp lost per enemy that reaches the core
inline constexpr float waveHeal = 9.f;         // core repaired this much on a wave clear
inline constexpr float mendPerLevel = 3.f;     // "Mend" meta node: + this to waveHeal per level
inline constexpr float regenPerLevel = 1.5f;   // "Regen" meta node: core hp/s during a wave, per level
inline constexpr float bastionPerWavePerLevel = 1.0f;  // "Bastion" meta node: + core max hp each wave, per level
}  // namespace core

// A run is a fixed sprint: survive to the final wave and you win.
namespace run {
inline constexpr int startBalls = 1;   // before the "Squad" meta unlock
inline constexpr int bossWave = 10;    // the miniboss duel
inline constexpr int finalWave = 20;   // last wave once "Continue" past the boss is unlocked
inline constexpr float coreSlideTime = 1.4f;  // core eases left -> arena centre entering wave 11
inline constexpr int rerollsPerLevel = 2;     // "Foresight" web node: reroll charges per run, per level
}  // namespace run

// The path map between waves (one per act) and the run's gold.
namespace map {
inline constexpr int rows = 9;          // choosable rows per act; the boss is row 10
inline constexpr int lanes = 4;         // max nodes per row
inline constexpr int minPerRow = 2;
// Node weights for rows 2..8 (row 1 is always a fight; row 9 is rest/shop/forge).
inline constexpr int wCombat = 44;
inline constexpr int wElite = 16;
inline constexpr int wShop = 13;
inline constexpr int wForge = 9;
inline constexpr int wRest = 9;
inline constexpr int wUpgrade = 9;
// Elite waves: tougher and more of them.
inline constexpr float eliteHpMul = 1.6f;
inline constexpr float eliteCountMul = 1.3f;
}  // namespace map

namespace gold {
inline constexpr int combatBase = 12;     // a cleared fight pays this + perRow * row
inline constexpr int elitePerRowMul = 2;  // an elite pays 2x a fight (and a pick)
inline constexpr int perRow = 2;
inline constexpr int bossPay = 40;
// Shop prices.
inline constexpr int priceNewBall = 60;
inline constexpr int priceRole = 35;
inline constexpr int priceElement = 40;
inline constexpr int priceItem = 45;
inline constexpr int priceModifier = 22;
inline constexpr int priceRelic = 70;
inline constexpr int priceRepair = 20;     // repairs repairFrac of the core's max HP
inline constexpr float repairFrac = 0.30f;
inline constexpr int shopOffers = 5;
inline constexpr int maxItemLevel = 3;     // forge cap
}  // namespace gold

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
inline constexpr float salvagePerKillPerLevel = 0.06f; // "Salvage" node: extra cores per enemy kill, per level
inline constexpr float interestPerLevel = 4.f;         // "Interest" node: cores for a no-damage wave, per level
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

// Pickups web branch, Fase A.
inline constexpr float reserveFillTime = 22.f;     // "Stockpile": seconds to refill the reserve slot
inline constexpr float chargedFracPerLevel = 0.15f;// "Charged": + this fraction of duration per level
inline constexpr float afterglowPerLevel = 1.5f;   // "Afterglow": a continuous effect fades over this many s past 0, per level
inline constexpr float magnetAccel = 900.f;        // "Magnet": pickup steering toward the nearest ball
inline constexpr float magnetMaxSpeed = 340.f;
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
