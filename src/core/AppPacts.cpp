// App: pacts, the run intro and the shop extras (Fase O). Split from App.cpp
// so the run-flow additions sit in one place.

#include <algorithm>
#include <cmath>
#include <numeric>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "ui/PactScreen.hpp"
#include "ui/Widgets.hpp"

namespace sb {

// ---------------------------------------------------------------- pacts: params

void App::foldPacts(WorldParams& p) const {
    namespace P = cfg::pact;
    for (int raw : data_.run.pacts) {
        switch (static_cast<PactId>(raw)) {
            case PactId::HotHands:
                p.cruiseMult *= P::hotCruise;
                p.pact.speedCeilMul *= P::hotCeil;
                p.pact.flingHold *= P::hotHold;
                break;
            case PactId::Nova: break;   // an active ability (usePactAbility) + a smaller core
            case PactId::Hunters:
                p.pact.hunters = true;
                p.damageMult *= P::huntDamage;
                break;
            case PactId::Clockwork:
                p.pact.autoFlingEvery = P::clockEvery;
                p.pact.autoFlingSpeed = P::clockSpeed;
                p.damageMult *= P::clockDamage;
                break;
            case PactId::Pinball: p.pact.pinball = true; break;
            case PactId::Duet:
                p.damageMult *= P::duetDamage;
                p.ballRadiusMult *= P::duetRadius;
                break;
            case PactId::Legion:
                p.pact.legion = true;
                p.damageMult *= P::legionDamage;
                break;
            case PactId::LivingCore:
                p.pact.livingCore = true;
                p.pact.enemySpeedMul *= P::coreEnemySpeed;
                break;
            case PactId::Fortress: p.pact.fortress = true; break;
            case PactId::LoadedDice: break;   // its luck is counted in App::luck
            case PactId::Alchemy:
                p.pact.alchemy = true;
                p.damageMult *= P::alchemyDamage;
                break;
            case PactId::Bloodlust:
                p.pact.bloodlust = true;
                p.pact.coreDamageMul *= P::bloodCoreDamage;
                break;
        }
    }
}

bool App::canGrab() const { return !hasPact(PactId::Hunters) && !hasPact(PactId::Clockwork); }

float App::flingPower() const {
    float k = data_.run.mods.strongArm ? cfg::combat::flingPowerBoost : 1.f;
    if (hasPact(PactId::HotHands)) k *= cfg::pact::hotFling;
    if (hasPact(PactId::Pinball)) k *= cfg::pact::pinFling;
    return k;
}

void App::usePactAbility() {
    if (!hasPact(PactId::Nova) || !data_.run.active || !simulating() || novaCd_ > 0.f) return;
    world_.pactNova(params());
    novaCd_ = cfg::pact::novaCooldown;
    effects_.addBurst(world_.core().pos, cfg::pact::novaRadius * world_.arenaScale(), theme::accent);
    effects_.flash(theme::accent, 0.35f);
    camKick_ = std::max(camKick_, 6.f);
    audio_.thrown(1.f);
}

// ---------------------------------------------------------------- pacts: flow

bool App::openPactChoice(PactSource src) {
    const RunState& r = data_.run;
    const int* u = data_.meta.unlock;
    if (static_cast<int>(r.pacts.size()) >= kMaxPacts) return false;

    std::vector<PactId> pool;
    for (int i = 0; i < kPactCount; ++i) {
        const auto id = static_cast<PactId>(i);
        const PactDef& d = pactDef(id);
        if (d.unlockNode >= 0 && u[d.unlockNode] == 0) continue;   // still behind its web node
        bool ok = !r.hasPact(id);
        for (int have : r.pacts)
            if (pactsConflict(id, static_cast<PactId>(have))) ok = false;
        if (ok) pool.push_back(id);
    }
    if (pool.empty()) return false;
    for (int i = static_cast<int>(pool.size()) - 1; i > 0; --i)
        std::swap(pool[static_cast<std::size_t>(i)], pool[static_cast<std::size_t>(rng_.irange(0, i))]);

    // Spread the offer over different archetypes first, so the three cards
    // are three different ways to play; fill up from the rest after.
    const int want = cfg::pact::offered + (u[MetaOath] > 0 ? 1 : 0);
    pactChoices_.clear();
    for (PactId id : pool) {
        if (static_cast<int>(pactChoices_.size()) >= want) break;
        bool fresh = true;
        for (PactId c : pactChoices_)
            if (pactDef(c).archetype == pactDef(id).archetype) fresh = false;
        if (fresh) pactChoices_.push_back(id);
    }
    for (PactId id : pool) {
        if (static_cast<int>(pactChoices_.size()) >= want) break;
        if (std::find(pactChoices_.begin(), pactChoices_.end(), id) == pactChoices_.end()) pactChoices_.push_back(id);
    }
    pactSrc_ = src;
    world_.forceRelease();
    setAiming(false);
    push(ScreenId::Pact);
    return true;
}

void App::continueAfterPact() {
    if (pactSrc_ == PactSource::Boss) openChoice(RollSource::Boss);   // then the boss treasure
    else if (introStep_ >= 0) advanceRunIntro();
}

void App::choosePact(int idx) {
    if (idx < 0 || idx >= static_cast<int>(pactChoices_.size())) return;
    const PactId id = pactChoices_[static_cast<std::size_t>(idx)];
    audio_.cardPick();
    back();   // close the pact screen
    grantPact(id);
    continueAfterPact();
    save();
}

void App::refusePacts() {
    back();
    data_.run.gold += cfg::pact::refuseGold;
    effects_.addLabel("+" + std::to_string(cfg::pact::refuseGold) + " gold", {size().x * 0.5f, size().y * 0.4f},
                      theme::puGolden, 26, 1.2f);
    audio_.purchase();
    continueAfterPact();
}

void App::grantPact(PactId id) {
    RunState& r = data_.run;
    if (r.hasPact(id)) return;
    r.pacts.push_back(static_cast<int>(id));
    switch (id) {
        case PactId::Nova:
            world_.addCoreMaxHp(-world_.core().maxHp * (1.f - cfg::pact::novaCoreHp));
            novaCd_ = 0.f;
            break;
        case PactId::Fortress:
            world_.addCoreMaxHp(world_.core().maxHp * (cfg::pact::fortressHp - 1.f));
            break;
        case PactId::Duet:   applyDuet(); break;
        case PactId::Legion: applyLegion(); break;
        default: break;
    }
    syncWorldBalls();
    const sf::Color col = pactColor(pactDef(id).archetype);
    effects_.flash(col, 0.8f);
    effects_.addLabel(std::string("PACT  ") + pactDef(id).name, {size().x * 0.5f, size().y * 0.1f}, col, 34, 1.8f);
    audio_.comboUp(cfg::combo::baseCapTier);
}

// "Duet": keep the two most built-up balls; everything the others carried
// flows into them - items become forge levels, modifiers move over.
void App::applyDuet() {
    RunState& r = data_.run;
    const int n = static_cast<int>(r.balls.size());
    const int keepN = cfg::pact::duetBalls;
    if (n <= keepN) return;
    auto score = [](const BallLoadout& L) {
        int s = 0;
        for (int i = 0; i < kBallSlots; ++i)
            if (L.gear[i] >= 0) s += 3 + L.gearLvl[i];
        for (int m : L.mods) s += m;
        return s;
    };
    std::vector<int> order(static_cast<std::size_t>(n));
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return score(r.balls[static_cast<std::size_t>(a)]) > score(r.balls[static_cast<std::size_t>(b)]);
    });
    std::vector<int> kept(order.begin(), order.begin() + keepN);
    std::sort(kept.begin(), kept.end());
    std::vector<BallLoadout> keep;
    for (int i : kept) keep.push_back(r.balls[static_cast<std::size_t>(i)]);

    int gold = 0, turn = 0, levels = 0;
    for (int i = 0; i < n; ++i) {
        if (std::find(kept.begin(), kept.end(), i) != kept.end()) continue;
        const BallLoadout& L = r.balls[static_cast<std::size_t>(i)];
        for (int m = 0; m < kModifierCount; ++m)
            keep[static_cast<std::size_t>(turn++ % keepN)].mods[m] += L.mods[m];
        for (int sl = 0; sl < kBallSlots; ++sl) {
            if (L.gear[sl] < 0) continue;
            std::vector<std::pair<int, int>> spots;   // kept (ball, slot) that can still level
            for (int b = 0; b < keepN; ++b)
                for (int s2 = 0; s2 < kBallSlots; ++s2)
                    if (keep[static_cast<std::size_t>(b)].gear[s2] >= 0 &&
                        keep[static_cast<std::size_t>(b)].gearLvl[s2] < kMaxItemLevel)
                        spots.push_back({b, s2});
            if (spots.empty()) { gold += cfg::pact::duetMeltGold; continue; }
            const auto [b, s2] = spots[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(spots.size()) - 1))];
            ++keep[static_cast<std::size_t>(b)].gearLvl[s2];
            ++levels;
        }
    }
    r.balls = keep;
    r.gold += gold;
    world_.trimBalls(keepN);
    std::string msg = "absorbed: +" + std::to_string(levels) + " forge levels";
    if (gold > 0) msg += "  +" + std::to_string(gold) + " gold";
    effects_.addLabel(msg, {size().x * 0.5f, size().y * 0.1f + 40.f}, theme::textHi, 20, 2.0f);
}

