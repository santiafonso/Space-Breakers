#include "platform/Audio.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>

#include "core/Math.hpp"

namespace sb {

namespace {

constexpr unsigned kSampleRate = 44100;
constexpr std::size_t kVoices = 16;   // roomy so overlapping note tails wash together
constexpr float kMusicVolume = 38.f;   // background bed, well under the sfx

// C major pentatonic over ~two octaves: any subset of these rings consonant, so
// a dense rally of bounces stays musical.
constexpr double kScale[] = {
    261.63, 293.66, 329.63, 392.00, 440.00,
    523.25, 587.33, 659.25, 783.99, 880.00, 1046.50,
};
constexpr int kScaleN = static_cast<int>(sizeof(kScale) / sizeof(kScale[0]));

enum Wave { Sine, Triangle, Square };

double waveform(int wave, double phase) {
    switch (wave) {
        case Triangle: {
            const double x = phase / (2.0 * kPi);
            return 2.0 * std::fabs(2.0 * (x - std::floor(x + 0.5))) - 1.0;
        }
        case Square:
            return std::sin(phase) >= 0.0 ? 1.0 : -1.0;
        default:
            return std::sin(phase);
    }
}

// A single enveloped tone that optionally glides from freqA to freqB.
std::vector<sf::Int16> tone(double freqA, double freqB, double ms, int wave, double gain) {
    const std::size_t n = static_cast<std::size_t>(ms * kSampleRate / 1000.0);
    std::vector<sf::Int16> out(n);
    double phase = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const double u = n > 1 ? static_cast<double>(i) / (n - 1) : 0.0;
        const double freq = freqA + (freqB - freqA) * u;
        phase += 2.0 * kPi * freq / kSampleRate;
        const double attack = u < 0.02 ? u / 0.02 : 1.0;
        const double env = attack * std::exp(-3.2 * u);
        const double s = waveform(wave, phase) * env * gain;
        out[i] = static_cast<sf::Int16>(clampf(static_cast<float>(s), -1.f, 1.f) * 32000.f);
    }
    return out;
}

void mixInto(std::vector<sf::Int16>& dst, const std::vector<sf::Int16>& src, std::size_t at) {
    if (dst.size() < at + src.size()) dst.resize(at + src.size(), 0);
    for (std::size_t i = 0; i < src.size(); ++i) {
        const int v = dst[at + i] + src[i];
        dst[at + i] = static_cast<sf::Int16>(std::max(-32000, std::min(32000, v)));
    }
}

// A soft bell at `f`: the fundamental plus a few decaying partials (octave, the
// fifth, twelfth) for a warm, hypnotic ring rather than a bare sine.
std::vector<sf::Int16> bell(double f, double ms, double gain) {
    std::vector<sf::Int16> out = tone(f, f, ms, Sine, gain);
    mixInto(out, tone(f * 2.0, f * 2.0, ms * 0.8, Sine, gain * 0.35), 0);
    mixInto(out, tone(f * 1.5, f * 1.5, ms * 0.7, Sine, gain * 0.22), 0);
    mixInto(out, tone(f * 3.0, f * 3.0, ms * 0.45, Sine, gain * 0.12), 0);
    return out;
}

bool load(sf::SoundBuffer& buf, const std::vector<sf::Int16>& samples) {
    return buf.loadFromSamples(samples.data(), samples.size(), 1, kSampleRate);
}

}  // namespace

bool Audio::init() {
    // Escape hatch for headless / SSH / broken-audio setups: the game runs
    // silently and is otherwise unaffected.
    if (const char* off = std::getenv("SPACE_BREAKERS_NO_AUDIO"); off && *off && *off != '0')
        return false;

    std::vector<sf::Int16> pickup = tone(520, 780, 90, Triangle, 0.5);
    mixInto(pickup, tone(780, 1180, 120, Triangle, 0.42), pickup.size());

    std::vector<sf::Int16> purchase = tone(523.25, 523.25, 260, Sine, 0.22);
    mixInto(purchase, tone(659.25, 659.25, 240, Sine, 0.2), static_cast<std::size_t>(kSampleRate * 0.02));
    mixInto(purchase, tone(783.99, 783.99, 220, Sine, 0.18), static_cast<std::size_t>(kSampleRate * 0.05));

    // combo tier-up: a gentle two-note rise, pitched further up per tier at the call site
    std::vector<sf::Int16> comboUp = tone(523.25, 523.25, 110, Sine, 0.26);
    mixInto(comboUp, tone(783.99, 783.99, 200, Sine, 0.22), static_cast<std::size_t>(kSampleRate * 0.055));

    noteSoft_.resize(kScaleN);
    noteRich_.resize(kScaleN);
    bool notesOk = true;
    for (int i = 0; i < kScaleN; ++i) {
        notesOk = notesOk && load(noteSoft_[i], tone(kScale[i], kScale[i], 150, Sine, 0.5));
        notesOk = notesOk && load(noteRich_[i], bell(kScale[i], 340, 0.42));
    }

    ok_ = notesOk &&
          load(thud_, tone(150, 82, 200, Triangle, 0.5)) &&
          load(pickup_, pickup) &&
          load(purchase_, purchase) &&
          load(combo_, comboUp) &&
          load(throw_, tone(340, 120, 120, Triangle, 0.4));

    if (ok_) pool_.resize(kVoices);
    return ok_;
}

