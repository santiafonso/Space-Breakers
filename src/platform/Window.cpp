#include "platform/Window.hpp"

#include <algorithm>

namespace sb {

Window::Window(sf::Vector2f logicalSize) : logical_(logicalSize) {}

void Window::applyVideoMode(bool fullscreen) {
    if (fullscreen) {
        win_.create(sf::VideoMode::getDesktopMode(), "Space-Breakers", sf::Style::Fullscreen);
    } else {
        const sf::VideoMode d = sf::VideoMode::getDesktopMode();
        const unsigned w = std::min<unsigned>(static_cast<unsigned>(logical_.x), d.width);
        const unsigned h = std::min<unsigned>(static_cast<unsigned>(logical_.y),
                                              d.height > 70 ? d.height - 70 : d.height);
        win_.create(sf::VideoMode(w, h), "Space-Breakers",
                    sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
    }
    win_.setFramerateLimit(60);
    win_.setKeyRepeatEnabled(false);
    applyLetterbox(win_.getSize().x, win_.getSize().y);
}

void Window::applyLetterbox(unsigned pixelW, unsigned pixelH) {
    view_.setSize(logical_.x, logical_.y);
    view_.setCenter(logical_.x * 0.5f, logical_.y * 0.5f);

    const float windowRatio = static_cast<float>(pixelW) / std::max(1u, pixelH);
    const float viewRatio = logical_.x / logical_.y;
    float sx = 1.f, sy = 1.f, ox = 0.f, oy = 0.f;
    if (windowRatio > viewRatio) {
        sx = viewRatio / windowRatio;
        ox = (1.f - sx) * 0.5f;
    } else {
        sy = windowRatio / viewRatio;
        oy = (1.f - sy) * 0.5f;
    }
    view_.setViewport({ox, oy, sx, sy});
    win_.setView(view_);
}

sf::Vector2f Window::mousePosition() const {
    return win_.mapPixelToCoords(sf::Mouse::getPosition(win_), view_);
}

}  // namespace sb
