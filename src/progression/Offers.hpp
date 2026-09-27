#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <vector>

#include "progression/ClassItems.hpp"
#include "progression/UpgradeKind.hpp"
#include "sim/Entities.hpp"

namespace sb {

// ---- between-wave picks ------------------------------------------------------
//
// Every ball is a small "character": four item slots, one TYPE slot (its
// element), one ability slot (more for a Mage), any number of stacked
// modifiers, and up to two classes that EMERGE from its items. Every item
// carries a tag (Striker / Guardian / Support / Mage / Shooter / Assassin /
// Summoner / Jester): 2 items of a tag give the ball that class, 4 give its
// ascended form. A pick is one of six kinds:
//  - New ball: one more ball (up to cfg::ball::maxBalls).
//  - Element:  goes in the ball's type slot and makes it fire / poison / ...;
//              one per ball, a new one swaps it. Gated by its web node. Two
//              DIFFERENT balls landing different elements on one enemy set off
//              a reaction (see World). Doesn't count toward a class.
//  - Ability:  a timed active in an ability slot; fires by itself on a
//              cooldown. Doesn't count toward a class.
//  - Item:     a unique effect for one ball; takes one of its 4 item slots.
//  - Modifier: a stat bump for one ball; no slot, stacks without limit.
//  - Relic:    a whole-run passive.

inline constexpr int kChoiceCount = 4;
inline constexpr int kBallSlots = 4;          // item slots per ball
inline constexpr int kElementItemCount = 6;
inline constexpr int kAbilityItemCount = 8;   // AbilityDash..AbilityMissile
inline constexpr int kModifierCount = 3;      // HeavyImpact..Swift
// Items level up: picking one a ball already has (or forging it) raises its
// level instead of taking another slot. Each item scales its own way per level
// (App::ballSpec, upgradeLevelDesc). Elements and abilities level the same way.
inline constexpr int kMaxItemLevel = 5;

// A ball's slots, addressed as one list: 0..3 items, then the type slot, then
// the ability slots (kMaxAbilitySlots of them, abilitySlotCount() active).
inline constexpr int kSlotType = kBallSlots;
inline constexpr int kSlotAbility = kBallSlots + 1;
inline constexpr int kLoadoutSlots = kSlotAbility + kMaxAbilitySlots;
inline bool isItemSlot(int s) { return s >= 0 && s < kBallSlots; }
inline bool isAbilitySlot(int s) { return s >= kSlotAbility && s < kLoadoutSlots; }

enum class UpgradeCat { NewBall, Element, Ability, Item, Modifier, Relic };

inline UpgradeCat upgradeCat(UpgradeKind k) {
    const int i = static_cast<int>(k);
    if (i <= static_cast<int>(UpgradeKind::AddBall)) return UpgradeCat::NewBall;
    if (i <= static_cast<int>(UpgradeKind::ElemElectric)) return UpgradeCat::Element;
    if (i <= static_cast<int>(UpgradeKind::AbilityMissile)) return UpgradeCat::Ability;
    if (i <= static_cast<int>(UpgradeKind::Swift)) return UpgradeCat::Modifier;
    if (i >= static_cast<int>(UpgradeKind::CoreSpring)) return UpgradeCat::Relic;
    return UpgradeCat::Item;
}

inline const char* upgradeCatName(UpgradeCat c) {
    switch (c) {
        case UpgradeCat::NewBall:  return "NEW BALL";
        case UpgradeCat::Element:  return "ELEMENT";
        case UpgradeCat::Ability:  return "ABILITY";
        case UpgradeCat::Item:     return "ITEM";
        case UpgradeCat::Modifier: return "MODIFIER";
        case UpgradeCat::Relic:    return "RELIC";
    }
    return "";
}

inline const char* upgradeCatDesc(UpgradeCat c) {
    switch (c) {
        case UpgradeCat::NewBall:  return "adds one more ball to the arena";
        case UpgradeCat::Element:  return "goes in the ball's type slot (one element per ball; a new one swaps it). Two balls with different elements hitting the same enemy set off a reaction. Taking it again on the same ball levels it up. Doesn't count toward a class.";
        case UpgradeCat::Ability:  return "a timed active in the ball's ability slot: it fires by itself every few seconds. Taking it again on the same ball levels it up. Doesn't count toward a class.";
        case UpgradeCat::Item:     return "a unique effect for one ball; takes one of its 4 item slots. Its tag counts toward the ball's class: 2 of a tag = that class, 4 = its ascended form. Taking it again on the same ball levels it up (max level 5).";
        case UpgradeCat::Modifier: return "a stat bump for one ball; no slot, stacks without limit";
        case UpgradeCat::Relic:    return "a passive for the whole run, on every ball";
    }
    return "";
}

inline Tier upgradeTier(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::Ricochet: case UpgradeKind::Crit:
        case UpgradeKind::HeavyImpact: case UpgradeKind::BigBall: case UpgradeKind::Swift:
        case UpgradeKind::CoreSpring: case UpgradeKind::StrongArm:
            return Tier::Common;
        case UpgradeKind::AddBall:
        case UpgradeKind::ElemFire: case UpgradeKind::ElemPoison: case UpgradeKind::ElemWater:
        case UpgradeKind::ElemIce: case UpgradeKind::ElemStone: case UpgradeKind::ElemElectric:
        case UpgradeKind::AbilityDash: case UpgradeKind::AbilityBulwark: case UpgradeKind::AbilityArc:
        case UpgradeKind::AbilityMissile:
        case UpgradeKind::Rampart: case UpgradeKind::Mender: case UpgradeKind::Bedrock:
        case UpgradeKind::Conductor: case UpgradeKind::Shatter: case UpgradeKind::Bumper:
        case UpgradeKind::CoreSlowField: case UpgradeKind::Contagion: case UpgradeKind::Primed:
            return Tier::Uncommon;
        case UpgradeKind::AbilityNova: case UpgradeKind::AbilitySplit: case UpgradeKind::AbilityOverclock:
        case UpgradeKind::AbilityMeteor:
        case UpgradeKind::Cleave: case UpgradeKind::Executioner: case UpgradeKind::Overkill:
        case UpgradeKind::Tesla: case UpgradeKind::Bomber: case UpgradeKind::Echo:
        case UpgradeKind::Berserk: case UpgradeKind::Giant: case UpgradeKind::Midas:
        case UpgradeKind::Comet: case UpgradeKind::Mitosis: case UpgradeKind::Boomerang:
        case UpgradeKind::Glutton:
        case UpgradeKind::LuckyClover: case UpgradeKind::MagneticCore: case UpgradeKind::Overcharge:
            return Tier::Rare;
        case UpgradeKind::SplitShot: case UpgradeKind::Piercing: case UpgradeKind::Storm:
        case UpgradeKind::Hunter: case UpgradeKind::Tether: case UpgradeKind::BlackHole:
        case UpgradeKind::Resonance:
        case UpgradeKind::Catalyst: case UpgradeKind::ChainReaction:
        case UpgradeKind::GlassCannon: case UpgradeKind::Phoenix: case UpgradeKind::TimeDilation:
            return Tier::Epic;
        case UpgradeKind::Railgun: case UpgradeKind::Satellite: case UpgradeKind::GravityWell:
        case UpgradeKind::Gemini: case UpgradeKind::PrismCore: case UpgradeKind::Seeker:
            return Tier::Legendary;
        default: break;
    }
    if (const ItemDef* d = classItemDef(k)) return d->tier;   // the newer classes' items
    return Tier::Common;
}

inline const char* tierName(Tier t) {
    switch (t) {
        case Tier::Common:    return "Common";
        case Tier::Uncommon:  return "Uncommon";
        case Tier::Rare:      return "Rare";
        case Tier::Epic:      return "Epic";
        case Tier::Legendary: return "Legendary";
    }
    return "";
}

// ---- item tags: a ball's classes come from them ----------------------------

