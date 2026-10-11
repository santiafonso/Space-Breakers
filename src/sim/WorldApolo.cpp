// Apolo, the Support's god (2026-10-07). Four golden muses fly with your
// balls, one each - never two on one ball - and lend it a gift: Strength
// (+damage), Speed, a Twin (a copy with its items and abilities), Giant
// (bigger, harder). Each also marks a weak point on the enemy nearest her
// ball (the Support's mark: it takes more from everything). Every so often
// they all move on to the next ball. Lyre (a chord mends the core; a lost
// gift lingers), Laurel crown (stronger gifts), Silver bow (arrows at the
// weak points), Sun chariot (a fifth muse: her ball becomes a little sun),
// Oracle (faster rotations, a sure crit on arrival).

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

namespace {
constexpr int kStrength = 0, kSpeed = 1, kTwin = 2, kGiant = 3, kSun = 4;
}  // namespace

// What a gift does for a ball: its strength (0 = not there; a Lyre's lingering half counts).
float World::apoloGift(const Ball& b, int gift) const {
    const ApoloState& s = b.cls.apolo;
    if (s.gift == gift) return s.power;
    if (s.glow == gift && s.glowT > 0.f) return s.power * cfg::apolo::lyreHalf;
    return 0.f;
}

float World::apoloDamageMul(const Ball& b) const {
    namespace A = cfg::apolo;
    return (1.f + A::strength * apoloGift(b, kStrength)) * (1.f + A::giantDamage * apoloGift(b, kGiant));
}
float World::apoloSpeedMul(const Ball& b) const { return 1.f + cfg::apolo::speed * apoloGift(b, kSpeed); }
float World::apoloSizeMul(const Ball& b) const { return 1.f + cfg::apolo::giantSize * apoloGift(b, kGiant); }

// "Oracle": the hit right after a muse arrives is a sure crit.
float World::apoloPreHit(Ball& b) {
    if (b.cls.apolo.foresight <= 0.f) return 1.f;
    const float m = b.cls.apolo.foresight;
    b.cls.apolo.foresight = 0.f;
    return m;
}

// The Twin's gift: a copy of the ball, its items and abilities, for as long as she stays.
void World::apoloTwin(const Ball& parent, float life) {
    if (static_cast<int>(ghosts_.size() + pendingGhosts_.size()) >= cfg::synergy::maxGhosts) return;
    Ball g = parent;
    g.ghost = true;
    g.twin = false;
    g.classPulse = 0.f;
    g.ghostLife = life;
    g.held = false;
    g.trail.clear();
    g.waterTrail.clear();
    g.cls.apolo = ApoloState{};
    g.mods.cls.common.projection = true;   // (it casts, like its ball)
    const float a = rng_.range(0.f, 1.f) < 0.5f ? 0.6f : -0.6f;
    g.vel = {parent.vel.x * std::cos(a) - parent.vel.y * std::sin(a), parent.vel.x * std::sin(a) + parent.vel.y * std::cos(a)};
    pendingGhosts_.push_back(g);
}

