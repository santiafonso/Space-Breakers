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
        {UpgradeKind::SummonTurret, "SummonTurret", "Turret",
         "a wall bounce plants a small turret there that shoots the nearest enemy for a few seconds",
         "turrets fire faster, hit harder and last longer; one more at levels 3 and 5",
         Tier::Common, ItemTag::Summoner},
        {UpgradeKind::SummonWisps, "SummonWisps", "Wisps",
         "every kill lets loose a wisp that homes in on an enemy and bursts with the ball's element",
         "wisps hit harder; 2 per kill at level 3, 3 at level 5",
         Tier::Common, ItemTag::Summoner},
        {UpgradeKind::SummonTotem, "SummonTotem", "Totem",
         "with enemies close, it plants a totem that slows everything around it and pulses a little damage",
         "a wider, stronger, longer-lasting totem, planted more often",
         Tier::Uncommon, ItemTag::Summoner},
        {UpgradeKind::SummonWarden, "SummonWarden", "Warden",
         "two spirits circle the core and strike any enemy they touch, shoving it back",
         "spirits hit harder; one more at levels 3 and 5",
         Tier::Uncommon, ItemTag::Summoner},
        {UpgradeKind::SummonDragon, "SummonDragon", "Dragonling",
         "a small dragon flies with the ball and breathes its element in a cone at the nearest enemy",
         "hotter, wider and more often",
         Tier::Rare, ItemTag::Summoner},
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