inline ItemTag itemTag(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::Ricochet: case UpgradeKind::Cleave: case UpgradeKind::Crit:
        case UpgradeKind::Executioner: case UpgradeKind::Conductor: case UpgradeKind::Echo:
        case UpgradeKind::Hunter: case UpgradeKind::Comet: case UpgradeKind::Mitosis:
        case UpgradeKind::Seeker: case UpgradeKind::Piercing: case UpgradeKind::Railgun:
        case UpgradeKind::Berserk:
            return ItemTag::Striker;
        case UpgradeKind::Bedrock: case UpgradeKind::Rampart: case UpgradeKind::Mender:
        case UpgradeKind::Boomerang: case UpgradeKind::Bumper: case UpgradeKind::Glutton:
        case UpgradeKind::Giant: case UpgradeKind::Satellite:
            return ItemTag::Guardian;
        case UpgradeKind::Overkill: case UpgradeKind::Shatter: case UpgradeKind::Tesla:
        case UpgradeKind::Bomber: case UpgradeKind::SplitShot:
        case UpgradeKind::Tether: case UpgradeKind::BlackHole: case UpgradeKind::Resonance:
        case UpgradeKind::GravityWell: case UpgradeKind::Storm: case UpgradeKind::Gemini:
        case UpgradeKind::Midas:
            return ItemTag::Support;
        default:
            break;
    }
    if (const ItemDef* d = classItemDef(k)) return d->tag;   // the newer classes' items
    return ItemTag::None;   // elements, abilities, modifiers, relics
}

inline BallRole tagRole(ItemTag t) { return static_cast<BallRole>(static_cast<int>(t)); }
inline ItemTag roleTag(BallRole r) { return static_cast<ItemTag>(static_cast<int>(r)); }
inline ItemTag classTag(int i) { return static_cast<ItemTag>(i + 1); }   // i = 0..kClassCount-1
inline const char* itemTagName(ItemTag t) { return t == ItemTag::None ? "" : roleName(tagRole(t)); }

// Element picks map onto element web-node slots 0..5 (fire..electric);
// everything else returns -1. The ball element is Element(slot + 1).
inline int elementItemSlot(UpgradeKind k) {
    if (upgradeCat(k) != UpgradeCat::Element) return -1;
    return static_cast<int>(k) - static_cast<int>(UpgradeKind::ElemFire);
}

// Ability picks -> the sim's Ability (None for anything else).
inline Ability abilityOf(UpgradeKind k) {
    if (upgradeCat(k) != UpgradeCat::Ability) return Ability::None;
    return static_cast<Ability>(static_cast<int>(k) - static_cast<int>(UpgradeKind::AbilityDash) + 1);
}

// Modifiers index 0..kModifierCount-1 into BallLoadout::mods; -1 otherwise.
inline int modifierIndex(UpgradeKind k) {
    if (upgradeCat(k) != UpgradeCat::Modifier) return -1;
    return static_cast<int>(k) - static_cast<int>(UpgradeKind::HeavyImpact);
}

struct UpgradeInfo {
    const char* title;
    const char* desc;
};

// Machine-readable name, for the SB_UPGRADES dev override.
inline const char* upgradeKindId(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddBall:        return "AddBall";
        case UpgradeKind::ElemFire:       return "ElemFire";
        case UpgradeKind::ElemPoison:     return "ElemPoison";
        case UpgradeKind::ElemWater:      return "ElemWater";
        case UpgradeKind::ElemIce:        return "ElemIce";
        case UpgradeKind::ElemStone:      return "ElemStone";
        case UpgradeKind::ElemElectric:   return "ElemElectric";
        case UpgradeKind::AbilityDash:    return "AbilityDash";
        case UpgradeKind::AbilityNova:    return "AbilityNova";
        case UpgradeKind::AbilitySplit:   return "AbilitySplit";
        case UpgradeKind::AbilityBulwark: return "AbilityBulwark";
        case UpgradeKind::AbilityOverclock: return "AbilityOverclock";
        case UpgradeKind::AbilityArc:     return "AbilityArc";
        case UpgradeKind::AbilityMeteor:  return "AbilityMeteor";
        case UpgradeKind::AbilityMissile: return "AbilityMissile";
        case UpgradeKind::Ricochet:       return "Ricochet";
        case UpgradeKind::Cleave:         return "Cleave";
        case UpgradeKind::Crit:           return "Crit";
        case UpgradeKind::Executioner:    return "Executioner";
        case UpgradeKind::Overkill:       return "Overkill";
        case UpgradeKind::Shatter:        return "Shatter";
        case UpgradeKind::Conductor:      return "Conductor";
        case UpgradeKind::Bedrock:        return "Bedrock";
        case UpgradeKind::Echo:           return "Echo";
        case UpgradeKind::Tesla:          return "Tesla";
        case UpgradeKind::Bomber:         return "Bomber";
        case UpgradeKind::SplitShot:      return "SplitShot";
        case UpgradeKind::Rampart:        return "Rampart";
        case UpgradeKind::Mender:         return "Mender";
        case UpgradeKind::Hunter:         return "Hunter";
        case UpgradeKind::Comet:          return "Comet";
        case UpgradeKind::Mitosis:        return "Mitosis";
        case UpgradeKind::Boomerang:      return "Boomerang";
        case UpgradeKind::Bumper:         return "Bumper";
        case UpgradeKind::Glutton:        return "Glutton";
        case UpgradeKind::Tether:         return "Tether";
        case UpgradeKind::BlackHole:      return "BlackHole";
        case UpgradeKind::Resonance:      return "Resonance";
        case UpgradeKind::Seeker:         return "Seeker";
        case UpgradeKind::Piercing:       return "Piercing";
        case UpgradeKind::Railgun:        return "Railgun";
        case UpgradeKind::Berserk:        return "Berserk";
        case UpgradeKind::Giant:          return "Giant";
        case UpgradeKind::Satellite:      return "Satellite";
        case UpgradeKind::GravityWell:    return "GravityWell";
        case UpgradeKind::Storm:          return "Storm";
        case UpgradeKind::Gemini:         return "Gemini";
        case UpgradeKind::Midas:          return "Midas";
        case UpgradeKind::HeavyImpact:    return "HeavyImpact";
        case UpgradeKind::BigBall:        return "BigBall";
        case UpgradeKind::Swift:          return "Swift";
        case UpgradeKind::CoreSpring:     return "CoreSpring";
        case UpgradeKind::CoreSlowField:  return "CoreSlowField";
        case UpgradeKind::StrongArm:      return "StrongArm";
        case UpgradeKind::Contagion:      return "Contagion";
        case UpgradeKind::Primed:         return "Primed";
        case UpgradeKind::Catalyst:       return "Catalyst";
        case UpgradeKind::ChainReaction:  return "ChainReaction";
        case UpgradeKind::LuckyClover:    return "LuckyClover";
        case UpgradeKind::GlassCannon:    return "GlassCannon";
        case UpgradeKind::MagneticCore:   return "MagneticCore";
        case UpgradeKind::PrismCore:      return "PrismCore";
        case UpgradeKind::Phoenix:        return "Phoenix";
        case UpgradeKind::TimeDilation:   return "TimeDilation";
        case UpgradeKind::Overcharge:     return "Overcharge";
        default: break;
    }
    if (const ItemDef* d = classItemDef(k)) return d->id;
    return "";
}

