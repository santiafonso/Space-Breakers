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
inline constexpr float hitRefund = 0.25f;
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
inline constexpr float manaSpring = 0.25f, manaSpringPerLevel = 0.07f;
}  // namespace mage

// ==================================================================== Shooter
namespace shooter {
// Role base: a bullet at the nearest enemy in range every `fireInterval`
// seconds, faster the faster the ball flies (x rate = speed / cruise, clamped).
inline constexpr float fireInterval = 0.9f;     // s between volleys at cruise
inline constexpr float rateMin = 0.75f;         // speed factor floor (slow ball)
inline constexpr float rateMax = 2.0f;          // ...and ceiling (flung hard)
inline constexpr float range = 330.f;           // px: how far it looks for a target
inline constexpr float bulletFrac = 0.35f;      // bullet damage = the ball's hit x this
inline constexpr float bulletSpeed = 620.f;     // px/s
inline constexpr float bulletLife = 0.9f;       // s
inline constexpr float bulletRadius = 2.5f;     // px (hit size; drawn smaller)
inline constexpr float bossFrac = 0.5f;         // bullets chip the boss for this much
inline constexpr int maxBullets = 90;           // cap on live bullets (perf)
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
// Dragonling (Rare): a small dragon trails the ball and breathes its element
// in a cone at the nearest enemy.
inline constexpr float dragonInterval = 1.7f, dragonIntervalPerLevel = -0.15f;
inline constexpr float dragonFrac = 0.8f, dragonFracPerLevel = 0.2f;
inline constexpr float dragonRange = 250.f;
inline constexpr float dragonCone = 0.45f, dragonConePerLevel = 0.05f;  // half-angle, radians
inline constexpr float dragonHover = 26.f;       // how far from the ball it flies
inline constexpr float dragonFollow = 6.f;       // catch-up rate
inline constexpr float breathLife = 0.3f;        // the cone's fading visual
inline constexpr float dragonFreeze = 0.5f;      // its ice freezes this long (x potency)
// World caps, for performance.
inline constexpr int maxShots = 48;              // turret shots + wisps
inline constexpr int maxTurrets = 8;
inline constexpr int maxTotems = 6;
inline constexpr int maxBreaths = 8;
}  // namespace summoner

// ==================================================================== Jester
namespace jester {
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
inline constexpr float doubleWin = 2.5f, doubleWinPerLevel = 0.3f;
inline constexpr float doubleChance = 0.5f;
// "Execution throw": thrown first hit on an enemy at full health x this.
inline constexpr float execution = 2.2f, executionPerLevel = 0.3f;
}  // namespace slinger

// ==================================================================== Alchemist
namespace alchemist {
inline constexpr float archReaction = 1.3f;    // Archalchemist: its reactions x this
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

}  // namespace sb::cfg
