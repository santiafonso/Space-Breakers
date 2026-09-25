#include "render/Draw.hpp"

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

}  // namespace sb::draw
