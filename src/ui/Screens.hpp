#pragma once

#include <deque>
#include <string>
#include <vector>
#include <utility>

#include "progression/Offers.hpp"
#include "render/WorldRenderer.hpp"
#include "ui/Menu.hpp"
#include "ui/MenuBackdrop.hpp"
#include "ui/Screen.hpp"

namespace sb {

// Main menu: start playing, stats, how-to, quit.
class MenuScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;

private:
    void rebuild(App& app);
    Menu menu_;
    MenuBackdrop backdrop_;   // balls drifting behind the menu
    float resetArm_ = 0.f;   // >0 while "Reset progress" waits for a confirming click
};

// The game menu: a radial skill web. Spend cores (and prisms on a few key
// nodes) to unlock permanent buffs, then start a run. The centre node is "+1
// starting ball"; branches fan out - Base left, Ball combat down, Economy up,
// Special balls right - and each node stays locked until the node that gates
// it toward the centre has a level.
class LoadoutScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;

private:
    void rebuild(App& app);
    sf::Vector2f nodePos(App& app, int i) const;
    int nodeAt(App& app, sf::Vector2f mouse) const;     // 0..count-1, -1 none
    void moveSelection(App& app, int dx, int dy);       // arrow-key navigation
    void drawInfoCard(App& app, sf::RenderWindow& w, int node) const;
    sf::Vector2f webCentre(App& app) const;             // the root node, after panning
    void zoomAt(App& app, sf::Vector2f mouse, float factor);
    int legendAt(App& app, sf::Vector2f mouse) const;   // branch legend row under the pointer, -1 none

    Menu menu_;
    // Pan / zoom: the web grows past one screen, so it can be dragged around
    // and zoomed with the wheel. Labels keep their size; only the layout scales.
    float zoom_ = 1.f;
    sf::Vector2f pan_{0.f, 0.f};
    bool panning_ = false;
    sf::Vector2f panStart_{0.f, 0.f};
    sf::Vector2f panFrom_{0.f, 0.f};
    int legendHover_ = -1;                // MetaBranch under the pointer in the legend: light that branch
    int hoverNode_ = -1;
    int selNode_ = 0;
    bool selUsed_ = false;                // hovered a node or used the arrows at least once
    bool keyNav_ = false;                 // arrows in use - light selNode_ until the mouse moves
    sf::Vector2f lastMouse_{-1.f, -1.f};
    float glow_[MetaUnlockCount] = {};    // 0 = idle, 1 = lit; only the active node rises
};

// The peek itself: every ball's loadout, the relics and the pacts, dimming
// whatever is underneath. `paused` adds the fight's "paused" note.
void drawLoadoutOverlay(App& app, sf::RenderWindow& w, bool paused, bool latched);
// A small [tab] key cap with "loadout" beside it; `topLeft` in UI units.
void drawTabHint(App& app, sf::RenderWindow& w, sf::Vector2f topLeft);

// Combat. One or more balls bounce freely; you fling them into the enemies.
// The fight holds still while the TAB peek is open.
class PlayScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool simulates() const override { return true; }
    bool frozen() const override { return peek_.open; }
    bool ownsTab() const override { return true; }

private:
    void grab(App& app, sf::Vector2f mouse);
    void release(App& app);
    void commitAim(App& app);   // the pointer moved while pressed: aim with the slingshot
    void dropHeld(App& app);   // let go of a held ball without throwing it

    void drawWaveBanner(App& app, sf::RenderWindow& w) const;

    WorldRenderer renderer_;
    bool dragging_ = false;
    TabPeek peek_;             // TAB: the balls' loadouts + relics + pacts
    sf::Vector2f worldMouse_;  // pointer in arena units (enemy hover help)
    sf::Vector2f anchor_;      // slingshot: where the held ball sits
    bool aimCommitted_ = false; // the pointer moved off the ball: aiming by hand
    sf::Vector2f pressPos_;     // pointer (arena units) when the ball was grabbed
    void drawAim(App& app, sf::RenderWindow& w) const;
    float clock_ = 0.f;

    float sceneIn_ = 999.f;    // counts up from 0 on run start - fade the scene up from black
    int bannerWave_ = 0;       // act * 100 + map row the "Stage N" banner is showing
    float bannerT_ = 999.f;    // time since the banner started (large = inactive)
};

