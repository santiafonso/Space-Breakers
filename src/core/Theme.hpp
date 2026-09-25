#pragma once

#include "core/Math.hpp"

// The whole visual identity in one place: palette, type sizes and the ball
// speed-to-colour ramp. Nothing here knows about gameplay.
namespace sb::theme {

// Backdrop / surfaces
inline const sf::Color bg{13, 13, 19};
inline const sf::Color bgHot{58, 22, 38};   // the backdrop warms toward this as the combo climbs
inline const sf::Color panel{0, 0, 0};  // paired with alpha
inline const sf::Color arenaEdge{54, 58, 78};
inline constexpr float corner = 0.f;   // box corner radius: square, clean edges (the user's call)

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
inline const sf::Color elemPoison{150, 214, 96};
inline const sf::Color elemWater{92, 152, 255};
inline const sf::Color elemIce{170, 224, 240};
inline const sf::Color elemStone{176, 156, 132};
inline const sf::Color elemElectric{176, 116, 246};

// Power-ups
inline const sf::Color puPoints{245, 200, 90};
inline const sf::Color puSlow{130, 200, 255};
inline const sf::Color puSurge{198, 120, 255};
inline const sf::Color puGolden{255, 214, 120};
inline const sf::Color puOverdrive{255, 110, 150};

// Layout / type — kept compact so more can share the screen
inline constexpr float margin = 22.f;
inline constexpr unsigned fsTitle = 38;
inline constexpr unsigned fsHeading = 22;
inline constexpr unsigned fsItem = 24;
inline constexpr unsigned fsBody = 16;
inline constexpr unsigned fsHud = 24;
inline constexpr unsigned fsSmall = 13;

// Colour of a plain ball for a given speed relative to its (un-buffed) cruise
// speed: a neutral grey that just brightens as it speeds up, so elemental balls
// (which keep their hue) always stand out from the plain ones.
inline sf::Color speedColor(float speed, float cruise) {
    const float ratio = cruise > 1.f ? speed / cruise : 1.f;
    const sf::Color slow{104, 110, 124};
    const sf::Color mid{158, 164, 178};
    const sf::Color fast{212, 216, 226};
    const sf::Color hot{255, 255, 255};
    if (ratio < 0.8f) return lerpColor(slow, mid, ratio / 0.8f);
    if (ratio < 1.3f) return lerpColor(mid, fast, (ratio - 0.8f) / 0.5f);
    return lerpColor(fast, hot, std::min(1.f, (ratio - 1.3f) / 1.2f));
}

// Push a colour away from its own grey (raise saturation) and lift it a little,
// keeping the hue.
inline sf::Color vivify(sf::Color c, float sat, float lift) {
    const float avg = (c.r + c.g + c.b) / 3.f;
    auto ch = [&](float v) {
        return static_cast<std::uint8_t>(clampf(avg + (v - avg) * (1.f + sat) + lift, 0.f, 255.f));
    };
    return sf::Color(ch(c.r), ch(c.g), ch(c.b), c.a);
}

// An elemental ball keeps its own hue at every speed: a duller, greyer version
// below cruise, and MORE of its colour - deeper, punchier - the faster it goes
// (a fast green ball reads as vivid green, not washed-out white).
inline sf::Color elementSpeedColor(sf::Color elem, float speed, float cruise) {
    const float ratio = cruise > 1.f ? speed / cruise : 1.f;
    if (ratio < 1.f)
        return lerpColor(lerpColor(elem, sf::Color(90, 94, 105), 0.5f), elem,
                         clampf(ratio, 0.f, 1.f));
    const float t = clampf((ratio - 1.f) / 1.4f, 0.f, 1.f);
    return vivify(elem, t * 0.9f, t * 22.f);
}

}  // namespace sb::theme
