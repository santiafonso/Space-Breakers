#pragma once

#include <string>
#include <vector>

#include "core/Config.hpp"
#include "progression/Offers.hpp"

namespace sb {

// ---- squad traits (2026-10-22) ---------------------------------------------
//
// Every class brings two of five traits - each pair of traits is exactly one
// class, so every trait is shared by four. Balls with a trait (a ball counts
// once, however many of its classes share it; a god counts as its class) turn
// on its bonus for the whole squad at cfg::team::traitAt balls, and a stronger
// one further up. App::foldTraits folds them into the params.

enum class Trait { Impact, Swift, Arcane, Elemental, Squad };
inline constexpr int kTraitCount = 5;

inline const char* traitName(Trait t) {
    switch (t) {
        case Trait::Impact:    return "Impact";
        case Trait::Swift:     return "Swift";
        case Trait::Arcane:    return "Arcane";
        case Trait::Elemental: return "Elemental";
        case Trait::Squad:     return "Squad";
    }
    return "";
}

// What its level (1 or 2) does.
inline std::string traitDesc(Trait t, int level) {
    namespace T = cfg::team;
    const int i = level >= 2 ? 1 : 0;
    auto pct = [](float m) { return std::to_string(static_cast<int>(m * 100.f - 100.f + 0.5f)) + "%"; };
    switch (t) {
        case Trait::Impact:    return "every ball hits +" + pct(T::impactDamage[i]);
        case Trait::Swift:     return "every ball +" + pct(T::swiftCruise[i]) + " speed";
        case Trait::Arcane:    return "abilities recharge +" + pct(T::arcaneRate[i]) + " faster";
        case Trait::Elemental: return "elements +" + pct(T::elementalPower[i]) + " stronger";
        case Trait::Squad:
            return T::squadAegis[i] == 1 ? std::string("the core shrugs off the first hit of each fight")
                                         : "the core shrugs off the first " + std::to_string(T::squadAegis[i]) + " hits of each fight";
    }
    return "";
}

// A base class's two traits; false for Normal and the gods (use baseOf).
inline bool traitsOf(ItemTag t, Trait& a, Trait& b) {
    using T = Trait;
    switch (t) {
        case ItemTag::Striker:   a = T::Impact;    b = T::Swift;     return true;
        case ItemTag::Guardian:  a = T::Impact;    b = T::Squad;     return true;
        case ItemTag::Alchemist: a = T::Impact;    b = T::Elemental; return true;
        case ItemTag::Jester:    a = T::Impact;    b = T::Arcane;    return true;
        case ItemTag::Assassin:  a = T::Swift;     b = T::Arcane;    return true;
        case ItemTag::Shooter:   a = T::Swift;     b = T::Elemental; return true;
        case ItemTag::Slinger:   a = T::Swift;     b = T::Squad;     return true;
        case ItemTag::Mage:      a = T::Arcane;    b = T::Elemental; return true;
        case ItemTag::Support:   a = T::Arcane;    b = T::Squad;     return true;
        case ItemTag::Summoner:  a = T::Elemental; b = T::Squad;     return true;
        default:                 return false;
    }
}

// "Striker, Guardian, Alchemist, Jester": the classes that bring it.
inline std::string traitClasses(Trait t) {
    std::string s;
    for (int i = 1; i <= static_cast<int>(ItemTag::Alchemist); ++i) {
        Trait a, b;
        if (!traitsOf(static_cast<ItemTag>(i), a, b) || (a != t && b != t)) continue;
        if (!s.empty()) s += ", ";
        s += itemTagName(static_cast<ItemTag>(i));
    }
    return s;
}

// Which traits this ball has: from its classes (2 items of a tag), or the class
// its god ascended from.
inline void ballTraits(const BallLoadout& L, bool out[kTraitCount]) {
    for (int i = 0; i < kTraitCount; ++i) out[i] = false;
    auto add = [&](ItemTag t) {
        Trait a, b;
        if (isGod(t)) t = baseOf(t);
        if (!traitsOf(t, a, b)) return;
        out[static_cast<int>(a)] = out[static_cast<int>(b)] = true;
    };
    if (L.ascendedFrom != 0) add(static_cast<ItemTag>(L.ascendedFrom));
    ItemTag r[3];
    for (int i = 0, n = L.roles(r); i < n; ++i) add(r[i]);
}

// Balls per trait across the squad.
inline void teamTraitCounts(const std::vector<BallLoadout>& balls, int out[kTraitCount]) {
    for (int i = 0; i < kTraitCount; ++i) out[i] = 0;
    for (const BallLoadout& L : balls) {
        bool has[kTraitCount];
        ballTraits(L, has);
        for (int i = 0; i < kTraitCount; ++i) out[i] += has[i] ? 1 : 0;
    }
}

// 0 = off, 1 or 2 = its level.
inline int traitTier(int balls) {
    return balls >= cfg::team::traitAt[1] ? 2 : balls >= cfg::team::traitAt[0] ? 1 : 0;
}

}  // namespace sb
