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
}  // namespace shooter

// ==================================================================== Assassin
namespace assassin {
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
inline constexpr float wardenSpin = 2.2f;        // rad/s
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
}  // namespace jester

}  // namespace sb::cfg
