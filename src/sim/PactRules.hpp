#pragma once

namespace sb {

// The slice of the run's pacts (progression/Pacts.hpp) the simulation has to
// know about. Plain numbers (damage, cruise, radius, elements, crit, throw
// power) are folded in by App (core/AppPacts.cpp). All defaults = no pact.
// The hooks live in sim/WorldPacts.cpp.
struct PactRules {
    float catchMul = 1.f;         // Quick Hands: catch reward x this...
    bool idlePenalty = false;     // ...and a ball you left alone hits softer
    float stillness = 0.f;        // Stillness: + damage at a standstill (scales with slowness)
    bool glassEdge = false;       // Glass Edge: non-crit hits weaker
    bool hotPotato = false;       // Hot Potato: holding charges the throw; too long drops it
    bool juggler = false;         // Juggler: catches in a row stack damage
    bool voidWalls = false;       // Void Walls: balls wrap around the arena edges
    bool anchorWalls = false;     // Anchor Walls: a wall stops a ball almost dead
    bool lastBreath = false;      // Last Breath: x2 damage with the core low
    bool mirror = false;          // Mirror: a throw also sends a ghost the other way
    bool frenzy = false;          // Frenzy: the combo climbs twice as fast; grabbing breaks it
    float enemyHpMul = 1.f;       // Mirror's cost
    float enemyCountMul = 1.f;    // Horde's cost
};

}  // namespace sb
