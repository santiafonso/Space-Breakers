#include "core/App.hpp"
#include "render/Backdrop.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "core/ClassSpec.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "platform/Paths.hpp"
#include "platform/Save.hpp"
#include "render/Draw.hpp"
#include "ui/PactScreen.hpp"
#include "ui/Screens.hpp"
#include "ui/SoundScreen.hpp"
#include "ui/UiSound.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

sf::Vector2f kLogical() { return {1280.f, 800.f}; }

std::string upperCase(std::string s) {
    for (char& ch : s) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return s;
}

bool envDevMode() {
    const char* v = std::getenv("SB_DEV");
    return v && *v && *v != '0';
}

int envInt(const char* key, int fallback) {
    const char* v = std::getenv(key);
    if (!v || !*v) return fallback;
    try {
        return std::stoi(v);
    } catch (...) {
        return fallback;
    }
}


// First existing path for an asset given as "music/menu.ogg" etc.: next to the
// binary first (packaged build), then the project-root layouts used when
// running straight from a build tree. Empty string when nothing matches.
std::string findAsset(const std::string& rel) {
    const std::filesystem::path dir = exeDir();
    const std::filesystem::path candidates[] = {
        dir / "assets" / rel, dir / rel, dir / ".." / "assets" / rel,
        std::filesystem::path("assets") / rel, std::filesystem::path("..") / "assets" / rel,
        std::filesystem::path(rel),
    };
    for (const std::filesystem::path& p : candidates)
        if (std::filesystem::exists(p)) return p.string();
    return {};
}

}  // namespace

App::App() : window_(kLogical()), world_(kLogical()) {
    savePath_ = (exeDir() / "saves" / "save.txt").string();

    // Lato Bold for the UI, Lato Black for titles (Widgets picks it for big
    // text); the old Arial is the fallback.
    const std::string body = findAsset("fonts/Lato-Bold.ttf");
    if (!(!body.empty() && font_.loadFromFile(body))) {
        const std::string fallback = findAsset("arial.ttf");
        if (fallback.empty() || !font_.loadFromFile(fallback)) {
            std::cerr << "Space-Breakers: could not load a font from assets/ (run from the project root)\n";
            std::exit(1);
        }
    }
    if (const std::string title = findAsset("fonts/Lato-Black.ttf");
        !title.empty() && titleFont_.loadFromFile(title))
        setTitleFont(&titleFont_);
    if (!audio_.init())
        std::cerr << "Space-Breakers: audio unavailable, continuing without sound\n";
    audio_.loadMusic(findAsset("music/menu.ogg"), findAsset("music/game.ogg"));

    loadGame(savePath_, data_);
    audio_.setEnabled(data_.meta.soundOn);
    audio_.applySettings(data_.meta.sound);
    uisound::attach(&audio_);
    audio_.setTrack(Audio::Track::Menu);
    window_.applyVideoMode(data_.meta.fullscreen);

    effects_.init(font_, size());
    hud_.init(font_, size());
    autosaveTimer_ = cfg::app::autosaveInterval;

    // Idle world so the renderer always has a valid core to draw.
    world_.startRun(params(), {}, cfg::core::baseHp, cfg::core::baseHp);
    replaceStack(ScreenId::Menu);
}

WorldParams App::params() const {
    const RunMods& m = data_.run.mods;
    const int* u = data_.meta.unlock;
    WorldParams p;
    p.damageMult = 1.f + cfg::combat::heftPerLevel * static_cast<float>(u[MetaHeft]);
    p.wave = std::max(1, data_.run.wave);
    p.ballRadiusMult = 1.f + cfg::combat::massPerLevel * static_cast<float>(u[MetaMass]);
    p.coreBounceBoost = m.spring ? cfg::combat::springBoost : 1.f;
    p.slowField = m.slowField;
    p.contagion = m.contagion;
    p.primed = m.primed;
    p.catalyst = m.catalyst;
    p.chainReaction = m.chainReaction;
    p.magneticCore = m.magneticCore;
    p.luck = 1.f + cfg::luck::chancePerPoint * static_cast<float>(luck());
    p.prismCore = m.prismCore;
    p.timeDilation = m.timeDilation;
    p.overcharge = m.overcharge;
    if (m.glassCannon) p.damageMult *= cfg::synergy::glassDamage;

    // Meta web (Fase A).
    p.emberLevel = u[MetaEmber];
    p.aegisHits = u[MetaAegis];
    p.coreRegenPerSec = cfg::core::regenPerLevel * static_cast<float>(u[MetaRegen]);
    p.stockpile = u[MetaStockpile] > 0;
    p.magnetPickups = u[MetaMagnet] > 0;
    p.afterglowLevel = u[MetaAfterglow];
    p.chargedFrac = cfg::powerup::chargedFracPerLevel * static_cast<float>(u[MetaCharged]);

    p.powerUpMask = powerUpMask();
    p.pickupSpawnMult = std::max(0.15f, 1.f - 0.25f * static_cast<float>(u[MetaUplink]));
    p.pickupDurMult = 0.5f + 0.35f * static_cast<float>(u[MetaCapacitor]);
    // Class routes (2026-09-27).
    p.cruiseMult *= 1.f + cfg::meta::velocityPerLevel * static_cast<float>(u[MetaVelocity]);   // "Velocity"
    p.markMul = cfg::role::markDamageMul + cfg::meta::rallyPerLevel * static_cast<float>(u[MetaRally]);   // "Rally"

    // Per-element potency: web level 1 unlocks the element, levels past that
    // raise elemMult. Index 1..6 = Fire..Electric (see enum Element).
    static const int kElemNode[kElementItemCount] = {MetaFireItem, MetaVenom, MetaTide,
                                                     MetaFrost, MetaQuarry, MetaArc};
    for (int i = 0; i < kElementItemCount; ++i) {
        const int lvl = u[kElemNode[i]];
        p.elemMult[i + 1] = 1.f + cfg::element::powerPerLevel * static_cast<float>(std::max(0, lvl - 1));
    }
    foldPacts(p);   // Fase O: the run's pacts
    return p;
}

// Fold one ball's items + modifiers into the numbers the sim uses. Every item
// scales with its level (1..kMaxItemLevel, raised by duplicates and the forge):
// value = level-1 value + perLevel * (level - 1). Every level past the first
// also hardens the ball a little (itemLevelDamage).
BallSpec App::ballSpec(const BallLoadout& L) const {
    namespace C = cfg::combat;
    namespace S = cfg::synergy;
    namespace G = cfg::changer;
    BallSpec s;
    s.roles = L.roleMask();   // from its item tags: 2 = the class, 4 = ascended
    s.primary = tagRole(L.leadTag());
    if (const ItemTag asc = L.ascended(); asc != ItemTag::None) s.ascended = roleBit(tagRole(asc));
    if (hasPact(PactId::Duet)) s.ascended = s.roles;   // "Duet": every class it has is ascended
    s.element = L.element();
    BallMods& m = s.mods;
    // The type slot: the element's own level makes it stronger.
    if (L.type >= 0)
        m.elemMult = 1.f + C::elemPerLevel * static_cast<float>(std::clamp(L.typeLvl, 1, kMaxItemLevel) - 1);
    // Ability slots: only the open ones (abilitySlotCount) are live.
    for (int i = 0; i < std::min(abilitySlotCount(L), kMaxAbilitySlots); ++i)
        if (L.ability[i] >= 0)
            s.abilities[i] = {abilityOf(static_cast<UpgradeKind>(L.ability[i])), std::clamp(L.abilityLvl[i], 1, kMaxItemLevel)};
    auto stacks = [&L](UpgradeKind k) { return static_cast<float>(L.mods[modifierIndex(k)]); };
    m.damageMult = 1.f + C::heavyImpactPerStack * stacks(UpgradeKind::HeavyImpact);
    m.radiusMult = std::min(1.f + C::bigBallPerStack * stacks(UpgradeKind::BigBall), C::bigBallMaxMult);
    m.knockMult = 1.f + C::bigBallKnockPerStack * stacks(UpgradeKind::BigBall);
    m.cruiseMult = 1.f + C::swiftPerStack * stacks(UpgradeKind::Swift);
    m.maxSpeedMult = 1.f + C::swiftTopPerStack * stacks(UpgradeKind::Swift);
    m.flingDecay = std::pow(C::swiftFlingPerStack, stacks(UpgradeKind::Swift));

    for (int i = 0; i < kBallSlots; ++i) {
        if (L.gear[i] < 0) continue;
        const int lvl = std::clamp(L.gearLvl[i], 1, kMaxItemLevel);
        const float n = static_cast<float>(lvl - 1);   // levels past the first
        m.damageMult *= 1.f + C::itemLevelDamage * n;
        switch (static_cast<UpgradeKind>(L.gear[i])) {
            case UpgradeKind::Ricochet:
                m.ricochetMult = C::ricochetMult + C::ricochetMultPerLevel * n;
                m.wallBoost = C::ricochetBoost + C::ricochetBoostPerLevel * n;
                break;
            case UpgradeKind::Cleave:
                m.cleave = true;
                m.cleaveExec = C::cleaveExecPerLevel * n;
                break;
            case UpgradeKind::Crit:
                m.critChance = C::critChance + C::critChancePerLevel * n;
                m.critMult = C::critMult + C::critMultPerLevel * n;
                break;
            case UpgradeKind::Executioner:
                m.executeThreshold = C::executeThreshold + C::executeThresholdPerLevel * n;
                m.executeMult = C::executeMult + C::executeMultPerLevel * n;
                break;
            case UpgradeKind::Overkill:
                m.overkillFrac = C::overkillFrac + C::overkillFracPerLevel * n;
                m.overkillTargets = 1 + (lvl - 1) / 2;
                break;
            case UpgradeKind::Shatter:   m.shatterMult = C::shatterBonus + C::shatterPerLevel * n; break;
            case UpgradeKind::Conductor: m.conductorJumps = lvl; break;
            case UpgradeKind::Bedrock:   m.bedrockLife = C::bedrockLifeMult + C::bedrockPerLevel * n; break;
            case UpgradeKind::Echo:      m.echoChance = S::echoChance + S::echoPerLevel * n; break;
            case UpgradeKind::Tesla:
                m.teslaChance = S::teslaChance + S::teslaPerLevel * n;
                m.teslaTargets = S::teslaTargets + (lvl - 1);
                break;
            case UpgradeKind::Bomber:
                m.bomberChance = S::bomberChance + S::bomberPerLevel * n;
                m.bombRadius = S::bombRadius * (1.f + S::bombRadiusPerLevel * n);
                break;
            case UpgradeKind::SplitShot: m.splitChance = S::splitChance + S::splitPerLevel * n; break;
            case UpgradeKind::Rampart:
                m.rampartKnock = S::rampartKnock + S::rampartKnockPerLevel * n;
                m.rampartStagger = S::rampartStagger + S::rampartStaggerPerLevel * n;
                break;
            case UpgradeKind::Mender:    m.menderHeal = S::menderHeal + S::menderPerLevel * n; break;
            case UpgradeKind::Hunter:
                m.hunterMult = G::hunterDamage + G::hunterDamagePerLevel * n;
                m.hunterTurn = G::hunterTurn + G::hunterTurnPerLevel * n;
                break;
            case UpgradeKind::Comet:
                m.cometFling = G::cometFling + G::cometFlingPerLevel * n;
                m.cometPlow = G::cometPlow + G::cometPlowPerLevel * n;
                m.maxSpeedMult *= G::cometCap + G::cometCapPerLevel * n;
                m.flingDecay *= G::cometDecay;
                break;
            case UpgradeKind::Mitosis:
                m.mitosis = 1 + (lvl >= 3 ? 1 : 0) + (lvl >= 5 ? 1 : 0);
                m.mitosisLife = G::mitosisLife + G::mitosisLifePerLevel * n;
                break;
            case UpgradeKind::Boomerang:
                m.boomerangHit = G::boomerangHit + G::boomerangHitPerLevel * n;
                m.boomerangKick = G::boomerangKick + G::boomerangKickPerLevel * n;
                break;
            case UpgradeKind::Bumper:
                m.bumperBoost = G::bumperBoost + G::bumperBoostPerLevel * n;
                m.radiusMult *= G::bumperRadius;
                m.knockMult *= G::bumperKnock;
                break;
            case UpgradeKind::Glutton:
                m.gluttonDamage = G::gluttonDamage + G::gluttonDamagePerLevel * n;
                m.gluttonMax = G::gluttonMax + G::gluttonMaxPerLevel * (lvl - 1);
                break;
            case UpgradeKind::Tether:
                m.tetherFrac = G::tetherFrac + G::tetherFracPerLevel * n;
                m.tetherWidth = G::tetherWidth + G::tetherWidthPerLevel * n;
                break;
            case UpgradeKind::BlackHole:
                m.blackHoleChance = G::blackHoleChance + G::blackHolePerLevel * n;
                m.blackHoleFrac = G::blackHoleFrac + G::blackHoleFracPerLevel * n;
                m.blackHolePull = 1.f + 0.2f * n;
                break;
            case UpgradeKind::Resonance:
                m.resonanceFrac = G::resonanceFrac + G::resonanceFracPerLevel * n;
                m.resonanceCd = G::resonanceCooldown + G::resonanceCooldownPerLevel * n;
                break;
            case UpgradeKind::Seeker:
                m.seekerTurn = G::seekerTurn * (1.f + G::seekerPerLevel * n);
                m.seekerRange = G::seekerRange * (1.f + G::seekerPerLevel * n);
                break;
            case UpgradeKind::Piercing:
                m.piercing = true;
                m.pierceMult = 1.f + G::piercePerLevel * n;
                break;
            case UpgradeKind::Railgun:
                m.railFrac = G::railFrac + G::railFracPerLevel * n;
                m.railWidth = G::railWidth + G::railWidthPerLevel * n;
                break;
            case UpgradeKind::Berserk:
                m.berserkPerHit = G::berserkPerHit + G::berserkPerLevel * n;
                m.berserkMax = G::berserkMax + G::berserkMaxPerLevel * (lvl - 1);
                break;
            case UpgradeKind::Giant:   // huge, heavy, a touch slower
                m.radiusMult *= G::giantRadius;
                m.damageMult *= G::giantDamage + G::giantDamagePerLevel * n;
                m.cruiseMult *= G::giantCruise;
                break;
            case UpgradeKind::Satellite:
                m.satellite = true;
                m.satelliteDamage = G::satelliteDamage + G::satellitePerLevel * n;
                break;
            case UpgradeKind::GravityWell: m.gravityMult = 1.f + G::gravityPerLevel * n; break;
            case UpgradeKind::Storm:
                m.stormFrac = G::stormFrac + G::stormFracPerLevel * n;
                m.stormInterval = G::stormInterval / (1.f + 0.2f * n);
                break;
            case UpgradeKind::Gemini: m.twins = 1 + (lvl >= 3 ? 1 : 0) + (lvl >= 5 ? 1 : 0); break;
            case UpgradeKind::Midas:  m.midasGold = G::midasGold * lvl; break;
            default:   // the newer classes' items (core/ClassSpec.cpp)
                foldClassItem(static_cast<UpgradeKind>(L.gear[i]), lvl, m);
                break;
        }
    }

    // The web's class routes (2026-09-27): perks for every ball, then for balls
    // that have the route's class.
    namespace W = cfg::meta;
    const int* u = data_.meta.unlock;
    auto lv = [u](int node) { return static_cast<float>(u[node]); };
    m.critChance += W::keenPerLevel * lv(MetaKeenInstinct);                    // "Keen instinct"
    m.copyLife = 1.f + W::broodPerLevel * lv(MetaBrood);                       // "Brood"
    m.mitosisLife *= m.copyLife;
    m.cls.mage.focus += W::channelPerLevel * lv(MetaChannel);                  // "Channel"
    m.cls.summoner.bond = 1.f + W::bondPerLevel * lv(MetaBond);                // "Bond"
    if (L.hasRole(ItemTag::Striker)) m.damageMult *= 1.f + W::momentumPerLevel * lv(MetaMomentum);   // "Momentum"
    if (L.hasRole(ItemTag::Shooter)) m.cls.shooter.dmgMul *= 1.f + W::caliberPerLevel * lv(MetaCaliber);   // "Caliber"
    if (L.hasRole(ItemTag::Assassin)) m.cls.assassin.cull += W::deathmarkPerLevel * lv(MetaDeathmark);    // "Deathmark"
    if (L.hasRole(ItemTag::Mage)) m.cls.mage.focus += W::archivePerLevel * lv(MetaArchive);             // "Archive"
    if (L.hasRole(ItemTag::Guardian)) m.menderHeal += W::stonewallPerLevel * lv(MetaStonewall);         // "Stonewall"
    return s;
}

