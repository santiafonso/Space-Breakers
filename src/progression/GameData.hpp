#pragma once

#include <cstdint>
#include <vector>

#include "core/Config.hpp"
#include "platform/SoundSettings.hpp"
#include "progression/Offers.hpp"
#include "progression/Creeds.hpp"
#include "progression/Pacts.hpp"
#include "progression/RunMap.hpp"

namespace sb {

// Lifetime records shown on the Stats screen. Part of the persistent meta.
struct Stats {
    std::uint64_t enemiesKilled = 0;
    std::uint64_t coresEarned = 0;
    std::uint32_t bestWave = 0;
    std::uint32_t bestCombo = 0;   // longest damage-combo streak
    std::uint32_t bestScore = 0;   // highest run score
    std::uint32_t runs = 0;
    std::uint32_t wins = 0;        // runs that cleared the final wave
    float maxSpeed = 0.f;          // px/s
    double timePlayed = 0.0;       // seconds
};

// Everything that survives between sessions: the meta currency, permanent
// unlocks and settings.
struct MetaState {
    std::uint32_t cores = 0;
    std::uint32_t prisms = 0;   // special currency from beating the miniboss
    int unlock[MetaUnlockCount] = {};
    bool soundOn = true;
    SoundSettings sound;      // volumes + a style per sound category (the Sound screen)
    bool fullscreen = false;
    bool hardMode = false;    // the next run starts in hard mode (cfg::hard)
    Stats stats;
};

// Whole-run relics picked between waves. Ball gear lives on each ball's
// loadout instead (RunState::balls). Reset when a run ends; not persisted - a
// run is a short sprint, so there is no mid-run resume.
struct RunMods {
    bool spring = false;
    bool slowField = false;
    bool strongArm = false;   // flung balls leave the hand harder
    bool contagion = false;   // a poisoned enemy dying re-poisons nearby
    bool primed = false;      // +damage vs enemies under an element effect
    bool catalyst = false;       // reactions harder + wider
    bool chainReaction = false;  // reactions can cascade
    bool luckyClover = false;    // +6 luck
    bool glassCannon = false;    // all damage x1.6, core -30% max HP
    bool magneticCore = false;   // core bounces aim at the nearest enemy
    bool prismCore = false;      // elementless balls leave a random element
    bool phoenix = false;        // the core comes back once per act
    bool timeDilation = false;   // enemies slower
    bool overcharge = false;     // higher combo cap
};

// The current run, in memory only.
struct RunState {
    bool active = false;
    bool hard = false;        // hard mode, fixed at the run start
    int wave = 0;
    float coreHp = cfg::core::baseHp;
    float coreMaxHp = cfg::core::baseHp;
    int score = 0;             // arcade points, +100 per kill (x2 under DOUBLE POINTS)
    int rerollsLeft = 0;       // "Foresight" web node: item-reroll charges remaining this run
    float bountyCores = 0.f;   // cores accrued from the "Fortune" node this run
    std::vector<BallLoadout> balls;   // one per ball in play, same order as World::balls()
    RunMods mods;

    // Path map. mapNode = node you're standing on (-1 = start of the act: any
    // row-1 node is open). mapRow = row of that node (0 at the act start).
    int gold = 0;
    float goldFrac = 0.f;             // fractional per-kill gold not paid out yet
    RunMap map;
    int mapNode = -1;
    int mapRow = 0;
    bool eliteWave = false;           // the wave in progress came from an Elite node

    // Shop stock at the current Shop node.
    std::vector<int> shopOffers;      // UpgradeKind
    std::vector<bool> shopSold;
    std::vector<char> shopDeal;       // per offer: 0 full price, 1 on sale, 2 prepaid (a revealed mystery box)
    int shopMystery = 0;              // 0 none, 1 on offer, 2 bought (its pick sits in shopOffers)
    int shopRerolls = 0;              // paid rerolls at this shop (each costs more)
    int shopSells = 0;                // items sold at this shop

    // Creeds (Fase O): CreedId values, at most kMaxCreeds.
    std::vector<int> creeds;
    bool hasCreed(CreedId id) const {
        for (int p : creeds)
            if (p == static_cast<int>(id)) return true;
        return false;
    }

    // Pacts (2026-09-28): PactId values, any number. Found at an Altar.
    std::vector<int> pacts;
    bool hasPact(PactId id) const {
        for (int p : pacts)
            if (p == static_cast<int>(id)) return true;
        return false;
    }
    // The hidden Altar path: flawless fights in a row this act (a fight that
    // lets anything reach the core resets it; stops in between don't), and
    // 0 = not earned / 1 = earned, still hidden / 2 = revealed this act.
    int cleanStreak = 0;
    int altarState = 0;
    bool altarReveal = false;         // just revealed: the map plays the "a path opens" animation

    int lastStandLeft = 0;            // "Last stand" web node: once-per-run core save still unused
    bool phoenixUsedAct = false;      // the Phoenix relic already fired this act
    bool repairedThisAct = false;     // "Iron core": a rest / shop repair / repair-skip healed the core this act
};

struct GameData {
    MetaState meta;
    RunState run;
};

}  // namespace sb
