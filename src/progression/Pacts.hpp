#pragma once

namespace sb {

// ---- pacts: a gift with a price (2026-09-28) --------------------------------
//
// Smaller than a creed and only about how the fight plays: each one gives
// something and takes something. Found at an Altar on the map - rare, or down
// the hidden path that opens before the boss after three flawless fights in a
// row (see App::openMap). A run can hold any number of them. They reach the
// sim as WorldParams::pact (sim/PactRules.hpp); App folds the plain numbers.
//
// Order is free to change: pacts are never saved (a run is never resumed).

enum class PactId {
    Lead,        // +damage / balls much slower
    QuickHands,  // catch reward x2 / a ball you leave alone hits softer
    HeavyArm,    // stronger throws / no slow motion while aiming
    GlassEdge,   // +crit chance / hits that don't crit are weaker
    Stillness,   // the slower a ball, the harder it hits / weaker quick throws
    Overflow,    // +1 ball / every ball hits softer
    Tiny,        // much more damage / balls half as big
    Colossus,    // your best ball twice as big and strong / the rest weaker
    HotPotato,   // a held ball charges up / held too long it drops dead
    Juggler,     // catches in a row stack damage / the core breaks the streak and hurts
    VoidWalls,   // balls pass through the edges / wall-bounce items stop working
    AnchorWalls, // throws x2 / walls stop a ball dead
    LastBreath,  // everything x2 while the core is low / smaller core
    Mirror,      // every throw sends a ghost the other way / tougher enemies
    Elemental,   // elements x2 / contact hits weaker
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
         "Stillness, Guardian items, Slug, catching", false},
        {"Quick Hands", "the catch reward is doubled: snatch a fast ball and its next hit lands up to twice as hard",
         "a ball you haven't thrown for 10 s hits 20% softer", "Juggler, Hot Potato, Striker items, fast balls", true},
        {"Heavy Arm", "your throws leave your hand 50% faster", "no slow motion: none while you aim, and no bullet time (E)",
         "Striker items, Comet, Catch reward", true},
        {"Glass Edge", "+30% crit chance on every ball", "hits that don't crit deal 20% less",
         "Keen eye, Echo, Jester items", false},
        {"Stillness", "the slower a ball moves, the harder it hits (up to +60% when nearly still)",
         "quick throws (a click) are 40% weaker", "Lead, Guardian items, Ballast, Totem", true},
        {"Overflow", "one more ball, right now", "every ball deals 15% less damage",
         "Legion, Support items, Bumper", false},
        {"Tiny", "every ball hits 80% harder", "every ball is half as big - harder to catch, easier to miss",
         "Big ball, Giant, Keen eye", false},
        {"Colossus", "your most built-up ball is twice as big and hits twice as hard",
         "every other ball deals 30% less damage", "Guardian items, Giant, Duet", false},
        {"Hot Potato", "a ball you hold charges up: its throw's first hit +25% per second held",
         "hold it past 3 s and it slips out of your hand with no throw", "Heavy Arm, Quick Hands, Striker items", true},
        {"Juggler", "every catch in a row stacks +12% damage on that ball (up to 8)",
         "a juggled ball touching the core drops its streak and chips the core", "Quick Hands, Hot Potato", true},
        {"Void Walls", "balls fly out one edge and come back through the other - no wall stops them",
         "wall-bounce items (Ricochet, Split shot, Railgun...) never fire", "Seeker, Hunter, Piercing, Comet", false},
        {"Anchor Walls", "your throws leave your hand twice as fast", "a wall stops a ball almost dead",
         "Heavy Arm, catching, Striker items", true},
        {"Last Breath", "while the core is under 30% health, every ball hits twice as hard",
         "the core loses 20% of its max health", "Regen, Aegis, Mender, Glass cannon", false},
        {"Mirror", "every throw also sends a ghost copy the opposite way", "enemies have 20% more health",
         "Split shot, Brood, Striker items", true},
        {"Elemental", "every element is twice as strong", "contact hits deal 30% less",
         "Support items, element nodes, Catalyst, Chain reaction", false},
        {"Frenzy", "the combo climbs twice as fast", "grabbing a ball resets the combo",
         "Overcharge, Bloodlust, many balls", true},
        {"Blind", "your throws leave your hand 40% faster", "the slingshot shows no aim guide",
         "Heavy Arm, Anchor Walls", true},
        {"Horde", "every boss you beat adds a ball", "30% more enemies in every fight",
         "Overflow, Legion, Support items", false},
    };
    return defs[static_cast<int>(id)];
}

// Two pacts that can't sit in the same run.
inline bool pactsConflict(PactId a, PactId b) {
    auto pair = [&](PactId x, PactId y) { return (a == x && b == y) || (a == y && b == x); };
    return pair(PactId::VoidWalls, PactId::AnchorWalls) || pair(PactId::Tiny, PactId::Colossus);
}

}  // namespace sb
