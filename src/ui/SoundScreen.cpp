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

// Layout, in UI units: a panel centred on the screen, its tabs under the
// title, then the tab's rows. General is narrow (three sections of a few rows
// each); Sound mix is wide (a table of every category).
constexpr float kTop = 64.f;            // the panel's top edge
constexpr float kTitleY = kTop + 42.f;
constexpr float kTabsY = kTop + 84.f;   // the tabs' top
constexpr float kTabW = 150.f, kTabH = 30.f;
constexpr float kGenW = 600.f;          // General's panel width
constexpr float kMixW = 860.f;          // Sound mix's
constexpr float kPad = 40.f;            // inside the panel, left and right

// General: section headers and rows, from the top.
constexpr float kAudioHead = kTop + 150.f;
constexpr float kGlobalY = kTop + 186.f;   // first of the three volumes
constexpr float kGlobalStep = 40.f;
constexpr float kDisplayHead = kTop + 320.f;
constexpr float kFullY = kTop + 356.f;
constexpr float kCardsHead = kTop + 400.f;
constexpr float kTextY = kTop + 436.f;
constexpr float kGenBottom = kTop + 556.f;
// a volume row: name | slider | value | switch
constexpr float kGenSliderX0 = -150.f, kGenSliderX1 = 120.f;   // from the centre
constexpr float kGenValueX = 166.f;                             // right edge of the number
constexpr float kSwitchW = 42.f, kSwitchH = 20.f;

// Sound mix: the table.
constexpr float kHeadY = kTop + 152.f;   // column captions
constexpr float kRowY = kTop + 182.f;    // first category row
constexpr float kRowStep = 27.f;
constexpr float kNameX = -370.f;         // name column (left edge), from the centre
constexpr float kSliderX0 = -190.f;
constexpr float kSliderX1 = 30.f;
constexpr float kValueX = 74.f;          // volume number (right edge)
constexpr float kChipX = 96.f;           // first style chip
constexpr float kChipW = 66.f;
constexpr float kChipGap = 6.f;
constexpr float kChipH = 20.f;
constexpr float kMixBottom = kRowY + kRowStep * static_cast<float>(SoundCatCount) + 96.f;

const char* const kGlobalNames[3] = {"Master", "Music", "Effects"};

int& globalValue(SoundSettings& s, int i) { return i == 0 ? s.master : (i == 1 ? s.music : s.sfx); }
int globalValue(const SoundSettings& s, int i) { return i == 0 ? s.master : (i == 1 ? s.music : s.sfx); }

float panelW(bool mix) { return mix ? kMixW : kGenW; }
float panelBottom(bool mix) { return mix ? kMixBottom : kGenBottom; }

float sliderY(int id) {
    return id < 100 ? kGlobalY + kGlobalStep * static_cast<float>(id)
                    : kRowY + kRowStep * static_cast<float>(id - 100);
}
// The slider's ends, x from the screen centre: the global ones live on General, the rest in the table.
float sliderX0(int id) { return id < 100 ? kGenSliderX0 : kSliderX0; }
float sliderX1(int id) { return id < 100 ? kGenSliderX1 : kSliderX1; }

sf::FloatRect sliderHit(sf::Vector2f s, int id) {
    const float cx = s.x * 0.5f;
    return {cx + sliderX0(id) - 10.f, sliderY(id) - 12.f, sliderX1(id) - sliderX0(id) + 20.f, 24.f};
}

sf::FloatRect chipRect(sf::Vector2f s, int cat, int style) {
    const float cx = s.x * 0.5f;
    return {cx + kChipX + static_cast<float>(style) * (kChipW + kChipGap), sliderY(100 + cat) - kChipH * 0.5f,
            kChipW, kChipH};
}

// An on-off switch at the right end of a General row.
sf::FloatRect switchAt(sf::Vector2f s, float y) {
    return {s.x * 0.5f + kGenW * 0.5f - kPad - kSwitchW, y - kSwitchH * 0.5f, kSwitchW, kSwitchH};
}
// The switch beside each global volume: 0 = all sound, 1 = music, 2 = effects.
sf::FloatRect switchRect(sf::Vector2f s, int i) { return switchAt(s, sliderY(i)); }
sf::FloatRect fullRect(sf::Vector2f s) { return switchAt(s, kFullY); }
// Descriptions: a two-part toggle, Simple | Detailed, at the row's right end.
sf::FloatRect textRect(sf::Vector2f s, int part) {
    const float w = 92.f, right = s.x * 0.5f + kGenW * 0.5f - kPad;
    return {right - w * static_cast<float>(2 - part), kTextY - 13.f, w, 26.f};
}
sf::FloatRect tabRect(sf::Vector2f s, int i) {
    return {s.x * 0.5f - kTabW + static_cast<float>(i) * kTabW, kTabsY, kTabW, kTabH};
}
sf::FloatRect backRect(sf::Vector2f s, bool mix) {
    return {s.x * 0.5f - 90.f, panelBottom(mix) - 58.f, 180.f, 34.f};
}

}  // namespace

