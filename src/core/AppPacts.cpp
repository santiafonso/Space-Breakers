// App: pacts and the Altar (2026-09-28) - the params they fold, the choice
// flow and the hidden Altar path before each boss.

#include <algorithm>
#include <numeric>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"

namespace sb {

// ---------------------------------------------------------------- pacts: params

void App::foldPacts(WorldParams& p) const {
    namespace P = cfg::pact;
    for (int raw : data_.run.pacts) {
        switch (static_cast<PactId>(raw)) {
            case PactId::Lead:
                p.damageMult *= P::leadDamage;
                p.cruiseMult *= P::leadCruise;
                break;
            case PactId::QuickHands:
                p.pact.catchMul *= P::quickCatchMul;
                p.pact.idlePenalty = true;
                break;
            case PactId::HeavyArm: break;    // flingPower / aimSlows
            case PactId::GlassEdge: p.pact.glassEdge = true; break;   // + crit in ballSpecs
            case PactId::Stillness: p.pact.stillness = P::stillnessMax; break;   // weaker quick throws: quickThrowMul
            case PactId::Overflow: p.damageMult *= P::overflowDamage; break;   // the ball came with the grant
            case PactId::Tiny:
                p.damageMult *= P::tinyDamage;
                p.ballRadiusMult *= P::tinyRadius;
                break;
            case PactId::Colossus: break;    // per ball, in ballSpecs
            case PactId::HotPotato: p.pact.hotPotato = true; break;
            case PactId::Juggler: p.pact.juggler = true; break;
            case PactId::VoidWalls: p.pact.voidWalls = true; break;
            case PactId::AnchorWalls: p.pact.anchorWalls = true; break;   // + flingPower
            case PactId::LastBreath: p.pact.lastBreath = true; break;    // the core shrank on the grant
            case PactId::Mirror:
                p.pact.mirror = true;
                p.pact.enemyHpMul *= P::mirrorEnemyHp;
                break;
            case PactId::Elemental:
                for (float& m : p.elemMult) m *= P::elementalMul;
                p.damageMult *= P::elementalContact;
                break;
            case PactId::Frenzy: p.pact.frenzy = true; break;
            case PactId::Blind: break;       // flingPower / aimGuide
            case PactId::Horde: p.pact.enemyCountMul *= P::hordeCount; break;   // the balls come with each boss
        }
    }
}

float App::quickThrowMul() const { return hasPact(PactId::Stillness) ? cfg::pact::stillnessQuick : 1.f; }

// "Colossus": the ball with the most on it (items + their levels + modifiers).
int App::colossusBall() const {
    const auto& balls = data_.run.balls;
    int best = -1, bestScore = -1;
    for (int i = 0; i < static_cast<int>(balls.size()); ++i) {
        const BallLoadout& L = balls[static_cast<std::size_t>(i)];
        int s = 0;
        for (int sl = 0; sl < kLoadoutSlots; ++sl)
            if (L.kindAt(sl) >= 0) s += 3 + L.levelAt(sl);
        for (int m : L.mods) s += m;
        if (s > bestScore) { bestScore = s; best = i; }
    }
    return best;
}

// ---------------------------------------------------------------- pacts: flow

bool App::openPactChoice(bool fromMap) {
    const RunState& r = data_.run;
    std::vector<PactId> pool;
    for (int i = 0; i < kPactCount; ++i) {
        const auto id = static_cast<PactId>(i);
        if (r.hasPact(id)) continue;
        if (pactDef(id).needsHands && !canGrab()) continue;   // Hunters / Clockwork: hands off, nothing to trade
        if (id == PactId::Overflow && static_cast<int>(r.balls.size()) >= ballCap()) continue;
        bool ok = true;
        for (int have : r.pacts)
            if (pactsConflict(id, static_cast<PactId>(have))) ok = false;
        if (ok) pool.push_back(id);
    }
    if (pool.empty()) return false;
    for (int i = static_cast<int>(pool.size()) - 1; i > 0; --i)
        std::swap(pool[static_cast<std::size_t>(i)], pool[static_cast<std::size_t>(rng_.irange(0, i))]);
    pool.resize(std::min<std::size_t>(pool.size(), cfg::pact::offered));
    pactChoices_ = pool;
    pactFromMap_ = fromMap;
    world_.forceRelease();
    setAiming(false);
    push(ScreenId::Altar);
    return true;
}

void App::choosePact(int idx) {
    if (idx < 0 || idx >= static_cast<int>(pactChoices_.size())) return;
    const PactId id = pactChoices_[static_cast<std::size_t>(idx)];
    audio_.cardPick();
    back();   // close the Altar
    grantPact(id);
    if (pactFromMap_) openMap();
    save();
}

void App::refusePacts() {
    back();
    audio_.letGo();
    if (pactFromMap_) openMap();
}

void App::grantPact(PactId id) {
    RunState& r = data_.run;
    if (r.hasPact(id)) return;
    r.pacts.push_back(static_cast<int>(id));
    switch (id) {
        case PactId::Overflow:
            if (static_cast<int>(r.balls.size()) < ballCap()) r.balls.push_back(BallLoadout{});
            break;
        case PactId::LastBreath:
            world_.addCoreMaxHp(-world_.core().maxHp * (1.f - cfg::pact::lastBreathCore));
            break;
        default: break;
    }
    syncWorldBalls();
    effects_.flash(theme::pact, 0.8f);
    effects_.addLabel(std::string("PACT  ") + pactDef(id).name, {size().x * 0.5f, size().y * 0.1f}, theme::pact, 34, 1.8f);
    audio_.comboUp(cfg::combo::baseCapTier);
}

void App::devTogglePact(PactId id) {
    if (!devMode() || !data_.run.active) return;
    auto& v = data_.run.pacts;
    const auto it = std::find(v.begin(), v.end(), static_cast<int>(id));
    if (it != v.end()) {   // dev only: drops the rule, one-off effects (balls, core HP) stay
        v.erase(it);
        syncWorldBalls();
        return;
    }
    grantPact(id);
}

// ---------------------------------------------------------------- the hidden Altar

// A cleared fight (not the boss): flawless ones build the act's streak, one
// that let anything through resets it. Stops in between don't count.
void App::notePactFight(bool flawless) {
    RunState& r = data_.run;
    r.cleanStreak = flawless ? r.cleanStreak + 1 : 0;
    if (r.altarState == 0 && r.cleanStreak >= cfg::map::altarStreak) r.altarState = 1;   // earned, still hidden
}

// Standing on the pre-boss row with the streak earned: a hidden Altar appears
// beside the boss, reached from where you stand and leading on to the boss.
// The map plays its reveal (consumeAltarReveal).
void App::revealAltarPath() {
    RunState& r = data_.run;
    if (!r.active || r.altarState != 1 || r.mapNode < 0 || r.mapRow != r.map.rowCount()) return;
    int boss = -1;
    for (int i = 0; i < static_cast<int>(r.map.nodes.size()); ++i)
        if (r.map.nodes[static_cast<std::size_t>(i)].type == MapNodeType::Boss) boss = i;
    if (boss < 0) return;
    MapNode altar;
    altar.type = MapNodeType::Altar;
    altar.row = r.map.bossRow();
    // Straight up from where you stand, on an outer lane so it keeps clear of
    // the boss in the middle.
    const int here = r.map.nodes[static_cast<std::size_t>(r.mapNode)].lane;
    altar.lane = here < cfg::map::lanes / 2 ? 0 : cfg::map::lanes - 1;
    altar.next.push_back(boss);
    const int id = static_cast<int>(r.map.nodes.size());
    r.map.nodes.push_back(altar);
    r.map.nodes[static_cast<std::size_t>(r.mapNode)].next.push_back(id);
    r.altarState = 2;
    r.altarReveal = true;
}

bool App::consumeAltarReveal() {
    if (!data_.run.altarReveal) return false;
    data_.run.altarReveal = false;
    return true;
}

}  // namespace sb
