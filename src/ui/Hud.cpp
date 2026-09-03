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

void Hud::update(float dt, int wave, int finalWave, int enemiesLeft, float coreFrac,
                 float comboMultiplier, const std::optional<ActiveEffect>& effect, bool bossWave) {
    wave_ = wave;
    finalWave_ = finalWave;
    bossWave_ = bossWave;
    enemiesLeft_ = enemiesLeft;
    coreFrac_ = clampf(coreFrac, 0.f, 1.f);
    comboMul_ = comboMultiplier;
    effect_ = effect;

    comboPop_ *= std::exp(-7.f * dt);
    const float target = effect ? 1.f : 0.f;
    effectAlpha_ = lerpf(effectAlpha_, target, 1.f - std::exp(-10.f * dt));
}

void Hud::draw(sf::RenderWindow& window) const {
    if (!font_) return;

    // Damage-combo chip, top-left.
    if (comboMul_ > 1.001f) {
        char chip[16];
        std::snprintf(chip, sizeof(chip), "dmg x%.1f", comboMul_);
        sf::Text combo = makeText(*font_, chip, 18, theme::accent);
        const sf::FloatRect cb = combo.getLocalBounds();
        combo.setOrigin(cb.left, cb.top);
        const float cs = 1.f + 0.22f * comboPop_;
        combo.setScale(cs, cs);
        combo.setPosition(theme::margin, theme::margin - 2.f);
        window.draw(combo);
    }

    // Wave / core-health banner, top centre.
    char banner[48];
    if (bossWave_)
        std::snprintf(banner, sizeof(banner), "Wave %d / %d  -  MINIBOSS", wave_, finalWave_);
    else
        std::snprintf(banner, sizeof(banner), "Wave %d / %d", wave_, finalWave_);
    drawCentered(window, *font_, banner, theme::fsHeading, {size_.x * 0.5f, theme::margin + 6.f},
                 bossWave_ ? theme::coreLow : theme::textHi);

    const float barW = 260.f;
    const float x = size_.x * 0.5f - barW * 0.5f;
    const float y = theme::margin + 28.f;
    const sf::Color hpCol = lerpColor(theme::coreLow, theme::core, coreFrac_);

    sf::RectangleShape track({barW, 4.f});
    track.setPosition(x, y);
    track.setFillColor(withAlpha(theme::arenaEdge, 0.9f));
    window.draw(track);

    sf::RectangleShape fill({barW * coreFrac_, 4.f});
    fill.setPosition(x, y);
    fill.setFillColor(hpCol);
    window.draw(fill);

    drawCentered(window, *font_, std::to_string(enemiesLeft_) + " left", theme::fsSmall,
                 {size_.x * 0.5f, y + 16.f}, theme::textLo);

    // Active power-up bar, a bit lower so it clears the banner.
    if (effectAlpha_ > 0.01f && effect_) {
        const float w = 172.f;
        const float px = size_.x * 0.5f - w / 2.f;
        const float py = theme::margin + 64.f;
        const sf::Color col = powerUpColor(effect_->kind);
        const float frac = clampf(effect_->remaining / effect_->duration, 0.f, 1.f);

        drawCentered(window, *font_, powerUpName(effect_->kind), theme::fsSmall,
                     {size_.x * 0.5f, py - 6.f}, withAlpha(col, effectAlpha_));

        sf::RectangleShape t({w, 3.f});
        t.setPosition(px, py + 8.f);
        t.setFillColor(withAlpha(theme::arenaEdge, effectAlpha_));
        window.draw(t);

        sf::RectangleShape ff({w * frac, 3.f});
        ff.setPosition(px, py + 8.f);
        ff.setFillColor(withAlpha(col, effectAlpha_));
        window.draw(ff);
    }
}

}  // namespace sb
