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
        {UpgradeKind::RapidFire, "RapidFire", "Rapid fire", "fires bullets 30% more often",
         "+15% more fire rate", Tier::Common, ItemTag::Shooter},
        {UpgradeKind::Scattershot, "Scattershot", "Scattershot", "each volley is a fan of 2 pellets (x0.75 damage each)",
         "one more pellet at Lv 3 and 5, +6% pellet damage", Tier::Common, ItemTag::Shooter},
        {UpgradeKind::Rebound, "Rebound", "Rebound", "a bullet that hits hops on to another enemy nearby (x0.8 damage)",
         "+1 hop, keeps +4% damage per hop", Tier::Uncommon, ItemTag::Shooter},
        {UpgradeKind::Tracer, "Tracer", "Tracer", "40% of its bullets carry the ball's element - they can set off reactions",
         "+15% chance, +5% bullet damage", Tier::Uncommon, ItemTag::Shooter},
        {UpgradeKind::DrillRounds, "DrillRounds", "Drill rounds", "bullets punch through 2 enemies and through shields, x1.15 damage",
         "+1 enemy pierced, +8% bullet damage", Tier::Rare, ItemTag::Shooter},
        {UpgradeKind::HairTrigger, "HairTrigger", "Hair trigger", "when it hits an enemy it fires 3 bullets at the ones around it",
         "+1 bullet per burst, +5% burst damage", Tier::Rare, ItemTag::Shooter},
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
