#include "render/Draw.hpp"

#include "core/Theme.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace sb::draw {

namespace {

constexpr float kTau = 6.2831853f;

sf::Color mix(sf::Color a, sf::Color b, float t) {
    auto m = [t](sf::Uint8 x, sf::Uint8 y) {
        return static_cast<sf::Uint8>(std::lround(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t));
    };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

sf::Color alphaScaled(sf::Color c, float a) {
    c.a = static_cast<sf::Uint8>(std::clamp(a, 0.f, 1.f) * static_cast<float>(c.a));
    return c;
}

// Outline points of a rounded rectangle, clockwise, each with its outward
// normal (the direction from its corner's arc centre).
struct EdgePt { sf::Vector2f p, n; };
std::vector<EdgePt> roundedOutline(sf::FloatRect r, float corner) {
    corner = std::max(0.5f, std::min({corner, r.width * 0.5f, r.height * 0.5f}));
    const int seg = 6;
    const sf::Vector2f centres[4] = {{r.left + r.width - corner, r.top + corner},
                                     {r.left + r.width - corner, r.top + r.height - corner},
                                     {r.left + corner, r.top + r.height - corner},
                                     {r.left + corner, r.top + corner}};
    const float start[4] = {-kTau / 4.f, 0.f, kTau / 4.f, kTau / 2.f};
    std::vector<EdgePt> pts;
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i <= seg; ++i) {
            const float a = start[c] + (kTau / 4.f) * static_cast<float>(i) / static_cast<float>(seg);
            const sf::Vector2f n{std::cos(a), std::sin(a)};
            pts.push_back({centres[c] + n * corner, n});
        }
    return pts;
}

}  // namespace

void glow(sf::RenderTarget& t, sf::Vector2f pos, float radius, sf::Color color, float alpha) {
    if (alpha <= 0.003f || radius <= 0.5f) return;
    const int n = 28;
    sf::VertexArray fan(sf::TriangleFan, n + 2);
    fan[0].position = pos;
    fan[0].color = alphaScaled(color, alpha);
    sf::Color edge = color;
    edge.a = 0;
    for (int i = 0; i <= n; ++i) {
        const float a = kTau * static_cast<float>(i) / static_cast<float>(n);
        fan[static_cast<std::size_t>(i) + 1].position = pos + sf::Vector2f{std::cos(a), std::sin(a)} * radius;
        fan[static_cast<std::size_t>(i) + 1].color = edge;
    }
    t.draw(fan, sf::RenderStates(sf::BlendAdd));
}

void disc(sf::RenderTarget& t, sf::Vector2f pos, float radius, sf::Color inner, sf::Color outer,
          sf::Vector2f scale, int points) {
    sf::VertexArray fan(sf::TriangleFan, static_cast<std::size_t>(points) + 2);
    fan[0].position = pos;
    fan[0].color = inner;
    for (int i = 0; i <= points; ++i) {
        const float a = kTau * static_cast<float>(i) / static_cast<float>(points);
        fan[static_cast<std::size_t>(i) + 1].position =
            pos + sf::Vector2f{std::cos(a) * scale.x, std::sin(a) * scale.y} * radius;
        fan[static_cast<std::size_t>(i) + 1].color = outer;
    }
    t.draw(fan);
}

void polygon(sf::RenderTarget& t, sf::Vector2f pos, float radius, int sides, float rot,
             sf::Color inner, sf::Color outer) {
    sf::VertexArray fan(sf::TriangleFan, static_cast<std::size_t>(sides) + 2);
    fan[0].position = pos;
    fan[0].color = inner;
    for (int i = 0; i <= sides; ++i) {
        const float a = rot + kTau * static_cast<float>(i) / static_cast<float>(sides);
        fan[static_cast<std::size_t>(i) + 1].position = pos + sf::Vector2f{std::cos(a), std::sin(a)} * radius;
        fan[static_cast<std::size_t>(i) + 1].color = outer;
    }
    t.draw(fan);
}