int SoundScreen::itemAt(App& app, sf::Vector2f m) const {
    const sf::Vector2f s = app.size();
    for (int i = 0; i < 2; ++i)
        if (tabRect(s, i).contains(m)) return kTab + i;
    if (backRect(s, advanced_).contains(m)) return kBack;
    if (!advanced_) {
        for (int i = 0; i < 3; ++i) {
            if (sliderHit(s, i).contains(m)) return i;
            if (switchRect(s, i).contains(m)) return kMute + i;
        }
        if (fullRect(s).contains(m)) return kFull;
        if (textRect(s, 0).contains(m)) return kSimple;
        if (textRect(s, 1).contains(m)) return kDetailed;
        return -1;
    }
    for (int c = 0; c < SoundCatCount; ++c) {
        if (sliderHit(s, 100 + c).contains(m)) return 100 + c;
        for (int st = 0; st < SoundStyleCount; ++st)
            if (chipRect(s, c, st).contains(m)) return 200 + c * SoundStyleCount + st;
    }
    return -1;
}

void SoundScreen::setSlider(App& app, int id, float x) {
    const float cx = app.size().x * 0.5f;
    const float t = clampf((x - (cx + sliderX0(id))) / (sliderX1(id) - sliderX0(id)), 0.f, 1.f);
    const int v = static_cast<int>(std::lround(t * 20.f)) * 5;   // steps of 5%
    SoundSettings& snd = app.data().meta.sound;
    int& target = id < 100 ? globalValue(snd, id) : snd.vol[static_cast<std::size_t>(id - 100)];
    if (target == v) return;
    target = v;
    if (id == 1) snd.musicOn = v > 0;   // the slider is the switch too: 0 is off
    if (id == 2) snd.sfxOn = v > 0;
    app.audio().applySettings(snd);
}

void SoundScreen::close(App& app) {
    drag_ = -1;
    app.save();
    app.back();
}

void SoundScreen::handleEvent(App& app, const sf::Event& e, sf::Vector2f mouse) {
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) { close(app); return; }
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Tab) { advanced_ = !advanced_; return; }
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
    if (id == kTab || id == kTab + 1) { advanced_ = id == kTab + 1; return; }
    if (id == kFull) { app.toggleFullscreen(); return; }
    if (id == kSimple || id == kDetailed) {
        if (app.data().meta.simpleText != (id == kSimple)) app.toggleSimpleText();
        return;
    }
    if (id == kMute) {
        app.toggleSound();
        return;
    }
    if (id == kMute + 1 || id == kMute + 2) {   // music / effects on-off
        SoundSettings& snd = app.data().meta.sound;
        bool& on = id == kMute + 1 ? snd.musicOn : snd.sfxOn;
        on = !on;
        int& vol = globalValue(snd, id - kMute);
        if (on && vol == 0) vol = 50;   // switched on at 0: give it a volume to be heard at
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
    uisound::hover(this, hover_, (hover_ >= kMute && hover_ <= kMute + 2) || hover_ == kBack || hover_ == kFull ||
                                 hover_ == kSimple || hover_ == kDetailed || hover_ == kTab || hover_ == kTab + 1);
}

