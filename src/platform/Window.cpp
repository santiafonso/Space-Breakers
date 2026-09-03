#include "platform/Window.hpp"

#include <algorithm>

namespace sb {

Window::Window(sf::Vector2f logicalSize)
    : logical_(logicalSize), worldSize_(logicalSize), worldCenter_(logicalSize * 0.5f) {}

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

// The letterbox rectangle is derived from the UI aspect and shared by both
// views, so switching between them never shifts the picture on screen.
void Window::rebuildViews(unsigned pixelW, unsigned pixelH) {
    pixelW_ = pixelW;
    pixelH_ = std::max(1u, pixelH);

    const float windowRatio = static_cast<float>(pixelW) / static_cast<float>(pixelH_);
    const float uiRatio = logical_.x / logical_.y;
    float sx = 1.f, sy = 1.f, ox = 0.f, oy = 0.f;
    if (windowRatio > uiRatio) {
        sx = uiRatio / windowRatio;
        ox = (1.f - sx) * 0.5f;
    } else {
        sy = windowRatio / uiRatio;
        oy = (1.f - sy) * 0.5f;
    }
    const sf::FloatRect viewport{ox, oy, sx, sy};

    uiView_.setSize(logical_.x, logical_.y);
    uiView_.setCenter(logical_ * 0.5f);
    uiView_.setViewport(viewport);

    // Fit the requested world area inside the same UI aspect (extra head-room
    // above/below rather than stretching).
    float w = std::max(worldSize_.x, worldSize_.y * uiRatio);
    float h = w / uiRatio;
    worldView_.setSize(w, h);
    worldView_.setCenter(worldCenter_);
    worldView_.setViewport(viewport);

    win_.setView(uiView_);
}

void Window::applyLetterbox(unsigned pixelW, unsigned pixelH) { rebuildViews(pixelW, pixelH); }

void Window::setWorldView(sf::Vector2f size, sf::Vector2f center) {
    worldSize_ = size;
    worldCenter_ = center;
    rebuildViews(pixelW_, pixelH_);
}

sf::Vector2f Window::mousePosition() const {
    return win_.mapPixelToCoords(sf::Mouse::getPosition(win_), worldView_);
}

}  // namespace sb