// "Legion": two more balls with an item each; a full arena gets modifier
// stacks instead.
void App::applyLegion() {
    RunState& r = data_.run;
    for (int k = 0; k < cfg::pact::legionBalls; ++k) {
        if (static_cast<int>(r.balls.size()) < cfg::ball::maxBalls) {
            BallLoadout L;
            const int item = randomItemFor(L, Tier::Rare);
            if (item >= 0) { L.gear[0] = item; L.gearLvl[0] = 1; }
            r.balls.push_back(L);
        } else {
            for (BallLoadout& L : r.balls) ++L.mods[rng_.irange(0, kModifierCount - 1)];
        }
    }
}

int App::randomItemFor(const BallLoadout& b, Tier maxTier) {
    const UpgradeCtx c = buildUpgradeCtx();
    std::vector<int> pool;
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        if (upgradeCat(k) != UpgradeCat::Item || upgradeTier(k) > maxTier) continue;
        if ((c.locked & upgradeBit(k)) || !upgradeFitsBall(k, b)) continue;
        if (k == UpgradeKind::Shatter && !c.elemUnlocked[3]) continue;   // needs ice
        pool.push_back(i);
    }
    if (pool.empty()) return -1;
    return pool[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(pool.size()) - 1))];
}

void App::devTogglePact(PactId id) {
    if (!devMode() || !data_.run.active) return;
    auto& v = data_.run.pacts;
    const auto it = std::find(v.begin(), v.end(), static_cast<int>(id));
    if (it != v.end()) {   // dev only: drops the rule, one-off effects (balls, core HP) stay
        v.erase(it);
        syncWorldBalls();
        effects_.addLabel(std::string("- ") + pactDef(id).name, {size().x * 0.5f, size().y * 0.9f}, theme::textLo, 18, 0.9f);
        return;
    }
    grantPact(id);
}

