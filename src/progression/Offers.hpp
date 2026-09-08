#pragma once

#include <cstdint>

namespace sb {

// ---- between-wave items: pick 1 of 4 rolled from this pool ------------------
//
// A run shows four random, eligible items after every wave (or you can skip the
// pick to repair the core to full instead). Ball-element items beyond "turn a
// ball to fire" are deliberately left out - those are meant to come from a boss.

enum class UpgradeKind {
    AddBall,        // +1 plain ball
    BallToFire,     // turn one plain ball into a fire ball (needs the Ignition node)
    CoreSpring,     // balls ricochet off the core faster
    CoreSlowField,  // a zone around the core slows enemies inside it
    FlingMomentum,  // a flung ball keeps its speed longer
    HeavyImpact,    // +contact damage, small, stacks to a cap
    BigBall,        // +ball radius, small, stacks to a cap
};
inline constexpr int kUpgradeKindCount = 7;
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
        case UpgradeKind::CoreSpring:    return "CoreSpring";
        case UpgradeKind::CoreSlowField: return "CoreSlowField";
        case UpgradeKind::FlingMomentum: return "FlingMomentum";
        case UpgradeKind::HeavyImpact:   return "HeavyImpact";
        case UpgradeKind::BigBall:       return "BigBall";
    }
    return "";
}

inline UpgradeInfo upgradeInfo(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddBall:       return {"Extra ball", "one more ball in the arena"};
        case UpgradeKind::BallToFire:    return {"Ignite a ball", "turns a plain ball into a fire ball"};
        case UpgradeKind::CoreSpring:    return {"Spring core", "your balls bounce off the core faster"};
        case UpgradeKind::CoreSlowField: return {"Slow field", "enemies near the core are slowed"};
        case UpgradeKind::FlingMomentum: return {"Reflexes", "a flung ball keeps its speed longer"};
        case UpgradeKind::HeavyImpact:   return {"Heavy impact", "+8% ball contact damage"};
        case UpgradeKind::BigBall:       return {"Big ball", "+10% ball radius"};
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
    int bigBallPicks = 0;
    int heavyImpactPicks = 0;
    bool spring = false;
    bool slowField = false;
    bool flingMomentum = false;
};

inline bool upgradeEligible(UpgradeKind k, const UpgradeCtx& c) {
    switch (k) {
        case UpgradeKind::AddBall:       return c.ballCount < c.maxBalls;
        case UpgradeKind::BallToFire:    return c.fireUnlocked && c.fireBalls < c.fireCap &&
                                                (c.ballCount - c.fireBalls) > 0;
        case UpgradeKind::BigBall:       return c.bigBallPicks < 3;
        case UpgradeKind::HeavyImpact:   return c.heavyImpactPicks < 3;
        case UpgradeKind::CoreSpring:    return !c.spring;
        case UpgradeKind::CoreSlowField: return !c.slowField;
        case UpgradeKind::FlingMomentum: return !c.flingMomentum;
    }
    return false;
}

// ---- permanent meta unlocks: the skill web in the game menu ----------------
//
// The menu draws these as a radial graph: a central node ("Squad", +1 ball)
// with branches that fan out - Base (left), Combat (bottom), Economy (top),
// Special balls (right) and Power-ups (upper-right). A node can only be bought
// once the node that gates it (`parent`) has at least one level. Most cost
// cores; the unlock nodes cost prisms (the win currency).
//
// v8 renumbered the enum (Aegis / Forge / Momentum / Prospector removed, a
// Power-ups branch added), so pre-v8 saves reset their unlock levels on load.

enum MetaUnlock {
    MetaStartBalls,   // Squad     - +1 starting ball                      (root)
    MetaCoreHp,       // Bulwark   - +core HP at the start
    MetaMend,         // Mend      - core heals more between waves
    MetaFireItem,     // Ignition  - unlocks the "ignite a ball" item
    MetaBounty,       // Fortune   - earn cores for every enemy killed
    MetaWindfall,     // Windfall  - chance a cleared run pays a 2nd prism
    MetaHeft,         // Heft      - balls start with +contact damage
    MetaMass,         // Mass      - balls start larger
    MetaUplink,       // Uplink    - power-ups appear more often
    MetaCapacitor,    // Capacitor - power-ups last longer
    MetaDamper,       // Damper    - unlocks the SLOW MOTION power-up
    MetaFacet,        // Facet     - unlocks the GOLDEN BOUNCE power-up
    MetaOverload,     // Overload  - unlocks the OVERDRIVE power-up
    MetaUnlockCount
};

enum class MetaBranch { Root, Base, Combat, Eco, Special, Pickups };
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
        /* Squad     */ {"Squad",     "start each run with one more ball",
                         8u,  2, MetaBranch::Root,    MetaCurrency::Cores,  -1,  0.00f,  0.00f},
        /* Bulwark   */ {"Bulwark",   "start with +20 core health",
                         10u, 3, MetaBranch::Base,    MetaCurrency::Cores,   0, -1.00f,  0.00f},
        /* Mend      */ {"Mend",      "the core heals +3 more between waves",
                         12u, 3, MetaBranch::Base,    MetaCurrency::Cores,   1, -2.00f,  0.00f},
        /* Ignition  */ {"Ignition",  "the ignite-a-ball item can appear",
                         2u,  1, MetaBranch::Special, MetaCurrency::Prisms,  0,  1.00f,  0.00f},
        /* Fortune   */ {"Fortune",   "earn cores for every enemy you kill",
                         6u,  3, MetaBranch::Eco,     MetaCurrency::Cores,   0,  0.00f, -1.00f},
        /* Windfall  */ {"Windfall",  "20% chance a cleared run pays a 2nd prism",
                         14u, 1, MetaBranch::Eco,     MetaCurrency::Cores,   4,  0.00f, -2.00f},
        /* Heft      */ {"Heft",      "balls start with +8% contact damage",
                         10u, 2, MetaBranch::Combat,  MetaCurrency::Cores,   0,  0.00f,  1.00f},
        /* Mass      */ {"Mass",      "balls start 10% larger",
                         12u, 2, MetaBranch::Combat,  MetaCurrency::Cores,   6,  0.00f,  2.00f},
        /* Uplink    */ {"Uplink",    "power-ups appear more often",
                         8u,  3, MetaBranch::Pickups, MetaCurrency::Cores,   0,  1.00f, -1.00f},
        /* Capacitor */ {"Capacitor", "power-ups last longer",
                         8u,  3, MetaBranch::Pickups, MetaCurrency::Cores,   8,  2.00f, -2.00f},
        /* Damper    */ {"Damper",    "unlocks the Slow Motion power-up",
                         2u,  1, MetaBranch::Pickups, MetaCurrency::Prisms,  8,  2.00f, -1.00f},
        /* Facet     */ {"Facet",     "unlocks the Golden Bounce power-up",
                         2u,  1, MetaBranch::Pickups, MetaCurrency::Prisms,  9,  3.00f, -3.00f},
        /* Overload  */ {"Overload",  "unlocks the Overdrive power-up",
                         3u,  1, MetaBranch::Pickups, MetaCurrency::Prisms,  9,  3.00f, -2.00f},
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
