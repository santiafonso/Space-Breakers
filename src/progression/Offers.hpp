#pragma once

#include <cstdint>
#include <vector>

#include "sim/Entities.hpp"

namespace sb {

// ---- between-wave picks ------------------------------------------------------
//
// Every ball is a small "character": up to four items, any number of stacked
// modifiers, and a role that EMERGES from its items. Every item carries a tag
// (Striker / Guardian / Support): 2 items of one tag give the ball that role,
// 4 give its mastery. A pick is one of five kinds:
//  - New ball: one more ball (up to cfg::ball::maxBalls).
//  - Element:  an item that makes the ball fire / poison / ... while equipped.
//              One element per ball. Gated by that element's web node. Two
//              DIFFERENT balls landing different elements on one enemy set off
//              a reaction (see World).
//  - Item:     a unique effect for one ball; takes one of its 4 slots.
//  - Modifier: a stat bump for one ball; no slot, stacks without limit.
//  - Relic:    a whole-run passive.

enum class UpgradeKind {
    // new ball
    AddBall,
    // elements (order = element web-node slots 0..5)
    ElemFire,
    ElemPoison,
    ElemWater,
    ElemIce,
    ElemStone,
    ElemElectric,
    // items (take a slot)
    WallRush,          // speeds up on every wall bounce                 [Striker]
    Carom,             // speeds up when it clacks another ball          [Guardian]
    Ricochet,          // brief damage bonus right after a wall bounce   [Striker]
    WarmUp,            // cruise speed climbs over the wave              [Guardian]
    Cleave,            // punches through an enemy it kills              [Striker]
    Crit,              // "Keen eye": chance of a double-damage hit      [Striker]
    Bruiser,           // "Battering": damage scales with speed          [Striker]
    Executioner,       // big bonus vs badly hurt enemies                [Striker]
    Overkill,          // a kill's leftover damage splashes              [Support]
    Tempo,             // snaps back to cruise faster after a hit        [Guardian]
    Shatter,           // bonus damage vs frozen enemies (needs Frost)   [Support]
    Conductor,         // its electric arc jumps to a 2nd enemy          [Striker]
    Bedrock,           // its stone rubble lasts far longer              [Guardian]
    Echo,              // chance a hit strikes twice                     [Striker]
    Tesla,             // chance a hit zaps 3 nearby enemies             [Support]
    Bomber,            // chance a kill explodes                         [Support]
    SplitShot,         // chance a wall bounce spawns a ghost copy       [Support]
    Rampart,           // hits shove much further and stagger longer     [Guardian]
    Mender,            // each core bounce repairs the core a little     [Guardian]
    // modifiers (no slot, stack)
    HeavyImpact,       // +contact damage
    BigBall,           // +radius
    Swift,             // +cruise speed
    CeilingBreak,      // +top speed
    HeavyKnock,        // +knockback
    FlingMomentum,     // "Reflexes": keeps a fling's speed longer
    // relics
    CoreSpring,        // balls ricochet off the core faster
    CoreSlowField,     // a zone around the core slows enemies inside it
    StrongArm,         // you fling every ball harder
    Contagion,         // a poisoned enemy dying poisons its neighbours (needs Venom)
    Primed,            // +damage vs enemies under any element effect (needs any element node)
    Catalyst,          // reactions hit twice as hard and wider (needs 2 element nodes)
    ChainReaction,     // a reaction may echo onto another afflicted enemy (needs 2 element nodes)
    LuckyClover,       // every chance x1.6
    GlassCannon,       // all damage x1.6, core max HP -30%
    MagneticCore,      // balls leave the core aimed at the nearest enemy
};
inline constexpr int kUpgradeKindCount = 42;
static_assert(static_cast<int>(UpgradeKind::MagneticCore) + 1 == kUpgradeKindCount, "update kUpgradeKindCount");
inline constexpr int kChoiceCount = 4;
inline constexpr int kBallSlots = 4;          // item slots per ball
inline constexpr int kElementItemCount = 6;
inline constexpr int kModifierCount = 6;      // HeavyImpact..FlingMomentum

enum class UpgradeCat { NewBall, Element, Item, Modifier, Relic };

inline UpgradeCat upgradeCat(UpgradeKind k) {
    const int i = static_cast<int>(k);
    if (i <= static_cast<int>(UpgradeKind::AddBall)) return UpgradeCat::NewBall;
    if (i <= static_cast<int>(UpgradeKind::ElemElectric)) return UpgradeCat::Element;
    if (i <= static_cast<int>(UpgradeKind::Mender)) return UpgradeCat::Item;
    if (i <= static_cast<int>(UpgradeKind::FlingMomentum)) return UpgradeCat::Modifier;
    return UpgradeCat::Relic;
}

inline const char* upgradeCatName(UpgradeCat c) {
    switch (c) {
        case UpgradeCat::NewBall:  return "NEW BALL";
        case UpgradeCat::Element:  return "ELEMENT";
        case UpgradeCat::Item:     return "ITEM";
        case UpgradeCat::Modifier: return "MODIFIER";
        case UpgradeCat::Relic:    return "RELIC";
    }
    return "";
}

inline const char* upgradeCatDesc(UpgradeCat c) {
    switch (c) {
        case UpgradeCat::NewBall:  return "adds one more ball to the arena";
        case UpgradeCat::Element:  return "an item: one element per ball, takes one of its 4 slots. Two balls with different elements hitting the same enemy set off a reaction.";
        case UpgradeCat::Item:     return "a unique effect for one ball; takes one of its 4 slots. Its tag counts toward the ball's role.";
        case UpgradeCat::Modifier: return "a stat bump for one ball; no slot, stacks without limit";
        case UpgradeCat::Relic:    return "a passive for the whole run";
    }
    return "";
}

// ---- item tags: a ball's role comes from them ------------------------------
enum class ItemTag { None, Striker, Guardian, Support };

inline ItemTag itemTag(UpgradeKind k) {
    switch (k) {
        case UpgradeKind::ElemFire: case UpgradeKind::ElemElectric:
        case UpgradeKind::WallRush: case UpgradeKind::Ricochet: case UpgradeKind::Cleave:
        case UpgradeKind::Crit: case UpgradeKind::Bruiser: case UpgradeKind::Executioner:
        case UpgradeKind::Conductor: case UpgradeKind::Echo:
            return ItemTag::Striker;
        case UpgradeKind::ElemIce: case UpgradeKind::ElemStone:
        case UpgradeKind::Carom: case UpgradeKind::WarmUp: case UpgradeKind::Tempo:
        case UpgradeKind::Bedrock: case UpgradeKind::Rampart: case UpgradeKind::Mender:
            return ItemTag::Guardian;
        case UpgradeKind::ElemPoison: case UpgradeKind::ElemWater:
        case UpgradeKind::Overkill: case UpgradeKind::Shatter: case UpgradeKind::Tesla:
        case UpgradeKind::Bomber: case UpgradeKind::SplitShot:
            return ItemTag::Support;
        default:
            return ItemTag::None;
    }
}

inline const char* itemTagName(ItemTag t) {
    switch (t) {
        case ItemTag::Striker:  return "Striker";
        case ItemTag::Guardian: return "Guardian";
        case ItemTag::Support:  return "Support";
        case ItemTag::None:     return "";
    }
    return "";
}

inline BallRole tagRole(ItemTag t) {
    switch (t) {
        case ItemTag::Striker:  return BallRole::Striker;
        case ItemTag::Guardian: return BallRole::Guardian;
        case ItemTag::Support:  return BallRole::Support;
        case ItemTag::None:     return BallRole::Normal;
    }
    return BallRole::Normal;
}

// Element items map onto element web-node slots 0..5 (fire..electric);
// everything else returns -1. The ball element is Element(slot + 1).
inline int elementItemSlot(UpgradeKind k) {
    if (upgradeCat(k) != UpgradeCat::Element) return -1;
    return static_cast<int>(k) - static_cast<int>(UpgradeKind::ElemFire);
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
        case UpgradeKind::WallRush:       return "WallRush";
        case UpgradeKind::Carom:          return "Carom";
        case UpgradeKind::Ricochet:       return "Ricochet";
        case UpgradeKind::WarmUp:         return "WarmUp";
        case UpgradeKind::Cleave:         return "Cleave";
        case UpgradeKind::Crit:           return "Crit";
        case UpgradeKind::Bruiser:        return "Bruiser";
        case UpgradeKind::Executioner:    return "Executioner";
        case UpgradeKind::Overkill:       return "Overkill";
        case UpgradeKind::Tempo:          return "Tempo";
        case UpgradeKind::Shatter:        return "Shatter";
        case UpgradeKind::Conductor:      return "Conductor";
        case UpgradeKind::Bedrock:        return "Bedrock";
        case UpgradeKind::Echo:           return "Echo";
        case UpgradeKind::Tesla:          return "Tesla";
        case UpgradeKind::Bomber:         return "Bomber";
        case UpgradeKind::SplitShot:      return "SplitShot";
        case UpgradeKind::Rampart:        return "Rampart";
        case UpgradeKind::Mender:         return "Mender";
        case UpgradeKind::HeavyImpact:    return "HeavyImpact";
        case UpgradeKind::BigBall:        return "BigBall";
        case UpgradeKind::Swift:          return "Swift";
        case UpgradeKind::CeilingBreak:   return "CeilingBreak";
        case UpgradeKind::HeavyKnock:     return "HeavyKnock";
        case UpgradeKind::FlingMomentum:  return "FlingMomentum";
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
    }
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
        case UpgradeKind::WallRush:      return {"Wall rush", "speeds up every time it hits a wall"};
        case UpgradeKind::Carom:         return {"Carom", "speeds up when it clacks another ball"};
        case UpgradeKind::Ricochet:      return {"Ricochet", "hits harder for a moment after a wall bounce"};
        case UpgradeKind::WarmUp:        return {"Warm-up", "its cruise speed climbs as a wave goes on"};
        case UpgradeKind::Cleave:        return {"Cleave", "punches straight through an enemy it kills"};
        case UpgradeKind::Crit:          return {"Keen eye", "15% chance a hit deals double damage"};
        case UpgradeKind::Bruiser:       return {"Battering", "the faster it moves, the harder it hits"};
        case UpgradeKind::Executioner:   return {"Executioner", "big bonus damage to badly hurt enemies"};
        case UpgradeKind::Overkill:      return {"Overkill", "leftover damage from a kill splashes onto the next enemy"};
        case UpgradeKind::Tempo:         return {"Tempo", "snaps back to cruise speed faster after a hit"};
        case UpgradeKind::Shatter:       return {"Shatter", "hitting a frozen enemy deals bonus damage"};
        case UpgradeKind::Conductor:     return {"Conductor", "its electric arc jumps on to a second enemy"};
        case UpgradeKind::Bedrock:       return {"Bedrock", "its stone rubble lasts much longer"};
        case UpgradeKind::Echo:          return {"Echo", "25% chance a hit strikes twice (effects and all)"};
        case UpgradeKind::Tesla:         return {"Tesla", "20% chance a hit zaps up to 3 enemies nearby"};
        case UpgradeKind::Bomber:        return {"Bomber", "30% chance an enemy it kills explodes"};
        case UpgradeKind::SplitShot:     return {"Split shot", "15% chance a wall bounce spawns a ghost copy with the same items (lasts a few seconds)"};
        case UpgradeKind::Rampart:       return {"Rampart", "its hits shove enemies much further and stagger them longer"};
        case UpgradeKind::Mender:        return {"Mender", "every time it bounces off the core, the core repairs a little"};
        case UpgradeKind::HeavyImpact:   return {"Heavy impact", "+15% contact damage (stacks)"};
        case UpgradeKind::BigBall:       return {"Big ball", "+10% radius (stacks)"};
        case UpgradeKind::Swift:         return {"Swift", "+8% cruise speed (stacks)"};
        case UpgradeKind::CeilingBreak:  return {"Ceiling break", "+20% top speed (stacks)"};
        case UpgradeKind::HeavyKnock:    return {"Heavy knock", "shoves enemies back harder (stacks)"};
        case UpgradeKind::FlingMomentum: return {"Reflexes", "keeps a fling's speed longer (stacks)"};
        case UpgradeKind::CoreSpring:    return {"Spring core", "your balls bounce off the core faster"};
        case UpgradeKind::CoreSlowField: return {"Slow field", "enemies near the core are slowed"};
        case UpgradeKind::StrongArm:     return {"Strong arm", "you fling every ball noticeably harder"};
        case UpgradeKind::Contagion:     return {"Contagion", "an enemy that dies poisoned poisons those near it"};
        case UpgradeKind::Primed:        return {"Primed", "+damage to enemies already burning, poisoned or frozen"};
        case UpgradeKind::Catalyst:      return {"Catalyst", "element reactions hit twice as hard and reach further"};
        case UpgradeKind::ChainReaction: return {"Chain reaction", "35% chance a reaction sets off again on another afflicted enemy - it can cascade"};
        case UpgradeKind::LuckyClover:   return {"Lucky clover", "every chance (crits, echoes, zaps, bombs, ghosts...) x1.6"};
        case UpgradeKind::GlassCannon:   return {"Glass cannon", "all damage x1.6, but the core loses 30% of its max health"};
        case UpgradeKind::MagneticCore:  return {"Magnetic core", "every ball bouncing off the core flies at the nearest enemy"};
    }
    return {"", ""};
}