void SoundScreen::draw(App& app, sf::RenderWindow& w) {
    const sf::Vector2f s = app.size();
    const float cx = s.x * 0.5f;
    const sf::Font& f = app.font();
    const float a = clampf(introPop(intro(), 0.f, 0.22f), 0.f, 1.f);
    const SoundSettings& snd = app.data().meta.sound;
    const bool muted = !app.data().meta.soundOn;
    const float pw = panelW(advanced_);
    const float left = cx - pw * 0.5f + kPad, right = cx + pw * 0.5f - kPad;

    drawDim(w, s, 0.8f * a);
    draw::panel(w, {cx - pw * 0.5f, kTop, pw, panelBottom(advanced_) - kTop}, theme::arenaEdge, a);
    drawCenteredPop(w, f, "Options", theme::fsTitle, {cx, kTitleY}, theme::textHi, introPop(intro(), 0.03f, 0.28f));

    // ---- the tabs: the one you're on lit, with a bar under it
    for (int i = 0; i < 2; ++i) {
        const sf::FloatRect r = tabRect(s, i);
        const bool on = advanced_ == (i == 1);
        const bool hov = hover_ == kTab + i;
        const sf::Color c = on ? theme::textHi : (hov ? theme::textLo : theme::textDim);
        drawLabel(w, f, i == 0 ? "General" : "Sound mix", 13, {r.left + r.width * 0.5f, r.top + r.height * 0.5f},
                  withAlpha(c, a));
        draw::line(w, {r.left + 12.f, r.top + r.height + 2.f}, {r.left + r.width - 12.f, r.top + r.height + 2.f},
                   on ? 2.f : 1.f, withAlpha(on ? theme::accent : theme::arenaEdge, a));
    }

    // A slider: a track, its filled part in the accent, a knob; the value at the right.
    auto slider = [&](int id, int value, bool live, float valueX) {
        const float y = sliderY(id);
        const float x0 = cx + sliderX0(id), x1 = cx + sliderX1(id);
        const float h = hover_ == id ? 1.f : 0.f;
        const float xv = x0 + (x1 - x0) * static_cast<float>(value) / 100.f;
        draw::line(w, {x0, y}, {x1, y}, 4.f, withAlpha(theme::arenaEdge, 0.8f * a));
        const sf::Color fill = live ? lerpColor(theme::accent, theme::textHi, 0.3f * h) : theme::textDim;
        if (xv > x0 + 0.5f) draw::line(w, {x0, y}, {xv, y}, 4.f, withAlpha(fill, a));
        const sf::Color kc = live ? lerpColor(theme::textLo, theme::textHi, 0.5f + 0.5f * h) : theme::textDim;
        draw::box(w, {std::round(xv) - 4.f, std::round(y) - 9.f, 8.f, 18.f}, theme::corner, withAlpha(kc, a),
                  withAlpha(lerpColor(kc, theme::bg, 0.25f), a));
        drawLabel(w, f, std::to_string(value), 12, {cx + valueX, y}, withAlpha(live ? theme::textLo : theme::textDim, a), 1);
    };
    // An on-off switch: a track lit in `on` colour with the knob at its end.
    auto toggle = [&](sf::FloatRect r, bool on, bool hov, sf::Color onCol) {
        const sf::Color edge = on ? onCol : theme::textDim;
        draw::box(w, r, theme::corner, withAlpha(on ? lerpColor(theme::bgDeep, onCol, 0.45f) : theme::bgDeep, a),
                  withAlpha(on ? lerpColor(theme::bgDeep, onCol, 0.3f) : theme::bgDeep, a),
                  withAlpha(edge, (hov ? 1.f : 0.75f) * a), 1.f);
        const float k = r.height - 6.f;
        const float kx = on ? r.left + r.width - 3.f - k : r.left + 3.f;
        const sf::Color kc = on ? lerpColor(onCol, sf::Color::White, 0.5f) : theme::textLo;
        draw::box(w, {kx, r.top + 3.f, k, k}, theme::corner, withAlpha(kc, a), withAlpha(lerpColor(kc, theme::bg, 0.2f), a));
        drawLabel(w, f, on ? "on" : "off", 11, {r.left - 10.f, r.top + r.height * 0.5f},
                  withAlpha(on ? theme::textLo : theme::textDim, a), 1);
    };
    // A section: its name, small and lit, and a hairline to the panel's edge.
    auto section = [&](const char* name, float y) {
        const float tw = makeLabel(f, name, 11, theme::accent).getLocalBounds().width;
        drawLabel(w, f, name, 11, {left, y}, withAlpha(theme::accent, a), -1);
        draw::line(w, {left + tw + 12.f, y}, {right, y}, 1.f, withAlpha(theme::arenaEdge, 0.7f * a));
    };
    auto rowName = [&](const std::string& name, float y, bool hot, bool live) {
        sf::Text t = makeText(f, name, theme::fsBody, withAlpha(hot ? theme::textHi : (live ? theme::textLo : theme::textDim), a));
        const sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(b.left, b.top + b.height * 0.5f);
        t.setPosition(std::round(left), std::round(y));
        w.draw(t);
    };

    std::string footnote;   // one quiet line above Back
    if (!advanced_) {
        // ---- Sound: a volume and a switch each
        section("SOUND", kAudioHead);
        for (int i = 0; i < 3; ++i) {
            const float y = sliderY(i);
            const bool on = i == 0 ? !muted : (i == 1 ? snd.musicOn : snd.sfxOn);
            rowName(kGlobalNames[i], y, hover_ == i || hover_ == kMute + i, !muted && on);
            slider(i, globalValue(snd, i), !muted && on, kGenValueX);
            toggle(switchRect(s, i), on, hover_ == kMute + i, i == 0 ? theme::core : theme::accent);
        }
        // ---- Display
        section("DISPLAY", kDisplayHead);
        const bool full = app.data().meta.fullscreen;
        rowName("Fullscreen", kFullY, hover_ == kFull, true);
        toggle(fullRect(s), full, hover_ == kFull, theme::accent);
        // ---- Cards: two choices side by side, the chosen one lit
        section("CARDS", kCardsHead);
        const bool simple = app.data().meta.simpleText;
        rowName("Descriptions", kTextY, hover_ == kSimple || hover_ == kDetailed, true);
        for (int part = 0; part < 2; ++part) {
            const sf::FloatRect r = textRect(s, part);
            const bool on = simple == (part == 0);
            const bool hov = hover_ == (part == 0 ? kSimple : kDetailed);
            draw::panel(w, r, on ? theme::accent : theme::arenaEdge, a * (on || hov ? 1.f : 0.7f), on ? 0.7f : (hov ? 0.4f : 0.f));
            drawLabel(w, f, part == 0 ? "Simple" : "Detailed", 11, {r.left + r.width * 0.5f, r.top + r.height * 0.5f},
                      withAlpha(on ? theme::textHi : (hov ? theme::textLo : theme::textDim), a));
        }
        if (hover_ == kSimple || hover_ == kDetailed)
            footnote = hover_ == kSimple ? "cards show the gist of each item - click a card for every number"
                                         : "cards show every number of each item";
        else if (hover_ == kFull)
            footnote = "F11 toggles it anywhere";
        else if (muted)
            footnote = "all sound is muted  -  M in a fight toggles it too";
    } else {
        // ---- per category: name, volume, style
        drawLabel(w, f, "Category", 11, {cx + kNameX, kHeadY}, withAlpha(theme::textDim, a), -1);
        drawLabel(w, f, "Volume", 11, {cx + kSliderX0, kHeadY}, withAlpha(theme::textDim, a), -1);
        drawLabel(w, f, "Style", 11, {cx + kChipX, kHeadY}, withAlpha(theme::textDim, a), -1);
        draw::line(w, {left, kHeadY + 14.f}, {right, kHeadY + 14.f}, 1.f, withAlpha(theme::arenaEdge, 0.7f * a));

        int rowHover = -1;
        if (hover_ >= 100 && hover_ < 200) rowHover = hover_ - 100;
        else if (hover_ >= 200) rowHover = (hover_ - 200) / SoundStyleCount;

        for (int c = 0; c < SoundCatCount; ++c) {
            const float ra = a * clampf(introPop(intro(), 0.04f + 0.018f * static_cast<float>(c), 0.2f), 0.f, 1.f);
            if (ra <= 0.001f) continue;
            const float y = sliderY(100 + c);
            if (rowHover == c)   // the row under the pointer, faintly lit
                draw::box(w, {left - 10.f, y - kRowStep * 0.5f, right - left + 20.f, kRowStep}, theme::corner,
                          withAlpha(theme::accent, 0.06f * ra), withAlpha(theme::accent, 0.03f * ra));
            const int style = snd.style[static_cast<std::size_t>(c)];
            const bool live = !muted && snd.sfxOn && style != StyleOff;
            drawLabel(w, f, soundCatName(c), 12, {cx + kNameX, y},
                      withAlpha(rowHover == c ? theme::textHi : (live ? theme::textLo : theme::textDim), ra), -1);
            slider(100 + c, snd.vol[static_cast<std::size_t>(c)], live, kValueX);
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
        if (rowHover >= 0) footnote = soundCatDesc(rowHover);
        else if (muted || !snd.sfxOn) footnote = "effects are off - switch them on in General";
    }
    const sf::FloatRect br = backRect(s, advanced_);
    if (!footnote.empty())
        drawCentered(w, f, footnote, theme::fsSmall, {cx, br.top - 22.f}, withAlpha(theme::textLo, a));

    draw::panel(w, br, theme::accent, a, hover_ == kBack ? 1.f : 0.f);
    drawLabel(w, f, "Back (Esc)", 12, {br.left + br.width * 0.5f, br.top + br.height * 0.5f},
              withAlpha(lerpColor(theme::textLo, theme::textHi, hover_ == kBack ? 1.f : 0.5f), a));
}

}  // namespace sb
