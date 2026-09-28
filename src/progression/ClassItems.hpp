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
        // cooldowns), so it helps any ball that carries one.
        {UpgradeKind::Focus, "Focus", "Focus",
         "its abilities recharge 15% faster",
         "+7% faster", Tier::Common, ItemTag::Mage},
        {UpgradeKind::ArcaneMissile, "Barrage", "Barrage",
         "Magic missile looses +1 missile, and every other ability it casts looses a magic missile too (60% of its hit)",
         "missiles +10% harder, the free one +12%; +1 more missile at levels 3 and 5", Tier::Common, ItemTag::Mage},
        {UpgradeKind::Attunement, "Attunement", "Attunement",
         "its abilities (and magic missiles) hit 30% harder and reach further",
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

// ==================================================================== Slinger
inline const ItemDef* slingerItemDef(UpgradeKind k) {
    static const std::vector<ItemDef> defs = {
        // Every Slinger item acts on your throws and catches, so it works on
        // any ball you like to handle.
        {UpgradeKind::Momentum, "Momentum", "Momentum",
         "the faster it flies, the harder it hits: +35% per cruise of speed above its own, no cap",
         "+10% per cruise", Tier::Common, ItemTag::Slinger},
        {UpgradeKind::Grip, "Grip", "Grip",
         "when it's near your pointer it curves toward it - easier to catch",
         "turns harder, from further", Tier::Common, ItemTag::Slinger},
        {UpgradeKind::CatchRelease, "CatchRelease", "Catch & release",
         "catch it within 2.5 s of your throw: +15% damage, stacking up to 5 (a slow catch drops the stacks)",
         "+4% per stack", Tier::Common, ItemTag::Slinger},
        {UpgradeKind::Coil, "Coil", "Coil",
         "left alone it slows down to a stop - but when you throw it, it flies twice as fast",
         "throws +20% faster, coasts longer", Tier::Uncommon, ItemTag::Slinger},
        {UpgradeKind::Afterburner, "Afterburner", "Afterburner",
         "for 1.4 s after your throw, while it flies fast, it leaves a trail of fire: it burns (60% of its hit per s) and sets off fire reactions",
         "hotter (+15%) and longer (+0.2 s)", Tier::Uncommon, ItemTag::Slinger},
        {UpgradeKind::ExecutionThrow, "ExecutionThrow", "Execution throw",
         "the first hit after your throw, on an enemy at full health, deals x2.2",
         "+0.3x", Tier::Uncommon, ItemTag::Slinger},
        {UpgradeKind::Ambush, "Ambush", "Ambush",
         "the first hit after your throw blinks it on to the nearest other enemy, striking it for x1.5",
         "+0.2x", Tier::Rare, ItemTag::Slinger},
        {UpgradeKind::TrickShot, "TrickShot", "Trick shot",
         "from your throw until its first hit, every chance it has (crits, echoes, zaps, Jester rolls) is x3",
         "x0.5 more", Tier::Rare, ItemTag::Slinger},
        {UpgradeKind::DoubleDown, "DoubleDown", "Double down",
         "catch it within 2.5 s of your throw and its next hit is double or nothing: x2.5 or a miss (luck favours the win)",
         "the win +0.3x", Tier::Epic, ItemTag::Slinger},
    };
    return findItemDef(defs, k);
}

// ==================================================================== speed items
// The other classes' speed items (2026-09-28): a "still" one grows as the
// ball slows (full at a standstill), a "moving" one as it speeds up. None is
// on / off - a slow ball just leans into it.
inline const ItemDef* styleItemDef(UpgradeKind k) {
    static const std::vector<ItemDef> defs = {
        {UpgradeKind::Anchor, "Anchor", "Anchor",
         "the slower it moves, the more the enemies near it are slowed (up to 55%) and drawn toward it",
         "+8% slow, wider, a stronger pull", Tier::Uncommon, ItemTag::Guardian},
        {UpgradeKind::Plow, "Plow", "Plow",
         "flying faster than its cruise, it shoves aside every enemy it passes and hits it for 30% of its hit",
         "a harder shove, +10% of its hit", Tier::Uncommon, ItemTag::Guardian},
        {UpgradeKind::Slug, "Slug", "Slug",
         "the slower it moves, the faster it fires (up to x2.2) and the further it reaches",
         "+30% fire rate at a standstill", Tier::Common, ItemTag::Shooter},
        {UpgradeKind::Strafe, "Strafe", "Strafe",
         "flying faster than its cruise, it fires volleys out to both sides (60% of a bullet)",
         "+12% of a bullet", Tier::Uncommon, ItemTag::Shooter},
        {UpgradeKind::Lurk, "Lurk", "Lurk",
         "staying slow charges its next hit after a blink: up to +150% after 2.5 s nearly still",
         "+30% at full charge", Tier::Uncommon, ItemTag::Assassin},
        {UpgradeKind::Blur, "Blur", "Blur",
         "well above its cruise it passes through enemies instead of bouncing, marking every one it cuts",
         "marks last longer", Tier::Rare, ItemTag::Assassin},
        {UpgradeKind::Sleight, "Sleight", "Sleight",
         "the slower it moves, the sooner it vanishes and reappears on a random enemy, striking it for x1.4",
         "sooner, +0.2x", Tier::Uncommon, ItemTag::Jester},
        {UpgradeKind::Meditate, "Meditate", "Meditate",
         "the slower it moves, the faster its abilities recharge (twice as fast at a standstill)",
         "+25% at a standstill", Tier::Common, ItemTag::Mage},
        {UpgradeKind::Leyline, "Leyline", "Leyline",
         "moving, it drops runes (up to 6); each ability it casts bursts every rune for 80% of its hit, with its element",
         "+20% of its hit", Tier::Uncommon, ItemTag::Mage},
        {UpgradeKind::Beacon, "Beacon", "Beacon",
         "the slower it moves, the wider its glow: balls passing through it hit +30% harder next and leave its element (reactions!)",
         "+8% harder, wider", Tier::Uncommon, ItemTag::Support},
        {UpgradeKind::Wake, "Wake", "Wake",
         "flying fast, it leaves a trail: any other ball crossing it is sped up x1.25",
         "+5% speed", Tier::Common, ItemTag::Support},
        {UpgradeKind::Pass, "Pass", "Pass",
         "a ball it clacks into is launched x1.5, its next hit +20% with this ball's element - billiards",
         "+15% launch", Tier::Rare, ItemTag::Support},
        {UpgradeKind::Kennel, "Kennel", "Kennel",
         "the slower it moves, the more often it lets loose a homing wisp (50% of its hit), every 3.5 s at a standstill",
         "sooner, +12% of its hit", Tier::Uncommon, ItemTag::Summoner},
        {UpgradeKind::DropTurret, "DropTurret", "Drop turret",
         "when you throw it, a turret is left where you let go (6 s, 35% of its hit per shot)",
         "+10% of its hit", Tier::Common, ItemTag::Summoner},
    };
    return findItemDef(defs, k);
}

// ---------------------------------------------------------------- lookup
inline const ItemDef* classItemDef(UpgradeKind k) {
    for (const ItemDef* (*f)(UpgradeKind) : {mageItemDef, shooterItemDef, assassinItemDef, summonerItemDef, jesterItemDef,
                                             slingerItemDef, styleItemDef})
        if (const ItemDef* d = f(k)) return d;
    return nullptr;
}

}  // namespace sb