// ---------------------------------------------------------------- run intro

std::string App::choiceTitle() const {
    if (!choiceTitle_.empty()) return choiceTitle_;
    return rollSource_ == RollSource::Boss ? "Boss treasure - choose one" : "Choose one";
}

void App::afterChoice() {
    if (introStep_ >= 0) advanceRunIntro();
    else openMap();
}

// newRun's opening beats, in order; each step may open a screen and comes
// back here when it closes. Ends on the map.
void App::advanceRunIntro() {
    const int* u = data_.meta.unlock;
    if (introStep_ == 0) {
        introStep_ = 1;
        if (u[MetaCovenant] > 0 && openPactChoice(PactSource::Start)) return;   // "Covenant"
    }
    if (introStep_ == 1) {
        introStep_ = 2;
        if (u[MetaQuartermaster] > 0 && u[MetaStarterKit] > 0 && openStarterChoice()) return;   // "Quartermaster"
    }
    introStep_ = -1;
    openMap();
}

// "Quartermaster": the Starter kit's free item, picked from 4 cards of its tier.
bool App::openStarterChoice() {
    const RunState& r = data_.run;
    if (r.balls.empty()) return false;
    const Tier want = data_.meta.unlock[MetaStarterKit] >= 2 ? Tier::Rare : Tier::Uncommon;
    const UpgradeCtx c = buildUpgradeCtx();
    std::vector<UpgradeKind> pool;
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        if (upgradeCat(k) == UpgradeCat::Item && upgradeTier(k) == want && !(c.locked & upgradeBit(k)) &&
            upgradeFitsBall(k, r.balls[0]) && !(k == UpgradeKind::Shatter && !c.elemUnlocked[3]))   // Shatter needs ice
            pool.push_back(k);
    }
    if (static_cast<int>(pool.size()) < kChoiceCount) return false;
    for (int i = static_cast<int>(pool.size()) - 1; i > 0; --i)
        std::swap(pool[static_cast<std::size_t>(i)], pool[static_cast<std::size_t>(rng_.irange(0, i))]);
    for (int i = 0; i < kChoiceCount; ++i) choices_[static_cast<std::size_t>(i)] = pool[static_cast<std::size_t>(i)];
    rollSource_ = RollSource::Normal;
    choiceTitle_ = std::string("Starter kit - pick your ") + tierName(want) + " item";
    push(ScreenId::Choice);
    return true;
}