// Overlay after an Elite fight or on an Upgrade node: pick 1 of 4 (or skip to
// repair the core). Picks that go on a ball open the Equip picker.
class ChoiceScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    int cardAt(App& app, sf::Vector2f mouse) const;  // 0..3, -1 none
    sf::FloatRect healRect(sf::Vector2f size) const;  // "repair core" button, when the core isn't full
    sf::FloatRect rerollRect(sf::Vector2f size, int i) const;  // "reroll" strip under card i
    bool coreHurt(App& app) const;

    float hover_[4] = {};
    float rerollHover_[4] = {};
    float healHover_ = 0.f;
    sf::Vector2f mouse_;
};

// Pick which ball (and which of its 4 slots) a role / element / item /
// modifier goes on - from a Choice card or a shop buy. At a Forge node it
// picks the item to level up instead.
class EquipScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;

private:
    void targetAt(App& app, sf::Vector2f mouse, int& ball, int& slot) const;
    int hoverBall_ = -1;
    int hoverSlot_ = -1;
    sf::Vector2f mouse_;
};

// The act's path map: rows of nodes from the bottom up, the boss at the top. Click
// a lit node (one linked from where you stand) to go there.
class MapScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool ownsTab() const override { return true; }

private:
    sf::Vector2f nodePos(App& app, int node) const;
    int nodeAt(App& app, sf::Vector2f mouse, bool openOnly = true) const;
    int hover_ = -1;          // open node under the pointer
    int info_ = -1;           // any node under the pointer (for the tooltip)
    float clock_ = 0.f;
    sf::Vector2f mouse_;
    TabPeek peek_;            // TAB: the same loadout overlay as in a fight
};

// The F1 dev panel (SB_DEV): grant any pick to a chosen ball, spawn enemy
// kinds, change the sim speed, open any between-wave screen. The fight is
// paused underneath while it's open.
class DevScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    struct Button {
        sf::FloatRect rect;
        std::string label;
        sf::Color color;
        int action = 0;            // index into the actions table (see DevScreen.cpp)
        std::string tipTitle, tipDesc;
        bool on = false;           // highlighted (current speed / target ball / invuln)
    };
    void rebuild(App& app);
    std::vector<Button> buttons_;
    std::vector<std::pair<sf::Vector2f, std::string>> heads_;   // section titles, laid out with the buttons
    int hover_ = -1;
    sf::Vector2f mouse_;
};

// A Shop node: spend gold on a few rolled picks (one on sale), a mystery box,
// core repairs, a paid forge, selling an item back, or a reroll of the stock.
class ShopScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;

private:
    int cardCount(App& app) const;                 // offers + the mystery box while it's there
    sf::FloatRect offerRect(App& app, int i) const;
    sf::FloatRect buttonRect(App& app, int b) const;   // 0 repair, 1 forge, 2 sell, 3 reroll, 4 leave
    int hover_ = -1;          // card index, 100 + b = a button
    float clock_ = 0.f;
    sf::Vector2f mouse_;
};

class PauseScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    void rebuild(App& app);
    Menu menu_;
    bool lastSound_ = true;
    sf::Vector2f mouse_;
};

class StatsScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
};

class HowToScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
};

// Shown when the miniboss dies, and again when wave 20 is cleared. "Continue"
// only appears once a run has been won before (App::bossWinCanContinue) and
// resumes the run at wave 11; otherwise the only option is "Back to menu".
// Opaque so the camera / mouse mapping is the plain UI one, not the wide framing.
class BossWinScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;

private:
    void rebuild(App& app);
    Menu menu_;
};

}  // namespace sb
