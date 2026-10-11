// The Astronomer's planets (2026-10-06): Atlas, the Guardian's god.
//
// Every ball with planet items has them orbit the CORE, each kind at its own
// distance and pace (cfg::planet), on ellipses squeezed to fit the room
// around the core. A planet shoves and staggers what it touches, breaks shots
// aimed at the core, and does its own thing whenever its ball bounces off the
// core (planetsCoreBounce) - each planet with its own projectile or field,
// nothing borrowed: Earth's rockets, Mars's moons, Venus's acid storm,
// Jupiter's Great Red Spot, Saturn's ring wall round the core, Neptune's Triton, Mercury's solar
// trail, the belt's meteors, Starry night's falling stars. The bigger the
// ball, the bigger its planets and the more they throw. "Starry night" adds 3
// random planets a fight; a ball that ascended at a Shrine adds 2 weaker ones.

#include <algorithm>
#include <cmath>

#include "sim/World.hpp"

namespace sb {

namespace {

constexpr int idx(PlanetKind k) { return static_cast<int>(k); }

// The ball's size as it scales its planets (and what they throw).
float sizeOf(const Ball& b, float k) {
    namespace P = cfg::planet;
    return clampf(b.radius / (cfg::ball::radius * k), P::sizeMin, P::sizeMax);
}

}  // namespace

sf::Vector2f World::planetOrbit(float orbitPx, float ang) const {
    // Wider than tall (the arena is), and squeezed so even the belt fits.
    namespace P = cfg::planet;
    const float k = arenaScale();
    const float roomX = std::min(core_.pos.x, size_.x - core_.pos.x) - 26.f * k;
    const float roomY = std::min(core_.pos.y, size_.y - core_.pos.y) - 26.f * k;
    const float far = P::beltOrbit * k;
    const float fx = clampf((roomX - core_.radius) / (far * 1.3f), 0.2f, 1.f);
    const float fy = clampf((roomY - core_.radius) / far, 0.2f, 1.f);
    return core_.pos + sf::Vector2f{std::cos(ang) * (core_.radius + orbitPx * k * 1.3f * fx),
                                    std::sin(ang) * (core_.radius + orbitPx * k * fy)};
}

void World::buildPlanets(const WorldParams& p) {
    namespace P = cfg::planet;
    (void)p;
    AstronomerWorld& aw = classWorld_.astronomer;
    const auto old = aw.planets;   // keep where the ones that stay were
    aw.planets.clear();
    aw.rocks.clear();
    aw.projs.erase(std::remove_if(aw.projs.begin(), aw.projs.end(),   // moonlets point at planets by index
                                  [](const AstronomerWorld::Proj& q) { return q.kind == AstronomerWorld::Proj::Moonlet; }),
                   aw.projs.end());
    aw.storms.erase(std::remove_if(aw.storms.begin(), aw.storms.end(), [](const AstronomerWorld::Storm& s) { return !s.spot; }),
                    aw.storms.end());
    for (const Ball& b : balls_) {
        if (b.ghost) continue;
        const AstronomerMods& a = b.mods.cls.astronomer;
        if (!a.any()) continue;
        auto add = [&](PlanetKind kind, float power, bool bonus) {
            AstronomerWorld::Planet pl;
            pl.kind = kind;
            pl.owner = b.owner;
            pl.power = power;
            pl.bonus = bonus;
            pl.ang = rng_.range(0.f, 2.f * kPi);
            for (const auto& o : old)
                if (o.owner == b.owner && o.kind == kind && o.bonus == bonus) { pl.ang = o.ang; pl.rings = o.rings; pl.actT = o.actT; }
            pl.pos = pl.prev = planetOrbit(P::orbit[idx(kind)], pl.ang);
            aw.planets.push_back(pl);
        };
        bool owned[kPlanetKinds] = {};
        for (int i = 0; i < kPlanetItems; ++i)
            if (a.lvl[i] > 0) {
                owned[i] = true;
                add(static_cast<PlanetKind>(i), 1.f + P::powerPerLevel * static_cast<float>(a.lvl[i] - 1), false);
            }
        // Random extra planets for this fight: the ones it doesn't have.
        auto bonus = [&](int n, float power) {
            for (int j = 0; j < n; ++j) {
                int pick[kPlanetKinds], m = 0;
                for (int i = 0; i < kPlanetKinds; ++i)
                    if (!owned[i]) pick[m++] = i;
                if (m == 0) return;
                const int c = pick[rng_.irange(0, m - 1)];
                owned[c] = true;
                add(static_cast<PlanetKind>(c), power, true);
            }
        };
        bonus(a.bonus, P::bonusPower);
        if (a.starry > 0) bonus(P::starryBonus, P::starryBonusPower);
        if (a.belt > 0) {   // the belt: a ring of rocks, the outermost orbit
            const int n = P::beltRocks + P::beltRocksPerLevel * (a.belt - 1) / 2;
            for (int i = 0; i < n; ++i) {
                AstronomerWorld::Rock r;
                r.ang = 2.f * kPi * static_cast<float>(i) / static_cast<float>(n) + rng_.range(-0.08f, 0.08f);
                r.rad = P::beltOrbit + rng_.range(-14.f, 14.f);
                r.size = rng_.range(0.6f, 1.25f);
                r.owner = b.owner;
                r.power = 1.f + P::powerPerLevel * static_cast<float>(a.belt - 1);
                r.pos = planetOrbit(r.rad, r.ang);
                aw.rocks.push_back(r);
            }
        }
    }
}

void World::updatePlanets(float dt, const WorldParams& p, FrameEvents& ev) {
    namespace P = cfg::planet;
    using Proj = AstronomerWorld::Proj;
    AstronomerWorld& aw = classWorld_.astronomer;
    const float k = arenaScale();
    // Rebuild when what the balls carry changes, and each fight (new bonus planets).
    unsigned sig = 2166136261u;
    auto mix = [&sig](unsigned v) { sig = (sig ^ v) * 16777619u; };
    for (const Ball& b : balls_) {
        if (b.ghost) continue;
        const AstronomerMods& a = b.mods.cls.astronomer;
        if (!a.any()) continue;
        mix(static_cast<unsigned>(b.owner) + 1u);
        for (int l : a.lvl) mix(static_cast<unsigned>(l));
        mix(static_cast<unsigned>(a.belt) * 7u);
        mix(static_cast<unsigned>(a.starry) * 13u);
        mix(static_cast<unsigned>(a.bonus) * 31u);
    }
    if (sig != aw.sig || aw.builtWave != wave_) {
        if (aw.builtWave != wave_)   // a new fight: Atlas back to its size
            for (Ball& b : balls_) b.cls.astronomer.growth = 0;
        if (aw.builtWave != wave_) {   // a new fight: nothing left over
            aw.halos.clear();
            aw.tritons.clear();
            aw.puffs.clear();
            aw.projs.clear();
            aw.storms.clear();
            aw.flares.clear();
            aw.booms.clear();
        }
        aw.sig = sig;
        aw.builtWave = wave_;
        buildPlanets(p);
    }
    for (auto& bm : aw.booms) bm.life -= dt;
    aw.booms.erase(std::remove_if(aw.booms.begin(), aw.booms.end(), [](const AstronomerWorld::Boom& b) { return b.life <= 0.f; }),
                   aw.booms.end());
    if (aw.planets.empty() && aw.rocks.empty() && aw.halos.empty() && aw.tritons.empty() && aw.puffs.empty() && aw.projs.empty() &&
        aw.storms.empty() && aw.flares.empty())
        return;
    auto ownerBall = [&](int owner) -> const Ball* {
        for (const Ball& b : balls_)
            if (b.owner == owner) return &b;
        return nullptr;
    };
    auto enemyById = [&](int id) -> Enemy* {
        for (Enemy& e : enemies_)
            if (e.id == id && e.hp > 0.f) return &e;
        return nullptr;
    };
    const bool live = waveRunning_;

    // A touch: a share of the ball's hit, a shove out from the core, a stagger, an element.
    auto touch = [&](Enemy& e, sf::Vector2f from, float dmg, Element el, int owner) {
        dmgOwner_ = owner;
        damageEnemy(e, dmg);
        if (el != Element::Plain) applyElement(e, el, owner, dmg, p, ev);
        dmgOwner_ = -1;
        const sf::Vector2f push = normalized(e.pos - core_.pos, normalized(e.pos - from, {1.f, 0.f}));
        e.vel += push * P::knock * k * e.knockTaken;
        e.stagger = std::max(e.stagger, P::stagger);
    };
    // A blast where something lands: everything inside, the boss too.
    auto blast = [&](sf::Vector2f at, float R, float dmg, Element el, int owner, int look) {
        for (Enemy& e : enemies_) {
            if (e.hp <= 0.f || e.orbiter || length(e.pos - at) > R + e.radius) continue;
            dmgOwner_ = owner;
            damageEnemy(e, dmg);
            if (el != Element::Plain) applyElement(e, el, owner, dmg, p, ev);
            dmgOwner_ = -1;
        }
        if (boss_.targetable() && length(boss_.pos - at) < R + boss_.radius) {
            boss_.hp -= dmg * P::bossFrac;
            boss_.hitFlash = 1.f;
        }
        AstronomerWorld::Boom bm;
        bm.pos = at;
        bm.r = R;
        bm.life = bm.maxLife = look == 2 ? 0.5f : 0.4f;
        bm.kind = look;
        if (aw.booms.size() < 48) aw.booms.push_back(bm);
    };

    for (std::size_t pi = 0; pi < aw.planets.size(); ++pi) {
        auto& pl = aw.planets[pi];
        const Ball* o = ownerBall(pl.owner);
        if (!o) continue;
        const AstronomerMods& a = o->mods.cls.astronomer;
        const int ki = idx(pl.kind);
        const float starry = a.starry > 0 ? P::starrySize : 1.f;
        const float e = sizeOf(*o, k);
        pl.r = P::radius[ki] * k * e * a.size * starry;
        pl.boostT = std::max(0.f, pl.boostT - dt);
        pl.ang += P::speed[ki] * (pl.boostT > 0.f ? 2.f : 1.f) * dt;
        pl.prev = pl.pos;
        pl.pos = planetOrbit(P::orbit[ki], pl.ang);
        pl.hitT = std::max(0.f, pl.hitT - dt);
        if (!live) continue;
        const float hit = ballDamage(*o, p) * pl.power * starry;
        const Element own = hitElement(*o, p);
        const Element el = pl.kind == PlanetKind::Venus ? Element::Poison
                         : pl.kind == PlanetKind::Mars ? Element::Fire
                         : pl.kind == PlanetKind::Neptune ? Element::Ice
                                                          : own;
        // Saturn's rings cut a little further out than the planet.
        const float reach = pl.r * (pl.kind == PlanetKind::Saturn && pl.rings ? P::saturnAura : 1.f);
        if (pl.hitT <= 0.f) {
            bool any = false;
            for (Enemy& en : enemies_) {
                if (en.hp <= 0.f || en.orbiter || length(en.pos - pl.pos) > en.radius + reach) continue;
                touch(en, pl.pos, hit * P::frac[ki], el, pl.owner);
                any = true;
            }
            if (boss_.targetable() && length(boss_.pos - pl.pos) < boss_.radius + reach) {
                boss_.hp -= hit * P::frac[ki] * P::bossFrac;
                boss_.hitFlash = std::max(boss_.hitFlash, 0.4f);
                any = true;
            }
            if (any) pl.hitT = P::hitCd[ki];
        }
        // What some of them do on their own.
        if (pl.kind == PlanetKind::Earth && (pl.actT -= dt) <= 0.f) {   // a rocket at an enemy near the core
            pl.actT = P::earthEvery;
            const Enemy* t = focusEnemy();
            if (!t) t = nearestEnemy(core_.pos, P::earthRange * k);
            if (t) {
                Proj q;
                q.kind = Proj::Rocket;
                q.pos = pl.pos;
                q.vel = normalized(pl.pos - core_.pos, {1.f, 0.f}) * (P::rocketSpeed * k);   // it lifts off outward, then turns
                q.dmg = hit * P::rocketFrac;
                q.life = 2.5f;
                q.target = t->id;
                q.owner = pl.owner;
                q.elem = static_cast<int>(own);
                if (aw.projs.size() < 80) aw.projs.push_back(q);
            }
        }
        if (pl.kind == PlanetKind::Jupiter && (pl.actT -= dt) <= 0.f) {   // a puff of gas along its way
            pl.actT = P::jupiterPuffEvery;
            AstronomerWorld::Puff f;
            f.pos = pl.pos;
            f.r = P::jupiterPuff * k * e * starry;
            f.life = f.maxLife = P::jupiterPuffLife;
            f.dmg = hit * P::jupiterPuffFrac;
            f.owner = pl.owner;
            if (aw.puffs.size() < 80) aw.puffs.push_back(f);
        }
        if (pl.kind == PlanetKind::Mercury && pl.boostT > 0.f && (pl.flareT -= dt) <= 0.f) {   // racing: a burning trail
            pl.flareT = P::flareEvery;
            AstronomerWorld::Flare f;
            f.pos = pl.pos;
            f.life = P::flareLife;
            f.r = P::flareR * k * e;
            f.dps = hit * P::flareFrac;
            f.owner = pl.owner;
            if (aw.flares.size() < 160) aw.flares.push_back(f);
        }
        (void)pi;
    }

    // The belt: rocks on the outermost orbit, a wall.
    for (auto& r : aw.rocks) {
        r.ang += P::beltSpeed * dt;
        r.pos = planetOrbit(r.rad, r.ang);
        r.hitT = std::max(0.f, r.hitT - dt);
        if (r.regrow > 0.f) { r.regrow -= dt; continue; }
        if (!live || r.hitT > 0.f) continue;
        const Ball* o = ownerBall(r.owner);
        if (!o) continue;
        const float rr = 6.f * k * r.size * sizeOf(*o, k);
        for (Enemy& en : enemies_) {
            if (en.hp <= 0.f || en.orbiter || length(en.pos - r.pos) > en.radius + rr) continue;
            touch(en, r.pos, ballDamage(*o, p) * P::beltFrac * r.power, hitElement(*o, p), r.owner);
            r.hitT = 0.35f;
            break;
        }
    }

    // What the planets threw.
    for (auto& q : aw.projs) {
        q.life -= dt;
        q.t += dt;
        switch (q.kind) {
            case Proj::Rocket: {   // speeds up and steers in; bursts on what it meets
                Enemy* t = enemyById(q.target);
                if (!t) {
                    if (const Enemy* n = nearestEnemy(q.pos, 1e9f)) { q.target = n->id; t = enemyById(q.target); }
                }
                const sf::Vector2f aim = t ? t->pos : boss_.targetable() ? boss_.pos : q.pos + q.vel;
                const float sp = std::min(length(q.vel) + 1400.f * k * dt, P::rocketTop * k);
                const float cur = std::atan2(q.vel.y, q.vel.x);
                const sf::Vector2f to = aim - q.pos;
                const float d = clampf(std::remainder(std::atan2(to.y, to.x) - cur, 2.f * kPi), -P::rocketTurn * dt, P::rocketTurn * dt);
                q.vel = sf::Vector2f{std::cos(cur + d), std::sin(cur + d)} * sp;
                q.pos += q.vel * dt;
                bool hitSomething = false;
                for (const Enemy& en : enemies_)
                    if (en.hp > 0.f && !en.orbiter && length(en.pos - q.pos) < en.radius + 5.f * k) { hitSomething = true; break; }
                if (!hitSomething && boss_.targetable() && length(boss_.pos - q.pos) < boss_.radius) hitSomething = true;
                if (hitSomething || q.life <= 0.f) {
                    blast(q.pos, P::rocketBlast * k, q.dmg, static_cast<Element>(q.elem), q.owner, 0);
                    q.life = 0.f;
                }
                break;
            }
            case Proj::Moonlet: {   // Phobos / Deimos: at its mark, then home to Mars
                AstronomerWorld::Planet* mars = q.home >= 0 && q.home < static_cast<int>(aw.planets.size())
                                                    ? &aw.planets[static_cast<std::size_t>(q.home)] : nullptr;
                if (q.back) {
                    if (!mars) { q.life = 0.f; q.back = false; break; }
                    const sf::Vector2f to = mars->pos - q.pos;
                    if (length(to) < mars->r + 4.f * k) {   // home
                        mars->moonsOut = std::max(0, mars->moonsOut - 1);
                        q.life = 0.f;
                        q.back = false;
                        break;
                    }
                    q.vel = normalized(to, {1.f, 0.f}) * (P::moonletSpeed * k);
                    q.pos += q.vel * dt;
                    q.life = std::max(q.life, 0.5f);   // it always makes it home
                    break;
                }
                Enemy* t = enemyById(q.target);
                if (!t) {
                    if (const Enemy* n = nearestEnemy(q.pos, 1e9f)) { q.target = n->id; t = enemyById(q.target); }
                }
                if (!t && !boss_.targetable()) { q.back = true; break; }
                const sf::Vector2f aim = t ? t->pos : boss_.pos;
                const float cur = std::atan2(q.vel.y, q.vel.x);
                const sf::Vector2f to = aim - q.pos;
                const float d = clampf(std::remainder(std::atan2(to.y, to.x) - cur, 2.f * kPi), -P::moonletTurn * dt, P::moonletTurn * dt);
                q.vel = sf::Vector2f{std::cos(cur + d), std::sin(cur + d)} * (P::moonletSpeed * k);
                q.pos += q.vel * dt;
                const float hr = t ? t->radius : boss_.radius;
                if (length(aim - q.pos) < hr + 5.f * k) {
                    if (t) touch(*t, q.pos, q.dmg, Element::Fire, q.owner);
                    else { boss_.hp -= q.dmg * P::bossFrac; boss_.hitFlash = 1.f; }
                    q.back = true;
                } else if (q.life <= 0.f) {
                    q.back = true;
                }
                break;
            }
            case Proj::Meteor:   // a belt rock plunging onto its mark
            case Proj::Star: {   // a falling star
                const float dur = q.kind == Proj::Star ? P::starTime : P::meteorTime;
                const float u = clampf(q.t / dur, 0.f, 1.f);
                q.pos = q.from + (q.to - q.from) * (u * u);   // it accelerates down
                if (u >= 1.f) {
                    if (q.kind == Proj::Star) blast(q.to, P::starBlast * k, q.dmg, Element::Plain, q.owner, 2);
                    else blast(q.to, P::meteorBlast * k, q.dmg, Element::Stone, q.owner, 1);
                    q.life = 0.f;
                }
                break;
            }
        }
    }
    aw.projs.erase(std::remove_if(aw.projs.begin(), aw.projs.end(), [](const Proj& q) { return q.life <= 0.f && !q.back; }),
                   aw.projs.end());

    // Saturn's rings round the core: what tries to cross is cut and thrown back out.
    auto cool = [&](std::vector<std::pair<int, float>>& cd, int id, float every) {   // per-enemy cooldown
        for (auto& c : cd) {
            if (c.first != id) continue;
            if (c.second > 0.f) return false;
            c.second = every;
            return true;
        }
        cd.push_back({id, every});
        return true;
    };
    for (auto& h : aw.halos) {
        h.life -= dt;
        h.spin += dt * 1.6f;
        for (auto& c : h.cd) c.second -= dt;
        for (Enemy& en : enemies_) {
            if (en.hp <= 0.f || en.orbiter) continue;
            const float d = length(en.pos - core_.pos);
            if (std::fabs(d - h.r) > en.radius + 8.f * k || !cool(h.cd, en.id, P::haloEvery)) continue;
            touch(en, core_.pos, h.dmg, Element::Plain, h.owner);
            en.vel += normalized(en.pos - core_.pos, {1.f, 0.f}) * (P::haloKnock * k * en.knockTaken);
        }
        if (h.life <= 0.f && h.planet >= 0 && h.planet < static_cast<int>(aw.planets.size()))
            aw.planets[static_cast<std::size_t>(h.planet)].rings = true;   // home again
    }
    aw.halos.erase(std::remove_if(aw.halos.begin(), aw.halos.end(), [](const AstronomerWorld::Halo& h) { return h.life <= 0.f; }),
                   aw.halos.end());

    // Triton: backwards round the core, freezing solid what it meets.
    for (auto& tr : aw.tritons) {
        tr.life -= dt;
        tr.ang -= P::tritonSpeed * dt;
        tr.pos = planetOrbit(tr.orbit, tr.ang);
        for (int i = std::min(tr.trailN, 9); i > 0; --i) tr.trail[i] = tr.trail[i - 1];
        tr.trail[0] = tr.pos;
        tr.trailN = std::min(tr.trailN + 1, 10);
        for (auto& c : tr.cd) c.second -= dt;
        for (Enemy& en : enemies_) {
            if (en.hp <= 0.f || en.orbiter || length(en.pos - tr.pos) > en.radius + tr.r || !cool(tr.cd, en.id, 0.8f)) continue;
            dmgOwner_ = tr.owner;
            damageEnemy(en, tr.dmg);
            applyElement(en, Element::Ice, tr.owner, tr.dmg, p, ev);
            dmgOwner_ = -1;
            en.frozen = std::max(en.frozen, P::tritonFreeze);
        }
    }
    aw.tritons.erase(std::remove_if(aw.tritons.begin(), aw.tritons.end(), [](const AstronomerWorld::Triton& t) { return t.life <= 0.f; }),
                     aw.tritons.end());

    // Storms: Venus's acid rides its planet (poisons, corrodes); the Great Red Spot drifts to the crowd and pulls.
    for (auto& st : aw.storms) {
        st.life -= dt;
        st.spin += dt * (st.spot ? 2.2f : 3.5f);
        if (!st.spot) {
            if (st.planet >= 0 && st.planet < static_cast<int>(aw.planets.size()))
                st.pos = aw.planets[static_cast<std::size_t>(st.planet)].pos;
        } else {
            if ((st.retarget -= dt) <= 0.f) {   // toward the thickest pack
                st.retarget = 0.5f;
                int best = 0;
                for (const Enemy& a : enemies_) {
                    if (a.hp <= 0.f || a.orbiter) continue;
                    int c = 0;
                    for (const Enemy& b : enemies_)
                        if (b.hp > 0.f && length(b.pos - a.pos) < st.r) ++c;
                    if (c > best) { best = c; st.goal = a.pos; }
                }
                if (best == 0) st.goal = st.pos;
            }
            st.pos += normalized(st.goal - st.pos, {0.f, 0.f}) * std::min(length(st.goal - st.pos), P::spotDrift * k * dt);
        }
        const bool tick = (st.tickT -= dt) <= 0.f;
        if (tick) st.tickT = 0.3f;
        for (Enemy& en : enemies_) {
            if (en.hp <= 0.f || en.orbiter) continue;
            const sf::Vector2f d = st.pos - en.pos;
            const float l = length(d);
            if (l > st.r + en.radius) continue;
            if (st.spot && l > 1.f) {   // drawn round and in
                const sf::Vector2f in = d / l, side{-in.y, in.x};
                en.pos += (in * 0.7f + side * 0.5f) * (P::spotPull * k * en.knockTaken * dt);
            }
            if (!tick) continue;
            dmgOwner_ = st.owner;
            damageEnemy(en, st.dps * 0.3f);
            if (!st.spot) {
                applyElement(en, Element::Poison, st.owner, st.dps * 0.3f, p, ev);
                en.mark = std::max(en.mark, P::acidMark);   // the acid eats its armour: everything hits it harder
            }
            dmgOwner_ = -1;
        }
    }
    aw.storms.erase(std::remove_if(aw.storms.begin(), aw.storms.end(), [](const AstronomerWorld::Storm& s) { return s.life <= 0.f; }),
                    aw.storms.end());

    // Mercury's burning trail.
    for (auto& f : aw.flares) {
        f.life -= dt;
        for (Enemy& en : enemies_) {
            if (en.hp <= 0.f || length(en.pos - f.pos) > f.r + en.radius) continue;
            dmgOwner_ = f.owner;
            damageEnemy(en, f.dps * dt / 3.f);   // many flares overlap: a third each
            dmgOwner_ = -1;
            en.burn = std::max(en.burn, 0.6f);
            en.burnDps = std::max(en.burnDps, f.dps * 0.15f);
        }
    }
    aw.flares.erase(std::remove_if(aw.flares.begin(), aw.flares.end(), [](const AstronomerWorld::Flare& f) { return f.life <= 0.f; }),
                    aw.flares.end());

    // Jupiter's gas: what's inside is slowed and hurt.
    for (auto& f : aw.puffs) {
        f.life -= dt;
        const bool tick = (f.tickT -= dt) <= 0.f;
        if (tick) f.tickT = 0.4f;
        for (Enemy& en : enemies_) {
            if (en.hp <= 0.f || length(en.pos - f.pos) > f.r + en.radius) continue;
            en.braceT = std::max(en.braceT, 0.4f);
            en.braceSlow = std::max(en.braceSlow, 0.35f);
            if (tick) {
                dmgOwner_ = f.owner;
                damageEnemy(en, f.dmg);
                dmgOwner_ = -1;
            }
        }
    }
    aw.puffs.erase(std::remove_if(aw.puffs.begin(), aw.puffs.end(), [](const AstronomerWorld::Puff& f) { return f.life <= 0.f; }),
                   aw.puffs.end());
}

void World::planetsCoreBounce(Ball& b, const WorldParams& p, FrameEvents& ev) {
    namespace P = cfg::planet;
    using Proj = AstronomerWorld::Proj;
    AstronomerWorld& aw = classWorld_.astronomer;
    b.atlasBounces = 0;
    if (!waveRunning_ || !b.hasRole(BallRole::Astronomer)) return;   // the class sets them off
    (void)ev;
    const float k = arenaScale();
    const AstronomerMods& a = b.mods.cls.astronomer;
    const float e = sizeOf(b, k);
    const float starry = a.starry > 0 ? P::starrySize : 1.f;
    const float base = ballDamage(b, p) * starry;
    const Element own = hitElement(b, p);
    // Targets for what it throws: the player's target first, then the nearest to the core.
    std::vector<const Enemy*> near;
    for (const Enemy& en : enemies_)
        if (en.hp > 0.f && !en.orbiter) near.push_back(&en);
    std::sort(near.begin(), near.end(), [&](const Enemy* x, const Enemy* y) {
        return length(x->pos - core_.pos) < length(y->pos - core_.pos);
    });
    if (const Enemy* f = focusEnemy()) near.insert(near.begin(), f);
    auto targetFor = [&](int i) { return near.empty() ? -1 : near[static_cast<std::size_t>(i) % near.size()]->id; };
    auto targetPos = [&](int i, sf::Vector2f& at) {
        if (!near.empty()) { at = near[static_cast<std::size_t>(i) % near.size()]->pos; return true; }
        if (boss_.targetable()) { at = boss_.pos; return true; }
        return false;
    };

    int planets = 0;
    for (std::size_t pi = 0; pi < aw.planets.size(); ++pi) {
        auto& pl = aw.planets[pi];
        if (pl.owner != b.owner) continue;
        ++planets;
        const float hit = base * pl.power;
        switch (pl.kind) {
            case PlanetKind::Mercury: pl.boostT = P::mercuryBoost; pl.flareT = 0.f; break;   // races, burning
            case PlanetKind::Moon:    pl.boostT = 1.2f; break;
            case PlanetKind::Venus: {   // an acid storm wraps it for a while
                AstronomerWorld::Storm st;
                st.pos = pl.pos;
                st.r = P::acidR * k * e;
                st.life = st.maxLife = P::acidTime;
                st.dps = hit * P::acidFrac;
                st.owner = b.owner;
                st.planet = static_cast<int>(pi);
                aw.storms.push_back(st);
                break;
            }
            case PlanetKind::Earth: {   // a salvo of rockets, fanning out before they turn in
                const int n = std::max(1, static_cast<int>(std::lround(P::earthSalvo * e)));
                const sf::Vector2f out = normalized(pl.pos - core_.pos, {1.f, 0.f});
                for (int i = 0; i < n && aw.projs.size() < 80; ++i) {
                    Proj q;
                    q.kind = Proj::Rocket;
                    q.pos = pl.pos;
                    const float fan = (static_cast<float>(i) - static_cast<float>(n - 1) * 0.5f) * 0.45f;
                    q.vel = sf::Vector2f{out.x * std::cos(fan) - out.y * std::sin(fan), out.x * std::sin(fan) + out.y * std::cos(fan)} *
                            (P::rocketSpeed * k);
                    q.dmg = hit * P::rocketFrac;
                    q.life = 2.5f;
                    q.target = targetFor(i);
                    q.owner = b.owner;
                    q.elem = static_cast<int>(own);
                    aw.projs.push_back(q);
                }
                break;
            }
            case PlanetKind::Mars: {   // Phobos and Deimos (more for a bigger ball) leave their orbit
                const int n = std::max(1, static_cast<int>(std::lround(P::marsMoons * e)));
                for (int i = pl.moonsOut; i < n && aw.projs.size() < 80; ++i) {
                    Proj q;
                    q.kind = Proj::Moonlet;
                    const float an = static_cast<float>(i) * kPi;
                    q.pos = pl.pos + sf::Vector2f{std::cos(an), std::sin(an)} * (pl.r * 1.8f);
                    q.vel = sf::Vector2f{std::cos(an), std::sin(an)} * (P::moonletSpeed * k);
                    q.dmg = hit * P::moonletFrac;
                    q.life = 1.6f;
                    q.target = targetFor(i);
                    q.owner = b.owner;
                    q.home = static_cast<int>(pi);
                    aw.projs.push_back(q);
                    ++pl.moonsOut;
                }
                break;
            }
            case PlanetKind::Jupiter: {   // the Great Red Spot breaks off and drifts to the crowd
                AstronomerWorld::Storm st;
                st.spot = true;
                st.pos = st.goal = pl.pos;
                st.r = P::spotR * k * e;
                st.life = st.maxLife = P::spotTime;
                st.dps = hit * P::spotFrac;
                st.owner = b.owner;
                aw.storms.push_back(st);
                break;
            }
            case PlanetKind::Saturn:   // its rings leave it and settle round the core: a wall
                if (pl.rings) {
                    pl.rings = false;
                    AstronomerWorld::Halo h;
                    h.r = core_.radius + P::haloR * k * e;
                    h.life = h.maxLife = P::haloTime;
                    h.dmg = hit * P::haloFrac;
                    h.owner = b.owner;
                    h.planet = static_cast<int>(pi);
                    aw.halos.push_back(std::move(h));
                }
                break;
            case PlanetKind::Neptune: {   // Triton, loosed backwards round the core
                AstronomerWorld::Triton tr;
                tr.ang = pl.ang;
                tr.orbit = P::orbit[idx(PlanetKind::Neptune)] * 0.8f;
                tr.life = tr.maxLife = P::tritonTime;
                tr.dmg = hit * P::tritonFrac;
                tr.r = P::tritonR * k * e;
                tr.owner = b.owner;
                tr.pos = pl.pos;
                aw.tritons.push_back(std::move(tr));
                break;
            }
        }
    }
    // The belt: half of its rocks plunge onto the enemies, then grow back.
    int flung = 0;
    for (auto& r : aw.rocks) {
        if (r.owner != b.owner || r.regrow > 0.f) continue;
        if (++flung % 2 == 0) continue;
        sf::Vector2f at;
        if (!targetPos(flung, at) || aw.projs.size() >= 80) break;
        Proj q;
        q.kind = Proj::Meteor;
        q.from = q.pos = r.pos;
        q.to = at;
        q.dmg = base * P::meteorFrac * r.power;
        q.life = 1.f;
        q.owner = b.owner;
        aw.projs.push_back(q);
        r.regrow = P::beltRegrow * rng_.range(0.7f, 1.3f);
    }
    // Starry night: a star falls on an enemy for every planet.
    if (a.starry > 0 && planets > 0) {
        const float dmg = base * P::starFrac * (1.f + P::powerPerLevel * static_cast<float>(a.starry - 1));
        for (int i = 0; i < planets && aw.projs.size() < 80; ++i) {
            sf::Vector2f at;
            if (!targetPos(rng_.irange(0, 99), at)) break;
            Proj q;
            q.kind = Proj::Star;
            q.to = at;
            q.from = q.pos = {at.x - 220.f * k + rng_.range(-60.f, 60.f) * k, -30.f * k};   // in from the top, slanting
            q.dmg = dmg;
            q.life = 1.f;
            q.owner = b.owner;
            aw.projs.push_back(q);
        }
    }
}

// The Atlas ball: every few bounces (walls, enemies) it goes straight for the
// core - swinging between the field and the core, so its planets go off often.
void World::atlasBounce(Ball& b) {
    if (b.ghost || !b.hasRole(BallRole::Astronomer)) return;
    if (++b.atlasBounces < cfg::planet::homeEvery) return;
    b.atlasBounces = 0;
    const float sp = std::max(length(b.vel), 1.f);
    b.vel = normalized(core_.pos - b.pos, {1.f, 0.f}) * sp;
}

bool World::planetsCatch(sf::Vector2f pos, float r) {
    const AstronomerWorld& aw = classWorld_.astronomer;
    for (const auto& h : aw.halos)   // Saturn's ring wall round the core
        if (std::fabs(length(pos - core_.pos) - h.r) < r + 6.f) return true;
    for (const auto& pl : aw.planets)
        if (length(pl.pos - pos) < pl.r + r) return true;
    const float k = arenaScale();
    for (const auto& rk : aw.rocks)
        if (rk.regrow <= 0.f && length(rk.pos - pos) < 6.f * k * rk.size + r) return true;
    return false;
}

}  // namespace sb
