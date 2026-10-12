#pragma once

// Ascension (2026-10-12): the challenge levels past a won run. Each level adds
// its rule on top of every rule below it. Numbers in cfg::asc.

namespace sb {

inline const char* ascensionRule(int level) {
    switch (level) {
        case 1:  return "elites have +25% health and bring a second Brute from act 1";
        case 2:  return "a new act repairs only 25% of the core (instead of 50%)";
        case 3:  return "enemies move 10% faster";
        case 4:  return "a Rest repairs half the core, and shops cost 25% more";
        case 5:  return "bosses change phase sooner and rage harder";
        case 6:  return "every elite fight has a trait: Armored, Swift, Regenerating or Vengeful";
        case 7:  return "the damage combo drains twice as fast";
        case 8:  return "picks deal one card fewer";
        case 9:  return "every boss has a rule: Thick hide, Warded or Hex";
        case 10: return "every enemy and boss +15% health, and the last boss has two rules";
        default: return "";
    }
}

// Elite fight traits (level 6) and boss rules (level 9). Not saved: rolled at
// each fight's start.
enum class EliteTrait { None, Armored, Swift, Regenerating, Vengeful };
inline const char* eliteTraitName(EliteTrait t) {
    switch (t) {
        case EliteTrait::Armored:      return "armored";
        case EliteTrait::Swift:        return "swift";
        case EliteTrait::Regenerating: return "regenerating";
        case EliteTrait::Vengeful:     return "vengeful";
        default:                       return "";
    }
}
inline const char* eliteTraitDesc(EliteTrait t) {
    switch (t) {
        case EliteTrait::Armored:      return "its enemies take 30% less damage";
        case EliteTrait::Swift:        return "its enemies move 30% faster";
        case EliteTrait::Regenerating: return "its enemies heal 3% of their health every second";
        case EliteTrait::Vengeful:     return "its enemies hit the core 50% harder";
        default:                       return "";
    }
}

enum BossRule : unsigned { BossRuleThickHide = 1u, BossRuleWarded = 2u, BossRuleHex = 4u };
inline constexpr unsigned kBossRules[] = {BossRuleThickHide, BossRuleWarded, BossRuleHex};
inline const char* bossRuleName(unsigned r) {
    switch (r) {
        case BossRuleThickHide: return "thick hide";
        case BossRuleWarded:    return "warded";
        case BossRuleHex:       return "hex";
        default:                return "";
    }
}
inline const char* bossRuleDesc(unsigned r) {
    switch (r) {
        case BossRuleThickHide: return "balls hitting it deal 60% less - elements, abilities and summons hurt it fully";
        case BossRuleWarded:    return "only ball hits hurt it fully - elements, abilities and summons deal 60% less";
        case BossRuleHex:       return "every 11 s it freezes your strongest ball for 3 s";
        default:                return "";
    }
}

}  // namespace sb
