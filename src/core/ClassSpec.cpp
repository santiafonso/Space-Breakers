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
bool foldShooter(UpgradeKind k, int level, BallMods& m) {
    namespace S = cfg::shooter;
    ShooterMods& s = m.cls.shooter;
    const float n = static_cast<float>(level - 1);   // levels past the first
    switch (k) {
        case UpgradeKind::RapidFire: s.rate *= S::rapidRate + S::rapidRatePerLevel * n; return true;
        case UpgradeKind::Scattershot:
            s.pellets = S::scatterPellets + (level >= 3 ? 1 : 0) + (level >= 5 ? 1 : 0);
            s.pelletFrac = S::scatterFrac + S::scatterFracPerLevel * n;
            return true;
        case UpgradeKind::Rebound:
            s.hops += S::reboundHops + (level - 1);
            s.hopKeep = S::reboundKeep + S::reboundKeepPerLevel * n;
            return true;
        case UpgradeKind::Tracer:
            s.tracerChance = S::tracerChance + S::tracerChancePerLevel * n;
            s.dmgMul *= 1.f + S::tracerFracPerLevel * n;
            return true;
        case UpgradeKind::DrillRounds:
            s.pierce = S::drillPierce + (level - 1);
            s.dmgMul *= S::drillFrac + S::drillFracPerLevel * n;
            return true;
        case UpgradeKind::HairTrigger:
            s.burst = S::triggerBurst + (level - 1);
            s.burstFrac = S::triggerFrac + S::triggerFracPerLevel * n;
            return true;
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
