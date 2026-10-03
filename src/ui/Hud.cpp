#include "ui/Hud.hpp"

#include <algorithm>
#include <cctype>
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
                 bool bossWave, bool eliteWave) {
    eliteWave_ = eliteWave && !bossWave;
    if (act != act_ || stage != stage_ || bossWave != bossWave_) enemiesPeak_ = 0;
    act_ = act;
    stage_ = stage;
    stages_ = stages;
    bossWave_ = bossWave;
    enemiesLeft_ = enemiesLeft;
    enemiesPeak_ = std::max(enemiesPeak_, enemiesLeft);
    score_ = score;
    gold_ = gold;
    coreFrac_ = clampf(coreFrac, 0.f, 1.f);
    comboMul_ = comboMultiplier;
    effect_ = effect;

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
    if (sf::FloatRect(size_.x - mg - 130.f, mg - 8.f, 136.f, 30.f).contains(m)) {
        title = "Gold";
        desc = "earned from fights; spend it in shops on the map";
        color = theme::puGolden;
        return true;
    }
    if (effectAlpha_ > 0.5f && effect_ &&
        sf::FloatRect(size_.x * 0.5f - 110.f, mg + 40.f, 220.f, 34.f).contains(m)) {
        title = powerUpName(effect_->kind);
        desc = powerUpDesc(effect_->kind);
        color = powerUpColor(effect_->kind);
        return true;
    }
    if (sf::FloatRect(size_.x * 0.5f - 170.f, mg - 10.f, 340.f, 44.f).contains(m)) {
        title = bossWave_ ? std::string(bossName(bossOfAct(act_)))
                          : (eliteWave_ ? "Elite fight" : "Stage " + std::to_string(stage_) + " of " + std::to_string(stages_));
        if (bossWave_) {
            desc = std::string(bossDesc(bossOfAct(act_))) + ".  The red bar is its health; past the notch it enrages.";
            return true;
        }
        if (eliteWave_) {
            desc = "tougher and more enemies - clear it for extra gold and an item pick (items only come from elites "
                   "and shops). " + std::to_string(enemiesLeft_) + " left.";
            return true;
        }
        desc = std::to_string(enemiesLeft_) + (enemiesLeft_ == 1 ? " enemy" : " enemies") +
               " left - the bar fills as you clear the fight. Keep them off the core: its ring is its health.";
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

    // Gold, top-right: the one run number worth watching mid-fight.
    const float right = size_.x - mg;
    readout(window, *font_, "gold", std::to_string(static_cast<int>(std::lround(goldShown_))), 22, right,
            mg + 6.f, withAlpha(theme::puGolden, 0.55f), theme::puGolden, 1.f + 0.3f * goldPop_);

    // Stage readout, top centre: "ACT 1   STAGE 3 / 15" over the stage's
    // progress (enemies cleared). How many are left is on hover.
    {
        char stage[32];
        if (bossWave_) {
            std::string n = bossName(bossOfAct(act_));
            for (char& ch : n) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            std::snprintf(stage, sizeof(stage), "%s", n.c_str());
        }
        else if (eliteWave_) std::snprintf(stage, sizeof(stage), "elite  %d / %d", stage_, stages_);
        else std::snprintf(stage, sizeof(stage), "stage %d / %d", stage_, stages_);
        const std::string act = "act " + std::to_string(act_) + (hard_ ? "  hard" : "");
        const sf::Color stCol = bossWave_ ? theme::coreLow : (eliteWave_ ? theme::ember : theme::textHi);
        const float aw = makeLabel(*font_, act, 13, theme::textLo).getLocalBounds().width;
        const float sw = makeLabel(*font_, stage, 18, stCol).getLocalBounds().width, gap = 18.f;
        const float x0 = size_.x * 0.5f - (aw + gap + sw) * 0.5f;
        drawLabel(window, *font_, act, 13, {x0, mg + 4.f}, theme::textDim, -1);
        drawLabel(window, *font_, stage, 18, {x0 + aw + gap, mg + 3.f}, stCol, -1);
    }
    if (bossWave_ && bossFrac_ >= 0.f) {
        // A boss: its health, big and plain, cut in tenths, with a notch
        // where it enrages.
        const float barW = 420.f, x = size_.x * 0.5f - barW * 0.5f, y = mg + 26.f;
        gauge(window, x, y, barW, 9.f, bossFrac_, theme::coreLow, 0.95f, 10);
        const float nx = std::round(x + barW * cfg::boss::enrageAt);
        draw::line(window, {nx, y - 4.f}, {nx, y + 13.f}, 2.f, withAlpha(theme::textHi, 0.6f));
    } else {
        // A fight: how much of it is left - the bar fills as you clear it.
        const float barW = 300.f, x = size_.x * 0.5f - barW * 0.5f, y = mg + 26.f;
        const float done = enemiesPeak_ > 0
                               ? 1.f - static_cast<float>(enemiesLeft_) / static_cast<float>(enemiesPeak_)
                               : 0.f;
        const sf::Color col = eliteWave_ ? theme::ember : theme::accent;
        gauge(window, x, y, barW, 6.f, done, col, 0.9f, 1);
        if (enemiesLeft_ > 0)
            drawLabel(window, *font_, std::to_string(enemiesLeft_) + " left", 11, {x + barW + 14.f, y + 3.f},
                      withAlpha(theme::textLo, 0.9f), -1);
    }

    // Active power-up bar, a bit lower so it clears the banner.
    if (effectAlpha_ > 0.01f && effect_) {
        const float w = 172.f;
        const float px = size_.x * 0.5f - w / 2.f;
        const float py = mg + 52.f;
        const sf::Color col = powerUpColor(effect_->kind);
        const float frac = clampf(effect_->remaining / effect_->duration, 0.f, 1.f);
        drawLabel(window, *font_, powerUpName(effect_->kind), 12, {size_.x * 0.5f, py - 4.f},
                  withAlpha(col, effectAlpha_));
        gauge(window, px, py + 8.f, w, 3.f, frac, col, effectAlpha_, 1);
    }
}

}  // namespace sb
