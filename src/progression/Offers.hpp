#pragma once

#include <algorithm>
#include <cstdint>

#include "sim/Entities.hpp"  // Element

namespace sb {

// ---- between-wave offers: add an elemental ball, paid with scrap -------------

enum class OfferKind { BallFire, BallWind, BallWater, BallStone };
inline constexpr int kOfferKindCount = 4;

struct OfferInfo {
    const char* title;
    const char* desc;
    Element element;
    std::uint32_t baseCost;
};

inline OfferInfo offerInfo(OfferKind k) {
    switch (k) {
        case OfferKind::BallFire:
            return {"Fire ball", "sets enemies it touches burning", Element::Fire, 18};
        case OfferKind::BallWind:
            return {"Wind ball", "fires bolts at the nearest enemy", Element::Wind, 45};
        case OfferKind::BallWater:
            return {"Water ball", "leaves a damaging trail", Element::Water, 24};
        case OfferKind::BallStone:
            return {"Stone ball", "drops rubble that blocks enemies", Element::Stone, 45};
    }
    return {"", "", Element::Plain, 0};
}

// Cost rises with how many balls you already field.
inline std::uint32_t offerCost(OfferKind k, int ballCount) {
    return offerInfo(k).baseCost + 10u * static_cast<std::uint32_t>(std::max(0, ballCount - 1));
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
        {"Reinforced core", "start with +70 core health", 4u, 1},
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
