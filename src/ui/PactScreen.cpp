// The Altar: the pact choice (2026-09-28). Same cards as the creeds
// (drawRuleCard), in the pact colour.

#include "ui/PactScreen.hpp"

#include <cmath>
#include <string>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "render/Draw.hpp"
#include "ui/CreedScreen.hpp"
#include "ui/UiSound.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {
constexpr float kCardH = 372.f;
constexpr float kCardGap = 22.f;
}  // namespace

sf::FloatRect PactScreen::cardRect(App& app, int i) const {
    const sf::Vector2f s = app.size();
    const int n = static_cast<int>(app.pactChoices().size());
    const float cw = 300.f;
    const float total = static_cast<float>(n) * cw + static_cast<float>(n - 1) * kCardGap;
    const float x = s.x * 0.5f - total * 0.5f + static_cast<float>(i) * (cw + kCardGap);
    return {x, s.y * 0.54f - kCardH * 0.5f, cw, kCardH};
}

sf::FloatRect PactScreen::leaveRect(App& app) const {
    const sf::Vector2f s = app.size();
    const float wd = 240.f, ht = 32.f;
    return {s.x * 0.5f - wd * 0.5f, s.y * 0.54f + kCardH * 0.5f + 22.f, wd, ht};
}

int PactScreen::cardAt(App& app, sf::Vector2f mouse) const {
    for (int i = 0; i < static_cast<int>(app.pactChoices().size()); ++i)
        if (cardRect(app, i).contains(mouse)) return i;
    return -1;
}

void PactScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    const int n = static_cast<int>(app.pactChoices().size());
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 && e.key.code < sf::Keyboard::Num1 + n) {
        app.choosePact(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) { app.refusePacts(); return; }
    if (e.type != sf::Event::MouseButtonPressed || e.mouseButton.button != sf::Mouse::Left) return;
    if (leaveRect(app).contains(mouse)) { app.refusePacts(); return; }
    if (const int c = cardAt(app, mouse); c >= 0) app.choosePact(c);
}

void PactScreen::update(App& app, float dt, sf::Vector2f mouse) {
    mouse_ = mouse;
    clock_ += dt;
    const float k = 1.f - std::exp(-14.f * dt);
    const int c = cardAt(app, mouse);
    for (int i = 0; i < 4; ++i) hover_[i] = lerpf(hover_[i], c == i ? 1.f : 0.f, k);
    leaveHover_ = lerpf(leaveHover_, leaveRect(app).contains(mouse) ? 1.f : 0.f, k);
    uisound::hover(this, c >= 0 ? c : (leaveRect(app).contains(mouse) ? 10 : -1));
}

void PactScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const sf::Font& f = app.font();
    const sf::Color col = theme::pact;

    drawDim(w, s, 0.88f * clampf(introPop(it, 0.f, 0.25f), 0.f, 1.f));
    drawCenteredPop(w, f, "Altar", theme::fsTitle, {s.x * 0.5f, s.y * 0.085f}, col, introPop(it, 0.04f, 0.32f));
    drawCenteredPop(w, f, "Every gift here has a price. Take one - or walk away.", theme::fsBody,
                    {s.x * 0.5f, s.y * 0.085f + 40.f}, theme::textLo, introPop(it, 0.1f));
    if (!app.data().run.pacts.empty() || !app.data().run.creeds.empty())
        drawCreedStrip(app, w, {s.x * 0.5f, s.y * 0.085f + 68.f}, true, mouse_, false);

    const auto& picks = app.pactChoices();
    const int n = static_cast<int>(picks.size());
    for (int i = 0; i < n; ++i) {
        const PactDef& d = pactDef(picks[static_cast<std::size_t>(i)]);
        const float cp = introPop(it, 0.14f + 0.1f * static_cast<float>(i), 0.42f);
        if (cp <= 0.001f) continue;
        const float a = clampf(cp, 0.f, 1.f);
        const float h = hover_[i];
        sf::FloatRect r = cardRect(app, i);
        r.top += (1.f - a) * 50.f - 12.f * h;
        const float pulse = 0.5f + 0.5f * std::sin(clock_ * 1.8f + static_cast<float>(i) * 1.3f);
        drawRuleCard(w, f, r, col, a, h, pulse, std::to_string(i + 1) + "   PACT",
                     d.needsHands ? "for runs where you grab balls" : "for any run", d.name, d.gain, d.cost);
    }

    {   // walk away: nothing gained, nothing lost
        const float a = clampf(introPop(it, 0.2f + 0.1f * static_cast<float>(n)), 0.f, 1.f);
        const sf::FloatRect r = leaveRect(app);
        draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::bg, col, 0.08f + 0.2f * leaveHover_), a),
                  withAlpha(lerpColor(theme::bg, col, 0.03f), a), withAlpha(col, (0.25f + 0.5f * leaveHover_) * a), 1.5f);
        drawCentered(w, f, "Walk away", theme::fsSmall, {r.left + r.width * 0.5f, r.top + r.height * 0.5f - 1.f},
                     withAlpha(theme::textLo, a));
    }

    if (const int c = cardAt(app, mouse_); c >= 0)
        drawTooltip(w, f, mouse_, s, "Pairs well with", pactDef(picks[static_cast<std::size_t>(c)]).synergy, col);
}

}  // namespace sb
