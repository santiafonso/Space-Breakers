#include "ui/SoundScreen.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/App.hpp"
#include "core/Theme.hpp"
#include "platform/SoundSettings.hpp"
#include "render/Draw.hpp"
#include "ui/UiSound.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

// Layout, in UI units around the screen centre.
constexpr float kTitleY = 86.f;
constexpr float kGlobalY = 138.f;     // first of the three global sliders
constexpr float kGlobalStep = 32.f;
constexpr float kHeadY = 252.f;       // column captions
constexpr float kRowY = 280.f;        // first category row
constexpr float kRowStep = 29.f;
constexpr float kNameX = -380.f;      // name column (left edge)
constexpr float kSliderX0 = -180.f;
constexpr float kSliderX1 = 40.f;
constexpr float kValueX = 84.f;       // volume number (right edge)
constexpr float kChipX = 104.f;       // first style chip
constexpr float kChipW = 66.f;
constexpr float kChipGap = 6.f;
constexpr float kChipH = 21.f;

const char* const kGlobalNames[3] = {"Master", "Music", "Effects"};

int& globalValue(SoundSettings& s, int i) { return i == 0 ? s.master : (i == 1 ? s.music : s.sfx); }
int globalValue(const SoundSettings& s, int i) { return i == 0 ? s.master : (i == 1 ? s.music : s.sfx); }

float sliderY(int id) {
    return id < 100 ? kGlobalY + kGlobalStep * static_cast<float>(id)
                    : kRowY + kRowStep * static_cast<float>(id - 100);
}

sf::FloatRect sliderHit(sf::Vector2f s, int id) {
    const float cx = s.x * 0.5f;
    return {cx + kSliderX0 - 8.f, sliderY(id) - 11.f, kSliderX1 - kSliderX0 + 16.f, 22.f};
}

sf::FloatRect chipRect(sf::Vector2f s, int cat, int style) {
    const float cx = s.x * 0.5f;
    return {cx + kChipX + static_cast<float>(style) * (kChipW + kChipGap), sliderY(100 + cat) - kChipH * 0.5f,
            kChipW, kChipH};
}

// The switch beside each global slider: 0 = all sound, 1 = music, 2 = effects.
sf::FloatRect switchRect(sf::Vector2f s, int i) {
    const float cx = s.x * 0.5f;
    return {cx + kChipX, sliderY(i) - kChipH * 0.5f, kChipW * 2.f + kChipGap, kChipH};
}

sf::FloatRect backRect(sf::Vector2f s) { return {s.x * 0.5f - 80.f, 716.f, 160.f, 30.f}; }

}  // namespace

int SoundScreen::itemAt(App& app, sf::Vector2f m) const {
    const sf::Vector2f s = app.size();
    for (int i = 0; i < 3; ++i)
        if (sliderHit(s, i).contains(m)) return i;
    for (int i = 0; i < 3; ++i)
        if (switchRect(s, i).contains(m)) return kMute + i;
    if (backRect(s).contains(m)) return kBack;
    for (int c = 0; c < SoundCatCount; ++c) {
        if (sliderHit(s, 100 + c).contains(m)) return 100 + c;
        for (int st = 0; st < SoundStyleCount; ++st)
            if (chipRect(s, c, st).contains(m)) return 200 + c * SoundStyleCount + st;
    }
    return -1;
}

void SoundScreen::setSlider(App& app, int id, float x) {
    const float cx = app.size().x * 0.5f;
    const float t = clampf((x - (cx + kSliderX0)) / (kSliderX1 - kSliderX0), 0.f, 1.f);
    const int v = static_cast<int>(std::lround(t * 20.f)) * 5;   // steps of 5%
    SoundSettings& snd = app.data().meta.sound;
    int& target = id < 100 ? globalValue(snd, id) : snd.vol[static_cast<std::size_t>(id - 100)];
    if (target == v) return;
    target = v;
    app.audio().applySettings(snd);
}

void SoundScreen::close(App& app) {
    drag_ = -1;
    app.save();
    app.back();
}

void SoundScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) { close(app); return; }
    if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
        if (drag_ >= 100) app.audio().preview(drag_ - 100);    // hear the category at its new volume
        else if (drag_ == 0 || drag_ == 2) app.audio().preview(SndCards);
        drag_ = -1;
        return;
    }
    if (e.type != sf::Event::MouseButtonPressed || e.mouseButton.button != sf::Mouse::Left) return;

    const int id = itemAt(app, mouse);
    if (id < 0) return;
    if (id == kBack) { close(app); return; }
    if (id == kMute) {
        app.toggleSound();
        return;
    }
    if (id == kMute + 1 || id == kMute + 2) {   // music / effects on-off
        SoundSettings& snd = app.data().meta.sound;
        bool& on = id == kMute + 1 ? snd.musicOn : snd.sfxOn;
        on = !on;
        app.audio().applySettings(snd);
        if (on && id == kMute + 2) app.audio().preview(SndCards);
        return;
    }
    if (id < 200) {
        drag_ = id;
        setSlider(app, id, mouse.x);
        return;
    }
    const int cat = (id - 200) / SoundStyleCount;
    const int st = (id - 200) % SoundStyleCount;
    SoundSettings& snd = app.data().meta.sound;
    snd.style[static_cast<std::size_t>(cat)] = st;
    app.audio().applySettings(snd);
    if (st != StyleOff) app.audio().preview(cat);
}

void SoundScreen::update(App& app, float, sf::Vector2f mouse) {
    mouse_ = mouse;
    if (drag_ >= 0) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) setSlider(app, drag_, mouse.x);
        else drag_ = -1;
    }
    hover_ = drag_ >= 0 ? drag_ : itemAt(app, mouse);
    // Sliders and chips preview their own sound, so only the plain buttons click.
    uisound::hover(this, hover_, (hover_ >= kMute && hover_ <= kMute + 2) || hover_ == kBack);
}

void SoundScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float cx = s.x * 0.5f;
    const sf::Font& f = app.font();
    const float a = clampf(introPop(intro(), 0.f, 0.22f), 0.f, 1.f);
    const SoundSettings& snd = app.data().meta.sound;
    const bool muted = !app.data().meta.soundOn;

    drawDim(w, s, 0.8f * a);
    draw::panel(w, {cx - 410.f, 40.f, 820.f, 724.f}, theme::arenaEdge, a);
    drawCenteredPop(w, f, "Sound", theme::fsTitle, {cx, kTitleY - 12.f}, theme::textHi, introPop(intro(), 0.03f, 0.28f));

    auto slider = [&](int id, int value, bool live) {
        const float y = sliderY(id);
        const float x0 = cx + kSliderX0, x1 = cx + kSliderX1;
        const float h = hover_ == id ? 1.f : 0.f;
        const float xv = x0 + (x1 - x0) * static_cast<float>(value) / 100.f;
        draw::line(w, {x0, y}, {x1, y}, 2.f, withAlpha(theme::arenaEdge, a));
        const sf::Color fill = live ? lerpColor(theme::accent, theme::textHi, 0.3f * h) : theme::textDim;
        if (xv > x0 + 0.5f) draw::line(w, {x0, y}, {xv, y}, 2.f, withAlpha(fill, a));
        sf::RectangleShape knob({6.f, 14.f});
        knob.setOrigin(3.f, 7.f);
        knob.setPosition(std::round(xv), std::round(y));
        knob.setFillColor(withAlpha(live ? lerpColor(theme::textLo, theme::textHi, 0.5f + 0.5f * h) : theme::textDim, a));
        w.draw(knob);
        drawLabel(w, f, std::to_string(value), 11, {cx + kValueX, y},
                  withAlpha(live ? theme::textLo : theme::textDim, a), 1);
    };

    // ---- the global mix
    for (int i = 0; i < 3; ++i) {
        const float y = sliderY(i);
        drawLabel(w, f, kGlobalNames[i], 12, {cx + kNameX, y},
                  withAlpha(hover_ == i ? theme::textHi : theme::textLo, a), -1);
        const bool on = i == 0 ? !muted : (i == 1 ? snd.musicOn : snd.sfxOn);
        slider(i, globalValue(snd, i), !muted && on);
        const sf::FloatRect r = switchRect(s, i);
        const char* text = i == 0 ? (on ? "Sound on" : "Muted")
                                  : (i == 1 ? (on ? "Music on" : "Music off") : (on ? "Effects on" : "Effects off"));
        draw::panel(w, r, on ? theme::accent : theme::coreLow, a, hover_ == kMute + i ? 0.6f : 0.25f);
        drawLabel(w, f, text, 11, {r.left + r.width * 0.5f, r.top + r.height * 0.5f},
                  withAlpha(on ? theme::textHi : theme::coreLow, a));
    }

    // ---- per category: name, volume, style
    draw::line(w, {cx + kNameX, kHeadY - 22.f}, {cx - kNameX + 6.f, kHeadY - 22.f}, 1.f,
               withAlpha(theme::arenaEdge, a));
    drawLabel(w, f, "Category", 11, {cx + kNameX, kHeadY}, withAlpha(theme::textDim, a), -1);
    drawLabel(w, f, "Volume", 11, {cx + kSliderX0, kHeadY}, withAlpha(theme::textDim, a), -1);
    drawLabel(w, f, "Style  -  click to hear it", 11, {cx + kChipX, kHeadY}, withAlpha(theme::textDim, a), -1);

    int rowHover = -1;
    if (hover_ >= 100 && hover_ < 200) rowHover = hover_ - 100;
    else if (hover_ >= 200) rowHover = (hover_ - 200) / SoundStyleCount;

    for (int c = 0; c < SoundCatCount; ++c) {
        const float ra = a * clampf(introPop(intro(), 0.04f + 0.018f * static_cast<float>(c), 0.2f), 0.f, 1.f);
        if (ra <= 0.001f) continue;
        const float y = sliderY(100 + c);
        const int style = snd.style[static_cast<std::size_t>(c)];
        const bool live = !muted && snd.sfxOn && style != StyleOff;
        drawLabel(w, f, soundCatName(c), 12, {cx + kNameX, y},
                  withAlpha(rowHover == c ? theme::textHi : (live ? theme::textLo : theme::textDim), ra), -1);
        slider(100 + c, snd.vol[static_cast<std::size_t>(c)], live);
        for (int st = 0; st < SoundStyleCount; ++st) {
            const sf::FloatRect r = chipRect(s, c, st);
            const bool on = st == style;
            const bool hov = hover_ == 200 + c * SoundStyleCount + st;
            const sf::Color edge = on ? (st == StyleOff ? theme::textLo : theme::accent) : theme::arenaEdge;
            draw::panel(w, r, edge, ra * (on || hov ? 1.f : 0.7f), on ? 0.7f : (hov ? 0.45f : 0.f));
            drawLabel(w, f, soundStyleName(st), 10, {r.left + r.width * 0.5f, r.top + r.height * 0.5f},
                      withAlpha(on ? theme::textHi : (hov ? theme::textLo : theme::textDim), ra));
        }
    }

    // What the hovered row covers - one quiet line, no tooltip clutter.
    const std::string hint = rowHover >= 0 ? soundCatDesc(rowHover)
                             : (muted ? "all sound is muted  -  M in a fight toggles it too" : "");
    if (!hint.empty())
        drawCentered(w, f, hint, theme::fsSmall, {cx, kRowY + kRowStep * SoundCatCount + 10.f},
                     withAlpha(theme::textLo, a));

    const sf::FloatRect br = backRect(s);
    draw::panel(w, br, theme::accent, a, hover_ == kBack ? 1.f : 0.f);
    drawLabel(w, f, "Back (Esc)", 12, {br.left + br.width * 0.5f, br.top + br.height * 0.5f},
              withAlpha(lerpColor(theme::textLo, theme::textHi, hover_ == kBack ? 1.f : 0.5f), a));
}

}  // namespace sb