// ---------------------------------------------------------------- shop extras

int App::forgeCap() const { return kMaxItemLevel; }   // Fase N lifted every item to Lv5, Duet included

int App::saleOffPercent() const {
    const float off = cfg::gold::saleOff + cfg::meta::merchantSalePerLevel * static_cast<float>(data_.meta.unlock[MetaMerchant]);
    return static_cast<int>(std::lround(off * 100.f));
}

int App::shopOfferPrice(int i) const {
    const RunState& r = data_.run;
    if (i < 0 || i >= static_cast<int>(r.shopOffers.size())) return 0;
    const int deal = i < static_cast<int>(r.shopDeal.size()) ? r.shopDeal[static_cast<std::size_t>(i)] : 0;
    if (deal == 2) return 0;   // a revealed mystery box: already paid for
    const int base = shopPrice(static_cast<UpgradeKind>(r.shopOffers[static_cast<std::size_t>(i)]));
    if (deal == 1)
        return std::max(1, static_cast<int>(std::lround(static_cast<float>(base) * (1.f - static_cast<float>(saleOffPercent()) / 100.f))));
    return base;
}

int App::mysteryPrice() const {
    const float off = cfg::meta::hagglerPerLevel * static_cast<float>(data_.meta.unlock[MetaHaggler]);
    return std::max(1, static_cast<int>(std::lround(static_cast<float>(cfg::gold::mysteryPrice) * (1.f - off))));
}

