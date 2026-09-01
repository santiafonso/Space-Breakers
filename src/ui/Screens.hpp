#pragma once

#include <deque>
#include <utility>

#include "render/WorldRenderer.hpp"
#include "ui/Menu.hpp"
#include "ui/Screen.hpp"

namespace sb {

// Between-run home: meta currency, permanent unlocks, start / continue a run.
class HubScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;

private:
    void rebuild(App& app);
    Menu menu_;
};

// Combat. Balls clear the wave; you drag field structures to bend their paths.
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

    WorldRenderer renderer_;
    bool dragging_ = false;
    float clock_ = 0.f;
    std::deque<std::pair<float, sf::Vector2f>> samples_;
};

// Overlay after a wave: pick 1 of 3 offers (or skip for scrap).
class ChoiceScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }

private:
    int cardAt(App& app, sf::Vector2f mouse) const;  // 0..3, -1 none
    bool skipAt(App& app, sf::Vector2f mouse) const;
    float hover_[4] = {};
    float skipHover_ = 0.f;
};

// Shown when the core falls: the run's result, then back to the hub.
class RunSummaryScreen : public Screen {
public:
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
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
