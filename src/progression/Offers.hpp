#pragma once

#include <cstdint>

namespace sb {

// ---- between-wave items: pick 1 of 4 rolled from this pool ------------------
//
// A run shows four random, eligible items after every wave (or you can skip the
// pick to repair the core to full instead).
//
// The elemental balls are just "add a ball of element X" items: they drop a new
// ball of that element straight into the arena. An element item only shows once
// its web node is unlocked; the unlock chain runs fire -> poison -> water ->
// ice -> stone -> electric.

enum class UpgradeKind {
    AddBall,           // +1 plain ball
    AddFireBall,       // +1 fire ball     (needs the Ignition node)
    AddPoisonBall,     // +1 poison ball   (needs the Venom node)
    AddWaterBall,      // +1 water ball    (needs the Tide node)
    AddIceBall,        // +1 ice ball      (needs the Frost node)
    AddStoneBall,      // +1 stone ball    (needs the Quarry node)
    AddElectricBall,   // +1 electric ball (needs the Arc node)
    CoreSpring,        // balls ricochet off the core faster
    CoreSlowField,     // a zone around the core slows enemies inside it
    FlingMomentum,     // a flung ball keeps its speed longer
    WallRush,          // a ball speeds up on every wall bounce
    Carom,             // a ball speeds up when it clacks another ball
    StrongArm,         // you fling balls harder
    Ricochet,          // brief damage bonus right after a wall bounce
    CeilingBreak,      // raises the ball top-speed ceiling
    WarmUp,            // cruise speed ramps up over the wave
    HeavyKnock,        // more knockback to enemies
    Conductor,         // electric arc jumps to a 2nd enemy   (needs the Arc node)
    Shatter,           // bonus damage vs frozen enemies      (needs the Frost node)
    Contagion,         // a poisoned enemy dying re-poisons nearby (needs the Venom node)
    Bedrock,           // stone rubble lasts far longer        (needs the Quarry node)
    Primed,            // +damage vs enemies already under an element effect (needs any element node)
    Spearhead,         // the ball you flung most recently has a higher cruise speed
    HeavyImpact,       // +contact damage, small, stacks to a cap
    BigBall,           // +ball radius, small, stacks to a cap
    // "Ball combat" items - synergise implicitly with the speed / damage picks.
    Cleave,            // the ball punches through an enemy it kills
    Crit,              // chance of a double-damage contact hit
    Bruiser,           // contact damage scales with ball speed
    Executioner,       // huge bonus damage to badly hurt enemies
    Overkill,          // a kill's leftover damage splashes onto the nearest enemy
    Tempo,             // the ball snaps back to cruise speed faster after a hit
};
inline constexpr int kUpgradeKindCount = 31;
inline constexpr int kChoiceCount = 4;

// "Add X ball" kinds map onto element web-node slots 0..5 (fire..electric);
// everything else returns -1. Element enum is Plain,Fire,Poison,Water,Ice,Stone,
// Electric, so the ball element is Element(elementItemSlot + 1).
inline constexpr int kElementItemCount = 6;
inline int elementItemSlot(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddFireBall:     return 0;
        case UpgradeKind::AddPoisonBall:   return 1;
        case UpgradeKind::AddWaterBall:    return 2;
        case UpgradeKind::AddIceBall:      return 3;
        case UpgradeKind::AddStoneBall:    return 4;
        case UpgradeKind::AddElectricBall: return 5;
        default:                           return -1;
    }
}

struct UpgradeInfo {
    const char* title;
    const char* desc;
};

