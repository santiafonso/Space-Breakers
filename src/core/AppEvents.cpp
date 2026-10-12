// "?" stop events (2026-10-01): a stranger with deals for gold. See
// progression/Events.hpp and cfg::event.

#include <algorithm>
#include <string>
#include <vector>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {
bool levelableItem(const BallLoadout& L, int sl, int cap) {   // an item the forge can raise (a Cocoon can't)
    return L.kindAt(sl) >= 0 && L.kindAt(sl) != static_cast<int>(UpgradeKind::Cocoon) && L.levelAt(sl) < cap;
}
}  // namespace

bool App::eventApplies(EventKind k) const {
    const RunState& r = data_.run;
    switch (k) {
        case EventKind::Drifter: return runBallCount() + static_cast<int>(r.bench.size()) < ballCap();
        case EventKind::Ascend:
            for (const BallLoadout& L : r.balls)
                if (shrineAscends(L)) return true;
            return false;
        case EventKind::Expand:
            for (const BallLoadout& L : r.balls)
                if (shrineExpands(L)) return true;
            return false;
        case EventKind::Smith:
        case EventKind::Temper:
            for (const BallLoadout& L : r.balls)
                for (int sl = 0; sl < kLoadoutSlots; ++sl)
                    if (levelableItem(L, sl, forgeCap(sl))) return true;
            return false;
        case EventKind::BloodForge:
            for (const BallLoadout& L : r.balls)
                for (int sl = 0; sl < kBallSlots; ++sl)
                    if (levelableItem(L, sl, forgeCap(sl))) return true;
            return false;
        case EventKind::Barter:
            for (const BallLoadout& L : r.balls)
                for (int sl = 0; sl < kBallSlots; ++sl)
                    if (L.gear[sl] >= 0 && L.gear[sl] != static_cast<int>(UpgradeKind::Cocoon)) return true;
            return false;
        case EventKind::Mercenary: return runBallCount() >= 2;
        case EventKind::Cocoon: return anyItemOfTier(Tier::Legendary);   // only once a Legendary could hatch out of it
        case EventKind::Purify: return !pactsWithPrice().empty();
        case EventKind::Scavenge: return anyItemOfTier(Tier::Common) || anyItemOfTier(Tier::Uncommon);
        case EventKind::FieldMedic: {   // a hurt core, and an item with a level to give
            if (world_.core().hp >= world_.core().maxHp - 0.5f) return false;
            for (const BallLoadout& L : r.balls)
                for (int sl = 0; sl < kBallSlots; ++sl)
                    if (L.gear[sl] >= 0 && L.gear[sl] != static_cast<int>(UpgradeKind::Cocoon) && L.gearLvl[sl] >= 2) return true;
            return false;
        }
        case EventKind::BloodRite:   // an ability that can still level up
            for (const BallLoadout& L : r.balls)
                for (int sl = 0; sl < kLoadoutSlots; ++sl)
                    if (L.kindAt(sl) >= 0 && upgradeCat(static_cast<UpgradeKind>(L.kindAt(sl))) == UpgradeCat::Ability &&
                        L.levelAt(sl) < forgeCap(sl))
                        return true;
            return false;
        default: return true;
    }
}

bool App::eventDealOk(EventKind k) const {
    if (!eventFree(k) && !eventPays(k) && data_.run.gold < eventGoldOf(k)) return false;
    return eventApplies(k);
}

