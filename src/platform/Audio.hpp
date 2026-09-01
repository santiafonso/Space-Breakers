#pragma once

#include <SFML/Audio.hpp>

#include <cstddef>
#include <vector>

namespace sb {

// All sound effects are synthesised at start-up into small buffers, so the
// project ships no audio assets. Everything is short, soft and pitched off the
// action so it reads as feedback rather than noise. Degrades silently when no
// audio device is available.
class Audio {
public:
    bool init();
    void setEnabled(bool e) { enabled_ = e; }
    bool enabled() const { return enabled_; }

    void bounce(float speed01);
    void pickup();
    void purchase();
    void comboUp(int tier);
    void thrown(float power01);

private:
    void play(const sf::SoundBuffer& buffer, float pitch, float volume01);

    bool ok_ = false;
    bool enabled_ = true;
    sf::SoundBuffer bounce_;
    sf::SoundBuffer pickup_;
    sf::SoundBuffer purchase_;
    sf::SoundBuffer combo_;
    sf::SoundBuffer throw_;
    std::vector<sf::Sound> pool_;
    std::size_t next_ = 0;
};

}  // namespace sb