// Machine-readable name, for the SB_UPGRADES dev override.
inline const char* upgradeKindId(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddBall:          return "AddBall";
        case UpgradeKind::AddFireBall:      return "AddFireBall";
        case UpgradeKind::AddPoisonBall:    return "AddPoisonBall";
        case UpgradeKind::AddWaterBall:     return "AddWaterBall";
        case UpgradeKind::AddIceBall:       return "AddIceBall";
        case UpgradeKind::AddStoneBall:     return "AddStoneBall";
        case UpgradeKind::AddElectricBall:  return "AddElectricBall";
        case UpgradeKind::CoreSpring:       return "CoreSpring";
        case UpgradeKind::CoreSlowField:    return "CoreSlowField";
        case UpgradeKind::FlingMomentum:    return "FlingMomentum";
        case UpgradeKind::WallRush:         return "WallRush";
        case UpgradeKind::Carom:            return "Carom";
        case UpgradeKind::StrongArm:        return "StrongArm";
        case UpgradeKind::Ricochet:        return "Ricochet";
        case UpgradeKind::CeilingBreak:    return "CeilingBreak";
        case UpgradeKind::WarmUp:          return "WarmUp";
        case UpgradeKind::HeavyKnock:      return "HeavyKnock";
        case UpgradeKind::Conductor:       return "Conductor";
        case UpgradeKind::Shatter:         return "Shatter";
        case UpgradeKind::Contagion:       return "Contagion";
        case UpgradeKind::Bedrock:         return "Bedrock";
        case UpgradeKind::Primed:          return "Primed";
        case UpgradeKind::Spearhead:      return "Spearhead";
        case UpgradeKind::HeavyImpact:      return "HeavyImpact";
        case UpgradeKind::BigBall:          return "BigBall";
        case UpgradeKind::Cleave:           return "Cleave";
        case UpgradeKind::Crit:             return "Crit";
        case UpgradeKind::Bruiser:          return "Bruiser";
        case UpgradeKind::Executioner:      return "Executioner";
        case UpgradeKind::Overkill:         return "Overkill";
        case UpgradeKind::Tempo:            return "Tempo";
    }
    return "";
}

inline UpgradeInfo upgradeInfo(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::AddBall:          return {"Extra ball", "one more ball in the arena"};
        case UpgradeKind::AddFireBall:      return {"Fire ball", "+1 ball that hits harder on contact"};
        case UpgradeKind::AddPoisonBall:    return {"Poison ball", "+1 ball; hits stack damage over time"};
        case UpgradeKind::AddWaterBall:     return {"Water ball", "+1 ball trailing a damaging wake"};
        case UpgradeKind::AddIceBall:       return {"Ice ball", "+1 ball that freezes enemies on hit"};
        case UpgradeKind::AddStoneBall:     return {"Stone ball", "+1 ball that drops grinding rubble"};
        case UpgradeKind::AddElectricBall:  return {"Electric ball", "+1 ball that zaps nearby enemies"};
        case UpgradeKind::CoreSpring:       return {"Spring core", "your balls bounce off the core faster"};
        case UpgradeKind::CoreSlowField:    return {"Slow field", "enemies near the core are slowed"};
        case UpgradeKind::FlingMomentum:    return {"Reflexes", "a flung ball keeps its speed longer"};
        case UpgradeKind::WallRush:         return {"Wall rush", "a ball speeds up every time it hits a wall"};
        case UpgradeKind::Carom:            return {"Carom", "a ball speeds up when it clacks another ball"};
        case UpgradeKind::StrongArm:        return {"Strong arm", "you fling balls noticeably harder"};
        case UpgradeKind::Ricochet:        return {"Ricochet", "a ball hits harder for a moment after a wall bounce"};
        case UpgradeKind::CeilingBreak:    return {"Ceiling break", "raises the ball's top-speed limit"};
        case UpgradeKind::WarmUp:          return {"Warm-up", "ball cruise speed climbs as a wave goes on"};
        case UpgradeKind::HeavyKnock:      return {"Heavy knock", "balls shove enemies back much harder"};
        case UpgradeKind::Conductor:       return {"Conductor", "the electric arc jumps on to a second enemy"};
        case UpgradeKind::Shatter:         return {"Shatter", "hitting a frozen enemy deals bonus damage"};
        case UpgradeKind::Contagion:       return {"Contagion", "an enemy that dies poisoned poisons those near it"};
        case UpgradeKind::Bedrock:         return {"Bedrock", "stone rubble lasts much longer"};
        case UpgradeKind::Primed:          return {"Primed", "+damage to enemies already burning, poisoned or frozen"};
        case UpgradeKind::Spearhead:      return {"Spearhead", "the ball you flung most recently cruises faster"};
        case UpgradeKind::HeavyImpact:      return {"Heavy impact", "+8% ball contact damage"};
        case UpgradeKind::BigBall:          return {"Big ball", "+10% ball radius"};
        case UpgradeKind::Cleave:           return {"Cleave", "the ball punches through an enemy it kills"};
        case UpgradeKind::Crit:             return {"Keen eye", "one contact hit in seven deals double damage"};
        case UpgradeKind::Bruiser:          return {"Battering", "the faster the ball moves, the harder it hits"};
        case UpgradeKind::Executioner:      return {"Executioner", "big bonus damage to badly hurt enemies"};
        case UpgradeKind::Overkill:         return {"Overkill", "leftover damage from a kill splashes onto the next enemy"};
        case UpgradeKind::Tempo:            return {"Tempo", "the ball snaps back to cruise speed faster after a hit"};
    }
    return {"", ""};
}