inline UpgradeInfo upgradeInfo(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddBall:       return {"Extra ball", "one more ball in the arena"};
        case UpgradeKind::ElemFire:      return {"Fire", "the ball turns fire: heavier contact hits"};
        case UpgradeKind::ElemPoison:    return {"Poison", "the ball turns poison: hits stack damage over time"};
        case UpgradeKind::ElemWater:     return {"Water", "the ball turns water: trails a damaging wake"};
        case UpgradeKind::ElemIce:       return {"Ice", "the ball turns ice: hits freeze enemies in place"};
        case UpgradeKind::ElemStone:     return {"Stone", "the ball turns stone: drops grinding rubble"};
        case UpgradeKind::ElemElectric:  return {"Electric", "the ball turns electric: zaps nearby enemies"};
        case UpgradeKind::AbilityDash:   return {"Dash", "every few seconds the ball bursts straight at the nearest enemy"};
        case UpgradeKind::AbilityNova:   return {"Nova", "every few seconds the ball lets out a shockwave that hits and shoves everything around it"};
        case UpgradeKind::AbilitySplit:  return {"Split", "every few seconds two ghost copies of the ball fan out for a moment, with its items"};
        case UpgradeKind::AbilityBulwark: return {"Bulwark", "when enemies close in, the core pushes out a pulse that shoves and staggers them"};
        case UpgradeKind::AbilityOverclock: return {"Overclock", "every few seconds the ball runs hot: faster and 50% harder-hitting for 3 s"};
        case UpgradeKind::AbilityArc:    return {"Arc", "every few seconds a bolt leaps from the ball through up to 4 enemies, with its element"};
        case UpgradeKind::AbilityMeteor: return {"Meteor", "every few seconds a meteor falls on the thickest pack of enemies and crushes it, with the ball's element"};
        case UpgradeKind::AbilityMissile: return {"Magic missile", "every few seconds the ball looses a magic missile that curves after the nearest enemy and follows it, with its element"};
        case UpgradeKind::Ricochet:      return {"Ricochet", "every wall bounce speeds it up and arms a harder hit for a moment"};
        case UpgradeKind::Cleave:        return {"Cleave", "punches straight through an enemy it kills"};
        case UpgradeKind::Crit:          return {"Keen eye", "12% chance a hit deals double damage"};
        case UpgradeKind::Executioner:   return {"Executioner", "big bonus damage to badly hurt enemies"};
        case UpgradeKind::Overkill:      return {"Overkill", "leftover damage from a kill splashes onto the next enemy"};
        case UpgradeKind::Shatter:       return {"Shatter", "hitting a frozen enemy deals bonus damage"};
        case UpgradeKind::Conductor:     return {"Conductor", "its electric arc jumps on to another enemy"};
        case UpgradeKind::Bedrock:       return {"Bedrock", "its stone rubble lasts much longer"};
        case UpgradeKind::Echo:          return {"Echo", "25% chance a hit strikes twice (effects and all)"};
        case UpgradeKind::Tesla:         return {"Tesla", "20% chance a hit zaps up to 3 enemies nearby"};
        case UpgradeKind::Bomber:        return {"Bomber", "30% chance an enemy it kills explodes"};
        case UpgradeKind::SplitShot:     return {"Split shot", "15% chance a wall bounce spawns a ghost copy with the same items (lasts a few seconds)"};
        case UpgradeKind::Rampart:       return {"Rampart", "its hits shove enemies much further and stagger them longer"};
        case UpgradeKind::Mender:        return {"Mender", "every time it bounces off the core, the core repairs a little"};
        case UpgradeKind::Hunter:        return {"Hunter", "locks onto the biggest threat and chases it down, hitting it harder, until it dies"};
        case UpgradeKind::Comet:         return {"Comet", "flung, it flies far faster and keeps the speed - and while that fast it plows through enemies"};
        case UpgradeKind::Mitosis:       return {"Mitosis", "every kill splits off a small copy of the ball, with its items, for a few seconds"};
        case UpgradeKind::Boomerang:     return {"Boomerang", "after a hit it flies back to the core, then out at the nearest threat; each trip home charges a harder hit"};
        case UpgradeKind::Bumper:        return {"Bumper", "a pinball bumper: bigger, and other balls that bounce off it are launched much faster"};
        case UpgradeKind::Glutton:       return {"Glutton", "every kill makes it bigger and hit harder, until the wave ends"};
        case UpgradeKind::Tether:        return {"Tether", "a laser links it to the nearest other ball and burns every enemy that crosses the line"};
        case UpgradeKind::BlackHole:     return {"Black hole", "35% chance a kill leaves a black hole that sucks enemies in, then bursts with the ball's element"};
        case UpgradeKind::Resonance:     return {"Resonance", "its hits arc lightning to every other ball of the same element, and each of those zaps an enemy"};
        case UpgradeKind::Seeker:        return {"Seeker", "the ball curves in flight toward the nearest enemy"};
        case UpgradeKind::Piercing:      return {"Piercing", "the ball passes straight through enemies, hitting every one on its path"};
        case UpgradeKind::Railgun:       return {"Railgun", "every wall bounce fires a beam along its new path, hitting all in line"};
        case UpgradeKind::Berserk:       return {"Berserk", "each enemy hit in a row adds +15% damage; touching a wall resets it"};
        case UpgradeKind::Giant:         return {"Giant", "the ball becomes huge and heavy: x1.8 size, x1.3 damage, a bit slower"};
        case UpgradeKind::Satellite:     return {"Satellite", "the ball stops bouncing and orbits the core, grinding whatever comes close"};
        case UpgradeKind::GravityWell:   return {"Gravity well", "drags every enemy near it toward itself - packs them up for reactions"};
        case UpgradeKind::Storm:         return {"Storm", "a constant storm around the ball zaps every enemy near it"};
        case UpgradeKind::Gemini:        return {"Gemini", "a permanent ghost twin flies with it, copying all its items"};
        case UpgradeKind::Midas:         return {"Midas", "enemies it kills pay 3 extra gold"};
        case UpgradeKind::HeavyImpact:   return {"Heavy impact", "+15% contact damage (stacks)"};
        case UpgradeKind::BigBall:       return {"Big ball", "+10% radius and harder knockback (stacks)"};
        case UpgradeKind::Swift:         return {"Swift", "+8% cruise and +15% top speed, holds a fling longer (stacks)"};
        case UpgradeKind::CoreSpring:    return {"Spring core", "your balls bounce off the core faster"};
        case UpgradeKind::CoreSlowField: return {"Slow field", "enemies near the core are slowed"};
        case UpgradeKind::StrongArm:     return {"Strong arm", "you fling every ball noticeably harder"};
        case UpgradeKind::Contagion:     return {"Contagion", "an enemy that dies poisoned poisons those near it"};
        case UpgradeKind::Primed:        return {"Primed", "+damage to enemies already burning, poisoned or frozen"};
        case UpgradeKind::Catalyst:      return {"Catalyst", "element reactions hit twice as hard and reach further"};
        case UpgradeKind::ChainReaction: return {"Chain reaction", "35% chance a reaction sets off again on another afflicted enemy - it can cascade"};
        case UpgradeKind::LuckyClover:   return {"Lucky clover", "+6 luck: every chance (crits, echoes, zaps, bombs, ghosts...) is higher and cards roll rarer"};
        case UpgradeKind::GlassCannon:   return {"Glass cannon", "all damage x1.6, but the core loses 30% of its max health"};
        case UpgradeKind::MagneticCore:  return {"Magnetic core", "every ball bouncing off the core flies at the nearest enemy"};
        case UpgradeKind::PrismCore:     return {"Prism core", "balls with no element leave a random element on every hit - reactions everywhere"};
        case UpgradeKind::Phoenix:       return {"Phoenix", "once per act, when the core breaks it comes back at half health"};
        case UpgradeKind::TimeDilation:  return {"Time dilation", "all enemies move 25% slower, all the time"};
        case UpgradeKind::Overcharge:    return {"Overcharge", "the damage combo can climb twice as high"};
        default: break;
    }
    if (const ItemDef* d = classItemDef(k)) return {d->title, d->desc};
    return {"", ""};
}