// Roll the deals: only ones that could apply (a ball needs room, the smith
// needs an item to level), regardless of the gold you hold - a deal you can't
// afford still shows, greyed out, so you know what you missed.
bool App::openEvent() {
    std::vector<EventKind> pool;
    for (int i = 0; i < kEventKindCount; ++i) {
        const auto k = static_cast<EventKind>(i);
        if (k == EventKind::Purify && rng_.irange(0, 99) >= cfg::event::purifyPct) continue;   // rare
        if (k == EventKind::Scavenge && data_.run.map.act > cfg::event::scavengeUntilAct &&
            rng_.irange(0, 99) >= cfg::event::scavengeLatePct)
            continue;   // past act 1 items come easier: the free one turns up less
        if (eventApplies(k)) pool.push_back(k);
    }
    if (static_cast<int>(pool.size()) < cfg::event::deals) return false;
    for (int i = static_cast<int>(pool.size()) - 1; i > 0; --i)
        std::swap(pool[static_cast<std::size_t>(i)], pool[static_cast<std::size_t>(rng_.irange(0, i))]);
    // Early on items are scarce: up to cfg::event::scavengeUntilAct a free item deal is always one of them.
    if (data_.run.map.act <= cfg::event::scavengeUntilAct)
        if (const auto it = std::find(pool.begin(), pool.end(), EventKind::Scavenge); it != pool.end())
            std::iter_swap(pool.begin(), it);
    pool.resize(static_cast<std::size_t>(cfg::event::deals));
    eventDeals_ = pool;
    push(ScreenId::Event);
    return true;
}

