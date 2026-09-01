#include "platform/Audio.hpp"

#include <cmath>
#include <cstdlib>

#include "core/Math.hpp"

namespace sb {

namespace {

constexpr unsigned kSampleRate = 44100;
constexpr std::size_t kVoices = 8;

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

    ok_ = load(bounce_, tone(440, 300, 55, Sine, 0.55)) &&
          load(pickup_, pickup) &&
          load(purchase_, purchase) &&
          load(combo_, tone(600, 600, 45, Square, 0.32)) &&
          load(throw_, tone(340, 120, 120, Triangle, 0.4));

    if (ok_) pool_.resize(kVoices);
    return ok_;
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

void Audio::bounce(float speed01) {
    speed01 = clampf(speed01, 0.f, 1.f);
    play(bounce_, 0.62f + speed01 * 1.25f, 0.35f + speed01 * 0.4f);
}

void Audio::pickup() { play(pickup_, 1.f, 0.8f); }

void Audio::purchase() { play(purchase_, 1.f, 0.9f); }

void Audio::comboUp(int tier) { play(combo_, 1.f + static_cast<float>(tier) * 0.11f, 0.5f); }

void Audio::thrown(float power01) {
    power01 = clampf(power01, 0.f, 1.f);
    play(throw_, 0.8f + power01 * 0.7f, 0.3f + power01 * 0.4f);
}

}  // namespace sb
