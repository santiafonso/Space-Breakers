// The newer classes' items -> the ball's numbers (see ClassSpec.hpp). One
// section per class: a switch over its own items that fills b.mods.cls.<class>
// (sim/Classes.hpp) from the item's level, with tuning from cfg::<class>
// (core/ConfigClasses.hpp). Value at level 1 + perLevel * (level - 1), like the
// older items in App::ballSpec.

#include "core/ClassSpec.hpp"

#include "core/Config.hpp"

namespace sb {

namespace {

// ==================================================================== Mage
bool foldMage(UpgradeKind k, int level, BallMods& m) {
    namespace M = cfg::mage;
    const float n = static_cast<float>(level - 1);   // levels past the first
    MageMods& g = m.cls.mage;
    switch (k) {
        case UpgradeKind::Focus: g.focus = M::focus + M::focusPerLevel * n; return true;
        case UpgradeKind::ArcaneMissile:
            g.missileFrac = M::missileFrac + M::missileFracPerLevel * n;
            g.missileEvery = M::missileEvery + M::missileEveryPerLevel * n;
            g.missileTargets = level >= 5 ? 3 : level >= 3 ? 2 : 1;
            return true;
        case UpgradeKind::Attunement: g.power = M::power + M::powerPerLevel * n; return true;
        case UpgradeKind::Twincast: g.twincast = M::twincast + M::twincastPerLevel * n; return true;
        case UpgradeKind::ManaSpring: g.manaSpring = M::manaSpring + M::manaSpringPerLevel * n; return true;
        default: return false;
    }
}

// ==================================================================== Shooter
bool foldShooter(UpgradeKind k, int /*level*/, BallMods& /*m*/) {
    switch (k) {
        default: return false;
    }
}

// ==================================================================== Assassin
bool foldAssassin(UpgradeKind k, int /*level*/, BallMods& /*m*/) {
    switch (k) {
        default: return false;
    }
}

// ==================================================================== Summoner
bool foldSummoner(UpgradeKind k, int /*level*/, BallMods& /*m*/) {
    switch (k) {
        default: return false;
    }
}

// ==================================================================== Jester
bool foldJester(UpgradeKind k, int /*level*/, BallMods& /*m*/) {
    switch (k) {
        default: return false;
    }
}

}  // namespace

bool foldClassItem(UpgradeKind k, int level, BallMods& m) {
    return foldMage(k, level, m) || foldShooter(k, level, m) || foldAssassin(k, level, m) ||
           foldSummoner(k, level, m) || foldJester(k, level, m);
}

}  // namespace sb