void App::takeEventDeal(int idx) {
    if (idx < 0 || idx >= static_cast<int>(eventDeals_.size())) return;
    const EventKind k = eventDeals_[static_cast<std::size_t>(idx)];
    if (!eventDealOk(k)) return;
    RunState& r = data_.run;
    const int gold = eventGoldOf(k);
    const sf::Vector2f mid{size().x * 0.5f, size().y * 0.4f};
    back();   // close the event; whatever follows opens on the Play screen
    switch (k) {
        case EventKind::Drifter:
            r.gold -= gold;
            applyUpgradeKind(UpgradeKind::AddBall);
            effects_.addLabel("a new ball joins", mid, theme::ballMid, 26, 1.4f);
            audio_.purchase();
            openMap();
            break;
        case EventKind::Smuggler:   // the item pick; its Choice goes back to the map
            r.gold -= gold;
            audio_.purchase();
            openChoice(RollSource::Elite);
            choiceTitle_ = "Smuggler - choose an item";
            break;
        case EventKind::BloodPrice:
            r.gold += gold;
            world_.woundCore(world_.core().maxHp * cfg::event::bloodHpFrac);   // damage, not max HP (2026-10-12)
            effects_.flash(theme::coreLow, 0.5f);
            effects_.addLabel("+" + std::to_string(gold) + " gold", mid, theme::puGolden, 26, 1.4f);
            audio_.purchase();
            openMap();
            break;
        case EventKind::Tithe: {
            r.gold -= gold;
            const float add = world_.core().maxHp * cfg::event::titheHpFrac;
            world_.addCoreMaxHp(add);
            world_.repairCore(add);
            effects_.flash(theme::core, 0.5f);
            effects_.addLabel("core strengthened", mid, theme::core, 26, 1.4f);
            audio_.purchase();
            openMap();
            break;
        }
        case EventKind::Gamble: {
            r.gold -= gold;
            const bool win = rng_.irange(0, 99) < cfg::event::gambleWinPct;
            if (win) {
                const int prize = static_cast<int>(static_cast<float>(gold) * cfg::event::gambleWinMul + 0.5f);
                r.gold += prize;
                effects_.flash(theme::puGolden, 0.6f);
                effects_.addLabel("HEADS  +" + std::to_string(prize) + " gold", mid, theme::puGolden, 28, 1.6f);
                audio_.purchase();
            } else {
                effects_.addLabel("tails - the coin is gone", mid, theme::textLo, 24, 1.6f);
                audio_.letGo();
            }
            openMap();
            break;
        }
        case EventKind::Smith:
            r.gold -= gold;
            audio_.purchase();
            if (!openForgePicker()) openMap();
            break;
        case EventKind::Barter:   // pick what goes; two random items come back (confirmEquip)
            beginEquip(EquipSource::Barter, UpgradeKind::AddBall, -1);
            break;
        case EventKind::Mercenary: {   // a random ball leaves for a while; an item pick for it
            const int i = rng_.irange(0, runBallCount() - 1);
            RunState::Benched bn;
            bn.ball = r.balls[static_cast<std::size_t>(i)];
            bn.fights = cfg::event::mercenaryFights;
            bn.slot = i;
            r.bench.push_back(bn);
            r.balls.erase(r.balls.begin() + i);
            world_.trimBalls(runBallCount());
            syncWorldBalls();
            rememberClasses();
            effects_.addLabel("ball " + std::to_string(i + 1) + " leaves for " + std::to_string(bn.fights) + " fights", mid,
                              theme::textLo, 24, 1.6f);
            audio_.purchase();
            openChoice(RollSource::Elite);
            choiceTitle_ = "Recruiter - choose an item";
            break;
        }
        case EventKind::Gift: {   // a random modifier on a random ball
            const int ball = rng_.irange(0, runBallCount() - 1);
            const auto mod = static_cast<UpgradeKind>(static_cast<int>(UpgradeKind::HeavyImpact) + rng_.irange(0, kModifierCount - 1));
            applyUpgradeKind(mod, ball, -1);
            effects_.addLabel(std::string(upgradeInfo(mod).title) + "  on ball " + std::to_string(ball + 1), mid, theme::ballMid, 26, 1.6f);
            effects_.flash(theme::ballMid, 0.35f);
            audio_.purchase();
            openMap();
            break;
        }
        case EventKind::Tome: {   // a random ability you can have, to place
            r.gold -= gold;
            const UpgradeCtx c = buildUpgradeCtx();
            std::vector<UpgradeKind> pool;
            for (int i = 0; i < kUpgradeKindCount; ++i) {
                const auto k2 = static_cast<UpgradeKind>(i);
                if (upgradeCat(k2) == UpgradeCat::Ability && !c.isLocked(k2) && abilityUnlocked(k2, data_.meta.unlock)) pool.push_back(k2);
            }
            audio_.purchase();
            if (pool.empty()) { openMap(); break; }
            giftQueue_.push_back(pool[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(pool.size()) - 1))]);
            nextGift();
            break;
        }
        case EventKind::BloodForge: {   // a random item levels up; the core pays
            std::vector<std::pair<int, int>> can;
            for (int b = 0; b < runBallCount(); ++b)
                for (int sl = 0; sl < kBallSlots; ++sl)
                    if (levelableItem(r.balls[static_cast<std::size_t>(b)], sl, forgeCap(sl))) can.push_back({b, sl});
            if (!can.empty()) {
                const auto [b, sl] = can[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(can.size()) - 1))];
                BallLoadout& L = r.balls[static_cast<std::size_t>(b)];
                const int lvl = L.levelUp(sl);
                audio_.levelUp(lvl);
                effects_.addLabel(std::string(upgradeInfo(static_cast<UpgradeKind>(L.gear[sl])).title) + "  Lv " + std::to_string(lvl),
                                  mid, tierColor(upgradeTier(static_cast<UpgradeKind>(L.gear[sl]))), 26, 1.6f);
                world_.woundCore(world_.core().maxHp * cfg::event::bloodForgeHpFrac);
                effects_.flash(theme::coreLow, 0.5f);
                syncWorldBalls();
            }
            openMap();
            break;
        }
        case EventKind::Cocoon:
            giftQueue_.push_back(UpgradeKind::Cocoon);
            nextGift();
            break;
        case EventKind::DarkBargain:   // the core pays for an item pick
            world_.woundCore(world_.core().maxHp * cfg::event::darkHpFrac);
            effects_.flash(theme::coreLow, 0.5f);
            audio_.purchase();
            openChoice(RollSource::Elite);
            choiceTitle_ = "Dark bargain - choose an item";
            break;
        case EventKind::FieldMedic: {   // an item gives a level, the core is patched
            std::vector<std::pair<int, int>> can;
            for (int b = 0; b < runBallCount(); ++b)
                for (int sl = 0; sl < kBallSlots; ++sl) {
                    const BallLoadout& L = r.balls[static_cast<std::size_t>(b)];
                    if (L.gear[sl] >= 0 && L.gear[sl] != static_cast<int>(UpgradeKind::Cocoon) && L.gearLvl[sl] >= 2) can.push_back({b, sl});
                }
            if (!can.empty()) {
                const auto [b, sl] = can[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(can.size()) - 1))];
                BallLoadout& L = r.balls[static_cast<std::size_t>(b)];
                L.gearLvl[sl] -= 1;
                effects_.addLabel(std::string(upgradeInfo(static_cast<UpgradeKind>(L.gear[sl])).title) + "  Lv " +
                                      std::to_string(L.gearLvl[sl]), mid + sf::Vector2f{0.f, 40.f}, theme::textLo, 22, 1.6f);
                world_.repairCore(world_.core().maxHp * cfg::event::medicHealFrac);
                r.repairedThisAct = true;   // a repair you chose: "Iron core" is off
                effects_.flash(theme::core, 0.5f);
                effects_.addLabel("core repaired", mid, theme::core, 26, 1.4f);
                syncWorldBalls();
            }
            audio_.purchase();
            openMap();
            break;
        }
        case EventKind::BloodRite: {   // an ability levels up, the core pays
            std::vector<std::pair<int, int>> can;
            for (int b = 0; b < runBallCount(); ++b)
                for (int sl = 0; sl < kLoadoutSlots; ++sl) {
                    const BallLoadout& L = r.balls[static_cast<std::size_t>(b)];
                    if (L.kindAt(sl) >= 0 && upgradeCat(static_cast<UpgradeKind>(L.kindAt(sl))) == UpgradeCat::Ability &&
                        L.levelAt(sl) < forgeCap(sl))
                        can.push_back({b, sl});
                }
            if (!can.empty()) {
                const auto [b, sl] = can[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(can.size()) - 1))];
                BallLoadout& L = r.balls[static_cast<std::size_t>(b)];
                const int lvl = L.levelUp(sl);
                audio_.levelUp(lvl);
                effects_.addLabel(std::string(upgradeInfo(static_cast<UpgradeKind>(L.kindAt(sl))).title) + "  Lv " + std::to_string(lvl),
                                  mid, theme::accent, 26, 1.6f);
                world_.woundCore(world_.core().maxHp * cfg::event::riteHpFrac);
                effects_.flash(theme::coreLow, 0.5f);
                syncWorldBalls();
            }
            openMap();
            break;
        }
        case EventKind::Scavenge: {   // a free item pick, nothing rare
            std::vector<UpgradeKind> taken;
            choiceCount_ = pickCards();
            for (int i = 0; i < choiceCount_; ++i) {
                const UpgradeKind c = rollPick(RollSource::Shop, taken, [](UpgradeKind u) {
                    return upgradeCat(u) == UpgradeCat::Item && upgradeTier(u) <= Tier::Uncommon;
                });
                if (upgradeCat(c) != UpgradeCat::Item || std::find(taken.begin(), taken.end(), c) != taken.end()) break;
                choices_[static_cast<std::size_t>(taken.size())] = c;
                taken.push_back(c);
            }
            choiceCount_ = static_cast<int>(taken.size());
            if (choiceCount_ == 0) { openMap(); break; }
            rollSource_ = RollSource::Shop;
            starterChoice_ = false;
            choiceTitle_ = "Scavenge - choose an item";
            audio_.purchase();
            push(ScreenId::Choice);
            break;
        }
        case EventKind::Purify: {   // a pact keeps its gift and loses its price
            r.gold -= gold;
            const std::vector<PactId> v = pactsWithPrice();
            if (!v.empty()) liftPactPrice(v[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(v.size()) - 1))]);
            audio_.purchase();
            openMap();
            break;
        }
        case EventKind::Rest:
            restRepair();
            openMap();
            break;
        case EventKind::Temper:
            audio_.purchase();
            if (!openForgePicker()) openMap();
            break;
        case EventKind::Ascend: beginEquip(EquipSource::Ascend, UpgradeKind::Satellite, -1); break;
        case EventKind::Expand: beginEquip(EquipSource::Expand, UpgradeKind::Satellite, -1); break;
    }
    save();
}