// One ball of the run: its item slots and its stacked modifiers. Its element
// is whichever element item is equipped (Plain if none); its role is the tag
// with 2+ items (4 = mastery).
struct BallLoadout {
    int gear[kBallSlots] = {-1, -1, -1, -1};   // UpgradeKind per item slot, -1 = empty
    int gearLvl[kBallSlots] = {0, 0, 0, 0};    // forge level, 1 once equipped
    int mods[kModifierCount] = {};             // stacks per modifier (modifierIndex)

    bool has(UpgradeKind k) const {
        for (int g : gear)
            if (g == static_cast<int>(k)) return true;
        return false;
    }
    // Slot holding an element item, -1 if none.
    int elementSlot() const {
        for (int i = 0; i < kBallSlots; ++i)
            if (gear[i] >= 0 && elementItemSlot(static_cast<UpgradeKind>(gear[i])) >= 0) return i;
        return -1;
    }
    Element element() const {
        const int s = elementSlot();
        return s < 0 ? Element::Plain
                     : static_cast<Element>(elementItemSlot(static_cast<UpgradeKind>(gear[s])) + 1);
    }
    int tagCount(ItemTag t) const {
        int n = 0;
        for (int g : gear)
            if (g >= 0 && itemTag(static_cast<UpgradeKind>(g)) == t) ++n;
        return n;
    }
    // The tag with the most items, if it has 2+. Ties go to the tag of the
    // earliest slot.
    ItemTag leadTag() const {
        ItemTag best = ItemTag::None;
        int bestN = 1;
        for (int g : gear) {
            if (g < 0) continue;
            const ItemTag t = itemTag(static_cast<UpgradeKind>(g));
            const int n = tagCount(t);
            if (t != ItemTag::None && n > bestN) { best = t; bestN = n; }
        }
        return best;
    }
    BallRole role() const { return tagRole(leadTag()); }
    bool mastery() const { return leadTag() != ItemTag::None && tagCount(leadTag()) >= 4; }
};

