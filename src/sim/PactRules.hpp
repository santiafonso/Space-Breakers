#pragma once

namespace sb {

// The slice of the run's pacts (progression/Pacts.hpp) the simulation has to
// know about. Plain numbers (damage, cruise, radius, elements, crit, throw
// power) are folded in by App (core/AppPacts.cpp). All defaults = no pact.
// The hooks live in sim/WorldPacts.cpp.
struct PactRules {
    bool afterimage = false;      // Afterimage: a caught ball leaves a ghost flying on
    bool starfall = false;        // Starfall: every Nth throw is a meteor...
    bool starfallCost = false;    // ...and the others hit softer
    bool powderKeg = false;       // Powder keg: deaths blast...
    bool kegChip = false;         // ...and a blast by the core chips it
    bool lodestone = false;       // Lodestone: a held ball pulls enemies in (its price, slower throws, is App's flingPower)
    bool billiards = false;       // Billiards: a ball's kill slams the ones behind
    bool rubberWalls = false;     // Rubber walls: wall bounces stack damage...
    bool rubberSpeed = false;     // ...and speed
    bool scorch = false;          // Scorch: balls leave a burning trail
    bool hotPotato = false;       // Hot Potato: holding charges the throw; too long drops it
    bool juggler = false;         // Juggler: catches in a row stack damage
    bool voidWalls = false;       // Void Walls: balls wrap around the arena edges
    bool anchorWalls = false;     // Anchor Walls: a wall stops a ball almost dead
    bool mirror = false;          // Mirror: a throw also sends a ghost the other way
    bool frenzy = false;          // Frenzy: the combo climbs twice as fast; grabbing breaks it
    // The prices, apart from the gifts, so an "Unbinding" can lift one (2026-10-12)
    bool potatoSlip = false;      // Hot Potato: held too long, it slips
    bool juggleChip = false;      // Juggler: the core drops the streak and gets chipped
    bool frenzyBreak = false;     // Frenzy: grabbing resets the combo
    float enemyHpMul = 1.f;       // Mirror's cost
    float enemyCountMul = 1.f;    // Horde's cost
    float enemySpeedMul = 1.f;    // Billiards' cost
};

}  // namespace sb
