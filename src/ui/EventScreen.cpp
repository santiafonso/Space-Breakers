// A "?" stop's event: deals for gold. Same cards as the Altar (drawRuleCard),
// in the "?" colour; a deal you can't take is greyed out.

#include "ui/EventScreen.hpp"

#include <cmath>
#include <string>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "progression/Events.hpp"
#include "render/Draw.hpp"
#include "ui/CreedScreen.hpp"
#include "ui/UiSound.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {
constexpr float kCardW = 300.f;
constexpr float kCardH = 340.f;
constexpr float kCardGap = 28.f;
bool isKey(const sf::Event& e, sf::Keyboard::Key k) { return e.type == sf::Event::KeyPressed && e.key.code == k; }
bool isLeftClick(const sf::Event& e) {
    return e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left;
}
}  // namespace

sf::FloatRect EventScreen::cardRect(App& app, int i) const {
    const sf::Vector2f s = app.size();
    const int n = static_cast<int>(app.eventDeals().size());
    const float total = static_cast<float>(n) * kCardW + static_cast<float>(n - 1) * kCardGap;
    const float x = s.x * 0.5f - total * 0.5f + static_cast<float>(i) * (kCardW + kCardGap);
    return {x, s.y * 0.54f - kCardH * 0.5f, kCardW, kCardH};
}

sf::FloatRect EventScreen::leaveRect(App& app) const {
    const sf::Vector2f s = app.size();
    const float wd = 240.f, ht = 32.f;
    return {s.x * 0.5f - wd * 0.5f, s.y * 0.54f + kCardH * 0.5f + 22.f, wd, ht};
}

int EventScreen::cardAt(App& app, sf::Vector2f mouse) const {
    for (int i = 0; i < static_cast<int>(app.eventDeals().size()); ++i)
        if (cardRect(app, i).contains(mouse)) return i;
    return -1;
}

void EventScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    const int n = static_cast<int>(app.eventDeals().size());
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 && e.key.code < sf::Keyboard::Num1 + n) {
        app.takeEventDeal(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (isKey(e, sf::Keyboard::Escape)) { app.leaveEvent(); return; }
    if (!isLeftClick(e)) return;
    if (leaveRect(app).contains(mouse)) { app.leaveEvent(); return; }
    if (const int c = cardAt(app, mouse); c >= 0) app.takeEventDeal(c);
}

void EventScreen::update(App& app, float dt, sf::Vector2f mouse) {
    mouse_ = mouse;
    clock_ += dt;
    const float k = 1.f - std::exp(-14.f * dt);
    const int c = cardAt(app, mouse);
    for (int i = 0; i < 4; ++i) hover_[i] = lerpf(hover_[i], c == i ? 1.f : 0.f, k);
    leaveHover_ = lerpf(leaveHover_, leaveRect(app).contains(mouse) ? 1.f : 0.f, k);
    uisound::hover(this, c >= 0 ? c : (leaveRect(app).contains(mouse) ? 10 : -1));
}

void EventScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const sf::Font& f = app.font();
    const auto& deals = app.eventDeals();
    // The Rest stop reuses this screen: repair the core or forge, in the core's colour.
    const bool rest = !deals.empty() && deals.front() == EventKind::Rest;
    const sf::Color col = rest ? theme::core : theme::puSurge;   // the stop's colour on the map

    drawDim(w, s, 0.88f * clampf(introPop(it, 0.f, 0.25f), 0.f, 1.f));
    drawCenteredPop(w, f, rest ? "Rest" : "A stranger", theme::fsTitle, {s.x * 0.5f, s.y * 0.085f}, col,
                    introPop(it, 0.04f, 0.32f));
    if (!rest)
        drawCenteredPop(w, f, "gold  " + std::to_string(app.gold()), theme::fsBody, {s.x * 0.5f, s.y * 0.085f + 42.f},
                        theme::puGolden, introPop(it, 0.1f));

    const int n = static_cast<int>(deals.size());
    for (int i = 0; i < n; ++i) {
        const EventKind k = deals[static_cast<std::size_t>(i)];
        const bool ok = app.eventDealOk(k);
        const float cp = introPop(it, 0.14f + 0.1f * static_cast<float>(i), 0.42f);
        if (cp <= 0.001f) continue;
        const float a = clampf(cp, 0.f, 1.f) * (ok ? 1.f : 0.4f);
        const float h = ok ? hover_[i] : 0.f;
        sf::FloatRect r = cardRect(app, i);
        r.top += (1.f - clampf(cp, 0.f, 1.f)) * 50.f - 12.f * h;
        const float pulse = 0.5f + 0.5f * std::sin(clock_ * 1.8f + static_cast<float>(i) * 1.3f);
        const int gold = app.eventGoldOf(k);
        std::string gain = eventGain(k, gold), cost = eventCost(k, gold);
        if (k == EventKind::Rest && app.hasCreed(CreedId::Fortress)) gain = "the core is half repaired";
        if (k == EventKind::Rest && app.ironCoreAlive()) cost = "ends this act's Iron core";
        drawRuleCard(w, f, r, col, a, h, pulse, std::to_string(i + 1) + (rest ? "   CHOICE" : "   DEAL"),
                     ok ? eventHint(k) : "not enough gold", eventName(k), gain, cost);
    }

    {   // walk away: nothing gained, nothing lost
        const float a = clampf(introPop(it, 0.2f + 0.1f * static_cast<float>(n)), 0.f, 1.f);
        const sf::FloatRect r = leaveRect(app);
        draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::bg, col, 0.08f + 0.2f * leaveHover_), a),
                  withAlpha(lerpColor(theme::bg, col, 0.03f), a), withAlpha(col, (0.25f + 0.5f * leaveHover_) * a), 1.5f);
        drawCentered(w, f, rest ? "Move on" : "Walk away", theme::fsSmall, {r.left + r.width * 0.5f, r.top + r.height * 0.5f - 1.f},
                     withAlpha(theme::textLo, a));
    }
}

}  // namespace sb
