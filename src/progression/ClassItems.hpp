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
         "the slower it moves, the faster it fires: up to x2.2 at a standstill",
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
         "+15% launch", Tier::Uncommon, ItemTag::Support},
        {UpgradeKind::Kennel, "Kennel", "Kennel",
         "the slower it moves, the more often it lets loose a homing wisp (50% of its hit), every 3.5 s at a standstill",
         "sooner, +12% of its hit", Tier::Uncommon, ItemTag::Summoner},
        {UpgradeKind::DropTurret, "DropTurret", "Drop turret",
         "when you throw it, a turret is left where you let go (6 s, 35% of its hit per shot)",
         "+10% of its hit", Tier::Common, ItemTag::Summoner},
    };
    return findItemDef(defs, k);
}

// ==================================================================== Alchemist
inline const ItemDef* alchemistItemDef(UpgradeKind k) {
    static const std::vector<ItemDef> defs = {
        {UpgradeKind::Attune, "Attune", "Attune",
         "its elements are 25% stronger (burns, soaks, cracks, freezes, poison, zaps)",
         "+10%", Tier::Common, ItemTag::Alchemist},
        {UpgradeKind::Crucible, "Crucible", "Crucible",
         "the reactions it sets off hit 40% harder and 20% wider",
         "+12% harder", Tier::Uncommon, ItemTag::Alchemist},
        {UpgradeKind::Aftershock, "Aftershock", "Aftershock",
         "a reaction it sets off leaves its element on every enemy around - the next hit reacts again",
         "reaches further", Tier::Uncommon, ItemTag::Alchemist},
        {UpgradeKind::Flux, "Flux", "Flux",
         "each reaction it sets off recharges its abilities by 15%",
         "+5%", Tier::Uncommon, ItemTag::Alchemist},
        {UpgradeKind::PrismHit, "Prism", "Prism",
         "every 4th hit carries all its elements at once",
         "one hit sooner every 2 levels", Tier::Rare, ItemTag::Alchemist},
        {UpgradeKind::Conflux, "Conflux", "Conflux",
         "every reaction it sets off leaps once to the nearest other enemy carrying an element",
         "its reactions +10% harder", Tier::Epic, ItemTag::Alchemist},
    };
    return findItemDef(defs, k);
}

