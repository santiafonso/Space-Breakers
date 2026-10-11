// Loki, the Jester's god (2026-10-07). It keeps splitting off clones of
// itself; each flies off, plays one game of chance once, and is gone - and
// what comes up, happens:
// a coin always (heads a bolt from the sky, tails a jab); its items add the
// casino - Loaded dice (that many knives), Roulette (red a fire nova, black a
// pull, green the jackpot) - then the cards (Marked deck: suit and rank), then
// a Pachinko (steel balls raining down through the enemies). High table: more
// clones, sooner, everything pays more. Every kill may split off a clone.

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

namespace {
constexpr int kCoin = 0, kDice = 1, kRoulette = 2, kCards = 3, kPachinko = 4;
}  // namespace

// A clone splits off the ball and flies off its own way.
void World::lokiSpawn(const Ball& b) {
    namespace L = cfg::loki;
    LokiWorld& lw = classWorld_.loki;
    const LokiMods& m = b.mods.cls.loki;
    const int cap = m.high > 0.f ? L::clonesHigh : L::clones;
    int mine = 0;
    for (const auto& c : lw.clones) mine += c.owner == b.owner;
    if (mine >= cap) return;
    LokiWorld::Clone c;
    c.pos = b.pos;
    c.vel = rng_.direction() * (L::cloneSpeed * arenaScale());
    c.life = c.maxLife = L::cloneLife;
    c.owner = b.owner;
    c.gameT = rng_.range(L::flyMin, L::flyMax);   // a moment on the wing, then its one game
    lw.clones.push_back(c);
    if (lw.fx.size() < 80) lw.fx.push_back({9, b.pos, c.pos, 0.4f, 0.4f, b.radius * 1.6f});
}

// The ball itself: a clone every so often.
void World::lokiTick(Ball& b, float dt) {
    namespace L = cfg::loki;
    if (!b.hasRole(BallRole::Loki) || b.ghost || !waveRunning_) return;
    LokiState& st = b.cls.loki;
    if ((st.cloneT -= dt) > 0.f) return;
    st.cloneT = b.mods.cls.loki.high > 0.f ? L::highEvery : L::cloneEvery;
    lokiSpawn(b);
}

