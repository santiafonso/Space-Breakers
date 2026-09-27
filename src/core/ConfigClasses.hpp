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
}  // namespace summoner

// ==================================================================== Jester
namespace jester {
}  // namespace jester

}  // namespace sb::cfg
