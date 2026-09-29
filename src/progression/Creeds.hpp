#pragma once

#include "progression/Offers.hpp"

namespace sb {

// ---- creeds: run-defining global rules --------------------------------------
//
// After the act-1 boss (and, with the "Covenant" web node, at the start of a
// run) you pick one creed out of three. A creed is not a stat bump: it changes
// what the run is ABOUT - throwing a lot, watching the balls work on their own,
// two huge balls, a swarm, a core that fights... Every one has a real cost.
// Creeds live in RunState::creeds (a run holds at most two) and reach the sim as
// WorldParams::creed (see sim/CreedRules.hpp); App folds the rest (damage,
// cruise, luck, ball count) into the usual params.
//
// Order is free to change: creeds are never saved (a run is never resumed).

enum class CreedId {
    HotHands,     // Thrower:   flings much faster and longer; idle balls slow
    Nova,         // Thrower:   an active ring-burst from the core; smaller core
    Hunters,      // Spectator: each ball chases its own prey; no grabbing
    Clockwork,    // Spectator: constant auto-throws; no grabbing
    Pinball,      // Spectator: walls are bumpers; weaker throws
    Duet,         // Few & mighty: two balls absorb the rest; max 2 balls
    Legion,       // Swarm:     +2 balls, clacks spark; less damage
    LivingCore,   // Core:      the core zaps and overcharges balls; faster enemies
    Fortress,     // Core:      a huge core that explodes on contact; no healing
    LoadedDice,   // Gambler:   +12 luck; fight gold is double or nothing
    Alchemy,      // Alchemist: random extra elements, self-reactions; weaker hits
    Bloodlust,    // Berserker: the combo never cools; core hits wipe it
};
inline constexpr int kCreedCount = 12;
inline constexpr int kMaxCreeds = 2;   // one from the run start (Covenant) + one from the boss

enum class CreedArchetype { Thrower, Spectator, FewMighty, Swarm, Core, Gambler, Alchemist, Berserker };

struct CreedDef {
    const char* name;
    CreedArchetype archetype;
    const char* gain;      // what you get
    const char* cost;      // what it takes
    const char* synergy;   // "works well with" - hover help
    int unlockNode;        // MetaUnlock that must be bought for it to be offered, -1 = always
};

inline const char* creedArchetypeName(CreedArchetype a) {
    switch (a) {
        case CreedArchetype::Thrower:   return "THROWER";
        case CreedArchetype::Spectator: return "SPECTATOR";
        case CreedArchetype::FewMighty: return "FEW BUT MIGHTY";
        case CreedArchetype::Swarm:     return "SWARM";
        case CreedArchetype::Core:      return "CORE";
        case CreedArchetype::Gambler:   return "GAMBLER";
        case CreedArchetype::Alchemist: return "ALCHEMY";
        case CreedArchetype::Berserker: return "BERSERKER";
    }
    return "";
}

// How the archetype plays, one line (card subtitle).
inline const char* creedArchetypeHint(CreedArchetype a) {
    switch (a) {
        case CreedArchetype::Thrower:   return "for runs where you throw a lot";
        case CreedArchetype::Spectator: return "for runs where you watch the balls work";
        case CreedArchetype::FewMighty: return "for a few balls, built very high";
        case CreedArchetype::Swarm:     return "for many balls at once";
        case CreedArchetype::Core:      return "for runs built around the core";
        case CreedArchetype::Gambler:   return "for runs that live on luck";
        case CreedArchetype::Alchemist: return "for runs built on element reactions";
        case CreedArchetype::Berserker: return "for runs that never stop hitting";
    }
    return "";
}

inline const CreedDef& creedDef(CreedId id) {
    static const CreedDef defs[kCreedCount] = {
        {"Hot Hands", CreedArchetype::Thrower,
         "flung balls leave your hand x1.7 faster, can fly three times past the usual speed cap and hold their speed far longer",
         "left alone, every ball cruises 35% slower",
         "Striker items, Battering, Reflexes, Strong arm, Ceiling break", -1},
        {"Nova", CreedArchetype::Thrower,
         "SPACE or right-click: every ball bursts out of the core in a ring at triple speed and the core shoves enemies away (7 s cooldown)",
         "the core loses 20% of its max health",
         "Spring core, Magnetic core, Iron Guardian, Piercing", -1},
        {"Hunters", CreedArchetype::Spectator,
         "every ball locks onto its own prey and chases it until it dies, then picks the next one. +20% damage",
         "you can no longer grab or throw balls",
         "Piercing, Cleave, Executioner, Berserk", MetaCreedHunters},
        {"Clockwork", CreedArchetype::Spectator,
         "a ball is flung for you at the enemy nearest the core every 0.6 s, at 2.5x its cruise. +15% damage",
         "you can no longer grab or throw balls",
         "Striker items, Battering, Keen eye, many balls", -1},
        {"Pinball", CreedArchetype::Spectator,
         "walls are bumpers: every wall bounce speeds the ball up, feeds the combo and sparks a blast on enemies near the wall",
         "your throws are 40% weaker",
         "Wall rush, Ricochet, Railgun, Split shot, Berserk", -1},
        {"Duet", CreedArchetype::FewMighty,
         "your two best balls absorb the rest (items become forge levels, modifiers move over), 2 items of a tag give the ascended class, x1.5 damage, 20% bigger",
         "never more than two balls",
         "Forge nodes, Gemini, Giant, any ascended class", -1},
        {"Legion", CreedArchetype::Swarm,
         "two more balls right now, each with a random item. Balls clacking together throw sparks that hurt enemies and feed the combo",
         "every ball deals 25% less damage",
         "Carom, Big ball, Split shot, Gemini", MetaCreedLegion},
        {"Living Core", CreedArchetype::Core,
         "the core zaps the nearest enemy every 0.8 s, and a ball bouncing off the core is overcharged: faster and +60% damage for 2 s",
         "enemies march 15% faster",
         "Magnetic core, Spring core, Guardian items, Mender", -1},
        {"Fortress", CreedArchetype::Core,
         "core max health x1.75, and anything that reaches the core blows up, hurting and shoving everything around it",
         "the core no longer heals before fights, and rests repair only half",
         "Aegis, Regen, Mender, Slow field", -1},
        {"Loaded Dice", CreedArchetype::Gambler,
         "+12 luck: every chance (crits, echoes, zaps, bombs, ghosts) almost doubles and cards roll much rarer",
         "each fight's gold is a coin flip: double or nothing",
         "Keen eye, Echo, Tesla, Bomber, Split shot, Lucky clover", MetaCreedDice},
        {"Alchemy", CreedArchetype::Alchemist,
         "half of all hits leave a random extra element, and a ball can react with itself - reactions everywhere",
         "contact hits deal 25% less damage",
         "Catalyst, Chain reaction, Primed, Contagion, Storm", MetaCreedAlchemy},
        {"Bloodlust", CreedArchetype::Berserker,
         "the damage combo never cools down on its own and climbs twice as fast",
         "anything reaching the core wipes the combo and hits the core 50% harder",
         "Overcharge, Golden bounce, fast balls, Guardian items", -1},
    };
    return defs[static_cast<int>(id)];
}

// Creeds that can't sit together in one run.
inline bool creedsConflict(CreedId a, CreedId b) {
    auto pair = [&](CreedId x, CreedId y) { return (a == x && b == y) || (a == y && b == x); };
    auto noGrab = [](CreedId p) { return p == CreedId::Hunters || p == CreedId::Clockwork; };
    if (a == b) return true;
    if (pair(CreedId::Duet, CreedId::Legion)) return true;          // two balls vs a swarm
    if ((a == CreedId::HotHands && noGrab(b)) || (b == CreedId::HotHands && noGrab(a))) return true;  // nothing to throw
    if (noGrab(a) && noGrab(b)) return true;                     // one hands-off rule is enough
    return false;
}

}  // namespace sb
