#pragma once

#include <deque>
#include <utility>

#include "render/WorldRenderer.hpp"
#include "ui/Menu.hpp"
#include "ui/Screen.hpp"
#include "ui/Shop.hpp"

namespace sb {

class MainMenuScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;

private:
    Menu menu_;
};

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

// Overlay: the game keeps simulating and rendering underneath.
class ShopScreen : public Screen {
public:
    void onEnter(App& app) override;
    void handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) override;
    void update(App& app, float dt, sf::Vector2f mouse) override;
    void draw(App& app, sf::RenderWindow& w) override;
    bool opaque() const override { return false; }
    bool simulates() const override { return true; }

private:
    Shop shop_;
};

// Overlay: frozen game underneath.
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