// What one more level of an item does (items, elements and abilities; ""
// otherwise). Every item level past the first also makes the ball hit 10% harder.
inline const char* upgradeLevelDesc(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::ElemFire: case UpgradeKind::ElemPoison: case UpgradeKind::ElemWater:
        case UpgradeKind::ElemIce: case UpgradeKind::ElemStone: case UpgradeKind::ElemElectric:
                                         return "its element is 30% stronger";
        case UpgradeKind::AbilityDash:   return "a faster burst, a shorter cooldown";
        case UpgradeKind::AbilityNova:   return "a wider, harder shockwave, a shorter cooldown";
        case UpgradeKind::AbilitySplit:  return "the copies last longer, a shorter cooldown";
        case UpgradeKind::AbilityBulwark: return "a wider pulse that staggers longer, a shorter cooldown";
        case UpgradeKind::AbilityOverclock: return "runs hot longer and harder, a shorter cooldown";
        case UpgradeKind::AbilityArc:    return "leaps to one more enemy, harder, a shorter cooldown";
        case UpgradeKind::AbilityMeteor: return "a wider, heavier impact, a shorter cooldown";
        case UpgradeKind::AbilityMissile: return "harder missiles, a shorter cooldown; a 2nd missile at level 3, a 3rd at level 5";
        case UpgradeKind::Ricochet:      return "a bigger speed kick and a harder armed hit";
        case UpgradeKind::Cleave:        return "also cuts through - and finishes - enemies it leaves under 8% more health";
        case UpgradeKind::Crit:          return "+7% chance, and crits hit harder";
        case UpgradeKind::Executioner:   return "kicks in on healthier enemies and hits harder";
        case UpgradeKind::Overkill:      return "more splash; a 2nd enemy at level 3, a 3rd at level 5";
        case UpgradeKind::Shatter:       return "+40% damage vs frozen enemies";
        case UpgradeKind::Conductor:     return "the arc jumps one more time";
        case UpgradeKind::Bedrock:       return "the rubble lasts even longer";
        case UpgradeKind::Echo:          return "+10% chance";
        case UpgradeKind::Tesla:         return "+8% chance, zaps one more enemy";
        case UpgradeKind::Bomber:        return "+12% chance, a bigger blast";
        case UpgradeKind::SplitShot:     return "+7% chance";
        case UpgradeKind::Rampart:       return "shoves further, staggers longer";
        case UpgradeKind::Mender:        return "repairs more per bounce";
        case UpgradeKind::Hunter:        return "hits its prey harder and turns tighter";
        case UpgradeKind::Comet:         return "flies faster still and plows through sooner";
        case UpgradeKind::Mitosis:       return "copies last longer; 2 copies per kill at level 3, 3 at level 5";
        case UpgradeKind::Boomerang:     return "a bigger charged hit and a harder kick off the core";
        case UpgradeKind::Bumper:        return "launches other balls even faster";
        case UpgradeKind::Glutton:       return "grows more per kill and can grow further";
        case UpgradeKind::Tether:        return "a hotter, wider laser";
        case UpgradeKind::BlackHole:     return "+10% chance, a stronger pull and burst";
        case UpgradeKind::Resonance:     return "stronger arcs, more often";
        case UpgradeKind::Seeker:        return "turns tighter and sees further";
        case UpgradeKind::Piercing:      return "+15% damage on every enemy it passes through";
        case UpgradeKind::Railgun:       return "a heavier, wider beam";
        case UpgradeKind::Berserk:       return "more damage per hit in a row, higher cap";
        case UpgradeKind::Giant:         return "hits harder still";
        case UpgradeKind::Satellite:     return "grinds harder";
        case UpgradeKind::GravityWell:   return "a stronger, wider pull";
        case UpgradeKind::Storm:         return "zaps more often and harder";
        case UpgradeKind::Gemini:        return "a 2nd twin at level 3, a 3rd at level 5";
        case UpgradeKind::Midas:         return "+3 gold per kill";
        default:                         break;
    }
    if (const ItemDef* d = classItemDef(k)) return d->levelDesc;
    return "";
}

// One ball of the run: 4 item slots, a type slot (its element), its ability
// slots and its stacked modifiers. Its classes are the item tags it holds 2+
// of (so 0, 1 or 2 classes); 4 of one tag is that class's ascended form.
// Elements and abilities never count toward a class.
struct BallLoadout {
    int gear[kBallSlots] = {-1, -1, -1, -1};   // UpgradeKind per item slot, -1 = empty
    int gearLvl[kBallSlots] = {0, 0, 0, 0};    // item level: 1 once equipped, up to kMaxItemLevel
    int type = -1;                             // element pick in the type slot, -1 = none (Plain)
    int typeLvl = 0;
    int ability[kMaxAbilitySlots] = {-1, -1, -1};   // ability picks, -1 = empty
    int abilityLvl[kMaxAbilitySlots] = {0, 0, 0};
    int mods[kModifierCount] = {};             // stacks per modifier (modifierIndex)

    // ---- any slot, addressed 0..kLoadoutSlots-1 (see kSlotType / kSlotAbility)
    int kindAt(int s) const {
        if (isItemSlot(s)) return gear[s];
        if (s == kSlotType) return type;
        if (isAbilitySlot(s)) return ability[s - kSlotAbility];
        return -1;
    }
    int levelAt(int s) const {
        if (isItemSlot(s)) return gearLvl[s];
        if (s == kSlotType) return typeLvl;
        if (isAbilitySlot(s)) return abilityLvl[s - kSlotAbility];
        return 0;
    }
    void setSlot(int s, int kind, int level) {
        if (isItemSlot(s)) { gear[s] = kind; gearLvl[s] = level; }
        else if (s == kSlotType) { type = kind; typeLvl = level; }
        else if (isAbilitySlot(s)) { ability[s - kSlotAbility] = kind; abilityLvl[s - kSlotAbility] = level; }
    }
    void clearSlot(int s) { setSlot(s, -1, 0); }
    int levelUp(int s) {   // +1 level on a filled slot; returns the new level
        if (kindAt(s) < 0) return 0;
        setSlot(s, kindAt(s), levelAt(s) + 1);
        return levelAt(s);
    }

    // Slot holding pick k (item, element or ability), -1 if none.
    int slotOf(UpgradeKind k) const {
        for (int s = 0; s < kLoadoutSlots; ++s)
            if (kindAt(s) == static_cast<int>(k)) return s;
        return -1;
    }
    bool has(UpgradeKind k) const { return slotOf(k) >= 0; }
    int levelOf(UpgradeKind k) const {
        const int s = slotOf(k);
        return s < 0 ? 0 : levelAt(s);
    }
    int elementSlot() const { return type >= 0 ? kSlotType : -1; }
    Element element() const {
        return type < 0 ? Element::Plain
                        : static_cast<Element>(elementItemSlot(static_cast<UpgradeKind>(type)) + 1);
    }

    // ---- classes
    int tagCount(ItemTag t) const {
        int n = 0;
        for (int g : gear)
            if (g >= 0 && itemTag(static_cast<UpgradeKind>(g)) == t) ++n;
        return n;
    }
    bool hasRole(ItemTag t) const { return t != ItemTag::None && tagCount(t) >= 2; }
    // Its classes (at most 2), in slot order: the class of the earliest item first.
    int roles(ItemTag out[2]) const {
        int n = 0;
        for (int g : gear) {
            if (g < 0 || n >= 2) continue;
            const ItemTag t = itemTag(static_cast<UpgradeKind>(g));
            if (!hasRole(t) || (n == 1 && out[0] == t)) continue;
            out[n++] = t;
        }
        return n;
    }
    RoleMask roleMask() const {
        ItemTag r[2];
        RoleMask m = 0;
        for (int i = 0, n = roles(r); i < n; ++i) m |= roleBit(tagRole(r[i]));
        return m;
    }
    // The class it has 4 items of, if any.
    ItemTag ascended() const {
        ItemTag r[2];
        for (int i = 0, n = roles(r); i < n; ++i)
            if (tagCount(r[i]) >= 4) return r[i];
        return ItemTag::None;
    }
    ItemTag leadTag() const {   // its first class, None without one
        ItemTag r[2];
        return roles(r) > 0 ? r[0] : ItemTag::None;
    }
};

// How many ability slots are open on a ball: 1 on every ball, 2 with the Mage
// class (2 Mage items), 3 as an Ancient Mage (4). This is the only place that
// decides it. Losing the class (selling / swapping a Mage item) closes the
// extra slots but never deletes what's in them: those abilities stay on the
// ball asleep (not fired, shown dimmed in TAB) and wake up as soon as the slot
// opens again. A new ability goes in an open slot; a copy of a sleeping one
// still levels it up.
inline int abilitySlotCount(const BallLoadout& b) {
    const int mage = b.tagCount(ItemTag::Mage);
    return std::min(kMaxAbilitySlots, mage >= 4 ? 3 : mage >= 2 ? 2 : 1);
}

inline bool upgradeTakesSlot(UpgradeKind k) {
    const UpgradeCat c = upgradeCat(k);
    return c == UpgradeCat::Item || c == UpgradeCat::Element || c == UpgradeCat::Ability;
}

inline bool upgradeNeedsTarget(UpgradeKind k) {
    return upgradeCat(k) == UpgradeCat::Modifier || upgradeTakesSlot(k);
}

// Taking `k` on this ball levels up the copy it already has (instead of
// filling a slot).
inline bool upgradeLevelsUp(UpgradeKind k, const BallLoadout& b) {
    return upgradeTakesSlot(k) && b.has(k);
}