inline bool upgradeTakesSlot(UpgradeKind k) {
    const UpgradeCat c = upgradeCat(k);
    return c == UpgradeCat::Item || c == UpgradeCat::Element;
}

inline bool upgradeNeedsTarget(UpgradeKind k) {
    return upgradeCat(k) == UpgradeCat::Modifier || upgradeTakesSlot(k);
}

// Can pick `k` go on this ball? Items: no duplicates on one ball; Conductor /
// Bedrock only with their element equipped. Modifiers: always.
inline bool upgradeFitsBall(UpgradeKind k, const BallLoadout& b) {
    switch (upgradeCat(k)) {
        case UpgradeCat::Modifier: return true;
        case UpgradeCat::Element:
        case UpgradeCat::Item:
            if (b.has(k)) return false;
            if (k == UpgradeKind::Conductor) return b.element() == Element::Electric;
            if (k == UpgradeKind::Bedrock) return b.element() == Element::Stone;
            return true;
        default: return false;
    }
}

// Where an item lands on this ball when no slot is picked: an element always
// replaces the ball's current element (one per ball); otherwise the first free
// slot, else slot 0.
inline int defaultSlot(UpgradeKind k, const BallLoadout& b) {
    if (elementItemSlot(k) >= 0 && b.elementSlot() >= 0) return b.elementSlot();
    for (int i = 0; i < kBallSlots; ++i)
        if (b.gear[i] < 0) return i;
    return 0;
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
};

