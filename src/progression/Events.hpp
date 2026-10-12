#pragma once

#include <string>

#include "core/Config.hpp"

namespace sb {

// "?" stop events (2026-10-01): now and then a "?" holds a stranger with a
// couple of deals for gold instead of the free pick - ways to spend (or
// raise) gold between shops. Take one, or walk away. App::openEvent rolls them,
// App::takeEventDeal applies them; tuning in cfg::event.

// Rest and Temper are the Rest stop's two choices (2026-10-02), never rolled
// on a "?": they sit after kEventKindCount.
// The second batch (2026-10-05) trades in items, balls and abilities, not just gold.
// The third batch (2026-10-12) pays in the core and in levels, not gold, and
// one rare deal lifts an Altar pact's price.
enum class EventKind { Drifter, Smuggler, BloodPrice, Tithe, Gamble, Smith,
                       Barter, Mercenary, Gift, Tome, BloodForge, Cocoon,
                       DarkBargain, FieldMedic, BloodRite, Purify, Scavenge,
                       Rest, Temper,
                       Ascend, Expand };   // a Shrine's two (2026-10-06)
inline constexpr int kEventKindCount = 17;

// The gold a deal costs (or, for Blood price, pays) in act `act`.
inline int eventGold(EventKind k, int act) {
    namespace E = cfg::event;
    const int a = act > 1 ? act - 1 : 0;
    switch (k) {
        case EventKind::Drifter:    return E::ballPrice + E::ballPerAct * a;
        case EventKind::Smuggler:   return E::smugglerPrice + E::smugglerPerAct * a;
        case EventKind::BloodPrice: return E::bloodGold + E::bloodPerAct * a;
        case EventKind::Tithe:      return E::tithePrice + E::tithePerAct * a;
        case EventKind::Gamble:     return E::gambleStake + E::gamblePerAct * a;
        case EventKind::Smith:      return E::smithPrice + E::smithPerAct * a;
        case EventKind::Tome:       return E::tomePrice + E::tomePerAct * a;
        case EventKind::Purify:     return E::purifyPrice + E::purifyPerAct * a;
        case EventKind::DarkBargain:
        case EventKind::Scavenge:
        case EventKind::FieldMedic:
        case EventKind::BloodRite:
        case EventKind::Barter:
        case EventKind::Mercenary:
        case EventKind::Gift:
        case EventKind::BloodForge:
        case EventKind::Cocoon:
        case EventKind::Rest:
        case EventKind::Temper:
        case EventKind::Ascend:
        case EventKind::Expand:     return 0;
    }
    return 0;
}

// Does taking it cost gold (the button needs that much), or pay it?
inline bool eventPays(EventKind k) { return k == EventKind::BloodPrice; }
// Free of gold: its price is something else (an item, a ball's time, core HP...).
inline bool eventFree(EventKind k) {
    return k == EventKind::Barter || k == EventKind::Mercenary || k == EventKind::Gift ||
           k == EventKind::BloodForge || k == EventKind::Cocoon || k == EventKind::Rest || k == EventKind::Temper ||
           k == EventKind::DarkBargain || k == EventKind::FieldMedic || k == EventKind::BloodRite || k == EventKind::Scavenge ||
           k == EventKind::Ascend || k == EventKind::Expand;
}

inline const char* eventName(EventKind k) {
    switch (k) {
        case EventKind::Drifter:    return "Drifter";
        case EventKind::Smuggler:   return "Smuggler";
        case EventKind::BloodPrice: return "Blood price";
        case EventKind::Tithe:      return "Tithe";
        case EventKind::Gamble:     return "Coin flip";
        case EventKind::Smith:      return "Wandering smith";
        case EventKind::Barter:     return "Barter";
        case EventKind::Mercenary:  return "Recruiter";
        case EventKind::Gift:       return "Lucky find";
        case EventKind::Tome:       return "Old tome";
        case EventKind::BloodForge: return "Blood forge";
        case EventKind::Cocoon:     return "Strange cocoon";
        case EventKind::DarkBargain: return "Dark bargain";
        case EventKind::FieldMedic: return "Field medic";
        case EventKind::BloodRite:  return "Blood rite";
        case EventKind::Purify:     return "Unbinding";
        case EventKind::Scavenge:   return "Scavenge";
        case EventKind::Rest:       return "Rest";
        case EventKind::Temper:     return "Forge";
        case EventKind::Ascend:     return "Ascend";
        case EventKind::Expand:     return "Expand";
    }
    return "";
}

inline const char* eventHint(EventKind k) {
    switch (k) {
        case EventKind::Drifter:    return "a ball adrift, looking for a crew";
        case EventKind::Smuggler:   return "goods that never pass through a shop";
        case EventKind::BloodPrice: return "gold now, a wound in the core";
        case EventKind::Tithe:      return "pay into the core";
        case EventKind::Gamble:     return "heads or tails";
        case EventKind::Smith:      return "a forge on a cart";
        case EventKind::Barter:     return "one of yours for two of theirs";
        case EventKind::Mercenary:  return "a contract for one of your balls";
        case EventKind::Gift:       return "something shiny on the road";
        case EventKind::Tome:       return "a spell, half legible";
        case EventKind::BloodForge: return "the anvil drinks from the core";
        case EventKind::Cocoon:     return "it twitches";
        case EventKind::DarkBargain: return "something in the dark wants a piece of the core";
        case EventKind::FieldMedic: return "patches the core with parts of your gear";
        case EventKind::BloodRite:  return "a spell sharpened on the core";
        case EventKind::Purify:     return "a hermit who knows the Altar's words backwards";
        case EventKind::Scavenge:   return "a wreck by the road, still full of gear";
        case EventKind::Rest:       return "tend to the core";
        case EventKind::Temper:     return "a quiet hour at the anvil";
        case EventKind::Ascend:     return "a ball with all 4 items of a class becomes its god";
        case EventKind::Expand:     return "room for a third class";
    }
    return "";
}

// GAIN / COST lines of the card (`gold` = eventGold for this act).
inline std::string eventGain(EventKind k, int gold) {
    namespace E = cfg::event;
    switch (k) {
        case EventKind::Drifter:    return "a new ball joins";
        case EventKind::Smuggler:   return "pick 1 of 3 items, at elite odds";
        case EventKind::BloodPrice: return "+" + std::to_string(gold) + " gold";
        case EventKind::Tithe:
            return "+" + std::to_string(static_cast<int>(E::titheHpFrac * 100.f + 0.5f)) + "% core max HP, healed";
        case EventKind::Gamble:
            return std::to_string(E::gambleWinPct) + "%: win " +
                   std::to_string(static_cast<int>(static_cast<float>(gold) * E::gambleWinMul + 0.5f)) + " gold";
        case EventKind::Smith:
        case EventKind::Temper:     return "a forge visit: 2 points of levels";
        case EventKind::Barter:     return "2 random items, any rarity";
        case EventKind::Mercenary:  return "pick 1 of 3 items, at elite odds";
        case EventKind::Gift:       return "a random modifier on a random ball";
        case EventKind::Tome:       return "a random ability";
        case EventKind::BloodForge: return "a random item levels up";
        case EventKind::Cocoon:     return "in 3 fights: an Epic or Legendary item";
        case EventKind::DarkBargain: return "pick 1 of 3 items, at elite odds";
        case EventKind::FieldMedic:
            return "the core repairs " + std::to_string(static_cast<int>(E::medicHealFrac * 100.f + 0.5f)) + "%";
        case EventKind::BloodRite:  return "a random ability levels up";
        case EventKind::Purify:     return "one of your pacts (at random) loses its price - its gift stays";
        case EventKind::Scavenge:   return "pick 1 of 3 items, Common or Uncommon";
        case EventKind::Rest:       return "the core is repaired to full";
        case EventKind::Ascend:     return "it ascends into its class's god: pick 1 of 3 of the god's items (Epic or better); they roll from now on";
        case EventKind::Expand:     return "a ball gets 2 more item slots (6)";
    }
    return "";
}

inline std::string eventCost(EventKind k, int gold) {
    namespace E = cfg::event;
    if (k == EventKind::BloodPrice)
        return "the core takes " + std::to_string(static_cast<int>(E::bloodHpFrac * 100.f + 0.5f)) + "% damage";
    if (k == EventKind::Gamble) return std::to_string(gold) + " gold, win or lose";
    if (k == EventKind::Rest) return "the core stays as it is";
    if (k == EventKind::Temper) return "the core is not repaired";
    if (k == EventKind::Barter) return "give up one of your items";
    if (k == EventKind::Mercenary)
        return "a random ball sits out the next " + std::to_string(E::mercenaryFights) + " fights";
    if (k == EventKind::Gift) return "free - but you don't choose";
    if (k == EventKind::BloodForge)
        return "the core takes " + std::to_string(static_cast<int>(E::bloodForgeHpFrac * 100.f + 0.5f)) + "% damage";
    if (k == EventKind::Cocoon) return "it takes an item slot meanwhile";
    if (k == EventKind::DarkBargain)
        return "the core takes " + std::to_string(static_cast<int>(E::darkHpFrac * 100.f + 0.5f)) + "% damage";
    if (k == EventKind::FieldMedic) return "a random item loses a level";
    if (k == EventKind::Scavenge) return "free - nothing rare in the wreck";
    if (k == EventKind::BloodRite)
        return "the core takes " + std::to_string(static_cast<int>(E::riteHpFrac * 100.f + 0.5f)) + "% damage";
    if (k == EventKind::Ascend) return "its class's items are gone, and that class's items stop showing up this run";
    if (k == EventKind::Expand) return "that ball can't ascend at a Shrine anymore";
    return std::to_string(gold) + " gold";
}

}  // namespace sb
