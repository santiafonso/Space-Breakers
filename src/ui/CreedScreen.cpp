// The creed choice screen and the small creed read-outs (chips, fight HUD,
// Hunters tethers). Fase O.

#include "ui/CreedScreen.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "render/Draw.hpp"
#include "ui/UiSound.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

constexpr float kCardH = 372.f;
constexpr float kCardGap = 22.f;

bool isKey(const sf::Event& e, sf::Keyboard::Key k) {
    return e.type == sf::Event::KeyPressed && e.key.code == k;
}
bool isLeftClick(const sf::Event& e) {
    return e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left;
}

void seg(sf::RenderTarget& w, sf::Vector2f a, sf::Vector2f b, float thick, sf::Color c) {
    const sf::Vector2f d = b - a;
    sf::RectangleShape bar({length(d), thick});
    bar.setOrigin(0.f, thick * 0.5f);
    bar.setPosition(a);
    bar.setRotation(std::atan2(d.y, d.x) * 180.f / kPi);
    bar.setFillColor(c);
    w.draw(bar);
}

// Wrapped paragraph, left-aligned, returns the y after it.
float paragraph(sf::RenderWindow& w, const sf::Font& f, const std::string& s, unsigned size, float x, float y,
                float maxW, sf::Color col, float lineH) {
    for (const std::string& l : wrapText(f, s, size, maxW)) {
        sf::Text t = makeText(f, l, size, col);
        t.setPosition(std::round(x), std::round(y));
        w.draw(t);
        y += lineH;
    }
    return y;
}

std::string creedTip(const CreedDef& d) {
    return std::string("+ ") + d.gain + ".   - " + d.cost + ".";
}

}  // namespace

sf::Color creedColor(CreedArchetype a) {
    switch (a) {
        case CreedArchetype::Thrower:   return theme::ember;
        case CreedArchetype::Spectator: return theme::accent;
        case CreedArchetype::FewMighty: return theme::puSurge;
        case CreedArchetype::Swarm:     return theme::ballMid;
        case CreedArchetype::Core:      return theme::core;
        case CreedArchetype::Gambler:   return theme::puGolden;
        case CreedArchetype::Alchemist: return theme::venom;
        case CreedArchetype::Berserker: return theme::coreLow;
    }
    return theme::textHi;
}

void drawRuleCard(sf::RenderWindow& w, const sf::Font& f, sf::FloatRect r, sf::Color col, float a, float h,
                  float pulse, const std::string& head, const std::string& hint, const std::string& name,
                  const std::string& gain, const std::string& cost) {
    // Breathing halo in the card's colour, stronger under the pointer.
    const float grow = 4.f + 5.f * pulse + 4.f * h;
    draw::box(w, {r.left - grow, r.top - grow, r.width + 2.f * grow, r.height + 2.f * grow}, theme::corner,
              withAlpha(col, (0.04f + 0.06f * h) * a), withAlpha(col, 0.f),
              withAlpha(col, (0.16f + 0.2f * pulse * (0.5f + h)) * a), 2.f);
    draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::bg, col, 0.20f + 0.12f * h), 0.98f * a),
              withAlpha(lerpColor(theme::bg, col, 0.04f), 0.98f * a), withAlpha(col, (0.55f + 0.4f * h) * a), 2.f);
    draw::box(w, {r.left, r.top, r.width, 4.f}, theme::corner, withAlpha(col, a), withAlpha(col, a));   // colour cap

    const float cx = r.left + r.width * 0.5f;
    drawCentered(w, f, head, theme::fsSmall, {cx, r.top + 22.f}, withAlpha(col, a));
    drawCentered(w, f, hint, theme::fsSmall, {cx, r.top + 40.f}, withAlpha(theme::textDim, a));
    drawCentered(w, f, name, theme::fsTitle, {cx, r.top + 80.f}, withAlpha(theme::textHi, a));
    seg(w, {r.left + 24.f, r.top + 112.f}, {r.left + r.width - 24.f, r.top + 112.f}, 1.f, withAlpha(col, 0.35f * a));

    const float x = r.left + 20.f, tw = r.width - 40.f;
    sf::Text gh = makeText(f, "GAIN", theme::fsSmall, withAlpha(theme::core, a));
    gh.setPosition(x, r.top + 126.f);
    w.draw(gh);
    float y = paragraph(w, f, gain, theme::fsBody, x, r.top + 146.f, tw, withAlpha(theme::textHi, a), 21.f);
    y += 12.f;
    sf::Text ch = makeText(f, "COST", theme::fsSmall, withAlpha(theme::coreLow, a));
    ch.setPosition(x, y);
    w.draw(ch);
    paragraph(w, f, cost, theme::fsBody, x, y + 20.f, tw, withAlpha(lerpColor(theme::coreLow, theme::textHi, 0.35f), a), 21.f);
}