std::vector<BallSpec> App::ballSpecs() const {
    std::vector<BallSpec> v;
    for (const BallLoadout& b : data_.run.balls) v.push_back(ballSpec(b));
    return v;
}

void App::syncWorldBalls() {
    grantMageMissiles();
    world_.syncBalls(ballSpecs(), params());
    announceClassGains();
}

void App::rememberClasses() {
    knownClasses_.clear();
    for (const BallLoadout& L : data_.run.balls) knownClasses_.push_back({L.roleMask(), L.ascended()});
}

// "FIRE BALL > STRIKER", "STRIKER > STRIKER + SUPPORT", "ASCENDED: MEGA
// STRIKER". A class is the thing worth chasing, so gaining one is a moment:
// a banner in the class colour over whatever screen is up, a rising chord,
// a soft flash, and the ball flares in its class colour when the fight runs.
// Losing a class (sell / swap) is silent. A ball that's new (recruit, Legion)
// compares against a classless one.
void App::announceClassGains() {
    const auto& balls = data_.run.balls;
    for (std::size_t i = 0; i < balls.size(); ++i) {
        const BallLoadout& L = balls[i];
        const KnownClasses was = i < knownClasses_.size() ? knownClasses_[i] : KnownClasses{};
        const RoleMask now = L.roleMask();
        const ItemTag asc = L.ascended();
        const RoleMask gained = now & ~was.roles;
        const bool ascends = asc != ItemTag::None && asc != was.ascended;
        if (!gained && !ascends) continue;

        std::string caption = "BALL " + std::to_string(i + 1);
        if (ascends) {
            const sf::Color col = tagColor(asc);
            effects_.classBanner(caption + "  -  ASCENDED", "", upperCase(ascendedName(tagRole(asc))), col, true);
            effects_.flash(col, 0.55f);
            audio_.classGain(true);
            world_.pulseClass(static_cast<int>(i), true);
            continue;
        }
        // What it was: its old class(es), or "FIRE BALL" / "BALL" without one.
        auto names = [](RoleMask m) {
            std::string out;
            for (int c = 0; c < kClassCount; ++c) {
                if ((m & roleBit(classAt(c))) == 0) continue;
                if (!out.empty()) out += " + ";
                out += upperCase(roleName(classAt(c)));
            }
            return out;
        };
        ItemTag order[2];
        const int n = L.roles(order);
        std::string to;
        for (int k = 0; k < n; ++k) to += (k ? " + " : "") + upperCase(roleName(tagRole(order[k])));
        std::string from = names(was.roles & now);
        if (from.empty()) {
            const Element el = L.element();
            from = el == Element::Plain ? "BALL" : upperCase(elementName(el)) + " BALL";
            caption += "  -  NEW CLASS";
        } else {
            caption += "  -  SECOND CLASS";
        }
        // the colour of the class it just got
        ItemTag got = n > 0 ? order[0] : ItemTag::None;
        for (int k = 0; k < n; ++k)
            if (gained & roleBit(tagRole(order[k]))) got = order[k];
        const sf::Color col = tagColor(got);
        effects_.classBanner(caption, from, to, col, false);
        effects_.flash(col, 0.4f);
        audio_.classGain(false);
        world_.pulseClass(static_cast<int>(i), false);
    }
    rememberClasses();
}

// A ball that becomes a Mage is handed its signature ability, Magic missile,
// in a free open ability slot (the class opens the 2nd one) - once it has it,
// it's an ordinary ability (it levels, it can be swapped).
void App::grantMageMissiles() {
    const int mm = static_cast<int>(UpgradeKind::AbilityMissile);
    for (BallLoadout& L : data_.run.balls) {
        if (!L.hasRole(ItemTag::Mage) || L.has(UpgradeKind::AbilityMissile)) continue;
        for (int i = 0; i < abilitySlotCount(L); ++i) {
            if (L.ability[i] >= 0) continue;
            L.setSlot(kSlotAbility + i, mm, 1);
            if (data_.run.active)
                effects_.addLabel("+ Magic missile", {size().x * 0.5f, size().y * 0.36f}, theme::classMage, 22, 1.2f);
            break;
        }
    }
}

int App::startBallCount() const { return cfg::run::startBalls; }   // one ball: more come from picks and pacts

float App::startCoreHp() const {
    return cfg::core::baseHp +
           cfg::core::hpPerBulwark * static_cast<float>(data_.meta.unlock[MetaCoreHp]);
}

// Which power-ups can drop: every one is gated by its web node now, so a fresh
// run has none until the Pickups branch is bought into.
unsigned App::powerUpMask() const {
    const int* u = data_.meta.unlock;
    unsigned mask = 0;
    if (u[MetaLedger] > 0)   mask |= 1u << static_cast<int>(PowerUp::Points2x);
    if (u[MetaKinetics] > 0) mask |= 1u << static_cast<int>(PowerUp::Surge);
    if (u[MetaDamper] > 0)   mask |= 1u << static_cast<int>(PowerUp::SlowMo);
    if (u[MetaFacet] > 0)    mask |= 1u << static_cast<int>(PowerUp::Golden);
    if (u[MetaOverload] > 0) mask |= 1u << static_cast<int>(PowerUp::Overdrive);
    return mask;
}


// ---------------------------------------------------------------- screen stack

std::unique_ptr<Screen> App::makeScreen(ScreenId id) {
    switch (id) {
        case ScreenId::Menu:    return std::make_unique<MenuScreen>();
        case ScreenId::Loadout: return std::make_unique<LoadoutScreen>();
        case ScreenId::Play:    return std::make_unique<PlayScreen>();
        case ScreenId::Choice:  return std::make_unique<ChoiceScreen>();
        case ScreenId::Pause:   return std::make_unique<PauseScreen>();
        case ScreenId::Stats:   return std::make_unique<StatsScreen>();
        case ScreenId::HowTo:   return std::make_unique<HowToScreen>();
        case ScreenId::BossWin: return std::make_unique<BossWinScreen>();
        case ScreenId::Map:     return std::make_unique<MapScreen>();
        case ScreenId::Shop:    return std::make_unique<ShopScreen>();
        case ScreenId::Equip:   return std::make_unique<EquipScreen>();
        case ScreenId::Dev:     return std::make_unique<DevScreen>();
        case ScreenId::Pact:    return std::make_unique<PactScreen>();
        case ScreenId::Sound:   return std::make_unique<SoundScreen>();
        case ScreenId::AbilityPick: return std::make_unique<AbilityPickScreen>();
    }
    return std::make_unique<MenuScreen>();
}

void App::replaceStack(ScreenId id) {
    stack_.clear();
    stack_.push_back(makeScreen(id));
    fade_ = 1.f;
    stack_.back()->onEnter(*this);
    stack_.back()->beginIntro();
}

void App::push(ScreenId id) {
    switch (id) {   // a soft cue as it opens: cards being dealt, or a plain panel
        case ScreenId::Choice: case ScreenId::Shop: case ScreenId::Pact: case ScreenId::AbilityPick:
            audio_.cardsDealt();
            break;
        case ScreenId::Play: case ScreenId::Dev: break;
        default: audio_.uiOpen(); break;
    }
    stack_.push_back(makeScreen(id));
    fade_ = 1.f;
    stack_.back()->onEnter(*this);
    stack_.back()->beginIntro();
}

void App::back() {
    if (stack_.size() > 1) {
        stack_.pop_back();
        audio_.uiClose();
    }
    fade_ = 1.f;
    if (!stack_.empty()) stack_.back()->beginIntro();   // replay the intro on the way back
}

bool App::simulating() const {
    return !stack_.empty() && stack_.back()->simulates();
}

// ---------------------------------------------------------------- run flow

void App::openLoadout() { push(ScreenId::Loadout); }