void World::updateLoki(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace L = cfg::loki;
    const float k = arenaScale();
    LokiWorld& lw = classWorld_.loki;
    for (auto& f : lw.fx) f.life -= dt;
    lw.fx.erase(std::remove_if(lw.fx.begin(), lw.fx.end(), [](const LokiWorld::Fx& f) { return f.life <= 0.f; }), lw.fx.end());
    if (lw.wave != wave_) {   // a new fight: the table's cleared
        lw.wave = wave_;
        lw.clones.clear();
        lw.pins.clear();
    }
    if (!waveRunning_) return;
    auto ownerBall = [&](int owner) -> const Ball* {
        for (const Ball& b : balls_)
            if (b.owner == owner && !b.ghost) return &b;
        return nullptr;
    };
    auto hurt = [&](int owner, Enemy& e, float dmg) {
        dmgOwner_ = owner;
        damageEnemy(e, dmg);
        dmgOwner_ = -1;
    };
    auto area = [&](int owner, sf::Vector2f at, float R, float dmg) {   // everything round a point (the boss at half)
        for (Enemy& e : enemies_)
            if (e.hp > 0.f && !e.orbiter && length(e.pos - at) < R + e.radius) hurt(owner, e, dmg);
        if (boss_.targetable() && length(boss_.pos - at) < R + boss_.radius) { boss_.hp -= dmg * 0.5f; boss_.hitFlash = 1.f; }
    };
    auto push = [&](LokiWorld::Fx f) { if (lw.fx.size() < 120) lw.fx.push_back(f); };

    for (auto& c : lw.clones) {
        const Ball* o = ownerBall(c.owner);
        if (!o) { c.life = 0.f; continue; }
        c.life -= dt;
        // It flies about, drifting at the nearest enemy, bouncing off the walls (an illusion: it goes through the rest).
        if (const Enemy* n = nearestEnemy(c.pos, 700.f * k)) {   // (close in, it circles: never sitting on top of one)
            const sf::Vector2f to = n->pos - c.pos;
            const sf::Vector2f dir = length(to) > 110.f * k ? normalized(to, c.vel) : normalized(sf::Vector2f{-to.y, to.x}, c.vel);
            const sf::Vector2f want = dir * (L::cloneSpeed * k);
            c.vel += (want - c.vel) * std::min(1.f, dt * L::cloneSteer);
        }
        c.pos += c.vel * dt;
        const float m = o->radius;
        if (c.pos.x < m || c.pos.x > size_.x - m) { c.vel.x = -c.vel.x; c.pos.x = clampf(c.pos.x, m, size_.x - m); }
        if (c.pos.y < m || c.pos.y > size_.y - m) { c.vel.y = -c.vel.y; c.pos.y = clampf(c.pos.y, m, size_.y - m); }

        const LokiMods& lm = o->mods.cls.loki;
        const float stake = lm.high > 0.f ? lm.high : 1.f;
        const float hit = ballDamage(*o, p) * stake;
        if (c.game < 0) {   // on the wing, then its one game: the coin, or one its items brought
            if ((c.gameT -= dt) > 0.f) continue;
            int games[5], n = 0;
            games[n++] = kCoin;
            if (lm.dice > 0.f) games[n++] = kDice;
            if (lm.roulette > 0.f) games[n++] = kRoulette;
            if (lm.cards > 0.f) games[n++] = kCards;
            if (lm.pachinko > 0.f) games[n++] = kPachinko;
            c.game = games[rng_.irange(0, n - 1)];
            c.show = c.showMax = L::gameShow;
            switch (c.game) {   // it's decided now; it shows while it spins
                case kCoin: c.a = rng_.range(0.f, 1.f) < 0.5f ? 1 : 0; break;
                case kDice: c.a = rng_.irange(1, 6); c.b = rng_.irange(1, 6); break;
                case kRoulette: {
                    const float g = L::green * (lm.high > 0.f ? 2.f : 1.f);
                    const float r = rng_.range(0.f, 1.f);
                    c.a = r < g ? 0 : r < g + (1.f - g) * 0.5f ? 1 : 2;
                    break;
                }
                case kCards: c.a = rng_.irange(0, 3); c.b = rng_.irange(1, 13); break;
                default: break;
            }
            continue;
        }
        if ((c.show -= dt) > 0.f) continue;
        // It pays.
        switch (c.game) {
            case kCoin: {   // heads: a bolt from the sky; tails: a jab
                Enemy* t = const_cast<Enemy*>(nearestEnemy(c.pos, L::coinRange * k));
                if (!t) break;
                if (c.a == 1) {
                    area(c.owner, t->pos, L::coinR * k, hit * L::coinHeads);
                    push({0, t->pos + sf::Vector2f{rng_.range(-40.f, 40.f) * k, -320.f * k}, t->pos, 0.3f, 0.3f, L::coinR * k});
                } else {
                    hurt(c.owner, *t, hit * L::coinTails);
                    push({4, c.pos, t->pos, 0.2f, 0.2f, 0.f});
                }
                break;
            }
            case kDice: {   // that many knives, at the enemies round it
                std::vector<Enemy*> es;
                for (Enemy& e : enemies_)
                    if (e.hp > 0.f && !e.orbiter && length(e.pos - c.pos) < L::diceRange * k) es.push_back(&e);
                if (es.empty()) break;
                for (int i = 0; i < c.a + c.b; ++i) {
                    Enemy& e = *es[static_cast<std::size_t>(rng_.irange(0, static_cast<int>(es.size()) - 1))];
                    if (e.hp <= 0.f) continue;
                    hurt(c.owner, e, hit * L::diceFrac * lm.dice);
                    push({4, c.pos, e.pos, 0.25f, 0.25f, 0.f});
                }
                break;
            }
            case kRoulette:
                if (c.a == 1) {   // red: a fire nova
                    for (Enemy& e : enemies_)
                        if (e.hp > 0.f && !e.orbiter && length(e.pos - c.pos) < L::redR * k + e.radius) {
                            hurt(c.owner, e, hit * L::redFrac * lm.roulette);
                            e.burn = std::max(e.burn, 1.5f);
                            e.burnDps = std::max(e.burnDps, hit * 0.3f);
                        }
                    push({1, c.pos, c.pos, 0.5f, 0.5f, L::redR * k});
                } else if (c.a == 2) {   // black: everything near dragged in and stunned
                    for (Enemy& e : enemies_) {
                        if (e.hp <= 0.f || e.orbiter || length(e.pos - c.pos) > L::blackR * k + e.radius) continue;
                        e.vel += normalized(c.pos - e.pos, {1.f, 0.f}) * (L::blackPull * k * e.knockTaken);
                        e.stagger = std::max(e.stagger, L::blackStun);
                    }
                    push({2, c.pos, c.pos, 0.6f, 0.6f, L::blackR * k});
                } else {   // green: the jackpot
                    area(c.owner, c.pos, L::jackpotR * k, hit * L::jackpotFrac * lm.roulette);
                    ev.bounties.push_back({c.pos, L::jackpotGold});
                    ev.callouts.push_back({c.pos, "JACKPOT"});
                    ev.kick = std::max(ev.kick, 7.f);
                    push({3, c.pos, c.pos, 0.9f, 0.9f, L::jackpotR * k});
                }
                break;
            case kCards: {   // the suit decides, the rank pays: a face card double, an ace four times
                const float rank = c.b == 1 ? 4.f : c.b >= 11 ? 2.f : 1.f;
                const float dmg = hit * L::cardFrac * lm.cards * rank;
                if (c.b == 1) ev.callouts.push_back({c.pos, "ACE"});
                if (c.a == 0) {   // spades: a slash down the line, through the nearest
                    const Enemy* t = nearestEnemy(c.pos, L::slashLen * k);
                    const sf::Vector2f dir = t ? normalized(t->pos - c.pos, {1.f, 0.f}) : rng_.direction();
                    const sf::Vector2f end = c.pos + dir * (L::slashLen * k);
                    for (Enemy& e : enemies_) {
                        if (e.hp <= 0.f || e.orbiter) continue;
                        const float u = clampf(dot(e.pos - c.pos, dir), 0.f, L::slashLen * k);
                        if (length(e.pos - (c.pos + dir * u)) < L::slashW * k + e.radius) hurt(c.owner, e, dmg);
                    }
                    push({5, c.pos, end, 0.35f, 0.35f, L::slashW * k});
                } else if (c.a == 1) {   // clubs: a stunning blow round the nearest
                    const Enemy* t = nearestEnemy(c.pos, 500.f * k);
                    const sf::Vector2f at = t ? t->pos : c.pos;
                    for (Enemy& e : enemies_)
                        if (e.hp > 0.f && !e.orbiter && length(e.pos - at) < L::clubR * k + e.radius) {
                            hurt(c.owner, e, dmg * 0.7f);
                            e.stagger = std::max(e.stagger, L::clubStun * rank);
                        }
                    push({6, at, at, 0.45f, 0.45f, L::clubR * k});
                } else if (c.a == 2) {   // diamonds: shards at the nearest few
                    std::vector<Enemy*> es;
                    for (Enemy& e : enemies_)
                        if (e.hp > 0.f && !e.orbiter) es.push_back(&e);
                    std::sort(es.begin(), es.end(), [&](const Enemy* x, const Enemy* y) { return length(x->pos - c.pos) < length(y->pos - c.pos); });
                    for (int i = 0; i < std::min(L::shardTargets, static_cast<int>(es.size())); ++i) {
                        hurt(c.owner, *es[static_cast<std::size_t>(i)], dmg);
                        push({8, c.pos, es[static_cast<std::size_t>(i)]->pos, 0.3f, 0.3f, 0.f});
                    }
                } else {   // hearts: the core mends
                    repairCore(L::heartHeal * rank * lm.cards);
                    push({7, c.pos, core_.pos, 0.6f, 0.6f, 0.f});
                }
                break;
            }
            default: {   // the pachinko: steel balls rain down across the whole top of the arena
                const int n = static_cast<int>(static_cast<float>(L::pachinkoBalls) * lm.pachinko);
                for (int i = 0; i < n && static_cast<int>(lw.pins.size()) < L::pinMax; ++i) {
                    LokiWorld::Pin pin;
                    pin.pos = {(static_cast<float>(i) + rng_.range(0.15f, 0.85f)) / static_cast<float>(n) * size_.x, rng_.range(-120.f, 0.f) * k};
                    pin.vel = {rng_.range(-60.f, 60.f) * k, rng_.range(0.f, 120.f) * k};
                    pin.life = L::pinLife;
                    pin.dmg = hit * L::pachinkoFrac * lm.pachinko;
                    pin.owner = c.owner;
                    lw.pins.push_back(pin);
                }
                break;
            }
        }
        c.game = -1;   // played: it's gone
        c.gameT = 1e9f;
        c.life = std::min(c.life, 0.3f);
        push({9, c.pos, c.pos, 0.35f, 0.35f, 16.f * k});
    }
    lw.clones.erase(std::remove_if(lw.clones.begin(), lw.clones.end(), [](const LokiWorld::Clone& c) { return c.life <= 0.f; }), lw.clones.end());

    // The pachinko's steel balls: they fall, and every enemy they meet knocks them about like a pin.
    const float pr = 5.f * k;
    for (auto& pin : lw.pins) {
        pin.life -= dt;
        pin.hitCd -= dt;
        pin.vel.y += L::pinGravity * k * dt;
        pin.pos += pin.vel * dt;
        if (pin.pos.x < pr || pin.pos.x > size_.x - pr) { pin.vel.x = -pin.vel.x * 0.8f; pin.pos.x = clampf(pin.pos.x, pr, size_.x - pr); }
        if (pin.pos.y > size_.y + 20.f) { pin.life = 0.f; continue; }
        if (pin.hitCd > 0.f) continue;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - pin.pos) > e.radius + pr) continue;
            hurt(pin.owner, e, pin.dmg);
            const sf::Vector2f off = normalized(pin.pos - e.pos, {0.f, -1.f});
            pin.pos = e.pos + off * (e.radius + pr + 1.f);
            pin.vel = sf::Vector2f{off.x * 260.f * k + rng_.range(-90.f, 90.f) * k, -std::fabs(pin.vel.y) * 0.45f - 60.f * k};
            pin.hitCd = 0.12f;
            break;
        }
    }
    lw.pins.erase(std::remove_if(lw.pins.begin(), lw.pins.end(), [](const LokiWorld::Pin& q) { return q.life <= 0.f; }), lw.pins.end());
}

}  // namespace sb