// Can pick `k` go into slot `s` of this ball? Items: an item slot. Elements:
// the type slot. Abilities: an open ability slot.
inline bool slotAccepts(UpgradeKind k, int s, const BallLoadout& b) {
    switch (upgradeCat(k)) {
        case UpgradeCat::Item:    return isItemSlot(s);
        case UpgradeCat::Element: return s == kSlotType;
        case UpgradeCat::Ability: return isAbilitySlot(s) && s - kSlotAbility < abilitySlotCount(b);
        default:                  return false;
    }
}

// Can pick `k` go on this ball? A duplicate levels up the equipped one, until
// kMaxItemLevel; a new element / ability swaps the old one if the slot is
// full; Conductor / Bedrock only with their element. Modifiers: always.
inline bool upgradeFitsBall(UpgradeKind k, const BallLoadout& b) {
    switch (upgradeCat(k)) {
        case UpgradeCat::Modifier: return true;
        case UpgradeCat::Element:
        case UpgradeCat::Ability:
            return !b.has(k) || b.levelOf(k) < kMaxItemLevel;
        case UpgradeCat::Item:
            if (b.has(k)) return b.levelOf(k) < kMaxItemLevel;
            if (k == UpgradeKind::Conductor) return b.element() == Element::Electric;
            if (k == UpgradeKind::Bedrock) return b.element() == Element::Stone;
            return true;
        default: return false;
    }
}

// Where a pick lands on this ball when no slot is picked: a duplicate levels
// its own slot; an element goes in the type slot; an ability in the first free
// open ability slot (else the first one); an item in the first free item slot,
// else slot 0.
inline int defaultSlot(UpgradeKind k, const BallLoadout& b) {
    if (b.has(k)) return b.slotOf(k);
    switch (upgradeCat(k)) {
        case UpgradeCat::Element: return kSlotType;
        case UpgradeCat::Ability:
            for (int i = 0; i < abilitySlotCount(b); ++i)
                if (b.ability[i] < 0) return kSlotAbility + i;
            return kSlotAbility;
        default:
            for (int i = 0; i < kBallSlots; ++i)
                if (b.gear[i] < 0) return i;
            return 0;
    }
}

// What the roll needs to know to drop picks that would do nothing.
struct UpgradeCtx {
    const std::vector<BallLoadout>* balls = nullptr;
    int maxBalls = 5;
    bool elemUnlocked[kElementItemCount] = {};   // element web node has a level
    bool spring = false;
    bool slowField = false;
    bool strongArm = false;
    bool contagion = false;
    bool primed = false;
    bool catalyst = false;
    bool chainReaction = false;
    bool luckyClover = false;
    bool glassCannon = false;
    bool magneticCore = false;
    bool prismCore = false;
    bool phoenix = false;
    bool timeDilation = false;
    bool overcharge = false;
    // Picks still behind the web: legendaries without their node, and every
    // item of a class that isn't unlocked yet.
    std::bitset<kMaxUpgradeKinds> locked;
    void lock(UpgradeKind k) { locked.set(static_cast<std::size_t>(k)); }
    bool isLocked(UpgradeKind k) const { return locked.test(static_cast<std::size_t>(k)); }
};

inline int elementsUnlocked(const UpgradeCtx& c) {
    int n = 0;
    for (bool u : c.elemUnlocked)
        if (u) ++n;
    return n;
}

inline bool upgradeEligible(UpgradeKind k, const UpgradeCtx& c) {
    if (c.isLocked(k)) return false;
    const int ballCount = c.balls ? static_cast<int>(c.balls->size()) : 0;
    auto anyBallFits = [&] {
        if (!c.balls) return false;
        for (const BallLoadout& b : *c.balls)
            if (upgradeFitsBall(k, b)) return true;
        return false;
    };
    switch (upgradeCat(k)) {
        case UpgradeCat::NewBall:  return ballCount < c.maxBalls;
        case UpgradeCat::Modifier:
        case UpgradeCat::Ability:  return anyBallFits();
        case UpgradeCat::Element:  return c.elemUnlocked[elementItemSlot(k)] && anyBallFits();
        case UpgradeCat::Item:
            if (k == UpgradeKind::Shatter && !c.elemUnlocked[3]) return false;   // ice
            return anyBallFits();
        case UpgradeCat::Relic: break;
    }
    switch (k) {
        case UpgradeKind::CoreSpring:    return !c.spring;
        case UpgradeKind::CoreSlowField: return !c.slowField;
        case UpgradeKind::StrongArm:     return !c.strongArm;
        case UpgradeKind::Contagion:     return !c.contagion && c.elemUnlocked[1];   // poison
        case UpgradeKind::Primed:        return !c.primed && elementsUnlocked(c) > 0;
        case UpgradeKind::Catalyst:      return !c.catalyst && elementsUnlocked(c) >= 2;
        case UpgradeKind::ChainReaction: return !c.chainReaction && elementsUnlocked(c) >= 2;
        case UpgradeKind::LuckyClover:   return !c.luckyClover;
        case UpgradeKind::GlassCannon:   return !c.glassCannon;
        case UpgradeKind::MagneticCore:  return !c.magneticCore;
        case UpgradeKind::PrismCore:     return !c.prismCore;
        case UpgradeKind::Phoenix:       return !c.phoenix;
        case UpgradeKind::TimeDilation:  return !c.timeDilation;
        case UpgradeKind::Overcharge:    return !c.overcharge;
        default:                         return false;
    }
}

// Does any item carry this tag? (A newer class may have none yet - it can't
// be picked as a starting class then.)
inline bool classHasItems(ItemTag t) {
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        if (upgradeCat(k) == UpgradeCat::Item && itemTag(k) == t) return true;
    }
    return false;
}

// ---- permanent meta unlocks: the skill web in the game menu ----------------
//
// The menu draws these as a radial graph. The centre node ("Calling") opens
// the web; around it, one ROUTE per class radiates outward, and each class
// sits near the END of its own themed route (2026-09-27, "routes by class"):
//   Striker  (top)         damage / throw power
//   Shooter  (upper right) ball speed, speed power-ups
//   Jester   (right)       gold, cores, rerolls, luck
//   Assassin (lower right) crits, elite kills, execute
//   Pacts    (bottom)      its own branch, no class
//   Summoner (bottom)      copies, recruits, the starter kit, Gemini
//   Support  (lower left)  power-ups, marks
//   Mage     (left)        abilities (unlocked here) and elements
//   Guardian (upper left)  the core: health, heals, shields
// Past each class node: "<Class> lore" (its items show up more often) and one
// class-flavoured perk. A node can only be bought once the node that gates it
// (`parent`) has at least one level. Most cost cores; the prism nodes cost
// prisms (the win currency).
//
// SAVE: node indices are append-only - never reorder, never renumber; the
// layout (branch, parent, position) is free to change. v9 inserted the five
// extra Special-ball nodes (pre-v9 saves reset their unlock levels); v11
// appended the Fase-A nodes; v15 turned the root "Squad" (+1 starting ball) into
// "Calling" and appended the class unlocks; v16 moved every node onto the class
// routes, turned "Calling" into the start-ability pick's 4th card and appended
// the route perks and ability unlocks (58+).

