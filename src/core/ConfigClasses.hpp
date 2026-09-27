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