inline int elementsUnlocked(const UpgradeCtx& c) {
    int n = 0;
    for (bool u : c.elemUnlocked)
        if (u) ++n;
    return n;
}

inline bool upgradeEligible(UpgradeKind k, const UpgradeCtx& c) {
    const int ballCount = c.balls ? static_cast<int>(c.balls->size()) : 0;
    auto anyBallFits = [&] {
        if (!c.balls) return false;
        for (const BallLoadout& b : *c.balls)
            if (upgradeFitsBall(k, b)) return true;
        return false;
    };
    switch (upgradeCat(k)) {
        case UpgradeCat::NewBall:  return ballCount < c.maxBalls;
        case UpgradeCat::Modifier: return anyBallFits();
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
        /* Ignition  */ {"Ignition",  "the fire item can appear; higher levels hit harder",
                         2u,  3, MetaBranch::Special, MetaCurrency::Prisms,  0,  1.00f,  0.00f},
        /* Venom     */ {"Venom",     "the poison item can appear; higher levels stack faster",
                         2u,  3, MetaBranch::Special, MetaCurrency::Prisms,  3,  2.00f,  0.80f},
        /* Tide      */ {"Tide",      "the water item can appear; higher levels leave a wider wake",
                         3u,  3, MetaBranch::Special, MetaCurrency::Prisms,  4,  3.00f,  1.70f},
        /* Frost     */ {"Frost",     "the ice item can appear; higher levels freeze for longer",
                         3u,  3, MetaBranch::Special, MetaCurrency::Prisms,  5,  4.00f,  2.60f},
        /* Quarry    */ {"Quarry",    "the stone item can appear; higher levels grind harder",
                         4u,  3, MetaBranch::Special, MetaCurrency::Prisms,  6,  4.00f,  4.00f},
        /* Arc       */ {"Arc",       "the electric item can appear; higher levels zap harder",
                         4u,  3, MetaBranch::Special, MetaCurrency::Prisms,  7,  5.00f,  4.60f},
        /* Fortune   */ {"Fortune",   "earn cores for every enemy you kill",
                         6u,  3, MetaBranch::Eco,     MetaCurrency::Cores,   0,  0.00f, -1.00f},
        /* Windfall  */ {"Windfall",  "20% chance a cleared run pays a 2nd prism",
                         14u, 1, MetaBranch::Eco,     MetaCurrency::Cores,   9,  0.00f, -2.00f},
        /* Heft      */ {"Heft",      "every ball gets +8% contact damage",
                         10u, 2, MetaBranch::Combat,  MetaCurrency::Cores,   0,  0.00f,  1.00f},
        /* Mass      */ {"Mass",      "every ball is 10% larger",
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
