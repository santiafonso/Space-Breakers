#pragma once

// Every between-wave pick, its rarity tier and its class tag. Split from
// Offers.hpp so the per-class item tables (progression/ClassItems.hpp) can see
// the enum.

namespace sb {

// Order = category ranges (see upgradeCat in Offers.hpp): new ball, elements,
// abilities, modifiers, ITEMS, relics. Items sit between the modifiers and the
// relics, so a class adds its items inside its own section below and nothing
// else moves. The numbers are never saved (a run isn't saved mid-run), so
// inserting is safe.
enum class UpgradeKind {
    // new ball
    AddBall,
    // elements: the ball's TYPE slot (order = element web-node slots 0..5)
    ElemFire,
    ElemPoison,
    ElemWater,
    ElemIce,
    ElemStone,
    ElemElectric,
    // abilities: timed actives in the ball's ability slot(s) (order = Ability 1..7;
    // keep AbilityMeteor last - upgradeCat reads the range)
    AbilityDash,       // bursts straight at the nearest enemy
    AbilityNova,       // a shockwave around the ball
    AbilitySplit,      // two short-lived ghost copies
    AbilityBulwark,    // the core pushes out a shove-and-stagger pulse
    AbilityOverclock,  // a few seconds faster and harder-hitting
    AbilityArc,        // a bolt leaps through a chain of enemies (added with the Mage)
    AbilityMeteor,     // a meteor on the thickest pack of enemies (added with the Mage)
    // modifiers (no slot, stack)
    HeavyImpact,       // +contact damage
    BigBall,           // +radius, +knockback
    Swift,             // +cruise and top speed, holds a fling longer

    // ---- items (take one of the 4 item slots; a duplicate levels it up) ----
    Ricochet,          // a wall bounce speeds it up and arms a harder hit [Striker]
    Cleave,            // punches through an enemy it kills               [Striker]
    Crit,              // "Keen eye": chance of a big hit                  [Striker]
    Executioner,       // big bonus vs badly hurt enemies                  [Striker]
    Overkill,          // a kill's leftover damage splashes                [Support]
    Shatter,           // bonus damage vs frozen enemies (needs Frost)     [Support]
    Conductor,         // its electric arc jumps on to more enemies        [Striker]
    Bedrock,           // its stone rubble lasts far longer                [Guardian]
    Echo,              // chance a hit strikes twice                       [Striker]
    Tesla,             // chance a hit zaps nearby enemies                 [Support]
    Bomber,            // chance a kill explodes                           [Support]
    SplitShot,         // chance a wall bounce spawns a ghost copy         [Support]
    Rampart,           // hits shove much further and stagger longer       [Guardian]
    Mender,            // each core bounce repairs the core a little       [Guardian]
    // behaviour items (Fase M): each one makes the ball play differently
    Hunter,            // locks onto the biggest threat until it dies      [Striker]
    Comet,             // flung, it flies far faster and plows through     [Striker]
    Mitosis,           // a kill splits off small copies of it             [Striker]
    Boomerang,         // after a hit it flies home; comes out charged     [Guardian]
    Bumper,            // other balls bounce off it much faster            [Guardian]
    Glutton,           // grows and hits harder with every kill (per wave) [Guardian]
    Tether,            // a damaging laser to the nearest other ball       [Support]
    BlackHole,         // kills may leave a black hole: sucks in, bursts   [Support]
    Resonance,         // hits arc through every same-element ball         [Support]
    // game-changers: they change how the ball itself behaves
    Seeker,            // curves toward the nearest enemy                  [Striker]
    Piercing,          // passes through enemies instead of bouncing       [Striker]
    Railgun,           // every wall bounce fires a beam along its path    [Striker]
    Berserk,           // each hit without touching a wall hits harder     [Striker]
    Giant,             // huge, slower, heavier                            [Guardian]
    Satellite,         // orbits the core instead of bouncing around       [Guardian]
    GravityWell,       // drags nearby enemies toward itself               [Support]
    Storm,             // zaps everything around it, all the time          [Support]
    Gemini,            // permanent ghost twins with the same items        [Support]
    Midas,             // its kills pay extra gold                         [Support]

    // ---- Mage items (phase 2: add them here, describe them in ClassItems.hpp) ----
    Focus,             // its abilities recharge faster                    [Mage]
    ArcaneMissile,     // arcane missiles on a timer and on every cast     [Mage]
    Attunement,        // its abilities hit harder and reach further       [Mage]
    Twincast,          // chance a cast fires again a moment later         [Mage]
    ManaSpring,        // a cast charges its other abilities               [Mage]

