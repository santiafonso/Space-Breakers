#include "ui/Hud.hpp"

#include <cmath>
#include <cstdio>
#include <string>

#include "core/Theme.hpp"
#include "ui/Widgets.hpp"

namespace sb {

void Hud::init(const sf::Font& font, sf::Vector2f size) {
    font_ = &font;
    size_ = size;
}

void Hud::pulseCombo() { comboPop_ = 1.f; }

void Hud::update(float dt, std::uint32_t score, std::uint32_t bestScore, float comboMultiplier,
                 const std::optional<ActiveEffect>& effect) {
    if (score != score_) {
        scorePop_ = 1.f;
        score_ = score;
    }
    best_ = bestScore;
    comboMul_ = comboMultiplier;
    effect_ = effect;

    scorePop_ *= std::exp(-9.f * dt);
    comboPop_ *= std::exp(-7.f * dt);
    const float target = effect ? 1.f : 0.f;
    effectAlpha_ = lerpf(effectAlpha_, target, 1.f - std::exp(-10.f * dt));
}

void Hud::draw(sf::RenderWindow& window) const {
    if (!font_) return;

    // Score, top-left, with a small pop on gain.
    sf::Text score = makeText(*font_, std::to_string(score_), theme::fsHud, theme::textHi);
    const sf::FloatRect sb = score.getLocalBounds();
    score.setOrigin(sb.left, sb.top);
    const float scale = 1.f + 0.16f * scorePop_;
    score.setScale(scale, scale);
    score.setPosition(theme::margin, theme::margin - 4.f);
    window.draw(score);

    if (best_ > score_) {
        sf::Text best = makeText(*font_, "best  " + std::to_string(best_), theme::fsSmall, theme::textLo);
        best.setPosition(theme::margin + 2.f, theme::margin + theme::fsHud + 4.f);
        window.draw(best);
    }

    // Combo multiplier chip, just under the score. SFML renders std::string as
    // Latin-1, so stick to ASCII glyphs here.
    if (comboMul_ > 1.001f) {
        char chip[16];
        std::snprintf(chip, sizeof(chip), "x%.1f", comboMul_);
        sf::Text combo = makeText(*font_, chip, 18, theme::accent);
        const sf::FloatRect cb = combo.getLocalBounds();
        combo.setOrigin(cb.left, cb.top);
        const float cs = 1.f + 0.22f * comboPop_;
        combo.setScale(cs, cs);
        combo.setPosition(theme::margin + 2.f, theme::margin + theme::fsHud + 22.f);
        window.draw(combo);
    }

    // Active power-up: a thin countdown bar centred at the top.
    if (effectAlpha_ > 0.01f && effect_) {
        const float w = 172.f;
        const float x = size_.x * 0.5f - w / 2.f;
        const float y = theme::margin + 6.f;
        const sf::Color col = powerUpColor(effect_->kind);
        const float frac = clampf(effect_->remaining / effect_->duration, 0.f, 1.f);

        drawCentered(window, *font_, powerUpName(effect_->kind), theme::fsSmall,
                     {size_.x * 0.5f, y - 6.f}, withAlpha(col, effectAlpha_));

        sf::RectangleShape track({w, 3.f});
        track.setPosition(x, y + 8.f);
        track.setFillColor(withAlpha(theme::arenaEdge, effectAlpha_));
        window.draw(track);

        sf::RectangleShape fill({w * frac, 3.f});
        fill.setPosition(x, y + 8.f);
        fill.setFillColor(withAlpha(col, effectAlpha_));
        window.draw(fill);
    }
}

}  // namespace sb
