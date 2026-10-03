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
enum class EventKind { Drifter, Smuggler, BloodPrice, Tithe, Gamble, Smith, Rest, Temper };
inline constexpr int kEventKindCount = 6;

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
        case EventKind::Rest:
        case EventKind::Temper:     return 0;
    }
    return 0;
}

// Does taking it cost gold (the button needs that much), or pay it?
inline bool eventPays(EventKind k) { return k == EventKind::BloodPrice; }

inline const char* eventName(EventKind k) {
    switch (k) {
        case EventKind::Drifter:    return "Drifter";
        case EventKind::Smuggler:   return "Smuggler";
        case EventKind::BloodPrice: return "Blood price";
        case EventKind::Tithe:      return "Tithe";
        case EventKind::Gamble:     return "Coin flip";
        case EventKind::Smith:      return "Wandering smith";
        case EventKind::Rest:       return "Rest";
        case EventKind::Temper:     return "Forge";
    }
    return "";
}

inline const char* eventHint(EventKind k) {
    switch (k) {
        case EventKind::Drifter:    return "a ball adrift, looking for a crew";
        case EventKind::Smuggler:   return "goods that never pass through a shop";
        case EventKind::BloodPrice: return "gold now, a weaker core for good";
        case EventKind::Tithe:      return "pay into the core";
        case EventKind::Gamble:     return "heads or tails";
        case EventKind::Smith:      return "a forge on a cart";
        case EventKind::Rest:       return "tend to the core";
        case EventKind::Temper:     return "a quiet hour at the anvil";
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
        case EventKind::Temper:     return "level up one item";
        case EventKind::Rest:       return "the core is repaired to full";
    }
    return "";
}

inline std::string eventCost(EventKind k, int gold) {
    namespace E = cfg::event;
    if (k == EventKind::BloodPrice)
        return "-" + std::to_string(static_cast<int>(E::bloodHpFrac * 100.f + 0.5f)) + "% core max HP";
    if (k == EventKind::Gamble) return std::to_string(gold) + " gold, win or lose";
    if (k == EventKind::Rest) return "the core stays as it is";
    if (k == EventKind::Temper) return "the core is not repaired";
    return std::to_string(gold) + " gold";
}

}  // namespace sb
