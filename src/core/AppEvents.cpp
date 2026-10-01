// "?" stop events (2026-10-01): a stranger with deals for gold. See
// progression/Events.hpp and cfg::event.

#include <algorithm>
#include <string>
#include <vector>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"

namespace sb {

bool App::eventDealOk(EventKind k) const {
    const RunState& r = data_.run;
    const int gold = eventGoldOf(k);
    if (!eventPays(k) && r.gold < gold) return false;
    switch (k) {
        case EventKind::Drifter: return runBallCount() < ballCap();
        case EventKind::Smith:
            for (const BallLoadout& L : r.balls)
                for (int sl = 0; sl < kLoadoutSlots; ++sl)
                    if (L.kindAt(sl) >= 0 && L.levelAt(sl) < forgeCap()) return true;
            return false;
        default: return true;
    }
}

// Roll the deals: only ones that could apply (a ball needs room, the smith
// needs an item to level), regardless of the gold you hold - a deal you can't
// afford still shows, greyed out, so you know what you missed.
bool App::openEvent() {
    std::vector<EventKind> pool;
    for (int i = 0; i < kEventKindCount; ++i) {
        const auto k = static_cast<EventKind>(i);
        if (k == EventKind::Drifter && runBallCount() >= ballCap()) continue;
        if (k == EventKind::Smith) {
            bool any = false;
            for (const BallLoadout& L : data_.run.balls)
                for (int sl = 0; sl < kLoadoutSlots; ++sl)
                    any = any || (L.kindAt(sl) >= 0 && L.levelAt(sl) < forgeCap());
            if (!any) continue;
        }
        pool.push_back(k);
    }
    if (static_cast<int>(pool.size()) < cfg::event::deals) return false;
    for (int i = static_cast<int>(pool.size()) - 1; i > 0; --i)
        std::swap(pool[static_cast<std::size_t>(i)], pool[static_cast<std::size_t>(rng_.irange(0, i))]);
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
            world_.addCoreMaxHp(-world_.core().maxHp * cfg::event::bloodHpFrac);
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
    }
    save();
}

void App::leaveEvent() {
    back();
    audio_.letGo();
    openMap();
}

}  // namespace sb
