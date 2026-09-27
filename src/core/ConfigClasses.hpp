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