// ================================================================ CreedScreen

sf::FloatRect CreedScreen::cardRect(App& app, int i) const {
    const sf::Vector2f s = app.size();
    const int n = static_cast<int>(app.creedChoices().size());
    const float cw = n >= 4 ? 262.f : 300.f;
    const float total = static_cast<float>(n) * cw + static_cast<float>(n - 1) * kCardGap;
    const float x = s.x * 0.5f - total * 0.5f + static_cast<float>(i) * (cw + kCardGap);
    return {x, s.y * 0.54f - kCardH * 0.5f, cw, kCardH};
}

sf::FloatRect CreedScreen::refuseRect(App& app) const {
    const sf::Vector2f s = app.size();
    const float wd = 300.f, ht = 32.f;
    return {s.x * 0.5f - wd * 0.5f, s.y * 0.54f + kCardH * 0.5f + 22.f, wd, ht};
}

int CreedScreen::cardAt(App& app, sf::Vector2f mouse) const {
    for (int i = 0; i < static_cast<int>(app.creedChoices().size()); ++i)
        if (cardRect(app, i).contains(mouse)) return i;
    return -1;
}

void CreedScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    const int n = static_cast<int>(app.creedChoices().size());
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 && e.key.code < sf::Keyboard::Num1 + n) {
        app.chooseCreed(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (isKey(e, sf::Keyboard::Escape)) return;   // a creed is a decision: pick one or refuse them
    if (!isLeftClick(e)) return;
    if (refuseRect(app).contains(mouse)) { app.refuseCreeds(); return; }
    if (const int c = cardAt(app, mouse); c >= 0) app.chooseCreed(c);
}

void CreedScreen::update(App& app, float dt, sf::Vector2f mouse) {
    mouse_ = mouse;
    clock_ += dt;
    const float k = 1.f - std::exp(-14.f * dt);
    const int c = cardAt(app, mouse);
    for (int i = 0; i < 4; ++i) hover_[i] = lerpf(hover_[i], c == i ? 1.f : 0.f, k);
    refuseHover_ = lerpf(refuseHover_, refuseRect(app).contains(mouse) ? 1.f : 0.f, k);
    uisound::hover(this, c >= 0 ? c : (refuseRect(app).contains(mouse) ? 10 : -1));
}

void CreedScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float it = intro();
    const sf::Font& f = app.font();
    const bool boss = app.creedSource() == CreedSource::Boss;

    drawDim(w, s, 0.86f * clampf(introPop(it, 0.f, 0.25f), 0.f, 1.f));
    drawCenteredPop(w, f, "Choose a creed", theme::fsTitle, {s.x * 0.5f, s.y * 0.085f}, theme::textHi,
                    introPop(it, 0.04f, 0.32f));
    drawCenteredPop(w, f,
                    boss ? "The boss is down. Seal one rule for the rest of the run - it decides how you win from here."
                         : "Covenant: one rule for this whole run. The act-1 boss will offer a second one.",
                    theme::fsBody, {s.x * 0.5f, s.y * 0.085f + 40.f}, theme::textLo, introPop(it, 0.1f));
    if (!app.data().run.creeds.empty()) {
        const float y = s.y * 0.085f + 68.f;
        drawCenteredPop(w, f, "sealed so far:", theme::fsSmall, {s.x * 0.5f - 90.f, y + 10.f}, theme::textDim, introPop(it, 0.12f));
        drawCreedStrip(app, w, {s.x * 0.5f - 30.f, y}, false, mouse_, false);
    }

    const auto& picks = app.creedChoices();
    const int n = static_cast<int>(picks.size());
    for (int i = 0; i < n; ++i) {
        const CreedDef& d = creedDef(picks[static_cast<std::size_t>(i)]);
        const sf::Color col = creedColor(d.archetype);
        const float cp = introPop(it, 0.14f + 0.1f * static_cast<float>(i), 0.42f);
        if (cp <= 0.001f) continue;
        const float a = clampf(cp, 0.f, 1.f);
        const float h = hover_[i];
        sf::FloatRect r = cardRect(app, i);
        r.top += (1.f - a) * 50.f - 12.f * h;
        const float pulse = 0.5f + 0.5f * std::sin(clock_ * 2.4f + static_cast<float>(i));
        drawRuleCard(w, f, r, col, a, h, pulse, std::to_string(i + 1) + "   " + creedArchetypeName(d.archetype),
                     creedArchetypeHint(d.archetype), d.name, d.gain, d.cost);
    }

    // Refuse: all of them, for a little gold.
    {
        const float a = clampf(introPop(it, 0.2f + 0.1f * static_cast<float>(n)), 0.f, 1.f);
        const sf::FloatRect r = refuseRect(app);
        draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::bg, theme::puGolden, 0.10f + 0.2f * refuseHover_), a),
                  withAlpha(lerpColor(theme::bg, theme::puGolden, 0.03f), a),
                  withAlpha(theme::puGolden, (0.25f + 0.5f * refuseHover_) * a), 1.5f);
        drawCentered(w, f, "Refuse every creed   +" + std::to_string(cfg::creed::refuseGold) + " gold", theme::fsSmall,
                     {r.left + r.width * 0.5f, r.top + r.height * 0.5f - 1.f}, withAlpha(theme::textLo, a));
    }

    if (const int c = cardAt(app, mouse_); c >= 0) {
        const CreedDef& d = creedDef(picks[static_cast<std::size_t>(c)]);
        drawTooltip(w, f, mouse_, s, "Pairs well with", d.synergy, creedColor(d.archetype));
    } else if (refuseRect(app).contains(mouse_)) {
        drawTooltip(w, f, mouse_, s, "Refuse",
                    boss ? "no creed this run - take the gold and go straight to the boss treasure"
                         : "start the run without a creed - the boss can still offer one", theme::puGolden);
    }
}