// What the roll needs to know to drop picks that would do nothing.
struct UpgradeCtx {
    int ballCount = 1;
    int maxBalls = 8;
    bool elemUnlocked[kElementItemCount] = {};   // element web node has a level
    int bigBallPicks = 0;
    int heavyImpactPicks = 0;
    bool spring = false;
    bool slowField = false;
    bool flingMomentum = false;
    bool wallRush = false;
    bool carom = false;
    bool strongArm = false;
    bool ricochet = false;
    bool ceilingBreak = false;
    bool warmUp = false;
    bool heavyKnock = false;
    bool conductor = false;
    bool shatter = false;
    bool contagion = false;
    bool bedrock = false;
    bool primed = false;
    bool spearhead = false;
    bool crit = false;
    bool bruiser = false;
    bool executioner = false;
    bool overkill = false;
    bool cleave = false;
    bool tempo = false;
};

// True if any elemental ball type is unlocked (for the "Primed" item).
inline bool anyElementUnlocked(const UpgradeCtx& c) {
    for (int i = 0; i < kElementItemCount; ++i)
        if (c.elemUnlocked[i]) return true;
    return false;
}

inline bool upgradeEligible(UpgradeKind k, const UpgradeCtx& c) {
    if (const int s = elementItemSlot(k); s >= 0)
        return c.ballCount < c.maxBalls && c.elemUnlocked[s];
    switch (k) {
        case UpgradeKind::AddBall:       return c.ballCount < c.maxBalls;
        case UpgradeKind::BigBall:       return c.bigBallPicks < 3;
        case UpgradeKind::HeavyImpact:   return c.heavyImpactPicks < 3;
        case UpgradeKind::CoreSpring:    return !c.spring;
        case UpgradeKind::CoreSlowField: return !c.slowField;
        case UpgradeKind::FlingMomentum: return !c.flingMomentum;
        case UpgradeKind::WallRush:      return !c.wallRush;
        case UpgradeKind::Carom:         return !c.carom;
        case UpgradeKind::StrongArm:     return !c.strongArm;
        case UpgradeKind::Ricochet:      return !c.ricochet;
        case UpgradeKind::CeilingBreak:  return !c.ceilingBreak;
        case UpgradeKind::WarmUp:        return !c.warmUp;
        case UpgradeKind::HeavyKnock:    return !c.heavyKnock;
        case UpgradeKind::Conductor:     return !c.conductor && c.elemUnlocked[5];   // electric
        case UpgradeKind::Shatter:       return !c.shatter   && c.elemUnlocked[3];   // ice
        case UpgradeKind::Contagion:     return !c.contagion && c.elemUnlocked[1];   // poison
        case UpgradeKind::Bedrock:       return !c.bedrock   && c.elemUnlocked[4];   // stone
        case UpgradeKind::Primed:        return !c.primed    && anyElementUnlocked(c);
        case UpgradeKind::Spearhead:     return !c.spearhead;
        case UpgradeKind::Cleave:        return !c.cleave;
        case UpgradeKind::Crit:          return !c.crit;
        case UpgradeKind::Bruiser:       return !c.bruiser;
        case UpgradeKind::Executioner:   return !c.executioner;
        case UpgradeKind::Overkill:      return !c.overkill;
        case UpgradeKind::Tempo:         return !c.tempo;
        default:                         return false;
    }
}

