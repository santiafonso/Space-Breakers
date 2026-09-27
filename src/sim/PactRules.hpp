#pragma once

namespace sb {

// The slice of the run's pacts (progression/Pacts.hpp) the simulation has to
// know about. Everything that is just a number on existing params (damage,
// cruise, luck, ball count, core HP) is folded in by App instead, so this stays
// small. All defaults = no pact. The hooks live in sim/WorldPacts.cpp.
struct PactRules {
    // Thrower
    float speedCeilMul = 1.f;    // Hot Hands: a ball's top speed x this (flings can go far past cruise)
    float flingHold = 1.f;       // Hot Hands: fling-speed decay x this (< 1 keeps a throw fast longer)
    // Spectator
    bool hunters = false;        // Hunters: every ball homes on its own prey until it dies
    float autoFlingEvery = 0.f;  // Clockwork: > 0 = auto-throw a ball this often (s)
    float autoFlingSpeed = 0.f;  // ...at this x its cruise
    bool pinball = false;        // Pinball: wall bounces speed up, feed the combo and spark
    // Swarm
    bool legion = false;         // Legion: ball-vs-ball clacks throw sparks and feed the combo
    // Core
    bool livingCore = false;     // Living Core: the core zaps; core bounces overcharge a ball
    bool fortress = false;       // Fortress: anything reaching the core blows up around it
    float enemySpeedMul = 1.f;   // Living Core's cost: enemies march faster
    // Alchemist
    bool alchemy = false;        // random extra elements; a ball can react with itself
    // Berserker
    bool bloodlust = false;      // the combo never decays and climbs twice as fast; core hits wipe it
    float coreDamageMul = 1.f;   // Bloodlust's cost: core hits x this
};

}  // namespace sb
