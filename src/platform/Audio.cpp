#include "platform/Audio.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>

#include "core/Math.hpp"

namespace sb {

namespace {

constexpr unsigned kSampleRate = 44100;
constexpr std::size_t kVoices = 24;    // roomy so overlapping note tails wash together
constexpr std::size_t kHitVoices = 8;  // ball hits: their own small pool
// Ball hits thin out as they pile up: never two notes closer than kHitGap s,
// and each note is quieter the busier the last moment was (kHitCrowd hits in
// about kHitWindow s already halve... see ballHit).
constexpr float kHitGap = 0.055f;
constexpr float kHitWindow = 0.45f;
constexpr float kHitCrowd = 3.f;
constexpr float kSfxVolume = 26.f;     // sf::Sound volume of a cue at full scale
constexpr float kMusicVolume = 38.f;   // background bed, well under the sfx
constexpr float kAmbienceVolume = 9.f; // the fight hum sits far under everything
constexpr double kAmbienceSeconds = 4.0;   // loop length: every partial fits a whole number of cycles

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

// A single enveloped tone that optionally glides from freqA to freqB. `attack`
// is the fraction of the length spent fading in (longer = a soft swell).
std::vector<sf::Int16> tone(double freqA, double freqB, double ms, int wave, double gain,
                            double attack = 0.02) {
    const std::size_t n = static_cast<std::size_t>(ms * kSampleRate / 1000.0);
    std::vector<sf::Int16> out(n);
    double phase = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const double u = n > 1 ? static_cast<double>(i) / (n - 1) : 0.0;
        const double freq = freqA + (freqB - freqA) * u;
        phase += 2.0 * kPi * freq / kSampleRate;
        const double a = u < attack ? u / attack : 1.0;
        const double env = a * std::exp(-3.2 * u);
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

std::size_t msToSamples(double ms) { return static_cast<std::size_t>(ms * kSampleRate / 1000.0); }

// A soft bell at `f`: the fundamental plus a few decaying partials (octave, the
// fifth, twelfth) for a warm, hypnotic ring rather than a bare sine.
std::vector<sf::Int16> bell(double f, double ms, double gain) {
    std::vector<sf::Int16> out = tone(f, f, ms, Sine, gain);
    mixInto(out, tone(f * 2.0, f * 2.0, ms * 0.8, Sine, gain * 0.35), 0);
    mixInto(out, tone(f * 1.5, f * 1.5, ms * 0.7, Sine, gain * 0.22), 0);
    mixInto(out, tone(f * 3.0, f * 3.0, ms * 0.45, Sine, gain * 0.12), 0);
    return out;
}

// One note of a cue recipe. `wave` is the Soft style's voice; Bright and Retro
// re-voice every note the same way (see render).
struct Note {
    double fA, fB, ms, at, gain;
    int wave = Sine;
    double attack = 0.02;
};

// Soft = the recipe as written (sine / triangle, round). Bright = a glassy
// voicing with octave + twelfth partials. Retro = a short square chip tone,
// much quieter since a square is loud for its gain.
std::vector<sf::Int16> render(const std::vector<Note>& notes, int style) {
    std::vector<sf::Int16> out;
    for (const Note& n : notes) {
        std::vector<sf::Int16> v;
        if (style == StyleBright) {
            v = tone(n.fA, n.fB, n.ms * 1.15, Sine, n.gain * 0.8, n.attack);
            mixInto(v, tone(n.fA * 2.0, n.fB * 2.0, n.ms * 0.9, Sine, n.gain * 0.3, n.attack), 0);
            mixInto(v, tone(n.fA * 3.0, n.fB * 3.0, n.ms * 0.5, Sine, n.gain * 0.12, n.attack), 0);
        } else if (style == StyleRetro) {
            v = tone(n.fA, n.fB, n.ms * 0.8, Square, n.gain * 0.3, n.attack);
        } else {
            v = tone(n.fA, n.fB, n.ms, n.wave, n.gain, n.attack);
        }
        mixInto(out, v, msToSamples(n.at));
    }
    return out;
}

// The fight hum: a seamless loop (every partial and the slow swell complete a
// whole number of cycles over kAmbienceSeconds, so the seam is silent).
std::vector<sf::Int16> ambienceLoop(int style) {
    struct Partial { double f, gain; int wave; };
    using Chord = std::array<Partial, 4>;   // unused partials have zero gain
    const Chord bright{{{220, 0.14, Sine}, {330, 0.1, Sine}, {440, 0.06, Sine}, {660, 0.03, Sine}}};
    const Chord retro{{{73.5, 0.3, Triangle}, {110.25, 0.14, Triangle}, {0, 0, Sine}, {0, 0, Sine}}};
    const Chord soft{{{73.5, 0.34, Sine}, {110.25, 0.14, Sine}, {147, 0.08, Sine}, {0, 0, Sine}}};
    const Chord& parts = style == StyleBright ? bright : (style == StyleRetro ? retro : soft);
    const double lfo = style == StyleBright ? 0.5 : 0.25;
    const std::size_t n = static_cast<std::size_t>(kAmbienceSeconds * kSampleRate);
    std::vector<sf::Int16> out(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / kSampleRate;
        const double swell = 0.72 + 0.28 * std::sin(2.0 * kPi * lfo * t);
        double s = 0.0;
        for (const Partial& p : parts) s += waveform(p.wave, 2.0 * kPi * p.f * t) * p.gain;
        out[i] = static_cast<sf::Int16>(clampf(static_cast<float>(s * swell), -1.f, 1.f) * 32000.f);
    }
    return out;
}

bool load(sf::SoundBuffer& buf, const std::vector<sf::Int16>& samples) {
    return buf.loadFromSamples(samples.data(), samples.size(), 1, kSampleRate);
}

}  // namespace

// What each cue plays: {from Hz, to Hz, ms, start ms, gain, soft wave, attack}.
// Everything short, soft and in (or near) the bounce notes' C pentatonic.
static std::vector<Note> cueRecipe(int c) {
    switch (c) {
        case 0:  return {{340, 120, 120, 0, 0.4, Triangle}};                                   // throw
        case 1:  return {{440, 660, 45, 0, 0.26}};                                             // grab
        case 2:  return {{523.25, 392, 70, 0, 0.2}};                                           // let go
        case 3:  return {{150, 82, 200, 0, 0.5, Triangle}};                                    // core thud
        case 4:  return {{620, 300, 80, 0, 0.3}, {1240, 1240, 30, 0, 0.08}};                   // kill
        case 5:  return {{520, 780, 90, 0, 0.5, Triangle}, {780, 1180, 120, 90, 0.42, Triangle}};   // pickup
        case 6:  return {{523.25, 523.25, 110, 0, 0.26}, {783.99, 783.99, 200, 55, 0.22}};     // combo
        case 7:  return {{523.25, 523.25, 260, 0, 0.22}, {659.25, 659.25, 240, 20, 0.2},
                         {783.99, 783.99, 220, 50, 0.18}};                                      // purchase
        case 8:  return {{1318.5, 1318.5, 60, 0, 0.16}, {1975.5, 1975.5, 70, 35, 0.12}};       // gold
        case 9:  return {{392, 392, 180, 0, 0.2}, {523.25, 523.25, 240, 90, 0.2}};             // wave start
        case 10: return {{523.25, 523.25, 260, 0, 0.17}, {659.25, 659.25, 260, 70, 0.17},
                         {783.99, 783.99, 260, 140, 0.17}, {1046.5, 1046.5, 380, 210, 0.16}};   // wave clear
        case 11: return {{98, 98, 900, 0, 0.36, Triangle, 0.25}, {146.83, 146.83, 900, 0, 0.26, Triangle, 0.25},
                         {196, 196, 700, 200, 0.14, Sine, 0.3}};                                // boss
        case 12: return {{329.63, 329.63, 140, 0, 0.24}, {261.63, 261.63, 180, 170, 0.24}};    // warning
        case 13: return {{880, 880, 50, 0, 0.13}, {987.77, 987.77, 50, 60, 0.13},
                         {1174.66, 1174.66, 60, 120, 0.13}};                                    // cards dealt
        case 14: return {{659.25, 659.25, 120, 0, 0.22}, {987.77, 987.77, 180, 50, 0.2}};      // card pick
        case 15: return {{523.25, 523.25, 160, 0, 0.16}, {659.25, 659.25, 160, 45, 0.16},
                         {783.99, 783.99, 160, 90, 0.16}, {1046.5, 1046.5, 200, 135, 0.15},
                         {1318.5, 1318.5, 260, 180, 0.13}};                                     // level up
        case 16: return {{260, 520, 150, 0, 0.2, Triangle, 0.15}};                             // travel
        case 17: return {{1760, 1760, 22, 0, 0.07}};                                           // hover
        case 18: return {{880, 700, 40, 0, 0.2, Triangle}};                                    // click
        case 19: return {{440, 660, 90, 0, 0.14, Sine, 0.2}};                                  // open
        case 20: return {{660, 440, 90, 0, 0.12, Sine, 0.1}};                                  // close
        case 21: return {{261.63, 261.63, 420, 0, 0.18, Sine, 0.12}, {329.63, 329.63, 400, 70, 0.17, Sine, 0.12},
                         {392, 392, 420, 140, 0.17, Sine, 0.12}, {523.25, 523.25, 560, 210, 0.18, Sine, 0.1},
                         {1046.5, 1046.5, 380, 260, 0.06}};                                     // class gained
        case 22: return {{196, 196, 700, 0, 0.16, Triangle, 0.2}, {261.63, 261.63, 620, 60, 0.17, Sine, 0.12},
                         {329.63, 329.63, 600, 130, 0.17, Sine, 0.12}, {392, 392, 600, 200, 0.17, Sine, 0.12},
                         {523.25, 523.25, 640, 270, 0.17, Sine, 0.1}, {659.25, 659.25, 660, 340, 0.15, Sine, 0.1},
                         {1046.5, 1046.5, 800, 420, 0.13}, {1568, 1568, 600, 480, 0.06}};   // ascended
        default: return {};
    }
}

bool Audio::init() {
    // Escape hatch for headless / SSH / broken-audio setups: the game runs
    // silently and is otherwise unaffected.
    if (const char* off = std::getenv("SPACE_BREAKERS_NO_AUDIO"); off && *off && *off != '0')
        return false;

    static_assert(CueAscend == 22, "cueRecipe is indexed by Cue");
    bool good = true;
    for (int st = 0; st < kStyles; ++st) {
        noteMain_[st].resize(kScaleN);
        noteRich_[st].resize(kScaleN);
        for (int i = 0; i < kScaleN; ++i) {
            const double f = kScale[i];
            std::vector<sf::Int16> main, rich;
            if (st == StyleBright) {          // glassy: bells all the way, the combo layer an octave up
                main = bell(f, 200, 0.4);
                rich = bell(f * 2.0, 380, 0.26);
            } else if (st == StyleRetro) {    // chip blips with a triangle chime on top
                main = tone(f, f, 90, Square, 0.15);
                rich = tone(f, f, 220, Triangle, 0.4);
            } else {                          // the original: a pure sine and a warm bell
                main = tone(f, f, 190, Sine, 0.42, 0.07);   // a soft swell, not a click
                rich = bell(f, 360, 0.36);
            }
            good = good && load(noteMain_[st][i], main) && load(noteRich_[st][i], rich);
        }
        for (int c = 0; c < CueCount; ++c) good = good && load(cues_[c][st], render(cueRecipe(c), st));
        good = good && load(ambienceBuf_[st], ambienceLoop(st));
    }
    ok_ = good;
    if (ok_) {
        pool_.resize(kVoices);
        hitPool_.resize(kHitVoices);
        ambience_.setLoop(true);
        lastCue_.fill(-10.f);
    }
    return ok_;
}

void Audio::addMusic(Track t, const std::string& file, float gain, Loop loop) {
    if (!ok_) return;   // no audio device / SPACE_BREAKERS_NO_AUDIO
    if (t == Track::None || t == Track::Count || file.empty() || !std::filesystem::exists(file)) return;
    Stream st;
    st.loop = loop;
    st.gain = gain;
    for (int v = 0; v < (loop.seam > 0.f ? 2 : 1); ++v) {
        auto m = std::make_unique<sf::Music>();
        if (!m->openFromFile(file)) return;
        m->setLoop(loop.seam <= 0.f);   // a seamed song loops by hand (update)
        m->setVolume(0.f);
        st.voice[static_cast<std::size_t>(v)] = std::move(m);
    }
    music_[static_cast<std::size_t>(t)].push_back(std::move(st));
}

void Audio::setEnabled(bool e) {
    enabled_ = e;   // the music fades out / back in through update()
    applyAmbience();
}

float Audio::musicVolume() const {
    const SoundSettings& s = settings_;
    return s.musicOn ? kMusicVolume * clampf(s.master / 100.f, 0.f, 1.f) * clampf(s.music / 100.f, 0.f, 1.f)
                     : 0.f;
}

void Audio::applySettings(const SoundSettings& s) {
    settings_ = s;
    for (int& st : settings_.style) st = std::clamp(st, 0, SoundStyleCount - 1);
    applyAmbience();   // the music picks the new volume up in update()
}

void Audio::setTrack(Track t, int variant) {
    track_ = t;
    if (const auto& loops = music_[static_cast<std::size_t>(t)]; !loops.empty())
        pick_[static_cast<std::size_t>(t)] = static_cast<std::size_t>(std::max(variant, 0)) % loops.size();
}

void Audio::setFadeIn(Track t, int variant, float seconds) {
    auto& loops = music_[static_cast<std::size_t>(t)];
    if (variant >= 0 && static_cast<std::size_t>(variant) < loops.size())
        loops[static_cast<std::size_t>(variant)].fadeIn = std::max(0.1f, seconds);
}

void Audio::update(float dt) {
    // A calm hand-over: the old loop eases out first and the new one only starts
    // rising once the old is mostly gone, so two tempos never clash at full level.
    constexpr float kFadeOut = 1.5f;     // seconds
    constexpr float kHandOver = 0.25f;   // the new loop waits until the rest are below this
    // Intensity swells into a fight in ~2 s and settles back down in ~3 s.
    const float di = intensityTarget_ - intensity_;
    intensity_ += di > 0.f ? std::min(di, dt / 2.f) : std::max(di, -dt / 3.f);
    const float vol = musicVolume();
    float others = 0.f;   // loudest loop that is on its way out
    for (std::size_t t = 0; t < kTracks; ++t)
        for (std::size_t i = 0; i < music_[t].size(); ++i)
            if (t != static_cast<std::size_t>(track_) || i != pick_[t]) others = std::max(others, music_[t][i].level);
    for (std::size_t t = 0; t < kTracks; ++t) {
        for (std::size_t i = 0; i < music_[t].size(); ++i) {
            Stream& st = music_[t][i];
            const bool want = enabled_ && t == static_cast<std::size_t>(track_) && i == pick_[t];
            if (want && others <= kHandOver) st.level = std::min(1.f, st.level + dt / st.fadeIn);
            else if (!want) st.level = std::max(0.f, st.level - dt / kFadeOut);
            const float k = t == static_cast<std::size_t>(Track::Menu) ? 1.f : intensity_;
            updateStream(st, t == static_cast<std::size_t>(Track::Boss), dt, vol * k);
        }
    }
}

void Audio::updateStream(Stream& st, bool restart, float dt, float vol) {
    sf::Music& cur = *st.voice[static_cast<std::size_t>(st.cur)];
    sf::Music* old = st.voice[1] ? st.voice[static_cast<std::size_t>(1 - st.cur)].get() : nullptr;
    auto isPlaying = [](const sf::Music& m) { return m.getStatus() == sf::Music::Playing; };

    if (st.level <= 0.f) {   // silent: park it
        for (auto& v : st.voice) {
            if (!v || !isPlaying(*v)) continue;
            if (restart) v->stop();
            else v->pause();   // resume from here
        }
        if (restart) { st.cur = 0; st.seamT = 1.f; }
        return;
    }
    // Audible (fading in or out): keep the sounding voices going.
    if (!isPlaying(cur)) cur.play();
    if (old && st.seamT < 1.f && !isPlaying(*old)) old->play();

    const Loop& L = st.loop;
    if (old) {
        // Near the loop end, the other voice picks the song up again from the
        // loop start and the two cross over the seam.
        if (st.seamT >= 1.f && cur.getPlayingOffset().asSeconds() >= L.end - L.seam) {
            old->play();
            old->setPlayingOffset(sf::seconds(L.start));
            st.cur = 1 - st.cur;
            st.seamT = 0.f;
        } else if (st.seamT < 1.f) {
            st.seamT = std::min(1.f, st.seamT + dt / L.seam);
            if (st.seamT >= 1.f) old->stop();
        }
    }

    // Track fade (smoothstep) x loudness match x the seam's equal-power crossfade.
    const float l = st.level;
    const float base = vol * st.gain * l * l * (3.f - 2.f * l);
    const float x = st.seamT * kPi * 0.5f;
    // (clamped: cos(pi/2) is a hair below zero, and OpenAL rejects a negative gain)
    st.voice[static_cast<std::size_t>(st.cur)]->setVolume(std::max(0.f, base * std::sin(x)));
    if (st.voice[1]) st.voice[static_cast<std::size_t>(1 - st.cur)]->setVolume(std::max(0.f, base * std::cos(x)));
}

void Audio::setAmbience(bool on) {
    if (on == ambienceWanted_) return;
    ambienceWanted_ = on;
    applyAmbience();
}

void Audio::applyAmbience() {
    if (!ok_) return;
    const float g = catGain(SndAmbience);
    if (!ambienceWanted_ || g <= 0.f) {
        if (ambience_.getStatus() != sf::Sound::Stopped) ambience_.stop();
        return;
    }
    const int st = settings_.style[SndAmbience];
    if (st != ambienceStyle_) {
        ambience_.stop();
        ambience_.setBuffer(ambienceBuf_[static_cast<std::size_t>(st)]);
        ambienceStyle_ = st;
    }
    ambience_.setVolume(kAmbienceVolume * g);
    if (ambience_.getStatus() != sf::Sound::Playing) ambience_.play();
}

float Audio::catGain(int cat) const {
    if (!enabled_ || cat < 0 || cat >= SoundCatCount) return 0.f;
    const int st = settings_.style[static_cast<std::size_t>(cat)];
    if (st < 0 || st >= kStyles || !settings_.sfxOn) return 0.f;   // Off
    return clampf(settings_.master / 100.f, 0.f, 1.f) * clampf(settings_.sfx / 100.f, 0.f, 1.f) *
           clampf(settings_.vol[static_cast<std::size_t>(cat)] / 100.f, 0.f, 1.f);
}

void Audio::play(const sf::SoundBuffer& buffer, float pitch, float volume01, int cat) {
    if (!ok_ || pool_.empty()) return;
    const float g = catGain(cat);
    if (g <= 0.f) return;
    sf::Sound& s = pool_[next_];
    next_ = (next_ + 1) % pool_.size();
    s.setBuffer(buffer);
    s.setPitch(pitch);
    s.setVolume(clampf(volume01, 0.f, 1.f) * kSfxVolume * g);
    s.play();
}

void Audio::cue(Cue c, float pitch, float volume01, float minGap) {
    static constexpr int kCat[CueCount] = {
        SndThrow, SndGrab, SndGrab, SndCoreHit, SndKill, SndPickup, SndCombo, SndGold, SndGold,
        SndWave, SndWave, SndWave, SndWave, SndCards, SndCards, SndCards,
        SndUiClick, SndUiHover, SndUiClick, SndScreens, SndScreens, SndCards, SndCards,
    };
    if (!ok_) return;
    const int cat = kCat[c];
    const float now = clock_.getElapsedTime().asSeconds();
    if (now - lastCue_[c] < minGap) return;
    const int st = settings_.style[static_cast<std::size_t>(cat)];
    if (st < 0 || st >= kStyles || catGain(cat) <= 0.f) return;
    lastCue_[c] = now;
    play(cues_[c][static_cast<std::size_t>(st)], pitch, volume01, cat);
}

void Audio::playHit(const sf::SoundBuffer& buffer, float pitch, float volume01) {
    if (hitPool_.empty()) return;
    const float g = catGain(SndBallHit);
    if (g <= 0.f) return;
    sf::Sound& s = hitPool_[hitNext_];
    hitNext_ = (hitNext_ + 1) % hitPool_.size();
    s.setBuffer(buffer);
    s.setPitch(pitch);
    s.setVolume(clampf(volume01, 0.f, 1.f) * kSfxVolume * g);
    s.play();
}

void Audio::ballHit(float speed01, float harmony01, bool ballPair) {
    if (!ok_ || catGain(SndBallHit) <= 0.f) return;
    const auto st = static_cast<std::size_t>(settings_.style[SndBallHit]);
    const auto& mainNotes = noteMain_[st];
    const auto& richNotes = noteRich_[st];
    speed01 = clampf(speed01, 0.f, 1.f);
    harmony01 = clampf(harmony01, 0.f, 1.f);

    // How busy the last moment was: every hit counts, the count fades out
    // over ~kHitWindow s. Hits closer than kHitGap don't sound at all, so a
    // pile of bounces becomes a steady patter instead of a wall.
    const float now = clock_.getElapsedTime().asSeconds();
    if (lastHitT_ >= 0.f) hitRate_ *= std::exp(-(now - lastHitT_) / kHitWindow);
    hitRate_ += 1.f;
    lastHitT_ = now;
    if (lastNoteT_ >= 0.f && now - lastNoteT_ < kHitGap) return;
    lastNoteT_ = now;
    const float crowd = 1.f / std::sqrt(std::max(1.f, hitRate_ / kHitCrowd));   // 1 alone, ~0.5 in a crowd
    ++hitTick_;

    // scale degree: faster -> higher, ball-vs-ball rings a few steps up, plus a
    // little wander so a steady rally isn't a monotone - but never the same
    // note twice running (a repeated note is what grates).
    int n = static_cast<int>(std::lround(speed01 * (kScaleN - 5)));
    if (ballPair) n += 4;
    n += static_cast<int>(hitTick_ % 3) - 1;
    n = std::max(0, std::min(kScaleN - 1, n));
    if (n == lastNote_) n = n > 0 ? n - 1 : n + 1;
    lastNote_ = n;

    const float shimmer = 1.f + 0.01f * (static_cast<float>(hitTick_ % 7) - 3.f);

    // The main tone is always there; it steps back as the harmony layer grows.
    playHit(mainNotes[static_cast<std::size_t>(n)], shimmer,
            (0.15f + 0.11f * speed01) * (1.f - 0.45f * harmony01) * crowd);

    // The bell fades in with the damage combo - "la armonia sube de a poco".
    if (harmony01 > 0.04f)
        playHit(richNotes[static_cast<std::size_t>(n)], shimmer,
                (0.07f + 0.24f * harmony01) * (0.55f + 0.45f * speed01) * crowd);

    // Deep into a chain, now and then a chord tone so it blooms - only while
    // it isn't already crowded.
    if (harmony01 > 0.4f && hitTick_ % 3 == 0 && crowd > 0.6f) {
        const int step = harmony01 > 0.75f ? 4 : 2;   // ~fifth vs ~third up the scale
        const int h = std::max(0, std::min(kScaleN - 1, n + step));
        playHit(richNotes[static_cast<std::size_t>(h)], shimmer, (0.05f + 0.12f * harmony01) * crowd);
    }
}

void Audio::coreThud() { cue(CueThud, 1.f, 0.6f); }

void Audio::pickup() { cue(CuePickup, 1.f, 0.8f); }

void Audio::purchase() { cue(CuePurchase, 1.f, 0.9f); }

void Audio::comboUp(int tier) { cue(CueCombo, 1.f + static_cast<float>(tier) * 0.06f, 0.5f); }

void Audio::thrown(float power01) {
    power01 = clampf(power01, 0.f, 1.f);
    cue(CueThrow, 0.8f + power01 * 0.7f, 0.3f + power01 * 0.4f);
}

void Audio::grab() { cue(CueGrab, 1.f, 0.55f, 0.05f); }

void Audio::letGo() { cue(CueLetGo, 1.f, 0.5f, 0.05f); }

void Audio::kill(int n) {
    if (n <= 0) return;
    // A touch higher for a burst; wander a little so a string of kills isn't a monotone.
    const float wander = 1.f + 0.03f * (static_cast<float>(hitTick_ % 5) - 2.f);
    const int k = std::min(n, 4);
    cue(CueKill, wander * (1.f + 0.05f * static_cast<float>(k - 1)), 0.45f + 0.08f * static_cast<float>(k),
        0.05f);
}

void Audio::gold() { cue(CueGold, 1.f, 0.4f, 0.09f); }

void Audio::waveStart() { cue(CueWaveStart, 1.f, 0.7f); }

void Audio::waveClear() { cue(CueWaveClear, 1.f, 0.8f); }

void Audio::bossAppear() { cue(CueBoss, 1.f, 0.9f); }

void Audio::coreWarning() { cue(CueWarning, 1.f, 0.55f, 1.f); }

void Audio::cardsDealt() { cue(CueCards, 1.f, 0.6f, 0.15f); }

void Audio::cardPick() { cue(CueCardPick, 1.f, 0.8f, 0.1f); }

void Audio::levelUp(int level) {
    cue(CueLevelUp, 1.f + 0.04f * static_cast<float>(std::max(0, level - 2)), 0.8f, 0.1f);
}

void Audio::classGain(bool ascended) { cue(ascended ? CueAscend : CueClassGain, 1.f, 0.9f, 0.3f); }

void Audio::travel() { cue(CueTravel, 1.f, 0.7f, 0.1f); }

void Audio::uiHover() { cue(CueHover, 1.f, 0.6f, 0.045f); }

void Audio::uiClick() { cue(CueClick, 1.f, 0.7f, 0.04f); }

// A screen opened / closed - unless a click just did it: the click already said so.
void Audio::uiOpen() {
    if (clock_.getElapsedTime().asSeconds() - lastCue_[CueClick] < 0.1f) return;
    cue(CueOpen, 1.f, 0.6f, 0.08f);
}

void Audio::uiClose() {
    if (clock_.getElapsedTime().asSeconds() - lastCue_[CueClick] < 0.1f) return;
    cue(CueClose, 1.f, 0.6f, 0.08f);
}

void Audio::preview(int cat) {
    if (!ok_) return;
    switch (cat) {
        case SndBallHit:  ballHit(0.5f, 0.6f, false); break;
        case SndThrow:    thrown(0.6f); break;
        case SndGrab:     cue(CueGrab, 1.f, 0.55f); break;
        case SndCoreHit:  coreThud(); break;
        case SndKill:     cue(CueKill, 1.f, 0.55f); break;
        case SndPickup:   pickup(); break;
        case SndCombo:    comboUp(2); break;
        case SndGold:     purchase(); break;
        case SndWave:     waveClear(); break;
        case SndCards:    cue(CueCardPick, 1.f, 0.8f); break;
        case SndUiClick:  cue(CueClick, 1.f, 0.7f); break;
        case SndUiHover:  cue(CueHover, 1.f, 0.6f); break;
        case SndScreens:  cue(CueOpen, 1.f, 0.6f); break;
        case SndAmbience: {
            // A few seconds of the loop, unless it's already humming under a fight.
            const int st = settings_.style[SndAmbience];
            if (ambience_.getStatus() != sf::Sound::Playing && st >= 0 && st < kStyles)
                play(ambienceBuf_[static_cast<std::size_t>(st)], 1.f, kAmbienceVolume / kSfxVolume, SndAmbience);
            break;
        }
        default: break;
    }
}

}  // namespace sb