// ================================================================ chips

float creedStripWidth(App& app) {
    constexpr float kPad = 9.f, kGap = 6.f;
    float total = -kGap;
    auto add = [&](const char* name) {
        total += makeText(app.font(), name, theme::fsSmall, theme::textHi).getLocalBounds().width + 2.f * kPad + kGap;
    };
    for (int id : app.data().run.creeds) add(creedDef(static_cast<CreedId>(id)).name);
    for (int id : app.data().run.pacts) add(pactDef(static_cast<PactId>(id)).name);
    return std::max(0.f, total);
}

bool drawCreedStrip(App& app, sf::RenderWindow& w, sf::Vector2f pos, bool centered, sf::Vector2f mouse, bool tips) {
    // The run's creeds, then its pacts (in the pact colour).
    struct Chip { std::string name, tipTitle, tip; sf::Color col; };
    std::vector<Chip> chips;
    for (int id : app.data().run.creeds) {
        const CreedDef& d = creedDef(static_cast<CreedId>(id));
        chips.push_back({d.name, std::string("Creed: ") + d.name, creedTip(d), creedColor(d.archetype)});
    }
    for (int id : app.data().run.pacts) {
        const PactDef& d = pactDef(static_cast<PactId>(id));
        chips.push_back({d.name, std::string("Pact: ") + d.name, std::string("+ ") + d.gain + ".   - " + d.cost + ".",
                         theme::pact});
    }
    if (chips.empty()) return false;
    const sf::Font& f = app.font();
    constexpr float kH = 22.f, kPad = 9.f, kGap = 6.f;
    std::vector<float> widths;
    float total = 0.f;
    for (const Chip& c : chips) {
        sf::Text t = makeText(f, c.name, theme::fsSmall, theme::textHi);
        widths.push_back(t.getLocalBounds().width + 2.f * kPad);
        total += widths.back() + kGap;
    }
    total -= kGap;
    float x = centered ? pos.x - total * 0.5f : pos.x;
    int hot = -1;
    for (std::size_t i = 0; i < chips.size(); ++i) {
        const sf::Color col = chips[i].col;
        const sf::FloatRect r{x, pos.y, widths[i], kH};
        const bool h = r.contains(mouse);
        if (h) hot = static_cast<int>(i);
        draw::box(w, r, theme::corner, withAlpha(lerpColor(theme::bg, col, h ? 0.4f : 0.24f), 0.95f),
                  withAlpha(lerpColor(theme::bg, col, 0.08f), 0.95f), withAlpha(col, h ? 0.95f : 0.6f), 1.f);
        drawCentered(w, f, chips[i].name, theme::fsSmall, {r.left + r.width * 0.5f, r.top + kH * 0.5f - 1.f}, theme::textHi);
        x += widths[i] + kGap;
    }
    if (hot >= 0 && tips) {
        const Chip& c = chips[static_cast<std::size_t>(hot)];
        drawTooltip(w, f, mouse, app.size(), c.tipTitle, c.tip, c.col);
    }
    return hot >= 0;
}