void App::newRun() {
    RunState& r = data_.run;
    r = RunState{};
    r.active = true;
    r.wave = 0;
    r.coreMaxHp = startCoreHp();
    r.coreHp = r.coreMaxHp;
    // Every ball starts Normal (classless); classes come from its items. The
    // run intro picks the first ball's first ability.
    auto startLoadout = [](int n) { return std::vector<BallLoadout>(static_cast<std::size_t>(n)); };
    r.balls = startLoadout(startBallCount());
    ++data_.meta.stats.runs;

    r.rerollsLeft = data_.meta.unlock[MetaReroll] * cfg::run::rerollsPerLevel;
    r.gold = cfg::meta::treasuryGoldPerLevel * data_.meta.unlock[MetaTreasury];   // "Treasury"
    r.lastStandLeft = data_.meta.unlock[MetaLastStand] > 0 ? 1 : 0;               // "Last stand"
    novaCd_ = 0.f;
    introStep_ = -1;

    runBanked_ = false;

    // Dev overrides: SB_BALLS / SB_WAVE / SB_UPGRADES=Name,Name,...
    int startWave = 1;
    if (devMode()) {
        const int nb = envInt("SB_BALLS", 0);
        if (nb > 0) r.balls = startLoadout(std::min(nb, cfg::ball::maxBalls));
        startWave = std::clamp(envInt("SB_WAVE", 1), 1, cfg::run::finalWave);
    }

    // "Continue" past the miniboss is offered only once a run has been won before
    // (or when a dev shortcut drops us past the boss already).
    continueUnlocked_ = data_.meta.stats.wins > 0 || startWave > cfg::run::bossWave;

    // "Starter kit": a free item on the first ball (Uncommon, then Rare). With
    // "Quartermaster" you pick it from 4 cards in the run intro instead.
    if (const int kit = data_.meta.unlock[MetaStarterKit];
        kit > 0 && !r.balls.empty() && data_.meta.unlock[MetaQuartermaster] == 0) {
        const std::vector<UpgradeKind> pool = starterPool(kit >= 2 ? Tier::Rare : Tier::Uncommon);
        if (!pool.empty()) r.balls[0].setSlot(0, static_cast<int>(pool[0]), 1);   // its tier, else the nearest
    }
    world_.startRun(params(), ballSpecs(), r.coreHp, r.coreMaxHp);
    rememberClasses();   // the class-gain watch starts from here (Calling announces its class)
    world_.setPhoenix(r.lastStandLeft);   // no Phoenix until it's picked; "Last stand" is one save per run
    effects_.clear();
    hitstop_ = 0.f;
    camKick_ = 0.f;

    if (devMode()) {
        if (const char* up = std::getenv("SB_UPGRADES")) {
            std::string s(up), tok;
            std::stringstream ss(s);
            while (std::getline(ss, tok, ',')) {
                for (int i = 0; i < kUpgradeKindCount; ++i) {
                    const auto k = static_cast<UpgradeKind>(i);
                    if (tok == upgradeKindId(k)) { applyUpgradeKind(k); break; }
                }
            }
        }
    }

    // The act's path map. A dev SB_WAVE start drops you at the row before it.
    const int act = (startWave - 1) / cfg::run::bossWave + 1;
    r.map = generateMap(rng_, act);
    r.mapNode = -1;
    r.mapRow = 0;   // stand just before the first row that plays as startWave
    while (r.mapRow <= cfg::map::rows && mapRowWave(act, r.mapRow + 1) < startWave) ++r.mapRow;
    r.wave = startWave - 1;
    replaceStack(ScreenId::Play);
    introStep_ = 0;
    advanceRunIntro();   // Covenant pact, Quartermaster pick, then the map
    save();
}

void App::startWaveAt(int wave, bool elite) {
    data_.run.wave = wave;
    data_.run.eliteWave = elite;
    waveIntro_ = cfg::app::waveIntroTime;   // ease the sim in instead of snapping
    if (!hasPact(PactId::Fortress))   // "Fortress" pact: no free healing before a fight
        world_.repairCore(cfg::core::waveHeal +
                          cfg::core::mendPerLevel * static_cast<float>(data_.meta.unlock[MetaMend]));
    if (const int bastion = data_.meta.unlock[MetaBastion]; bastion > 0)   // "Bastion": max HP grows each wave
        world_.addCoreMaxHp(cfg::core::bastionPerWavePerLevel * static_cast<float>(bastion));
    const int w = data_.run.wave;
    if (w == cfg::run::bossWave || w == cfg::run::finalWave) audio_.bossAppear();
    else audio_.waveStart();
    if (w == cfg::run::bossWave)
        world_.startBossWave(params());              // wave 10: Charger miniboss
    else if (w == cfg::run::finalWave)
        world_.startFinalBossWave(params());         // wave 20: Orbital boss + shield ring
    else if (w > cfg::run::bossWave)
        world_.startPostBossWave(w, params(), elite);   // waves 11..19: wide arena, core slides to centre
    else
        world_.startWave(w, params(), elite);
    data_.meta.stats.bestWave =
        std::max(data_.meta.stats.bestWave, static_cast<std::uint32_t>(data_.run.wave));
}

UpgradeCtx App::buildUpgradeCtx() const {
    const RunState& r = data_.run;
    UpgradeCtx c;
    c.balls = &r.balls;
    c.maxBalls = hasPact(PactId::Duet) ? cfg::pact::duetBalls : cfg::ball::maxBalls;   // "Duet": two, ever
    static const int kElemNode[kElementItemCount] = {MetaFireItem, MetaVenom, MetaTide,
                                                     MetaFrost, MetaQuarry, MetaArc};
    for (int i = 0; i < kElementItemCount; ++i)
        c.elemUnlocked[i] = data_.meta.unlock[kElemNode[i]] > 0;
    c.spring = r.mods.spring;
    c.slowField = r.mods.slowField;
    c.strongArm = r.mods.strongArm;
    c.contagion = r.mods.contagion;
    c.primed = r.mods.primed;
    c.catalyst = r.mods.catalyst;
    c.chainReaction = r.mods.chainReaction;
    c.luckyClover = r.mods.luckyClover;
    c.glassCannon = r.mods.glassCannon;
    c.magneticCore = r.mods.magneticCore;
    c.prismCore = r.mods.prismCore;
    c.phoenix = r.mods.phoenix;
    c.timeDilation = r.mods.timeDilation;
    c.overcharge = r.mods.overcharge;
    // Legendaries that still sit behind their web node.
    const int* u = data_.meta.unlock;
    if (u[MetaSatellite] == 0) c.lock(UpgradeKind::Satellite);
    if (u[MetaGravity] == 0)   c.lock(UpgradeKind::GravityWell);
    if (u[MetaGemini] == 0)    c.lock(UpgradeKind::Gemini);
    if (u[MetaPrism] == 0)     c.lock(UpgradeKind::PrismCore);
    // Items of a class that isn't unlocked yet (Striker is always open).
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        if (upgradeCat(k) == UpgradeCat::Item && !classUnlocked(itemTag(k), u)) c.lock(k);
        if (upgradeCat(k) == UpgradeCat::Ability && !abilityUnlocked(k, u)) c.lock(k);   // the Mage route
    }
    return c;
}

int App::luck() const {
    int l = cfg::luck::luckyStarPerLevel * data_.meta.unlock[MetaLuckyStar];
    if (data_.run.mods.luckyClover) l += cfg::luck::cloverPoints;
    if (hasPact(PactId::LoadedDice)) l += cfg::luck::dicePoints;
    // ---- Jester: every "Lucky charm" on every ball adds its points.
    for (const BallLoadout& b : data_.run.balls)
        for (int i = 0; i < kBallSlots; ++i)
            if (b.gear[i] == static_cast<int>(UpgradeKind::LuckyCharm))
                l += cfg::jester::charmPoints(std::clamp(b.gearLvl[i], 1, kMaxItemLevel));
    // web "Fool's luck": some luck always, more for every Jester ball
    if (const int fool = data_.meta.unlock[MetaFoolsLuck]; fool > 0) {
        l += cfg::meta::foolsLuckPerLevel * fool;
        for (const BallLoadout& b : data_.run.balls)
            if (b.hasRole(ItemTag::Jester)) l += cfg::meta::foolsLuckPerLevel * fool;
    }
    return l;
}

UpgradeKind App::rollPick(RollSource src, const std::vector<UpgradeKind>& exclude,
                          bool (*filter)(UpgradeKind)) {
    const UpgradeCtx c = buildUpgradeCtx();
    const int* base = src == RollSource::Elite ? cfg::tier::weightsElite
                    : src == RollSource::Boss  ? cfg::tier::weightsBoss
                                               : cfg::tier::weightsNormal;
    float w[kTierCount];
    for (int t = 0; t < kTierCount; ++t) w[t] = static_cast<float>(base[t]);
    const int* u = data_.meta.unlock;
    w[static_cast<int>(Tier::Epic)] *= 1.f + cfg::meta::armoryEpicPerLevel * static_cast<float>(u[MetaArmory]);
    // Luck nudges every tier's odds one step up.
    const float shift = std::min(cfg::luck::tierShiftPerPoint * static_cast<float>(luck()), cfg::luck::tierShiftCap);
    if (shift > 0.f)
        for (int t = kTierCount - 2; t >= 0; --t) {
            const float moved = w[t] * shift;
            w[t] -= moved;
            w[t + 1] += moved;
        }

    // Eligible picks by tier.
    std::vector<UpgradeKind> byTier[kTierCount];
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        if (!upgradeEligible(k, c) || (filter && !filter(k))) continue;
        if (std::find(exclude.begin(), exclude.end(), k) != exclude.end()) continue;
        byTier[static_cast<int>(upgradeTier(k))].push_back(k);
    }
    float total = 0.f;
    for (int t = 0; t < kTierCount; ++t) total += w[t];
    float roll = rng_.range(0.f, std::max(total, 1e-3f));
    int tier = 0;
    for (; tier < kTierCount - 1; ++tier) {
        if (roll < w[tier]) break;
        roll -= w[tier];
    }
    // That tier empty? Step down, then up, to the nearest tier that has something.
    for (int d = 0; d < kTierCount; ++d) {
        for (int t : {tier - d, tier + d}) {
            if (t < 0 || t >= kTierCount || byTier[t].empty()) continue;
            if (src == RollSource::Boss && t < static_cast<int>(Tier::Rare) && d < kTierCount - 1) continue;
            // "<Class> lore" (web): that class's items weigh more within the tier.
            const auto& v = byTier[t];
            float sum = 0.f;
            std::vector<float> wk(v.size(), 1.f);
            for (std::size_t j = 0; j < v.size(); ++j) {
                if (upgradeCat(v[j]) == UpgradeCat::Item)
                    if (const int lore = classLoreNode(itemTag(v[j])); lore >= 0)
                        wk[j] += cfg::meta::lorePerLevel * static_cast<float>(u[lore]);
                sum += wk[j];
            }
            float pick = rng_.range(0.f, sum);
            for (std::size_t j = 0; j < v.size(); ++j) {
                if (pick < wk[j]) return v[j];
                pick -= wk[j];
            }
            return v.back();
        }
    }
    return UpgradeKind::HeavyImpact;   // always eligible: a modifier fits any ball
}

void App::rollChoices(RollSource src) {
    rollSource_ = src;
    choiceTitle_.clear();
    std::vector<UpgradeKind> taken;
    for (int i = 0; i < kChoiceCount; ++i) {
        choices_[i] = rollPick(src, taken);
        taken.push_back(choices_[i]);
    }
}

// "reroll" button under a Choice card: swap that one card for a different
// eligible item that isn't already on the table. Costs one Foresight charge.
void App::rerollChoice(int idx) {
    if (idx < 0 || idx >= kChoiceCount || data_.run.rerollsLeft <= 0) return;
    const std::vector<UpgradeKind> shown(choices_.begin(), choices_.end());
    const UpgradeKind k = rollPick(rollSource_, shown);   // same odds as the card it replaces
    if (std::find(shown.begin(), shown.end(), k) != shown.end()) return;   // nothing new - keep the charge
    choices_[idx] = k;
    --data_.run.rerollsLeft;
    audio_.purchase();
    effects_.flash(theme::accent, 0.25f);
}

// Recruit node: a new ball plus one item of three different unlocked classes,
// so you can push a ball toward the class you want. With the arena full the
// ball card becomes a random modifier instead.
void App::rollRecruitChoices() {
    const UpgradeCtx c = buildUpgradeCtx();
    choices_[0] = upgradeEligible(UpgradeKind::AddBall, c)
        ? UpgradeKind::AddBall
        : static_cast<UpgradeKind>(static_cast<int>(UpgradeKind::HeavyImpact) + rng_.irange(0, kModifierCount - 1));
    rollSource_ = RollSource::Normal;
    choiceTitle_.clear();
    // One filter per class (rollPick takes a plain function pointer).
    static bool (*const tagFilters[kClassCount])(UpgradeKind) = {
        [](UpgradeKind k) { return itemTag(k) == ItemTag::Striker; },
        [](UpgradeKind k) { return itemTag(k) == ItemTag::Guardian; },
        [](UpgradeKind k) { return itemTag(k) == ItemTag::Support; },
        [](UpgradeKind k) { return itemTag(k) == ItemTag::Mage; },
        [](UpgradeKind k) { return itemTag(k) == ItemTag::Shooter; },
        [](UpgradeKind k) { return itemTag(k) == ItemTag::Assassin; },
        [](UpgradeKind k) { return itemTag(k) == ItemTag::Summoner; },
        [](UpgradeKind k) { return itemTag(k) == ItemTag::Jester; },
    };
    std::vector<int> classes;   // unlocked classes that have an item to offer
    for (int t = 0; t < kClassCount; ++t) {
        bool any = false;
        for (int i = 0; i < kUpgradeKindCount && !any; ++i)
            any = tagFilters[t](static_cast<UpgradeKind>(i)) && upgradeEligible(static_cast<UpgradeKind>(i), c);
        if (any) classes.push_back(t);
    }
    for (int i = static_cast<int>(classes.size()) - 1; i > 0; --i)
        std::swap(classes[static_cast<std::size_t>(i)], classes[static_cast<std::size_t>(rng_.irange(0, i))]);
    std::vector<UpgradeKind> taken{choices_[0]};
    for (int t = 0; t < kChoiceCount - 1; ++t) {
        // A different class per card while there are enough; any pick after that.
        const auto filter = t < static_cast<int>(classes.size()) ? tagFilters[classes[static_cast<std::size_t>(t)]] : nullptr;
        choices_[t + 1] = rollPick(RollSource::Normal, taken, filter);
        taken.push_back(choices_[t + 1]);
    }
}

