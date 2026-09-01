#include "sim/Collision.hpp"

#include <algorithm>
#include <cmath>

namespace sb::collision {

Contact circleVsBounds(Ball& b, sf::Vector2f size) {
    const float r = b.radius;
    Contact c;
    if (b.pos.x - r < 0.f) {
        b.pos.x = r;
        b.vel.x = std::fabs(b.vel.x);
        c.normal = {1.f, 0.f};
        c.point = {0.f, b.pos.y};
        c.hit = true;
    } else if (b.pos.x + r > size.x) {
        b.pos.x = size.x - r;
        b.vel.x = -std::fabs(b.vel.x);
        c.normal = {-1.f, 0.f};
        c.point = {size.x, b.pos.y};
        c.hit = true;
    }
    if (b.pos.y - r < 0.f) {
        b.pos.y = r;
        b.vel.y = std::fabs(b.vel.y);
        c.normal = {0.f, 1.f};
        c.point = {b.pos.x, 0.f};
        c.hit = true;
    } else if (b.pos.y + r > size.y) {
        b.pos.y = size.y - r;
        b.vel.y = -std::fabs(b.vel.y);
        c.normal = {0.f, -1.f};
        c.point = {b.pos.x, size.y};
        c.hit = true;
    }
    return c;
}

Contact circleVsWall(Ball& b, const Wall& w) {
    const float left = w.pos.x - w.half.x;
    const float right = w.pos.x + w.half.x;
    const float top = w.pos.y - w.half.y;
    const float bottom = w.pos.y + w.half.y;
    const float cx = clampf(b.pos.x, left, right);
    const float cy = clampf(b.pos.y, top, bottom);
    const sf::Vector2f d = b.pos - sf::Vector2f(cx, cy);
    const float dist2 = dot(d, d);
    if (dist2 >= b.radius * b.radius) return {};

    Contact c;
    if (dist2 > 1e-6f) {
        const float dist = std::sqrt(dist2);
        c.normal = d / dist;
        b.pos += c.normal * (b.radius - dist);
    } else {
        // Centre is inside the wall: shove out along the shallowest face.
        const float l = b.pos.x - left, rr = right - b.pos.x;
        const float t = b.pos.y - top, bo = bottom - b.pos.y;
        const float m = std::min({l, rr, t, bo});
        if (m == l) { b.pos.x = left - b.radius; c.normal = {-1.f, 0.f}; }
        else if (m == rr) { b.pos.x = right + b.radius; c.normal = {1.f, 0.f}; }
        else if (m == t) { b.pos.y = top - b.radius; c.normal = {0.f, -1.f}; }
        else { b.pos.y = bottom + b.radius; c.normal = {0.f, 1.f}; }
    }

    sf::Vector2f rel = b.vel - w.vel;
    const float vn = dot(rel, c.normal);
    if (vn < 0.f) rel -= 2.f * vn * c.normal;
    b.vel = rel + w.vel;
    c.point = {cx, cy};
    c.hit = true;
    return c;
}

void pushOutOfWall(Ball& b, const Wall& w) {
    const float cx = clampf(b.pos.x, w.pos.x - w.half.x, w.pos.x + w.half.x);
    const float cy = clampf(b.pos.y, w.pos.y - w.half.y, w.pos.y + w.half.y);
    const sf::Vector2f d = b.pos - sf::Vector2f(cx, cy);
    const float dist = length(d);
    if (dist < b.radius && dist > 1e-4f) b.pos += normalized(d) * (b.radius - dist);
}

void resolveBallPair(Ball& a, Ball& b) {
    const sf::Vector2f d = b.pos - a.pos;
    const float dist = length(d);
    const float minDist = a.radius + b.radius;
    if (dist <= 1e-4f || dist >= minDist) return;

    const sf::Vector2f n = d / dist;
    const float push = (minDist - dist) * 0.5f;
    a.pos -= n * push;
    b.pos += n * push;

    const float va = dot(a.vel, n);
    const float vb = dot(b.vel, n);
    if (vb - va < 0.f) {
        a.vel += (vb - va) * n;
        b.vel += (va - vb) * n;
    }
}

}  // namespace sb::collision
