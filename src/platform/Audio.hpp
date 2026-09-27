#pragma once

#include <SFML/Audio.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "platform/SoundSettings.hpp"

namespace sb {

// Sound effects are synthesised at start-up into small buffers - one per
// sound per style (Soft / Bright / Retro), picked per category by the player's
// SoundSettings. Background music streams from OGG files under assets/music/
// (optional - the game is silent if they are missing). Everything degrades
// silently when no audio device is available.
class Audio {
public:
    enum class Track { None, Menu, Game };

    bool init();
    // Streamed loops. Pass empty for a track that has no file; missing files are
    // simply skipped.
    void loadMusic(const std::string& menuFile, const std::string& gameFile);
    void setEnabled(bool e);
    bool enabled() const { return enabled_; }
    // The player's mix: volumes and a style per category (takes effect at once).
    void applySettings(const SoundSettings& s);

    void setTrack(Track t);   // idempotent: cross to the given looping track
    Track track() const { return track_; }
    void setAmbience(bool on);   // idempotent: the quiet fight hum loop

    // A ball clacked off a wall / core / another ball. Plays a note from a
    // pentatonic set (higher = faster, ball-vs-ball rings up an octave); as
    // `harmony01` (the damage combo, 0..1) climbs, a bell layer and chord tones
    // bloom in, so a long frantic chain sounds richer and fuller.
    void ballHit(float speed01, float harmony01, bool ballPair);
    void coreThud();          // an enemy reached the core: a low, dull knock
    void pickup();
    void purchase();
    void comboUp(int tier);
    void thrown(float power01);

    // Feedback cues. The spammy ones (hover, kill, gold, grab) are rate-limited here.
    void grab();              // a ball picked up
    void letGo();             // a held ball let go without a throw
    void kill(int n);         // n enemies destroyed this frame
    void gold();              // gold ticked up
    void waveStart();
    void waveClear();
    void bossAppear();
    void coreWarning();       // the core is low
    void cardsDealt();        // a set of cards / a shop opens
    void cardPick();
    void levelUp(int level);  // an item went up a level
    // A ball gained a class: a rising major chord that swells. Ascended = the
    // same chord an octave climb longer, with a high bell on top. (Cards category.)
    void classGain(bool ascended);
    void travel();            // stepped onto a map node
    void uiHover();
    void uiClick();
    void uiOpen();            // a screen opened (skipped right after a click)
    void uiClose();

    // Play the category's sound once with the current settings (Sound screen).
    void preview(int cat);

private:
    enum Cue {
        CueThrow, CueGrab, CueLetGo, CueThud, CueKill, CuePickup, CueCombo, CuePurchase, CueGold,
        CueWaveStart, CueWaveClear, CueBoss, CueWarning, CueCards, CueCardPick, CueLevelUp,
        CueTravel, CueHover, CueClick, CueOpen, CueClose, CueClassGain, CueAscend, CueCount
    };
    static constexpr int kStyles = 3;   // Soft / Bright / Retro (Off plays nothing)

    void play(const sf::SoundBuffer& buffer, float pitch, float volume01, int cat);
    // One cue in its category's style; `minGap` seconds of rate limiting.
    void cue(Cue c, float pitch, float volume01, float minGap = 0.f);
    float catGain(int cat) const;       // master * sfx * category, 0 when Off / muted
    void applyTrack();        // start / pause each music stream to match track_ + enabled_
    void applyAmbience();

    bool ok_ = false;
    bool enabled_ = true;
    SoundSettings settings_;
    // Bounce notes per style, one per pentatonic degree: the main tone and the
    // bell layer that fades in with the combo.
    std::array<std::vector<sf::SoundBuffer>, kStyles> noteMain_;
    std::array<std::vector<sf::SoundBuffer>, kStyles> noteRich_;
    std::array<std::array<sf::SoundBuffer, kStyles>, CueCount> cues_;
    std::array<float, CueCount> lastCue_{};   // clock time each cue last played
    std::array<sf::SoundBuffer, kStyles> ambienceBuf_;
    sf::Sound ambience_;
    bool ambienceWanted_ = false;
    int ambienceStyle_ = -1;
    sf::Clock clock_;
    std::vector<sf::Sound> pool_;
    std::size_t next_ = 0;
    unsigned hitTick_ = 0;    // rolls forward per ball hit, for subtle note / pitch wander

    Track track_ = Track::None;
    sf::Music menuMusic_;
    sf::Music gameMusic_;
    bool menuMusicOk_ = false;
    bool gameMusicOk_ = false;
};

}  // namespace sb
