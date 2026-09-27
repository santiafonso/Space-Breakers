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
        // Every Mage item works through the ball's abilities (casts and
        // cooldowns), so it helps any ball that carries one; Arcane missile
        // also fires on its own timer.
        {UpgradeKind::Focus, "Focus", "Focus",
         "its abilities recharge 15% faster",
         "+7% faster", Tier::Common, ItemTag::Mage},
        {UpgradeKind::ArcaneMissile, "ArcaneMissile", "Arcane missile",
         "every 4.5 s, and on every ability cast, an arcane missile strikes the nearest enemy (60% of its hit, with its element)",
         "harder and more often; a 2nd target at level 3, a 3rd at level 5", Tier::Common, ItemTag::Mage},
        {UpgradeKind::Attunement, "Attunement", "Attunement",
         "its abilities (and arcane missiles) hit 30% harder and reach further",
         "+10% harder", Tier::Uncommon, ItemTag::Mage},
        {UpgradeKind::Twincast, "Twincast", "Twincast",
         "30% chance an ability fires a second time a moment later",
         "+8% chance", Tier::Rare, ItemTag::Mage},
        {UpgradeKind::ManaSpring, "ManaSpring", "Mana spring",
         "every ability it casts recharges its other abilities by 25% - with several, they chain",
         "+7% of their cooldown", Tier::Epic, ItemTag::Mage},
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
