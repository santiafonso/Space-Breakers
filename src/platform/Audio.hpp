#pragma once

#include <SFML/Audio.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "platform/SoundSettings.hpp"

namespace sb {

// Sound effects are synthesised at start-up into small buffers - one per
// sound per style (Soft / Bright / Retro), picked per category by the player's
// SoundSettings. Background music streams from files under assets/music/ (the
// menu, one loop per act, the boss; optional - a part with no file is silent). Everything degrades silently when no audio device is available.
class Audio {
public:
    enum class Track { None, Menu, Run, Boss, Web, Count };   // Web: the skill web, between runs

    // Where a file loops. A song that fades out at its end loops back from
    // `end` (before the fade) to `start` through a `seam`-second crossfade
    // with a second copy of itself; seam 0 = a plain loop of the whole file
    // (for tracks already cut to loop).
    struct Loop {
        float start = 0.f;   // seconds
        float end = 0.f;
        float seam = 0.f;
    };

    bool init();
    // Add a streamed loop to a track (Run takes several: one per act, picked by
    // setTrack's `variant`). `gain` evens out loudness between files. Empty /
    // missing files are simply skipped.
    void addMusic(Track t, const std::string& file, float gain, Loop loop);
    void addMusic(Track t, const std::string& file, float gain = 1.f) { addMusic(t, file, gain, Loop{}); }
    void setEnabled(bool e);
    bool enabled() const { return enabled_; }
    // The player's mix: volumes and a style per category (takes effect at once).
    void applySettings(const SoundSettings& s);

    // Idempotent: cross to the track's loop number `variant` (wraps around).
    void setTrack(Track t, int variant = 0);
    Track track() const { return track_; }
    void update(float dt);    // per frame: the music crossfade
    // How hard the run / boss music plays, 0..1, eased toward: soft on the map
    // and in menus, full in a fight. The menu loop ignores it.
    void setIntensity(float target) { intensityTarget_ = target; }
    // Seconds a loop takes to fade in (default 2.5).
    void setFadeIn(Track t, int variant, float seconds);
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
    float musicVolume() const;   // sf::Music volume at full fade level
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
    // Ball hits get their own few voices (a crowd of bounces steals from
    // itself, not from the other cues) and thin out when they pile up.
    std::vector<sf::Sound> hitPool_;
    std::size_t hitNext_ = 0;
    float hitRate_ = 0.f;      // recent hits, decaying (see ballHit)
    float lastHitT_ = -1.f;
    float lastNoteT_ = -1.f;
    int lastNote_ = -1;
    void playHit(const sf::SoundBuffer& buffer, float pitch, float volume01);

    // One song that fades in / out between tracks; paused at silence so it
    // resumes from there (the boss loop restarts instead). A seamed Loop plays
    // on two voices that take turns across the loop point.
    struct Stream {
        std::array<std::unique_ptr<sf::Music>, 2> voice;   // [1] only with a seam
        Loop loop;
        int cur = 0;         // the voice carrying the song
        float seamT = 1.f;   // 0..1 through a loop-point crossfade (1 = none running)
        float level = 0.f;   // 0..1 track crossfade position
        float gain = 1.f;    // loudness match
        float fadeIn = 2.5f; // seconds from silence to full
    };
    // Park / run its voices, cross the loop point, set volumes. `restart`: rewind when parked.
    void updateStream(Stream& st, bool restart, float dt, float vol);
    static constexpr std::size_t kTracks = static_cast<std::size_t>(Track::Count);
    std::array<std::vector<Stream>, kTracks> music_;
    std::array<std::size_t, kTracks> pick_{};   // which of a track's loops plays
    Track track_ = Track::None;
    float intensity_ = 0.f, intensityTarget_ = 1.f;
};

}  // namespace sb
