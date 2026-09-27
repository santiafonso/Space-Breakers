#pragma once

#include "core/Math.hpp"

// The whole visual identity in one place: palette, type sizes and the ball
// speed-to-colour ramp. Nothing here knows about gameplay.
//
// Style: "orbital console" - a dark ink-navy instrument panel. Square glass
// panels with hairline edges and corner brackets, small tracked uppercase
// labels, a faint radar grid centred on the core. Everything structural stays
// low-contrast so the balls are always the brightest thing on screen.
namespace sb::theme {

// Backdrop / surfaces
inline const sf::Color bg{11, 13, 20};
inline const sf::Color bgDeep{5, 6, 11};        // where the vignette darkens to at the screen corners
inline const sf::Color grid{86, 110, 150};      // backdrop grid / radar lines (always at low alpha)
inline const sf::Color glassTop{24, 29, 43};    // panel fill, top -> bottom
inline const sf::Color glassBottom{13, 15, 24};
inline const sf::Color bgHot{24, 26, 58};   // the backdrop charges toward this deep blue-violet as the combo climbs
                                            // (was a red that read as "taking damage")
inline const sf::Color panel{3, 4, 9};  // overlay dim, paired with alpha (ink, not pure black)
inline const sf::Color arenaEdge{52, 60, 84};
inline constexpr float corner = 0.f;   // box corner radius: square, clean edges (the user's call)
inline constexpr float bracket = 9.f;  // corner-bracket arm on panels and frames
inline constexpr float tracking = 2.2f; // letter spacing of the small uppercase labels

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

// Ball classes (item tags, class names in the UI). Striker / Guardian /
// Support reuse ballFast / core / puSurge.
inline const sf::Color classMage{128, 146, 255};
inline const sf::Color classShooter{255, 156, 100};
inline const sf::Color classAssassin{232, 84, 112};
inline const sf::Color classSummoner{150, 226, 120};
inline const sf::Color classJester{255, 124, 214};
inline const sf::Color ability{120, 216, 255};   // ability picks and slots

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
