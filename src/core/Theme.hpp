#pragma once

#include "core/Math.hpp"

// The whole visual identity in one place: palette, type sizes and the ball
// speed-to-colour ramp. Nothing here knows about gameplay.
namespace sb::theme {

// Backdrop / surfaces
inline const sf::Color bg{13, 13, 19};
inline const sf::Color panel{0, 0, 0};  // paired with alpha
inline const sf::Color arenaEdge{54, 58, 78};

// Text
inline const sf::Color textHi{236, 238, 245};
inline const sf::Color textLo{136, 139, 156};
inline const sf::Color textDim{80, 83, 98};
inline const sf::Color accent{92, 200, 255};

// Ball speed ramp (calm -> hot)
inline const sf::Color ballSlow{84, 150, 235};
inline const sf::Color ballMid{90, 214, 160};
inline const sf::Color ballFast{240, 206, 96};
inline const sf::Color ballUltra{240, 96, 104};

// Combat
inline const sf::Color core{120, 230, 200};
inline const sf::Color coreLow{240, 110, 90};
inline const sf::Color enemy{232, 120, 120};

// Ball elements
inline const sf::Color elemFire{255, 148, 66};
inline const sf::Color elemWind{184, 240, 232};
inline const sf::Color elemWater{92, 152, 255};
inline const sf::Color elemStone{176, 156, 132};

// Power-ups
inline const sf::Color puPoints{245, 200, 90};
inline const sf::Color puSlow{130, 200, 255};
inline const sf::Color puSurge{198, 120, 255};
inline const sf::Color puGolden{255, 214, 120};
inline const sf::Color puGhost{206, 228, 244};
inline const sf::Color puFrenzy{255, 110, 150};

// Layout / type — kept compact so more can share the screen
inline constexpr float margin = 22.f;
inline constexpr unsigned fsTitle = 38;
inline constexpr unsigned fsHeading = 22;
inline constexpr unsigned fsItem = 24;
inline constexpr unsigned fsBody = 16;
inline constexpr unsigned fsHud = 24;
inline constexpr unsigned fsSmall = 13;

// Colour of the ball for a given speed relative to its (un-buffed) cruise speed.
inline sf::Color speedColor(float speed, float cruise) {
    const float ratio = cruise > 1.f ? speed / cruise : 1.f;
    if (ratio < 0.8f) return lerpColor(ballSlow, ballMid, ratio / 0.8f);
    if (ratio < 1.3f) return lerpColor(ballMid, ballFast, (ratio - 0.8f) / 0.5f);
    return lerpColor(ballFast, ballUltra, std::min(1.f, (ratio - 1.3f) / 1.2f));
}

}  // namespace sb::theme
