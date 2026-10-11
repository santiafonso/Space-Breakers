#pragma once

// Per-class tuning. Striker / Guardian / Support keep theirs in cfg::role and
// cfg::synergy (Config.hpp); every newer class owns exactly one namespace
// below. Level-1 values, then + ...PerLevel for each item level past it, like
// the rest of the item numbers.
namespace sb::cfg {

// ==================================================================== Mage
// More ability slots (abilitySlotCount, progression/Offers.hpp); its items all
// work through the ball's abilities.
namespace mage {
// The class: every hit it lands takes this many seconds off each of its ability cooldowns.
inline constexpr float hitRefund = 0.2f;
// Ancient Mage: every cast lets out an arcane nova around the ball.
inline constexpr float novaRadius = 90.f;
inline constexpr float novaFrac = 0.6f;        // damage x the ball's hit
inline constexpr float novaKnock = 140.f;
// Focus: abilities recharge this much faster.
inline constexpr float focus = 0.15f, focusPerLevel = 0.07f;
// Barrage (was "Arcane missile", reworked when Magic missile became an
// ability): +1 missile per Magic missile volley (+1 more at Lv3 and Lv5), and
// every OTHER ability cast looses one magic missile.
inline constexpr float barrageFrac = 0.6f, barrageFracPerLevel = 0.12f;   // that free missile: damage x the ball's hit
inline constexpr float barrageMissilePerLevel = 0.1f;                      // every magic missile +this per level past 1
// Attunement: ability (and missile) damage x this; reach grows half as much.
inline constexpr float power = 1.3f, powerPerLevel = 0.1f;
// Twincast: chance a cast fires again after echoDelay.
inline constexpr float twincast = 0.3f, twincastPerLevel = 0.08f;
inline constexpr float echoDelay = 0.35f;
// Mana spring: a cast charges its other abilities this share of their cooldown.
inline constexpr float manaSpring = 0.15f, manaSpringPerLevel = 0.05f;
}  // namespace mage

// ==================================================================== Shooter
namespace shooter {
// Role base: a bullet at the nearest enemy anywhere every `fireInterval`
// seconds, faster the faster the ball flies (x rate = speed / cruise, clamped).
// 2026-10-04: a real gunner - it fires fast, across the whole arena, and a
// slow ball no longer fires slower (the Magic missile outclassed it).
inline constexpr float fireInterval = 0.4f;     // s between volleys at cruise (was 0.9)
inline constexpr float rateMin = 1.0f;          // speed factor floor (was 0.75: slow = slower fire)
inline constexpr float rateMax = 2.0f;          // ...and ceiling (flung hard)
inline constexpr float range = 5000.f;          // px: how far it looks for a target - the whole arena (was 330)
inline constexpr float bulletFrac = 0.3f;       // bullet damage = the ball's hit x this (was 0.35)
inline constexpr float bulletSpeed = 950.f;     // px/s (was 620)
inline constexpr float bulletLife = 2.4f;       // s: wall to wall (was 0.9)
inline constexpr float bulletRadius = 2.5f;     // px (hit size; drawn smaller)
inline constexpr float bossFrac = 0.5f;         // bullets chip the boss for this much
inline constexpr int maxBullets = 160;          // cap on live bullets (perf; was 90)
inline constexpr float firstDelay = 0.4f;       // s: a wave's first volley
// "Rapid fire" (Common): volleys come this much faster.
inline constexpr float rapidRate = 1.3f;
inline constexpr float rapidRatePerLevel = 0.15f;
// "Scattershot" (Common): a fan of pellets instead of one bullet.
inline constexpr int scatterPellets = 2;        // 2, 3 at Lv3, 4 at Lv5
inline constexpr float scatterFrac = 0.75f;     // each pellet's damage x this...
inline constexpr float scatterFracPerLevel = 0.06f;
inline constexpr float scatterSpread = 0.22f;   // rad between pellets
// "Rebound" (Uncommon): a bullet hops on to another enemy after a hit.
inline constexpr int reboundHops = 1;           // + 1 per level
inline constexpr float reboundRange = 190.f;    // px: how far it looks for the next one
inline constexpr float reboundKeep = 0.8f;      // damage kept per hop...
inline constexpr float reboundKeepPerLevel = 0.04f;
// "Tracer" (Uncommon): bullets carry the ball's element.
inline constexpr float tracerChance = 0.4f;
inline constexpr float tracerChancePerLevel = 0.15f;
inline constexpr float tracerFracPerLevel = 0.05f;   // + bullet damage per level past 1
// "Drill rounds" (Rare): bullets punch through enemies (and shields).
inline constexpr int drillPierce = 2;           // + 1 per level
inline constexpr float drillFrac = 1.15f;       // bullet damage x this...
inline constexpr float drillFracPerLevel = 0.08f;
// "Hair trigger" (Rare): a hit fires a burst at the enemies around it.
inline constexpr int triggerBurst = 3;          // + 1 per level
inline constexpr float triggerCooldown = 0.35f; // s between bursts (Satellite grinds)
inline constexpr float triggerFrac = 0.8f;      // burst bullet damage x this...
inline constexpr float triggerFracPerLevel = 0.05f;
// Deadeye (4 Shooter items): every Nth volley is also a rail shot through
// everything on the line, and every bullet hops once more.
inline constexpr int deadeyeEvery = 4;
inline constexpr float deadeyeRailFrac = 1.6f;  // rail damage = the ball's hit x this
inline constexpr float deadeyeRailWidth = 7.f;  // px half-width
inline constexpr float deadeyeRailLife = 0.25f; // s the line lingers (drawn)
inline constexpr int deadeyeHops = 1;
}  // namespace shooter

// ==================================================================== Assassin
namespace assassin {
// The class (2 items): a kill queues a blink to the nearest enemy.
inline constexpr float blinkCooldown = 0.45f;   // s between two blinks (no teleport loops)
inline constexpr float blinkRange = 480.f;      // px (x arena scale): nothing nearer = no blink
inline constexpr float blinkStandoff = 22.f;    // px left between the ball and its target on arrival
inline constexpr float blinkMinSpeed = 1.15f;   // leaves a blink at least this x its cruise
inline constexpr float blinkStale = 0.2f;       // s a queued blink waits (e.g. while held) before it's dropped
inline constexpr float blinkFxLife = 0.35f;     // s the blink line lingers
inline constexpr int maxBlinkFx = 32;
// Shadow Assassin (4 items): the blink cuts through a chain of enemies first.
inline constexpr int ascChain = 3;              // extra enemies slashed on the way
inline constexpr float ascSlashFrac = 0.6f;     // each slash x the ball's hit
inline constexpr float ascCooldownMul = 0.5f;   // and it blinks twice as often
// Backstab (Common): first hit after a blink.
inline constexpr float backstab = 1.5f;
inline constexpr float backstabPerLevel = 0.2f;
inline constexpr float backstabWindow = 1.0f;     // s after the blink the bonus waits for a hit
// Cull (Common): a hit that leaves an enemy under this fraction of its HP kills it.
inline constexpr float cull = 0.14f;
inline constexpr float cullPerLevel = 0.03f;
// Killing spree (Uncommon): + damage per blink in a chain.
inline constexpr float spree = 0.08f;
inline constexpr float spreePerLevel = 0.03f;
inline constexpr int spreeMax = 5;               // + 1 stack per level
inline constexpr float spreeWindow = 2.5f;       // s without a blink and the chain breaks
// Shadow trail (Uncommon): the blink path cuts what it crosses.
inline constexpr float trail = 0.45f;
inline constexpr float trailPerLevel = 0.15f;
inline constexpr float trailWidth = 14.f;        // px half-width (x arena scale)
// Smoke bomb (Rare): a burst where it lands, with its element.
inline constexpr float smoke = 0.6f;
inline constexpr float smokePerLevel = 0.15f;
inline constexpr float smokeRadius = 70.f;
inline constexpr float smokeRadiusPerLevel = 8.f;
// Phantom (Epic): a shadow copy stays where it blinked from.
inline constexpr float phantomLife = 1.6f;
inline constexpr float phantomLifePerLevel = 0.4f;
}  // namespace assassin

// ==================================================================== Summoner
namespace summoner {
// Every summon's damage is a fraction of the summoner's own hit (World::ballDamage
// when it is summoned / fires); the class and its ascended form scale all of
// them (damage and lifetime).
inline constexpr float rolePower = 1.3f;        // with the class (2 items)
inline constexpr float ascendedPower = 1.6f;    // Archsummoner (4 items)
inline constexpr float bossFrac = 0.5f;         // summons chip the boss at this much
// Class (2 items): every so often it calls a SPRITELING - a small, plain,
// short-lived ball of its element launched at the nearest enemy.
inline constexpr float spriteInterval = 4.5f;
inline constexpr float spriteIntervalAscended = 2.5f;
inline constexpr float spriteLife = 3.0f;        // x power
inline constexpr float spriteScale = 0.5f;       // radius x this
inline constexpr float spriteDamage = 0.6f;      // x a plain ball's hit (x power)
inline constexpr float spriteSpeed = 1.2f;       // leaves at this x its cruise
inline constexpr int   maxSprites = 2;           // per ball alive at once
inline constexpr int   maxSpritesAscended = 4;
// Turret (Common): a wall bounce plants a turret that shoots the nearest enemy.
inline constexpr float turretLife = 5.f, turretLifePerLevel = 1.f;
inline constexpr float turretRate = 1.2f, turretRatePerLevel = 0.25f;   // shots/s
inline constexpr float turretFrac = 0.4f, turretFracPerLevel = 0.1f;
inline constexpr float turretRange = 330.f;
inline constexpr float turretCooldown = 1.2f;    // per ball, between two plants
inline constexpr int   turretMax = 2;            // per ball: +1 at Lv3 and Lv5
inline constexpr float turretInset = 22.f;       // planted this far in from the wall
inline constexpr float shotSpeed = 720.f;
inline constexpr float shotLife = 0.8f;
inline constexpr float shotRadius = 3.f;
// Wisps (Common): each kill lets loose wisps that home in on enemies.
inline constexpr int   wispCount = 1;            // +1 at Lv3 and Lv5
inline constexpr float wispFrac = 0.35f, wispFracPerLevel = 0.1f;
inline constexpr float wispLife = 3.f;
inline constexpr float wispSpeed = 380.f;
inline constexpr float wispTurn = 7.f;           // rad/s
inline constexpr float wispRadius = 4.f;
// Totem (Uncommon): with enemies near, it plants a totem that drags them and
// pulses a little damage.
inline constexpr float totemInterval = 7.f, totemIntervalPerLevel = -0.6f;
inline constexpr float totemLife = 5.f, totemLifePerLevel = 0.75f;
inline constexpr float totemRadius = 95.f, totemRadiusPerLevel = 10.f;
inline constexpr float totemSlow = 0.35f, totemSlowPerLevel = 0.06f;   // walking speed lost inside
inline constexpr float totemPulse = 1.f;         // seconds between pulses
inline constexpr float totemFrac = 0.3f;         // pulse damage x the hit
inline constexpr int   totemMax = 2;             // per ball
// Warden (Uncommon): spirits circle the core and hit whatever they touch.
inline constexpr int   wardenCount = 2;          // +1 at Lv3 and Lv5
inline constexpr float wardenFrac = 0.5f, wardenFracPerLevel = 0.12f;
inline constexpr float wardenOrbit = 92.f;       // from the core's centre
inline constexpr float wardenSpin = 3.2f;        // rad/s
inline constexpr float wardenRadius = 7.f;
inline constexpr float wardenRest = 0.55f;       // after a hit, a spirit rests this long
inline constexpr float wardenKnock = 170.f;      // shoves what it hits away from the core
inline constexpr int   maxWardens = 4;
// Spectral sword (Rare, 2026-10-05 - was the Dragonling): a sword floats by
// the ball; every so often it darts through the nearest enemy, cutting all it
// passes, and flies back.
inline constexpr float swordEvery = 2.2f, swordEveryPerLevel = -0.2f;
inline constexpr float swordFrac = 1.f, swordFracPerLevel = 0.25f;
inline constexpr float swordRange = 420.f, swordRangePerLevel = 40.f, swordHover = 30.f, swordFollow = 8.f;
inline constexpr float swordDash = 0.18f, swordBack = 0.3f, swordOvershoot = 60.f, swordWidth = 14.f;
inline constexpr float breathLife = 0.3f;        // (old dragonling breath visual, unused)
inline constexpr float summonFreeze = 0.5f;      // a summon's full ice freezes this long (x potency)
// World caps, for performance.
inline constexpr int maxShots = 48;              // turret shots + wisps
inline constexpr int maxTurrets = 8;
inline constexpr int maxTotems = 6;
inline constexpr int maxBreaths = 8;
}  // namespace summoner

// ==================================================================== Jester
namespace jester {
// The class's chaos (2026-10-05): every few seconds a Jester pulls a trick
// out of another class's bag - a blink, missiles, a burst of bullets, a launch,
// a nova, an arc, blades, a smite. More Jester items: more often; a Grand
// Jester pulls two (three with 6+ items) at once.
inline constexpr float chaosEvery = 3.5f, chaosPerItem = 0.4f, chaosMin = 1.2f;
// Role (2 items): every hit rolls an outcome. Each band is x the run's luck;
// together they never pass bandCap (the rest is a normal hit).
inline constexpr float rollDouble = 0.15f;     // the hit lands again (x2)
inline constexpr float rollSpark = 0.15f;      // a spark jumps to the nearest other enemy...
inline constexpr float sparkFrac = 0.8f;       // ...for this x the hit
inline constexpr float sparkRange = 170.f;
inline constexpr float rollElement = 0.15f;    // a random element that can react with anything, even its own
inline constexpr float bandCap = 0.75f;
// Grand Jester: every Jester roll is taken twice (best kept) and doubles triple.
inline constexpr float grandDoubleMul = 3.f;
// Outcome pips over the enemy (a double): spaced so they never spam.
inline constexpr float popLife = 0.45f;
inline constexpr float popSpacing = 0.12f;
inline constexpr int maxPops = 8;

// "Lucky charm": + luck points for the whole run (every charm on every ball adds up).
inline constexpr int charmLuck = 2, charmLuckPerLevel = 1;
inline constexpr int charmPoints(int level) { return charmLuck + charmLuckPerLevel * (level - 1); }
// "Coin flip": heads (chance x luck) hits x heads, tails x tails.
inline constexpr float coinHeadsChance = 0.5f;
inline constexpr float coinHeads = 2.f, coinHeadsPerLevel = 0.25f;
inline constexpr float coinTails = 0.6f;
// "Wild card": chance a hit fires a random proc borrowed from any ball's items.
inline constexpr float wildChance = 0.15f, wildChancePerLevel = 0.05f;
inline constexpr float wildPower = 1.f, wildPowerPerLevel = 0.1f;   // x the borrowed proc
// "Reroll": chance a missed Jester roll is rolled once more.
inline constexpr float rerollChance = 0.35f, rerollChancePerLevel = 0.15f;
// "Chaos bounce": off a wall at a random angle; the next hit x armed.
inline constexpr float chaosSpread = 1.1f;     // rad either side of the wall's normal
inline constexpr float chaosHit = 1.5f, chaosHitPerLevel = 0.15f;
// "Jackpot": chance a kill pays out gold and a big blast.
inline constexpr float jackpotChance = 0.04f, jackpotChancePerLevel = 0.015f;
inline constexpr float jackpotBlast = 3.f, jackpotBlastPerLevel = 0.5f;   // x the killing hit
inline constexpr float jackpotRadius = 150.f;
inline constexpr int jackpotGold = 10, jackpotGoldPerLevel = 5;
}  // namespace jester

// ==================================================================== Slinger
namespace slinger {
// Role (2 items): catching it pays more, your throw's first hit lands harder.
inline constexpr float catchMul = 1.5f;        // catch reward x this
inline constexpr float thrownHit = 1.25f;      // the first hit after your throw x this...
inline constexpr float armedTime = 3.f;        // ...if it lands within this long
// Master Slinger (4 items): every catch recharges its abilities; a bigger thrown hit.
inline constexpr float masterRecharge = 0.35f; // share of each ability's cooldown
inline constexpr float masterThrownHit = 1.5f;

// "Coil": left alone it coasts down to a stop; your throw x this.
inline constexpr float coilThrow = 2.f, coilThrowPerLevel = 0.2f;
inline constexpr float coilDrag = 0.45f;       // speed lost per s (exponential), slower each level
inline constexpr float coilDragPerLevel = -0.05f;
// "Catch & release": caught within window s of your throw, +per damage (up to max).
inline constexpr float releaseWindow = 2.5f;
inline constexpr float releasePer = 0.15f, releasePerPerLevel = 0.04f;
inline constexpr int releaseMax = 5;
// "Afterburner": after your throw, while faster than its cruise, it leaves
// fire - the Fire element for real (reactions, Ember's burn, element nodes).
inline constexpr float burnFrac = 0.6f, burnFracPerLevel = 0.15f;   // flame damage/s x the ball's hit
inline constexpr float burnTime = 1.4f, burnTimePerLevel = 0.2f;
inline constexpr float flameEvery = 0.05f;     // s between flames
inline constexpr float flameLife = 1.2f;
inline constexpr float flameRadius = 22.f;
inline constexpr int maxFlames = 90;
// "Momentum": + damage per cruise of speed over its cruise.
inline constexpr float momentum = 0.35f, momentumPerLevel = 0.1f;
// "Grip": bends toward your pointer when it's close.
inline constexpr float gripTurn = 2.2f, gripTurnPerLevel = 0.4f;   // rad/s
inline constexpr float gripRange = 230.f, gripRangePerLevel = 25.f;
// "Ambush": the thrown first hit blinks on to the nearest other enemy and hits it x this.
inline constexpr float ambush = 1.5f, ambushPerLevel = 0.2f;
inline constexpr float ambushRange = 420.f;
// "Trick shot": every chance x this until the thrown first hit lands.
inline constexpr float trick = 3.f, trickPerLevel = 0.5f;
// "Double down": catch a ball you threw (within releaseWindow): its next hit
// is doubled-and-more or nothing (heads x luck).
inline constexpr float beckonCd = 1.5f, beckonCdPerLevel = 0.125f;   // "Beckon": s between blinks (1.25 at Lv3, 1 at Lv5)
// "Execution throw": thrown first hit on an enemy at full health x this.
inline constexpr float execution = 2.2f, executionPerLevel = 0.3f;
}  // namespace slinger

// ==================================================================== Alchemist
namespace alchemist {
inline constexpr float archReaction = 1.5f;    // Archalchemist: its reactions x this
inline constexpr float attune = 0.25f, attunePerLevel = 0.1f;          // + element potency
inline constexpr float crucible = 1.4f, crucibleRadius = 1.2f, cruciblePerLevel = 0.12f;
inline constexpr float aftershock = 90.f, aftershockPerLevel = 15.f;  // reach (px)
inline constexpr float flux = 0.15f, fluxPerLevel = 0.05f;
inline constexpr int prismEvery = 4;                                  // -1 per 2 levels, down to 2
}  // namespace alchemist

// ==================================================================== speed items
// The other classes' speed items (sim/WorldStyle.cpp): "still" ones scale with
// slowness (0 at cruise, 1 stopped), "moving" ones with speed over cruise.
namespace style {
inline constexpr float fastFull = 1.f;          // "moving" items at full strength this far over cruise (x cruise)
// Guardian
inline constexpr float anchor = 0.55f, anchorPerLevel = 0.08f;          // enemy slow at a standstill
inline constexpr float anchorRadius = 150.f, anchorRadiusPerLevel = 20.f;
inline constexpr float anchorPull = 45.f, anchorPullPerLevel = 10.f;   // px/s toward it
inline constexpr float plow = 260.f, plowPerLevel = 50.f;              // sideways shove (px/s)
inline constexpr float plowFrac = 0.3f, plowFracPerLevel = 0.1f;
inline constexpr float plowReach = 2.4f;                                // x its radius
inline constexpr float plowStagger = 0.35f;
// Shooter
inline constexpr float slug = 1.2f, slugPerLevel = 0.3f;
inline constexpr float strafe = 0.6f, strafePerLevel = 0.12f;           // x a bullet
inline constexpr float strafeEvery = 0.45f;
// Assassin
inline constexpr float lurk = 1.5f, lurkPerLevel = 0.3f;
inline constexpr float lurkFill = 2.5f;                                 // s at a standstill to charge fully
inline constexpr float blur = 1.f, blurPerLevel = 0.3f;                 // mark length x this
inline constexpr float blurAt = 1.4f;                                   // x cruise to pass through
// Jester
inline constexpr float sleight = 1.4f, sleightPerLevel = 0.2f;
inline constexpr float sleightEvery = 3.2f, sleightEveryPerLevel = -0.3f;
// Mage
inline constexpr float meditate = 1.f, meditatePerLevel = 0.25f;       // + recharge speed at a standstill
inline constexpr float leyline = 0.8f, leylinePerLevel = 0.2f;
inline constexpr float runeEvery = 0.35f, runeLife = 8.f, runeRadius = 70.f;
inline constexpr int runesPerBall = 6;
// Support
inline constexpr float beacon = 0.3f, beaconPerLevel = 0.08f;
inline constexpr float beaconRadius = 140.f, beaconRadiusPerLevel = 15.f;
inline constexpr float beaconTime = 2.5f;
inline constexpr float wake = 1.25f, wakePerLevel = 0.05f;
inline constexpr float wakeEvery = 0.06f, wakeLife = 1.5f, wakeRadius = 26.f, wakeCd = 0.8f;
inline constexpr int maxWake = 120;
inline constexpr float pass = 1.5f, passPerLevel = 0.15f;
inline constexpr float passCharge = 1.2f;                               // the launched ball's next hit x this
// Summoner
inline constexpr float kennel = 3.5f, kennelPerLevel = -0.4f;           // s between wisps at a standstill
inline constexpr float kennelFrac = 0.5f, kennelFracPerLevel = 0.12f;
inline constexpr float drop = 0.35f, dropPerLevel = 0.1f;               // turret shot x its hit
inline constexpr float dropLife = 6.f, dropRate = 1.4f;
}  // namespace style

// ==================================================================== commons
// The 2026-10-04 commons (sim/WorldCommons.cpp): level-1 value, then
// + ...PerLevel per level past it.
namespace common {
inline constexpr float volatileMul = 0.4f, volatilePerLevel = 0.15f;
inline constexpr float seepLife = 2.f, seepLifePerLevel = 0.5f;
inline constexpr float seepRadius = 34.f, seepRadiusPerLevel = 6.f;
inline constexpr float seepTick = 0.4f, seepFrac = 0.5f;   // how often a puddle touches what's in it; its "hit" x the ball's
inline constexpr int maxPuddles = 14;
inline constexpr float reagent = 0.2f, reagentPerLevel = 0.08f;
inline constexpr float tincture = 1.f, tincturePerLevel = 0.1f;
inline constexpr int distillEvery = 3;
inline constexpr float distill = 2.f, distillPerLevel = 1.f;
inline constexpr float pickpocket = 0.1f, pickpocketPerLevel = 0.04f;
inline constexpr float hotStreak = 0.1f, hotStreakPerLevel = 0.05f;
inline constexpr int hotStreakMax = 5;
inline constexpr float luckyBreak = 0.05f, luckyBreakPerLevel = 0.02f;
inline constexpr int confetti = 3;
inline constexpr float confettiFrac = 0.3f, confettiFracPerLevel = 0.1f, confettiRange = 220.f;
inline constexpr float spellblade = 0.4f, spellbladePerLevel = 0.15f, spellbladeWindow = 3.f;
inline constexpr float aetherStep = 1.4f, aetherStepPerLevel = 0.1f;
inline constexpr float insight = 0.3f, insightPerLevel = 0.15f;
inline constexpr float linger = 0.3f, lingerPerLevel = 0.15f;
inline constexpr float packLeader = 0.04f, packLeaderPerLevel = 0.02f;
inline constexpr int packLeaderMax = 6;
inline constexpr float decoy = 45.f, decoyPerLevel = 15.f, decoyRadius = 170.f, decoyRadiusPerLevel = 25.f;
inline constexpr float doom = 0.2f, doomPerLevel = 0.08f, doomTime = 2.5f, doomTimePerLevel = 0.75f;
inline constexpr float bleed = 0.3f, bleedPerLevel = 0.1f, bleedTime = 2.f;
inline constexpr float quickstep = 1.3f, quickstepPerLevel = 0.1f;
inline constexpr float opportunist = 0.25f, opportunistPerLevel = 0.1f;
inline constexpr float hollow = 0.4f, hollowPerLevel = 0.15f;
inline constexpr float coverFrac = 1.f, coverSpread = 0.12f;   // a covering bullet x a normal one; fan spread (rad)
inline constexpr float longPull = 0.6f, longPullPerLevel = 0.2f;
inline constexpr float relay = 2.f, relayPerLevel = 0.4f, relayWindow = 2.5f;
inline constexpr float secondWind = 0.3f, secondWindPerLevel = 0.1f;
inline constexpr float plating = 10.f, platingPerLevel = 5.f;
inline constexpr float sentinel = 0.4f, sentinelPerLevel = 0.15f, sentinelRange = 280.f;
inline constexpr float brace = 0.2f, bracePerLevel = 0.08f, braceTime = 2.f;
inline constexpr float heavyLanding = 0.4f, heavyLandingPerLevel = 0.15f, landingRadius = 95.f, landingCd = 0.35f;
inline constexpr float bulkSize = 1.12f, bulkSizePerLevel = 0.04f, bulkDamage = 1.08f, bulkDamagePerLevel = 0.04f;
inline constexpr float edge = 0.12f, edgePerLevel = 0.06f;
inline constexpr float firstBlood = 3.f, firstBloodPerLevel = 0.5f;
inline constexpr float comboHit = 0.35f, comboHitPerLevel = 0.1f, comboWindow = 0.6f;
inline constexpr float adrenaline = 0.05f, adrenalinePerLevel = 0.02f;
inline constexpr int adrenalineMax = 10;
inline constexpr float spotter = 0.15f, spotterPerLevel = 0.05f, spotTime = 2.f;
inline constexpr float anthem = 0.06f, anthemPerLevel = 0.03f;
inline constexpr float medic = 1.f, medicPerLevel = 0.5f, medicCd = 0.5f;
inline constexpr float sync = 0.5f, syncPerLevel = 0.25f;
inline constexpr float handOff = 0.25f, handOffPerLevel = 0.1f;
// uncommons
inline constexpr float fission = 110.f, fissionPerLevel = 30.f;
inline constexpr float elixir = 0.1f, elixirPerLevel = 0.05f, elixirTime = 3.f;
inline constexpr float houseEdge = 0.2f, houseEdgePerLevel = 0.08f, houseEdgeCap = 0.6f;
inline constexpr float mirror = 0.12f, mirrorPerLevel = 0.04f;
inline constexpr float sigil = 0.4f, sigilPerLevel = 0.15f, sigilLife = 4.f, sigilRadius = 30.f, sigilNova = 110.f;
inline constexpr int maxGlyphs = 10;
inline constexpr float arcaneCharge = 0.2f, arcaneChargePerLevel = 0.07f;
inline constexpr float conduit = 150.f, conduitPerLevel = 35.f;
inline constexpr float martyr = 0.5f, martyrPerLevel = 0.2f, martyrRadius = 90.f;
inline constexpr float guardDog = 0.3f, guardDogPerLevel = 0.1f;
inline constexpr float chainKill = 1.f, chainKillPerLevel = 0.3f;
inline constexpr float venomBlade = 0.f, venomBladePerLevel = 0.15f;   // the hit's bonus (the element twice is the item)
inline constexpr float suppress = 0.1f, suppressPerLevel = 0.05f, suppressMax = 0.4f;
inline constexpr float windUp = 0.5f, windUpPerLevel = 0.15f, windUpTime = 1.5f;
inline constexpr int pingPong = 4;   // "Ping-pong": walls per throw that aim it at an enemy (+1 per level)
inline constexpr float bully = 0.3f, bullyPerLevel = 0.1f;
inline constexpr float carom = 0.4f, caromPerLevel = 0.15f, caromWindow = 0.5f;
// rares
inline constexpr float transmute = 5.f, transmutePerLevel = 1.5f;
inline constexpr float residue = 2.f, residuePerLevel = 0.5f, residueRadius = 50.f, residueRadiusPerLevel = 8.f;
inline constexpr int aurum = 2;
inline constexpr float highRoller = 0.75f, highRollerPerLevel = 0.25f, highRollerPlain = 0.85f;
inline constexpr int luckySeven = 7;
inline constexpr float fortune = 0.08f, fortunePerLevel = 0.03f;
inline constexpr int fortuneMax = 5;
inline constexpr float rareAbilityPerLevel = 0.1f;   // Projection / Cascade levels: ability damage
inline constexpr float cascadeFloor = 0.35f;         // "Cascade": the soonest it fires again (no per-frame spam)
inline constexpr float spellweave = 0.5f, spellweavePerLevel = 0.15f;
inline constexpr float golemEvery = 10.f, golemEveryPerLevel = -1.5f, golemFrac = 0.8f, golemFracPerLevel = 0.2f;
inline constexpr float golemLife = 6.f, golemSpeed = 110.f, golemRadius = 16.f, golemSlamEvery = 1.f,
                       golemSlamRadius = 55.f, golemKnock = 260.f, hatcheryFrac = 0.5f;
inline constexpr int maxGolems = 6;
inline constexpr float soulbindPerLevel = 0.1f;
inline constexpr float predator = 0.4f, predatorPerLevel = 0.15f, predatorRange = 2.f;
inline constexpr float flicker = 0.3f, flickerPerLevel = 0.1f;
inline constexpr int flickerMax = 3;
inline constexpr float frag = 0.4f, fragPerLevel = 0.15f, fragRadius = 50.f;
inline constexpr int marksman = 5;
inline constexpr float marksmanMul = 3.f;
inline constexpr float skipping = 1.f, skippingPerLevel = 0.05f;
inline constexpr float formation = 0.6f, formationPerLevel = 0.15f;
inline constexpr float shieldBash = 420.f, shieldBashPerLevel = 120.f, bashStun = 1.f, bashStunPerLevel = 0.3f,
                       bashRange = 230.f;
// epics
inline constexpr float saturate = 95.f, saturatePerLevel = 25.f;
inline constexpr float maelstrom = 8.f, maelstromPerLevel = -1.f, maelstromR = 150.f, maelstromRPerLevel = 25.f;
inline constexpr float sentence = 0.2f, sentencePerLevel = 0.05f, sentenceTime = 4.f;
inline constexpr float razor = 0.5f, razorPerLevel = 0.2f, razorLife = 2.f, razorLifePerLevel = 0.5f, razorWidth = 14.f,
                       razorTick = 0.25f;
inline constexpr int maxWires = 8;
inline constexpr float wardEvery = 6.f, wardEveryPerLevel = -1.f, wardKnock = 420.f;
inline constexpr float juggernaut = 2.f, juggernautPerLevel = 0.5f;
inline constexpr float lastLine = 1.f, lastLinePerLevel = 0.5f, lastLineAt = 0.25f, lastLineKnock = 900.f,
                       lastLineStun = 1.5f, lastLineStunPerLevel = 0.5f;
// "777" (legendary): reels over the ball, then a prize
inline constexpr float slotEvery = 7.f, slotEveryPerLevel = -0.5f, slotSpin = 1.6f;   // s between spins; the reels roll this long
inline constexpr int slotTriplePct = 30, slotPairPct = 50;          // three of a kind / a pair (luck helps the triple)
inline constexpr int slotTripleJester = 60, slotTripleGrand = 92;   // ...as a Jester / a Grand Jester (all the Jester items): nearly sure
inline constexpr int slotFrenzyW = 50, slotLuckW = 35, slotRainW = 15;   // which triple
inline constexpr float slotBuff = 1.f, slotBuffPerLevel = 0.15f;    // a triple lasts this x the spin interval (till the next pull, and past it)
inline constexpr float slotPairBuff = 0.5f;                         // a pair: half of that
inline constexpr float slotFrenzySpeed = 1.7f, slotFrenzyDamage = 2.f, slotFrenzyJolt = 0.35f;   // ...and it lurches about
inline constexpr float slotLuckMul = 3.f;                           // every chance in the run x this (x the run's luck)
inline constexpr float slotRainEvery = 0.045f, slotRainFrac = 0.8f, slotRainRadius = 34.f, slotRainLong = 1.4f;   // the rain outlasts the rest
inline constexpr int slotGold = 1;                                  // a coin that lands on an enemy pays this
// "Lucky number" (epic): a d10 every few seconds
inline constexpr float luckyEvery = 8.f, luckyEveryPerLevel = -0.75f;
inline constexpr float luckySpeedPer = 0.07f, luckyDamagePer = 0.09f;   // per point above 5
inline constexpr float luckySlowPer = 0.08f, luckyKnockPer = 0.3f, luckyStunPer = 0.15f;   // per point below 5
inline constexpr float luckyStop = 1.f, luckyOneHit = 3.f, luckyTenCrit = 2.f;
inline constexpr float wildCd = 1.5f, wildCdPerLevel = -0.4f;
// "Roulette" (common): red damage, black speed, green (1 in 7) both and gold
inline constexpr float rouletteEvery = 5.f, rouletteEveryPerLevel = -0.5f, rouletteSpin = 1.f;
inline constexpr float roulette = 0.4f, roulettePerLevel = 0.1f;
inline constexpr int rouletteGreenIn = 7, rouletteGold = 5;
inline constexpr int chorus = 5;
inline constexpr float chrono = 0.2f, chronoPerLevel = 0.05f, chronoTime = 1.5f;
inline constexpr float minigun = 0.08f, minigunPerLevel = 0.03f, minigunMax = 2.f, minigunIdle = 2.f;   // rate x (1 + max x spin)
inline constexpr float smart = 5.f, smartPerLevel = 2.f;
inline constexpr int shrapnel = 6;
inline constexpr float shrapnelFrac = 0.5f;
inline constexpr float perfect = 2.5f, perfectPerLevel = 0.5f, perfectAt = 0.95f;
inline constexpr float homecoming = 0.6f, homecomingPerLevel = 0.2f, homeTime = 1.6f, homeTurn = 6.f,
                       homeRadius = 110.f;
inline constexpr float lightspeed = 0.5f, lightspeedPerLevel = 0.1f, lightspeedAt = 2.f, lightRange = 240.f;
inline constexpr int lightChains = 3;
inline constexpr float spiritHost = 3.f, spiritHostPerLevel = 0.5f, spiritLife = 2.5f;
inline constexpr int spiritAt = 5, spiritPierce = 5;
inline constexpr float spiritWispEvery = 1.2f;   // "Spirit host" lets a wisp loose itself this often
inline constexpr float twinCallPerLevel = 0.1f;
// "Baby dragon" (epic): a cone of its element at the nearest enemy
inline constexpr float babyFrac = 0.4f, babyFracPerLevel = 0.05f, babyEvery = 2.f, babyEveryPerLevel = 0.15f;
inline constexpr float babyRange = 220.f, babyCone = 0.55f, babyOrbit = 30.f;   // reach, half-angle (rad), from the ball
inline constexpr float babyCharge = 0.35f, elderCharge = 0.6f;   // s drawing breath (turning to aim) before it lets go
inline constexpr float babyTurn = 7.f, elderTurn = 2.2f;         // rad/s it turns its head
inline constexpr float babyBreathTime = 0.9f;   // s a breath of fire lasts (the damage spread over it)
// The elder (2026-10-06): breathes longer than it rests, a steady stream - a
// legendary should look it. Damage per tick as a 1.6 s breath x elderSustain.
inline constexpr float elderBreathTime = 4.5f, elderBreathRef = 1.6f, elderSustain = 0.7f, elderTick = 0.08f;
inline constexpr float breathTick = 0.15f;
inline constexpr float headReach = 1.8f;                          // rad the head turns away from the body at most
// legendaries (2026-10-05)
inline constexpr float titan = 0.12f, titanPerLevel = 0.03f;      // size gained per core bounce (1 = twice the size)
inline constexpr float titanQuake = 1.f, titanQuakePerLevel = 0.25f, titanStun = 1.5f, titanStunPerLevel = 0.25f;
inline constexpr float titanDamage = 0.5f;                         // its growth counts this much as damage
inline constexpr float archmage = 2.f, archmagePerLevel = 0.25f;   // cast rate x
inline constexpr float archStack = 0.1f, archStackPerLevel = 0.0125f;
inline constexpr int timeStopEvery = 15;
inline constexpr float timeStop = 2.f, timeStopPerLevel = 0.25f;
// "Ion cannon" (2026-10-05 rework): every bullet that lands turns into a bolt
// that jumps enemy to enemy - more jumps the faster it fires.
inline constexpr float ion = 0.85f, ionPerLevel = 0.04f;   // each jump keeps this much of the hit
inline constexpr int ionJumps = 2;                          // +1 per level past the first...
inline constexpr float ionRateJumps = 3.f;                  // ...+this per +100% fire rate over the base
inline constexpr int ionJumpsMax = 12;
inline constexpr float ionRange = 170.f;
inline constexpr float ionLife = 0.35f;                     // (the old beam's fade, for the rail drawing)
inline constexpr float gunship = 0.4f, gunshipPerLevel = 0.05f;
inline constexpr float leadStorm = 10.f, leadStormPerLevel = -1.f, leadStormTime = 2.f, leadStormRate = 6.f,
                       leadStormTurn = 0.55f;   // rad between shots: a spiral
inline constexpr float reaper = 0.3f, reaperPerLevel = -0.05f;    // s: the blink's cooldown
inline constexpr float contract = 0.1f, contractPerLevel = 0.0125f, contractBoss = 0.02f, contractBossPerLevel = 0.0025f;
// "Hive" (legendary, the many): tiny bugs that run and bite
inline constexpr float hive = 0.08f, hivePerLevel = 0.015f;     // a bite x its hit
inline constexpr int hivePer = 3, hiveMax = 32;                 // bugs per hatch (+1 per 2 levels), alive at once
inline constexpr float hiveSplitLife = 3.f;                     // a bug that kills splits in two, both with this long to hunt
inline constexpr float hiveEvery = 1.f, hiveLife = 5.f, hiveSpeed = 230.f, hiveBiteEvery = 0.45f;
inline constexpr float hiveLitBlast = 3.f, hiveLitRadius = 45.f;   // lit by a baby dragon's breath: the next bite explodes
// "Elder dragon" (legendary, the one): circles the arena, breath sweeps a swath
inline constexpr float elder = 4.f, elderPerLevel = 0.5f, elderEvery = 2.5f, elderEveryPerLevel = 0.25f;   // rest 2.5 s (was 4)
inline constexpr float elderSpin = 0.35f, elderOrbit = 0.36f;   // rad/s round the core, x the arena's short side
inline constexpr float elderLength = 620.f, elderWidth = 70.f;  // the swath: reach, half-width
inline constexpr float elderBabyMul = 1.5f;                     // with an elder about, baby breath x this
inline constexpr int elderHiveBugs = 4;                         // with a Hive about, each swath hatches this many
inline constexpr int riggedLuck = 2;                                // luck per level past the first
inline constexpr float steady = 0.2f, steadyPerLevel = -0.025f;   // time scale while aiming
inline constexpr float steadyBonus = 0.6f, steadyBonusPerLevel = 0.1f, steadyFull = 0.5f;   // s of sim time aimed for the full bonus
inline constexpr float showtime = 0.2f, showtimePerLevel = 0.025f;
// "Toxic trail": puffs dropped as it flies, its elements in turn
inline constexpr float toxicEvery = 0.1f, toxicLife = 4.f, toxicLifePerLevel = 0.75f, toxicRadius = 34.f;
inline constexpr float toxicFrac = 0.35f, toxicFracPerLevel = 0.1f, toxicTick = 0.4f;
inline constexpr float toxicSwell = 2.2f, toxicSwellTime = 0.5f, toxicLast = 0.4f;   // it billows to this x, then shrinks back to this x
// a puff's reach right now: billowing out fast, then slowly shrinking away
inline float toxicNow(float r, float life, float maxLife) {
    const float age = maxLife - life;
    if (age < toxicSwellTime) return r * (0.5f + (toxicSwell - 0.5f) * age / toxicSwellTime);
    const float q = (age - toxicSwellTime) / std::max(0.01f, maxLife - toxicSwellTime);
    return r * (toxicSwell + (toxicLast - toxicSwell) * q);
}
inline constexpr int toxicMax = 70;
// "Primal": its first element's power, every few seconds
inline constexpr float primalEvery = 2.5f, primalEveryPerLevel = -0.25f, primalPower = 1.f, primalPowerPerLevel = 0.25f;
inline constexpr float flameTime = 1.1f, flameTick = 0.15f, flameFrac = 0.3f, flameRange = 240.f, flameCone = 0.45f;
inline constexpr float frostRadius = 150.f, frostTime = 1.6f, frostFrac = 0.45f, frostTick = 0.3f, frostFreeze = 0.6f;
inline constexpr float rainTime = 2.5f, rainSlow = 0.75f, rainHaste = 1.2f, rainFrac = 0.15f;   // the first drops soak everything...
inline constexpr float rainDropEvery = 0.07f, rainDropFrac = 0.06f;                           // ...then drops keep pelting enemies
inline constexpr float quakeFrac = 0.3f, quakeStun = 0.4f, quakeEvery = 0.5f;
inline constexpr int quakeWaves = 2, boulders = 3;
inline constexpr float boulderFrac = 0.6f, boulderRadius = 60.f, boulderFall = 0.45f;
inline constexpr int bottles = 4;
inline constexpr float bottleFlight = 0.5f, bottleRadius = 70.f, bottleFrac = 0.4f;
inline constexpr float puddleTime = 1.8f, puddleTick = 0.4f, puddleFrac = 0.25f;
inline constexpr int strikes = 7, strikeChain = 2;
inline constexpr float strikeEvery = 0.12f, strikeFrac = 0.5f, strikeChainRange = 150.f;
// "Black plague": a mad, spreading sickness
inline constexpr float plague = 0.25f, plaguePerLevel = 0.05f, plagueTime = 6.f;
inline constexpr float plagueDps = 0.3f, plagueDpsPerLevel = 0.08f;    // x the hit, per s: it burns them out fast...
inline constexpr float plagueSpeed = 1.9f, plagueBite = 1.f;            // ...but they run mad and fast; touching, they bite (x their dps)
inline constexpr int plagueMax = 8;
}  // namespace common

// ==================================================================== Astronomer
// The Guardian's ascended class (2026-10-06): planets orbit the core, each at
// its own distance and pace (sim/WorldPlanets.cpp). Orbits are ellipses that
// fit the arena (squeezed to the room around the core). Damage is x the ball's
// hit x the planet's power (its item level, weaker for a bonus planet). A
// bigger ball means bigger planets and more of what they throw.
namespace planet {
// per PlanetKind: Mercury, Venus, Earth, Mars, Jupiter, Saturn, Neptune, Moon
inline constexpr float orbit[]  = {95.f, 140.f, 185.f, 225.f, 330.f, 280.f, 375.f, 65.f};   // px past the core's edge
inline constexpr float speed[]  = {4.2f, 2.0f, 1.6f, 1.35f, 0.62f, 0.9f, 0.55f, 1.7f};      // rad/s (x1.7 since the first take)
inline constexpr float radius[] = {8.f, 13.f, 14.f, 12.f, 26.f, 17.f, 15.f, 8.f};           // px at size x1
inline constexpr float frac[]   = {0.4f, 0.45f, 0.45f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f};     // a touch, x the ball's hit
inline constexpr float hitCd[]  = {0.18f, 0.3f, 0.3f, 0.3f, 0.35f, 0.3f, 0.3f, 0.3f};      // s between two hits of one planet
inline constexpr float powerPerLevel = 0.3f;   // + per item level past the first (gearPower: 1 / 3 / 5)
inline constexpr float bonusPower = 0.5f;      // an ascension's bonus planet
inline constexpr float starryBonusPower = 0.75f;
inline constexpr int starryBonus = 3;          // "Starry night": planets that join each fight
inline constexpr float starrySize = 1.3f;      // ...and every planet x this (size and hits)
inline constexpr float knock = 380.f, stagger = 0.35f;   // what a touch does
inline constexpr float bossFrac = 0.5f;
inline constexpr float sizeMin = 0.8f, sizeMax = 2.6f;   // the ball's size, as it scales the planets
// The Atlas ball itself: every homeEvery bounces (walls or enemies) it heads
// straight for the core, so it swings between the field and the core and its
// planets go off often.
inline constexpr int homeEvery = 2;
// a core bounce, per planet - each its own thing
inline constexpr float mercuryBoost = 3.f;               // Mercury: s at double speed, leaving a burning trail
inline constexpr float flareEvery = 0.04f, flareLife = 1.2f, flareR = 16.f, flareFrac = 0.8f;   // (dps, x the hit)
inline constexpr float acidR = 85.f, acidTime = 3.5f, acidFrac = 0.6f, acidMark = 1.f;   // Venus: an acid storm riding it (dps; corrodes: marked)
inline constexpr float earthEvery = 2.f, earthRange = 460.f;                             // Earth: a rocket now and then...
inline constexpr int earthSalvo = 4;                                                     // ...a salvo on a core bounce
inline constexpr float rocketFrac = 0.45f, rocketBlast = 55.f, rocketSpeed = 380.f, rocketTop = 1150.f, rocketTurn = 4.f;
inline constexpr int marsMoons = 2;                                                      // Mars: Phobos and Deimos, thrown and back
inline constexpr float moonletFrac = 0.6f, moonletSpeed = 900.f, moonletTurn = 9.f;
inline constexpr float jupiterPuffEvery = 0.3f, jupiterPuff = 42.f, jupiterPuffLife = 3.f, jupiterPuffFrac = 0.12f;
inline constexpr float spotR = 150.f, spotTime = 4.5f, spotFrac = 0.3f, spotPull = 130.f, spotDrift = 70.f;   // the Great Red Spot
inline constexpr float saturnAura = 1.9f;              // Saturn: its rings cut this far out (x its radius)...
inline constexpr float haloR = 75.f, haloTime = 5.f, haloFrac = 0.3f, haloKnock = 520.f, haloEvery = 0.4f;   // ...and settle round the core
inline constexpr float tritonTime = 6.f, tritonSpeed = 2.8f, tritonFrac = 0.6f, tritonFreeze = 1.5f, tritonR = 7.f;   // Neptune: Triton, backwards
inline constexpr float beltOrbit = 420.f;               // px past the core's edge (the outermost)
inline constexpr int beltRocks = 16, beltRocksPerLevel = 4;
inline constexpr float beltFrac = 0.4f, beltRegrow = 6.f, beltSpeed = 0.25f;
inline constexpr float meteorFrac = 0.5f, meteorBlast = 45.f, meteorTime = 0.45f;       // a belt rock flung down on an enemy
inline constexpr float starFrac = 0.6f, starBlast = 34.f, starTime = 0.35f;             // "Starry night": a falling star
// the class (with any planet): contact hits x this, its planets x size
inline constexpr float contactMul = 0.75f, classSize = 1.1f;
inline constexpr float classPlating = 5.f;
inline constexpr float growPer = 0.015f;   // the class's passive: every kill this fight, the ball (and its planets) +3%...
inline constexpr int growMax = 20;        // ...up to this many
}  // namespace planet

// Shrines (2026-10-06): a map stop from act 2 on, one per act.
namespace shrine {
inline constexpr int fromAct = 4;
inline constexpr int maxAscended = 3;   // balls that can ascend in a run
inline constexpr int ascendPicks = 3;   // planets offered on ascending
}  // namespace shrine

// ==================================================================== Hermes
// The Striker's god (2026-10-06): speed. Few items, each a big one.
namespace hermes {
inline constexpr float cruise = 1.5f, top = 1.7f;      // the class: cruise / top speed x this (very fast from the start)
inline constexpr float regain = 12.f;                  // ...and it gets back up to speed almost at once after a hit
inline constexpr float speedDamage = 0.f;    // (its speed is its edge already)
inline constexpr float contactMul = 0.3f;      // ...so each hit is light: it lives on many, and on its message             // hits x (1 + this per cruise over its own)
inline constexpr float fastAt = 2.f;                   // "flying fast": over this x its cruise
inline constexpr float talaria = 0.12f, talariaPerLevel = 0.03f, talariaDecay = 0.3f;   // per wall bounce; fling fades x this
inline constexpr float caduceus = 0.3f, caduceusPerLevel = 0.08f, caduceusRange = 230.f;
inline constexpr float messengerEvery = 4.f, messengerPerLevel = -0.5f, messengerHit = 1.5f, messengerHitPerLevel = 0.3f;
inline constexpr float petasos = 0.25f, petasosPerLevel = 0.08f, boomR = 70.f, boomEvery = 0.12f;
inline constexpr float quicksilver = 0.3f, quicksilverPerLevel = 0.06f;
// The class's own way to fly (2026-10-06): it shies away from the core and
// leans toward the enemies; every bounce away from the core speeds it up, and
// walls and time flown fast load a message it delivers at the core - a hit on
// the whole arena.
inline constexpr float momentumPer = 0.03f, momentumMax = 0.5f;   // cruise + this per bounce, up to +this
inline constexpr float avoidRange = 230.f, avoidTurn = 14.f;       // px past the core's edge; rad/s at the closest
inline constexpr int deliverAfter = 100;                            // ...till this many bounces: then it goes for the core to deliver (not before)
inline constexpr float seekTurn = 1.1f, seekRange = 520.f;         // rad/s toward the nearest enemy
inline constexpr float chargePerWall = 0.2f, chargePerSecFast = 0.35f, chargeMax = 8.f;
inline constexpr float deliverFrac = 0.4f;                         // the arena hit: x its hit per point of charge
inline constexpr float deliverMin = 0.6f;                          // (less charge than this: nothing)
}  // namespace hermes

// ==================================================================== Odin
// The Mage's god (2026-10-06): ravens, runes, a spear.
namespace odin {
// (2026-10-06, toned down) Every recharge bonus - the class, Valknut, Focus,
// the web - goes into one sum with diminishing returns (World::mageCastRate):
// the first helps a lot, each more a little less, x3 at most. The all-cast,
// Huginn and Gungnir all come round at that rate.
inline constexpr float rate = 0.3f;                    // the class: + this recharge bonus...
inline constexpr float power = 1.0f;                   // ...abilities this much harder, wider...
inline constexpr float allEvery = 12.f;                // ...it casts only in volleys, all at once, every this-many s at its rate
inline constexpr float contactMul = 0.6f;
inline constexpr float killRefund = 0.04f;             // the class's passive: a kill brings its next volley this share sooner
inline constexpr float huginn = 1.f, huginnPerLevel = 0.f, burstGap = 0.25f;   // (on / off) every huginnEvery-th volley, 3 times running
inline constexpr int huginnCasts = 3, huginnEvery = 4;
inline constexpr float muninn = 0.5f, muninnPerLevel = 0.12f, blastR = 110.f, blastRange = 650.f;
inline constexpr float valknut = 0.4f, valknutPerLevel = 0.1f;   // + recharge bonus (into the same sum)
inline constexpr float gungnir = 6.f, gungnirPerLevel = 1.5f, gungnirEvery = 14.f, gungnirEveryPerLevel = -1.2f;   // s at its rate
inline constexpr int spearsMax = 10;                    // Gungnir: one more spear per spell cast since the last throw, up to this many more...
inline constexpr float spearExtra = 0.3f, spearGap = 0.07f;   // ...each this x its hit, thrown this many s apart
inline constexpr float spearTime = 0.3f, spearWidth = 18.f;   // s out (and as long back), px either side of the line
inline constexpr float draupnirPower = 1.5f, draupnirPowerPerLevel = 0.25f;   // abilities x this (damage, reach)...
inline constexpr int draupnirLevels = 1;               // ...and this many levels up (more of everything they throw)
}  // namespace odin

// ==================================================================== Thanatos
// The Assassin's god (2026-10-06): it latches onto an enemy and stabs it till
// it dies, then blinks to the next - one mark at a time.
namespace thanatos {
inline constexpr float stabEvery = 0.18f, stabMul = 1.7f;   // a stab from behind: x this
inline constexpr float blinkCd = 0.9f;                      // s between blinks (a kill halves what's left); free flight meanwhile
inline constexpr float daggerSpeed = 1500.f, daggerMin = 0.08f, daggerMax = 0.25f;   // the dagger thrown first: px/s, each flight (s)
inline constexpr int daggerBounces = 3;                                              // ...it bounces on this many times: to far ones, the way that cuts through most
inline constexpr float bounceRange = 800.f, daggerFrac = 0.6f, hopStep = 0.09f;      // ...how far; its cut (x the hit); s per blink along them
inline constexpr float daggerWidth = 26.f, passFrac = 0.25f;                         // everything it flies through on the way: px either side, a lighter cut
inline constexpr float blinkMin = 0.5f;                     // ...but never two blinks closer than this (Final hour aside)
inline constexpr float soulPer = 0.08f;                     // the passive: +this damage per kill this fight...
inline constexpr int soulMax = 15;
inline constexpr float shadowStep = 0.6f, shadowStepPerLevel = 0.15f, cutWidth = 22.f;
inline constexpr float deathMark = 0.25f, deathMarkPerLevel = 0.05f;
inline constexpr int eclipseEvery = 4;
inline constexpr float eclipse = 1.5f, eclipsePerLevel = 0.4f, eclipseR = 130.f;
inline constexpr int scythe = 2;
inline constexpr float finalHour = 15.f, finalHourPerLevel = -2.f, hourStep = 0.08f;
inline constexpr int hourBlinks = 12;
}  // namespace thanatos

// ==================================================================== Tiamat
// The Summoner's god (2026-10-06): a mount, dragons, lizards.
namespace tiamat {
inline constexpr float contactMul = 0.7f;
inline constexpr float cruise = 0.55f, top = 0.75f;   // the class: slow and heavy - the dragon carries her
inline constexpr float fireEvery = 1.f, fireRange = 520.f, fireR = 130.f, fireFrac = 3.5f, fireFlight = 0.55f;   // the mount's fireballs
inline constexpr float skillEvery = 3.5f;              // ...and a move of its own every so often (7 till 2026-10-07):
inline constexpr float roarR = 300.f, roarStun = 1.2f;               // roar: stuns what's near
inline constexpr float diveR = 150.f, diveFrac = 2.5f;               // dive: it swoops onto the thickest pack
inline constexpr float infernoR = 150.f, infernoTime = 4.f, infernoFrac = 1.2f;   // inferno: a ring of fire rides with it (dps)
inline constexpr int meteors = 3;                                    // meteors: called down on packs
inline constexpr float meteorR = 120.f, meteorFrac = 2.f;
// always at it, so it's never idle
inline constexpr float clawEvery = 0.35f, clawFrac = 1.8f, clawReach = 2.4f;     // claws: what's close (x its radius)
inline constexpr float emberEvery = 0.12f, emberLife = 2.5f, emberR = 26.f, emberFrac = 0.5f;   // its wake burns (dps)
inline constexpr int lizardMax = 18, broodMax = 24;   // (24 / 36 till 2026-10-07)
inline constexpr float breathEvery = 1.2f, breathRange = 260.f, breathCone = 0.4f, breathFrac = 1.4f;   // between its moves it breathes fire at what's near
inline constexpr float lizardLife = 10.f, lizardSpeed = 260.f, lizardBite = 0.5f, lizardFrac = 1.1f;
inline constexpr float hordeEvery = 5.f, hordeEveryPerLevel = -0.6f;
inline constexpr int hordeSize = 6;
inline constexpr float dragonPerLevel = 0.3f;
// Growth (2026-10-07): each dragon starts as an egg and grows all run long - every enemy that dies, whoever
// killed it, feeds it (x its rate). Stages: 0 egg, 1 hatchling, 2 young, 3 adult - each stage a new move.
// The quick growers stay small; the slow ones end up huge and hit harder.
inline constexpr float stageXp[3] = {3.f, 60.f, 200.f};   // xp to hatch / to be young / to be adult
inline constexpr float killXp = 0.5f;
//                                    frost  drake  gold  ravager wyrmling
inline constexpr float growRate[5]  = {0.7f, 1.6f, 0.5f, 1.f, 2.f};
inline constexpr float adultSize[5] = {46.f, 28.f, 58.f, 38.f, 18.f};   // body length, fully grown (px)
inline constexpr float adultPow[5]  = {1.15f, 0.85f, 1.3f, 1.f, 0.8f};  // its hit, fully grown
inline constexpr float hatchSize = 0.35f, hatchPow = 0.45f;               // a fresh hatchling, of the adult's
inline constexpr float mountRate = 1.f;                                   // her mount: always ridden (a hatchling at first), it grows too:
inline constexpr float mountSize = 0.6f, mountPow = 0.65f;                // ...fresh, of its full size and hit. Young: embers + its moves; adult: every move, sooner
inline constexpr float mountSkillAdult = 2.5f;
inline constexpr int wyrmBody[4] = {0, 6, 9, 14};                        // the frost wyrm's body segments by stage
inline constexpr float flyTurn[5] = {0.f, 2.6f, 1.3f, 2.f, 0.f};          // how hard each turns on the wing (rad/s): it carves arcs
// on the wing: a few beats, then a glide on still wings while it loses speed; each downstroke a push
inline constexpr float beatRate = 5.f, beatEffort = 3.5f, beatPush = 0.9f;   // wingbeats (rad/s, faster when it strains), the push of each
inline constexpr float glideMin = 0.5f, glideMax = 1.3f, glideDrag = 0.35f;  // a glide: how long, how fast it bleeds speed
// the ravager's run: it veers off, wings back, then hooks round and rushes in, steering as it goes
inline constexpr float windup = 0.28f, runTurn = 3.2f, runAccel = 5.f, runMax = 1.8f;
// the frost wyrm: circles the core; what its body touches freezes (once a lap), and it breathes ice outward
inline constexpr float wyrmOrbit = 210.f, wyrmSpin = 1.6f, wyrmEvery = 3.f, wyrmRange = 330.f, wyrmFrac = 1.f, wyrmFreeze = 1.f;
inline constexpr float coilR = 20.f, coilFrac = 0.8f, coilFreeze = 1.2f;   // its body: reach round each segment, hit, freeze
inline constexpr float patchR = 70.f, patchLife = 3.f, patchFrac = 0.3f;   // an adult's breath leaves frost where it lands (dps)
inline constexpr float staticEvery = 0.5f, staticRange = 190.f, staticFrac = 0.35f;            // the drake's field zaps what's near
// they roam the field (never still): speeds px/s
inline constexpr float drakeSpeed = 175.f, goldSpeed = 120.f;
inline constexpr float orbEvery = 1.6f, orbFrac = 0.6f, orbZapR = 75.f, orbZapEvery = 0.2f, orbTime = 1.3f;   // the drake's ball lightning
// an adult drake's storm: clouds over the thickest packs, drifting after them; each bolt blasts an area
inline constexpr float stormEvery = 5.f, stormR = 150.f, stormTime = 4.f, stormStrikeEvery = 0.45f, stormFrac = 2.f;
inline constexpr float boltR = 75.f, cloudDrift = 60.f;
inline constexpr int stormClouds = 2;
// the three-headed gold: each head its own - a flamethrower, molten gold, a pulling roar
inline constexpr float flameEvery = 0.3f, flameRange = 220.f, flameFrac = 0.9f;
inline constexpr float globHeadEvery = 1.5f;
inline constexpr float pullEvery = 4.f, pullR = 280.f, pullSpeed = 520.f;
// the ravager: charges straight through the thickest pack, hitting all on its way; its kills hatch lizards
inline constexpr float ravageSpeed = 130.f, ravageEvery = 1.8f, ravageEveryPerLevel = -0.25f;   // roaming between runs
inline constexpr float ravageRun = 900.f, ravageWidth = 42.f, ravageFrac = 3.f, ravagePast = 220.f;   // the run: px/s, its reach, past the pack
// the three-headed gold's heads: how far each turns from its body (rad), how fast, how often it picks another enemy
inline constexpr float headSwing = 1.3f, headTurn = 6.f, headPickMin = 1.2f, headPickMax = 3.f;
// the wyrmling: darts from enemy to enemy finishing the weakest; each kill it grows (harder bites) for the fight
inline constexpr float wyrmlingSpeed = 360.f, wyrmlingRange = 450.f, biteEvery = 0.3f, biteFrac = 1.8f;
inline constexpr float pounceTime = 0.5f, pounceSpeed = 1.8f, splashR = 45.f, splashFrac = 0.5f;   // young: a pounce after a kill; adult: bites splash
inline constexpr float globEvery = 2.5f, globFrac = 2.6f, globR = 90.f;                             // the gold spits molten gold
inline constexpr float shadowR = 110.f, shadowFrac = 0.4f, moltenR = 120.f, moltenLife = 3.f, moltenFrac = 1.f;   // the gold's shadow, its molten pool
inline constexpr float drakeEvery = 2.f, drakeFrac = 0.8f, drakeRange = 420.f, drakeMove = 8.f;
inline constexpr int drakeChain = 4;
inline constexpr float goldEvery = 7.f, goldDive = 0.6f, goldR = 160.f, goldFrac = 4.f;
}  // namespace tiamat

// ==================================================================== Loki
// The Jester's god (2026-10-07): clones playing games of chance.
namespace loki {
inline constexpr float contactMul = 1.f;
inline constexpr float cloneEvery = 0.45f, cloneLife = 4.f, cloneSpeed = 240.f, cloneSteer = 1.2f;   // its clones fly about, drifting at the enemies
inline constexpr int clones = 3, clonesHigh = 4;
inline constexpr float killClone = 0.25f;                         // the passive: a kill may split off a clone at once
inline constexpr float flyMin = 0.3f, flyMax = 0.6f, gameShow = 0.45f;   // each clone flies a moment, plays one game (it spins this long), pays, and is gone
inline constexpr float perLevel = 0.25f;                          // its items: each level, +this on that game's payout
inline constexpr float coinHeads = 8.f, coinTails = 3.5f, coinRange = 520.f, coinR = 70.f;   // the coin: heads a bolt (x the hit, round where it lands)
inline constexpr float diceFrac = 2.f, diceRange = 500.f;                                  // dice: that many knives
inline constexpr float redFrac = 6.f, redR = 130.f;                                         // roulette: red a fire nova...
inline constexpr float blackR = 260.f, blackPull = 480.f, blackStun = 1.f;                  // ...black drags in and stuns...
inline constexpr float green = 0.1f, jackpotFrac = 16.f, jackpotR = 260.f;                   // ...green (this often) the jackpot
inline constexpr int jackpotGold = 6;
inline constexpr float cardFrac = 4.f, slashLen = 420.f, slashW = 30.f, clubR = 130.f, clubStun = 0.6f, heartHeal = 1.f;   // cards
inline constexpr int shardTargets = 3;
inline constexpr float pachinkoEvery = 2.f, pachinkoFrac = 1.8f, pinGravity = 900.f, pinLife = 4.f;   // pachinko: its steel balls
inline constexpr int pachinkoBalls = 14, pinMax = 140;
inline constexpr float highMul = 1.5f, highEvery = 0.32f, highPerLevel = 0.15f;
}  // namespace loki

// ==================================================================== Nergal
// The Alchemist's god (2026-10-07): plague and pestilence.
namespace nergal {
inline constexpr float contactMul = 0.85f;
inline constexpr float rotTime = 4.f, rotFrac = 0.12f, rotMax = 0.5f;   // every hit infects: + this x the hit a second, up to rotMax x, for rotTime
inline constexpr int ratsPerDeath = 1, ratMax = 20;                    // the infected die into rats
inline constexpr float ratLife = 8.f, ratSpeed = 240.f, ratBite = 0.5f, ratFrac = 0.2f;
// the class: a trail of swarms behind it, each hunting what's near
inline constexpr float swarmEvery = 0.5f, swarmLife = 3.f, swarmR = 38.f, swarmFrac = 0.3f, swarmSpeed = 190.f, swarmRange = 260.f;
inline constexpr int swarmMax = 14;
inline constexpr float locustMul = 2.f, locustPerLevel = 0.3f;         // "Locust swarm": swarms x2 as often, bigger, harder, longer
inline constexpr float rainEvery = 6.f, rainPerLevel = -0.6f, rainR = 230.f, rainTime = 4.f, rainFrac = 0.35f, rainDrift = 45.f, rainMark = 0.6f;
inline constexpr float tideEvery = 5.f, tidePerLevel = -0.5f, tideBite = 1.5f;
inline constexpr int tideSize = 8;
inline constexpr float deathR = 100.f, deathFrac = 0.6f, deathPerLevel = 0.15f;   // (it infects, it doesn't chain bursts)
inline constexpr float horseEvery = 10.f, horsePerLevel = -1.f, horseFrac = 3.f, horseW = 55.f, horseSpeed = 1100.f;
// the black plague meeting each element (x the hit)
inline constexpr float pyreR = 120.f, pyreFrac = 1.6f;                 // fire: a pyre - burns, and spreads the rot
inline constexpr float coldR = 110.f, coldFrac = 1.f, coldFreeze = 1.5f;   // ice: cold death - freezes, the rot cracks out at once
inline constexpr float pestR = 150.f;                                  // poison: pestilence - the rot doubles and spreads
inline constexpr float feverRange = 300.f, feverFrac = 1.2f;           // electric: fever - black lightning through the infected
inline constexpr int feverJumps = 6;
inline constexpr float miasmaR = 110.f, miasmaTime = 3.f, miasmaFrac = 0.6f;   // water: a miasma - a black cloud that lingers
inline constexpr float graveR = 120.f, graveFrac = 1.5f, graveStun = 1.2f;     // stone: a mass grave - the ground swallows, rats crawl out
inline constexpr int graveRats = 2;
}  // namespace nergal

// ==================================================================== Apolo
// The Support's god (2026-10-07): the muses and their gifts.
namespace apolo {
inline constexpr float contactMul = 0.9f;
inline constexpr float rotEvery = 5.f, killHold = 0.5f;           // the muses move on this often; a kill holds them a little
inline constexpr float strength = 1.f, speed = 0.8f, giantSize = 0.5f, giantDamage = 0.5f;   // the gifts
inline constexpr float markTime = 0.3f;        // each muse's weak point: kept until it dies, then the enemy nearest her
inline constexpr float lyreHeal = 3.f, lyrePerLevel = 1.f, lyreLinger = 3.f, lyreHalf = 0.5f;
inline constexpr float laurel = 0.4f, laurelPerLevel = 0.15f;
inline constexpr float bowEvery = 0.8f, bowFrac = 2.f, bowPerLevel = 0.3f;
inline constexpr float sunR = 95.f, sunFrac = 1.f, sunPerLevel = 0.25f, sunTick = 0.25f;
inline constexpr float oracle = 3.f, oraclePerLevel = 0.5f;
}  // namespace apolo

// ==================================================================== Ares
// The Shooter's god (2026-10-07): an arsenal; every item one more weapon.
namespace ares {
inline constexpr float contactMul = 0.4f;
inline constexpr int cannons = 6;                                   // the class: ghost cannons round it...
inline constexpr float fireEvery = 0.1f, bulletFrac = 0.55f, range = 650.f;   // ...one fires this often, this x its hit, at what's in reach
inline constexpr float lustPerKill = 0.08f, lustMax = 1.f, lustHold = 2.f, lustFade = 0.25f;   // bloodlust: fire rate + this per kill; held, then fades /s
inline constexpr float perLevel = 0.2f;                             // every weapon: x(1 + this per level past 1)
inline constexpr float moonEvery = 1.1f, missileFrac = 1.5f, missileSpeed = 520.f, missileTurn = 5.f, missileR = 60.f, fear = 0.7f;   // Phobos & Deimos
inline constexpr float teethEvery = 6.f, hopliteLife = 8.f, sprout = 0.6f, volleyEvery = 1.f, volleyFrac = 0.35f;        // Dragon's teeth
inline constexpr int teeth = 3, hopliteMax = 9, volleyBullets = 3;
inline constexpr float chariotEvery = 7.f, chariotFrac = 2.f, chariotSpeed = 900.f, flameEvery = 0.05f, flameLife = 3.f, flameR = 32.f, flameFrac = 0.6f;   // War chariot
inline constexpr float wrathEvery = 2.5f, wrathWarn = 0.8f, wrathR = 150.f, wrathFrac = 6.f, craterLife = 3.f, craterFrac = 0.5f;          // Wrath of Olympus
inline constexpr float troyEvery = 10.f, troyBand = 140.f, troyTime = 1.2f, troyR = 75.f, troyFrac = 2.5f;                                 // Trojan war
inline constexpr int troySpears = 14;
inline constexpr int wrathMax = 9;   // Wrath of Olympus: stars falling + craters burning, at most this many at once
}  // namespace ares

// ==================================================================== Artemisa
// The Slinger's god (2026-10-07): the hunt, all on the throw.
namespace artemisa {
inline constexpr float contactMul = 0.8f, throwMul = 1.6f;
inline constexpr float autoEvery = 4.f, autoPull = 0.85f;                 // left alone this long, she throws herself (this hard a pull)
inline constexpr float arrowFrac = 0.8f, arrowWidth = 14.f;               // the silver arrow down the throw's line
inline constexpr float preyTime = 6.f, preyMul = 2.f;                     // its prey: marked; the ball's first hit on it x this
inline constexpr float perLevel = 0.2f;
inline constexpr float houndSpeed = 900.f, houndLife = 2.5f, houndFrac = 1.f, houndBite = 24.f;   // Silver hounds
inline constexpr float echoDelay = 0.12f, echoLife = 3.f;                 // Crescent moon: two balls on the throw's path, this far behind
inline constexpr float rainR = 70.f, rainFrac = 1.2f, rainGap = 0.08f, rainFall = 0.3f;   // Arrow rain
inline constexpr int rainMax = 12;
inline constexpr float stagPull = 0.9f, stagSpeed = 1400.f, stagFrac = 3.f, stagW = 40.f;  // Golden stag
inline constexpr float nightEvery = 15.f, nightTime = 5.f, nightSpeed = 2600.f, nightFrac = 1.5f;   // Night of the hunt
inline constexpr int nightMax = 40;
}  // namespace artemisa

}  // namespace sb::cfg
