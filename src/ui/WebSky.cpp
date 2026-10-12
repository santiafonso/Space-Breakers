#include "ui/WebSky.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

#include "core/Theme.hpp"
#include "progression/Offers.hpp"
#include "render/ClassRender.hpp"
#include "render/Draw.hpp"
#include "sim/Entities.hpp"
#include "ui/MenuBackdrop.hpp"

// The skill web's sky (2026-10-06): a galaxy twice the screen's size that
// drifts with the camera (half its pan), so moving round the web explores it.
// Black on a fresh save; every class unlocked adds its own motif, spread over
// the whole galaxy unless it's one big thing (the Mage's circle, the
// Guardian's planet, kept in view); more classes bring more of the sky -
// nebulae, spiral galaxies, twin stars, a pulsar, the milky way, supernovae,
// planets breaking apart, a black hole. Dim, slow, alive.

namespace sb {

namespace {

constexpr float kGalaxy = 2.f;      // the galaxy is this x the screen
constexpr float kDrift = 0.5f;      // it moves this much of the camera's pan

// A stable pseudo-random number in [0, 1) per index: the sky never reshuffles.
float h01(float x) {
    const float s = std::sin(x * 12.9898f + 78.233f) * 43758.5453f;
    return s - std::floor(s);
}

// Each class's motif fades in (0 -> 1) once it's unlocked; it flares on arrival.
struct SkyState {
    bool ready = false;
    float lastT = 0.f;
    std::array<float, kClassCount> appear{};   // 0..1
    std::array<float, kClassCount> flare{};    // 1 -> 0 just after unlocking
    std::array<double, kClassCount> flareAt{}; // real seconds when it arrived (the flare fades on the wall clock)
};
// Real seconds, for the arrival flare: the sky's own clock can stand still,
// and a flare frozen on it stayed on screen as a white ring (2026-10-12).
double wallSeconds() {
    using C = std::chrono::steady_clock;
    static const C::time_point start = C::now();
    return std::chrono::duration<double>(C::now() - start).count();
}
SkyState gSky;
sf::Vector2f gLook;   // where the watching eyes look (the middle of the screen, in galaxy space)

// A point of the galaxy (g = its size) in the part on screen when the camera
// rests - for the big motifs that should be seen without looking for them.
sf::Vector2f inView(sf::Vector2f g, float fx, float fy) {
    const float m = (kGalaxy - 1.f) / (2.f * kGalaxy);   // the margin either side, as a share of g
    return {g.x * (m + fx / kGalaxy), g.y * (m + fy / kGalaxy)};
}
// Anywhere in the galaxy.
sf::Vector2f anywhere(sf::Vector2f g, float seed) { return {g.x * h01(seed * 1.31f), g.y * h01(seed * 2.77f + 5.f)}; }

void stars(sf::RenderTarget& w, sf::Vector2f g, int count, float t, float a) {
    for (int i = 0; i < count; ++i) {
        const float fi = static_cast<float>(i);
        const sf::Vector2f p = anywhere(g, fi);
        const float tw = 0.55f + 0.45f * std::sin(t * (0.6f + h01(fi * 3.1f) * 1.4f) + fi);
        const float r = 0.6f + 1.3f * h01(fi * 4.3f);
        const float hue = h01(fi * 7.7f);
        const sf::Color tint = hue < 0.15f ? sf::Color(255, 220, 190) : hue < 0.3f ? sf::Color(190, 210, 255) : sf::Color(230, 232, 255);
        draw::disc(w, p, r, withAlpha(tint, 0.55f * tw * a), withAlpha(sf::Color::White, 0.f), {1.f, 1.f}, 6);
    }
}

void nebula(sf::RenderTarget& w, sf::Vector2f g, const std::array<sf::Color, kClassCount>& col,
            const std::array<float, kClassCount>& on, float t, float a) {
    // Big faint clouds in the colours of the classes you have, spread over the galaxy.
    int k = 0;
    for (int i = 0; i < kClassCount; ++i) {
        if (on[static_cast<std::size_t>(i)] <= 0.f) continue;
        const float fi = static_cast<float>(i);
        const sf::Vector2f p = anywhere(g, fi * 17.f + 3.f) + sf::Vector2f{std::sin(t * 0.03f + fi) * 40.f, std::cos(t * 0.025f + fi) * 30.f};
        draw::glow(w, p, g.y * (0.2f + 0.12f * h01(fi * 9.7f)), col[static_cast<std::size_t>(i)], 0.08f * a * on[static_cast<std::size_t>(i)]);
        if (++k >= 12) break;
    }
}

void spiralGalaxy(sf::RenderTarget& w, sf::Vector2f c, float R, float tilt, float spin, sf::Color tint, float a) {
    // Two arms of dust and stars, a bright core, turning slowly; flattened by its tilt.
    draw::glow(w, c, R * 0.35f, sf::Color(255, 240, 210), 0.35f * a);
    for (int arm = 0; arm < 2; ++arm)
        for (int i = 0; i < 70; ++i) {
            const float u = static_cast<float>(i) / 69.f;
            const float an = spin + static_cast<float>(arm) * kPi + u * 5.2f;
            const float rr = R * (0.08f + 0.92f * u);
            const float jitter = (h01(static_cast<float>(i * 7 + arm)) - 0.5f) * R * 0.08f;
            const sf::Vector2f e{std::cos(an) * (rr + jitter), std::sin(an) * (rr + jitter) * tilt};
            const sf::Vector2f q{e.x * 0.9f - e.y * 0.44f, e.x * 0.44f + e.y * 0.9f};
            draw::disc(w, c + q, 0.8f + 1.2f * (1.f - u), withAlpha(lerpColor(sf::Color(255, 245, 225), tint, u), (0.55f - 0.4f * u) * a),
                       withAlpha(tint, 0.f), {1.f, 1.f}, 5);
        }
    draw::glow(w, c, R, tint, 0.06f * a);
}

void milkyWay(sf::RenderTarget& w, sf::Vector2f g, float t, float a) {
    // A band of dust across the whole galaxy, corner to corner.
    for (int i = 0; i < 520; ++i) {
        const float fi = static_cast<float>(i);
        const float u = h01(fi * 1.7f);
        const float off = (h01(fi * 2.9f) - 0.5f) * (h01(fi * 6.1f) * 0.5f + 0.5f) * g.y * 0.18f;
        const sf::Vector2f p{u * g.x, g.y * (0.85f - 0.7f * u) + off};
        const float tw = 0.6f + 0.4f * std::sin(t * 0.8f + fi * 0.37f);
        draw::disc(w, p, 0.5f + 0.7f * h01(fi * 8.3f), withAlpha(sf::Color(210, 215, 255), 0.35f * tw * a),
                   withAlpha(sf::Color::White, 0.f), {1.f, 1.f}, 5);
    }
}

void twinStars(sf::RenderTarget& w, sf::Vector2f g, float t, float a) {   // binary stars dancing round each other
    for (int k = 0; k < 7; ++k) {
        const float fk = static_cast<float>(k);
        const sf::Vector2f c = anywhere(g, 50.f + fk * 3.f);
        const float an = t * (0.9f + 0.3f * fk) + fk;
        const sf::Vector2f d{std::cos(an) * 14.f, std::sin(an) * 6.f};
        const sf::Color c1(255, 200, 140), c2(150, 190, 255);
        draw::glow(w, c + d, 12.f, c1, 0.5f * a);
        draw::glow(w, c - d, 10.f, c2, 0.5f * a);
        draw::disc(w, c + d, 2.2f, withAlpha(sf::Color::White, 0.9f * a), withAlpha(c1, 0.f), {1.f, 1.f}, 8);
        draw::disc(w, c - d, 1.8f, withAlpha(sf::Color::White, 0.9f * a), withAlpha(c2, 0.f), {1.f, 1.f}, 8);
    }
}

void pulsar(sf::RenderTarget& w, sf::Vector2f g, float t, float a) {   // a spinning star sweeping two beams
    const sf::Vector2f c = anywhere(g, 71.f);
    const float an = t * 2.2f;
    for (int s = -1; s <= 1; s += 2) {
        const sf::Vector2f d{std::cos(an) * static_cast<float>(s), std::sin(an) * static_cast<float>(s)};
        draw::line(w, c, c + d * 90.f, 2.f, sf::Color(180, 220, 255, static_cast<sf::Uint8>(110 * a)));
        draw::line(w, c, c + d * 50.f, 4.f, sf::Color(200, 230, 255, static_cast<sf::Uint8>(70 * a)));
    }
    draw::glow(w, c, 16.f, sf::Color(170, 210, 255), (0.5f + 0.4f * std::sin(t * 9.f)) * a);
    draw::disc(w, c, 2.5f, withAlpha(sf::Color::White, a), withAlpha(sf::Color::White, 0.f), {1.f, 1.f}, 8);
}

void supernovae(sf::RenderTarget& w, sf::Vector2f g, float t, float a) {   // now and then a star goes out with a bang
    for (int k = 0; k < 6; ++k) {
        const float fk = static_cast<float>(k);
        const float period = 11.f + 4.f * fk;
        const float cyc = std::floor((t + fk * 3.7f) / period);
        const float u = std::fmod(t + fk * 3.7f, period) / 3.5f;   // 3.5 s of it, then quiet
        if (u > 1.f) continue;
        const sf::Vector2f c = anywhere(g, 90.f + cyc * 7.f + fk);
        const float flash = std::max(0.f, 1.f - u * 4.f);
        draw::glow(w, c, 30.f + 40.f * flash, sf::Color(255, 245, 220), (0.3f + 0.7f * flash) * a);
        const sf::Color shell = h01(cyc + fk) < 0.5f ? sf::Color(255, 140, 120) : sf::Color(140, 200, 255);
        draw::ring(w, c, 8.f + 120.f * u, 2.5f * (1.f - u) + 0.5f, withAlpha(shell, 0.6f * (1.f - u) * a));
        draw::glow(w, c, 20.f + 110.f * u, shell, 0.12f * (1.f - u) * a);
    }
}

void brokenPlanets(sf::RenderTarget& w, sf::Vector2f g, float t, float a) {   // a planet breaking apart, debris flying
    for (int k = 0; k < 4; ++k) {
        const float fk = static_cast<float>(k);
        const float period = 16.f + 5.f * fk;
        const float cyc = std::floor((t + fk * 6.f) / period);
        const float u = std::fmod(t + fk * 6.f, period) / 6.f;
        const sf::Vector2f c = anywhere(g, 130.f + cyc * 5.f + fk);
        const sf::Color pc = h01(cyc * 3.f + fk) < 0.5f ? sf::Color(200, 120, 90) : sf::Color(110, 160, 210);
        if (u < 0.3f) {   // whole, then cracking: a glow building inside
            const float crack = u / 0.3f;
            draw::disc(w, c, 9.f, withAlpha(pc, 0.8f * a), withAlpha(lerpColor(pc, sf::Color::Black, 0.5f), 0.8f * a), {1.f, 1.f}, 20);
            draw::glow(w, c, 10.f + 10.f * crack, sf::Color(255, 200, 120), 0.5f * crack * a);
        } else if (u < 1.f) {   // pieces flying out, fading
            const float v = (u - 0.3f) / 0.7f;
            for (int i = 0; i < 9; ++i) {
                const float an = static_cast<float>(i) / 9.f * 2.f * kPi + cyc;
                const sf::Vector2f d{std::cos(an), std::sin(an)};
                draw::polygon(w, c + d * (6.f + 70.f * v * (0.7f + 0.6f * h01(static_cast<float>(i) + cyc))), 2.5f, 4, an + v * 6.f,
                              withAlpha(pc, 0.8f * (1.f - v) * a), withAlpha(lerpColor(pc, sf::Color::Black, 0.4f), 0.8f * (1.f - v) * a));
            }
            draw::glow(w, c, 20.f + 40.f * v, sf::Color(255, 180, 100), 0.4f * (1.f - v) * a);
        }
    }
}

void blackHole(sf::RenderTarget& w, sf::Vector2f g, float t, float a) {   // a dark heart with a glowing disc
    const sf::Vector2f c = anywhere(g, 211.f);
    for (int i = 0; i < 90; ++i) {   // the accretion disc, flattened, streaming round
        const float fi = static_cast<float>(i);
        const float an = t * (1.5f - 0.8f * h01(fi)) + fi * 0.37f;
        const float rr = 20.f + 34.f * h01(fi * 2.3f);
        const sf::Vector2f q{std::cos(an) * rr, std::sin(an) * rr * 0.3f};
        draw::disc(w, c + q, 1.2f, sf::Color(255, 190, 120, static_cast<sf::Uint8>(160 * a * (1.f - (rr - 20.f) / 40.f))),
                   sf::Color(255, 190, 120, 0), {1.f, 1.f}, 5);
    }
    draw::glow(w, c, 40.f, sf::Color(255, 170, 100), 0.18f * a);
    draw::disc(w, c, 13.f, sf::Color(0, 0, 0, static_cast<sf::Uint8>(255 * a)), sf::Color(0, 0, 0, static_cast<sf::Uint8>(255 * a)), {1.f, 1.f}, 24);
    draw::ring(w, c, 14.f, 1.5f, sf::Color(255, 220, 170, static_cast<sf::Uint8>(200 * a)));
}

// ---- one motif per class -----------------------------------------------------

void striker(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // comets streaking across
    for (int k = 0; k < 16; ++k) {
        const float fk = static_cast<float>(k);
        const float period = 4.5f + 1.7f * fk;
        const float u = std::fmod(t + period * (0.37f * fk + 0.1f), period) / period;
        const float n = std::floor((t + period * (0.37f * fk + 0.1f)) / period);
        const sf::Vector2f from{g.x * h01(n * 3.3f + fk), -60.f};
        const sf::Vector2f dir = normalized(sf::Vector2f{h01(n * 5.7f + fk) < 0.5f ? -0.6f : 0.6f, 1.f});
        const sf::Vector2f p = from + dir * (u * (g.y + 120.f) * 1.3f);
        draw::line(w, p - dir * 130.f, p, 2.f, withAlpha(c, 0.16f * a));
        draw::line(w, p - dir * 50.f, p, 2.5f, withAlpha(c, 0.35f * a));
        draw::glow(w, p, 10.f, c, 0.55f * a);
    }
}

void guardian(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // a small planet in a turning hex shield
    const sf::Vector2f p = inView(g, 0.9f, 0.44f);
    const float R = g.y / kGalaxy * 0.04f;
    draw::glow(w, p, R * 3.f, c, 0.14f * a);
    draw::disc(w, p, R, withAlpha(lerpColor(c, sf::Color::Black, 0.55f), 0.8f * a), withAlpha(lerpColor(c, sf::Color::Black, 0.3f), 0.6f * a),
               {1.f, 1.f}, 32);
    draw::polygonOutline(w, p, R * 1.9f, 6, t * 0.15f, 1.5f, withAlpha(c, 0.4f * a));   // the shield
    draw::polygonOutline(w, p, R * 2.3f, 6, -t * 0.1f + 0.5f, 1.f, withAlpha(c, 0.2f * a));
    for (int i = 0; i < 3; ++i) {   // three little guards circling it
        const float an = t * 0.5f + static_cast<float>(i) * 2.094f;
        draw::disc(w, p + sf::Vector2f{std::cos(an), std::sin(an)} * (R * 2.7f), 2.f, withAlpha(c, 0.75f * a),
                   withAlpha(c, 0.f), {1.f, 1.f}, 8);
    }
}

void support(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // constellations all over
    for (int k = 0; k < 11; ++k) {
        const float fk = static_cast<float>(k);
        const sf::Vector2f o = anywhere(g, 300.f + fk * 11.f);
        sf::Vector2f pts[6];
        for (int i = 0; i < 6; ++i)
            pts[i] = o + sf::Vector2f{(h01(fk * 9.f + static_cast<float>(i)) - 0.5f) * 160.f,
                                      (h01(fk * 13.f + static_cast<float>(i) * 2.f) - 0.5f) * 90.f};
        for (int i = 0; i < 5; ++i) draw::line(w, pts[i], pts[i + 1], 1.f, withAlpha(c, 0.16f * a));
        for (int i = 0; i < 6; ++i) {
            const float tw = 0.6f + 0.4f * std::sin(t * 1.3f + static_cast<float>(i) * 1.7f + fk);
            draw::glow(w, pts[i], 8.f, c, 0.45f * tw * a);
            draw::disc(w, pts[i], 1.8f, withAlpha(sf::Color::White, 0.7f * tw * a), withAlpha(c, 0.f), {1.f, 1.f}, 8);
        }
    }
}

void mage(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // a magic circle, turning
    const sf::Vector2f p = inView(g, 0.1f, 0.52f);
    const float R = g.y / kGalaxy * 0.09f;
    draw::glow(w, p, R * 1.4f, c, 0.12f * a);
    draw::ring(w, p, R, 1.5f, withAlpha(c, 0.35f * a));
    draw::ring(w, p, R * 0.78f, 1.f, withAlpha(c, 0.25f * a));
    for (int tri = 0; tri < 2; ++tri)   // a hexagram
        draw::polygonOutline(w, p, R * 0.74f, 3, t * 0.12f + static_cast<float>(tri) * kPi, 1.2f, withAlpha(c, 0.32f * a));
    for (int i = 0; i < 12; ++i) {   // rune ticks between the rings, turning the other way
        const float an = -t * 0.08f + static_cast<float>(i) / 12.f * 2.f * kPi;
        const sf::Vector2f d{std::cos(an), std::sin(an)};
        draw::line(w, p + d * (R * 0.82f), p + d * (R * 0.95f), 1.5f, withAlpha(c, 0.4f * a));
    }
    for (int i = 0; i < 6; ++i) {   // glowing points at the star's tips
        const float an = t * 0.12f + static_cast<float>(i) * kPi / 3.f - kPi / 2.f;
        draw::glow(w, p + sf::Vector2f{std::cos(an), std::sin(an)} * (R * 0.74f), 6.f, c,
                   (0.4f + 0.3f * std::sin(t * 2.f + static_cast<float>(i))) * a);
    }
}

void shooter(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // far-off tracers crossing the whole galaxy
    for (int k = 0; k < 26; ++k) {
        const float fk = static_cast<float>(k);
        const float period = 3.f + fk * 0.53f;
        const float u = std::fmod(t + fk * 0.91f, period) / period;
        const float n = std::floor((t + fk * 0.91f) / period);
        const float y = g.y * h01(n * 2.1f + fk * 3.3f);
        const bool ltr = h01(n + fk * 5.f) < 0.5f;
        const float x = ltr ? -40.f + (g.x + 80.f) * u : g.x + 40.f - (g.x + 80.f) * u;
        draw::line(w, {x + (ltr ? -30.f : 30.f), y}, {x, y}, 1.f, withAlpha(c, 0.22f * a));
    }
}

void assassin(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // eyes in the dark that open and watch you
    for (int k = 0; k < 16; ++k) {
        const float fk = static_cast<float>(k);
        const sf::Vector2f p = anywhere(g, 400.f + fk * 7.f);
        // each pair opens now and then, holds, blinks shut
        const float open = clampf(std::sin(t * (0.35f + 0.08f * fk) + fk * 2.1f) * 3.f - 1.6f, 0.f, 1.f);
        if (open <= 0.f) continue;
        const sf::Vector2f look = normalized(gLook - p, {0.f, 1.f}) * 2.6f;   // they follow the middle of the screen
        for (int e = -1; e <= 1; e += 2) {
            const sf::Vector2f q = p + sf::Vector2f{static_cast<float>(e) * 15.f, 0.f};
            draw::glow(w, q, 16.f, sf::Color(255, 50, 70), 0.35f * open * a);
            // the almond: an eyelid opening top and bottom
            draw::disc(w, q, 7.f, withAlpha(sf::Color(255, 210, 200), 0.85f * open * a), withAlpha(sf::Color(200, 60, 70), 0.6f * open * a),
                       {1.f, 0.42f * open}, 14);
            draw::disc(w, q + look, 2.6f, withAlpha(sf::Color(200, 20, 40), open * a), withAlpha(sf::Color(120, 0, 20), open * a),
                       {1.f, std::min(1.f, 0.9f * open + 0.1f)}, 10);   // the iris, looking at you
            draw::disc(w, q + look, 1.1f, sf::Color(0, 0, 0, static_cast<sf::Uint8>(255 * open * a)), sf::Color(0, 0, 0, 0), {0.5f, 1.f}, 8);
        }
        (void)c;
    }
}

void summoner(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // wisps rising; now and then a dragon flies by
    for (int k = 0; k < 50; ++k) {
        const float fk = static_cast<float>(k);
        const float speed = 14.f + 10.f * h01(fk * 3.3f);
        const float y = g.y + 20.f - std::fmod(t * speed + h01(fk * 1.9f) * g.y, g.y + 40.f);
        const float x = g.x * h01(fk * 7.7f) + std::sin(t * 0.8f + fk) * 18.f;
        draw::glow(w, {x, y}, 9.f, c, 0.5f * a);
        draw::disc(w, {x, y}, 1.6f, withAlpha(sf::Color::White, 0.5f * a), withAlpha(c, 0.f), {1.f, 1.f}, 8);
    }
    // the dragon: every ~18 s it crosses the galaxy on a long gentle wave
    const float period = 18.f, cross = 9.f;
    const float u = std::fmod(t + 5.f, period) / cross;
    if (u > 1.f) return;
    const float n = std::floor((t + 5.f) / period);
    const bool ltr = h01(n) < 0.5f;
    const float baseY = g.y * (0.3f + 0.4f * h01(n * 3.1f));
    auto at = [&](float v) {
        const float x = ltr ? -150.f + (g.x + 300.f) * v : g.x + 150.f - (g.x + 300.f) * v;
        return sf::Vector2f{x, baseY + std::sin(v * 9.f) * 40.f};
    };
    // the Elder dragon's look, a different kind each pass: classic, wyvern, four wings, serpent
    const int form = static_cast<int>(n) % 4;
    const sf::Vector2f head = at(u), dir = normalized(head - at(std::max(0.f, u - 0.004f)), {1.f, 0.f});
    sf::Vector2f trail[14];
    for (int i = 0; i < 14; ++i) trail[i] = at(std::max(0.f, u - 0.0045f * static_cast<float>(i)));
    drawDragonForm(w, form, head, dir, dir, form == 3 ? 70.f : 60.f, t * 5.f, lerpColor(c, sf::Color::White, 0.15f), 0.25f * a,
                   trail, 14, 1.f);
}

void jester(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // cards tumbling down everywhere
    const sf::Color suits[4] = {c, sf::Color(255, 90, 110), sf::Color(240, 240, 255), sf::Color(255, 210, 90)};
    for (int k = 0; k < 40; ++k) {
        const float fk = static_cast<float>(k);
        const float speed = 10.f + 10.f * h01(fk * 2.3f);
        const float y = std::fmod(t * speed + h01(fk * 4.1f) * g.y, g.y + 60.f) - 30.f;
        const float x = g.x * h01(fk * 9.1f) + std::sin(t * 0.5f + fk) * 30.f;
        const float rot = t * (0.4f + 0.1f * fk);
        const sf::Vector2f ax{std::cos(rot), std::sin(rot)}, ay{-ax.y, ax.x};
        const sf::Vector2f p{x, y};
        const float wdt = 9.f, hgt = 13.f * std::fabs(std::cos(t * 0.9f + fk));   // it flips
        const sf::Vector2f q[4] = {p - ax * wdt - ay * hgt, p + ax * wdt - ay * hgt, p + ax * wdt + ay * hgt, p - ax * wdt + ay * hgt};
        const sf::Color sc = suits[k % 4];
        for (int i = 0; i < 4; ++i) draw::line(w, q[i], q[(i + 1) % 4], 1.f, withAlpha(sc, 0.3f * a));
        draw::disc(w, p, 1.6f, withAlpha(sc, 0.5f * a), withAlpha(sc, 0.f), {1.f, 1.f}, 4);
    }
}

void slinger(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // throws arcing across the galaxy
    for (int k = 0; k < 8; ++k) {
        const float fk = static_cast<float>(k);
        const sf::Vector2f from = anywhere(g, 500.f + fk * 5.f), to = from + sf::Vector2f{g.x * 0.25f, (h01(fk) - 0.5f) * 60.f};
        const float lift = g.y * 0.15f;
        auto at = [&](float v) { return from + (to - from) * v - sf::Vector2f{0.f, lift * 4.f * v * (1.f - v)}; };
        for (int i = 0; i < 40; ++i)
            draw::disc(w, at(static_cast<float>(i) / 39.f), 1.f, withAlpha(c, 0.18f * a), withAlpha(c, 0.f), {1.f, 1.f}, 6);
        const float u = std::fmod(t / (2.5f + fk) + fk * 0.3f, 1.f);
        draw::glow(w, at(u), 8.f, c, 0.55f * a);
        for (int i = 1; i <= 5; ++i)
            draw::disc(w, at(std::max(0.f, u - 0.02f * static_cast<float>(i))), 1.6f, withAlpha(c, 0.3f * a), withAlpha(c, 0.f), {1.f, 1.f}, 6);
    }
}

void alchemist(sf::RenderTarget& w, sf::Vector2f g, sf::Color c, float t, float a) {   // nebulae of its elements, breathing
    const sf::Color mix[] = {c, theme::elemPoison, theme::elemFire, theme::elemIce, sf::Color(200, 120, 255)};
    for (int k = 0; k < 11; ++k) {
        const float fk = static_cast<float>(k);
        const sf::Vector2f o = anywhere(g, 600.f + fk * 9.f);
        const float breathe = 0.85f + 0.15f * std::sin(t * 0.4f + fk);
        for (int i = 0; i < 4; ++i) {   // each nebula a few overlapping clouds in two colours
            const float fi = static_cast<float>(i);
            const sf::Vector2f q = o + sf::Vector2f{(h01(fk * 5.f + fi) - 0.5f) * 140.f, (h01(fk * 7.f + fi) - 0.5f) * 90.f} +
                                   sf::Vector2f{std::sin(t * 0.05f + fi), std::cos(t * 0.04f + fi)} * 12.f;
            draw::glow(w, q, (70.f + 50.f * h01(fk + fi * 3.f)) * breathe, mix[(k + i) % 5], 0.13f * a);
        }
    }
}

void astronomer(sf::RenderTarget&, sf::Vector2f, sf::Color, float, float) {}   // the gods: never on the web

using Motif = void (*)(sf::RenderTarget&, sf::Vector2f, sf::Color, float, float);
// In BallRole order (Striker ... the gods).
constexpr Motif kMotifs[kClassCount] = {striker, guardian, support, mage, shooter, assassin, summoner, jester, slinger, alchemist,
                                        astronomer, astronomer, astronomer, astronomer, astronomer, astronomer, astronomer, astronomer, astronomer, astronomer};

// The galaxy itself, by how far the web has come (n = classes unlocked).
void galaxy(sf::RenderTarget& w, sf::Vector2f g, int n, const std::array<sf::Color, kClassCount>& col,
            const std::array<float, kClassCount>& on, float t, float alpha) {
    auto layer = [&](int from) { return alpha * clampf((static_cast<float>(n) - static_cast<float>(from) + 1.f) / 2.f, 0.f, 1.f); };
    stars(w, g, 110 * n, t, alpha);
    if (n >= 3) nebula(w, g, col, on, t, layer(3));
    if (n >= 4)   // spiral galaxies, far off, turning
        for (int k = 0; k < 6; ++k) {
            const float fk = static_cast<float>(k);
            spiralGalaxy(w, anywhere(g, 700.f + fk * 13.f), 80.f + 70.f * h01(fk * 3.f), 0.35f + 0.4f * h01(fk * 5.f),
                         t * (0.05f + 0.03f * fk) * (k % 2 ? -1.f : 1.f), k == 1 ? sf::Color(255, 190, 150) : sf::Color(160, 180, 255),
                         layer(4));
        }
    if (n >= 6) twinStars(w, g, t, layer(6));
    if (n >= 7) pulsar(w, g, t, layer(7));
    if (n >= 8) milkyWay(w, g, t, layer(8));
    if (n >= 9) supernovae(w, g, t, layer(9));
    if (n >= 10) {
        brokenPlanets(w, g, t, layer(10));
        blackHole(w, g, t, layer(10));
    }
}

}  // namespace

void drawWebSkyClass(sf::RenderTarget& w, sf::Vector2f size, int cls, float t) {
    if (cls < 0 || cls >= kClassCount) return;
    const sf::View keep = w.getView();
    const sf::Vector2f g = size * kGalaxy;
    gLook = g * 0.5f;
    w.setView(sf::View(g * 0.5f, size));   // the part on screen at rest
    stars(w, g, 160, t, 1.f);
    kMotifs[cls](w, g, roleColor(static_cast<BallRole>(cls + 1)), t, 1.f);
    w.setView(keep);
}

void drawWebSkyStage(sf::RenderTarget& w, sf::Vector2f size, int classes, float t) {
    const sf::View keep = w.getView();
    const sf::Vector2f g = size * kGalaxy;
    std::array<sf::Color, kClassCount> col{};
    std::array<float, kClassCount> on{};
    for (int i = 0; i < kClassCount; ++i) {
        col[static_cast<std::size_t>(i)] = roleColor(static_cast<BallRole>(i + 1));
        on[static_cast<std::size_t>(i)] = i < classes ? 1.f : 0.f;
    }
    w.setView(sf::View(g * 0.5f, g));   // the whole galaxy, to see where everything is
    galaxy(w, g, classes, col, on, t, 1.f);
    w.setView(keep);
}

int webSkyClasses(const int* unlock) {
    int n = 0;
    for (int i = 0; i < kClassCount; ++i)
        if (classUnlockNode(classTag(i)) >= 0 && unlock[classUnlockNode(classTag(i))] > 0) ++n;
    return n;
}

void drawWebSky(sf::RenderWindow& w, sf::Vector2f size, sf::Vector2f centre, sf::Vector2f pan, const int* unlock, float t,
                float scale, float alpha) {
    SkyState& st = gSky;
    const float dt = st.ready ? clampf(t - st.lastT, 0.f, 0.1f) : 0.f;
    st.lastT = t;
    std::array<bool, kClassCount> has{};
    std::array<sf::Color, kClassCount> col{};
    int n = 0;
    for (int i = 0; i < kClassCount; ++i) {
        const int node = classUnlockNode(classTag(i));
        has[static_cast<std::size_t>(i)] = node >= 0 && unlock[node] > 0;
        col[static_cast<std::size_t>(i)] = roleColor(static_cast<BallRole>(i + 1));
        if (has[static_cast<std::size_t>(i)]) ++n;
    }
    for (int i = 0; i < kClassCount; ++i) {   // fade each motif in; a new one flares
        float& ap = st.appear[static_cast<std::size_t>(i)];
        float& fl = st.flare[static_cast<std::size_t>(i)];
        if (!has[static_cast<std::size_t>(i)]) { ap = 0.f; fl = 0.f; continue; }
        if (!st.ready) { ap = 1.f; continue; }   // already there when the web opens
        if (ap <= 0.f) st.flareAt[static_cast<std::size_t>(i)] = wallSeconds();   // just arrived
        ap = std::min(1.f, std::max(ap, 0.001f) + dt / 2.5f);   // (off zero at once: it flares only once)
        fl = clampf(1.f - static_cast<float>(wallSeconds() - st.flareAt[static_cast<std::size_t>(i)]) / 1.6f, 0.f, 1.f);
    }
    st.ready = true;
    if (alpha <= 0.01f || n == 0) return;   // nothing unlocked: a black sky

    // The galaxy drifts with the camera: half its pan, clamped to its edges.
    const sf::Vector2f g = size * kGalaxy;
    const sf::Vector2f room = (g - size) * 0.5f;
    const sf::Vector2f off{clampf(-pan.x * kDrift, -room.x, room.x), clampf(-pan.y * kDrift, -room.y, room.y)};
    const sf::View keep = w.getView();
    gLook = g * 0.5f + off;
    w.setView(sf::View(g * 0.5f + off, size));
    galaxy(w, g, n, col, st.appear, t, alpha);
    for (int i = 0; i < kClassCount; ++i) {
        const float ap = st.appear[static_cast<std::size_t>(i)];
        if (ap > 0.f) kMotifs[i](w, g, col[static_cast<std::size_t>(i)], t, alpha * ap);
    }
    w.setView(keep);
    if (n >= 5) drawOrbitingPlanets(w, centre, t, scale, alpha * clampf((static_cast<float>(n) - 4.f) / 2.f, 0.f, 1.f));
    for (int i = 0; i < kClassCount; ++i)   // a class just arrived: a thin ring of its colour, widening
        if (const float fl = st.flare[static_cast<std::size_t>(i)]; fl > 0.f)
            draw::ring(w, centre, size.y * (0.1f + 0.6f * (1.f - fl)), 2.f, withAlpha(col[static_cast<std::size_t>(i)], 0.5f * fl * alpha));
}

}  // namespace sb
