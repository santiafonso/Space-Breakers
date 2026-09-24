#include "ui/Hud.hpp"

#include <algorithm>
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
void Hud::pulseGold() { goldPop_ = std::min(1.f, goldPop_ + 0.5f); }

void Hud::update(float dt, int wave, int finalWave, int enemiesLeft, float coreFrac,
                 float comboMultiplier, int score, int gold, const std::optional<ActiveEffect>& effect,
                 bool bossWave, bool hasReserve, PowerUp reservePu) {
    wave_ = wave;
    finalWave_ = finalWave;
    bossWave_ = bossWave;
    enemiesLeft_ = enemiesLeft;
    score_ = score;
    gold_ = gold;
    coreFrac_ = clampf(coreFrac, 0.f, 1.f);
    comboMul_ = comboMultiplier;
    effect_ = effect;
    hasReserve_ = hasReserve;
    reservePu_ = reservePu;

    comboPop_ *= std::exp(-7.f * dt);
    goldPop_ *= std::exp(-9.f * dt);
    const float target = effect ? 1.f : 0.f;
    effectAlpha_ = lerpf(effectAlpha_, target, 1.f - std::exp(-10.f * dt));
}

bool Hud::tooltipAt(sf::Vector2f m, std::string& title, std::string& desc, sf::Color& color) const {
    const float mg = theme::margin;
    color = theme::textHi;
    if (comboMul_ > 1.001f && sf::FloatRect(mg - 4.f, mg - 6.f, 130.f, 30.f).contains(m)) {
        title = "Damage combo";
        desc = "chain hits without a pause and every ball hits harder; it fades if you stop hitting";
        color = theme::accent;
        return true;
    }
    if (sf::FloatRect(size_.x - mg - 170.f, mg - 6.f, 176.f, 24.f).contains(m)) {
        title = "Score";
        desc = "points for kills this run; your best is kept on the Stats screen";
        return true;
    }
    if (sf::FloatRect(size_.x - mg - 120.f, mg + 18.f, 126.f, 20.f).contains(m)) {
        title = "Gold";
        desc = "earned from fights; spend it in shops on the map";
        color = theme::puGolden;
        return true;
    }
    if (hasReserve_ && sf::FloatRect(size_.x - mg - 170.f, mg + 36.f, 176.f, 20.f).contains(m)) {
        title = std::string("Reserve: ") + powerUpName(reservePu_);
        desc = std::string(powerUpDesc(reservePu_)) + " - press Q to use it";
        color = powerUpColor(reservePu_);
        return true;
    }
    if (effectAlpha_ > 0.5f && effect_ &&
        sf::FloatRect(size_.x * 0.5f - 100.f, mg + 50.f, 200.f, 30.f).contains(m)) {
        title = powerUpName(effect_->kind);
        desc = powerUpDesc(effect_->kind);
        color = powerUpColor(effect_->kind);
        return true;
    }
    if (sf::FloatRect(size_.x * 0.5f - 160.f, mg - 8.f, 320.f, 60.f).contains(m)) {
        title = bossWave_ ? "Boss wave" : "Wave";
        desc = "the bar is the core's health - if it empties the run is over. Below: enemies left.";
        return true;
    }
    return false;
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

    // Run score, top-right.
    {
        sf::Text sc = makeText(*font_, "SCORE  " + std::to_string(score_), 18, theme::textLo);
        const sf::FloatRect sb = sc.getLocalBounds();
        sc.setOrigin(sb.left + sb.width, sb.top);
        sc.setPosition(size_.x - theme::margin, theme::margin - 2.f);
        window.draw(sc);
    }

    // Run gold, under the score.
    {
        sf::Text gd = makeText(*font_, "GOLD  " + std::to_string(gold_), theme::fsSmall, theme::puGolden);
        const sf::FloatRect gb = gd.getLocalBounds();
        gd.setOrigin(gb.left + gb.width, gb.top);
        const float gs = 1.f + 0.3f * goldPop_;   // bumps as coins land
        gd.setScale(gs, gs);
        gd.setPosition(size_.x - theme::margin, theme::margin + 22.f);
        window.draw(gd);
    }

    // "Stockpile" reserve power-up, under the gold.
    if (hasReserve_) {
        const sf::Color col = powerUpColor(reservePu_);
        sf::Text rs = makeText(*font_, std::string("[Q] ") + powerUpName(reservePu_),
                               theme::fsSmall, col);
        const sf::FloatRect rb = rs.getLocalBounds();
        rs.setOrigin(rb.left + rb.width, rb.top);
        rs.setPosition(size_.x - theme::margin, theme::margin + 40.f);
        window.draw(rs);
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
