// "Calling" (run intro): pick the class of your starting ball. One card per
// unlocked class that has items; the ball starts with two of its items.

#include <algorithm>
#include <cmath>
#include <string>

#include "core/App.hpp"
#include "core/Theme.hpp"
#include "render/ClassRender.hpp"
#include "render/Draw.hpp"
#include "ui/Screens.hpp"
#include "ui/UiSound.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {
constexpr float kCardH = 230.f;
constexpr float kGap = 18.f;
}  // namespace

sf::FloatRect ClassPickScreen::cardRect(App& app, int i) const {
    const sf::Vector2f s = app.size();
    const int n = std::max(1, static_cast<int>(app.classChoices().size()));
    const float wd = std::min(230.f, (s.x - 2.f * theme::margin - kGap * static_cast<float>(n - 1)) / static_cast<float>(n));
    const float total = wd * static_cast<float>(n) + kGap * static_cast<float>(n - 1);
    return {s.x * 0.5f - total * 0.5f + static_cast<float>(i) * (wd + kGap), s.y * 0.5f - kCardH * 0.5f, wd, kCardH};
}

int ClassPickScreen::cardAt(App& app, sf::Vector2f mouse) const {
    for (int i = 0; i < static_cast<int>(app.classChoices().size()); ++i)
        if (cardRect(app, i).contains(mouse)) return i;
    return -1;
}

void ClassPickScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    const int n = static_cast<int>(app.classChoices().size());
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 && e.key.code < sf::Keyboard::Num1 + n) {
        app.chooseClass(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (e.type != sf::Event::MouseButtonPressed || e.mouseButton.button != sf::Mouse::Left) return;
    if (const int c = cardAt(app, mouse); c >= 0) app.chooseClass(c);
}

void ClassPickScreen::update(App& app, float dt, sf::Vector2f mouse) {
    mouse_ = mouse;
    clock_ += dt;
    const int c = cardAt(app, mouse);
    const float k = 1.f - std::exp(-16.f * dt);
    for (int i = 0; i < kClassCount; ++i) hover_[i] = hover_[i] + ((c == i ? 1.f : 0.f) - hover_[i]) * k;
    uisound::hover(this, c);
}

void ClassPickScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const sf::Font& f = app.font();
    drawDim(w, s, 0.82f * clampf(introPop(it, 0.f, 0.2f), 0.f, 1.f));
    drawCenteredPop(w, f, "Choose your ball's class", theme::fsTitle, {s.x * 0.5f, s.y * 0.2f}, theme::textHi,
                    introPop(it, 0.04f, 0.3f));
    drawCenteredPop(w, f, "it starts with 2 items of that class - 2 of a tag make a class, 4 its ascended form",
                    theme::fsSmall, {s.x * 0.5f, s.y * 0.2f + 36.f}, theme::textLo, introPop(it, 0.08f, 0.3f));

    const auto& picks = app.classChoices();
    for (int i = 0; i < static_cast<int>(picks.size()); ++i) {
        const ItemTag t = picks[static_cast<std::size_t>(i)];
        const BallRole role = tagRole(t);
        const sf::Color col = tagColor(t);
        const float cp = introPop(it, 0.10f + 0.07f * static_cast<float>(i), 0.40f);
        if (cp <= 0.001f) continue;
        const float a = clampf(cp, 0.f, 1.f);
        const float h = hover_[i];
        sf::FloatRect r = cardRect(app, i);
        r.top += (1.f - a) * 40.f - 10.f * h;
        const float cx = r.left + r.width * 0.5f;

        draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::glassTop, col, 0.12f + 0.12f * h), 0.97f * a),
                  withAlpha(lerpColor(theme::glassBottom, col, 0.04f), 0.97f * a), withAlpha(col, (0.35f + 0.45f * h) * a), 1.f);
        draw::box(w, {r.left, r.top, r.width, 3.f}, 0.f, withAlpha(col, 0.9f * a), withAlpha(col, 0.9f * a));
        draw::brackets(w, r, theme::bracket + 3.f, 2.f, withAlpha(col, (0.6f + 0.4f * h) * a));

        drawLabel(w, f, std::to_string(i + 1), 10, {cx, r.top + 18.f}, withAlpha(theme::textDim, a));
        // The ball as it would look: plain body, the class mark.
        const sf::Vector2f bp{cx, r.top + 62.f};
        const float br = role == BallRole::Guardian ? 19.f : 16.f;
        draw::glow(w, bp, br * 1.8f, col, (0.06f + 0.06f * h) * a);
        draw::disc(w, bp, br, withAlpha(sf::Color(178, 184, 198), a), withAlpha(sf::Color(96, 102, 118), a));
        draw::ring(w, bp, br, 1.5f, withAlpha(sf::Color::White, 0.25f * a));
        drawClassMark(w, role, bp, br, -kPi * 0.25f, a);

        drawCentered(w, f, roleName(role), theme::fsItem, {cx, r.top + 108.f}, withAlpha(col, a));
        const std::vector<std::string> lines = wrapText(f, roleDesc(role), theme::fsSmall, r.width - 24.f);
        float y = r.top + 138.f;
        for (const std::string& l : lines) {
            drawCentered(w, f, l, theme::fsSmall, {cx, y}, withAlpha(theme::textLo, a));
            y += 17.f;
        }
        drawLabel(w, f, std::string("ascends to ") + ascendedName(role), 9, {cx, r.top + r.height - 16.f},
                  withAlpha(lerpColor(col, theme::textDim, 0.4f), a));
    }
    drawCenteredPop(w, f, "click a class or press 1-" + std::to_string(picks.size()), theme::fsSmall,
                    {s.x * 0.5f, s.y * 0.5f + kCardH * 0.5f + 34.f}, theme::textDim, introPop(it, 0.5f));
}

}  // namespace sb
