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
bool foldSummoner(UpgradeKind k, int level, BallMods& m) {
    namespace S = cfg::summoner;
    SummonerMods& s = m.cls.summoner;
    const float l = static_cast<float>(level - 1);
    const int extra = (level >= 3 ? 1 : 0) + (level >= 5 ? 1 : 0);   // +1 at Lv3 and Lv5
    switch (k) {
        case UpgradeKind::SummonTurret:
            s.turretLife = S::turretLife + S::turretLifePerLevel * l;
            s.turretRate = S::turretRate + S::turretRatePerLevel * l;
            s.turretFrac = S::turretFrac + S::turretFracPerLevel * l;
            s.turretMax = S::turretMax + extra;
            return true;
        case UpgradeKind::SummonWisps:
            s.wisps = S::wispCount + extra;
            s.wispFrac = S::wispFrac + S::wispFracPerLevel * l;
            return true;
        case UpgradeKind::SummonTotem:
            s.totemInterval = S::totemInterval + S::totemIntervalPerLevel * l;
            s.totemLife = S::totemLife + S::totemLifePerLevel * l;
            s.totemRadius = S::totemRadius + S::totemRadiusPerLevel * l;
            s.totemSlow = S::totemSlow + S::totemSlowPerLevel * l;
            return true;
        case UpgradeKind::SummonWarden:
            s.wardens = std::min(S::wardenCount + extra, S::maxWardens);
            s.wardenFrac = S::wardenFrac + S::wardenFracPerLevel * l;
            return true;
        case UpgradeKind::SummonDragon:
            s.dragonInterval = S::dragonInterval + S::dragonIntervalPerLevel * l;
            s.dragonFrac = S::dragonFrac + S::dragonFracPerLevel * l;
            s.dragonCone = S::dragonCone + S::dragonConePerLevel * l;
            return true;
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