void App::openChoice(RollSource src) {
    rollChoices(src);
    push(ScreenId::Choice);
}

// First ball the pick fits, preferring one with a free slot (dev tools use
// this; the Choice screen normally asks).
bool App::autoTarget(UpgradeKind k, int& ball, int& slot) const {
    const auto& balls = data_.run.balls;
    ball = slot = -1;
    for (int i = 0; i < static_cast<int>(balls.size()); ++i) {
        if (!upgradeFitsBall(k, balls[i])) continue;
        const int sl = defaultSlot(k, balls[i]);
        const bool free = balls[i].kindAt(sl) < 0;
        if (ball < 0 || free) { ball = i; slot = sl; }
        if (free) break;
    }
    return ball >= 0;
}

void App::applyUpgradeKind(UpgradeKind k, int ball, int slot) {
    RunState& r = data_.run;
    RunMods& m = r.mods;
    // A big pick deserves a moment: flash + banner in its tier colour.
    if (const Tier t = upgradeTier(k); t >= Tier::Epic) {
        effects_.flash(tierColor(t), t == Tier::Legendary ? 0.9f : 0.6f);
        effects_.addLabel(std::string(t == Tier::Legendary ? "LEGENDARY  " : "EPIC  ") + upgradeInfo(k).title,
                          {size().x * 0.5f, size().y * 0.3f}, tierColor(t), t == Tier::Legendary ? 34 : 28, 1.6f);
    }
    if (upgradeNeedsTarget(k)) {
        if (ball < 0 && !autoTarget(k, ball, slot)) return;
        if (ball >= static_cast<int>(r.balls.size()) || !upgradeFitsBall(k, r.balls[ball])) return;
        BallLoadout& b = r.balls[ball];
        if (const int mi = modifierIndex(k); mi >= 0) {
            ++b.mods[mi];
        } else if (upgradeLevelsUp(k, b)) {
            // A duplicate levels up the copy the ball already has.
            const int lvl = b.levelUp(b.slotOf(k));
            audio_.levelUp(lvl);
            effects_.addLabel(std::string(upgradeInfo(k).title) + "  Lv " + std::to_string(lvl),
                              {size().x * 0.5f, size().y * 0.36f}, tierColor(upgradeTier(k)), 26, 1.3f);
        } else {
            // Items go in an item slot, an element in the type slot (swapping
            // the old one), an ability in an open ability slot.
            if (!slotAccepts(k, slot, b)) slot = defaultSlot(k, b);
            b.setSlot(slot, static_cast<int>(k), 1);
        }
        syncWorldBalls();
        return;
    }
    switch (k) {
        case UpgradeKind::AddBall:
            if (static_cast<int>(r.balls.size()) >= cfg::ball::maxBalls) return;
            r.balls.push_back(BallLoadout{});
            if (data_.meta.unlock[MetaMuster] > 0) {   // web "Muster": it joins with a Common item
                const int item = randomItemFor(r.balls.back(), Tier::Common);
                if (item >= 0) r.balls.back().setSlot(0, item, 1);
            }
            syncWorldBalls();
            break;
        case UpgradeKind::CoreSpring:    m.spring = true; break;
        case UpgradeKind::CoreSlowField: m.slowField = true; break;
        case UpgradeKind::StrongArm:     m.strongArm = true; break;
        case UpgradeKind::Contagion:     m.contagion = true; break;
        case UpgradeKind::Primed:        m.primed = true; break;
        case UpgradeKind::Catalyst:      m.catalyst = true; break;
        case UpgradeKind::ChainReaction: m.chainReaction = true; break;
        case UpgradeKind::LuckyClover:   m.luckyClover = true; break;
        case UpgradeKind::MagneticCore:  m.magneticCore = true; break;
        case UpgradeKind::PrismCore:     m.prismCore = true; break;
        case UpgradeKind::TimeDilation:  m.timeDilation = true; break;
        case UpgradeKind::Overcharge:    m.overcharge = true; break;
        case UpgradeKind::Phoenix:       m.phoenix = true; world_.setPhoenix(1 + r.lastStandLeft); break;
        case UpgradeKind::GlassCannon:   // big damage, a smaller core
            m.glassCannon = true;
            world_.addCoreMaxHp(-world_.core().maxHp * (1.f - cfg::synergy::glassCoreHp));
            break;
        default: break;
    }
}

// "Repair core" button on the Item screen: heal to full, but forfeit the pick.
void App::repairCoreSkipItem() {
    playerRepair(hasPact(PactId::Fortress) ? world_.core().maxHp * cfg::pact::fortressRestHeal : 1e9f);
    if (const int prospector = data_.meta.unlock[MetaProspector]; prospector > 0)
        data_.run.rerollsLeft += prospector;   // "Prospector": skipping refunds reroll charges
    audio_.purchase();
    effects_.flash(theme::core, 0.4f);
    back();
    afterChoice();
}

// A repair the player chose (rest, shop, the Choice repair-skip). If it
// actually restores HP it ends this act's "Iron core" streak; the automatic
// heals (before each fight, Regen, Mender, Bastion, Phoenix / Last stand) don't.
void App::playerRepair(float amount) {
    const float before = world_.core().hp;
    world_.repairCore(amount);
    if (world_.core().hp > before + 0.01f) data_.run.repairedThisAct = true;
}

// "Stockpile" web node: fire the reserved power-up (Q during play).
void App::useReserve() {
    if (!world_.hasReserve()) return;
    world_.useReserve(params());
    audio_.pickup();
    effects_.flash(theme::accent, 0.5f);
}

void App::finishChoice() {
    audio_.cardPick();
    effects_.flash(theme::accent, 0.4f);
    back();      // close the Choice
    afterChoice();
}

void App::applyUpgrade(int idx) {
    if (idx < 0 || idx >= kChoiceCount) return;
    if (upgradeNeedsTarget(choices_[idx])) {
        beginEquip(EquipSource::Choice, choices_[idx], idx);
        return;
    }
    applyUpgradeKind(choices_[idx]);
    finishChoice();
}

// ---------------------------------------------------------------- path map

void App::openMap() { push(ScreenId::Map); }

bool App::mapNodeOpen(int node) const {
    const RunState& r = data_.run;
    if (node < 0 || node >= static_cast<int>(r.map.nodes.size())) return false;
    if (r.mapNode < 0) return r.map.nodes[static_cast<std::size_t>(node)].row == r.mapRow + 1;
    const auto& nx = r.map.nodes[static_cast<std::size_t>(r.mapNode)].next;
    return std::find(nx.begin(), nx.end(), node) != nx.end();
}

void App::travelTo(int node) {
    if (!mapNodeOpen(node)) return;
    RunState& r = data_.run;
    MapNode& n = r.map.nodes[static_cast<std::size_t>(node)];
    n.visited = true;
    r.mapNode = node;
    r.mapRow = n.row;
    const int wave = mapRowWave(r.map.act, n.row);
    audio_.travel();
    back();   // close the map: the Play screen is underneath
    const sf::Vector2f mid{size().x * 0.5f, size().y * 0.4f};
    switch (n.type) {
        case MapNodeType::Combat:
        case MapNodeType::Elite:
        case MapNodeType::Boss:
            startWaveAt(wave, n.type == MapNodeType::Elite);
            break;
        case MapNodeType::Rest:
            r.wave = wave;
            playerRepair(hasPact(PactId::Fortress) ? world_.core().maxHp * cfg::pact::fortressRestHeal : 1e9f);
            audio_.purchase();
            effects_.flash(theme::core, 0.5f);
            effects_.addLabel(hasPact(PactId::Fortress) ? "Core half repaired" : "Core repaired", mid, theme::core, 26, 1.2f);
            openMap();
            break;
        case MapNodeType::Upgrade:
            r.wave = wave;
            openChoice();
            break;
        case MapNodeType::Recruit:
            r.wave = wave;
            rollRecruitChoices();
            push(ScreenId::Choice);
            break;
        case MapNodeType::Shop:
            r.wave = wave;
            rollShop();
            push(ScreenId::Shop);
            break;
        case MapNodeType::Forge: {
            r.wave = wave;
            bool any = false;
            for (int b = 0; b < runBallCount() && !any; ++b)
                for (int sl = 0; sl < kLoadoutSlots; ++sl)
                    if (r.balls[static_cast<std::size_t>(b)].kindAt(sl) >= 0 &&
                        r.balls[static_cast<std::size_t>(b)].levelAt(sl) < forgeCap()) any = true;
            if (any) {
                equipSrc_ = EquipSource::Forge;
                equipRef_ = -1;
                push(ScreenId::Equip);
            } else {
                effects_.addLabel("nothing to forge yet", mid, theme::textLo, 22, 1.2f);
                openMap();
            }
            break;
        }
    }
    save();
}

// ---------------------------------------------------------------- equip picker

void App::beginEquip(EquipSource src, UpgradeKind k, int ref) {
    equipSrc_ = src;
    equipKind_ = k;
    equipRef_ = ref;
    push(ScreenId::Equip);
}

bool App::equipFitsSlot(int ball, int slot) const {
    if (ball < 0 || ball >= runBallCount()) return false;
    const BallLoadout& b = data_.run.balls[static_cast<std::size_t>(ball)];
    // The forge levels any filled slot (items, element, abilities); selling
    // takes items only.
    if (equipSrc_ == EquipSource::Forge || equipSrc_ == EquipSource::ShopForge)
        return slot >= 0 && slot < kLoadoutSlots && b.kindAt(slot) >= 0 && b.levelAt(slot) < forgeCap();
    if (equipSrc_ == EquipSource::Sell) return isItemSlot(slot) && b.gear[slot] >= 0;
    return upgradeFitsBall(equipKind_, b);
}

bool App::equipFitsBall(int ball) const {
    if (equipSrc_ == EquipSource::Forge || equipSrc_ == EquipSource::ShopForge || equipSrc_ == EquipSource::Sell) {
        for (int sl = 0; sl < kLoadoutSlots; ++sl)
            if (equipFitsSlot(ball, sl)) return true;
        return false;
    }
    return equipFitsSlot(ball, 0);
}

void App::confirmEquip(int ball, int slot) {
    if (!equipFitsBall(ball)) return;
    RunState& r = data_.run;
    switch (equipSrc_) {
        case EquipSource::Choice:
            back();   // close the picker, back on the Choice
            applyUpgradeKind(equipKind_, ball, slot);
            finishChoice();
            break;
        case EquipSource::Shop: {
            const int price = shopOfferPrice(equipRef_);
            if (r.gold < price || equipRef_ < 0 || equipRef_ >= static_cast<int>(r.shopSold.size())) return;
            r.gold -= price;
            r.shopSold[static_cast<std::size_t>(equipRef_)] = true;
            applyUpgradeKind(equipKind_, ball, slot);
            audio_.purchase();
            effects_.flash(theme::puGolden, 0.35f);
            back();   // back to the shop
            break;
        }
        case EquipSource::Forge: {
            if (!equipFitsSlot(ball, slot))
                for (slot = 0; slot < kLoadoutSlots && !equipFitsSlot(ball, slot); ++slot) {}
            if (!equipFitsSlot(ball, slot)) return;
            audio_.levelUp(r.balls[static_cast<std::size_t>(ball)].levelUp(slot));
            syncWorldBalls();
            effects_.flash(theme::accent, 0.4f);
            back();
            openMap();
            break;
        }
        case EquipSource::ShopForge: {   // the shop's paid forge: back to the shop after
            if (!equipFitsSlot(ball, slot))
                for (slot = 0; slot < kLoadoutSlots && !equipFitsSlot(ball, slot); ++slot) {}
            if (!equipFitsSlot(ball, slot) || r.gold < cfg::gold::forgeServicePrice) return;
            r.gold -= cfg::gold::forgeServicePrice;
            audio_.levelUp(r.balls[static_cast<std::size_t>(ball)].levelUp(slot));
            syncWorldBalls();
            effects_.flash(theme::accent, 0.4f);
            back();
            break;
        }
        case EquipSource::Sell: {        // sell the item under the pointer back to the shop
            if (!equipFitsSlot(ball, slot)) return;
            const int value = sellValue(ball, slot);
            BallLoadout& L = r.balls[static_cast<std::size_t>(ball)];
            const std::string name = upgradeInfo(static_cast<UpgradeKind>(L.gear[slot])).title;
            L.gear[slot] = -1;
            L.gearLvl[slot] = 0;
            r.gold += value;
            syncWorldBalls();
            audio_.purchase();
            effects_.flash(theme::puGolden, 0.35f);
            effects_.addLabel("sold " + name + "  +" + std::to_string(value) + " gold",
                              {size().x * 0.5f, size().y * 0.14f}, theme::puGolden, 20, 1.2f);
            back();
            break;
        }
    }
}

