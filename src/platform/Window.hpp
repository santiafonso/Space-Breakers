#pragma once

#include <SFML/Graphics.hpp>

namespace sb {

// Owns the render window and two letterboxed views that share the same on-screen
// rectangle: a fixed UI view (menus, HUD) and a world view whose framed area can
// change - it widens for the boss wave so more of the arena is visible.
class Window {
public:
    explicit Window(sf::Vector2f logicalSize);

    void applyVideoMode(bool fullscreen);          // (re)creates the window
    void applyLetterbox(unsigned pixelW, unsigned pixelH);

    // Set what the world view frames (size + centre, in world units).
    void setWorldView(sf::Vector2f size, sf::Vector2f center);
    void useWorldView() { win_.setView(worldView_); }
    void useUiView() { win_.setView(uiView_); }

    sf::RenderWindow& handle() { return win_; }
    bool isOpen() const { return win_.isOpen(); }
    void close() { win_.close(); }
    void display() { win_.display(); }
    void clear(sf::Color c) { win_.clear(c); }

    sf::Vector2f logicalSize() const { return logical_; }
    sf::Vector2f mousePosition() const;             // in world coordinates

private:
    void rebuildViews(unsigned pixelW, unsigned pixelH);

    sf::RenderWindow win_;
    sf::View uiView_;
    sf::View worldView_;
    sf::Vector2f logical_;
    sf::Vector2f worldSize_;
    sf::Vector2f worldCenter_;
    unsigned pixelW_ = 1;
    unsigned pixelH_ = 1;
};

}  // namespace sb
