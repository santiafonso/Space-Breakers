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

// ---- permanent meta unlocks: the skill web in the game menu ----------------
//
// The menu draws these as a radial graph: a central node ("Squad", +1 ball)
// with four branches that fan out - Base (left), Ball combat (bottom),
// Economy (top) and Special balls (right). A node can only be bought once the
// node that gates it (`parent`) has at least one level. Most cost cores; a few
// key nodes cost prisms (the miniboss currency).
//
// The first five values keep their old order and indices so v6 saves, which
// stored levels by index, still map cleanly.

enum MetaUnlock {
    MetaStartBalls,   // Squad     - +1 starting ball                (root)
    MetaCoreHp,       // Bulwark   - +core HP at the start
    MetaFireBall,     // Ignition  - unlocks the "ignite a ball" upgrade
    MetaFireCap,      // Forge     - +1 to how many balls can be fire in a run
    MetaPowerups,     // Fortune   - +1 power-up type that can drop
    MetaMend,         // Mend      - core heals more between waves
    MetaAegis,        // Aegis     - start every run with Second Chance
    MetaHeft,         // Heft      - balls start with +contact damage
    MetaMass,         // Mass      - balls start larger
    MetaMomentum,     // Momentum  - start every run with Reflexes (fling momentum)
    MetaProspector,   // Prospector- +% cores earned per run
    MetaWindfall,     // Windfall  - +prisms per boss kill
    MetaUnlockCount
};

enum class MetaBranch { Root, Base, Combat, Eco, Special };
enum class MetaCurrency { Cores, Prisms };

struct MetaUnlockDef {
    const char* name;
    const char* effect;
    std::uint32_t baseCost;
    int maxLevel;
    MetaBranch branch;
    MetaCurrency currency;
    int parent;     // node that gates this one; -1 for the root
    float gx, gy;   // layout offset from the centre, in grid cells (+x right, +y down)
};

inline const MetaUnlockDef& metaUnlockDef(int u) {
    static const MetaUnlockDef defs[MetaUnlockCount] = {
        /* Squad      */ {"Squad",      "start each run with one more ball",
                          8u,  3, MetaBranch::Root,    MetaCurrency::Cores,  -1,  0.00f,  0.00f},
        /* Bulwark    */ {"Bulwark",    "start with +40 core health",
                          10u, 3, MetaBranch::Base,    MetaCurrency::Cores,   0, -1.00f,  0.00f},
        /* Ignition   */ {"Ignition",   "the fire-ball upgrade can appear",
                          2u,  1, MetaBranch::Special, MetaCurrency::Prisms,  0,  1.00f,  0.00f},
        /* Forge      */ {"Forge",      "one more ball may be fire in a run",
                          10u, 3, MetaBranch::Special, MetaCurrency::Cores,   2,  2.00f,  0.00f},
        /* Fortune    */ {"Fortune",    "one more power-up type can drop",
                          6u,  4, MetaBranch::Eco,     MetaCurrency::Cores,   0,  0.00f, -1.00f},
        /* Mend       */ {"Mend",       "the core heals more between waves",
                          12u, 3, MetaBranch::Base,    MetaCurrency::Cores,   1, -2.00f,  0.00f},
        /* Aegis      */ {"Aegis",      "begin every run with Second Chance",
                          3u,  1, MetaBranch::Base,    MetaCurrency::Prisms,  1, -2.00f,  0.95f},
        /* Heft       */ {"Heft",       "balls start with +30% contact damage",
                          10u, 3, MetaBranch::Combat,  MetaCurrency::Cores,   0,  0.00f,  1.00f},
        /* Mass       */ {"Mass",       "balls start 25% larger",
                          12u, 2, MetaBranch::Combat,  MetaCurrency::Cores,   7,  0.00f,  2.00f},
        /* Momentum   */ {"Momentum",   "begin every run with Reflexes",
                          3u,  1, MetaBranch::Combat,  MetaCurrency::Prisms,  7, -0.95f,  2.00f},
        /* Prospector */ {"Prospector", "+15% cores earned per run, stacks",
                          12u, 3, MetaBranch::Eco,     MetaCurrency::Cores,   4,  0.00f, -2.00f},
        /* Windfall   */ {"Windfall",   "+1 prism each time you beat a boss",
                          18u, 2, MetaBranch::Eco,     MetaCurrency::Cores,   4,  0.95f, -2.00f},
    };
    return defs[u];
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

}  // namespace sb
