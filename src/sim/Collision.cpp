#include "sim/Collision.hpp"

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

Contact circleVsSolidCircle(Ball& b, sf::Vector2f center, float radius, float rebound) {
    const sf::Vector2f d = b.pos - center;
    const float sum = b.radius + radius;
    const float dist2 = dot(d, d);
    if (dist2 >= sum * sum) return {};

    Contact c;
    const float dist = std::sqrt(std::max(dist2, 1e-6f));
    c.normal = dist > 1e-4f ? d / dist : sf::Vector2f{0.f, -1.f};
    b.pos += c.normal * (sum - dist);  // separate

    const float vn = dot(b.vel, c.normal);
    if (vn < 0.f) b.vel -= (1.f + rebound) * vn * c.normal;
    c.point = center + c.normal * radius;
    c.hit = true;
    return c;
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