void App::cancelEquip() {
    back();
    if (equipSrc_ == EquipSource::Forge) openMap();   // walking away from the forge
}

// ---------------------------------------------------------------- TAB: move a slot between balls

// Same kind only (item / type / ability). Refused: an empty source, the same
// spot, a ball with no free slot of that kind, a closed ability slot, or a
// result that breaks the pick rules - two copies of one pick on a ball, or
// Conductor / Bedrock without their element (upgradeFitsBall). The type slot
// is one per ball, so a drop on a ball's panel swaps elements.
int App::slotMoveTarget(int fromBall, int fromSlot, int toBall, int toSlot) const {
    const int n = runBallCount();
    if (fromBall < 0 || fromBall >= n || toBall < 0 || toBall >= n) return -1;
    if (fromSlot < 0 || fromSlot >= kLoadoutSlots) return -1;
    const BallLoadout& src = data_.run.balls[static_cast<std::size_t>(fromBall)];
    const BallLoadout& dst = data_.run.balls[static_cast<std::size_t>(toBall)];
    if (src.kindAt(fromSlot) < 0) return -1;
    auto sameKind = [&](int s) {
        if (isItemSlot(fromSlot)) return isItemSlot(s);
        if (fromSlot == kSlotType) return s == kSlotType;
        return isAbilitySlot(s);
    };
    auto isOpen = [&](int s) { return !isAbilitySlot(s) || s - kSlotAbility < abilitySlotCount(dst); };
    if (toSlot < 0) {
        if (toBall == fromBall) return -1;
        for (int s = 0; s < kLoadoutSlots && toSlot < 0; ++s)
            if (sameKind(s) && isOpen(s) && dst.kindAt(s) < 0) toSlot = s;
        if (toSlot < 0 && fromSlot == kSlotType) toSlot = kSlotType;
    }
    if (toSlot < 0 || toSlot >= kLoadoutSlots || !sameKind(toSlot) || !isOpen(toSlot)) return -1;
    if (toBall == fromBall) return toSlot == fromSlot ? -1 : toSlot;   // reordering one ball
    BallLoadout a = src, b = dst;
    const int ka = a.kindAt(fromSlot), kb = b.kindAt(toSlot);
    a.clearSlot(fromSlot);
    b.clearSlot(toSlot);
    auto fits = [](const BallLoadout& L, int kind) {
        if (kind < 0) return true;
        const auto k = static_cast<UpgradeKind>(kind);
        return !L.has(k) && upgradeFitsBall(k, L);
    };
    return fits(b, ka) && fits(a, kb) ? toSlot : -1;
}

// Levels travel with the pick. Everything derived (classes, ascended, ability
// slot count - Mage abilities fall asleep / wake up) comes out of the loadout,
// so re-syncing the world balls is all it takes. A moved ability restarts its
// cooldown like a fresh pick (World::applySpec), so a charged one can't be
// passed around to fire twice.
bool App::moveSlot(int fromBall, int fromSlot, int toBall, int toSlot) {
    toSlot = slotMoveTarget(fromBall, fromSlot, toBall, toSlot);
    if (toSlot < 0) return false;
    BallLoadout& a = data_.run.balls[static_cast<std::size_t>(fromBall)];
    BallLoadout& b = data_.run.balls[static_cast<std::size_t>(toBall)];
    const int ka = a.kindAt(fromSlot), la = a.levelAt(fromSlot);
    const int kb = b.kindAt(toSlot), lb = b.levelAt(toSlot);
    a.setSlot(fromSlot, kb, kb < 0 ? 0 : lb);
    b.setSlot(toSlot, ka, la);
    syncWorldBalls();
    return true;
}

// ---------------------------------------------------------------- shop

int App::shopPrice(UpgradeKind k) const {
    int base = 0;
    switch (upgradeCat(k)) {
        case UpgradeCat::NewBall:  base = cfg::gold::priceNewBall; break;
        case UpgradeCat::Modifier: base = cfg::gold::priceModifier; break;
        default:                   base = cfg::gold::priceByTier[static_cast<int>(upgradeTier(k))]; break;
    }
    const float off = cfg::meta::hagglerPerLevel * static_cast<float>(data_.meta.unlock[MetaHaggler]);   // "Haggler"
    return std::max(1, static_cast<int>(std::lround(static_cast<float>(base) * (1.f - off))));
}

// A fresh shop visit: new stock, a mystery box, rerolls back to full price.
void App::rollShop() {
    RunState& r = data_.run;
    r.shopOffers.clear();
    r.shopSold.clear();
    r.shopDeal.clear();
    r.shopMystery = 1;
    r.shopRerolls = 0;
    rollShopOffers();
}

void App::buyShopOffer(int i) {
    RunState& r = data_.run;
    if (i < 0 || i >= static_cast<int>(r.shopOffers.size()) || r.shopSold[static_cast<std::size_t>(i)]) return;
    const auto k = static_cast<UpgradeKind>(r.shopOffers[static_cast<std::size_t>(i)]);
    if (r.gold < shopOfferPrice(i) || !upgradeEligible(k, buildUpgradeCtx())) return;
    if (upgradeNeedsTarget(k)) {
        beginEquip(EquipSource::Shop, k, i);
        return;
    }
    r.gold -= shopOfferPrice(i);
    r.shopSold[static_cast<std::size_t>(i)] = true;
    applyUpgradeKind(k);
    audio_.purchase();
    effects_.flash(theme::puGolden, 0.35f);
}

int App::repairAmount() const {
    return static_cast<int>(std::lround(world_.core().maxHp * cfg::gold::repairFrac));
}

void App::buyRepair() {
    const Core& c = world_.core();
    if (data_.run.gold < cfg::gold::priceRepair || c.hp >= c.maxHp - 0.5f) return;
    data_.run.gold -= cfg::gold::priceRepair;
    playerRepair(static_cast<float>(repairAmount()));
    audio_.purchase();
    effects_.flash(theme::core, 0.35f);
}

void App::leaveShop() {
    back();
    openMap();
}

// Pay out the run: cores, prisms and lifetime stats. No navigation - the caller
// decides where to go (BossWin card, or straight to the game menu). Idempotent
// via runBanked_ so a "Back to menu" after an already-banked win pays nothing.
void App::bankRun(bool won) {
    if (runBanked_) return;
    runBanked_ = true;

    RunState& r = data_.run;
    lastRunWave_ = r.wave;
    lastRunWon_ = won;

    int cores = r.wave * cfg::meta::coresPerWave + (won ? cfg::meta::winBonus : 0);
    cores += static_cast<int>(r.bountyCores);   // "Fortune" node: cores per enemy killed
    lastRunCores_ = cores;

    lastRunPrisms_ = 0;
    if (won) {
        lastRunPrisms_ = cfg::meta::prismsPerWin;
        if (data_.meta.unlock[MetaWindfall] > 0 &&
            rng_.range(0.f, 1.f) < cfg::meta::windfallChance)
            lastRunPrisms_ += 1;                 // "Windfall" node
    }

    data_.meta.cores += static_cast<std::uint32_t>(cores);
    data_.meta.prisms += static_cast<std::uint32_t>(lastRunPrisms_);
    data_.meta.stats.coresEarned += static_cast<std::uint32_t>(cores);
    data_.meta.stats.bestWave =
        std::max(data_.meta.stats.bestWave, static_cast<std::uint32_t>(r.wave));
    data_.meta.stats.bestScore =
        std::max(data_.meta.stats.bestScore, static_cast<std::uint32_t>(r.score));
    if (won) ++data_.meta.stats.wins;

    save();
}

void App::finishToMenu() {
    data_.run = RunState{};
    save();
    replaceStack(ScreenId::Menu);
    push(ScreenId::Loadout);
}

bool App::bossWinCanContinue() const {
    return continueUnlocked_ && data_.run.active && !runBanked_ &&
           data_.run.wave == cfg::run::bossWave;
}

// "Continue" on the BossWin card: on to act 2 (a fresh map, waves 11-20).
// Nothing is banked - the run is still live and pays out when it truly ends.
void App::continuePastBoss() {
    back();   // drop the BossWin card, back to the PlayScreen underneath
    RunState& r = data_.run;
    r.gold += cfg::gold::bossPay;
    r.map = generateMap(rng_, 2);
    r.mapNode = -1;
    r.mapRow = 0;
    r.phoenixUsedAct = false;
    r.repairedThisAct = false;   // "Iron core": a fresh streak for the new act
    world_.setPhoenix((r.mods.phoenix ? 1 : 0) + r.lastStandLeft);   // "Phoenix" recharges for the new act
    // A pact first (Fase O), then the boss treasure (an Epic / Legendary pick), then the map.
    if (!openPactChoice(PactSource::Boss)) openChoice(RollSource::Boss);
}

// "Back to menu" on the BossWin card. If the run wasn't banked yet (miniboss just
// cleared on a run that had "Continue" available), it counts as a win now.
void App::leaveBossWin() {
    if (!runBanked_) bankRun(true);
    finishToMenu();
}

void App::abandonRun() {
    data_.run = RunState{};
    save();
    replaceStack(ScreenId::Menu);
    push(ScreenId::Loadout);
}

void App::wipeSave() {
    std::error_code ec;
    std::filesystem::remove(savePath_, ec);   // start the next save from nothing

    const SoundSettings sound = data_.meta.sound;   // a preference, not progress: keep the mix
    data_ = GameData{};                        // cores, prisms, unlocks, stats, run
    data_.meta.sound = sound;
    audio_.setEnabled(data_.meta.soundOn);
    window_.applyVideoMode(data_.meta.fullscreen);

    lastRunWave_ = 0;
    lastRunCores_ = 0;
    lastRunPrisms_ = 0;
    lastRunWon_ = false;

    world_.startRun(params(), {}, cfg::core::baseHp, cfg::core::baseHp);
    effects_.clear();
    effects_.flash(theme::coreLow, 0.6f);
    save();
    replaceStack(ScreenId::Menu);
}

// ---------------------------------------------------------------- dev tools

bool App::devMode() const {
    static const bool on = envDevMode();
    return on;
}

void App::devWinWave() {
    if (!devMode() || !data_.run.active) return;
    world_.devWinWave();
}

void App::devGrantCores(int n) {
    if (!devMode()) return;
    data_.meta.cores += static_cast<std::uint32_t>(std::max(0, n));
    effects_.flash(theme::accent, 0.3f);
}

void App::devGrantCurrency() {
    if (!devMode()) return;
    data_.meta.cores = std::max(data_.meta.cores, 999999u);
    data_.meta.prisms = std::max(data_.meta.prisms, 999999u);
    effects_.flash(theme::accent, 0.4f);
    save();
}

void App::devHealCore() {
    if (!devMode() || !data_.run.active) return;
    world_.repairCore(1e9f);
    effects_.flash(theme::core, 0.3f);
}

void App::devToggleInvuln() {
    if (!devMode() || !data_.run.active) return;
    world_.devSetInvuln(!world_.devInvuln());
    effects_.flash(world_.devInvuln() ? theme::core : theme::coreLow, 0.3f);
}

void App::devAddBall() {
    if (!devMode() || !data_.run.active) return;
    if (runBallCount() >= cfg::ball::maxBalls) return;
    data_.run.balls.push_back(BallLoadout{});
    syncWorldBalls();
}

void App::devOpenPanel() {
    if (!devMode() || !data_.run.active) return;
    world_.forceRelease();
    setAiming(false);
    push(ScreenId::Dev);
}