// Stock the shelves: shopOffers (+1 with "Merchant"), one of them on sale.
// A revealed mystery pick that hasn't been taken yet stays on the shelf.
void App::rollShopOffers() {
    RunState& r = data_.run;
    std::vector<int> prepaid;
    for (std::size_t i = 0; i < r.shopOffers.size(); ++i)
        if (i < r.shopDeal.size() && r.shopDeal[i] == 2 && !r.shopSold[i]) prepaid.push_back(r.shopOffers[i]);

    std::vector<UpgradeKind> taken;
    for (int k : prepaid) taken.push_back(static_cast<UpgradeKind>(k));
    const int count = cfg::gold::shopOffers + (data_.meta.unlock[MetaMerchant] > 0 ? 1 : 0);
    std::vector<UpgradeKind> fresh;
    for (int i = 0; i < count; ++i) {
        const UpgradeKind k = rollPick(RollSource::Normal, taken);
        if (std::find(taken.begin(), taken.end(), k) != taken.end()) break;   // pool ran dry
        taken.push_back(k);
        fresh.push_back(k);
    }
    r.shopOffers.clear();
    for (UpgradeKind k : fresh) r.shopOffers.push_back(static_cast<int>(k));
    r.shopDeal.assign(r.shopOffers.size(), 0);
    if (!fresh.empty()) r.shopDeal[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(fresh.size()) - 1))] = 1;
    for (int k : prepaid) {
        r.shopOffers.push_back(k);
        r.shopDeal.push_back(2);
    }
    r.shopSold.assign(r.shopOffers.size(), false);
}

void App::buyMystery() {
    RunState& r = data_.run;
    if (r.shopMystery != 1 || r.gold < mysteryPrice()) return;
    r.gold -= mysteryPrice();
    std::vector<UpgradeKind> shown;
    for (int k : r.shopOffers) shown.push_back(static_cast<UpgradeKind>(k));
    const UpgradeKind k = rollPick(RollSource::Elite, shown);   // elite odds: often Rare or better
    r.shopOffers.push_back(static_cast<int>(k));
    r.shopSold.push_back(false);
    r.shopDeal.push_back(2);
    r.shopMystery = 2;
    const sf::Color col = tierColor(upgradeTier(k));
    effects_.flash(col, upgradeTier(k) >= Tier::Epic ? 0.8f : 0.45f);
    effects_.addLabel(std::string(tierName(upgradeTier(k))) + "  " + upgradeInfo(k).title,
                      {size().x * 0.5f, size().y * 0.72f}, col, 28, 1.6f);   // under the shop buttons
    audio_.purchase();
}

int App::shopRerollPrice() const { return cfg::gold::rerollBase + cfg::gold::rerollStep * data_.run.shopRerolls; }

void App::rerollShop() {
    RunState& r = data_.run;
    if (r.gold < shopRerollPrice()) return;
    r.gold -= shopRerollPrice();
    ++r.shopRerolls;
    rollShopOffers();
    audio_.purchase();
    effects_.flash(theme::accent, 0.25f);
}

void App::beginShopForge() {
    if (data_.run.gold < cfg::gold::forgeServicePrice) return;
    equipSrc_ = EquipSource::ShopForge;
    equipRef_ = -1;
    bool any = false;
    for (int b = 0; b < runBallCount(); ++b) any = any || equipFitsBall(b);
    if (!any) {
        effects_.addLabel("nothing to forge yet", {size().x * 0.5f, size().y * 0.14f}, theme::textLo, 20, 1.2f);
        return;
    }
    push(ScreenId::Equip);
}

void App::beginSell() {
    equipSrc_ = EquipSource::Sell;
    equipRef_ = -1;
    bool any = false;
    for (int b = 0; b < runBallCount(); ++b) any = any || equipFitsBall(b);
    if (!any) {
        effects_.addLabel("no items to sell", {size().x * 0.5f, size().y * 0.14f}, theme::textLo, 20, 1.2f);
        return;
    }
    push(ScreenId::Equip);
}

int App::sellValue(int ball, int slot) const {
    if (ball < 0 || ball >= runBallCount() || slot < 0 || slot >= kBallSlots) return 0;
    const BallLoadout& L = data_.run.balls[static_cast<std::size_t>(ball)];
    if (L.gear[slot] < 0) return 0;
    const int base = cfg::gold::priceByTier[static_cast<int>(upgradeTier(static_cast<UpgradeKind>(L.gear[slot])))];
    return std::max(1, static_cast<int>(std::lround(static_cast<float>(base) * cfg::gold::sellFrac *
                                                    static_cast<float>(std::max(1, L.gearLvl[slot])))));
}

}  // namespace sb
