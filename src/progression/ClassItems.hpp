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
