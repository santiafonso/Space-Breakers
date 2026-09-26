#include "ui/Hud.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "core/Theme.hpp"
#include "render/Draw.hpp"
#include "ui/Widgets.hpp"

namespace sb {

void Hud::init(const sf::Font& font, sf::Vector2f size) {
    font_ = &font;
    size_ = size;
}

void Hud::pulseCombo() { comboPop_ = 1.f; }
void Hud::pulseGold() { goldPop_ = std::min(1.f, goldPop_ + 0.5f); }

void Hud::update(float dt, int act, int stage, int stages, int enemiesLeft, float coreFrac,
                 float comboMultiplier, int score, int gold, const std::optional<ActiveEffect>& effect,
                 bool bossWave, bool hasReserve, PowerUp reservePu) {
    act_ = act;
    stage_ = stage;
    stages_ = stages;
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
    goldShown_ += (static_cast<float>(gold_) - goldShown_) * (1.f - std::exp(-8.f * dt));
    if (std::fabs(static_cast<float>(gold_) - goldShown_) < 0.5f) goldShown_ = static_cast<float>(gold_);
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
        title = bossWave_ ? "Boss" : "Stage";
        desc = "the bar is the core's health - if it empties the run is over. Below: enemies left.";
        return true;
    }
    return false;
}

namespace {

// A value with a small tracked caption to its left, right-aligned at `right`.
// Returns the left edge of the whole thing.
float readout(sf::RenderWindow& w, const sf::Font& font, const std::string& caption, const std::string& value,
              unsigned size, float right, float cy, sf::Color capCol, sf::Color valCol, float scale = 1.f) {
    sf::Text v = makeText(font, value, size, valCol);
    const sf::FloatRect vb = v.getLocalBounds();
    v.setOrigin(vb.left + vb.width, vb.top + vb.height * 0.5f);
    v.setScale(scale, scale);
    v.setPosition(std::round(right), std::round(cy));
    w.draw(v);
    const float capRight = right - vb.width * scale - 9.f;
    drawLabel(w, font, caption, 11, {capRight, cy}, capCol, 1);
    return capRight - makeLabel(font, caption, 11, capCol).getLocalBounds().width;
}

// A thin horizontal gauge with square end ticks: the console's bar style.
void gauge(sf::RenderWindow& w, float x, float y, float wd, float ht, float frac, sf::Color col, float alpha,
           int segments) {
    sf::RectangleShape track({wd, ht});
    track.setPosition(x, y);
    track.setFillColor(withAlpha(theme::arenaEdge, 0.75f * alpha));
    w.draw(track);
    sf::RectangleShape fill({wd * clampf(frac, 0.f, 1.f), ht});
    fill.setPosition(x, y);
    fill.setFillColor(withAlpha(col, alpha));
    w.draw(fill);
    for (int i = 1; i < segments; ++i) {   // cut into cells, like a meter
        const float sx = std::round(x + wd * static_cast<float>(i) / static_cast<float>(segments));
        draw::line(w, {sx, y}, {sx, y + ht}, 2.f, withAlpha(theme::bg, alpha));
    }
    const sf::Color tick = withAlpha(lerpColor(theme::arenaEdge, theme::textLo, 0.5f), alpha);
    draw::line(w, {x - 5.f, y - 4.f}, {x - 5.f, y + ht + 4.f}, 2.f, tick);
    draw::line(w, {x + wd + 5.f, y - 4.f}, {x + wd + 5.f, y + ht + 4.f}, 2.f, tick);
}

}  // namespace

