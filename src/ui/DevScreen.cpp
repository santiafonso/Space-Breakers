// The F1 dev panel: a gameplay test bench. Pick a target ball, grant it any
// pick, spawn enemy kinds, change the sim speed, jump to any between-wave
// screen. Only reachable with SB_DEV.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "core/App.hpp"
#include "core/Theme.hpp"
#include "render/Draw.hpp"
#include "ui/PactScreen.hpp"
#include "ui/Screens.hpp"
#include "ui/UiSound.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

// Button action codes.
constexpr int kActBall = 100;     // + ball index: set the target ball
constexpr int kActMisc = 200;     // + one of the Misc entries
constexpr int kActSpawn = 300;    // + EnemyKind
constexpr int kActSpeed = 400;    // + index into kSpeeds
constexpr int kActOpen = 500;     // + App::DevOpen
constexpr int kActPact = 600;     // + PactId: grant / drop that pact

enum Misc { WinWave, KillAll, Heal, Invuln, Gold, AddBall, ClearBall };
constexpr float kSpeeds[] = {0.25f, 0.5f, 1.f, 2.f, 4.f};

struct SpawnDef { EnemyKind kind; int count; const char* label; };
constexpr SpawnDef kSpawns[] = {
    {EnemyKind::Grunt, 5, "5 Grunts"},     {EnemyKind::Runner, 5, "5 Runners"},
    {EnemyKind::Tank, 2, "2 Tanks"},       {EnemyKind::Splitter, 3, "3 Splitters"},
    {EnemyKind::Shielded, 3, "3 Shielded"},
};

struct OpenDef { App::DevOpen what; const char* label; };
constexpr OpenDef kOpens[] = {
    {App::DevOpen::Shop, "Shop"},       {App::DevOpen::Forge, "Forge"},
    {App::DevOpen::Upgrade, "Pick (normal)"}, {App::DevOpen::Elite, "Pick (elite)"},
    {App::DevOpen::BossTreasure, "Boss treasure"}, {App::DevOpen::Recruit, "Recruit"},
    {App::DevOpen::JumpToBoss, "Jump to boss"},
    {App::DevOpen::PactBoss, "Pact choice (boss)"}, {App::DevOpen::PactStart, "Pact choice (start)"},
    {App::DevOpen::AbilityPick, "Ability pick (start)"},
};
constexpr int kOpenCount = static_cast<int>(sizeof(kOpens) / sizeof(kOpens[0]));

bool isKey(const sf::Event& e, sf::Keyboard::Key k) {
    return e.type == sf::Event::KeyPressed && e.key.code == k;
}

}  // namespace