void App::devGrant(UpgradeKind k) {
    if (!devMode() || !data_.run.active) return;
    if (upgradeCat(k) == UpgradeCat::NewBall) {
        devAddBall();
        return;
    }
    const int b = devBall();
    if (upgradeNeedsTarget(k)) {
        if (b >= runBallCount() || !upgradeFitsBall(k, data_.run.balls[static_cast<std::size_t>(b)])) {
            effects_.addLabel("doesn't fit that ball", {size().x * 0.5f, size().y * 0.9f}, theme::coreLow, 18, 0.9f);
            return;
        }
        applyUpgradeKind(k, b, -1);
    } else {
        applyUpgradeKind(k);
    }
    effects_.addLabel(std::string("+ ") + upgradeInfo(k).title, {size().x * 0.5f, size().y * 0.9f},
                      tierColor(upgradeTier(k)), 18, 0.9f);
}

void App::devClearBall() {
    if (!devMode() || !data_.run.active || runBallCount() == 0) return;
    data_.run.balls[static_cast<std::size_t>(devBall())] = BallLoadout{};
    syncWorldBalls();
}

void App::devSpawn(EnemyKind k, int n) {
    if (!devMode() || !data_.run.active) return;
    world_.devSpawn(k, n);
}

void App::devKillAll() {
    if (!devMode() || !data_.run.active) return;
    world_.devKillAll();
}

void App::devGold(int n) {
    if (!devMode() || !data_.run.active) return;
    data_.run.gold += n;
}

void App::devOpen(DevOpen what) {
    if (!devMode() || !data_.run.active) return;
    back();   // close the dev panel, back on the fight
    switch (what) {
        case DevOpen::Shop:         rollShop(); push(ScreenId::Shop); break;
        case DevOpen::Forge:        equipSrc_ = EquipSource::Forge; equipRef_ = -1; push(ScreenId::Equip); break;
        case DevOpen::Upgrade:      openChoice(RollSource::Normal); break;
        case DevOpen::Elite:        openChoice(RollSource::Elite); break;
        case DevOpen::BossTreasure: openChoice(RollSource::Boss); break;
        case DevOpen::Recruit:      rollRecruitChoices(); push(ScreenId::Choice); break;
        case DevOpen::PactBoss:     // the pact choice; after it the boss treasure / map follow as usual
        case DevOpen::PactStart:
            if (!openPactChoice(what == DevOpen::PactBoss ? PactSource::Boss : PactSource::Start))
                effects_.addLabel("no pact left to offer", {size().x * 0.5f, size().y * 0.5f}, theme::coreLow, 22, 1.2f);
            break;
        case DevOpen::AbilityPick:  // the first-ability pick, on the first ball, as at the run start
            if (!openAbilityChoice())
                effects_.addLabel("no ability to pick (or only one: granted)", {size().x * 0.5f, size().y * 0.5f},
                                  theme::textLo, 22, 1.2f);
            break;
        case DevOpen::JumpToBoss:   // stand right before the boss row and open the map
            data_.run.mapRow = cfg::map::rows;
            data_.run.mapNode = -1;
            world_.devWinWave();
            openMap();
            break;
    }
}

void App::devCycleGrant() {
    if (!devMode() || !data_.run.active) return;
    const auto k = static_cast<UpgradeKind>(devGrantNext_ % kUpgradeKindCount);
    devGrantNext_ = (devGrantNext_ + 1) % kUpgradeKindCount;
    applyUpgradeKind(k);
    effects_.addLabel(std::string("+ ") + upgradeInfo(k).title, {size().x * 0.5f, size().y * 0.4f},
                      theme::accent, 22, 1.0f);
}

void App::openPause() {
    world_.forceRelease();
    setAiming(false);
    push(ScreenId::Pause);
}
void App::openStats() { push(ScreenId::Stats); }
void App::openSound() { push(ScreenId::Sound); }

bool App::onOptions() const {
    return !stack_.empty() && dynamic_cast<const SoundScreen*>(stack_.back().get()) != nullptr;
}

void App::openHowTo() { push(ScreenId::HowTo); }

void App::quit() {
    save();
    window_.close();
}

void App::buyMetaUnlock(int u) {
    if (u < 0 || u >= MetaUnlockCount) return;
    if (metaUnlockMaxed(u, data_.meta.unlock[u])) return;
    if (!metaUnlockAvailable(u, data_.meta.unlock)) return;
    const std::uint32_t cost = metaUnlockCost(u, data_.meta.unlock[u]);
    if (metaUnlockCurrency(u) == MetaCurrency::Prisms) {
        if (data_.meta.prisms < cost) return;
        data_.meta.prisms -= cost;
    } else {
        if (data_.meta.cores < cost) return;
        data_.meta.cores -= cost;
    }
    ++data_.meta.unlock[u];
    audio_.purchase();
    effects_.flash(theme::accent, 0.4f);
    save();
}

void App::toggleSound() {
    data_.meta.soundOn = !data_.meta.soundOn;
    audio_.setEnabled(data_.meta.soundOn);
    save();
}

void App::setAiming(bool on) {
    aiming_ = on;
    aimT_ = 0.f;
}

void App::toggleFullscreen() {
    data_.meta.fullscreen = !data_.meta.fullscreen;
    window_.applyVideoMode(data_.meta.fullscreen);
    save();
}

void App::save() { saveGame(savePath_, data_); }

// ---------------------------------------------------------------- loop

void App::handleEvent(const sf::Event& e) {
    if (e.type == sf::Event::Closed) {
        quit();
        return;
    }
    if (e.type == sf::Event::Resized) {
        window_.applyLetterbox(e.size.width, e.size.height);
        return;
    }
    if (e.type == sf::Event::KeyPressed &&
        (e.key.code == sf::Keyboard::F11 || e.key.code == sf::Keyboard::F)) {
        toggleFullscreen();
        return;
    }
    if (!stack_.empty()) {
        // Options from anywhere: the O key (not mid-throw).
        const bool pressO = e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::O &&
                            !sf::Mouse::isButtonPressed(sf::Mouse::Left);
        if (!onOptions() && pressO) {
            peek_.close();
            openSound();
            return;
        }
        // TAB anywhere in a live run: screens without their own peek get the App's.
        if (data_.run.active && !stack_.back()->ownsTab() && !onOptions()) {
            if (peek_.handle(e)) return;
            if (peek_.open) {   // looking: the screen underneath waits; slots can be dragged
                loadoutDragEvent(*this, peek_, e);
                return;
            }
        }
        // The play screen wants the pointer in world units (grab / throw); every
        // other screen lays its widgets out in fixed UI units, so it must get the
        // UI-mapped pointer - the world view may be zoomed out on the boss arena.
        const sf::Vector2f mouse =
            simulating() ? window_.mousePosition() : window_.uiMousePosition();
        // Menus / buttons: one shared click whenever a press lands on something
        // clickable (widgets report it via uisound::hover).
        if (!simulating() && e.type == sf::Event::MouseButtonPressed &&
            e.mouseButton.button == sf::Mouse::Left && uisound::hot())
            audio_.uiClick();
        stack_.back()->handleEvent(*this, e, mouse);
    }
}

sf::Vector2f App::worldToUi(sf::Vector2f p) const {
    const sf::Vector2f tl = camCenter_ + camShake_ - camSize_ * 0.5f;
    return {(p.x - tl.x) * kLogical().x / camSize_.x, (p.y - tl.y) * kLogical().y / camSize_.y};
}

sf::Vector2f App::goldCounterPos() const {
    return {size().x - theme::margin - 30.f, theme::margin + 30.f};
}

void App::flushMultiKill() {
    if (multiKillN_ >= cfg::gold::multiKillMin) {
        const int bonus = multiKillN_ * cfg::gold::multiKillGoldPer;
        data_.run.gold += bonus;
        effects_.addLabel("x" + std::to_string(multiKillN_) + " MULTI-KILL  +" + std::to_string(bonus),
                          multiKillPos_ + sf::Vector2f{0.f, -24.f}, theme::puGolden,
                          static_cast<unsigned>(std::min(34, 18 + 2 * multiKillN_)), 1.0f);
        audio_.comboUp(std::min(multiKillN_, cfg::combo::baseCapTier));
        for (int i = 0; i < std::min(multiKillN_, 6); ++i)
            effects_.addCoin(multiKillPos_, goldCounterPos(), 7.f);
    }
    multiKillN_ = 0;
    multiKillT_ = 0.f;
}

void App::processEvents(const FrameEvents& ev) {
    // Every kill drops gold that grows with the combo; the coin grows with it.
    const float comboGold = 1.f + cfg::gold::comboBonusPerTier * static_cast<float>(ev.comboTier);
    audio_.kill(static_cast<int>(ev.kills.size()));
    for (const sf::Vector2f& k : ev.kills) {
        ++data_.meta.stats.enemiesKilled;
        effects_.addPop(k, cfg::wave::enemyRadius, theme::enemy);
        if (!data_.run.active) continue;
        data_.run.goldFrac += cfg::gold::perKill * comboGold;
        const sf::Vector2f ui = worldToUi(k);
        effects_.addCoin(ui, goldCounterPos(), 3.5f + 1.2f * static_cast<float>(ev.comboTier));
        multiKillPos_ = ui;
        ++multiKillN_;
        multiKillT_ = cfg::gold::multiKillWindow;
    }
    if (data_.run.goldFrac >= 1.f) {
        const int whole = static_cast<int>(data_.run.goldFrac);
        data_.run.gold += whole;
        data_.run.goldFrac -= static_cast<float>(whole);
        audio_.gold();
    }
    if (!ev.kills.empty()) {
        hitstop_ = std::max(hitstop_, cfg::app::hitstopKill);
        const int n = static_cast<int>(ev.kills.size());
        const auto& eff = world_.effect();
        const bool dbl = eff && eff->kind == PowerUp::Points2x && eff->remaining > 0.f;
        data_.run.score += n * cfg::score::perKill * (dbl ? 2 : 1);
        data_.run.bountyCores += static_cast<float>(n) *
            (cfg::meta::bountyPerKillPerLevel * static_cast<float>(data_.meta.unlock[MetaBounty]) +
             cfg::meta::salvagePerKillPerLevel * static_cast<float>(data_.meta.unlock[MetaSalvage]));
    }
    // The combo tier drives how "harmonic" the bounce notes get (0..1).
    const float harmony = cfg::combo::baseCapTier > 0
        ? static_cast<float>(ev.comboTier) / static_cast<float>(cfg::combo::baseCapTier)
        : 0.f;
    // Reactions, explosions, ascended pulses, abilities: a big ring, and the reaction's
    // name (rate-limited so a cascade reads as a burst, not a wall of text).
    for (const BurstFx& b : ev.bursts) {
        effects_.addBurst(b.pos, b.radius, b.color);
        if (b.label && reactLabelCd_ <= 0.f) {
            effects_.addLabel(b.label, worldToUi(b.pos) + sf::Vector2f{0.f, -18.f}, b.color, 20, 0.8f);
            audio_.comboUp(4);
            camKick_ = std::max(camKick_, 3.5f);   // a reaction lands with a small jolt
            reactLabelCd_ = 0.25f;
        }
    }
    for (const BounceFx& b : ev.bounces) {
        effects_.addRing(b.pos, b.speed, b.color);
        if (!b.ballPair) effects_.edgeHit(b.normal);   // edge flash only for wall / core hits
        audio_.ballHit(clampf(b.speed / 900.f, 0.f, 1.f), harmony, b.ballPair);
    }
    if (ev.autoFlung) audio_.thrown(0.35f);
    if (ev.midasGold > 0 && data_.run.active)   // "Midas": extra gold per kill
        data_.run.gold += ev.midasGold;
    if (ev.phoenix) {
        // The Phoenix relic (once per act) goes first; "Last stand" is the once-per-run spare.
        RunState& rr = data_.run;
        const bool relic = rr.mods.phoenix && !rr.phoenixUsedAct;
        if (relic) rr.phoenixUsedAct = true;
        else if (rr.lastStandLeft > 0) --rr.lastStandLeft;
        effects_.flash(theme::puGolden, 1.f);
        effects_.addLabel(relic ? "PHOENIX" : "LAST STAND", {size().x * 0.5f, size().y * 0.4f}, theme::puGolden, 40, 1.6f);
        audio_.comboUp(cfg::combo::baseCapTier);
    }
    if (ev.comboTierUp) {
        hud_.pulseCombo();
        audio_.comboUp(ev.comboTier);
    }
    if (ev.gotPickup) {
        const sf::Color c = powerUpColor(ev.pickupKind);
        effects_.addLabel(powerUpName(ev.pickupKind), {size().x * 0.5f, size().y * 0.34f}, c, 24, 1.1f);
        effects_.flash(c, 0.7f);
        audio_.pickup();
    }
    if (ev.coreHit) {
        effects_.flash(theme::coreLow, 0.55f);
        audio_.coreThud();
        hitstop_ = std::max(hitstop_, cfg::app::hitstopCoreHit);
        camKick_ = std::max(camKick_, cfg::app::camKickCoreHit);
    }
    if (ev.bossHit) {
        hitstop_ = std::max(hitstop_, cfg::app::hitstopBossHit);
        camKick_ = std::max(camKick_, cfg::app::camKickBossHit);
    }
    data_.meta.stats.bestCombo =
        std::max(data_.meta.stats.bestCombo, static_cast<std::uint32_t>(world_.comboStreak()));
    data_.meta.stats.maxSpeed = std::max(data_.meta.stats.maxSpeed, world_.fastestBall());

    if (ev.runOver) {
        bankRun(false);
        finishToMenu();
        return;
    }
    if (ev.waveCleared) {
        flushMultiKill();   // the last burst of the wave still counts
        audio_.waveClear();
        if (const int interest = data_.meta.unlock[MetaInterest];   // "Interest": reward a clean wave
            interest > 0 && world_.coreCleanWave())
            data_.run.bountyCores += cfg::meta::interestPerLevel * static_cast<float>(interest);
        const int w = data_.run.wave;
        const bool flawless = world_.coreCleanWave();   // nothing reached the core this fight
        if (w == cfg::run::bossWave || w >= cfg::run::finalWave) {
            // Boss down: a flawless act-1 boss pays gold for act 2, and an act
            // with no deliberate repair banks "Iron core" cores (before bankRun).
            RunState& r = data_.run;
            bossFlawlessGold_ = flawless && w == cfg::run::bossWave ? cfg::gold::flawlessBoss : 0;
            bossIronCores_ = r.repairedThisAct ? 0 : cfg::meta::ironCoreCores;
            r.gold += bossFlawlessGold_;
            r.bountyCores += static_cast<float>(bossIronCores_);
        }
        if (w == cfg::run::bossWave) {
            // Miniboss down. First win ever: bank it now, card offers only "Back".
            // Otherwise leave the run live so "Continue" can carry it to wave 11.
            if (!continueUnlocked_) bankRun(true);
            push(ScreenId::BossWin);
            return;
        }
        if (w >= cfg::run::finalWave) {
            bankRun(true);
            push(ScreenId::BossWin);
            return;
        }
        // A cleared fight pays gold (an Elite double, plus a pick), and a
        // flawless one - nothing reached the core - pays a bonus on top
        // (outside "Loaded Dice"'s gamble).
        RunState& r = data_.run;
        const int row = w - (r.map.act - 1) * cfg::run::bossWave;
        int pay = cfg::gold::combatBase + cfg::gold::perRow * row;
        if (r.eliteWave)
            pay = static_cast<int>(std::lround(static_cast<float>(pay * cfg::gold::elitePerRowMul) *
                (1.f + cfg::meta::eliteSpoilsPerLevel * static_cast<float>(data_.meta.unlock[MetaEliteSpoils]))));
        if (hasPact(PactId::LoadedDice)) {   // "Loaded Dice": double or nothing
            const bool win = rng_.range(0.f, 1.f) < 0.5f;
            pay = win ? pay * 2 : 0;
            effects_.addLabel(win ? "DOUBLE!" : "NOTHING", {size().x * 0.5f, size().y * 0.4f - 36.f},
                              win ? theme::puGolden : theme::coreLow, 24, 1.3f);
        }
        r.gold += pay;
        effects_.addLabel("+" + std::to_string(pay) + " gold", {size().x * 0.5f, size().y * 0.4f},
                          theme::puGolden, 26, 1.2f);
        if (flawless) {
            int bonus = cfg::gold::flawlessBase + cfg::gold::flawlessPerRow * row;
            if (r.eliteWave) bonus *= cfg::gold::flawlessEliteMul;
            r.gold += bonus;
            effects_.addLabel("FLAWLESS  +" + std::to_string(bonus) + " gold", {size().x * 0.5f, size().y * 0.4f + 36.f},
                              theme::core, 24, 1.5f);
            effects_.flash(theme::core, 0.3f);
        }
        postFight();
    }
}