void Hud::draw(sf::RenderWindow& window) const {
    if (!font_) return;
    const float mg = theme::margin;

    // Damage-combo chip, top-left: a bracketed readout.
    if (comboMul_ > 1.001f) {
        const sf::FloatRect chip{mg - 4.f, mg - 6.f, 118.f, 30.f};
        const float heat = clampf((comboMul_ - 1.f) / 2.f, 0.f, 1.f);
        draw::box(window, chip, 0.f, withAlpha(theme::accent, 0.05f + 0.06f * heat), withAlpha(theme::accent, 0.f));
        draw::brackets(window, chip, 6.f, 1.5f, withAlpha(theme::accent, 0.55f + 0.45f * comboPop_));
        drawLabel(window, *font_, "dmg", 11, {chip.left + 10.f, chip.top + chip.height * 0.5f},
                  withAlpha(theme::accent, 0.7f), -1);
        char val[16];
        std::snprintf(val, sizeof(val), "x%.1f", comboMul_);
        sf::Text combo = makeText(*font_, val, 20, theme::accent);
        const sf::FloatRect cb = combo.getLocalBounds();
        combo.setOrigin(cb.left, cb.top + cb.height * 0.5f);
        const float cs = 1.f + 0.22f * comboPop_;
        combo.setScale(cs, cs);
        combo.setPosition(chip.left + 50.f, std::round(chip.top + chip.height * 0.5f));
        window.draw(combo);
    }

    // Run score and gold, top-right, as captioned readouts.
    const float right = size_.x - mg;
    readout(window, *font_, "score", std::to_string(score_), 18, right, mg + 6.f, theme::textDim, theme::textHi);
    readout(window, *font_, "gold", std::to_string(static_cast<int>(std::lround(goldShown_))), theme::fsBody, right,
            mg + 30.f, withAlpha(theme::puGolden, 0.6f), theme::puGolden, 1.f + 0.3f * goldPop_);

    // "Stockpile" reserve power-up, under the gold.
    if (hasReserve_) {
        const sf::Color col = powerUpColor(reservePu_);
        const float cy = mg + 52.f;
        drawLabel(window, *font_, powerUpName(reservePu_), 11, {right, cy}, col, 1);
        const float lw = makeLabel(*font_, powerUpName(reservePu_), 11, col).getLocalBounds().width;
        const sf::FloatRect key{right - lw - 28.f, cy - 9.f, 18.f, 18.f};   // a [Q] key cap
        draw::box(window, key, 0.f, withAlpha(col, 0.12f), withAlpha(col, 0.04f), withAlpha(col, 0.7f), 1.f);
        drawLabel(window, *font_, "q", 11, {key.left + key.width * 0.5f + 1.f, cy}, col);
    }

    // Stage readout, top centre: "ACT 1  STAGE 3 / 15" over the core gauge.
    {
        char stage[32];
        if (bossWave_) std::snprintf(stage, sizeof(stage), "%s", act_ == 1 ? "miniboss" : "final boss");
        else std::snprintf(stage, sizeof(stage), "stage %d / %d", stage_, stages_);
        const std::string act = "act " + std::to_string(act_);
        sf::Text at = makeLabel(*font_, act, 12, theme::textLo);
        sf::Text st = makeLabel(*font_, stage, 16, bossWave_ ? theme::coreLow : theme::textHi);
        const float aw = at.getLocalBounds().width, sw = st.getLocalBounds().width, gap = 16.f;
        const float x0 = size_.x * 0.5f - (aw + gap + sw) * 0.5f;
        drawLabel(window, *font_, act, 12, {x0, mg + 4.f}, theme::textLo, -1);
        drawLabel(window, *font_, stage, 16, {x0 + aw + gap, mg + 3.f}, bossWave_ ? theme::coreLow : theme::textHi, -1);
    }

    const float barW = 280.f;
    const float x = size_.x * 0.5f - barW * 0.5f;
    const float y = mg + 20.f;
    gauge(window, x, y, barW, 6.f, coreFrac_, lerpColor(theme::coreLow, theme::core, coreFrac_), 1.f, 20);
    drawLabel(window, *font_, std::to_string(enemiesLeft_) + " left", 11, {size_.x * 0.5f, y + 20.f}, theme::textLo);

    // Active power-up bar, a bit lower so it clears the banner.
    if (effectAlpha_ > 0.01f && effect_) {
        const float w = 172.f;
        const float px = size_.x * 0.5f - w / 2.f;
        const float py = mg + 64.f;
        const sf::Color col = powerUpColor(effect_->kind);
        const float frac = clampf(effect_->remaining / effect_->duration, 0.f, 1.f);
        drawLabel(window, *font_, powerUpName(effect_->kind), 11, {size_.x * 0.5f, py - 4.f},
                  withAlpha(col, effectAlpha_));
        gauge(window, px, py + 8.f, w, 3.f, frac, col, effectAlpha_, 1);
    }
}

}  // namespace sb
