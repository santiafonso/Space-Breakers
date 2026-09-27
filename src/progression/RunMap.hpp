#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "core/Config.hpp"
#include "core/Math.hpp"

namespace sb {

// ---- the path map: one per act ---------------------------------------------
//
// An act is cfg::map::rows rows of branching nodes followed by the boss. Each
// row plays as a "difficulty wave" (mapRowWave): the act's 9 normal waves are
// spread over its rows, so enemy scaling and the boss waves (10 / 20) don't
// move however long the map is; a non-combat node just skips that row's fight.
// From the node you're on you may step to any node it links to in the next row.

enum class MapNodeType { Combat, Elite, Shop, Forge, Rest, Upgrade, Recruit, Boss };
inline constexpr int kMapNodeTypeCount = 8;

struct MapNode {
    MapNodeType type = MapNodeType::Combat;
    int row = 1;                // 1..rows, rows+1 = boss
    int lane = 0;               // 0..lanes-1 (vertical position on screen)
    std::vector<int> next;      // node indices in the following row
    bool visited = false;
};

struct RunMap {
    int act = 1;
    std::vector<MapNode> nodes;
    int bossRow() const { return cfg::map::rows + 1; }
};

inline const char* mapNodeName(MapNodeType t) {
    switch (t) {
        case MapNodeType::Combat:  return "Fight";
        case MapNodeType::Elite:   return "Elite fight";
        case MapNodeType::Shop:    return "Shop";
        case MapNodeType::Forge:   return "Forge";
        case MapNodeType::Rest:    return "Rest";
        case MapNodeType::Upgrade: return "Upgrade";
        case MapNodeType::Recruit: return "Recruit";
        case MapNodeType::Boss:    return "Boss";
    }
    return "";
}

inline const char* mapNodeDesc(MapNodeType t) {
    switch (t) {
        case MapNodeType::Combat:  return "a wave of enemies - pays gold";
        case MapNodeType::Elite:   return "a much harder wave - more gold and a pick of 1 of 4";
        case MapNodeType::Shop:    return "spend gold on balls, items, modifiers, relics or repairs";
        case MapNodeType::Forge:   return "level up one item a ball already carries";
        case MapNodeType::Rest:    return "no fight: the core is repaired to full";
        case MapNodeType::Upgrade: return "no fight: a free pick of 1 of 4";
        case MapNodeType::Recruit: return "no fight: a new ball, or an item from each of three classes to steer a ball's class";
        case MapNodeType::Boss:    return "the act's boss";
    }
    return "";
}

// The difficulty wave map row `row` of act `act` plays as: rows 1..rows spread
// over waves 1..9 of the act, the boss row is wave 10 (20 in act 2).
inline int mapRowWave(int act, int row) {
    const int base = (act - 1) * cfg::run::bossWave;
    if (row > cfg::map::rows) return base + cfg::run::bossWave;
    const int span = cfg::run::bossWave - 1;
    return base + std::clamp((row * span + cfg::map::rows - 1) / cfg::map::rows, 1, span);
}

namespace detail {

inline MapNodeType rollNodeType(Rng& rng, int row) {
    if (row == 1) return MapNodeType::Combat;              // always open on a fight
    struct W { MapNodeType t; int w; };
    const W table[] = {
        {MapNodeType::Combat,  cfg::map::wCombat},
        {MapNodeType::Elite,   row >= 3 ? cfg::map::wElite : 0},
        {MapNodeType::Shop,    row >= 3 ? cfg::map::wShop : 0},
        {MapNodeType::Forge,   row >= 4 ? cfg::map::wForge : 0},
        {MapNodeType::Rest,    row >= 4 ? cfg::map::wRest : 0},
        {MapNodeType::Upgrade, cfg::map::wUpgrade},
        {MapNodeType::Recruit, row >= 2 ? cfg::map::wRecruit : 0},
    };
    int total = 0;
    for (const W& w : table) total += w.w;
    int pick = rng.irange(0, total - 1);
    for (const W& w : table) {
        if (pick < w.w) return w.t;
        pick -= w.w;
    }
    return MapNodeType::Combat;
}

}  // namespace detail

// The last row before the boss is a fixed stop with no fight, one node per lane.
inline constexpr MapNodeType kPreBossRow[] = {MapNodeType::Shop, MapNodeType::Rest,
                                              MapNodeType::Upgrade, MapNodeType::Recruit};
static_assert(sizeof(kPreBossRow) / sizeof(kPreBossRow[0]) == cfg::map::lanes);

// Build an act's map. Row 1 is a single fight (the trunk). Above it a few
// paths climb toward the boss: each path mostly keeps its lane and now and
// then drifts one lane over, splitting off or merging into a neighbour, and
// paths never cross - so a branch you take is a commitment, not a detour.
// Every row between has 2..lanes nodes. The last row is kPreBossRow: each
// node of the row before links to the stops in its lane and the lanes beside
// it, and all of it feeds the boss.
inline RunMap generateMap(Rng& rng, int act) {
    RunMap m;
    m.act = act;
    const int L = cfg::map::lanes;
    const int R = cfg::map::rows;

    // Walk the paths: walkers[k] = path k's lane on the current row.
    std::vector<std::vector<int>> laneRows;   // lanes used per row 2..R-1
    std::vector<std::vector<std::pair<int, int>>> steps;   // lane -> lane moves out of each of those rows
    {
        std::vector<int> walkers;
        const int paths = rng.irange(cfg::map::minPerRow, L);   // 2..4 paths leave the trunk
        std::vector<int> lanes(static_cast<std::size_t>(L));
        for (int i = 0; i < L; ++i) lanes[static_cast<std::size_t>(i)] = i;
        for (int i = L - 1; i > 0; --i)
            std::swap(lanes[static_cast<std::size_t>(i)], lanes[static_cast<std::size_t>(rng.irange(0, i))]);
        walkers.assign(lanes.begin(), lanes.begin() + paths);
        std::sort(walkers.begin(), walkers.end());
        for (int r = 2; r < R; ++r) {
            std::vector<int> row = walkers;
            row.erase(std::unique(row.begin(), row.end()), row.end());
            laneRows.push_back(row);
            if (r + 1 == R) break;
            // Step every walker to the next row without crossing another's
            // step (walkers stay in lane order); keep at least 2 lanes lit.
            std::vector<int> next;
            for (int tries = 0; tries < 30; ++tries) {
                next.clear();
                int floor = 0;   // lane order: no walker may pass the one on its left
                for (int wv : walkers) {
                    const int roll = rng.irange(0, 99);
                    int to = wv + (roll < cfg::map::driftPct / 2 ? -1 : (roll < cfg::map::driftPct ? 1 : 0));
                    to = std::clamp(to, std::max(0, floor), L - 1);
                    next.push_back(to);
                    floor = to;
                }
                std::vector<int> uniq = next;
                uniq.erase(std::unique(uniq.begin(), uniq.end()), uniq.end());
                if (static_cast<int>(uniq.size()) >= cfg::map::minPerRow) break;
            }
            // Now and then a path forks: a new walker leaves from the same
            // node toward a free neighbouring lane (never across another path).
            if (walkers.size() < static_cast<std::size_t>(L) && rng.irange(0, 99) < cfg::map::splitPct) {
                const std::size_t k = static_cast<std::size_t>(rng.irange(0, static_cast<int>(walkers.size()) - 1));
                const int right = next[k] + 1, left = next[k] - 1;
                const bool canR = right < L && (k + 1 >= next.size() || right <= next[k + 1]);
                const bool canL = left >= 0 && (k == 0 || left >= next[k - 1]);
                if (canR && (!canL || rng.irange(0, 1) == 0)) {
                    walkers.insert(walkers.begin() + static_cast<long>(k) + 1, walkers[k]);
                    next.insert(next.begin() + static_cast<long>(k) + 1, right);
                } else if (canL) {
                    walkers.insert(walkers.begin() + static_cast<long>(k), walkers[k]);
                    next.insert(next.begin() + static_cast<long>(k), left);
                }
            }
            // A merged pair may split again later: walkers on one lane keep
            // walking separately.
            std::vector<std::pair<int, int>> mv;
            for (std::size_t k = 0; k < walkers.size(); ++k) mv.emplace_back(walkers[k], next[k]);
            steps.push_back(mv);
            walkers = next;
        }
    }

    std::vector<std::vector<int>> rowNodes;   // node indices per row (0-based row)
    auto addRow = [&](int r, const std::vector<int>& lanes, bool preBoss) {
        std::vector<int> ids;
        for (int lane : lanes) {
            MapNode n;
            n.row = r;
            n.lane = lane;
            n.type = r == 1 ? MapNodeType::Combat
                            : (preBoss ? kPreBossRow[lane] : detail::rollNodeType(rng, r));
            ids.push_back(static_cast<int>(m.nodes.size()));
            m.nodes.push_back(n);
        }
        rowNodes.push_back(ids);
    };
    addRow(1, {-1}, false);   // the trunk, drawn centred
    for (int r = 2; r < R; ++r) addRow(r, laneRows[static_cast<std::size_t>(r - 2)], false);
    std::vector<int> all(static_cast<std::size_t>(L));
    for (int i = 0; i < L; ++i) all[static_cast<std::size_t>(i)] = i;
    addRow(R, all, true);

    MapNode boss;
    boss.type = MapNodeType::Boss;
    boss.row = R + 1;
    boss.lane = -1;   // drawn centred
    const int bossId = static_cast<int>(m.nodes.size());
    m.nodes.push_back(boss);

    auto link = [&](int a, int b) {
        auto& nx = m.nodes[static_cast<std::size_t>(a)].next;
        if (std::find(nx.begin(), nx.end(), b) == nx.end()) nx.push_back(b);
    };
    auto nodeOn = [&](std::size_t row, int lane) {
        for (int id : rowNodes[row])
            if (m.nodes[static_cast<std::size_t>(id)].lane == lane) return id;
        return -1;
    };
    for (int b : rowNodes[1]) link(rowNodes[0][0], b);   // the trunk opens onto every path
    for (std::size_t i = 0; i < steps.size(); ++i)        // rows 2..R-2: the walkers' own steps
        for (const auto& [from, to] : steps[i]) link(nodeOn(i + 1, from), nodeOn(i + 2, to));
    for (int a : rowNodes[rowNodes.size() - 2])           // the pre-boss stops beside you
        for (int b : rowNodes.back())
            if (std::abs(m.nodes[static_cast<std::size_t>(a)].lane - m.nodes[static_cast<std::size_t>(b)].lane) <= 1)
                link(a, b);
    for (int b : rowNodes.back()) {   // a stop no lane runs beside: reached from the nearest
        const auto& prev = rowNodes[rowNodes.size() - 2];
        bool reached = false;
        for (int a : prev)
            for (int t : m.nodes[static_cast<std::size_t>(a)].next) reached = reached || t == b;
        if (reached) continue;
        int best = prev.front();
        for (int a : prev)
            if (std::abs(m.nodes[static_cast<std::size_t>(a)].lane - m.nodes[static_cast<std::size_t>(b)].lane) <
                std::abs(m.nodes[static_cast<std::size_t>(best)].lane - m.nodes[static_cast<std::size_t>(b)].lane))
                best = a;
        link(best, b);
    }
    for (int a : rowNodes.back()) link(a, bossId);
    for (auto& n : m.nodes) std::sort(n.next.begin(), n.next.end());
    return m;
}

}  // namespace sb
