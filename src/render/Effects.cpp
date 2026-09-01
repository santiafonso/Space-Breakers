#include "render/Effects.hpp"

#include <algorithm>

namespace sb {

namespace {
constexpr std::size_t kMaxRings = 18;
}

void Effects::init(const sf::Font& font, sf::Vector2f size) {
    font_ = &font;
    size_ = size;
}

void Effects::clear() {
    rings_.clear();
    labels_.clear();
    for (float& e : edge_) e = 0.f;
    flash_ = 0.f;
}

void Effects::addRing(sf::Vector2f pos, float speed, sf::Color color) {
    // Skip near-duplicates so a fast multiball volley doesn't stack rings.
    for (const Ring& r : rings_)
        if (r.age < 0.05f && length(r.pos - pos) < 10.f) return;

    if (rings_.size() >= kMaxRings) rings_.erase(rings_.begin());
    Ring r;
    r.pos = pos;
    r.life = 0.38f;
    r.r0 = 10.f;
    r.r1 = 30.f + speed * 0.03f;
    r.color = color;
    rings_.push_back(r);
}

void Effects::edgeHit(sf::Vector2f normal) {
    if (normal.x > 0.5f) edge_[0] = 1.f;
    else if (normal.x < -0.5f) edge_[1] = 1.f;
    if (normal.y > 0.5f) edge_[2] = 1.f;
    else if (normal.y < -0.5f) edge_[3] = 1.f;
}

void Effects::addLabel(const std::string& text, sf::Vector2f pos, sf::Color color,
                       unsigned size, float life) {
    if (!font_) return;
    Label l;
    l.text.setFont(*font_);
    l.text.setCharacterSize(size);
    l.text.setString(text);
    l.text.setFillColor(color);
    const sf::FloatRect b = l.text.getLocalBounds();
    l.text.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    l.text.setPosition(pos);
    l.life = life;
    l.vel = {0.f, -26.f};
    labels_.push_back(l);
}

void Effects::flash(sf::Color color, float strength) {
    flashColor_ = color;
    flash_ = std::max(flash_, clampf(strength, 0.f, 1.f));
}

void Effects::update(float dt) {
    for (auto it = rings_.begin(); it != rings_.end();) {
        it->age += dt;
        if (it->age >= it->life) it = rings_.erase(it);
        else ++it;
    }
    for (auto it = labels_.begin(); it != labels_.end();) {
        it->age += dt;
        it->text.move(it->vel * dt);
        if (it->age >= it->life) it = labels_.erase(it);
        else ++it;
    }
    for (float& e : edge_) e *= std::exp(-6.f * dt);
    flash_ *= std::exp(-6.f * dt);
}

void Effects::drawBorder(sf::RenderWindow& window) const {
    const float thickness = 2.f;
    const struct {
        sf::Vector2f size;
        sf::Vector2f pos;
    } bars[4] = {
        {{thickness, size_.y}, {0.f, 0.f}},
        {{thickness, size_.y}, {size_.x - thickness, 0.f}},
        {{size_.x, thickness}, {0.f, 0.f}},
        {{size_.x, thickness}, {0.f, size_.y - thickness}},
    };
    for (int i = 0; i < 4; ++i) {
        sf::RectangleShape bar(bars[i].size);
        bar.setPosition(bars[i].pos);
        bar.setFillColor(lerpColor(theme::arenaEdge, theme::accent, edge_[i]));
        window.draw(bar);
    }
}

void Effects::drawRings(sf::RenderWindow& window) const {
    for (const Ring& r : rings_) {
        const float t = r.age / r.life;
        sf::CircleShape c(lerpf(r.r0, r.r1, t), 32);
        c.setOrigin(c.getRadius(), c.getRadius());
        c.setPosition(r.pos);
        c.setFillColor(sf::Color::Transparent);
        c.setOutlineThickness(2.f);
        c.setOutlineColor(withAlpha(r.color, (1.f - t) * 0.5f));
        window.draw(c);
    }
}

void Effects::drawOverlay(sf::RenderWindow& window) const {
    if (flash_ > 0.01f) {
        sf::RectangleShape r(size_);
        r.setFillColor(withAlpha(flashColor_, flash_ * 0.16f));
        window.draw(r);
    }
    for (const Label& l : labels_) {
        const float t = l.age / l.life;
        const float fade = t < 0.15f ? t / 0.15f : 1.f - (t - 0.15f) / 0.85f;
        sf::Text text = l.text;
        sf::Color c = text.getFillColor();
        text.setFillColor(withAlpha(c, clampf(fade, 0.f, 1.f)));
        window.draw(text);
    }
}

}  // namespace sb