// After a cleared fight: the build grows. The run's first fight hands the
// first ball its first ability; then every fight offers a pick (an Elite's at
// elite odds) before the map.
void App::postFight() {
    RunState& r = data_.run;
    const bool firstFight = r.map.act == 1 && r.mapRow <= 1;
    const bool abilityless = !r.balls.empty() && std::all_of(std::begin(r.balls[0].ability),
                                                             std::end(r.balls[0].ability),
                                                             [](int a) { return a < 0; });
    if (firstFight && abilityless && openAbilityChoice()) {
        abilityAfterFight_ = true;
        return;
    }
    openChoice(r.eliteWave ? RollSource::Elite : RollSource::Normal);
}

void App::update(float frameDt) {
    // World units for the play screen, fixed UI units for menus / cards (see
    // handleEvent) - the world view can be zoomed out on the boss arena.
    const sf::Vector2f mouse =
        simulating() ? window_.mousePosition() : window_.uiMousePosition();
    effects_.update(frameDt);
    fade_ *= std::exp(-cfg::app::fadeRate * frameDt);

    // Music follows the run: the play track through a live run (Choice / Pause /
    // BossWin card included), the menu track everywhere else.
    audio_.setTrack(data_.run.active ? Audio::Track::Game : Audio::Track::Menu);

    // A quiet hum under a live fight; a soft two-note warning while the core is low.
    audio_.setAmbience(simulating() && data_.run.active);
    if (simulating() && data_.run.active && world_.core().maxHp > 0.f &&
        world_.core().hp > 0.f && world_.core().hp < world_.core().maxHp * 0.3f) {
        lowCoreCd_ -= frameDt;
        if (lowCoreCd_ <= 0.f) { audio_.coreWarning(); lowCoreCd_ = 4.f; }
    } else {
        lowCoreCd_ = 0.f;   // warn at once the next time it drops low
    }

    uisound::beginFrame();
    if (!data_.run.active || stack_.empty() || stack_.back()->ownsTab() || onOptions()) peek_.close();
    peek_.update(frameDt);
    if (!stack_.empty()) {
        // Under the TAB peek the screen gets no pointer (no hover, no tooltips).
        stack_.back()->update(*this, frameDt, peek_.open ? sf::Vector2f{-1e6f, -1e6f} : mouse);
        stack_.back()->advanceIntro(frameDt);
    }

    if (simulating() && !stack_.back()->frozen()) {
        // A fresh wave eases in: feed the fixed-step accumulator slowly at first
        // and ramp to real time, so balls flow out of the previous wave.
        float simDt = frameDt;
        if (waveIntro_ > 0.f) {
            waveIntro_ = std::max(0.f, waveIntro_ - frameDt);
            const float t = 1.f - waveIntro_ / cfg::app::waveIntroTime;  // 0 -> 1
            simDt *= cfg::app::waveIntroSlow + (1.f - cfg::app::waveIntroSlow) * t;
        }
        if (aiming_ && aimT_ < cfg::app::aimSlowMax) {   // slingshot aim: bullet time, briefly
            aimT_ += frameDt;
            simDt *= cfg::app::aimTimeScale;
        }
        simDt *= devTimeScale_;   // dev panel: slow motion / fast forward
        novaCd_ = std::max(0.f, novaCd_ - frameDt);   // "Nova" pact recharges while you fight
        if (hitstop_ > 0.f) {  // an impact landed: hold the frame, no catch-up after
            hitstop_ = std::max(0.f, hitstop_ - frameDt);
            simDt = 0.f;
        }
        worldAccum_ += simDt;
        int steps = 0;
        while (worldAccum_ >= cfg::loop::fixedDt && steps < cfg::loop::maxSteps) {
            processEvents(world_.step(cfg::loop::fixedDt, params()));
            worldAccum_ -= cfg::loop::fixedDt;
            ++steps;
            if (!simulating()) break;  // a screen (choice / summary) was just pushed
            if (hitstop_ > 0.f) { worldAccum_ = 0.f; break; }  // impact mid-frame: stop now
        }
        if (steps == cfg::loop::maxSteps) worldAccum_ = 0.f;

        reactLabelCd_ = std::max(0.f, reactLabelCd_ - frameDt);
        if (multiKillT_ > 0.f) {   // a kill burst ends once kills stop chaining
            multiKillT_ -= frameDt;
            if (multiKillT_ <= 0.f) flushMultiKill();
        }
        data_.meta.stats.timePlayed += frameDt;
        autosaveTimer_ -= frameDt;
        if (autosaveTimer_ <= 0.f) {
            save();
            autosaveTimer_ = cfg::app::autosaveInterval;
        }
    } else {
        worldAccum_ = 0.f;
    }

    // Camera: ease toward the area the world wants framed (wider from the boss
    // wave on). Hold that framing for the whole live run - through the between-
    // wave Choice and the "Continue" card too - so the view doesn't zoom in and
    // straight back out. It snaps to the fixed view once the run is banked (a
    // loss, or the final "Back to menu") or gone.
    sf::Vector2f tgtSize = kLogical();
    sf::Vector2f tgtCenter = kLogical() * 0.5f;
    bool snap = true;
    if (data_.run.active && !runBanked_) {
        tgtSize = world_.viewSize();
        tgtCenter = world_.viewCenter();
        snap = false;
    }
    if (snap) {
        camSize_ = tgtSize;
        camCenter_ = tgtCenter;
        camKick_ = 0.f;
        camShake_ = {0.f, 0.f};
    } else {
        const float k = 1.f - std::exp(-cfg::boss::camEase * frameDt);
        camSize_ += (tgtSize - camSize_) * k;
        camCenter_ += (tgtCenter - camCenter_) * k;

        // Camera kick: a brief shake on hits, scaled to the current zoom so it
        // reads the same whether or not the boss arena has pulled the view back.
        camKick_ *= std::exp(-cfg::app::camKickDecay * frameDt);
        if (camKick_ > 0.2f) {
            const float amp = camKick_ * (camSize_.x / kLogical().x);
            camShake_ = {rng_.range(-amp, amp), rng_.range(-amp, amp)};
        } else {
            camKick_ = 0.f;
            camShake_ = {0.f, 0.f};
        }
    }
    window_.setWorldView(camSize_, camCenter_ + camShake_);

    // Backdrop heat follows the damage combo while you fight; it cools off
    // slowly when the combo drops or the fight ends.
    {
        const float target = simulating() && cfg::combo::baseCapTier > 0
            ? static_cast<float>(world_.comboTier()) / static_cast<float>(cfg::combo::baseCapTier)
            : 0.f;
        const float rate = target > heat_ ? cfg::app::heatRise : cfg::app::heatFall;
        heat_ += (target - heat_) * (1.f - std::exp(-rate * frameDt));
    }
    if (const int n = effects_.takeArrivedCoins(); n > 0) hud_.pulseGold();

    const Core& c = world_.core();
    hud_.update(frameDt, data_.run.map.act, data_.run.mapRow, cfg::map::rows + 1, world_.enemiesLeft(),
                c.maxHp > 0.f ? c.hp / c.maxHp : 0.f, world_.comboMultiplier(),
                data_.run.score, data_.run.gold, world_.effect(), world_.bossWave(),
                world_.hasReserve(), world_.reservePu());
}

void App::render() {
    sf::RenderWindow& w = window_.handle();
    window_.useUiView();
    w.clear(theme::bg);
    backdrop::draw(w, size());
    if (heat_ > 0.01f) {   // warm tint over the arena (not the letterbox bars)
        sf::RectangleShape hot(size());
        hot.setFillColor(withAlpha(theme::bgHot, heat_ * cfg::app::heatAlpha));
        w.draw(hot);
    }
    effects_.drawBorder(w);

    std::size_t start = 0;
    for (std::size_t i = stack_.size(); i-- > 0;) {
        if (stack_[i]->opaque()) {
            start = i;
            break;
        }
    }
    for (std::size_t i = start; i < stack_.size(); ++i) stack_[i]->draw(*this, w);
    if (peek_.open) drawLoadoutOverlay(*this, w, false, peek_);

    effects_.drawOverlay(w);
    if (fade_ > 0.01f) drawDim(w, size(), fade_ * 0.5f);
    if (devMode()) drawDevOverlay(w);
    if (!capturePath_.empty()) {   // photo mode: grab the finished frame before it's shown
        sf::Texture shot;
        if (shot.create(w.getSize().x, w.getSize().y)) {
            shot.update(w);
            shot.copyToImage().saveToFile(capturePath_);
        }
        capturePath_.clear();
    }
    w.display();
}

void App::snapFrame(const std::string& file) {
    for (int i = 0; i < 12; ++i) update(0.1f);   // let intros / fades settle
    capturePath_ = file;
    render();
}

