#pragma once

#include <array>

// The player's sound mix: master / music / effects volume, plus a volume and a
// synthesised "style" for each category of sound. Plain data - persisted in the
// save (MetaState::sound) and handed to Audio::applySettings.
namespace sb {

enum SoundCat : int {
    SndBallHit,    // ball bounces (the pentatonic notes)
    SndThrow,      // a throw leaving your hand
    SndGrab,       // picking a ball up / letting it go without a throw
    SndCoreHit,    // an enemy reaching the core
    SndKill,       // an enemy destroyed
    SndPickup,     // power-up pickups
    SndCombo,      // combo tier-ups, multi-kills, reactions
    SndGold,       // gold coming in, purchases
    SndWave,       // wave start / cleared, boss arrival, low-core warning
    SndCards,      // cards dealt / picked, item level-ups
    SndUiClick,    // clicking buttons and menu rows, map travel
    SndUiHover,    // the pointer moving onto a button
    SndScreens,    // screens opening / closing
    SndAmbience,   // a quiet hum under a fight
    SoundCatCount
};

// Every category offers the same set of synthesised styles.
enum SoundStyle : int { StyleSoft, StyleBright, StyleRetro, StyleOff, SoundStyleCount };

struct SoundSettings {
    int master = 100;   // 0..100
    int music = 100;
    int sfx = 100;
    std::array<int, SoundCatCount> vol = [] {
        std::array<int, SoundCatCount> v{};
        v.fill(100);
        v[SndUiHover] = 70;
        v[SndAmbience] = 60;
        return v;
    }();
    std::array<int, SoundCatCount> style{};   // all StyleSoft
};

inline const char* soundCatName(int c) {
    static const char* const kNames[SoundCatCount] = {
        "Ball hits", "Throw", "Grab", "Core hit", "Kill", "Pickup", "Combo",
        "Gold", "Waves", "Cards", "Click", "Hover", "Screens", "Ambience",
    };
    return c >= 0 && c < SoundCatCount ? kNames[c] : "";
}

inline const char* soundCatDesc(int c) {
    static const char* const kDescs[SoundCatCount] = {
        "every bounce off a wall, the core or another ball - notes that grow richer with the combo",
        "a ball leaving your hand",
        "picking a ball up, and letting it go without a throw",
        "an enemy reaching the core",
        "an enemy destroyed",
        "power-up pickups",
        "combo tier-ups, multi-kills and reactions",
        "gold coming in and purchases",
        "a wave starting or cleared, a boss arriving, the low-core warning",
        "cards dealt and picked, item level-ups",
        "clicking buttons and menu rows, travelling on the map",
        "the pointer moving onto a button",
        "screens opening and closing",
        "a very quiet hum under a fight",
    };
    return c >= 0 && c < SoundCatCount ? kDescs[c] : "";
}

inline const char* soundStyleName(int s) {
    static const char* const kNames[SoundStyleCount] = {"Soft", "Bright", "Retro", "Off"};
    return s >= 0 && s < SoundStyleCount ? kNames[s] : "";
}

}  // namespace sb
