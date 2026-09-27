#pragma once

#include <vector>

#include "progression/UpgradeKind.hpp"

// The newer classes' items, one table per class. A class adds an item by:
//   1. an enum value in its section of UpgradeKind (progression/UpgradeKind.hpp),
//   2. one ItemDef line in its table below (title, text, tier, tag),
//   3. what it does per level in its section of core/ClassSpec.cpp (the ball's
//      numbers) and sim/WorldClasses.cpp (the behaviour).
// Everything else - card text, tooltips, rolls, shops, the class-unlock gate,
// SB_UPGRADES - reads these tables through classItemDef().

namespace sb {

inline const ItemDef* findItemDef(const std::vector<ItemDef>& defs, UpgradeKind k) {
    for (const ItemDef& d : defs)
        if (d.kind == k) return &d;
    return nullptr;
}

// ==================================================================== Mage
inline const ItemDef* mageItemDef(UpgradeKind k) {
    static const std::vector<ItemDef> defs = {
        // {UpgradeKind::Example, "Example", "Example", "what it does", "what one more level does", Tier::Rare, ItemTag::Mage},
    };
    return findItemDef(defs, k);
}

// ==================================================================== Shooter
inline const ItemDef* shooterItemDef(UpgradeKind k) {
    static const std::vector<ItemDef> defs = {
    };
    return findItemDef(defs, k);
}

// ==================================================================== Assassin
inline const ItemDef* assassinItemDef(UpgradeKind k) {
    static const std::vector<ItemDef> defs = {
    };
    return findItemDef(defs, k);
}

// ==================================================================== Summoner
inline const ItemDef* summonerItemDef(UpgradeKind k) {
    static const std::vector<ItemDef> defs = {
    };
    return findItemDef(defs, k);
}

// ==================================================================== Jester
inline const ItemDef* jesterItemDef(UpgradeKind k) {
    static const std::vector<ItemDef> defs = {
        {UpgradeKind::LuckyCharm, "LuckyCharm", "Lucky charm",
         "+2 luck for the run: every chance is higher and cards roll rarer (charms on every ball add up)",
         "+1 luck", Tier::Common, ItemTag::Jester},
        {UpgradeKind::CoinFlip, "CoinFlip", "Coin flip",
         "every hit flips a coin: heads x2 damage, tails x0.6 (luck favours heads)",
         "heads hit +0.25x harder", Tier::Common, ItemTag::Jester},
        {UpgradeKind::WildCard, "WildCard", "Wild card",
         "15% chance a hit fires a random trick borrowed from any ball's items (crit, echo, zap, bomb, black hole)",
         "+5% chance, the borrowed trick hits 10% harder", Tier::Uncommon, ItemTag::Jester},
        {UpgradeKind::Reroll, "Reroll", "Reroll",
         "a missed Jester roll (outcome, coin, wild card, jackpot) gets a second try 35% of the time",
         "+15% chance of a second try", Tier::Uncommon, ItemTag::Jester},
        {UpgradeKind::ChaosBounce, "ChaosBounce", "Chaos bounce",
         "wall bounces fly off at a random angle and arm its next hit at x1.5",
         "the armed hit +0.15x harder", Tier::Rare, ItemTag::Jester},
        {UpgradeKind::Jackpot, "Jackpot", "Jackpot",
         "4% chance a kill hits the jackpot: 10 gold and a big blast (x3 the killing hit)",
         "+1.5% chance, +5 gold, a bigger blast", Tier::Epic, ItemTag::Jester},
    };
    return findItemDef(defs, k);
}

// ---------------------------------------------------------------- lookup
inline const ItemDef* classItemDef(UpgradeKind k) {
    for (const ItemDef* (*f)(UpgradeKind) : {mageItemDef, shooterItemDef, assassinItemDef, summonerItemDef, jesterItemDef})
        if (const ItemDef* d = f(k)) return d;
    return nullptr;
}

}  // namespace sb
