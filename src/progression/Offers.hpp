#pragma once

#include <cstdint>

namespace sb {

// ---- between-wave offers (spent with scrap / picked for free) ----------------

enum class OfferKind { AddBall, MoreDamage, AddBlackHole, CoreRepair, Scrap };
inline constexpr int kOfferKindCount = 5;

struct OfferInfo {
    const char* title;
    const char* desc;
};

inline OfferInfo offerInfo(OfferKind k) {
    switch (k) {
        case OfferKind::AddBall:      return {"Another ball", "one more ball in play"};
        case OfferKind::MoreDamage:   return {"More damage", "+25% ball damage"};
        case OfferKind::AddBlackHole: return {"Black hole", "one more structure that bends the balls"};
        case OfferKind::CoreRepair:   return {"Repair core", "+35 core health"};
        case OfferKind::Scrap:        return {"Scrap", "+40 scrap right now"};
    }
    return {"", ""};
}

// ---- permanent meta unlocks (spent with cores, in the hub) ------------------

enum MetaUnlock { MetaStartBalls, MetaCoreHp, MetaUnlockCount };

struct MetaUnlockDef {
    const char* name;
    const char* effect;
    std::uint32_t baseCost;
    int maxLevel;
};

inline const MetaUnlockDef& metaUnlockDef(int u) {
    static const MetaUnlockDef defs[MetaUnlockCount] = {
        {"Squad", "start every run with one more ball", 3u, 3},
        {"Reinforced core", "start with +60 core health", 4u, 1},
    };
    return defs[u];
}

inline bool metaUnlockMaxed(int u, int level) { return level >= metaUnlockDef(u).maxLevel; }

inline std::uint32_t metaUnlockCost(int u, int level) {
    std::uint32_t c = metaUnlockDef(u).baseCost;
    for (int i = 0; i < level; ++i) c *= 2u;
    return c;
}

}  // namespace sb
