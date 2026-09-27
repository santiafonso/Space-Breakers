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
        {UpgradeKind::Backstab, "backstab", "Backstab",
         "the first hit after a blink deals x1.5 damage", "+0.2x", Tier::Common, ItemTag::Assassin},
        {UpgradeKind::Cull, "cull", "Cull",
         "a hit that leaves an enemy under 14% health finishes it (and the kill blinks on)",
         "+3% threshold", Tier::Common, ItemTag::Assassin},
        {UpgradeKind::KillingSpree, "spree", "Killing spree",
         "each blink in a chain adds +8% damage (up to 5); 2.5 s without a blink ends the chain",
         "+3% per blink, +1 max", Tier::Uncommon, ItemTag::Assassin},
        {UpgradeKind::ShadowTrail, "shadowtrail", "Shadow trail",
         "the blink cuts every enemy on its path for 45% of the ball's hit",
         "+15% of the hit", Tier::Uncommon, ItemTag::Assassin},
        {UpgradeKind::SmokeBomb, "smokebomb", "Smoke bomb",
         "where it lands, a burst hits everything near for 60% of its hit and leaves its element",
         "+15% of the hit, wider", Tier::Rare, ItemTag::Assassin},
        {UpgradeKind::Phantom, "phantom", "Phantom",
         "each blink leaves a shadow copy behind (1.6 s, same items) that dives at the next enemy",
         "the copy lasts +0.4 s", Tier::Epic, ItemTag::Assassin},
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