void Audio::loadMusic(const std::string& menuFile, const std::string& gameFile) {
    if (!ok_) return;   // no audio device / SPACE_BREAKERS_NO_AUDIO
    auto open = [](sf::Music& m, const std::string& file) {
        if (file.empty() || !std::filesystem::exists(file)) return false;
        if (!m.openFromFile(file)) return false;
        m.setLoop(true);
        m.setVolume(kMusicVolume);
        return true;
    };
    menuMusicOk_ = open(menuMusic_, menuFile);
    gameMusicOk_ = open(gameMusic_, gameFile);
}

void Audio::setEnabled(bool e) {
    enabled_ = e;
    applyTrack();
}

void Audio::setTrack(Track t) {
    if (t == track_) return;
    track_ = t;
    applyTrack();
}

void Audio::applyTrack() {
    auto sync = [](sf::Music& m, bool loaded, bool wantPlaying) {
        if (!loaded) return;
        if (wantPlaying) {
            if (m.getStatus() != sf::Music::Playing) m.play();
        } else if (m.getStatus() == sf::Music::Playing) {
            m.pause();   // resume from here when we come back
        }
    };
    sync(menuMusic_, menuMusicOk_, enabled_ && track_ == Track::Menu);
    sync(gameMusic_, gameMusicOk_, enabled_ && track_ == Track::Game);
}

void Audio::play(const sf::SoundBuffer& buffer, float pitch, float volume01) {
    if (!ok_ || !enabled_ || pool_.empty()) return;
    sf::Sound& s = pool_[next_];
    next_ = (next_ + 1) % pool_.size();
    s.setBuffer(buffer);
    s.setPitch(pitch);
    s.setVolume(clampf(volume01, 0.f, 1.f) * 26.f);
    s.play();
}

void Audio::ballHit(float speed01, float harmony01, bool ballPair) {
    if (!ok_ || !enabled_ || noteSoft_.empty()) return;
    speed01 = clampf(speed01, 0.f, 1.f);
    harmony01 = clampf(harmony01, 0.f, 1.f);
    ++hitTick_;

    // scale degree: faster -> higher, ball-vs-ball rings a few steps up, plus a
    // little wander so a steady rally isn't a monotone.
    int n = static_cast<int>(std::lround(speed01 * (kScaleN - 5)));
    if (ballPair) n += 4;
    n += static_cast<int>(hitTick_ % 3) - 1;
    n = std::max(0, std::min(kScaleN - 1, n));

    const float shimmer = 1.f + 0.014f * (static_cast<float>(hitTick_ % 7) - 3.f);

    // The bare sine is always there; it steps back as the harmony layer grows.
    play(noteSoft_[static_cast<std::size_t>(n)], shimmer,
         (0.20f + 0.16f * speed01) * (1.f - 0.45f * harmony01));

    // The bell fades in with the damage combo - "la armonia sube de a poco".
    if (harmony01 > 0.04f)
        play(noteRich_[static_cast<std::size_t>(n)], shimmer,
             (0.09f + 0.32f * harmony01) * (0.55f + 0.45f * speed01));

    // Deep into a chain, sprinkle a chord tone so it blooms into a fuller sound.
    if (harmony01 > 0.4f && hitTick_ % 2 == 0) {
        const int step = harmony01 > 0.75f ? 4 : 2;   // ~fifth vs ~third up the scale
        const int h = std::max(0, std::min(kScaleN - 1, n + step));
        play(noteRich_[static_cast<std::size_t>(h)], shimmer, 0.07f + 0.16f * harmony01);
    }
}

void Audio::coreThud() { play(thud_, 1.f, 0.6f); }

void Audio::pickup() { play(pickup_, 1.f, 0.8f); }

void Audio::purchase() { play(purchase_, 1.f, 0.9f); }

void Audio::comboUp(int tier) { play(combo_, 1.f + static_cast<float>(tier) * 0.06f, 0.5f); }

void Audio::thrown(float power01) {
    power01 = clampf(power01, 0.f, 1.f);
    play(throw_, 0.8f + power01 * 0.7f, 0.3f + power01 * 0.4f);
}

}  // namespace sb