enum MetaUnlock {
    MetaCalling,      // Calling   - a 4th card in the run's first-ability pick   (root; was Squad, then the class pick)
    MetaCoreHp,       // Bulwark   - +core HP at the start
    MetaMend,         // Mend      - core heals more between waves
    MetaFireItem,     // Ignition  - unlocks the fire item, +fire potency
    MetaVenom,        // Venom     - unlocks the poison ball item, +poison potency
    MetaTide,         // Tide      - unlocks the water ball item, +water potency
    MetaFrost,        // Frost     - unlocks the ice ball item, +freeze time
    MetaQuarry,       // Quarry    - unlocks the stone ball item, +rubble potency
    MetaArc,          // Arc       - unlocks the electric ball item, +zap potency
    MetaBounty,       // Fortune   - earn cores for every enemy killed
    MetaWindfall,     // Windfall  - chance a cleared run pays a 2nd prism
    MetaHeft,         // Heft      - balls start with +contact damage
    MetaMass,         // Mass      - balls start larger
    MetaUplink,       // Uplink    - power-ups appear more often
    MetaCapacitor,    // Capacitor - power-ups last longer
    MetaDamper,       // Damper    - unlocks the SLOW MOTION power-up
    MetaFacet,        // Facet     - unlocks the GOLDEN BOUNCE power-up
    MetaOverload,     // Overload  - unlocks the OVERDRIVE power-up
    MetaLedger,       // Ledger    - unlocks the DOUBLE POINTS power-up
    MetaKinetics,     // Kinetics  - unlocks the SPEED SURGE power-up
    MetaReroll,       // Foresight - reroll charges each run to swap an offered item
    // ---- v11 Fase-A append (indices 21+, never reorder) ----
    MetaAegis,        // Aegis      - core shrugs off the first hit(s) of each wave
    MetaRegen,        // Regen      - core regenerates during a wave
    MetaBastion,      // Bastion    - core max HP grows each wave
    MetaSalvage,      // Salvage    - more cores per enemy kill
    MetaInterest,     // Interest   - cores for clearing a wave with no core damage
    MetaProspector,   // Prospector - skipping a pick refunds reroll charges
    MetaStockpile,    // Stockpile  - a random power-up refills a reserve slot (key Q)
    MetaMagnet,       // Magnet     - power-up orbs drift toward the nearest ball
    MetaAfterglow,    // Afterglow  - continuous power-ups fade out instead of cutting
    MetaCharged,      // Charged    - power-ups start with part of their duration
    MetaEmber,        // Ember      - fire ball hits apply a burn (fire has no DoT alone)
    // ---- v12 append (indices 32+, never reorder) ----
    MetaArmory,       // Armory       - epic picks turn up more often
    MetaSatellite,    // Satellite    - the Satellite legendary can appear
    MetaGravity,      // Singularity  - the Gravity well legendary can appear
    MetaGemini,       // Twins        - the Gemini legendary can appear
    MetaPrism,        // Prism        - the Prism core legendary relic can appear
    MetaLuckyStar,    // Lucky star   - better tier odds on every roll
    MetaHaggler,      // Haggler      - shop prices drop
    MetaStarterKit,   // Starter kit  - start the run with a free item
    MetaEliteSpoils,  // Elite spoils - elites pay more gold
    // ---- v13 append (indices 41+, never reorder): the Pacts branch + shop / start nodes ----
    MetaOath,         // Oath          - the boss offers 4 pacts instead of 3        (Pacts hub)
    MetaCovenant,     // Covenant      - start every run by choosing a pact
    MetaPactHunters,  // Hunters       - the Hunters pact can be offered
    MetaPactLegion,   // Legion        - the Legion pact can be offered
    MetaPactDice,     // Loaded dice   - the Loaded Dice pact can be offered
    MetaPactAlchemy,  // Alchemy       - the Alchemy pact can be offered
    MetaMerchant,     // Merchant      - shops stock more and their sale is deeper
    MetaTreasury,     // Treasury      - start each run with gold
    MetaQuartermaster,// Quartermaster - choose the Starter kit item from 4 cards
    MetaLastStand,    // Last stand    - once per run the core comes back from 0
    // ---- v15 append (indices 51+, never reorder): the class unlocks ----
    MetaClassSupport, // Support   - Support items can appear
    MetaClassGuardian,// Guardian  - Guardian items can appear
    MetaClassMage,    // Mage      - Mage items can appear
    MetaClassShooter, // Shooter   - Shooter items can appear
    MetaClassAssassin,// Assassin  - Assassin items can appear
    MetaClassSummoner,// Summoner  - Summoner items can appear
    MetaClassJester,  // Jester    - Jester items can appear
    // ---- v16 append (indices 58+, never reorder): route perks + ability unlocks ----
    MetaSling,        // Sling          - throws fly faster                               (Striker)
    MetaMomentum,     // Momentum       - Striker balls hit harder
    MetaLoreStriker,  // Striker lore   - Striker items show up more often
    MetaVelocity,     // Velocity       - every ball cruises faster                       (Shooter)
    MetaLoreShooter,  // Shooter lore
    MetaCaliber,      // Caliber        - Shooter bullets hit harder
    MetaLoreJester,   // Jester lore                                                      (Jester)
    MetaFoolsLuck,    // Fool's luck    - luck, more with Jester balls
    MetaKeenInstinct, // Keen instinct  - every ball crits more often                     (Assassin)
    MetaLoreAssassin, // Assassin lore
    MetaDeathmark,    // Deathmark      - Assassin balls finish off low enemies
    MetaBrood,        // Brood          - ghost copies last longer                        (Summoner)
    MetaMuster,       // Muster         - a new ball joins with a Common item
    MetaLoreSummoner, // Summoner lore
    MetaBond,         // Bond           - summons hit harder and last longer
    MetaLoreSupport,  // Support lore                                                     (Support)
    MetaRally,        // Rally          - marked enemies take more from every hit
    MetaAbilityMissile,  // Magic missile  - the Magic missile ability can be offered    (Mage)
    MetaChannel,         // Channel        - every ability recharges faster
    MetaAbilityArc,      // Arc (ability)  - the Arc ability can be offered
    MetaAbilityBulwark,  // Bulwark (ab.)  - the Bulwark ability can be offered
    MetaAbilitySplit,    // Split          - the Split ability can be offered
    MetaAbilityOverclock,// Overclock      - the Overclock ability can be offered
    MetaAbilityMeteor,   // Meteor         - the Meteor ability can be offered
    MetaLoreMage,     // Mage lore
    MetaArchive,      // Archive        - Mage balls recharge faster still
    MetaLoreGuardian, // Guardian lore                                                    (Guardian)
    MetaStonewall,    // Stonewall      - Guardian balls patch the core on core bounces
    MetaUnlockCount
};

// A route = a class (same order as ItemTag: Striker = 1 ... Jester = 8), plus
// the root and the Pacts branch. Not saved.
enum class MetaBranch { Root, Striker, Guardian, Support, Mage, Shooter, Assassin, Summoner, Jester, Pacts };
inline constexpr int kMetaBranchCount = 10;
inline ItemTag metaBranchTag(MetaBranch b) {   // the class a route leads to (None: root / Pacts)
    return b == MetaBranch::Root || b == MetaBranch::Pacts ? ItemTag::None : static_cast<ItemTag>(static_cast<int>(b));
}
enum class MetaCurrency { Cores, Prisms };

struct MetaUnlockDef {
    const char* name;
    const char* effect;
    std::uint32_t baseCost;
    int maxLevel;
    MetaBranch branch;
    MetaCurrency currency;
    int parent;     // node that gates this one; -1 for the root
    float ang;      // layout: direction from the centre, degrees clockwise from straight up
    float ring;     // ...and distance, in rings (0 = the centre)
};