// ---------------------------------------------------------------- lookup
// ==================================================================== commons + uncommons (2026-10-04)
// One line each, every class: the base a build stands on. Each one is meant
// to combine with other classes' items too (DESIGN.md, "comunes").
inline const ItemDef* commonItemDef(UpgradeKind k) {
    using K = UpgradeKind;
    static const std::vector<ItemDef> defs = {
        // Alchemist
        {K::Volatile, "Volatile", "Volatile", "its elements last 40% longer on enemies - more time for another ball to set them off",
         "+15% longer", Tier::Common, ItemTag::Alchemist},
        {K::Seep, "Seep", "Seep", "a wall bounce leaves a puddle of its element for 2 s; enemies walking through take it",
         "+0.5 s, wider", Tier::Common, ItemTag::Alchemist},
        {K::Reagent, "Reagent", "Reagent", "+20% damage to enemies carrying another ball's element",
         "+8%", Tier::Common, ItemTag::Alchemist},
        {K::Tincture, "Tincture", "Tincture", "a ball it clacks into carries its element on its next hit (reactions!)",
         "that hit +10% harder", Tier::Common, ItemTag::Alchemist},
        {K::Distill, "Distill", "Distill", "every 3rd reaction it sets off repairs the core by 2",
         "+1 repair", Tier::Common, ItemTag::Alchemist},
        // Jester
        {K::Pickpocket, "Pickpocket", "Pickpocket", "10% chance a hit steals 1 gold (luck raises it)",
         "+4% chance", Tier::Common, ItemTag::Jester},
        {K::HotStreak, "HotStreak", "Hot streak", "each doubled hit in a row (crit, coin heads, a Jester double) adds +10% damage, up to 5; a plain hit ends it",
         "+5% per step", Tier::Common, ItemTag::Jester},
        {K::LuckyBreak, "LuckyBreak", "Lucky break", "5% chance a hit fully recharges its abilities (luck raises it)",
         "+2% chance", Tier::Common, ItemTag::Jester},
        {K::Confetti, "Confetti", "Confetti", "a doubled hit (crit, coin heads, a Jester double) throws 3 sparks at enemies nearby for 30% of the hit",
         "+1 spark, +10%", Tier::Common, ItemTag::Jester},
        // Mage
        {K::Spellblade, "Spellblade", "Spellblade", "the first hit after it casts an ability deals +40%",
         "+15%", Tier::Common, ItemTag::Mage},
        {K::AetherStep, "AetherStep", "Aether step", "casting an ability kicks it to x1.4 speed",
         "+0.1x", Tier::Common, ItemTag::Mage},
        {K::Insight, "Insight", "Insight", "each kill takes 0.5 s off its ability cooldowns",
         "+0.25 s", Tier::Common, ItemTag::Mage},
        // Summoner
        {K::Lingering, "Lingering", "Lingering", "its summons (turrets, totems, wisps, spritelings) last 30% longer",
         "+15% longer", Tier::Common, ItemTag::Summoner},
        {K::PackLeader, "PackLeader", "Pack leader", "+4% damage for each of its summons alive (up to 6)",
         "+2% per summon", Tier::Common, ItemTag::Summoner},
        {K::Decoy, "Decoy", "Decoy", "its turrets and totems draw nearby enemies toward them, away from the core",
         "a stronger pull, wider", Tier::Common, ItemTag::Summoner},
        // Assassin
        {K::MarkForDeath, "MarkForDeath", "Mark for death", "the enemy it blinks to takes +20% from every ball for 2.5 s",
         "+8%, +0.75 s", Tier::Common, ItemTag::Assassin},
        {K::Bleed, "Bleed", "Bleed", "its hits bleed: 30% of the hit more over 2 s",
         "+10% of the hit", Tier::Common, ItemTag::Assassin},
        {K::Quickstep, "Quickstep", "Quickstep", "each blink kicks it to x1.3 speed",
         "+0.1x", Tier::Common, ItemTag::Assassin},
        {K::Opportunist, "Opportunist", "Opportunist", "+25% damage to enemies that are frozen or staggered",
         "+10%", Tier::Common, ItemTag::Assassin},
        // Shooter
        {K::HollowPoint, "HollowPoint", "Hollow point", "its bullets deal +40% to enemies under an element (burning, poisoned, frozen, soaked, cracked or primed)",
         "+15%", Tier::Common, ItemTag::Shooter},
        {K::BankShot, "BankShot", "Bank shot", "its bullets bounce off the walls once",
         "+1 bounce", Tier::Common, ItemTag::Shooter},
        {K::CoveringFire, "CoveringFire", "Covering fire", "when you throw another ball, it fires a volley at the enemy nearest the core",
         "+1 bullet", Tier::Common, ItemTag::Shooter},
        // Slinger
        {K::LongPull, "LongPull", "Long pull", "the further you pull the slingshot, the harder the throw's first hit: up to +60%",
         "+20%", Tier::Common, ItemTag::Slinger},
        {K::Relay, "Relay", "Relay", "catch it within 2.5 s of your throw and the nearest other ball is launched at the enemy nearest it",
         "launched faster", Tier::Common, ItemTag::Slinger},
        {K::SecondWind, "SecondWind", "Second wind", "catching it recharges your Q by 30%",
         "+10%", Tier::Common, ItemTag::Slinger},
        // Guardian
        {K::Plating, "Plating", "Plating", "+10 core max health",
         "+5", Tier::Common, ItemTag::Guardian},
        {K::Sentinel, "Sentinel", "Sentinel", "the closer it flies to the core, the harder it hits: up to +40% right beside it",
         "+15%", Tier::Common, ItemTag::Guardian},
        {K::Brace, "Brace", "Brace", "enemies it hits move 20% slower for 2 s",
         "+8% slower", Tier::Common, ItemTag::Guardian},
        {K::HeavyLanding, "HeavyLanding", "Heavy landing", "bouncing off the core sends out a shockwave for 40% of its hit",
         "+15%", Tier::Common, ItemTag::Guardian},
        {K::Bulk, "Bulk", "Bulk", "12% bigger and 8% harder-hitting",
         "+4% bigger, +4% harder", Tier::Common, ItemTag::Guardian},
        // Striker
        {K::Edge, "Edge", "Edge", "+12% damage",
         "+6%", Tier::Common, ItemTag::Striker},
        {K::FirstBlood, "FirstBlood", "First blood", "the first hit it lands in each fight deals x3",
         "+0.5x", Tier::Common, ItemTag::Striker},
        {K::ComboHit, "ComboHit", "Combo hit", "hitting a different enemy within 0.6 s of the last one deals +35%",
         "+10%", Tier::Common, ItemTag::Striker},
        {K::Adrenaline, "Adrenaline", "Adrenaline", "each kill makes it 5% faster until the fight ends (up to 10 kills)",
         "+2% per kill", Tier::Common, ItemTag::Striker},
        // Support
        {K::Spotter, "Spotter", "Spotter", "enemies it hits take +15% from the other balls for 2 s",
         "+5%", Tier::Common, ItemTag::Support},
        {K::Anthem, "Anthem", "Anthem", "every other ball deals +6% damage",
         "+3%", Tier::Common, ItemTag::Support},
        {K::Medic, "Medic", "Medic", "clacking into another ball repairs the core by 1",
         "+0.5", Tier::Common, ItemTag::Support},
        {K::Sync, "Sync", "Sync", "each ability it casts takes 0.5 s off the other balls' ability cooldowns",
         "+0.25 s", Tier::Common, ItemTag::Support},
        {K::HandOff, "HandOff", "Hand-off", "its kills give the nearest other ball +25% on its next hit",
         "+10%", Tier::Common, ItemTag::Support},
        // ---- uncommons (2026-10-04) ----
        {K::Fission, "Fission", "Fission", "an enemy that dies carrying its element spreads that element to the enemies around it",
         "wider", Tier::Uncommon, ItemTag::Alchemist},
        {K::Elixir, "Elixir", "Elixir", "each reaction it sets off makes every ball hit 10% harder for 3 s",
         "+5%", Tier::Uncommon, ItemTag::Alchemist},
        {K::HouseEdge, "HouseEdge", "House edge", "20% chance (luck raises it) that gold you earn comes double - kills, Midas, the fight's pay",
         "+8% chance", Tier::Uncommon, ItemTag::Jester},
        {K::Mirror, "Mirror", "Mirror", "12% chance (luck raises it) the nearest other ball copies its hit onto the same enemy",
         "+4% chance", Tier::Uncommon, ItemTag::Jester},
        {K::Sigil, "Sigil", "Sigil", "each ability it casts leaves a glyph for 4 s; another ball crossing it sets off a small nova (40% of its hit)",
         "+15% of the hit", Tier::Uncommon, ItemTag::Mage},
        {K::ArcaneCharge, "ArcaneCharge", "Arcane charge", "each wall bounce takes 0.3 s off its ability cooldowns",
         "+0.1 s", Tier::Uncommon, ItemTag::Mage},
        {K::Conduit, "Conduit", "Conduit", "each ability it casts leaves the nearest other ball's element on the enemies around it - its own hits then set off reactions",
         "wider", Tier::Uncommon, ItemTag::Mage},
        {K::Martyr, "Martyr", "Martyr", "its turrets and totems burst with its element when they run out, for 50% of its hit",
         "+20% of the hit", Tier::Uncommon, ItemTag::Summoner},
        {K::GuardDog, "GuardDog", "Guard dog", "its summons go for the enemy nearest the core and hit 30% harder",
         "+10%", Tier::Uncommon, ItemTag::Summoner},
        {K::ChainKill, "ChainKill", "Chain kill", "a kill within 1 s of a blink recharges the blink at once",
         "+0.3 s window", Tier::Uncommon, ItemTag::Assassin},
        {K::VenomBlade, "VenomBlade", "Venom blade", "the first hit after a blink leaves its element twice over (two burns, two poison stacks, two cracks...)",
         "+15% damage on that hit", Tier::Uncommon, ItemTag::Assassin},
        {K::SuppressingFire, "SuppressingFire", "Suppressing fire", "each bullet that hits slows the enemy 10% more for 2 s, up to 40%",
         "+5% per bullet", Tier::Uncommon, ItemTag::Shooter},
        {K::AmmoFeed, "AmmoFeed", "Ammo feed", "each wall bounce fires a volley at the enemy nearest it",
         "+1 bullet", Tier::Uncommon, ItemTag::Shooter},
        {K::WindUp, "WindUp", "Wind-up", "the longer you hold it before throwing (up to 1.5 s), the faster it flies: up to +50%",
         "+15%", Tier::Uncommon, ItemTag::Slinger},
        {K::BankThrow, "BankThrow", "Bank throw", "each wall it bounces off after your throw adds +15% to the throw's first hit",
         "+5% per wall", Tier::Uncommon, ItemTag::Slinger},
        {K::Bully, "Bully", "Bully", "+30% damage to enemies bigger than it - Tanks, Brutes, bosses",
         "+10%", Tier::Uncommon, ItemTag::Striker},
        {K::Carom, "Carom", "Carom", "a hit within 0.5 s of clacking off another ball deals +40%",
         "+15%", Tier::Uncommon, ItemTag::Striker},
        // ---- rares (2026-10-04) ----
        {K::Transmute, "Transmute", "Transmute", "when it sets off a reaction it takes the other element as its own for 5 s - it can chain reactions with any ball",
         "+1.5 s", Tier::Rare, ItemTag::Alchemist},
        {K::Residue, "Residue", "Residue", "its reactions leave a puddle for 2 s that keeps priming whatever walks in - the next hit there reacts again",
         "+0.5 s, wider", Tier::Rare, ItemTag::Alchemist},
        {K::Aurum, "Aurum", "Aurum", "every enemy killed by a reaction it set off pays 2 gold",
         "+1 gold", Tier::Rare, ItemTag::Alchemist},
        {K::HighRoller, "HighRoller", "High roller", "its doubled hits (crit, coin heads, a Jester double) land 75% harder on top - about triple - but every hit deals 15% less",
         "+25% on doubles", Tier::Rare, ItemTag::Jester},
        {K::LuckySeven, "LuckySeven", "Lucky seven", "every 7th hit it lands is a sure double",
         "one hit sooner", Tier::Rare, ItemTag::Jester},
        {K::Fortune, "Fortune", "Fortune", "each doubled hit makes every chance it has 8% likelier until the fight ends (up to 5 times)",
         "+3% per double", Tier::Rare, ItemTag::Jester},
        {K::Projection, "Projection", "Projection", "its ghost copies (Split shot, Mitosis, Gemini, Phantom, Split) cast its abilities too",
         "+10% ability damage", Tier::Rare, ItemTag::Mage},
        {K::Spellweave, "Spellweave", "Spellweave", "its abilities deal +50% to enemies under an element",
         "+15%", Tier::Rare, ItemTag::Mage},
        {K::Cascade, "Cascade", "Cascade", "an ability that kills is ready again at once",
         "+10% ability damage", Tier::Rare, ItemTag::Mage},
        {K::Hatchery, "Hatchery", "Hatchery", "every enemy its summons kill lets a wisp loose",
         "+1 wisp", Tier::Rare, ItemTag::Summoner},
        {K::Golem, "Golem", "Golem", "every 10 s it calls a stone golem (6 s) that walks to the enemy nearest the core and slams it away (80% of its hit)",
         "comes 1.5 s sooner, +20% slam", Tier::Rare, ItemTag::Summoner},
        {K::Soulbind, "Soulbind", "Soulbind", "its summons leave their element's full effect - freeze, poison, burn, soak, crack - not just the primer",
         "+10% summon power", Tier::Rare, ItemTag::Summoner},
        {K::Predator, "Predator", "Predator", "its blink reaches twice as far and goes for the weakest enemy in reach; the hit after it deals +40%",
         "+15%", Tier::Rare, ItemTag::Assassin},
        {K::Flicker, "Flicker", "Flicker", "30% chance a blink is followed at once by another (up to 3 in a row)",
         "+10% chance", Tier::Rare, ItemTag::Assassin},
        {K::FragRounds, "FragRounds", "Frag rounds", "its bullets burst on impact, hitting what's around for 40% of the bullet",
         "+15%", Tier::Rare, ItemTag::Shooter},
        {K::Marksman, "Marksman", "Marksman", "every 5th bullet hits x3 and marks the enemy (every ball hits it harder)",
         "one bullet sooner", Tier::Rare, ItemTag::Shooter},
        {K::SkippingStone, "SkippingStone", "Skipping stone", "after your throw it skips off enemies without losing speed, until it touches the core",
         "+5% speed per skip", Tier::Rare, ItemTag::Slinger},
        {K::Formation, "Formation", "Formation", "when you throw it, every other free ball flies off the same way at 60% of its speed",
         "+15%", Tier::Rare, ItemTag::Slinger},
        {K::ShieldBash, "ShieldBash", "Shield bash", "enemies it hits near the core are knocked far back and stunned for 1 s",
         "harder, +0.3 s", Tier::Rare, ItemTag::Guardian},
        // ---- epics (2026-10-04) ----
        {K::Saturate, "Saturate", "Saturate", "the reactions it sets off leave both elements' full effects (burn, poison, freeze, soak, crack) on everything caught in them",
         "wider", Tier::Epic, ItemTag::Alchemist},
        {K::Maelstrom, "Maelstrom", "Maelstrom", "every 8 s it lets out a ring that leaves all its elements on everything near it - reactions go off",
         "1 s sooner, wider", Tier::Epic, ItemTag::Alchemist},
        {K::DeathSentence, "DeathSentence", "Death sentence", "enemies it hits are sentenced for 4 s: any ball's hit that leaves them under 20% kills them - and its blink fires",
         "+5% threshold", Tier::Epic, ItemTag::Assassin},
        {K::RazorWire, "RazorWire", "Razor wire", "each blink leaves a cutting line for 2 s that hurts whatever crosses it (50% of its hit)",
         "+20% of the hit, lasts longer", Tier::Epic, ItemTag::Assassin},
        {K::WardRing, "WardRing", "Ward ring", "every 6 s a ring on the core throws back the first enemy that reaches it - no damage taken",
         "1 s sooner", Tier::Epic, ItemTag::Guardian},
        {K::Juggernaut, "Juggernaut", "Juggernaut", "the bigger it is, the harder it hits: its size bonus counts double as damage",
         "x2.5, then x3", Tier::Epic, ItemTag::Guardian},
        {K::LastLine, "LastLine", "Last line", "once per fight, when the core drops under 25%, a blast from the core hits every enemy and throws it to the walls",
         "+50% of the hit, a longer stun", Tier::Epic, ItemTag::Guardian},
        {K::SlotMachine, "SlotMachine", "Slot machine", "every 10 s it spins 3 symbols: a pair gives 6 s of damage, speed or gold; three of a kind gives all three and a jackpot blast",
         "spins 1.5 s sooner", Tier::Epic, ItemTag::Jester},
        {K::WildMagic, "WildMagic", "Wild magic", "each doubled hit casts a random ability that any ball in the run carries",
         "less time between casts", Tier::Epic, ItemTag::Jester},
        {K::SpellChorus, "SpellChorus", "Spell chorus", "every 5th ability it casts, every other ball casts its first ability too",
         "one cast sooner", Tier::Epic, ItemTag::Mage},
        {K::Chronomancy, "Chronomancy", "Chronomancy", "each ability it casts slows every enemy 20% for 1.5 s",
         "+5% slower", Tier::Epic, ItemTag::Mage},
        {K::Minigun, "Minigun", "Minigun", "the longer it keeps firing, the faster it fires: up to x3; 2 s without a target and it spins down",
         "spins up faster", Tier::Epic, ItemTag::Shooter},
        {K::SmartRounds, "SmartRounds", "Smart rounds", "its bullets curve after enemies",
         "they turn tighter", Tier::Epic, ItemTag::Shooter},
        {K::Shrapnel, "Shrapnel", "Shrapnel", "every enemy it kills bursts into a ring of 6 bullets (50% of a bullet)",
         "+2 bullets", Tier::Epic, ItemTag::Shooter},
        {K::PerfectRelease, "PerfectRelease", "Perfect release", "a slingshot throw at full pull is perfect: it hits x2.5 and goes through enemies until it touches a wall",
         "+0.5x", Tier::Epic, ItemTag::Slinger},
        {K::Homecoming, "Homecoming", "Homecoming", "after the first hit of your throw it curves back toward your pointer; caught on the way home, it sends out a shockwave (60% of its hit)",
         "+20% of the hit", Tier::Epic, ItemTag::Slinger},
        {K::Lightspeed, "Lightspeed", "Lightspeed", "above twice its cruise speed its hits chain lightning to 3 enemies (50% of the hit) and don't slow it down",
         "+1 chain, +10%", Tier::Epic, ItemTag::Striker},
        {K::SpiritHost, "SpiritHost", "Spirit host", "with 5 of its wisps out, they merge into a great spirit: x3 damage, lasts longer, goes through 5 enemies",
         "+1 enemy, +0.5x", Tier::Epic, ItemTag::Summoner},
        {K::TwinCall, "TwinCall", "Twin call", "everything it summons (turrets, totems, wisps, golems) comes in pairs - the second on the far side of the core",
         "+10% summon power", Tier::Epic, ItemTag::Summoner},
        {K::Fortify, "Fortify", "Fortify", "its turrets stop dead any enemy that touches them, and last 50% longer",
         "+25% longer", Tier::Epic, ItemTag::Summoner},
        // ---- legendaries (2026-10-05) ----
        {K::Titan, "Titan", "Titan", "every bounce off the core makes it bigger and harder-hitting, up to twice its size; at full size the next core bounce sets off a quake that stuns every enemy",
         "grows faster, a bigger quake", Tier::Legendary, ItemTag::Guardian},
        {K::Archmage, "Archmage", "Archmage", "its abilities recharge twice as fast, and each one it casts makes the next ones 10% stronger until the fight ends",
         "faster still, +2.5% per cast", Tier::Legendary, ItemTag::Mage},
        {K::TimeStop, "TimeStop", "Time stop", "every 5th ability it casts stops every enemy for 2 s - the balls keep moving",
         "+0.5 s", Tier::Legendary, ItemTag::Mage},
        {K::IonCannon, "IonCannon", "Ion cannon", "it flies 40% slower and stops shooting bullets: it charges for 2 s and fires a laser across the arena, down the line with the most enemies, for x4 its hit",
         "+1x, charges faster", Tier::Legendary, ItemTag::Shooter},
        {K::Gunship, "Gunship", "Gunship", "every other ball fires a bullet when it hits (40% of its hit), carrying this ball's Shooter items",
         "+10% of the hit", Tier::Legendary, ItemTag::Shooter},
        {K::LeadStorm, "LeadStorm", "Lead storm", "every 10 s it empties the gun: for 2 s it fires 6 times as fast, all around it",
         "2 s sooner", Tier::Legendary, ItemTag::Shooter},
        {K::Reaper, "Reaper", "Reaper", "its blink barely needs a cooldown: it blinks on after every hit, not just kills",
         "an even shorter wait", Tier::Legendary, ItemTag::Assassin},
        {K::Contract, "Contract", "Contract", "each fight it takes a contract on the strongest enemy (the boss or a Brute first): its hits on that one take 10% of its max health (2% of a boss's)",
         "+2.5% (+0.5% on a boss)", Tier::Legendary, ItemTag::Assassin},
        {K::Necromancer, "Necromancer", "Necromancer", "enemies its summons kill rise as ghosts that fly at the nearest enemy and explode with its element (80% of its hit)",
         "+20% of the hit", Tier::Legendary, ItemTag::Summoner},
        {K::Overlord, "Overlord", "Overlord", "every ball's summons serve it: they take its element and its summon items (Guard dog, Soulbind, Pack leader...) and hit 20% harder",
         "+7.5% harder", Tier::Legendary, ItemTag::Summoner},
        {K::Rigged, "Rigged", "Rigged", "every chance in the run - crits, coins, echoes, Jester rolls - is rolled twice and keeps the better one",
         "+2 luck", Tier::Legendary, ItemTag::Jester},
        {K::Joker, "Joker", "Joker", "each fight it turns into a copy of a random legendary item",
         "the copy is a level higher", Tier::Legendary, ItemTag::Jester},
        {K::SteadyHand, "SteadyHand", "Steady hand", "while you aim it with the slingshot, time slows to 20% and never runs out; the longer you aim, the harder the throw's first hit (up to +60%)",
         "time slower, +20% more", Tier::Legendary, ItemTag::Slinger},
        {K::Showtime, "Showtime", "Showtime", "every catch within 2.5 s of your throw adds +20% damage and speed until the fight ends, no cap; a slow catch keeps half",
         "+5% per catch", Tier::Legendary, ItemTag::Slinger},
        {K::PhilosopherStone, "PhilosopherStone", "Philosopher's stone", "it carries all six elements: every hit leaves two of them at random - a reaction every time",
         "+1 element per hit", Tier::Legendary, ItemTag::Alchemist},
        {K::GrandReaction, "GrandReaction", "Grand reaction", "every 10th reaction it sets off is a grand one: x5 the hit, across the whole arena",
         "one reaction sooner, +0.5x", Tier::Legendary, ItemTag::Alchemist},
    };
    return findItemDef(defs, k);
}

inline const ItemDef* classItemDef(UpgradeKind k) {
    for (const ItemDef* (*f)(UpgradeKind) : {mageItemDef, shooterItemDef, assassinItemDef, summonerItemDef, jesterItemDef,
                                             slingerItemDef, styleItemDef, alchemistItemDef, commonItemDef})
        if (const ItemDef* d = f(k)) return d;
    return nullptr;
}

}  // namespace sb