// The Rest stop: repair the core, or level up an item at the forge. With
// nothing to forge it just repairs, as before.
void App::openRestStop() {
    if (!eventDealOk(EventKind::Temper)) {
        restRepair();
        openMap();
        return;
    }
    eventDeals_ = {EventKind::Rest, EventKind::Temper};
    push(ScreenId::Event);
}

void App::restRepair() {
    const sf::Vector2f mid{size().x * 0.5f, size().y * 0.4f};
    playerRepair(halfRepair() ? world_.core().maxHp * cfg::creed::fortressRestHeal : 1e9f);
    audio_.purchase();
    effects_.flash(theme::core, 0.5f);
    effects_.addLabel(halfRepair() ? "Core half repaired" : "Core repaired", mid, theme::core, 26, 1.2f);
}

// Event picks, one Equip screen each; then the map.
void App::nextGift() {
    if (giftQueue_.empty()) { openMap(); return; }
    const UpgradeKind k = giftQueue_.front();
    giftQueue_.erase(giftQueue_.begin());
    beginEquip(EquipSource::Gift, k, -1);
}

// A Barter's haul: an item of a random rarity (weighted) that fits some ball.
bool App::anyItemOfTier(Tier t) const {
    const UpgradeCtx c = buildUpgradeCtx();
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        if (upgradeCat(k) != UpgradeCat::Item || upgradeTier(k) != t || c.isLocked(k)) continue;
        for (const BallLoadout& L : data_.run.balls)
            if (upgradeFitsBall(k, L)) return true;
    }
    return false;
}

