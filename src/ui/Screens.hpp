#pragma once

#include <deque>
#include <utility>

#include "progression/Offers.hpp"
#include "render/WorldRenderer.hpp"
#include "ui/Menu.hpp"
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
};

// The game menu: spend cores on permanent unlocks, then start a run.
class LoadoutScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;

private:
    void rebuild(App& app);
    int unlockRowAt(App& app, sf::Vector2f mouse) const;  // 0..count-1, -1 none
    sf::Vector2f unlockRowCenter(App& app, int i) const;

    Menu menu_;
    float hover_[MetaUnlockCount] = {};
};

// Combat. One or more balls bounce freely; you fling them into the enemies.
class PlayScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool simulates() const override { return true; }

private:
    void grab(App& app, sf::Vector2f mouse);
    void release(App& app);
    sf::Vector2f pointerVelocity() const;
    void drawPicks(App& app, sf::RenderWindow& w) const;
    void drawDevKeys(App& app, sf::RenderWindow& w) const;

    WorldRenderer renderer_;
    bool dragging_ = false;
    bool showPicks_ = false;   // Tab held: list upgrades taken this run
    float clock_ = 0.f;
    std::deque<std::pair<float, sf::Vector2f>> samples_;
};

// Overlay after a wave: pick 1 of 4 rolled upgrades.
class ChoiceScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    int cardAt(App& app, sf::Vector2f mouse) const;  // 0..3, -1 none
    float hover_[4] = {};
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

}  // namespace sb
