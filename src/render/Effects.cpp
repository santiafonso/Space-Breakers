#include "render/Effects.hpp"

#include <algorithm>

#include "render/Draw.hpp"
#include <cmath>

namespace sb {

namespace {
constexpr std::size_t kMaxRings = 28;
}

void Effects::init(const sf::Font& font, sf::Vector2f size) {
    font_ = &font;
    size_ = size;
}

void Effects::clear() {
    rings_.clear();
    coins_.clear();
    labels_.clear();
    banners_.clear();
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

void Effects::addBurst(sf::Vector2f pos, float radius, sf::Color color) {
    if (rings_.size() >= kMaxRings) rings_.erase(rings_.begin());
    Ring r;
    r.pos = pos;
    r.life = 0.45f;
    r.r0 = radius * 0.3f;
    r.r1 = radius;
    r.color = color;
    r.burst = true;
    rings_.push_back(r);
}

void Effects::addPop(sf::Vector2f pos, float radius, sf::Color color) {
    if (rings_.size() >= kMaxRings) rings_.erase(rings_.begin());
    Ring r;
    r.pos = pos;
    r.life = 0.24f;
    r.r0 = radius * 0.6f;
    r.r1 = radius * 1.8f;
    r.color = color;
    r.pop = true;
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

void Effects::addCoin(sf::Vector2f pos, sf::Vector2f target, float radius) {
    if (coins_.size() >= 60) return;
    Coin c;
    c.pos = pos;
    c.target = target;
    c.radius = radius;
    // Pop up and a little sideways before being pulled to the counter.
    const float a = -kPi * 0.5f + (static_cast<float>(coins_.size() % 7) - 3.f) * 0.22f;
    c.vel = sf::Vector2f{std::cos(a), std::sin(a)} * 260.f;
    coins_.push_back(c);
}

int Effects::takeArrivedCoins() {
    const int n = arrived_;
    arrived_ = 0;
    return n;
}

void Effects::flash(sf::Color color, float strength) {
    flashColor_ = color;
    flash_ = std::max(flash_, clampf(strength, 0.f, 1.f));
}

void Effects::classBanner(const std::string& caption, const std::string& from, const std::string& to,
                          sf::Color color, bool ascended) {
    if (banners_.size() >= 4) return;   // a flood (a creed handing out balls) keeps the first few
    Banner b;
    b.caption = caption;
    b.from = from;
    b.to = to;
    b.color = color;
    b.ascended = ascended;
    b.life = ascended ? 2.8f : 2.2f;
    banners_.push_back(b);
}

void Effects::update(float dt) {
    if (!banners_.empty()) {
        // the next one in the queue starts a touch early, as the current fades
        banners_.front().age += dt;
        if (banners_.front().age >= banners_.front().life) banners_.erase(banners_.begin());
    }
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
    for (auto it = coins_.begin(); it != coins_.end();) {
        Coin& c = *it;
        c.age += dt;
        if (c.age < 0.22f) {
            c.vel *= std::exp(-6.f * dt);   // the pop, easing off
        } else {
            // Then home in on the gold counter, faster and faster.
            const sf::Vector2f d = c.target - c.pos;
            const float pull = 900.f + 2600.f * (c.age - 0.22f);
            c.vel += normalized(d, {1.f, 0.f}) * pull * dt;
            c.vel *= std::exp(-2.5f * dt);
        }
        c.pos += c.vel * dt;
        if ((c.age > 0.22f && length(c.target - c.pos) < 14.f) || c.age > 2.5f) {
            ++arrived_;
            it = coins_.erase(it);
        } else {
            ++it;
        }
    }
    for (float& e : edge_) e *= std::exp(-6.f * dt);
    flash_ *= std::exp(-6.f * dt);
}

void Effects::drawBorder(sf::RenderWindow& window) const {
    // The arena frame: a hairline all round, corner brackets and a small notch
    // at the middle of each side, like the bezel of an instrument. A wall that
    // was just hit lights up along its whole length.
    const float W = size_.x, H = size_.y;
    const sf::Color hair = withAlpha(theme::arenaEdge, 0.9f);
    draw::box(window, {0.5f, 0.5f, W - 1.f, H - 1.f}, 0.f, sf::Color::Transparent, sf::Color::Transparent, hair, 1.f);
    const sf::Color mark = lerpColor(theme::arenaEdge, theme::accent, 0.3f);
    draw::brackets(window, {3.f, 3.f, W - 6.f, H - 6.f}, 26.f, 2.f, mark);
    draw::line(window, {W * 0.5f - 14.f, 3.f}, {W * 0.5f + 14.f, 3.f}, 2.f, mark);
    draw::line(window, {W * 0.5f - 14.f, H - 3.f}, {W * 0.5f + 14.f, H - 3.f}, 2.f, mark);
    draw::line(window, {3.f, H * 0.5f - 14.f}, {3.f, H * 0.5f + 14.f}, 2.f, mark);
    draw::line(window, {W - 3.f, H * 0.5f - 14.f}, {W - 3.f, H * 0.5f + 14.f}, 2.f, mark);

    const float th = 3.f;
    const sf::FloatRect bars[4] = {{0.f, 0.f, th, H}, {W - th, 0.f, th, H}, {0.f, 0.f, W, th}, {0.f, H - th, W, th}};
    for (int i = 0; i < 4; ++i) {
        if (edge_[i] < 0.02f) continue;
        sf::RectangleShape bar({bars[i].width, bars[i].height});
        bar.setPosition(bars[i].left, bars[i].top);
        bar.setFillColor(withAlpha(theme::accent, 0.8f * edge_[i]));
        window.draw(bar);
    }
}

void Effects::drawRings(sf::RenderWindow& window) const {
    for (const Ring& r : rings_) {
        const float t = r.age / r.life;
        if (r.pop) {   // a bright disc that swells and fades - the enemy bursting
            const float rad = lerpf(r.r0, r.r1, 1.f - (1.f - t) * (1.f - t));
            draw::disc(window, r.pos, rad, withAlpha(lerpColor(r.color, sf::Color::White, 0.5f), (1.f - t) * 0.7f),
                       withAlpha(r.color, 0.f), {1.f, 1.f}, 28);
            continue;
        }
        sf::CircleShape c(lerpf(r.r0, r.r1, t), 32);
        c.setOrigin(c.getRadius(), c.getRadius());
        c.setPosition(r.pos);
        c.setFillColor(sf::Color::Transparent);
        c.setOutlineThickness(r.burst ? 4.f : 2.f);
        c.setOutlineColor(withAlpha(r.color, (1.f - t) * (r.burst ? 0.8f : 0.5f)));
        if (r.burst) c.setFillColor(withAlpha(r.color, (1.f - t) * 0.12f));   // a faint fill for area hits
        window.draw(c);
    }
}

// The class-gain band: dark glass across the screen, hairlines and corner
// brackets in the class colour; it opens from a line, holds, and fades.
void Effects::drawBanner(sf::RenderWindow& window, const Banner& b) const {
    if (!font_) return;
    const float t = b.age;
    const float open = clampf(t / 0.18f, 0.f, 1.f);
    const float fade = clampf((b.life - t) / 0.45f, 0.f, 1.f);
    const float a = fade;
    const float cy = size_.y * 0.22f;
    const float h = (b.ascended ? 86.f : 72.f) * (1.f - (1.f - open) * (1.f - open));
    const sf::FloatRect band{0.f, cy - h * 0.5f, size_.x, h};
    draw::box(window, band, 0.f, withAlpha(lerpColor(theme::glassTop, b.color, 0.12f), 0.92f * a),
              withAlpha(theme::glassBottom, 0.92f * a));
    for (float y : {band.top, band.top + band.height})
        draw::line(window, {0.f, y}, {size_.x, y}, 1.f, withAlpha(b.color, 0.55f * a));
    if (open < 1.f) return;

    const float in = clampf((t - 0.18f) / 0.25f, 0.f, 1.f);   // the words slide in
    const float ta = a * in;
    auto text = [&](const std::string& str, unsigned sz, sf::Color c, float spacing) {
        sf::Text x(str, *font_, sz);
        x.setLetterSpacing(spacing);
        x.setFillColor(withAlpha(c, ta));
        return x;
    };
    auto place = [&](sf::Text& x, float left, float y) {
        const sf::FloatRect lb = x.getLocalBounds();
        x.setOrigin(lb.left, lb.top + lb.height * 0.5f);
        x.setPosition(std::round(left), std::round(y));
    };

    sf::Text cap = text(b.caption, 12, theme::textLo, 2.2f);
    place(cap, size_.x * 0.5f - cap.getLocalBounds().width * 0.5f, band.top + 15.f);
    window.draw(cap);

    const float ty = band.top + h * 0.5f + (b.ascended ? 10.f : 8.f);
    const unsigned big = b.ascended ? 34u : 30u;
    sf::Text to = text(b.to, big, lerpColor(b.color, sf::Color::White, b.ascended ? 0.2f : 0.f), 1.6f);
    const float toW = to.getLocalBounds().width;
    const float slide = (1.f - in) * 18.f;
    if (b.from.empty()) {
        place(to, size_.x * 0.5f - toW * 0.5f + slide, ty);
        window.draw(to);
        const float half = toW * 0.5f + 26.f;
        draw::brackets(window, {size_.x * 0.5f - half, ty - 24.f, 2.f * half, 48.f}, 10.f, 2.f,
                       withAlpha(b.color, 0.8f * ta), 4.f * (1.f - in));
        return;
    }
    sf::Text from = text(b.from, 20, theme::textLo, 1.6f);
    const float fromW = from.getLocalBounds().width;
    const float gap = 44.f;
    const float x0 = size_.x * 0.5f - (fromW + gap + toW) * 0.5f;
    place(from, x0, ty);
    window.draw(from);
    // a drawn arrow (no glyph to depend on)
    const float ax = x0 + fromW + 12.f;
    const sf::Color ac = withAlpha(b.color, 0.8f * ta);
    draw::line(window, {ax, ty}, {ax + 20.f, ty}, 2.f, ac);
    draw::line(window, {ax + 13.f, ty - 6.f}, {ax + 20.5f, ty + 0.5f}, 2.f, ac);
    draw::line(window, {ax + 13.f, ty + 6.f}, {ax + 20.5f, ty - 0.5f}, 2.f, ac);
    place(to, x0 + fromW + gap + slide, ty);
    window.draw(to);
}

void Effects::drawOverlay(sf::RenderWindow& window) const {
    if (flash_ > 0.01f) {
        sf::RectangleShape r(size_);
        r.setFillColor(withAlpha(flashColor_, flash_ * 0.16f));
        window.draw(r);
    }
    for (const Coin& c : coins_) {
        sf::CircleShape coin(c.radius, 18);
        coin.setOrigin(c.radius, c.radius);
        coin.setPosition(c.pos);
        coin.setFillColor(theme::puGolden);
        coin.setOutlineThickness(1.5f);
        coin.setOutlineColor(withAlpha(theme::ember, 0.8f));
        window.draw(coin);
    }
    for (const Label& l : labels_) {
        const float t = l.age / l.life;
        const float fade = t < 0.15f ? t / 0.15f : 1.f - (t - 0.15f) / 0.85f;
        sf::Text text = l.text;
        sf::Color c = text.getFillColor();
        text.setFillColor(withAlpha(c, clampf(fade, 0.f, 1.f)));
        window.draw(text);
    }
    if (!banners_.empty()) drawBanner(window, banners_.front());
}

}  // namespace sb