int App::randomItemOfTier(Tier t, const BallLoadout* fits) {
    const UpgradeCtx c = buildUpgradeCtx();
    std::vector<int> pool;
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        if (upgradeCat(k) != UpgradeCat::Item || upgradeTier(k) != t || c.isLocked(k)) continue;
        bool ok = false;
        if (fits) ok = upgradeFitsBall(k, *fits);
        else
            for (const BallLoadout& L : data_.run.balls) ok = ok || upgradeFitsBall(k, L);
        if (ok) pool.push_back(i);
    }
    if (pool.empty()) return -1;
    return pool[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(pool.size()) - 1))];
}

// A fight won: Cocoons count a fight down (their level) and hatch at zero;
// balls away with the Recruiter count down too and come back.
void App::afterFight() {
    RunState& r = data_.run;
    for (int i = 0; i < static_cast<int>(r.balls.size()); ++i)   // Tiamat's dragons keep what they grew (before the bench moves balls about)
        for (int kind = 0; kind < 6; ++kind)
            r.balls[static_cast<std::size_t>(i)].dragonXp[kind] = std::max(r.balls[static_cast<std::size_t>(i)].dragonXp[kind], world_.tiamatXp(i, kind));
    const sf::Vector2f mid{size().x * 0.5f, size().y * 0.3f};
    bool changed = false;
    for (BallLoadout& L : r.balls)
        for (int sl = 0; sl < kBallSlots; ++sl) {
            if (L.gear[sl] != static_cast<int>(UpgradeKind::Cocoon)) continue;
            changed = true;
            if (--L.gearLvl[sl] > 0) continue;
            const Tier t = rng_.irange(0, 99) < cfg::event::cocoonLegendaryPct ? Tier::Legendary : Tier::Epic;
            L.gear[sl] = -1;   // out of the way, so what hatches is judged on the rest
            L.gearLvl[sl] = 0;
            int k = randomItemOfTier(t, &L);
            if (k < 0) k = randomItemOfTier(Tier::Epic, &L);
            if (k < 0) continue;   // nothing fits: it crumbles
            L.setSlot(sl, k, 1);
            effects_.flash(tierColor(t), 0.7f);
            effects_.addLabel(std::string("the cocoon hatches:  ") + upgradeInfo(static_cast<UpgradeKind>(k)).title, mid, tierColor(t),
                              30, 2.f);
        }
    for (auto it = r.bench.begin(); it != r.bench.end();) {
        if (--it->fights > 0) { ++it; continue; }
        const int at = std::min(it->slot, runBallCount());
        r.balls.insert(r.balls.begin() + at, it->ball);
        effects_.addLabel("ball " + std::to_string(at + 1) + " is back", mid + sf::Vector2f{0.f, 40.f}, theme::ballMid, 24, 1.6f);
        it = r.bench.erase(it);
        changed = true;
    }
    if (changed) {
        syncWorldBalls();
        rememberClasses();
    }
}

void App::leaveEvent() {
    back();
    audio_.letGo();
    openMap();
}

}  // namespace sb