// ---- permanent meta unlocks: the skill web in the game menu ----------------
//
// The menu draws these as a radial graph: a central node ("Squad", +1 ball)
// with branches that fan out - Base (left), Combat (bottom), Economy (top),
// Power-ups (upper-right) and Special balls (a chain fanning to the lower
// right). A node can only be bought once the node that gates it (`parent`) has
// at least one level. Most cost cores; the prism nodes cost prisms (the win
// currency).
//
// v9 inserted the five extra Special-ball nodes, shifting every index after
// Ignition, so pre-v9 saves reset their unlock levels on load. v11 appends the
// Fase-A nodes (Aegis..Ember) at the end, so indices stay put and older saves
// just load with the new nodes unbought.

enum MetaUnlock {
    MetaStartBalls,   // Squad     - +1 starting ball                      (root)
    MetaCoreHp,       // Bulwark   - +core HP at the start
    MetaMend,         // Mend      - core heals more between waves
    MetaFireItem,     // Ignition  - unlocks the fire ball item, +fire potency
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
        // Special-ball chain: it fans down-right. Each node's dominant axis is an
        // integer so it lands exactly on a background ring (ring = max(|gx|,|gy|)).
        /* Ignition  */ {"Ignition",  "the fire ball item can appear; higher levels hit harder",
                         2u,  3, MetaBranch::Special, MetaCurrency::Prisms,  0,  1.00f,  0.00f},
        /* Venom     */ {"Venom",     "the poison ball item can appear; higher levels stack faster",
                         2u,  3, MetaBranch::Special, MetaCurrency::Prisms,  3,  2.00f,  0.80f},
        /* Tide      */ {"Tide",      "the water ball item can appear; higher levels leave a wider wake",
                         3u,  3, MetaBranch::Special, MetaCurrency::Prisms,  4,  3.00f,  1.70f},
        /* Frost     */ {"Frost",     "the ice ball item can appear; higher levels freeze for longer",
                         3u,  3, MetaBranch::Special, MetaCurrency::Prisms,  5,  4.00f,  2.60f},
        /* Quarry    */ {"Quarry",    "the stone ball item can appear; higher levels grind harder",
                         4u,  3, MetaBranch::Special, MetaCurrency::Prisms,  6,  4.00f,  4.00f},
        /* Arc       */ {"Arc",       "the electric ball item can appear; higher levels zap harder",
                         4u,  3, MetaBranch::Special, MetaCurrency::Prisms,  7,  5.00f,  4.60f},
        /* Fortune   */ {"Fortune",   "earn cores for every enemy you kill",
                         6u,  3, MetaBranch::Eco,     MetaCurrency::Cores,   0,  0.00f, -1.00f},
        /* Windfall  */ {"Windfall",  "20% chance a cleared run pays a 2nd prism",
                         14u, 1, MetaBranch::Eco,     MetaCurrency::Cores,   9,  0.00f, -2.00f},
        /* Heft      */ {"Heft",      "balls start with +8% contact damage",
                         10u, 2, MetaBranch::Combat,  MetaCurrency::Cores,   0,  0.00f,  1.00f},
        /* Mass      */ {"Mass",      "balls start 10% larger",
                         12u, 2, MetaBranch::Combat,  MetaCurrency::Cores,  11,  0.00f,  2.00f},
        /* Uplink    */ {"Uplink",    "power-ups appear more often",
                         8u,  3, MetaBranch::Pickups, MetaCurrency::Cores,   0,  1.00f, -1.00f},
        /* Capacitor */ {"Capacitor", "power-ups last longer",
                         8u,  3, MetaBranch::Pickups, MetaCurrency::Cores,  13,  2.00f, -2.00f},
        /* Damper    */ {"Damper",    "unlocks the Slow Motion power-up",
                         2u,  1, MetaBranch::Pickups, MetaCurrency::Prisms, 13,  2.00f, -1.00f},
        /* Facet     */ {"Facet",     "unlocks the Golden Bounce power-up",
                         2u,  1, MetaBranch::Pickups, MetaCurrency::Prisms, 14,  3.00f, -3.00f},
        /* Overload  */ {"Overload",  "unlocks the Overdrive power-up",
                         3u,  1, MetaBranch::Pickups, MetaCurrency::Prisms, 14,  3.00f, -2.00f},
        /* Ledger    */ {"Ledger",    "the DOUBLE POINTS power-up can appear",
                         6u,  1, MetaBranch::Pickups, MetaCurrency::Cores,  13,  1.00f, -2.00f},
        /* Kinetics  */ {"Kinetics",  "the SPEED SURGE power-up can appear",
                         6u,  1, MetaBranch::Pickups, MetaCurrency::Cores,  18,  1.00f, -3.00f},
        /* Foresight */ {"Foresight", "start each run with reroll charges to swap an offered item",
                         10u, 3, MetaBranch::Eco,     MetaCurrency::Cores,   0, -1.00f, -1.00f},
        // ---- v11 Fase-A nodes. Appended so indices 0..20 stay put. ----
        /* Aegis     */ {"Aegis",     "the core shrugs off the first hit of each wave (+1 hit per level)",
                         12u, 2, MetaBranch::Base,    MetaCurrency::Cores,   1, -1.00f,  1.00f},
        /* Regen     */ {"Regen",     "the core slowly regenerates during a wave, not only between them",
                         12u, 3, MetaBranch::Base,    MetaCurrency::Cores,   2, -2.00f,  1.00f},
        /* Bastion   */ {"Bastion",   "the core's max health grows a little with every wave cleared",
                         14u, 2, MetaBranch::Base,    MetaCurrency::Cores,   2, -3.00f,  0.00f},
        /* Salvage   */ {"Salvage",   "enemies drop cores more often",
                         10u, 3, MetaBranch::Eco,     MetaCurrency::Cores,   9, -1.00f, -2.00f},
        /* Interest  */ {"Interest",  "clearing a wave with no core damage pays a core bonus",
                         12u, 3, MetaBranch::Eco,     MetaCurrency::Cores,  10,  0.00f, -3.00f},
        /* Prospector*/ {"Prospector","skipping a pick to repair the core refunds a reroll charge",
                         12u, 2, MetaBranch::Eco,     MetaCurrency::Cores,  20, -2.00f, -2.00f},
        /* Stockpile */ {"Stockpile", "keep one random power-up in reserve; press Q to use it",
                         6u,  1, MetaBranch::Pickups, MetaCurrency::Cores,  13,  2.00f,  0.00f},
        /* Magnet    */ {"Magnet",    "power-up orbs drift toward your nearest ball",
                         10u, 1, MetaBranch::Pickups, MetaCurrency::Cores,  14,  4.00f, -1.00f},
        /* Afterglow */ {"Afterglow", "when a power-up ends its effect fades out instead of cutting",
                         10u, 2, MetaBranch::Pickups, MetaCurrency::Cores,  14,  3.00f, -1.00f},
        /* Charged   */ {"Charged",   "power-ups arrive with part of their duration already charged",
                         10u, 2, MetaBranch::Pickups, MetaCurrency::Cores,  14,  4.00f, -2.00f},
        /* Ember     */ {"Ember",     "fire ball hits set enemies alight for a burn; scales with Ignition",
                         2u,  3, MetaBranch::Special, MetaCurrency::Prisms,  3,  1.00f,  1.00f},
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
