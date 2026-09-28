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

// The palette runs a notch calmer than pure hues: colours are pulled toward
// their own grey and dimmed a little, so nothing on screen shouts.
inline sf::Color soften(sf::Color c, float sat = 0.8f, float val = 0.93f) {
    const float avg = (c.r + c.g + c.b) / 3.f;
    auto ch = [&](float v) { return static_cast<std::uint8_t>(clampf((avg + (v - avg) * sat) * val, 0.f, 255.f)); };
    return sf::Color(ch(c.r), ch(c.g), ch(c.b), c.a);
}

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
inline const sf::Color accent = soften({92, 200, 255});

// Ball speed ramp (calm -> hot)
inline const sf::Color ballSlow = soften({84, 150, 235});
inline const sf::Color ballMid = soften({90, 214, 160});
inline const sf::Color ballFast = soften({240, 206, 96});
inline const sf::Color ballUltra = soften({240, 96, 104});

// Combat
inline const sf::Color core = soften({120, 230, 200});
inline const sf::Color coreLow = soften({240, 110, 90});
inline const sf::Color enemy = soften({232, 120, 120});

// Ball elements - MUTED on purpose (2026-09-27): a ball's identity is its
// class (vivid colours below); the element is a secondary detail (a rim, the
// trail, status tints, reactions), so its hues are desaturated and a touch
// darker. Old vivid values in DESIGN.md (Fase Q).
inline const sf::Color elemFire{206, 136, 92};
inline const sf::Color elemPoison{142, 180, 108};
inline const sf::Color elemWater{104, 138, 204};
inline const sf::Color elemIce{158, 192, 204};
inline const sf::Color elemStone{154, 142, 126};
inline const sf::Color elemElectric{152, 122, 204};

// Vivid warm / green hues for things that aren't elements (pact archetypes,
// a web branch, coins) - they used to borrow the old element colours.
inline const sf::Color ember = soften({255, 148, 66});
inline const sf::Color venom = soften({150, 214, 96});

// Power-ups
inline const sf::Color puPoints = soften({245, 200, 90});
inline const sf::Color puSlow = soften({130, 200, 255});
inline const sf::Color puSurge = soften({198, 120, 255});
inline const sf::Color puGolden = soften({255, 214, 120});
inline const sf::Color puOverdrive = soften({255, 110, 150});

// Ball classes (item tags, class names in the UI) - the vivid ones: a class
// is a ball's identity, so its colour is the ball's body colour. Striker /
// Guardian / Support reuse ballFast / core / puSurge.
inline const sf::Color classMage = soften({128, 146, 255});
inline const sf::Color classShooter = soften({255, 156, 100});
inline const sf::Color classAssassin = soften({232, 84, 112});
inline const sf::Color classSummoner = soften({150, 226, 120});
inline const sf::Color classJester = soften({255, 124, 214});
inline const sf::Color ability = soften({120, 216, 255});   // ability picks and slots

// Layout / type - room to breathe: few things on screen, none of them tiny
inline constexpr float margin = 30.f;
inline constexpr unsigned fsTitle = 38;
inline constexpr unsigned fsHeading = 22;
inline constexpr unsigned fsItem = 24;
inline constexpr unsigned fsBody = 16;
inline constexpr unsigned fsHud = 24;
inline constexpr unsigned fsSmall = 14;

// Colour of a plain ball for a given speed relative to its (un-buffed) cruise
// speed: a neutral grey that just brightens as it speeds up, so class balls
// (which keep their class hue) always stand out from the classless ones.
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

// A class ball keeps its class hue at every speed: a duller, greyer version
// below cruise, and MORE of its colour - deeper, punchier - the faster it goes
// (a fast Striker reads as vivid gold, not washed-out white). `rich` = the
// ascended form: always a step more saturated and brighter.
inline sf::Color hueSpeedColor(sf::Color hue, float speed, float cruise, bool rich = false) {
    if (rich) hue = vivify(hue, 0.25f, 14.f);
    const float ratio = cruise > 1.f ? speed / cruise : 1.f;
    if (ratio < 1.f)
        return lerpColor(lerpColor(hue, sf::Color(90, 94, 105), rich ? 0.3f : 0.5f), hue,
                         clampf(ratio, 0.f, 1.f));
    const float t = clampf((ratio - 1.f) / 1.4f, 0.f, 1.f);
    return vivify(hue, t * 0.9f, t * 22.f);
}

}  // namespace sb::theme
