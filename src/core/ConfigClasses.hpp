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
namespace mage {
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
}  // namespace jester

}  // namespace sb::cfg
