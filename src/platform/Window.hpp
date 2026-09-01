#pragma once

#include <SFML/Graphics.hpp>

namespace sb {

// Owns the render window and a letterboxed view. The game renders to a fixed
// logical size; this class maps that onto whatever real pixels the window has,
// windowed or fullscreen, and converts mouse coordinates back.
class Window {
public:
    explicit Window(sf::Vector2f logicalSize);

    void applyVideoMode(bool fullscreen);          // (re)creates the window
    void applyLetterbox(unsigned pixelW, unsigned pixelH);

    sf::RenderWindow& handle() { return win_; }
    bool isOpen() const { return win_.isOpen(); }
    void close() { win_.close(); }
    void display() { win_.display(); }
    void clear(sf::Color c) { win_.clear(c); }

    sf::Vector2f logicalSize() const { return logical_; }
    sf::Vector2f mousePosition() const;             // in logical coordinates

private:
    sf::RenderWindow win_;
    sf::View view_;
    sf::Vector2f logical_;
};

}  // namespace sb