void DevScreen::rebuild(App& app) {
    buttons_.clear();
    heads_.clear();
    const sf::Vector2f s = app.size();

    // ---- target ball
    const int nb = app.runBallCount();
    for (int b = 0; b < nb; ++b) {
        Button bt;
        bt.rect = {150.f + static_cast<float>(b) * 46.f, 58.f, 40.f, 26.f};
        bt.label = std::to_string(b + 1);
        bt.color = theme::accent;
        bt.action = kActBall + b;
        bt.on = app.devBall() == b;
        bt.tipTitle = "Target ball " + std::to_string(b + 1);
        bt.tipDesc = "picks you grant go on this ball";
        buttons_.push_back(bt);
    }

    // ---- every pick, in five columns: balls + elements + abilities | items (x3) | modifiers + relics
    std::vector<UpgradeKind> items;
    std::vector<UpgradeKind> cols[5];
    for (int i = 0; i < kUpgradeKindCount; ++i) {
        const auto k = static_cast<UpgradeKind>(i);
        switch (upgradeCat(k)) {
            case UpgradeCat::NewBall:
            case UpgradeCat::Element:
            case UpgradeCat::Ability:  cols[0].push_back(k); break;
            case UpgradeCat::Item:     items.push_back(k); break;
            case UpgradeCat::Modifier:
            case UpgradeCat::Relic:    cols[4].push_back(k); break;
        }
    }
    const std::size_t perCol = (items.size() + 2) / 3;
    for (std::size_t i = 0; i < items.size(); ++i) cols[1 + std::min<std::size_t>(2, i / std::max<std::size_t>(1, perCol))].push_back(items[i]);
    float col0End = 112.f;
    for (int c = 0; c < 5; ++c) {
        float y = 112.f;
        for (UpgradeKind k : cols[c]) {
            Button bt;
            bt.rect = {24.f + static_cast<float>(c) * 172.f, y, 164.f, 22.f};
            bt.label = upgradeInfo(k).title;
            bt.color = tierColor(upgradeTier(k));
            bt.action = static_cast<int>(k);
            bt.tipTitle = std::string(upgradeInfo(k).title) + "  -  " + tierName(upgradeTier(k));
            bt.tipDesc = std::string(upgradeCatName(upgradeCat(k))) + ": " + upgradeInfo(k).desc;
            buttons_.push_back(bt);
            y += 25.f;
        }
        if (c == 0) col0End = y;
    }

    // ---- pacts, under the first column: click to grant (or drop) one
    {
        float y = col0End + 34.f;
        heads_.push_back({{26.f, y - 20.f}, "PACTS (click: grant / drop)"});
        for (int i = 0; i < kPactCount; ++i) {
            const auto id = static_cast<PactId>(i);
            const PactDef& d = pactDef(id);
            Button bt;
            bt.rect = {24.f, y, 164.f, 22.f};
            bt.label = d.name;
            bt.color = pactColor(d.archetype);
            bt.action = kActPact + i;
            bt.on = app.hasPact(id);
            bt.tipTitle = std::string(d.name) + "  -  " + pactArchetypeName(d.archetype);
            bt.tipDesc = std::string("+ ") + d.gain + ".  - " + d.cost + ".";
            buttons_.push_back(bt);
            y += 25.f;
        }
    }

    // ---- right column: actions, spawns, speed, screens
    const float rx = s.x - 330.f;
    float ry = 112.f;
    auto add = [&](const std::string& label, int action, sf::Color col, bool on, const std::string& tip,
                   float x, float wd) {
        Button bt;
        bt.rect = {x, ry, wd, 24.f};
        bt.label = label;
        bt.color = col;
        bt.action = action;
        bt.on = on;
        bt.tipTitle = label;
        bt.tipDesc = tip;
        buttons_.push_back(bt);
    };
    auto head = [&](const char* t) { heads_.push_back({{rx, ry - 20.f}, t}); };
    head("ACTIONS");
    const bool inv = app.world().devInvuln();
    const std::pair<const char*, const char*> misc[] = {
        {"Win wave", "clear the wave now"}, {"Kill all", "everything on the field dies (counts as kills)"},
        {"Heal core", "core back to full"}, {inv ? "Invulnerable: ON" : "Invulnerable: off", "the core takes no damage"},
        {"+100 gold", "for testing the shop"}, {"Add ball", "one more plain ball"},
        {"Clear target ball", "strip the target ball back to nothing"},
    };
    for (int i = 0; i < 7; ++i) {
        add(misc[i].first, kActMisc + i, i == Invuln && inv ? theme::core : theme::textLo, i == Invuln && inv,
            misc[i].second, rx + (i % 2) * 158.f, 150.f);
        if (i % 2 == 1) ry += 30.f;
    }
    ry += 44.f;
    head("SPAWN");
    for (int i = 0; i < 5; ++i) {
        add(kSpawns[i].label, kActSpawn + static_cast<int>(kSpawns[i].kind), theme::enemy, false,
            enemyDesc(kSpawns[i].kind), rx + (i % 2) * 158.f, 150.f);
        if (i % 2 == 1) ry += 30.f;
    }
    ry += 44.f;
    head("SPEED");
    for (int i = 0; i < 5; ++i) {
        char lbl[16];
        std::snprintf(lbl, sizeof(lbl), "%gx", kSpeeds[i]);
        Button bt;
        bt.rect = {rx + static_cast<float>(i) * 62.f, ry, 56.f, 24.f};
        bt.label = lbl;
        bt.color = theme::puSurge;
        bt.action = kActSpeed + i;
        bt.on = std::abs(app.devTimeScale() - kSpeeds[i]) < 0.01f;
        bt.tipTitle = "Sim speed";
        bt.tipDesc = "slow it down to watch a combo, speed it up to get through waves";
        buttons_.push_back(bt);
    }
    ry += 64.f;
    head("OPEN");
    for (int i = 0; i < kOpenCount; ++i) {
        add(kOpens[i].label, kActOpen + static_cast<int>(kOpens[i].what), theme::puGolden, false,
            "open it now (the dev panel closes)", rx + (i % 2) * 158.f, 150.f);
        if (i % 2 == 1) ry += 30.f;
    }
}

void DevScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (isKey(e, sf::Keyboard::Escape) || isKey(e, sf::Keyboard::F1)) { app.back(); return; }
    if (e.type == sf::Event::KeyPressed && e.key.code >= sf::Keyboard::Num1 &&
        e.key.code < sf::Keyboard::Num1 + app.runBallCount()) {
        app.devSetBall(e.key.code - sf::Keyboard::Num1);
        return;
    }
    if (e.type != sf::Event::MouseButtonPressed || e.mouseButton.button != sf::Mouse::Left) return;
    for (const Button& b : buttons_) {
        if (!b.rect.contains(mouse)) continue;
        const int a = b.action;
        if (a < kUpgradeKindCount) {
            app.devGrant(static_cast<UpgradeKind>(a));
        } else if (a >= kActPact) {
            app.devTogglePact(static_cast<PactId>(a - kActPact));
        } else if (a >= kActOpen) {
            app.devOpen(static_cast<App::DevOpen>(a - kActOpen));
            return;   // this screen is gone
        } else if (a >= kActSpeed) {
            app.devSetTimeScale(kSpeeds[a - kActSpeed]);
        } else if (a >= kActSpawn) {
            for (const SpawnDef& sd : kSpawns)
                if (static_cast<int>(sd.kind) == a - kActSpawn) app.devSpawn(sd.kind, sd.count);
        } else if (a >= kActMisc) {
            switch (a - kActMisc) {
                case WinWave:   app.devWinWave(); break;
                case KillAll:   app.devKillAll(); break;
                case Heal:      app.devHealCore(); break;
                case Invuln:    app.devToggleInvuln(); break;
                case Gold:      app.devGold(100); break;
                case AddBall:   app.devAddBall(); break;
                case ClearBall: app.devClearBall(); break;
                default: break;
            }
        } else if (a >= kActBall) {
            app.devSetBall(a - kActBall);
        }
        return;
    }
}

