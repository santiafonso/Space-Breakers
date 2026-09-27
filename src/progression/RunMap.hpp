#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "core/Config.hpp"
#include "core/Math.hpp"

namespace sb {

// ---- the path map: one per act ---------------------------------------------
//
// An act is mapRows(act) rows of branching nodes followed by the boss. Each
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

// Choosable rows in an act: act 1 is a shorter climb than act 2.
inline int mapRows(int act) { return act == 1 ? cfg::map::rowsAct1 : cfg::map::rows; }

struct RunMap {
    int act = 1;
    std::vector<MapNode> nodes;
    int bossRow() const { return mapRows(act) + 1; }
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
        case MapNodeType::Combat:  return "a wave of enemies - pays gold and a modifier pick";
        case MapNodeType::Elite:   return "a much harder wave - more gold and an item pick (items only come from elites and shops)";
        case MapNodeType::Shop:    return "buy what's on the shelf: items, abilities, relics, modifiers. Sell one item";
        case MapNodeType::Forge:   return "level up one item a ball already carries";
        case MapNodeType::Rest:    return "no fight: the core is repaired to full";
        case MapNodeType::Upgrade: return "no fight: a free pick of 1 of 4 (no items)";
        case MapNodeType::Recruit: return "no fight: a new ball, or a free pick";
        case MapNodeType::Boss:    return "the act's boss";
    }
    return "";
}

// The difficulty wave map row `row` of act `act` plays as: rows 1..rows spread
// over waves 1..9 of the act, the boss row is wave 10 (20 in act 2).
inline int mapRowWave(int act, int row) {
    const int base = (act - 1) * cfg::run::bossWave;
    const int rows = mapRows(act);
    if (row > rows) return base + cfg::run::bossWave;
    const int span = cfg::run::bossWave - 1;
    return base + std::clamp((row * span + rows - 1) / rows, 1, span);
}