void World::updateApolo(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace A = cfg::apolo;
    const float k = arenaScale();
    ApoloWorld& aw = classWorld_.apolo;
    for (auto& f : aw.fx) f.life -= dt;
    aw.fx.erase(std::remove_if(aw.fx.begin(), aw.fx.end(), [](const ApoloWorld::Fx& f) { return f.life <= 0.f; }), aw.fx.end());
    for (Ball& b : balls_) {   // gifts are set fresh each frame below; a lingering one fades
        b.cls.apolo.gift = -1;
        b.cls.apolo.glowT = std::max(0.f, b.cls.apolo.glowT - dt);
    }
    const Ball* apolo = nullptr;
    for (const Ball& b : balls_)
        if (b.hasRole(BallRole::Apolo) && !b.ghost) { apolo = &b; break; }
    if (!apolo) { aw.muses.clear(); aw.owner = -1; return; }
    const ApoloMods& m = apolo->mods.cls.apolo;
    const int want = m.chariot > 0.f ? 5 : 4;
    if (aw.owner != apolo->owner || static_cast<int>(aw.muses.size()) != want) {   // the muses gather
        aw.owner = apolo->owner;
        aw.muses.clear();
        for (int i = 0; i < want; ++i) {
            ApoloWorld::Muse mu;
            mu.gift = i;
            mu.pos = apolo->pos;
            aw.muses.push_back(mu);
        }
        aw.rotT = 0.f;
    }
    const float every = A::rotEvery * (m.oracle > 0.f ? 0.5f : 1.f);
    if (aw.wave != wave_) { aw.wave = wave_; aw.rotT = 0.f; }   // a new fight: they hand their gifts out afresh

    // The balls they go to: every real one, in order.
    std::vector<Ball*> real;
    for (Ball& b : balls_)
        if (!b.ghost) real.push_back(&b);
    std::sort(real.begin(), real.end(), [](const Ball* x, const Ball* y) { return x->owner < y->owner; });
    const int n = static_cast<int>(real.size());
    const float power = 1.f + m.laurel;
    bool moved = false;
    if (waveRunning_ && (aw.rotT -= dt) <= 0.f) {   // they move on
        aw.rotT = every;
        ++aw.rot;
        moved = true;
        if (m.lyre > 0.f) {   // a chord: the core mends
            repairCore(m.lyre);
            if (aw.fx.size() < 60) aw.fx.push_back({1, core_.pos, core_.pos, 0.8f, 0.8f, 90.f * k});
        }
    }
    for (int i = 0; i < static_cast<int>(aw.muses.size()); ++i) {
        ApoloWorld::Muse& mu = aw.muses[static_cast<std::size_t>(i)];
        // muse i goes to the (i + rot)th of max(balls, muses) places: distinct balls, and with fewer
        // balls than muses the gifts still come round in turn (the rest wait by the core)
        const int places = std::max(n, static_cast<int>(aw.muses.size()));
        const int idx = (i + aw.rot) % places;
        Ball* to = idx < n ? real[static_cast<std::size_t>(idx)] : nullptr;
        if (moved) {
            if (m.lyre > 0.f && mu.ball >= 0)   // "Lyre": the ball she left keeps half her gift a while
                for (Ball* b : real)
                    if (b->owner == mu.ball) { b->cls.apolo.glow = mu.gift; b->cls.apolo.glowT = A::lyreLinger; }
            if (to) {
                if (m.oracle > 0.f) to->cls.apolo.foresight = m.oracle;
                if (mu.gift == kTwin) apoloTwin(*to, every);
                if (aw.fx.size() < 60) aw.fx.push_back({2, mu.pos, to->pos, 0.5f, 0.5f, to->radius, mu.gift});
            }
        }
        mu.ball = to ? to->owner : -1;
        // she flies at her ball's side (or waits by the core)
        const sf::Vector2f home = to ? to->pos + sf::Vector2f{std::cos(static_cast<float>(i) * 1.7f + to->age * 2.f),
                                                             std::sin(static_cast<float>(i) * 1.7f + to->age * 2.f)} * (to->radius + 16.f * k)
                                     : core_.pos + sf::Vector2f{std::cos(static_cast<float>(i) * 1.6f), std::sin(static_cast<float>(i) * 1.6f)} * (70.f * k);
        mu.pos += (home - mu.pos) * std::min(1.f, dt * 7.f);
        if (to) {
            to->cls.apolo.gift = mu.gift;
            to->cls.apolo.power = power;
        }
        if (!waveRunning_) continue;
        // her weak point stays hers until it dies; only then does she pick a new one,
        // the enemy nearest her that no other muse has marked
        Enemy* mark = nullptr;
        for (Enemy& e : enemies_)
            if (e.id == mu.mark && e.hp > 0.f) { mark = &e; break; }
        if (!mark) {
            float bd = 1e9f;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter) continue;
                bool taken = false;
                for (const auto& o : aw.muses) taken = taken || (&o != &mu && o.mark == e.id);
                if (taken) continue;
                const float d = length(e.pos - mu.pos);
                if (d < bd) { bd = d; mark = &e; }
            }
        }
        mu.mark = mark ? mark->id : -1;
        if (mark) mark->mark = std::max(mark->mark, A::markTime);
        if (!to) continue;
        const float hit = ballDamage(*apolo, p);
        if (m.bow > 0.f && mark && (mu.arrowT -= dt) <= 0.f) {   // a sun arrow at her weak point
            mu.arrowT = A::bowEvery;
            dmgOwner_ = aw.owner;
            damageEnemy(*mark, hit * m.bow);
            dmgOwner_ = -1;
            if (aw.fx.size() < 60) aw.fx.push_back({0, mu.pos, mark->pos, 0.25f, 0.25f, 0.f, mu.gift});
        }
        if (mu.gift == kSun && (mu.sunT -= dt) <= 0.f) {   // the sun: everything round her ball burns
            mu.sunT = A::sunTick;
            for (Enemy& e : enemies_) {
                if (e.hp <= 0.f || e.orbiter || length(e.pos - to->pos) > A::sunR * k * power + e.radius) continue;
                dmgOwner_ = aw.owner;
                damageEnemy(e, hit * m.chariot * A::sunTick);
                applyElement(e, Element::Fire, aw.owner, hit * m.chariot * A::sunTick, p, ev);
                dmgOwner_ = -1;
            }
        }
    }
}

}  // namespace sb
