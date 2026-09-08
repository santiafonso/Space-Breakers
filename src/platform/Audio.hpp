#pragma once

#include <SFML/Audio.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace sb {

// Sound effects are synthesised at start-up into small buffers. Background music
// streams from OGG files under assets/music/ (optional - the game is silent if
// they are missing). Everything degrades silently when no audio device is
// available.
class Audio {
public:
    enum class Track { None, Menu, Game };

    bool init();
    // Streamed loops. Pass empty for a track that has no file; missing files are
    // simply skipped.
    void loadMusic(const std::string& menuFile, const std::string& gameFile);
    void setEnabled(bool e);
    bool enabled() const { return enabled_; }

    void setTrack(Track t);   // idempotent: cross to the given looping track
    Track track() const { return track_; }

    void bounce(float speed01);
    void pickup();
    void purchase();
    void comboUp(int tier);
    void thrown(float power01);

private:
    void play(const sf::SoundBuffer& buffer, float pitch, float volume01);
    void applyTrack();        // start / pause each music stream to match track_ + enabled_

    bool ok_ = false;
    bool enabled_ = true;
    sf::SoundBuffer bounce_;
    sf::SoundBuffer pickup_;
    sf::SoundBuffer purchase_;
    sf::SoundBuffer combo_;
    sf::SoundBuffer throw_;
    std::vector<sf::Sound> pool_;
    std::size_t next_ = 0;

    Track track_ = Track::None;
    sf::Music menuMusic_;
    sf::Music gameMusic_;
    bool menuMusicOk_ = false;
    bool gameMusicOk_ = false;
};

}  // namespace sb