namespace detail {

// A path's leaning (see cfg::map): which of balls or items it tends to offer.
enum class PathLean { Neutral, Recruit, Item };

inline MapNodeType rollNodeType(Rng& rng, int row, int act, PathLean lean) {
    if (row == 1) return MapNodeType::Combat;              // always open on a fight
    namespace M = cfg::map;
    // Recruit stops aren't rolled: generateMap places them on the recruit paths.
    (void)act;
    const int recruit = 0;
    int elite = M::wEliteNeutral, shop = M::wShopNeutral;
    if (lean == PathLean::Recruit) {
        elite = M::wEliteRecruitPath;
        shop = 0;
    } else if (lean == PathLean::Item) {
        elite = M::wEliteItemPath;
        shop = M::wShopItemPath;
    }
    struct W { MapNodeType t; int w; };
    const W table[] = {
        {MapNodeType::Combat,  M::wCombat},
        {MapNodeType::Elite,   row >= 3 ? elite : 0},
        {MapNodeType::Shop,    row >= 3 ? shop : 0},
        {MapNodeType::Forge,   row >= 4 ? M::wForge : 0},
        {MapNodeType::Rest,    row >= 4 ? M::wRest : 0},
        {MapNodeType::Upgrade, M::wUpgrade},
        {MapNodeType::Recruit, row >= 3 ? recruit : 0},
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
// (No Recruit here: a sure ball at the end of every act would undo how
// scarce balls are meant to be.)
inline constexpr MapNodeType kPreBossRow[] = {MapNodeType::Shop, MapNodeType::Rest,
                                              MapNodeType::Upgrade, MapNodeType::Forge};
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
    const int R = mapRows(act);

    // Walk the paths: walkers[k] = path k's lane on the current row.
    std::vector<std::vector<int>> laneRows;   // lanes used per row 2..R-1
    std::vector<std::vector<detail::PathLean>> leanRows;   // each of those nodes' leaning
    std::vector<std::vector<std::pair<int, int>>> recruitTrails;   // (row, lane) of every recruit path's nodes
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
        // Leanings alternate across the paths from a random start, so a map
        // always has both kinds.
        using detail::PathLean;
        std::vector<PathLean> leans;
        const bool firstRecruit = rng.irange(0, 1) == 0;
        for (int k = 0; k < paths; ++k)
            leans.push_back((k % 2 == 0) == firstRecruit ? PathLean::Recruit : PathLean::Item);
        std::vector<std::vector<std::pair<int, int>>> trail(walkers.size());   // each walker's nodes so far
        for (int r = 2; r < R; ++r) {
            std::vector<int> row = walkers;
            row.erase(std::unique(row.begin(), row.end()), row.end());
            laneRows.push_back(row);
            std::vector<PathLean> lr;
            for (int lane : row) {   // walkers sharing a node: neutral unless they agree
                int kinds = 0;
                PathLean l = PathLean::Neutral;
                for (std::size_t k = 0; k < walkers.size(); ++k)
                    if (walkers[k] == lane && (kinds == 0 || leans[k] != l)) { l = leans[k]; ++kinds; }
                lr.push_back(kinds == 1 ? l : PathLean::Neutral);
            }
            leanRows.push_back(lr);
            for (std::size_t k = 0; k < walkers.size(); ++k) trail[k].emplace_back(r, walkers[k]);
            if (r + 1 == R) {
                for (std::size_t k = 0; k < walkers.size(); ++k)
                    if (leans[k] == PathLean::Recruit) recruitTrails.push_back(trail[k]);
                break;
            }
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
                // The fork leans the other way from the path it leaves.
                const PathLean other = leans[k] == PathLean::Recruit ? PathLean::Item : PathLean::Recruit;
                if (canR && (!canL || rng.irange(0, 1) == 0)) {
                    walkers.insert(walkers.begin() + static_cast<long>(k) + 1, walkers[k]);
                    next.insert(next.begin() + static_cast<long>(k) + 1, right);
                    leans.insert(leans.begin() + static_cast<long>(k) + 1, other);
                    trail.insert(trail.begin() + static_cast<long>(k) + 1, trail[k]);
                } else if (canL) {
                    walkers.insert(walkers.begin() + static_cast<long>(k), walkers[k]);
                    next.insert(next.begin() + static_cast<long>(k), left);
                    leans.insert(leans.begin() + static_cast<long>(k), other);
                    trail.insert(trail.begin() + static_cast<long>(k), trail[k]);
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
        for (std::size_t li = 0; li < lanes.size(); ++li) {
            const int lane = lanes[li];
            const detail::PathLean lean = (r >= 2 && r < R) ? leanRows[static_cast<std::size_t>(r - 2)][li]
                                                          : detail::PathLean::Neutral;
            MapNode n;
            n.row = r;
            n.lane = lane;
            n.type = r == 1 ? MapNodeType::Combat
                            : (preBoss ? kPreBossRow[lane] : detail::rollNodeType(rng, r, act, lean));
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

    // Every recruit path gets its Recruit stops (1 in act 1, 2 in act 2) on
    // its own recruit-leaning nodes from row 3 up, spread out; a path whose
    // nodes already hold enough (shared with another recruit path) gets none.
    {
        auto nodeAt = [&](int row, int lane) -> MapNode* {
            for (int id : rowNodes[static_cast<std::size_t>(row - 1)])
                if (m.nodes[static_cast<std::size_t>(id)].lane == lane) return &m.nodes[static_cast<std::size_t>(id)];
            return nullptr;
        };
        const int want = act == 1 ? cfg::map::recruitsPerPathAct1 : cfg::map::recruitsPerPathAct2;
        for (const auto& tr : recruitTrails) {
            std::vector<MapNode*> spots, fallback;   // recruit-leaning nodes; else neutral ones
            int have = 0;
            for (const auto& [row, lane] : tr) {
                MapNode* n = nodeAt(row, lane);
                if (!n) continue;
                if (n->type == MapNodeType::Recruit) ++have;
                else if (row >= 3 && leanRows[static_cast<std::size_t>(row - 2)][static_cast<std::size_t>(
                                         std::find(laneRows[static_cast<std::size_t>(row - 2)].begin(),
                                                   laneRows[static_cast<std::size_t>(row - 2)].end(), lane) -
                                         laneRows[static_cast<std::size_t>(row - 2)].begin())] == detail::PathLean::Recruit)
                    spots.push_back(n);
                else if (row >= 3 && n->type != MapNodeType::Elite && n->type != MapNodeType::Shop)
                    fallback.push_back(n);
            }
            if (spots.empty()) spots = fallback;   // every node shared with an item path: use a neutral one
            for (int k = have; k < want && !spots.empty(); ++k) {
                // one per equal slice of the path, so two don't sit back to back
                const int slices = want - have;
                const int slice = k - have;
                const std::size_t lo = spots.size() * static_cast<std::size_t>(slice) / static_cast<std::size_t>(slices);
                const std::size_t hi = std::max(lo + 1, spots.size() * static_cast<std::size_t>(slice + 1) /
                                                            static_cast<std::size_t>(slices));
                spots[static_cast<std::size_t>(rng.irange(static_cast<int>(lo), static_cast<int>(hi) - 1))]->type =
                    MapNodeType::Recruit;
            }
        }
    }

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