void drawCreedHud(App& app, sf::RenderWindow& w, sf::Vector2f mouse, bool tips) {
    if (app.data().run.creeds.empty() && app.data().run.pacts.empty()) return;
    const sf::Vector2f s = app.size();
    const float y = s.y - theme::margin - 92.f;
    bool shown = drawCreedStrip(app, w, {theme::margin, y}, false, mouse, tips);
    if (app.hasCreed(CreedId::Nova)) {   // the Nova charge, right above the chips
        const float cd = app.novaCooldown();
        const bool ready = cd <= 0.f;
        char buf[48];
        std::snprintf(buf, sizeof(buf), ready ? "NOVA ready - SPACE / right-click" : "NOVA  %.1fs", static_cast<double>(cd));
        sf::Text t = makeText(app.font(), buf, theme::fsSmall, ready ? theme::accent : theme::textDim);
        t.setPosition(theme::margin, y - 20.f);
        w.draw(t);
        if (!shown && tips && t.getGlobalBounds().contains(mouse))
            drawTooltip(w, app.font(), mouse, s, "Nova",
                        "SPACE or right-click: every ball bursts out of the core in a ring at triple speed, "
                        "and the core shoves nearby enemies away. Recharges in 7 s.", theme::accent);
    }
}

void drawCreedWorld(App& app, sf::RenderWindow& w) {
    const World& world = app.world();
    const auto& balls = world.balls();
    const float k = world.arenaScale();
    // "Hunters": a faint tether from each ball to its prey, and a small sight on it.
    const auto& prey = world.huntPrey();
    const auto& has = world.huntHas();
    for (std::size_t i = 0; i < balls.size() && i < prey.size() && i < has.size(); ++i) {
        if (!has[i]) continue;
        const sf::Color c = ballHue(balls[i]);
        seg(w, balls[i].pos, prey[i], 1.5f * k, withAlpha(c, 0.16f));
        draw::ring(w, prey[i], 30.f * k, 1.5f * k, withAlpha(c, 0.45f));
    }
    // "Living Core": an overcharged ball wears a core-coloured ring while it lasts.
    for (const Ball& b : balls)
        if (b.creedCharge > 0.f)
            draw::ring(w, b.pos, b.radius + 5.f * k, 2.f * k,
                       withAlpha(theme::core, 0.7f * clampf(b.creedCharge / cfg::creed::coreChargeTime, 0.f, 1.f)));
}

}  // namespace sb