void DevScreen::update(App& app, float, sf::Vector2f mouse) {
    mouse_ = mouse;
    rebuild(app);   // labels follow state (speed, invuln, target ball, ball count)
    hover_ = -1;
    for (std::size_t i = 0; i < buttons_.size(); ++i)
        if (buttons_[i].rect.contains(mouse)) hover_ = static_cast<int>(i);
    uisound::hover(this, hover_);
}

void DevScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    drawDim(w, s, 0.8f);
    const sf::Font& f = app.font();

    drawLabel(w, f, "dev panel", 18, {24.f, 32.f}, theme::coreLow, -1);
    draw::line(w, {24.f, 48.f}, {s.x - 24.f, 48.f}, 1.f, withAlpha(theme::arenaEdge, 0.9f));
    drawLabel(w, f, "target ball", 10, {24.f, 71.f}, theme::textLo, -1);

    // What the target ball carries right now.
    if (app.runBallCount() > 0) {
        const BallLoadout& L = app.data().run.balls[static_cast<std::size_t>(app.devBall())];
        ItemTag roles[2];
        const int nr = L.roles(roles);
        std::string line = nr == 0 ? "Normal" : "";
        for (int i = 0; i < nr; ++i)
            line += std::string(i ? " / " : "") +
                    (roles[i] == L.ascended() ? ascendedName(tagRole(roles[i])) : roleName(tagRole(roles[i])));
        line += ":";
        for (int sl = 0; sl < kLoadoutSlots; ++sl)
            if (L.kindAt(sl) >= 0) line += std::string("  ") + upgradeInfo(static_cast<UpgradeKind>(L.kindAt(sl))).title;
        const std::string mods = modifierLine(L);
        if (!mods.empty()) line += "   |  " + mods;
        sf::Text lt = makeText(f, line, theme::fsSmall, theme::textHi);
        lt.setPosition(150.f + static_cast<float>(app.runBallCount()) * 46.f + 12.f, 63.f);
        w.draw(lt);
    }

    const char* heads[] = {"BALL / TYPE / ABILITY", "ITEMS", "", "", "MODIFIERS / RELICS"};
    for (int c = 0; c < 5; ++c) {
        sf::Text h = makeLabel(f, heads[c], 10, theme::textDim);
        h.setPosition(26.f + static_cast<float>(c) * 172.f, 96.f);
        w.draw(h);
    }
    for (const auto& [pos, txt] : heads_) {
        sf::Text h = makeLabel(f, txt, 10, theme::textDim);
        h.setPosition(pos + sf::Vector2f{0.f, 4.f});
        w.draw(h);
    }

    for (std::size_t i = 0; i < buttons_.size(); ++i) {
        const Button& b = buttons_[i];
        const bool hot = static_cast<int>(i) == hover_;
        // Glass keys with a colour tick on the left; lit when on, bracketed under the pointer.
        draw::box(w, b.rect, theme::corner,
                  withAlpha(lerpColor(theme::glassTop, b.color, b.on ? 0.4f : (hot ? 0.22f : 0.06f)), 0.95f),
                  withAlpha(lerpColor(theme::glassBottom, b.color, b.on ? 0.25f : 0.03f), 0.95f),
                  withAlpha(b.color, b.on || hot ? 0.7f : 0.18f), 1.f);
        draw::box(w, {b.rect.left, b.rect.top, 2.f, b.rect.height}, 0.f, withAlpha(b.color, 0.8f), withAlpha(b.color, 0.8f));
        if (hot) draw::brackets(w, b.rect, 5.f, 1.5f, b.color);
        sf::Text t = makeText(f, b.label, theme::fsSmall, b.on ? theme::textHi : lerpColor(b.color, theme::textHi, 0.35f));
        const sf::FloatRect tb = t.getLocalBounds();
        t.setOrigin(tb.left + tb.width * 0.5f, tb.top + tb.height * 0.5f);
        t.setPosition(std::round(b.rect.left + b.rect.width * 0.5f), std::round(b.rect.top + b.rect.height * 0.5f));
        w.draw(t);
    }

    sf::Text foot = makeText(f, "F1 / Esc: close  -  1-5: target ball  -  the fight is paused while this is open",
                             theme::fsSmall, theme::textDim);
    foot.setPosition(24.f, s.y - 34.f);
    w.draw(foot);

    if (hover_ >= 0) {
        const Button& b = buttons_[static_cast<std::size_t>(hover_)];
        drawTooltip(w, f, mouse_, s, b.tipTitle, b.tipDesc, b.color);
    }
}

}  // namespace sb
