#pragma once

#include <cmath>
#include <cstdint>

namespace sb {

// The permanent progression track. Each upgrade visibly changes how the ball
// (or the arena) behaves, and the shop stays a glance rather than a spreadsheet.
enum Upgrade {
    UpgSpeed,
    UpgPoints,
    UpgMultiball,
    UpgWalls,
    UpgCombo,
    UpgLuck,
    UpgradeCount
};

struct UpgradeDef {
    const char* name;
    const char* effect;
    std::uint32_t baseCost;
    float growth;
    int maxLevel;  // -1 == uncapped
};

inline const UpgradeDef& upgradeDef(int u) {
    static const UpgradeDef defs[UpgradeCount] = {
        {"Speed", "+16% cruise speed", 15u, 1.55f, -1},
        {"Points", "+1 point per bounce", 20u, 1.70f, -1},
        {"Multiball", "one more ball in play", 90u, 2.35f, 5},
        {"Walls", "one more movable wall", 300u, 2.55f, 6},
        {"Combo Hold", "combo lasts longer, climbs higher", 120u, 2.10f, 4},
        {"Lucky Orbs", "power-ups appear sooner", 150u, 2.10f, 4},
    };
    return defs[u];
}

inline bool upgradeMaxed(int u, int level) {
    const int cap = upgradeDef(u).maxLevel;
    return cap >= 0 && level >= cap;
}

// Returns 0 when the upgrade is maxed (callers should check upgradeMaxed first).
inline std::uint32_t upgradeCost(int u, int level) {
    if (upgradeMaxed(u, level)) return 0u;
    const UpgradeDef& d = upgradeDef(u);
    return static_cast<std::uint32_t>(
        std::llround(d.baseCost * std::pow(d.growth, static_cast<float>(level))));
}

}  // namespace sb
