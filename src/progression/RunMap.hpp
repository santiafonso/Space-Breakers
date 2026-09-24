#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "core/Config.hpp"
#include "core/Math.hpp"

namespace sb {

// ---- the path map: one per act ---------------------------------------------
//
// An act is cfg::map::rows rows of branching nodes followed by the boss. Row r
// (1-based) is wave  (act-1)*bossWave + r  whatever the node is, so enemy
// scaling and the boss waves (10 / 20) don't move; a non-combat node just skips
// that row's fight. From the node you're on you may step to any node it links
// to in the next row.

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
        case MapNodeType::Elite:   return "a much harder wave - pays more gold and a pick of 1 of 4";
        case MapNodeType::Shop:    return "spend gold on balls, items, modifiers, relics or repairs";
        case MapNodeType::Forge:   return "level up one item a ball already carries";
        case MapNodeType::Rest:    return "no fight: the core is repaired to full";
        case MapNodeType::Upgrade: return "no fight: a free pick of 1 of 4";
        case MapNodeType::Recruit: return "no fight: a new ball, or a role (Striker / Support / Guardian) for one you have";
        case MapNodeType::Boss:    return "the act's boss";
    }
    return "";
}

namespace detail {

inline MapNodeType rollNodeType(Rng& rng, int row) {
    if (row == 1) return MapNodeType::Combat;              // always open on a fight
    if (row == cfg::map::rows) {                            // last stop before the boss
        const int r = rng.irange(0, 99);
        return r < 55 ? MapNodeType::Rest : (r < 80 ? MapNodeType::Shop : MapNodeType::Forge);
    }
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

// Build an act's map: each row picks 2..lanes lanes, nodes link to the nodes in
// the next row within one lane of theirs, and every node gets at least one way
// in and one way out. All last-row nodes feed the boss.
inline RunMap generateMap(Rng& rng, int act) {
    RunMap m;
    m.act = act;
    const int L = cfg::map::lanes;
    std::vector<std::vector<int>> rowNodes;   // node indices per row (0-based row)

    for (int r = 1; r <= cfg::map::rows; ++r) {
        std::vector<int> lanes(static_cast<std::size_t>(L));
        for (int i = 0; i < L; ++i) lanes[static_cast<std::size_t>(i)] = i;
        for (int i = L - 1; i > 0; --i) std::swap(lanes[static_cast<std::size_t>(i)], lanes[static_cast<std::size_t>(rng.irange(0, i))]);
        const int count = rng.irange(cfg::map::minPerRow, L);
        lanes.resize(static_cast<std::size_t>(count));
        std::sort(lanes.begin(), lanes.end());

        std::vector<int> ids;
        for (int lane : lanes) {
            MapNode n;
            n.row = r;
            n.lane = lane;
            n.type = detail::rollNodeType(rng, r);
            ids.push_back(static_cast<int>(m.nodes.size()));
            m.nodes.push_back(n);
        }
        rowNodes.push_back(ids);
    }
    MapNode boss;
    boss.type = MapNodeType::Boss;
    boss.row = cfg::map::rows + 1;
    boss.lane = -1;   // drawn centred
    const int bossId = static_cast<int>(m.nodes.size());
    m.nodes.push_back(boss);

    auto link = [&](int a, int b) {
        auto& nx = m.nodes[static_cast<std::size_t>(a)].next;
        if (std::find(nx.begin(), nx.end(), b) == nx.end()) nx.push_back(b);
    };
    for (std::size_t r = 0; r + 1 < rowNodes.size(); ++r) {
        const auto& cur = rowNodes[r];
        const auto& nxt = rowNodes[r + 1];
        auto nearest = [&](int from, const std::vector<int>& pool) {
            int best = pool.front(), bd = 1 << 20;
            for (int id : pool) {
                const int d = std::abs(m.nodes[static_cast<std::size_t>(id)].lane -
                                       m.nodes[static_cast<std::size_t>(from)].lane);
                if (d < bd) { bd = d; best = id; }
            }
            return best;
        };
        for (int a : cur)
            for (int b : nxt)
                if (std::abs(m.nodes[static_cast<std::size_t>(a)].lane -
                             m.nodes[static_cast<std::size_t>(b)].lane) <= 1)
                    link(a, b);
        for (int a : cur)   // a way out for everyone
            if (m.nodes[static_cast<std::size_t>(a)].next.empty()) link(a, nearest(a, nxt));
        for (int b : nxt) {  // a way in for everyone
            bool reached = false;
            for (int a : cur)
                for (int t : m.nodes[static_cast<std::size_t>(a)].next)
                    if (t == b) reached = true;
            if (!reached) link(nearest(b, cur), b);
        }
    }
    for (int a : rowNodes.back()) link(a, bossId);
    for (auto& n : m.nodes) std::sort(n.next.begin(), n.next.end());
    return m;
}

}  // namespace sb
