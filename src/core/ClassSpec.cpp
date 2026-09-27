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
bool foldMage(UpgradeKind k, int /*level*/, BallMods& /*m*/) {
    switch (k) {
        // case UpgradeKind::Example: m.cls.mage.x = cfg::mage::x + cfg::mage::xPerLevel * (level - 1); return true;
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
