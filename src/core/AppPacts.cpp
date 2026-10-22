// App: pacts and the Altar (2026-09-28) - the params they fold, the choice
// flow and the hidden Altar path before each boss.

#include <algorithm>
#include <numeric>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"

namespace sb {

// ---------------------------------------------------------------- pacts: params

// The squad's traits (2026-10-22): each one past cfg::team::traitAt balls, on
// numbers the sim already reads.
void App::foldTraits(WorldParams& p) const {
    namespace T = cfg::team;
    int counts[kTraitCount];
    teamTraitCounts(data_.run.balls, counts);
    for (int t = 0; t < kTraitCount; ++t) {
        const int tier = traitTier(counts[t]);
        if (tier == 0) continue;
        const int i = tier - 1;
        switch (static_cast<Trait>(t)) {
            case Trait::Impact:    p.damageMult *= T::impactDamage[i]; break;
            case Trait::Swift:     p.cruiseMult *= T::swiftCruise[i]; break;
            case Trait::Arcane:    p.creed.castRate *= T::arcaneRate[i]; break;
            case Trait::Elemental:
                for (int e = 1; e < kElementCount; ++e) p.elemMult[e] *= T::elementalPower[i];
                break;
            case Trait::Squad:     p.aegisHits += T::squadAegis[i]; break;
        }
    }
}

void App::foldPacts(WorldParams& p) const {
    namespace P = cfg::pact;
    for (int raw : data_.run.pacts) {
        const auto id = static_cast<PactId>(raw);
        const bool cost = data_.run.pactCost(id);   // "Unbinding" lifts the price, the gift stays
        switch (id) {
            case PactId::Lead:
                p.damageMult *= P::leadDamage;
                if (cost) p.cruiseMult *= P::leadCruise;
                break;
            case PactId::QuickHands:
                p.pact.catchMul *= P::quickCatchMul;
                p.pact.idlePenalty = cost;
                break;
            case PactId::HeavyArm: break;    // flingPower / aimSlows
            case PactId::GlassEdge: p.pact.glassEdge = cost; break;   // + crit in ballSpecs
            case PactId::Stillness: p.pact.stillness = P::stillnessMax; break;   // weaker quick throws: quickThrowMul
            case PactId::Overflow: if (cost) p.damageMult *= P::overflowDamage; break;   // the ball came with the grant
            case PactId::Tiny:
                p.damageMult *= P::tinyDamage;
                if (cost) p.ballRadiusMult *= P::tinyRadius;
                break;
            case PactId::Colossus: break;    // per ball, in ballSpecs
            case PactId::HotPotato: p.pact.hotPotato = true; p.pact.potatoSlip = cost; break;
            case PactId::Juggler: p.pact.juggler = true; p.pact.juggleChip = cost; break;
            case PactId::VoidWalls: p.pact.voidWalls = true; break;   // (its price is its gift: never lifted)
            case PactId::AnchorWalls: p.pact.anchorWalls = cost; break;   // + flingPower
            case PactId::LastBreath: p.pact.lastBreath = true; break;    // the core shrank on the grant (given back on a lift)
            case PactId::Mirror:
                p.pact.mirror = true;
                if (cost) p.pact.enemyHpMul *= P::mirrorEnemyHp;
                break;
            case PactId::Elemental:
                for (float& m : p.elemMult) m *= P::elementalMul;
                if (cost) p.damageMult *= P::elementalContact;
                break;
            case PactId::Frenzy: p.pact.frenzy = true; p.pact.frenzyBreak = cost; break;
            case PactId::Blind: break;       // flingPower / aimGuide
            case PactId::Horde: if (cost) p.pact.enemyCountMul *= P::hordeCount; break;   // the balls come with each boss
        }
    }
}

float App::quickThrowMul() const { return data_.run.pactCost(PactId::Stillness) ? cfg::pact::stillnessQuick : 1.f; }

// "Unbinding" ("?"): the pacts whose price can be lifted (Void Walls' price is
// its gift - no walls - so it can't).
std::vector<PactId> App::pactsWithPrice() const {
    std::vector<PactId> v;
    for (int raw : data_.run.pacts) {
        const auto id = static_cast<PactId>(raw);
        if (id != PactId::VoidWalls && data_.run.pactCost(id)) v.push_back(id);
    }
    return v;
}

void App::liftPactPrice(PactId id) {
    RunState& r = data_.run;
    r.pactsPure |= 1u << static_cast<unsigned>(id);
    if (id == PactId::LastBreath) {   // the core it took comes back
        const float before = world_.core().maxHp;
        world_.addCoreMaxHp(before / cfg::pact::lastBreathCore - before);
    }
    syncWorldBalls();
    effects_.flash(theme::pact, 0.7f);
    effects_.addLabel(std::string("UNBOUND  ") + pactDef(id).name + " - its price is gone", {size().x * 0.5f, size().y * 0.4f},
                      theme::pact, 28, 2.f);
}

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
        if (id == PactId::Elemental && elementsUnlocked(buildUpgradeCtx()) == 0) continue;   // no element to double yet
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
