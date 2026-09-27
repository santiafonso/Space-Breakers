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
bool foldAssassin(UpgradeKind k, int level, BallMods& m) {
    namespace A = cfg::assassin;
    const float l = static_cast<float>(level - 1);
    AssassinMods& a = m.cls.assassin;
    switch (k) {
        case UpgradeKind::Backstab: a.backstab = A::backstab + A::backstabPerLevel * l; return true;
        case UpgradeKind::Cull: a.cull = A::cull + A::cullPerLevel * l; return true;
        case UpgradeKind::KillingSpree:
            a.spreePer = A::spree + A::spreePerLevel * l;
            a.spreeMax = A::spreeMax + level - 1;
            return true;
        case UpgradeKind::ShadowTrail: a.trailFrac = A::trail + A::trailPerLevel * l; return true;
        case UpgradeKind::SmokeBomb:
            a.smokeFrac = A::smoke + A::smokePerLevel * l;
            a.smokeRadius = A::smokeRadius + A::smokeRadiusPerLevel * l;
            return true;
        case UpgradeKind::Phantom: a.phantomLife = A::phantomLife + A::phantomLifePerLevel * l; return true;
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