void polygonOutline(sf::RenderTarget& t, sf::Vector2f pos, float radius, int sides, float rot,
                    float thickness, sf::Color color) {
    sf::VertexArray strip(sf::TriangleStrip, static_cast<std::size_t>(sides) * 2 + 2);
    for (int i = 0; i <= sides; ++i) {
        const float a = rot + kTau * static_cast<float>(i) / static_cast<float>(sides);
        const sf::Vector2f d{std::cos(a), std::sin(a)};
        strip[static_cast<std::size_t>(i) * 2].position = pos + d * (radius + thickness * 0.5f);
        strip[static_cast<std::size_t>(i) * 2 + 1].position = pos + d * (radius - thickness * 0.5f);
        strip[static_cast<std::size_t>(i) * 2].color = strip[static_cast<std::size_t>(i) * 2 + 1].color = color;
    }
    t.draw(strip);
}

void ring(sf::RenderTarget& t, sf::Vector2f pos, float radius, float thickness, sf::Color color,
          float a0, float a1, int segments) {
    if (a1 <= a0) return;
    const int n = std::max(2, static_cast<int>(static_cast<float>(segments) * (a1 - a0) / kTau));
    sf::VertexArray strip(sf::TriangleStrip, static_cast<std::size_t>(n) * 2 + 2);
    for (int i = 0; i <= n; ++i) {
        const float a = a0 + (a1 - a0) * static_cast<float>(i) / static_cast<float>(n);
        const sf::Vector2f d{std::cos(a), std::sin(a)};
        strip[static_cast<std::size_t>(i) * 2].position = pos + d * (radius + thickness * 0.5f);
        strip[static_cast<std::size_t>(i) * 2 + 1].position = pos + d * (radius - thickness * 0.5f);
        strip[static_cast<std::size_t>(i) * 2].color = strip[static_cast<std::size_t>(i) * 2 + 1].color = color;
    }
    t.draw(strip);
}

void box(sf::RenderTarget& t, sf::FloatRect r, float corner, sf::Color top, sf::Color bottom,
         sf::Color outline, float thickness) {
    const std::vector<EdgePt> pts = roundedOutline(r, corner);
    sf::VertexArray fan(sf::TriangleFan, pts.size() + 2);
    fan[0].position = {r.left + r.width * 0.5f, r.top + r.height * 0.5f};
    fan[0].color = mix(top, bottom, 0.5f);
    for (std::size_t i = 0; i <= pts.size(); ++i) {
        const sf::Vector2f p = pts[i % pts.size()].p;
        fan[i + 1].position = p;
        fan[i + 1].color = mix(top, bottom, r.height > 0.f ? (p.y - r.top) / r.height : 0.f);
    }
    t.draw(fan);
    if (thickness <= 0.f || outline.a == 0) return;
    sf::VertexArray edge(sf::TriangleStrip, pts.size() * 2 + 2);
    for (std::size_t i = 0; i <= pts.size(); ++i) {
        const EdgePt& e = pts[i % pts.size()];
        edge[i * 2].position = e.p + e.n * (thickness * 0.5f);
        edge[i * 2 + 1].position = e.p - e.n * (thickness * 0.5f);
        edge[i * 2].color = edge[i * 2 + 1].color = outline;
    }
    t.draw(edge);
}

void line(sf::RenderTarget& t, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color color) {
    const sf::Vector2f d = b - a;
    const float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 0.01f || color.a == 0) return;
    const sf::Vector2f n = sf::Vector2f{-d.y, d.x} / len * (thickness * 0.5f);
    sf::Vertex q[4] = {{a + n, color}, {b + n, color}, {b - n, color}, {a - n, color}};
    t.draw(q, 4, sf::Quads);
}

void brackets(sf::RenderTarget& t, sf::FloatRect r, float arm, float thickness, sf::Color color,
              float out) {
    if (color.a == 0) return;
    const float l = r.left - out, tp = r.top - out;
    const float rt = r.left + r.width + out, bt = r.top + r.height + out;
    const float th = thickness;
    sf::VertexArray q(sf::Quads);
    auto rect = [&](float x, float y, float w, float h) {
        q.append({{x, y}, color});
        q.append({{x + w, y}, color});
        q.append({{x + w, y + h}, color});
        q.append({{x, y + h}, color});
    };
    rect(l, tp, arm, th);            rect(l, tp + th, th, arm - th);             // top-left
    rect(rt - arm, tp, arm, th);     rect(rt - th, tp + th, th, arm - th);       // top-right
    rect(l, bt - th, arm, th);       rect(l, bt - arm, th, arm - th);            // bottom-left
    rect(rt - arm, bt - th, arm, th); rect(rt - th, bt - arm, th, arm - th);     // bottom-right
    t.draw(q);
}

