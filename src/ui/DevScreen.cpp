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
#include "ui/CreedScreen.hpp"
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
constexpr int kActCreed = 600;     // + CreedId: grant / drop that creed
constexpr int kActPact = 700;      // + PactId: grant / drop that pact

enum Misc { WinWave, KillAll, Heal, Invuln, Gold, AddBall, ClearBall, Currency };
constexpr float kSpeeds[] = {0.25f, 0.5f, 1.f, 2.f, 4.f};

struct SpawnDef { EnemyKind kind; int count; const char* label; };
constexpr SpawnDef kSpawns[] = {
    {EnemyKind::Grunt, 5, "5 Grunts"},     {EnemyKind::Runner, 5, "5 Runners"},
    {EnemyKind::Tank, 2, "2 Tanks"},       {EnemyKind::Splitter, 3, "3 Splitters"},
    {EnemyKind::Shielded, 3, "3 Shielded"}, {EnemyKind::Blinker, 3, "3 Blinkers"},
    {EnemyKind::Mender, 2, "2 Menders"},   {EnemyKind::Brute, 1, "1 Brute"},
};
constexpr int kSpawnCount = static_cast<int>(sizeof(kSpawns) / sizeof(kSpawns[0]));

struct OpenDef { App::DevOpen what; const char* label; };
constexpr OpenDef kOpens[] = {
    {App::DevOpen::Shop, "Shop"},       {App::DevOpen::Forge, "Forge"},
    {App::DevOpen::PostFight, "Pick: after fight"}, {App::DevOpen::Elite, "Pick: elite (items)"},
    {App::DevOpen::Upgrade, "Pick: Upgrade node"}, {App::DevOpen::Recruit, "Recruit"},
    {App::DevOpen::BossTreasure, "Boss treasure"}, {App::DevOpen::AbilityPick, "First ability pick"},
    {App::DevOpen::CreedBoss, "Creed choice (boss)"}, {App::DevOpen::CreedStart, "Creed choice (start)"},
    {App::DevOpen::Altar, "Altar (pacts)"},    {App::DevOpen::JumpToBoss, "Jump to boss"},
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
    // Items go by class: a coloured class heading, then its items in the class
    // colour (the tier is in the tooltip). A class stays in one column; the
    // next column starts when this one is past its share.
    std::stable_sort(items.begin(), items.end(), [](UpgradeKind a, UpgradeKind b) {
        return static_cast<int>(itemTag(a)) < static_cast<int>(itemTag(b));
    });
    constexpr float kRowH = 25.f, kClassHeadH = 18.f;
    float itemsH = 0.f;
    for (std::size_t i = 0; i < items.size(); ++i)
        itemsH += kRowH + (i == 0 || itemTag(items[i]) != itemTag(items[i - 1]) ? kClassHeadH : 0.f);
    const float colShare = itemsH / 3.f;
    auto pickButton = [&](UpgradeKind k, float x, float y, sf::Color col) {
        Button bt;
        bt.rect = {x, y, 164.f, 22.f};
        bt.label = upgradeInfo(k).title;
        bt.color = col;
        bt.action = static_cast<int>(k);
        const bool item = upgradeCat(k) == UpgradeCat::Item;
        bt.tipTitle = std::string(upgradeInfo(k).title) + "  -  " + tierName(upgradeTier(k)) +
                      (item ? std::string("  -  ") + itemTagName(itemTag(k)) : std::string());
        bt.tipDesc = std::string(upgradeCatName(upgradeCat(k))) + ": " + upgradeInfo(k).desc;
        buttons_.push_back(bt);
    };
    {
        int c = 1;
        float y = 112.f;
        for (std::size_t i = 0; i < items.size(); ++i) {
            const ItemTag tag = itemTag(items[i]);
            if (i == 0 || tag != itemTag(items[i - 1])) {
                if (i > 0 && y - 112.f > colShare && c < 3) { ++c; y = 112.f; }
                const std::string name = itemTagName(tag);
                std::string up = name.empty() ? "CLASSLESS" : name;
                for (char& ch : up) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                y += kClassHeadH;
                heads_.push_back({{26.f + static_cast<float>(c) * 172.f, y - 18.f}, up, tagColor(tag)});
            }
            pickButton(items[i], 24.f + static_cast<float>(c) * 172.f, y, tagColor(tag));
            y += kRowH;
        }
    }
    float col0End = 112.f, col3End = 112.f, col4End = 112.f;
    for (int c : {0, 4}) {
        float y = 112.f;
        for (UpgradeKind k : cols[c]) {
            pickButton(k, 24.f + static_cast<float>(c) * 172.f, y, tierColor(upgradeTier(k)));
            y += kRowH;
        }
        if (c == 0) col0End = y;
        if (c == 4) col4End = y;
    }
    for (const Button& b : buttons_)   // where the last item column ends (the creeds go under it)
        if (b.rect.left > 24.f + 2.5f * 172.f && b.rect.left < 24.f + 3.5f * 172.f)
            col3End = std::max(col3End, b.rect.top + kRowH);

    // ---- creeds, two columns under the last item column and the modifiers /
    // relics one: click to grant (or drop) one
    (void)col0End;
    {
        const float px0 = 24.f + 3.f * 172.f;
        const float y0 = std::max(col3End, col4End) + 34.f;
        heads_.push_back({{px0 + 2.f, y0 - 20.f}, "CREEDS (click: grant / drop)", theme::textDim});
        const int rows = (kCreedCount + 1) / 2;
        for (int i = 0; i < kCreedCount; ++i) {
            const auto id = static_cast<CreedId>(i);
            const CreedDef& d = creedDef(id);
            const float y = y0 + static_cast<float>(i % rows) * 24.f;
            Button bt;
            bt.rect = {px0 + static_cast<float>(i / rows) * 172.f, y, 164.f, 21.f};
            bt.label = d.name;
            bt.color = creedColor(d.archetype);
            bt.action = kActCreed + i;
            bt.on = app.hasCreed(id);
            bt.tipTitle = std::string(d.name) + "  -  " + creedArchetypeName(d.archetype);
            bt.tipDesc = std::string("+ ") + d.gain + ".  - " + d.cost + ".";
            buttons_.push_back(bt);
        }
    }

    // ---- pacts, three columns under the first three item columns
    {
        float yEnd = 0.f;
        for (const Button& b : buttons_)
            if (b.rect.left < 24.f + 2.5f * 172.f) yEnd = std::max(yEnd, b.rect.top + b.rect.height);
        const float y0 = yEnd + 34.f;
        heads_.push_back({{26.f, y0 - 20.f}, "PACTS (click: grant / drop)", theme::textDim});
        const int rows = (kPactCount + 2) / 3;
        for (int i = 0; i < kPactCount; ++i) {
            const auto id = static_cast<PactId>(i);
            const PactDef& d = pactDef(id);
            Button bt;
            bt.rect = {24.f + static_cast<float>(i / rows) * 172.f, y0 + static_cast<float>(i % rows) * 24.f, 164.f, 21.f};
            bt.label = d.name;
            bt.color = theme::pact;
            bt.action = kActPact + i;
            bt.on = app.hasPact(id);
            bt.tipTitle = std::string("Pact: ") + d.name;
            bt.tipDesc = std::string("+ ") + d.gain + ".  - " + d.cost + ".";
            buttons_.push_back(bt);
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
    auto head = [&](const char* t) { heads_.push_back({{rx, ry - 20.f}, t, theme::textDim}); };
    head("ACTIONS");
    const bool inv = app.world().devInvuln();
    const std::pair<const char*, const char*> misc[] = {
        {"Win wave", "clear the wave now"}, {"Kill all", "everything on the field dies (counts as kills)"},
        {"Heal core", "core back to full"}, {inv ? "Invulnerable: ON" : "Invulnerable: off", "the core takes no damage"},
        {"+100 gold", "for testing the shop"}, {"Add ball", "one more plain ball"},
        {"Clear target ball", "strip the target ball back to nothing"},
        {"Cores & prisms", "a huge pile of both, for testing the skill web"},
    };
    for (int i = 0; i < 8; ++i) {
        add(misc[i].first, kActMisc + i, i == Invuln && inv ? theme::core : theme::textLo, i == Invuln && inv,
            misc[i].second, rx + (i % 2) * 158.f, 150.f);
        if (i % 2 == 1) ry += 30.f;
    }
    ry += 44.f;
    head("SPAWN");
    for (int i = 0; i < kSpawnCount; ++i) {
        add(kSpawns[i].label, kActSpawn + static_cast<int>(kSpawns[i].kind), theme::enemy, false,
            enemyDesc(kSpawns[i].kind), rx + (i % 2) * 158.f, 150.f);
        if (i % 2 == 1 && i + 1 < kSpawnCount) ry += 30.f;
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
        } else if (a >= kActCreed) {
            app.devToggleCreed(static_cast<CreedId>(a - kActCreed));
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
                case Currency:  app.devGrantCurrency(); break;
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
    for (const Head& hd : heads_) {
        sf::Text h = makeLabel(f, hd.text, 10, hd.color);
        h.setPosition(hd.pos + sf::Vector2f{0.f, 4.f});
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
