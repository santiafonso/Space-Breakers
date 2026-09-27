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
bool foldJester(UpgradeKind k, int level, BallMods& m) {
    namespace J = cfg::jester;
    const float n = static_cast<float>(level - 1);
    JesterMods& j = m.cls.jester;
    // Its items work on any ball: the hooks run with one Jester item too
    // (the role's own roll still needs the class).
    if (itemTag(k) == ItemTag::Jester) m.cls.loose |= roleBit(BallRole::Jester);
    switch (k) {
        case UpgradeKind::LuckyCharm: return true;   // its luck is counted in App::luck
        case UpgradeKind::CoinFlip:    j.coinHeads = J::coinHeads + J::coinHeadsPerLevel * n; return true;
        case UpgradeKind::WildCard:
            j.wildChance = J::wildChance + J::wildChancePerLevel * n;
            j.wildPower = J::wildPower + J::wildPowerPerLevel * n;
            return true;
        case UpgradeKind::Reroll:      j.reroll = J::rerollChance + J::rerollChancePerLevel * n; return true;
        case UpgradeKind::ChaosBounce: j.chaosHit = J::chaosHit + J::chaosHitPerLevel * n; return true;
        case UpgradeKind::Jackpot:
            j.jackpotChance = J::jackpotChance + J::jackpotChancePerLevel * n;
            j.jackpotBlast = J::jackpotBlast + J::jackpotBlastPerLevel * n;
            j.jackpotGold = J::jackpotGold + J::jackpotGoldPerLevel * (level - 1);
            return true;
        default: return false;
    }
}

}  // namespace

bool foldClassItem(UpgradeKind k, int level, BallMods& m) {
    return foldMage(k, level, m) || foldShooter(k, level, m) || foldAssassin(k, level, m) ||
           foldSummoner(k, level, m) || foldJester(k, level, m);
}

}  // namespace sb