void panel(sf::RenderTarget& t, sf::FloatRect r, sf::Color edge, float alpha, float lit) {
    const sf::Color top = theme::glassTop, bottom = theme::glassBottom;
    box(t, r, 0.f, alphaScaled(mix(top, edge, 0.10f + 0.14f * lit), 0.96f * alpha),
        alphaScaled(mix(bottom, edge, 0.03f + 0.05f * lit), 0.96f * alpha),
        alphaScaled(edge, (0.22f + 0.4f * lit) * alpha), 1.f);
    // A hairline of light along the inside of the top edge: reads as glass.
    line(t, {r.left + 1.f, r.top + 1.5f}, {r.left + r.width - 1.f, r.top + 1.5f}, 1.f,
         alphaScaled(sf::Color::White, (0.05f + 0.05f * lit) * alpha));
    brackets(t, r, theme::bracket, 2.f, alphaScaled(edge, (0.55f + 0.45f * lit) * alpha));
}

void vignette(sf::RenderTarget& t, sf::Vector2f size, sf::Color edge, float strength) {
    // Concentric annuli from mid-screen out past the corners, each band's alpha
    // following a smooth ease: a clean radial falloff with no fan artefacts.
    const sf::Vector2f c = size * 0.5f;
    const float maxD = std::sqrt(c.x * c.x + c.y * c.y);
    const int bands = 24, seg = 64;
    const float r0 = 0.35f * maxD;
    auto alphaAt = [&](float r) {
        const float x = std::clamp((r - r0) / (maxD - r0), 0.f, 1.f);
        return alphaScaled(edge, strength * x * x * (3.f - 2.f * x));
    };
    sf::VertexArray strip(sf::Triangles);
    for (int b = 0; b < bands; ++b) {
        const float ra = r0 + (maxD * 1.02f - r0) * static_cast<float>(b) / static_cast<float>(bands);
        const float rb = r0 + (maxD * 1.02f - r0) * static_cast<float>(b + 1) / static_cast<float>(bands);
        const sf::Color ca = alphaAt(ra), cb = alphaAt(rb);
        for (int i = 0; i < seg; ++i) {
            const float a0 = kTau * static_cast<float>(i) / static_cast<float>(seg);
            const float a1 = kTau * static_cast<float>(i + 1) / static_cast<float>(seg);
            const sf::Vector2f d0{std::cos(a0), std::sin(a0)}, d1{std::cos(a1), std::sin(a1)};
            strip.append({c + d0 * ra, ca});
            strip.append({c + d0 * rb, cb});
            strip.append({c + d1 * rb, cb});
            strip.append({c + d0 * ra, ca});
            strip.append({c + d1 * rb, cb});
            strip.append({c + d1 * ra, ca});
        }
    }
    t.draw(strip);
}

void radar(sf::RenderTarget& t, sf::Vector2f c, float maxR, float gap, int spokes, sf::Color color,
           float alpha) {
    if (alpha <= 0.003f) return;
    const sf::Color ringCol = alphaScaled(color, alpha);
    int k = 1;
    for (float rad = gap; rad <= maxR; rad += gap, ++k)
        ring(t, c, rad, 1.f, alphaScaled(ringCol, k % 2 == 0 ? 1.f : 0.6f), 0.f, kTau,
             std::max(48, static_cast<int>(rad * 0.35f)));
    sf::VertexArray lines(sf::Lines);
    for (int i = 0; i < spokes; ++i) {
        const float a = kTau * static_cast<float>(i) / static_cast<float>(spokes);
        const sf::Vector2f d{std::cos(a), std::sin(a)};
        lines.append({c + d * gap * 0.5f, alphaScaled(ringCol, 0.f)});   // fades in off the centre
        lines.append({c + d * maxR, alphaScaled(ringCol, 0.7f)});
        // small ticks across each ring where a spoke crosses it
        const sf::Vector2f n{-d.y, d.x};
        for (float rad = gap; rad <= maxR; rad += gap) {
            lines.append({c + d * rad - n * 4.f, alphaScaled(ringCol, 1.6f)});
            lines.append({c + d * rad + n * 4.f, alphaScaled(ringCol, 1.6f)});
        }
    }
    t.draw(lines);
}

}  // namespace sb::draw