int App::runSnapshots(const std::string& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    savePath_ = dir + "/snapshot_save.txt";   // never touch the real save
    const std::string d = dir + "/";

    snapFrame(d + "01_menu.png");

    data_.meta.cores = 480;
    data_.meta.prisms = 6;
    for (int u : {MetaCalling, MetaCoreHp, MetaMass, MetaClassGuardian, MetaHeft, MetaSling, MetaAbilityMissile,
                  MetaAbilityArc, MetaUplink, MetaCapacitor, MetaBounty, MetaVelocity})
        data_.meta.unlock[u] = 1;
    openLoadout();
    snapFrame(d + "02_web.png");

    // The run intro: pick the first ball's first ability.
    newRun();
    snapFrame(d + "03_ability_pick.png");
    chooseAbility(0);
    snapFrame(d + "03_map.png");

    // A dressed-up squad so the arena and the panels have something to show:
    // a dual-class ball, an ascended one, types and abilities in their slots.
    RunState& r = data_.run;
    r.gold = 187;
    r.balls.assign(3, BallLoadout{});
    auto put = [](BallLoadout& L, int slot, UpgradeKind k, int lvl) { L.setSlot(slot, static_cast<int>(k), lvl); };
    put(r.balls[0], kSlotType, UpgradeKind::ElemFire, 1);
    put(r.balls[0], 0, UpgradeKind::Cleave, 2);
    put(r.balls[0], 1, UpgradeKind::Crit, 1);
    put(r.balls[0], 2, UpgradeKind::Tesla, 1);
    put(r.balls[0], 3, UpgradeKind::Bomber, 1);
    put(r.balls[0], kSlotAbility, UpgradeKind::AbilityDash, 2);          // Striker / Support
    put(r.balls[1], kSlotType, UpgradeKind::ElemIce, 1);
    put(r.balls[1], 0, UpgradeKind::Rampart, 1);
    put(r.balls[1], 1, UpgradeKind::Mender, 1);
    put(r.balls[1], 2, UpgradeKind::Bumper, 1);
    put(r.balls[1], 3, UpgradeKind::Glutton, 1);
    put(r.balls[1], kSlotAbility, UpgradeKind::AbilityBulwark, 1);       // Iron Guardian
    r.balls[1].mods[0] = 2;
    put(r.balls[2], kSlotType, UpgradeKind::ElemElectric, 1);
    put(r.balls[2], 0, UpgradeKind::Storm, 1);
    put(r.balls[2], kSlotAbility, UpgradeKind::AbilityMissile, 3);   // magic missiles in flight
    r.mods.catalyst = true;
    r.mods.luckyClover = true;
    r.mods.spring = true;
    rememberClasses();   // staged, not earned: no class-gain banners for the dressed-up squad
    syncWorldBalls();
    for (int i = 0; i < static_cast<int>(r.map.nodes.size()); ++i)
        if (mapNodeOpen(i)) { travelTo(i); break; }
    for (int i = 0; i < 300; ++i) update(1.f / 60.f);   // five seconds of fighting
    capturePath_ = d + "04_play.png";
    render();

    // Tesla would give ball 3 (Storm) its Support class: the card says so.
    choices_ = {UpgradeKind::Railgun, UpgradeKind::Tesla, UpgradeKind::Cleave, UpgradeKind::Hunter};
    rollSource_ = RollSource::Normal;
    push(ScreenId::Choice);
    snapFrame(d + "05_choice.png");
    back();

    // Class-gain moments: ball 3 takes Tesla (2 Support = the class), then -
    // staged for a moment - two more Support items (4 = Grand Support).
    applyUpgradeKind(UpgradeKind::Tesla, 2, -1);
    for (int i = 0; i < 20; ++i) update(1.f / 60.f);
    capturePath_ = d + "05b_class_gain.png";
    render();
    const BallLoadout keep = r.balls[2];
    effects_.clear();
    r.balls[2].setSlot(2, static_cast<int>(UpgradeKind::Bomber), 1);
    r.balls[2].setSlot(3, static_cast<int>(UpgradeKind::Resonance), 1);
    syncWorldBalls();
    for (int i = 0; i < 24; ++i) update(1.f / 60.f);
    capturePath_ = d + "05c_ascend.png";
    render();
    r.balls[2] = keep;
    syncWorldBalls();
    for (int i = 0; i < 90; ++i) update(1.f / 60.f);   // let the flare play out
    effects_.clear();

    rollShop();
    push(ScreenId::Shop);
    snapFrame(d + "06_shop.png");
    back();

    beginEquip(EquipSource::Choice, UpgradeKind::Railgun, 0);
    snapFrame(d + "07_equip.png");
    back();
    beginEquip(EquipSource::Choice, UpgradeKind::AbilityOverclock, 0);   // an ability: the ability slots
    snapFrame(d + "07_equip_ability.png");
    back();

    sf::Event tab{};
    tab.type = sf::Event::KeyPressed;
    tab.key.code = sf::Keyboard::Tab;
    stack_.back()->handleEvent(*this, tab, {0.f, 0.f});
    snapFrame(d + "08_tab.png");
    {   // a drag in the peek: ball 1's Cleave in hand over ball 3's free item slot
        auto slotMid = [&](int ball, int slot) {
            const sf::FloatRect sr = slotRect(loadoutPanelCenter(*this, ball), slot, r.balls[static_cast<std::size_t>(ball)]);
            return sf::Vector2f{sr.left + sr.width * 0.6f, sr.top + sr.height * 0.5f} * loadoutZoom(*this);
        };
        sf::Event press{};
        press.type = sf::Event::MouseButtonPressed;
        press.mouseButton.button = sf::Mouse::Left;
        snapMouseOn_ = true;
        snapMouse_ = slotMid(0, 0);
        stack_.back()->handleEvent(*this, press, {0.f, 0.f});
        snapMouse_ = slotMid(2, 1);
        snapFrame(d + "08_tab_drag.png");
        snapMouse_ = {size().x * 0.5f, 20.f};   // let go off every panel: it snaps back
        press.type = sf::Event::MouseButtonReleased;
        stack_.back()->handleEvent(*this, press, {0.f, 0.f});
        snapMouseOn_ = false;
    }
    tab.type = sf::Event::KeyReleased;
    stack_.back()->handleEvent(*this, tab, {0.f, 0.f});

    push(ScreenId::Dev);
    snapFrame(d + "09_dev.png");
    back();

    // A crowded field with every enemy kind, to judge readability under load.
    for (EnemyKind k : {EnemyKind::Grunt, EnemyKind::Runner, EnemyKind::Tank, EnemyKind::Splitter,
                        EnemyKind::Shielded})
        world_.devSpawn(k, 3);
    for (int i = 0; i < 90; ++i) update(1.f / 60.f);
    capturePath_ = d + "10_horde.png";
    render();

    // Straight to the act's boss row: the map late in the act, then the fight.
    // Walk a path up the map (first branch each time) so the walked trail and
    // the "you are here" marker show.
    for (int node = r.mapNode; node >= 0;) {
        MapNode& n = r.map.nodes[static_cast<std::size_t>(node)];
        n.visited = true;
        r.mapNode = node;
        r.mapRow = n.row;
        if (n.row >= cfg::map::rows || n.next.empty()) break;
        node = n.next[static_cast<std::size_t>(n.row) % n.next.size()];
    }
    world_.devWinWave();
    openMap();
    snapFrame(d + "11_map_late.png");
    for (int i = 0; i < static_cast<int>(r.map.nodes.size()); ++i)
        if (mapNodeOpen(i)) { travelTo(i); break; }
    for (int i = 0; i < 240; ++i) update(1.f / 60.f);
    capturePath_ = d + "12_boss.png";
    render();

    openPause();
    snapFrame(d + "13_pause.png");
    openStats();
    snapFrame(d + "14_stats.png");

    // Fase O: the pact choice (Oath: 4 cards), a fight under two pacts, the
    // shop's extras, the sell picker, the map with pacts and the bigger web.
    for (int u : {MetaOath, MetaCovenant, MetaPactHunters, MetaPactLegion, MetaPactDice, MetaPactAlchemy})
        data_.meta.unlock[u] = 1;
    if (openPactChoice(PactSource::Boss)) {
        snapFrame(d + "10_pact.png");
        back();
    }
    grantPact(PactId::Hunters);
    grantPact(PactId::LivingCore);
    world_.devSpawn(EnemyKind::Grunt, 8);
    world_.devSpawn(EnemyKind::Tank, 1);
    for (int i = 0; i < 150; ++i) update(1.f / 60.f);
    capturePath_ = d + "11_pact_fight.png";
    render();

    r.gold = 240;
    rollShop();
    push(ScreenId::Shop);
    buyMystery();
    snapFrame(d + "12_shop_extras.png");
    beginSell();
    snapFrame(d + "13_sell.png");
    back();
    back();

    openMap();
    snapFrame(d + "14_map_pacts.png");
    tab.type = sf::Event::KeyPressed;   // the TAB loadout peek, on the map
    stack_.back()->handleEvent(*this, tab, {0.f, 0.f});
    snapFrame(d + "14_map_tab.png");
    tab.type = sf::Event::KeyReleased;
    stack_.back()->handleEvent(*this, tab, {0.f, 0.f});
    back();

    data_.run = RunState{};   // back in the game menu, a well-grown web
    for (int u : {MetaChannel, MetaClassMage, MetaLoreMage, MetaAbilityBulwark, MetaFireItem, MetaVenom, MetaReroll,
                  MetaLuckyStar, MetaClassJester, MetaTreasury, MetaHaggler, MetaCharged, MetaClassSupport, MetaRally,
                  MetaKinetics, MetaClassShooter, MetaBrood, MetaMend, MetaAegis, MetaMomentum, MetaKeenInstinct})
        data_.meta.unlock[u] = 1;
    replaceStack(ScreenId::Menu);
    openLoadout();
    snapFrame(d + "15_web_grown.png");

    // The run intro: Covenant pact -> Quartermaster starter pick -> the map.
    data_.meta.unlock[MetaQuartermaster] = 1;
    data_.meta.unlock[MetaStarterKit] = 1;
    newRun();
    snapFrame(d + "16_start_pact.png");
    choosePact(0);
    snapFrame(d + "17_start_ability.png");   // the first-ability pick comes between the pact and the starter item
    chooseAbility(1);
    if (introStep_ >= 0) {   // the Quartermaster pick is up (it needs 4 items to choose from)
        snapFrame(d + "17_starter_pick.png");
        applyUpgrade(0);
        confirmEquip(0, -1);
    }
    snapFrame(d + "18_intro_map.png");

    // Clean-play bonuses on the BossWin card (flawless boss + "Iron core").
    data_.run.wave = cfg::run::bossWave;
    continueUnlocked_ = true;
    bossFlawlessGold_ = cfg::gold::flawlessBoss;
    bossIronCores_ = cfg::meta::ironCoreCores;
    push(ScreenId::BossWin);
    snapFrame(d + "19_boss_bonus.png");

    replaceStack(ScreenId::Menu);
    openSound();
    snapFrame(d + "20_sound.png");
    return 0;
}

// A fixed cheat-sheet of the dev keys, top-right on every screen, so there's no
// need to remember which key does what. Keys are screen-specific: the "run" ones
// only do anything on the Play screen, "menu" ones on the game menu.
void App::drawDevOverlay(sf::RenderWindow& w) const {
    const bool invuln = world_.devInvuln();
    const std::array<std::pair<const char*, bool>, 11> lines = {{
        {"- DEV -", true},
        {"menu / web:", false},
        {"  C   +999999 cores & prisms", false},
        {"in a run:", false},
        {"  N win wave   H heal core", false},
        {invuln ? "  G invuln: ON   B add ball" : "  G invuln: off   B add ball", invuln},
        {"  F1  DEV PANEL (items, spawns, speed)", true},
        {"  U grant next item   C +25 cores", false},
        {"  TAB   items taken", false},
        {"env (on Start):", false},
        {"  SB_WAVE  SB_BALLS  SB_UPGRADES", false},
    }};
    const float right = size().x - theme::margin;
    float y = theme::margin;
    for (const auto& [text, hot] : lines) {
        sf::Text t = makeText(font_, text, theme::fsSmall,
                              hot ? theme::core : theme::accent);
        const sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left + b.width, b.top);
        t.setPosition(right, y);
        w.draw(t);
        y += 16.f;
    }
}

int App::run() {
    if (const char* snap = std::getenv("SB_SNAPSHOT"); snap && *snap) return runSnapshots(snap);
    sf::Clock clock;
    while (window_.isOpen()) {
        sf::Event e;
        while (window_.handle().pollEvent(e)) handleEvent(e);
        if (!window_.isOpen()) break;

        const float frameDt = std::min(clock.restart().asSeconds(), cfg::loop::maxFrame);
        update(frameDt);
        render();
    }
    save();
    return 0;
}

}  // namespace sb
