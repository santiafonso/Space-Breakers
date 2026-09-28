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
        case UpgradeKind::ArcaneMissile:   // "Barrage"
            g.barrage = 1 + (level >= 3 ? 1 : 0) + (level >= 5 ? 1 : 0);
            g.barrageFrac = M::barrageFrac + M::barrageFracPerLevel * n;
            g.missileMul = 1.f + M::barrageMissilePerLevel * n;
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

// ==================================================================== Slinger
bool foldSlinger(UpgradeKind k, int level, BallMods& m) {
    namespace S = cfg::slinger;
    const float n = static_cast<float>(level - 1);
    SlingerMods& s = m.cls.slinger;
    // Its items act on any ball you throw: the hooks run with one item too
    // (the role's own catch / throw bonus still needs the class).
    if (itemTag(k) == ItemTag::Slinger) m.cls.loose |= roleBit(BallRole::Slinger);
    switch (k) {
        case UpgradeKind::Coil:
            s.coil = S::coilThrow + S::coilThrowPerLevel * n;
            s.coilDrag = std::max(0.15f, S::coilDrag + S::coilDragPerLevel * n);
            return true;
        case UpgradeKind::CatchRelease:
            s.releasePer = S::releasePer + S::releasePerPerLevel * n;
            s.releaseMax = S::releaseMax;
            return true;
        case UpgradeKind::Afterburner:
            s.burnFrac = S::burnFrac + S::burnFracPerLevel * n;
            s.burnTime = S::burnTime + S::burnTimePerLevel * n;
            return true;
        case UpgradeKind::Momentum:   s.momentum = S::momentum + S::momentumPerLevel * n; return true;
        case UpgradeKind::Grip:
            s.gripTurn = S::gripTurn + S::gripTurnPerLevel * n;
            s.gripRange = S::gripRange + S::gripRangePerLevel * n;
            return true;
        case UpgradeKind::Ambush:     s.ambush = S::ambush + S::ambushPerLevel * n; return true;
        case UpgradeKind::TrickShot:  s.trick = S::trick + S::trickPerLevel * n; return true;
        case UpgradeKind::DoubleDown: s.doubleDown = S::doubleWin + S::doubleWinPerLevel * n; return true;
        case UpgradeKind::ExecutionThrow: s.execution = S::execution + S::executionPerLevel * n; return true;
        default: return false;
    }
}

// ==================================================================== speed items
bool foldStyle(UpgradeKind k, int level, BallMods& m) {
    namespace S = cfg::style;
    const float n = static_cast<float>(level - 1);
    ClassMods& c = m.cls;
    switch (k) {
        case UpgradeKind::Anchor:
            c.guardian.anchor = std::min(0.9f, S::anchor + S::anchorPerLevel * n);
            c.guardian.anchorRadius = S::anchorRadius + S::anchorRadiusPerLevel * n;
            c.guardian.anchorPull = S::anchorPull + S::anchorPullPerLevel * n;
            return true;
        case UpgradeKind::Plow:
            c.guardian.plow = S::plow + S::plowPerLevel * n;
            c.guardian.plowFrac = S::plowFrac + S::plowFracPerLevel * n;
            return true;
        case UpgradeKind::Slug:     c.shooter.slug = S::slug + S::slugPerLevel * n; return true;
        case UpgradeKind::Strafe:   c.shooter.strafe = S::strafe + S::strafePerLevel * n; return true;
        case UpgradeKind::Lurk:     c.assassin.lurk = S::lurk + S::lurkPerLevel * n; return true;
        case UpgradeKind::Blur:     c.assassin.blur = S::blur + S::blurPerLevel * n; return true;
        case UpgradeKind::Sleight:
            c.jester.sleight = S::sleight + S::sleightPerLevel * n;
            c.jester.sleightEvery = std::max(1.2f, S::sleightEvery + S::sleightEveryPerLevel * n);
            c.loose |= roleBit(BallRole::Jester);   // like every Jester item: works on any ball
            return true;
        case UpgradeKind::Meditate: c.mage.meditate = S::meditate + S::meditatePerLevel * n; return true;
        case UpgradeKind::Leyline:  c.mage.leyline = S::leyline + S::leylinePerLevel * n; return true;
        case UpgradeKind::Beacon:
            c.support.beacon = S::beacon + S::beaconPerLevel * n;
            c.support.beaconRadius = S::beaconRadius + S::beaconRadiusPerLevel * n;
            return true;
        case UpgradeKind::Wake:     c.support.wake = S::wake + S::wakePerLevel * n; return true;
        case UpgradeKind::Pass:     c.support.pass = S::pass + S::passPerLevel * n; return true;
        case UpgradeKind::Kennel:
            c.summoner.kennel = std::max(1.2f, S::kennel + S::kennelPerLevel * n);
            c.summoner.kennelFrac = S::kennelFrac + S::kennelFracPerLevel * n;
            return true;
        case UpgradeKind::DropTurret: c.summoner.drop = S::drop + S::dropPerLevel * n; return true;
        default: return false;
    }
}

}  // namespace

bool foldClassItem(UpgradeKind k, int level, BallMods& m) {
    return foldMage(k, level, m) || foldShooter(k, level, m) || foldAssassin(k, level, m) ||
           foldSummoner(k, level, m) || foldJester(k, level, m) || foldSlinger(k, level, m) || foldStyle(k, level, m);
}

}  // namespace sb
