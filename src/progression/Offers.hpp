#pragma once

#include <cstdint>

namespace sb {

// ---- between-wave upgrades: pick 1 of 4 rolled from this pool ----------------
//
// A run shows four random, eligible upgrades after every wave. Ball-element
// upgrades beyond "turn a ball to fire" are deliberately left out - those are
// meant to come from a boss later.

enum class UpgradeKind {
    AddBall,        // +1 plain ball
    BallToFire,     // turn one plain ball into a fire ball (needs the meta unlock)
    CoreArmor,      // +max core HP, stacks
    CoreRepair,     // heal the core to full, now
    CoreSpring,     // balls ricochet off the core faster
    CoreRetaliate,  // an enemy hitting the core triggers a damaging pulse
    FlingMomentum,  // a flung ball keeps its speed longer
    HeavyImpact,    // +contact damage, stacks
    BigBall,        // +ball radius, stacks (capped)
    Loot,           // +cores at the end of the run
    SecondChance,   // once per run the core survives a lethal hit
};
inline constexpr int kUpgradeKindCount = 11;
inline constexpr int kChoiceCount = 4;

struct UpgradeInfo {
    const char* title;
    const char* desc;
};

// Machine-readable name, for the SB_UPGRADES dev override.
inline const char* upgradeKindId(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddBall:       return "AddBall";
        case UpgradeKind::BallToFire:    return "BallToFire";
        case UpgradeKind::CoreArmor:     return "CoreArmor";
        case UpgradeKind::CoreRepair:    return "CoreRepair";
        case UpgradeKind::CoreSpring:    return "CoreSpring";
        case UpgradeKind::CoreRetaliate: return "CoreRetaliate";
        case UpgradeKind::FlingMomentum: return "FlingMomentum";
        case UpgradeKind::HeavyImpact:   return "HeavyImpact";
        case UpgradeKind::BigBall:       return "BigBall";
        case UpgradeKind::Loot:          return "Loot";
        case UpgradeKind::SecondChance:  return "SecondChance";
    }
    return "";
}

inline UpgradeInfo upgradeInfo(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddBall:       return {"Extra ball", "one more ball in the arena"};
        case UpgradeKind::BallToFire:    return {"Ignite a ball", "turns a plain ball into a fire ball"};
        case UpgradeKind::CoreArmor:     return {"Reinforce core", "+25 max core health, and heal it"};
        case UpgradeKind::CoreRepair:    return {"Repair core", "heal the core back to full"};
        case UpgradeKind::CoreSpring:    return {"Spring core", "your balls bounce off the core faster"};
        case UpgradeKind::CoreRetaliate: return {"Retaliate", "a hit on the core blasts nearby enemies"};
        case UpgradeKind::FlingMomentum: return {"Reflexes", "a flung ball keeps its speed longer"};
        case UpgradeKind::HeavyImpact:   return {"Heavy impact", "+30% ball contact damage"};
        case UpgradeKind::BigBall:       return {"Big ball", "+25% ball radius"};
        case UpgradeKind::Loot:          return {"Loot", "+20% cores at the end of the run"};
        case UpgradeKind::SecondChance:  return {"Second chance", "once, the core survives a lethal hit"};
    }
    return {"", ""};
}

// What the roll needs to know to drop picks that would do nothing.
struct UpgradeCtx {
    int ballCount = 1;
    int maxBalls = 8;
    int fireBalls = 0;
    int fireCap = 1;
    bool fireUnlocked = false;
    bool coreFull = true;
    int bigBallPicks = 0;
    bool spring = false;
    bool retaliate = false;
    bool flingMomentum = false;
    bool loot = false;
    bool secondChance = false;
};

inline bool upgradeEligible(UpgradeKind k, const UpgradeCtx& c) {
    switch (k) {
        case UpgradeKind::AddBall:       return c.ballCount < c.maxBalls;
        case UpgradeKind::BallToFire:    return c.fireUnlocked && c.fireBalls < c.fireCap &&
                                                (c.ballCount - c.fireBalls) > 0;
        case UpgradeKind::CoreRepair:    return !c.coreFull;
        case UpgradeKind::BigBall:       return c.bigBallPicks < 3;
        case UpgradeKind::CoreSpring:    return !c.spring;
        case UpgradeKind::CoreRetaliate: return !c.retaliate;
        case UpgradeKind::FlingMomentum: return !c.flingMomentum;
        case UpgradeKind::Loot:          return !c.loot;
        case UpgradeKind::SecondChance:  return !c.secondChance;
        case UpgradeKind::CoreArmor:     return true;
        case UpgradeKind::HeavyImpact:   return true;
    }
    return false;
}

// ---- permanent meta unlocks (spent with cores, in the game menu) -----------

enum MetaUnlock {
    MetaStartBalls,   // +1 starting ball
    MetaCoreHp,       // +core HP at the start
    MetaFireBall,     // unlocks the "ignite a ball" between-wave upgrade
    MetaFireCap,      // +1 to how many balls can be fire in one run
    MetaPowerups,     // +1 power-up type that can drop
    MetaUnlockCount
};

struct MetaUnlockDef {
    const char* name;
    const char* effect;
    std::uint32_t baseCost;
    int maxLevel;
};

inline const MetaUnlockDef& metaUnlockDef(int u) {
    static const MetaUnlockDef defs[MetaUnlockCount] = {
        {"Squad",    "start each run with one more ball",      8u,  3},
        {"Bulwark",  "start with +40 core health",             10u, 3},
        {"Ignition", "the fire-ball upgrade can appear",       12u, 1},
        {"Forge",    "one more ball may be fire in a run",      10u, 3},
        {"Fortune",  "one more power-up type can drop",         6u,  4},
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
