#pragma once

namespace sb {

// ---- pacts: a gift with a price (2026-09-28) --------------------------------
//
// Smaller than a creed and only about how the fight plays: each one gives
// something and takes something. Found only down the hidden path that opens
// before the boss after three flawless fights in a row (App::revealAltarPath),
// once the act 2 boss has fallen in some run (2026-10-29). A run can hold any number of them. They reach the
// sim as WorldParams::pact (sim/PactRules.hpp); App folds the plain numbers.
//
// Ids are saved with the run: append only. 2026-10-29 the seven pacts that were
// only numbers gave their slots to ones that change how a fight plays
// (QuickHands -> Afterimage, HeavyArm -> Starfall, GlassEdge -> PowderKeg,
// Stillness -> Lodestone, Overflow -> Billiards, LastBreath -> RubberWalls,
// Elemental -> Scorch).

enum class PactId {
    Lead,        // +damage / balls much slower
    Afterimage,  // a caught ball leaves a ghost flying on / tougher enemies
    Starfall,    // every 6th throw is a meteor / the other throws weaker
    PowderKeg,   // enemies blow up when they die / blasts by the core chip it
    Lodestone,   // a held ball pulls enemies in / slower throws
    Billiards,   // a kill sends the enemy flying into the ones behind / faster enemies
    Tiny,        // much more damage / balls half as big
    Colossus,    // your best ball twice as big and strong / the rest weaker
    HotPotato,   // a held ball charges up / held too long it drops dead
    Juggler,     // catches in a row stack damage / the core breaks the streak and hurts
    VoidWalls,   // balls pass through the edges / wall-bounce items stop working
    AnchorWalls, // throws x2 / walls stop a ball dead
    RubberWalls, // every wall bounce stacks damage / and speed: harder to catch
    Mirror,      // every throw sends a ghost the other way / tougher enemies
    Scorch,      // balls leave a burning trail / smaller balls
    Frenzy,      // the combo climbs twice as fast / grabbing a ball breaks it
    Blind,       // stronger throws / no aim guide
    Horde,       // a ball for every boss you kill / more enemies per fight
};
inline constexpr int kPactCount = 18;

struct PactDef {
    const char* name;
    const char* gain;      // what you get
    const char* cost;      // what it takes
    const char* synergy;   // "pairs well with" - hover help
    bool needsHands;       // only worth it if you grab balls (not offered under Hunters / Clockwork)
};

inline const PactDef& pactDef(PactId id) {
    static const PactDef defs[kPactCount] = {
        {"Lead", "every ball hits 60% harder", "every ball cruises 40% slower",
         "Billiards, Guardian items, Slug, catching", false},
        {"Afterimage", "catch a flying ball and a ghost of it flies on for 5 s, hitting like the ball",
         "enemies have 20% more health", "Juggler, Mirror, fast balls, Striker items", true},
        {"Starfall", "every 6th throw falls as a meteor: its first hit lands 3x and blasts everything around",
         "the other throws hit 15% softer", "Hot Potato, catch reward, Big ball, Slinger items", true},
        {"Powder keg", "every enemy blows up when it dies, hurting the ones around it - chains of them",
         "a blast next to the core chips it", "Billiards, crowds, Horde, Overkill", false},
        {"Lodestone", "while you hold a ball, enemies around it are pulled toward it - then throw into the clump",
         "your throws leave your hand 25% slower", "Starfall, Hot Potato, area items, Powder keg", true},
        {"Billiards", "an enemy you kill with a ball is knocked flying and slams the ones behind it for half the hit",
         "enemies move 10% faster", "Powder keg, Lead, Guardian items, big hits", false},
        {"Tiny", "every ball hits 80% harder", "every ball is half as big - harder to catch, easier to miss",
         "Big ball, Giant, Keen eye, Scorch", false},
        {"Colossus", "your most built-up ball is twice as big and hits twice as hard",
         "every other ball deals 30% less damage", "Guardian items, Giant, Duet", false},
        {"Hot Potato", "a ball you hold charges up: its throw's first hit +25% per second held",
         "hold it past 3 s and it slips out of your hand with no throw", "Starfall, Lodestone, Striker items", true},
        {"Juggler", "every catch in a row stacks +12% damage on that ball (up to 8)",
         "a juggled ball touching the core drops its streak and chips the core", "Afterimage, Hot Potato", true},
        {"Void Walls", "balls fly out one edge and come back through the other - no wall stops them",
         "wall-bounce items (Ricochet, Split shot, Railgun...) never fire", "Seeker, Hunter, Piercing, Comet", false},
        {"Anchor Walls", "your throws leave your hand twice as fast", "a wall stops a ball almost dead",
         "Rubber walls, catching, Striker items", true},
        {"Rubber walls", "every wall bounce adds +15% damage to that ball (up to 6) until you catch it",
         "each bounce also makes it 6% faster - harder to catch", "Ricochet, Pinball, Anchor Walls, Bank shot", false},
        {"Mirror", "every throw also sends a ghost copy the opposite way", "enemies have 20% more health",
         "Split shot, Brood, Striker items", true},
        {"Scorch", "balls leave a burning trail: enemies crossing it catch fire",
         "every ball is 20% smaller", "fire items, Ember, Tiny, many balls, fast balls", false},
        {"Frenzy", "the combo climbs twice as fast", "grabbing a ball resets the combo",
         "Overcharge, Bloodlust, many balls", true},
        {"Blind", "your throws leave your hand 40% faster", "the slingshot shows no aim guide",
         "Anchor Walls, Starfall", true},
        {"Horde", "every boss you beat adds a ball", "30% more enemies in every fight",
         "Powder keg, Legion, Support items", false},
    };
    return defs[static_cast<int>(id)];
}

// Two pacts that can't sit in the same run.
inline bool pactsConflict(PactId a, PactId b) {
    auto pair = [&](PactId x, PactId y) { return (a == x && b == y) || (a == y && b == x); };
    return pair(PactId::VoidWalls, PactId::AnchorWalls) || pair(PactId::Tiny, PactId::Colossus) ||
           pair(PactId::VoidWalls, PactId::RubberWalls);   // no walls to bounce off
}

}  // namespace sb