inline const MetaUnlockDef& metaUnlockDef(int u) {
    using B = MetaBranch;
    constexpr MetaCurrency C = MetaCurrency::Cores, P = MetaCurrency::Prisms;
    static const MetaUnlockDef defs[MetaUnlockCount] = {
        /* Calling   */ {"Calling",   "opens the web. Every run starts by picking your ball's first ability: with Calling you choose from 4 cards instead of 3",
                         8u,  1, B::Root,     C, -1,   0.f, 0.f},
        /* Bulwark   */ {"Bulwark",   "start with +20 core health",
                         10u, 3, B::Guardian, C,  0, 330.f, 1.f},
        /* Mend      */ {"Mend",      "the core heals +3 more between waves",
                         12u, 3, B::Guardian, C,  1, 316.f, 2.f},
        /* Ignition  */ {"Ignition",  "the fire element can appear; higher levels hit harder",
                         2u,  3, B::Mage,     P, MetaAbilityMissile, 294.f, 2.f},
        /* Venom     */ {"Venom",     "the poison element can appear; higher levels stack faster",
                         2u,  3, B::Mage,     P,  3, 294.f, 3.f},
        /* Tide      */ {"Tide",      "the water element can appear; higher levels leave a wider wake",
                         3u,  3, B::Mage,     P,  4, 294.f, 4.f},
        /* Frost     */ {"Frost",     "the ice element can appear; higher levels freeze for longer",
                         3u,  3, B::Mage,     P,  5, 294.f, 5.f},
        /* Quarry    */ {"Quarry",    "the stone element can appear; higher levels grind harder",
                         4u,  3, B::Mage,     P,  6, 294.f, 6.f},
        /* Arc       */ {"Static",    "the electric element can appear; higher levels zap harder",
                         4u,  3, B::Mage,     P,  7, 294.f, 7.f},
        /* Fortune   */ {"Fortune",   "earn cores for every enemy you kill",
                         6u,  3, B::Jester,   C,  0,  72.f, 1.f},
        /* Windfall  */ {"Windfall",  "20% chance a cleared run pays a 2nd prism",
                         14u, 1, B::Jester,   C, MetaSalvage, 56.f, 3.f},
        /* Heft      */ {"Heft",      "every ball gets +8% contact damage",
                         10u, 2, B::Striker,  C,  0,   0.f, 1.f},
        /* Mass      */ {"Mass",      "every ball is 10% larger",
                         12u, 2, B::Guardian, C,  1, 330.f, 2.f},
        /* Uplink    */ {"Uplink",    "power-ups appear more often",
                         8u,  3, B::Support,  C,  0, 222.f, 1.f},
        /* Capacitor */ {"Capacitor", "power-ups last longer",
                         8u,  3, B::Support,  C, 13, 222.f, 2.f},
        /* Damper    */ {"Damper",    "unlocks the Slow Motion power-up",
                         2u,  1, B::Support,  P, 13, 237.f, 2.f},
        /* Facet     */ {"Facet",     "unlocks the Golden Bounce power-up",
                         2u,  1, B::Support,  P, 15, 238.f, 3.f},
        /* Overload  */ {"Overload",  "unlocks the Overdrive power-up",
                         3u,  1, B::Shooter,  P, 19,  45.f, 2.1f},
        /* Ledger    */ {"Ledger",    "the DOUBLE POINTS power-up can appear",
                         6u,  1, B::Support,  C, 13, 207.f, 2.f},
        /* Kinetics  */ {"Kinetics",  "the SPEED SURGE power-up can appear",
                         6u,  1, B::Shooter,  C, MetaVelocity, 27.f, 2.f},
        /* Foresight */ {"Foresight", "start each run with reroll charges to swap an offered item",
                         10u, 3, B::Jester,   C,  9,  72.f, 2.f},
        /* Aegis     */ {"Aegis",     "the core shrugs off the first hit of each wave (+1 hit per level)",
                         12u, 2, B::Guardian, C,  1, 344.f, 2.f},
        /* Regen     */ {"Regen",     "the core slowly regenerates during a wave, not only between them",
                         12u, 3, B::Guardian, C,  2, 316.f, 3.f},
        /* Bastion   */ {"Bastion",   "the core's max health grows a little with every wave cleared",
                         14u, 2, B::Guardian, C, 22, 316.f, 4.f},
        /* Salvage   */ {"Salvage",   "enemies drop cores more often",
                         10u, 3, B::Jester,   C,  9,  56.f, 2.f},
        /* Interest  */ {"Interest",  "clearing a wave with no core damage pays a core bonus",
                         12u, 3, B::Jester,   C, 10,  56.f, 4.f},
        /* Prospector*/ {"Prospector","skipping a pick to repair the core refunds a reroll charge",
                         12u, 2, B::Jester,   C, 25,  56.f, 5.f},
        /* Stockpile */ {"Stockpile", "keep one random power-up in reserve; press Q to use it",
                         6u,  1, B::Support,  C, 18, 206.f, 3.f},
        /* Magnet    */ {"Magnet",    "power-up orbs drift toward your nearest ball",
                         10u, 1, B::Support,  C, 27, 206.f, 4.f},
        /* Afterglow */ {"Afterglow", "when a power-up ends its effect fades out instead of cutting",
                         10u, 2, B::Support,  C, 28, 207.f, 5.f},
        /* Charged   */ {"Charged",   "power-ups arrive with part of their duration already charged",
                         10u, 2, B::Support,  C, 14, 222.f, 3.f},
        /* Ember     */ {"Ember",     "fire ball hits set enemies alight for a burn; scales with Ignition",
                         2u,  3, B::Mage,     P,  3, 303.f, 3.f},
        /* Armory    */ {"Armory",    "Epic picks turn up more often (+50% odds per level)",
                         2u,  2, B::Jester,   P, 47,  88.f, 5.f},
        /* Satellite */ {"Satellite", "the Satellite legendary can appear: a ball that orbits the core",
                         2u,  1, B::Guardian, P, 21, 345.f, 3.f},
        /* Singularity*/{"Singularity","the Gravity well legendary can appear: a ball that drags enemies in",
                         3u,  1, B::Support,  P, 16, 238.f, 4.f},
        /* Twins     */ {"Twins",     "the Gemini legendary can appear: a permanent ghost twin",
                         3u,  1, B::Summoner, P, MetaBrood, 192.f, 2.3f},
        /* Prism     */ {"Prism",     "the Prism core legendary relic can appear: reactions everywhere",
                         4u,  1, B::Mage,     P,  5, 303.f, 4.6f},
        /* Lucky star*/ {"Lucky star","+2 luck per level: higher chances and rarer cards",
                         12u, 3, B::Jester,   C, 20,  72.f, 3.f},
        /* Haggler   */ {"Haggler",   "shop prices drop 10% per level",
                         10u, 3, B::Jester,   C, 48,  88.f, 3.f},
        /* StarterKit*/ {"Starter kit","start every run with a free item on your first ball (Uncommon, then Rare)",
                         14u, 2, B::Summoner, C, MetaBrood, 164.f, 2.3f},
        /* EliteSpoils*/{"Elite spoils","elite fights pay +50% gold per level",
                         12u, 2, B::Assassin, C, MetaKeenInstinct, 115.f, 2.f},
        /* Oath      */ {"Oath",      "after the act-1 boss, choose from 4 pacts instead of 3. Opens the Pacts.",
                         14u, 1, B::Pacts,    C,  0, 145.f, 1.f},
        /* Covenant  */ {"Covenant",  "start every run by choosing a pact (1 of 3) - with the boss's, a run can hold two",
                         3u,  1, B::Pacts,    P, 41, 145.f, 2.f},
        /* Hunters   */ {"Hunters",   "the Hunters pact can be offered: every ball chases its own prey, hands off",
                         2u,  1, B::Pacts,    P, 41, 136.f, 2.6f},
        /* Legion    */ {"Legion",    "the Legion pact can be offered: two more balls at once, clacks throw sparks",
                         2u,  1, B::Pacts,    P, 41, 154.f, 2.6f},
        /* Dice      */ {"Loaded dice","the Loaded Dice pact can be offered: +12 luck, fight gold is double or nothing",
                         3u,  1, B::Pacts,    P, 43, 138.f, 3.6f},
        /* Alchemy   */ {"Alchemy",   "the Alchemy pact can be offered: random extra elements, a ball reacts with itself",
                         3u,  1, B::Pacts,    P, 44, 152.f, 3.6f},
        /* Merchant  */ {"Merchant",  "shops stock one more pick and their sale gets 15% deeper per level",
                         12u, 2, B::Jester,   C, 38,  88.f, 4.f},
        /* Treasury  */ {"Treasury",  "start every run with +20 gold per level",
                         10u, 3, B::Jester,   C,  9,  88.f, 2.f},
        /* Quartermaster*/{"Quartermaster","choose your Starter kit item from 4 cards instead of getting a random one",
                         16u, 1, B::Summoner, C, 39, 163.f, 3.3f},
        /* Last stand*/ {"Last stand","once per run, when the core breaks it comes back at half health",
                         20u, 1, B::Guardian, C, 23, 316.f, 5.f},
        // ---- the classes: each near the end of its route ----
        /* Support   */ {"Support",   "unlocks the Support class: its items can appear (weak hits that mark enemies for every ball)",
                         12u, 1, B::Support,  C, 30, 222.f, 4.f},
        /* Guardian  */ {"Guardian",  "unlocks the Guardian class: its items can appear (big, shoves and staggers, guards the core)",
                         12u, 1, B::Guardian, C, 12, 330.f, 3.f},
        /* Mage      */ {"Mage",      "unlocks the Mage class: its items can appear (more ability slots; a Mage ball gets Magic missile)",
                         18u, 1, B::Mage,     C, MetaChannel, 280.f, 3.f},
        /* Shooter   */ {"Shooter",   "unlocks the Shooter class: its items can appear (fires bullets)",
                         18u, 1, B::Shooter,  C, 19,  27.f, 3.f},
        /* Assassin  */ {"Assassin",  "unlocks the Assassin class: its items can appear (teleports to the next enemy on a kill)",
                         18u, 1, B::Assassin, C, 40, 115.f, 3.f},
        /* Summoner  */ {"Summoner",  "unlocks the Summoner class: its items can appear (summons helpers)",
                         18u, 1, B::Summoner, C, MetaMuster, 178.f, 3.f},
        /* Jester    */ {"Jester",    "unlocks the Jester class: its items can appear (plays on chance)",
                         18u, 1, B::Jester,   C, 37,  72.f, 4.f},
        // ---- v16: route perks and ability unlocks ----
        /* Sling     */ {"Sling",     "every throw flies 8% faster per level",
                         8u,  3, B::Striker,  C, 11,   0.f, 2.f},
        /* Momentum  */ {"Momentum",  "balls with the Striker class hit 10% harder per level",
                         12u, 2, B::Striker,  C, 58,   0.f, 3.f},
        /* StrikerLore*/{"Striker lore","Striker items show up 50% more often per level",
                         10u, 2, B::Striker,  C, 59,   0.f, 4.f},
        /* Velocity  */ {"Velocity",  "every ball cruises 5% faster per level",
                         8u,  2, B::Shooter,  C,  0,  27.f, 1.f},
        /* ShooterLore*/{"Shooter lore","Shooter items show up 50% more often per level",
                         10u, 2, B::Shooter,  C, 54,  21.f, 4.f},
        /* Caliber   */ {"Caliber",   "Shooter bullets hit 15% harder per level",
                         12u, 2, B::Shooter,  C, 54,  33.f, 4.f},
        /* JesterLore*/ {"Jester lore","Jester items show up 50% more often per level",
                         10u, 2, B::Jester,   C, 57,  67.f, 5.f},
        /* FoolsLuck */ {"Fool's luck","+2 luck per level, and +2 more per level for every ball with the Jester class",
                         12u, 2, B::Jester,   C, 57,  77.f, 5.f},
        /* Keen      */ {"Keen instinct","every ball has a 4% chance per level to land a critical hit (x2)",
                         8u,  3, B::Assassin, C,  0, 115.f, 1.f},
        /* AssassinLore*/{"Assassin lore","Assassin items show up 50% more often per level",
                         10u, 2, B::Assassin, C, 55, 109.f, 4.f},
        /* Deathmark */ {"Deathmark", "Assassin balls finish off any enemy a hit leaves under 5% health per level",
                         12u, 2, B::Assassin, C, 55, 121.f, 4.f},
        /* Brood     */ {"Brood",     "ghost copies (Split shot, Mitosis, Split, Phantom) last 25% longer per level",
                         8u,  2, B::Summoner, C,  0, 178.f, 1.f},
        /* Muster    */ {"Muster",    "every new ball joins with a random Common item",
                         12u, 1, B::Summoner, C, 69, 178.f, 2.f},
        /* SummonerLore*/{"Summoner lore","Summoner items show up 50% more often per level",
                         10u, 2, B::Summoner, C, 56, 172.f, 4.f},
        /* Bond      */ {"Bond",      "every summon hits 15% harder and lasts 15% longer per level",
                         12u, 2, B::Summoner, C, 56, 184.f, 4.f},
        /* SupportLore*/{"Support lore","Support items show up 50% more often per level",
                         10u, 2, B::Support,  C, 51, 217.f, 5.f},
        /* Rally     */ {"Rally",     "marked enemies take +10% more from every hit per level",
                         12u, 2, B::Support,  C, 51, 227.f, 5.f},
        /* Missile   */ {"Magic missile","the Magic missile ability can be offered: homing missiles that follow enemies",
                         6u,  1, B::Mage,     C,  0, 280.f, 1.f},
        /* Channel   */ {"Channel",   "every ability recharges 6% faster per level",
                         8u,  3, B::Mage,     C, 75, 280.f, 2.f},
        /* AbArc     */ {"Arc",       "the Arc ability can be offered: a bolt that leaps through enemies",
                         8u,  1, B::Mage,     C, 75, 266.f, 2.f},
        /* AbBulwark */ {"Bulwark pulse","the Bulwark ability can be offered: the core shoves back when enemies close in",
                         8u,  1, B::Mage,     C, 77, 265.f, 3.f},
        /* AbSplit   */ {"Split",     "the Split ability can be offered: two ghost copies fan out for a moment",
                         12u, 1, B::Mage,     C, 78, 265.f, 4.f},
        /* AbOverclock*/{"Overclock", "the Overclock ability can be offered: a few seconds faster and harder-hitting",
                         12u, 1, B::Mage,     C, 79, 265.f, 5.f},
        /* AbMeteor  */ {"Meteor",    "the Meteor ability can be offered: crushes the thickest pack of enemies",
                         16u, 1, B::Mage,     C, 80, 266.f, 6.f},
        /* MageLore  */ {"Mage lore", "Mage items show up 50% more often per level",
                         10u, 2, B::Mage,     C, 53, 275.f, 4.f},
        /* Archive   */ {"Archive",   "balls with the Mage class recharge their abilities 15% faster per level",
                         12u, 2, B::Mage,     C, 53, 285.f, 4.f},
        /* GuardianLore*/{"Guardian lore","Guardian items show up 50% more often per level",
                         10u, 2, B::Guardian, C, 52, 324.f, 4.f},
        /* Stonewall */ {"Stonewall", "balls with the Guardian class patch the core up by 0.5 per level on every core bounce",
                         12u, 2, B::Guardian, C, 52, 336.f, 4.f},
    };
    return defs[u];
}

