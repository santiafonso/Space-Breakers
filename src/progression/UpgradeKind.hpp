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
    // abilities: timed actives in the ball's ability slot(s) (order = Ability 1..8;
    // keep AbilityMissile last - upgradeCat reads the range)
    AbilityDash,       // bursts straight at the nearest enemy
    AbilityNova,       // a shockwave around the ball
    AbilitySplit,      // two short-lived ghost copies
    AbilityBulwark,    // the core pushes out a shove-and-stagger pulse
    AbilityOverclock,  // a few seconds faster and harder-hitting
    AbilityArc,        // a bolt leaps through a chain of enemies (added with the Mage)
    AbilityMeteor,     // a meteor on the thickest pack of enemies (added with the Mage)
    AbilityMissile,    // homing magic missiles: the Mage's signature ability
    // modifiers (no slot, stack)
    HeavyImpact,       // +contact damage
    BigBall,           // +radius, +knockback
    Swift,             // +cruise and top speed, holds a fling longer
    // (2026-09-28: more modifiers, so a post-fight pick isn't always the same three)
    Ballast,           // slower, but hits harder
    Keen,              // + crit chance
    Reach,             // you can grab it from further away
    Tempered,          // its element is stronger
    Spin,              // its hits may climb the combo an extra step
    Leech,             // its kills patch the core up a little
    QuickMind,         // its abilities recharge faster
    HeavyThrow,        // the first hit after your throw lands harder
    Bouncy,            // leaves the core faster after a bounce

    // ---- items (take one of the 4 item slots; a duplicate levels it up) ----
    Ricochet,          // a wall bounce speeds it up and arms a harder hit [Striker]
    Cleave,            // punches through an enemy it kills               [Striker]
    Crit,              // "Keen eye": chance of a big hit                  [Striker]
    Executioner,       // big bonus vs badly hurt enemies                  [Striker]
    Overkill,          // a kill's leftover damage splashes                [Support]
    Shatter,           // bonus damage vs frozen enemies (needs Frost)     [Support]
    Conductor,         // its electric arc jumps on to more enemies        [Striker]
    Bedrock,           // its stone cracks last longer and go deeper       [Guardian]
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
    ArcaneMissile,     // "Barrage": more magic missiles; every cast looses one [Mage]
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

    // ---- Slinger items (the class of throwing and catching, 2026-09-28) ----
    Coil,              // coasts to a stop; thrown by you it flies twice as fast [Slinger]
    CatchRelease,      // caught soon after your throw: stacks damage         [Slinger]
    Afterburner,       // thrown by you, it leaves a trail of real fire        [Slinger]
    Momentum,          // the faster it flies, the harder it hits (no cap)     [Slinger]
    Grip,              // curves toward your pointer when it's near            [Slinger]
    Ambush,            // a thrown ball's first hit blinks it onto the next enemy [Slinger]
    TrickShot,         // until the first hit after a throw, every chance x3   [Slinger]
    DoubleDown,        // catch a ball you threw: its next hit is double or nothing [Slinger]
    ExecutionThrow,    // a thrown ball's first hit on an unhurt enemy crits   [Slinger]

    // ---- speed items for the other classes (2026-09-28): "still" ones grow
    // as the ball slows, "moving" ones as it speeds up - never on / off ----
    Anchor,            // slow: enemies near it are slowed and drawn in       [Guardian]
    Plow,              // fast: shoves and hits the enemies it passes         [Guardian]
    Slug,              // slow: fires faster and further                      [Shooter]
    Strafe,            // fast: fires side volleys                            [Shooter]
    Lurk,              // slow: charges its next hit after a blink            [Assassin]
    Blur,              // fast: passes through enemies, marking them          [Assassin]
    Sleight,           // slow: now and then teleports onto an enemy and hits [Jester]
    Meditate,          // slow: abilities recharge faster                     [Mage]
    Leyline,           // moving: drops runes that burst on each cast         [Mage]
    Beacon,            // slow: balls passing near it hit harder with its element [Support]
    Wake,              // fast: its trail speeds up the balls that cross it   [Support]
    Pass,              // a ball it clacks is launched with its element       [Support]
    Kennel,            // slow: lets loose homing wisps                       [Summoner]
    DropTurret,        // your throw leaves a turret where you let go         [Summoner]

    // ---- Alchemist items (the class of elements, 2026-09-28) ----
    Attune,            // its elements are stronger                          [Alchemist]
    Crucible,          // its reactions hit harder and wider                 [Alchemist]
    Aftershock,        // its reactions leave its element on the pack        [Alchemist]
    Flux,              // each of its reactions recharges its abilities      [Alchemist]
    PrismHit,          // "Prism": every 4th hit carries all its elements    [Alchemist]
    Conflux,           // its reactions leap once to another afflicted enemy [Alchemist]

    // ---- commons (2026-10-04): every class's base widened, one line each,
    // each built to combine with other classes too. Described in ClassItems.hpp
    // (commonItemDef), numbers in core/ClassSpec.cpp (foldCommon), behaviour in
    // sim/WorldCommons.cpp. Appended here (not in the class sections) so a
    // saved run's item numbers keep meaning the same items. ----
    Volatile,          // its elements last longer on enemies                [Alchemist]
    Seep,              // a wall bounce leaves a puddle of its element       [Alchemist]
    Reagent,           // + damage vs another ball's element                 [Alchemist]
    Tincture,          // a ball it clacks carries its element               [Alchemist]
    Distill,           // every few reactions it sets off patch the core     [Alchemist]
    Pickpocket,        // a hit may steal a coin                             [Jester]
    HotStreak,         // doubles in a row stack damage                      [Jester]
    LuckyBreak,        // a hit may recharge all its abilities               [Jester]
    Confetti,          // a doubled hit throws sparks                        [Jester]
    Spellblade,        // the first hit after a cast hits harder             [Mage]
    AetherStep,        // a cast speeds it up                                [Mage]
    Insight,           // kills shorten its cooldowns                        [Mage]
    Lingering,         // its summons last longer                            [Summoner]
    PackLeader,        // + damage per live summon                           [Summoner]
    Decoy,             // its turrets and totems draw enemies in             [Summoner]
    MarkForDeath,      // its blink target takes more from every ball        [Assassin]
    Bleed,             // its hits bleed                                     [Assassin]
    Quickstep,         // faster for a moment after a blink                  [Assassin]
    Opportunist,       // + damage vs frozen or staggered enemies            [Assassin]
    HollowPoint,       // bullets hit harder on enemies under an element     [Shooter]
    BankShot,          // bullets bounce off the walls                       [Shooter]
    CoveringFire,      // fires a volley when you throw another ball         [Shooter]
    LongPull,          // a longer slingshot pull, a harder first hit        [Slinger]
    Relay,             // caught soon after a throw: launches another ball   [Slinger]
    SecondWind,        // caught: recharges your Q                           [Slinger]
    Plating,           // + core max HP                                      [Guardian]
    Sentinel,          // hits harder the closer it flies to the core        [Guardian]
    Brace,             // its hits slow enemies                              [Guardian]
    HeavyLanding,      // a core bounce sends out a shockwave                [Guardian]
    Bulk,              // bigger and a little harder                         [Guardian]
    Edge,              // + damage, plain                                    [Striker]
    FirstBlood,        // the first hit of each fight hits x3                [Striker]
    ComboHit,          // a second enemy hit quickly takes more              [Striker]
    Adrenaline,        // kills speed it up for the fight                    [Striker]
    Spotter,           // enemies it hits take more from the other balls     [Support]
    Anthem,            // every other ball hits a little harder              [Support]
    Medic,             // a clack with another ball patches the core         [Support]
    Sync,              // its casts shorten the other balls' cooldowns       [Support]
    HandOff,           // its kills charge the nearest ball's next hit       [Support]
    // ---- uncommons (2026-10-04), same home and same rule ----
    Fission,           // an enemy dying under its element spreads it         [Alchemist]
    Elixir,            // its reactions make every ball hit harder a moment  [Alchemist]
    HouseEdge,         // gold you earn may come double                      [Jester]
    Mirror,            // the nearest ball may copy its hit                   [Jester]
    Sigil,             // a cast leaves a glyph the other balls set off       [Mage]
    ArcaneCharge,      // wall bounces shorten its cooldowns                  [Mage]
    Conduit,           // a cast primes enemies with another ball's element   [Mage]
    Martyr,            // its turrets and totems burst when they end          [Summoner]
    GuardDog,          // its summons go for the enemy nearest the core       [Summoner]
    ChainKill,         // a kill right after a blink recharges the blink      [Assassin]
    VenomBlade,        // the first hit after a blink leaves its element twice [Assassin]
    SuppressingFire,   // bullets slow what they hit                          [Shooter]
    AmmoFeed,          // a wall bounce fires a volley                        [Shooter]
    WindUp,            // held longer before your throw, it flies faster      [Slinger]
    BankThrow,         // wall bounces after your throw charge its first hit  [Slinger]
    Bully,             // + damage vs bigger enemies                          [Striker]
    Carom,             // a hit right after a clack hits harder               [Striker]
    // ---- rares (2026-10-04), same home and same rule ----
    Transmute,         // a reaction it sets off lends it the other element   [Alchemist]
    Residue,           // its reactions leave a priming puddle                [Alchemist]
    Aurum,             // kills by its reactions pay gold                     [Alchemist]
    HighRoller,        // doubled hits land far harder, plain ones softer     [Jester]
    LuckySeven,        // every 7th hit is a sure double                      [Jester]
    Fortune,           // each double makes its chances likelier this fight   [Jester]
    Projection,        // its ghost copies cast its abilities too             [Mage]
    Spellweave,        // its abilities hit harder on enemies under an element [Mage]
    Cascade,           // an ability that kills is ready again at once        [Mage]
    Hatchery,          // a summon's kill lets a wisp loose                   [Summoner]
    Golem,             // calls a stone golem now and then                    [Summoner]
    Soulbind,          // its summons leave their element's full effect       [Summoner]
    Predator,          // the blink reaches twice as far, at the weakest      [Assassin]
    Flicker,           // a blink may blink again                             [Assassin]
    FragRounds,        // bullets burst                                       [Shooter]
    Marksman,          // every 5th bullet crits and marks                    [Shooter]
    SkippingStone,     // thrown, it skips off enemies without slowing        [Slinger]
    Formation,         // your throw sends the other balls the same way       [Slinger]
    ShieldBash,        // enemies it hits near the core fly off, stunned      [Guardian]
    // ---- epics (2026-10-04), same home and same rule ----
    Saturate,          // its reactions leave both elements' full effects     [Alchemist]
    Maelstrom,         // now and then a ring of all its elements             [Alchemist]
    DeathSentence,     // what it hits dies under 20% to any ball             [Assassin]
    RazorWire,         // each blink leaves a cutting line                    [Assassin]
    WardRing,          // the core throws back the first enemy every few s    [Guardian]
    Juggernaut,        // its size bonus counts double as damage              [Guardian]
    LastLine,          // once a fight, a low core blasts everything away     [Guardian]
    SlotMachine,       // spins for buffs now and then                        [Jester]
    WildMagic,         // a double casts a random ability from the run        [Jester]
    SpellChorus,       // every 5th cast, every ball casts                    [Mage]
    Chronomancy,       // each cast slows every enemy a moment                [Mage]
    Minigun,           // fires faster the longer it keeps firing             [Shooter]
    SmartRounds,       // bullets home in                                     [Shooter]
    Shrapnel,          // its kills burst into a ring of bullets              [Shooter]
    PerfectRelease,    // a full-pull throw hits x2.5 and pierces to a wall   [Slinger]
    Homecoming,        // after its first hit it curves back to your hand     [Slinger]
    Lightspeed,        // very fast, its hits chain and don't slow it         [Striker]
    SpiritHost,        // 5 wisps out merge into a great spirit               [Summoner]
    TwinCall,          // everything it summons comes in pairs                [Summoner]
    Fortify,           // its turrets stop what touches them and last longer  [Summoner]
    // ---- legendaries (2026-10-05), same home and same rule ----
    Titan,             // grows on every core bounce; full size, a quake      [Guardian]
    Archmage,          // abilities twice as fast, each cast stronger         [Mage]
    TimeStop,          // every 5th cast stops every enemy a moment           [Mage]
    IonCannon,         // slower; charges a laser across the arena            [Shooter]
    Gunship,           // every ball fires a bullet when it hits              [Shooter]
    LeadStorm,         // now and then it empties the gun all around          [Shooter]
    Reaper,            // the blink fires after every hit                     [Assassin]
    Contract,          // the strongest enemy takes a share of its max hp     [Assassin]
    Necromancer,       // what its summons kill rises as an exploding ghost   [Summoner]
    Overlord,          // every ball's summons are its own                    [Summoner]
    Rigged,            // every chance in the run rolls twice                 [Jester]
    Joker,             // each fight, a copy of a random legendary            [Jester]
    SteadyHand,        // aiming, time almost stops - and the throw hits more [Slinger]
    Showtime,          // quick catch-and-throws stack damage and speed       [Slinger]
    PhilosopherStone,  // every hit carries elements of all six              [Alchemist]
    GrandReaction,     // every 10th reaction is huge                         [Alchemist]

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

    // ---- abilities, second batch (2026-10-05): after the relics so saved
    // runs keep their numbers (upgradeCat / abilityOf read this range too;
    // order = Ability 9..; keep AbilityBounty last) ----
    AbilityHook,       // yanks the enemy nearest the core over to the ball
    AbilityMines,      // drops mines along its path
    AbilityEmpower,    // its next hit x3 and a shockwave
    AbilityTailwind,   // every ball faster for a few seconds
    AbilityVortex,     // a whirlpool that drags enemies together
    AbilityBeam,       // a steady beam at the nearest enemy
    AbilityBlades,     // blades spin around the ball
    AbilityCloud,      // a cloud of its element hangs where it was
    AbilitySmite,      // lightning on the strongest enemies
    AbilityWarp,       // teleports onto the enemy nearest the core
    AbilityPortal,     // a pair of portals the balls fly through
    AbilityLeash,      // tied to the core, it swings around like a flail
    AbilitySwap,       // trades places with the enemy nearest the core
    AbilityShrink,     // enemies nearby shrink: slower, and take double
    AbilityBounty,     // a price on the strongest enemy's head
};
inline constexpr int kUpgradeKindCount = static_cast<int>(UpgradeKind::AbilityBounty) + 1;
inline constexpr int kMaxUpgradeKinds = 384;   // capacity of UpgradeCtx::locked
static_assert(kUpgradeKindCount <= kMaxUpgradeKinds, "raise kMaxUpgradeKinds");

// ---- tiers: how rare (and how strong) a pick is -----------------------------
// Commons are small, steady bumps that grow when stacked; legendaries change
// how the run plays.
enum class Tier { Common, Uncommon, Rare, Epic, Legendary };
inline constexpr int kTierCount = 5;

// ---- item tags: a ball's classes come from them -----------------------------
// Same order as BallRole (sim/Entities.hpp), None = Normal.
enum class ItemTag { None, Striker, Guardian, Support, Mage, Shooter, Assassin, Summoner, Jester, Slinger, Alchemist };

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