    // ---- Shooter items ----
    RapidFire,         // fires far more often                             [Shooter]
    Scattershot,       // each volley is a fan of pellets                  [Shooter]
    Rebound,           // bullets hop from enemy to enemy                  [Shooter]
    Tracer,            // bullets carry the ball's element                 [Shooter]
    DrillRounds,       // bullets punch through enemies and shields        [Shooter]
    HairTrigger,       // a hit fires a burst at the enemies around it     [Shooter]

    // ---- Assassin items ----
    Backstab,          // the first hit after a blink hits much harder    [Assassin]
    Cull,              // a hit that leaves an enemy nearly dead kills it  [Assassin]
    KillingSpree,      // each blink in a chain adds damage               [Assassin]
    ShadowTrail,       // the blink path cuts what it crosses             [Assassin]
    SmokeBomb,         // a burst with its element where it lands         [Assassin]
    Phantom,           // a shadow copy stays behind where it blinked from [Assassin]

    // ---- Summoner items ----
    SummonTurret,      // a wall bounce plants a turret                    [Summoner]
    SummonWisps,       // kills let loose homing wisps                     [Summoner]
    SummonTotem,       // plants a totem that slows enemies around it      [Summoner]
    SummonWarden,      // spirits circle the core, hitting what they touch [Summoner]
    SummonDragon,      // a dragonling breathes the ball's element         [Summoner]

    // ---- Jester items ----
    LuckyCharm,        // + luck points for the whole run                  [Jester]
    CoinFlip,          // every hit flips a coin: heads hits harder, tails softer [Jester]
    WildCard,          // chance a hit fires a random proc borrowed from any ball [Jester]
    Reroll,            // a missed Jester roll gets a second try           [Jester]
    ChaosBounce,       // wall bounces fly off at random, arming a harder hit [Jester]
    Jackpot,           // a kill may hit the jackpot: gold and a big blast [Jester]

    // ---- relics (keep CoreSpring first and Overcharge last) ----
    CoreSpring,        // balls ricochet off the core faster
    CoreSlowField,     // a zone around the core slows enemies inside it
    StrongArm,         // you fling every ball harder
    Contagion,         // a poisoned enemy dying poisons its neighbours (needs Venom)
    Primed,            // +damage vs enemies under any element effect (needs any element node)
    Catalyst,          // reactions hit twice as hard and wider (needs 2 element nodes)
    ChainReaction,     // a reaction may echo onto another afflicted enemy (needs 2 element nodes)
    LuckyClover,       // +6 luck
    GlassCannon,       // all damage x1.6, core max HP -30%
    MagneticCore,      // balls leave the core aimed at the nearest enemy
    PrismCore,         // balls with no element leave a random one on every hit (reactions everywhere)
    Phoenix,           // once per act the core comes back from 0 at half health
    TimeDilation,      // enemies move 25% slower, always
    Overcharge,        // the damage combo can climb twice as high
};
inline constexpr int kUpgradeKindCount = static_cast<int>(UpgradeKind::Overcharge) + 1;
inline constexpr int kMaxUpgradeKinds = 256;   // capacity of UpgradeCtx::locked
static_assert(kUpgradeKindCount <= kMaxUpgradeKinds, "raise kMaxUpgradeKinds");

// ---- tiers: how rare (and how strong) a pick is -----------------------------
// Commons are small, steady bumps that grow when stacked; legendaries change
// how the run plays.
enum class Tier { Common, Uncommon, Rare, Epic, Legendary };
inline constexpr int kTierCount = 5;

// ---- item tags: a ball's classes come from them -----------------------------
// Same order as BallRole (sim/Entities.hpp), None = Normal.
enum class ItemTag { None, Striker, Guardian, Support, Mage, Shooter, Assassin, Summoner, Jester };

// Everything the UI and the roll need to know about one class item. The five
// newer classes describe their items with these (ClassItems.hpp); the older
// items keep their switch cases in Offers.hpp.
struct ItemDef {
    UpgradeKind kind;
    const char* id;          // machine name, for SB_UPGRADES
    const char* title;
    const char* desc;
    const char* levelDesc;   // what one more level does
    Tier tier;
    ItemTag tag;
};

}  // namespace sb