// "<Class> lore": that class's items show up more often. -1 for none.
inline int classLoreNode(ItemTag t) {
    switch (t) {
        case ItemTag::Striker:  return MetaLoreStriker;
        case ItemTag::Guardian: return MetaLoreGuardian;
        case ItemTag::Support:  return MetaLoreSupport;
        case ItemTag::Mage:     return MetaLoreMage;
        case ItemTag::Shooter:  return MetaLoreShooter;
        case ItemTag::Assassin: return MetaLoreAssassin;
        case ItemTag::Summoner: return MetaLoreSummoner;
        case ItemTag::Jester:   return MetaLoreJester;
        default:                return -1;
    }
}

// The web node that lets an ability be offered; -1 = open from the start
// (Dash and Nova).
inline int abilityUnlockNode(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AbilityMissile:   return MetaAbilityMissile;
        case UpgradeKind::AbilityArc:       return MetaAbilityArc;
        case UpgradeKind::AbilityBulwark:   return MetaAbilityBulwark;
        case UpgradeKind::AbilitySplit:     return MetaAbilitySplit;
        case UpgradeKind::AbilityOverclock: return MetaAbilityOverclock;
        case UpgradeKind::AbilityMeteor:    return MetaAbilityMeteor;
        default:                            return -1;
    }
}

inline bool abilityUnlocked(UpgradeKind k, const int* levels) {
    const int n = abilityUnlockNode(k);
    return n < 0 || levels[n] > 0;
}

inline bool metaUnlockMaxed(int u, int level) { return level >= metaUnlockDef(u).maxLevel; }

inline std::uint32_t metaUnlockCost(int u, int level) {
    std::uint32_t c = metaUnlockDef(u).baseCost;
    for (int i = 0; i < level; ++i) c *= 2u;
    return c;
}

inline MetaCurrency metaUnlockCurrency(int u) { return metaUnlockDef(u).currency; }

// A node opens up once the node toward the centre that gates it has a level.
inline bool metaUnlockAvailable(int u, const int* levels) {
    const int p = metaUnlockDef(u).parent;
    return p < 0 || levels[p] > 0;
}

// The web node that unlocks a class; -1 for Striker (always open).
inline int classUnlockNode(ItemTag t) {
    switch (t) {
        case ItemTag::Support:  return MetaClassSupport;
        case ItemTag::Guardian: return MetaClassGuardian;
        case ItemTag::Mage:     return MetaClassMage;
        case ItemTag::Shooter:  return MetaClassShooter;
        case ItemTag::Assassin: return MetaClassAssassin;
        case ItemTag::Summoner: return MetaClassSummoner;
        case ItemTag::Jester:   return MetaClassJester;
        default:                return -1;
    }
}

inline bool classUnlocked(ItemTag t, const int* levels) {
    const int n = classUnlockNode(t);
    return n < 0 || levels[n] > 0;
}

}  // namespace sb
