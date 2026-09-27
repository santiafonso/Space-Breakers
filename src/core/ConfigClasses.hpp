#pragma once

// Per-class tuning. Striker / Guardian / Support keep theirs in cfg::role and
// cfg::synergy (Config.hpp); every newer class owns exactly one namespace
// below. Level-1 values, then + ...PerLevel for each item level past it, like
// the rest of the item numbers.
namespace sb::cfg {

// The class framework itself.
namespace classes {
inline constexpr int startItems = 2;   // "Calling": the starting ball's items of the chosen class
}  // namespace classes

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
// Arcane missile: one volley on a timer and one on every cast.
inline constexpr float missileEvery = 4.5f, missileEveryPerLevel = -0.4f;   // seconds between timed volleys
inline constexpr float missileFrac = 0.6f, missileFracPerLevel = 0.12f;     // damage x the ball's hit
inline constexpr float missileRange = 480.f;
inline constexpr float missileLife = 0.22f;    // the streak's fade (visual)
inline constexpr int maxStreaks = 48;
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
}  // namespace assassin

// ==================================================================== Summoner
namespace summoner {
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

}  // namespace sb::cfg
