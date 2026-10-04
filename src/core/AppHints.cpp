// App: first-run hints (2026-10-04) - one line at a time in the bottom band,
// shown once ever: it goes away the moment you do what it says (or after a
// while, if you never do). Which ones you've done is in the save (meta.hints).

#include <algorithm>
#include <cmath>

#include "core/App.hpp"
#include "core/Config.hpp"
#include "core/Theme.hpp"
#include "ui/Screens.hpp"
#include "ui/Widgets.hpp"

namespace sb {

namespace {

const char* hintText(App::Hint h) {
    switch (h) {
        case App::Hint::Throw:   return "click a ball: it flies at the enemy nearest the core";
        case App::Hint::Aim:     return "hold a ball and pull back to aim it, like a slingshot";
        case App::Hint::Volley:  return "Q: every free ball at once";
        case App::Hint::Bullet:  return "hold E: time slows while the green ring lasts";
        case App::Hint::Repulse: return "F: a shockwave pushes the enemies off the core";
        case App::Hint::Map:     return "click a lit stop to go there";
        case App::Hint::Tab:     return "TAB: your balls up close - drag items between them";
        case App::Hint::Recruit: return "Recruit: a new ball joins you";
        case App::Hint::Elite:   return "Elite: a harder fight that pays an item";
        case App::Hint::Shop:    return "Shop: spend your gold on what's on the shelf";
        case App::Hint::Forge:   return "Forge: level up an item a ball already carries";
        case App::Hint::Boss:    return "the boss: break it to end the act";
        case App::Hint::WebBuy:  return "your cores buy upgrades: click a lit node";
        case App::Hint::WebStart: return "Start run: a real one this time, with paths to choose";
        case App::Hint::Count:   break;
    }
    return "";
}

constexpr float kHintGiveUp = 25.f;   // seconds up before it stops asking
constexpr float kHintGap = 1.5f;      // a breath between two hints

}  // namespace

void App::hintDone(Hint h) {
    if (hintSeen(h)) return;
    data_.meta.hints |= 1u << static_cast<int>(h);
    save();
}

void App::updateHints(float dt) {
    // The hint this moment wants: the first one not done yet that fits the
    // screen on top - a live fight or the map.
    auto first = [&](std::initializer_list<Hint> hs) {
        for (Hint h : hs)
            if (!hintSeen(h)) return static_cast<int>(h);
        return -1;
    };
    int want = -1;
    if (data_.run.active && !stack_.empty() && !peek_.open) {
        if (simulating() && !stack_.back()->frozen() && world_.waveRunning() && !world_.balls().empty()) {
            if (canGrab() && !hintSeen(Hint::Throw)) want = static_cast<int>(Hint::Throw);
            else if (canGrab() && !hintSeen(Hint::Aim)) want = static_cast<int>(Hint::Aim);
            else if (!hintSeen(Hint::Volley)) want = static_cast<int>(Hint::Volley);
            else if (bulletOpen() && !hintSeen(Hint::Bullet)) want = static_cast<int>(Hint::Bullet);
            else if (repulseOpen() && !hintSeen(Hint::Repulse)) want = static_cast<int>(Hint::Repulse);
            else want = first({Hint::Tab});
        } else if (dynamic_cast<const MapScreen*>(stack_.back().get())) {
            // After "click a stop": what the next stop is, the first time you meet
            // one of its kind (left once you step on it), then TAB.
            want = first({Hint::Map});
            for (int i = 0; want < 0 && i < static_cast<int>(data_.run.map.nodes.size()); ++i) {
                if (!mapNodeOpen(i)) continue;
                Hint h = Hint::Count;
                switch (data_.run.map.nodes[static_cast<std::size_t>(i)].type) {
                    case MapNodeType::Recruit: h = Hint::Recruit; break;
                    case MapNodeType::Elite:   h = Hint::Elite; break;
                    case MapNodeType::Shop:    h = Hint::Shop; break;
                    case MapNodeType::Forge:   h = Hint::Forge; break;
                    case MapNodeType::Boss:    h = Hint::Boss; break;
                    default: break;
                }
                if (h != Hint::Count && !hintSeen(h)) want = static_cast<int>(h);
            }
            if (want < 0) want = first({Hint::Tab});
        }
    } else if (!data_.run.active && data_.meta.stats.runs > 0 && !stack_.empty() &&
               dynamic_cast<const LoadoutScreen*>(stack_.back().get())) {
        // Back at the skill web after a run: what the cores are for, then go again.
        want = first({Hint::WebBuy, Hint::WebStart});
    }

    if (hint_ >= 0 && (hintSeen(static_cast<Hint>(hint_)) || want != hint_)) {
        hintA_ = std::max(0.f, hintA_ - dt * 4.f);   // done, or the screen moved on: fade out
        if (hintA_ <= 0.f) {
            hint_ = -1;
            hintGap_ = kHintGap;
        }
        return;
    }
    if (hint_ < 0) {
        hintGap_ -= dt;
        if (hintGap_ > 0.f || want < 0) return;
        hint_ = want;
        hintA_ = 0.f;
        hintT_ = 0.f;
    }
    hintA_ = std::min(1.f, hintA_ + dt * 2.5f);
    if ((hintT_ += dt) > kHintGiveUp) hintDone(static_cast<Hint>(hint_));
}

void App::drawHint(sf::RenderWindow& w) const {
    if (hint_ < 0 || hintA_ <= 0.01f) return;
    // Centred in the bottom band; a soft breathing so the eye finds it.
    const sf::Vector2f s = size();
    const float pulse = 0.85f + 0.15f * std::sin(hintT_ * 3.f);
    drawCentered(w, font_, hintText(static_cast<Hint>(hint_)), theme::fsSmall,
                 {s.x * 0.5f, s.y - cfg::app::arenaBand * 0.5f}, withAlpha(theme::textHi, hintA_ * pulse));
}

}  // namespace sb
