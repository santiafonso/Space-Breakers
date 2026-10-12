#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "core/Math.hpp"
#include "sim/Entities.hpp"
#include "progression/Ascension.hpp"

namespace sb {

// Combat HUD: the damage-combo chip, the stage banner with its progress bar,
// gold and the active power-up bar. Nothing else - the core's health is its
// own dial, lifetime numbers live on the Stats screen.
class Hud {
public:
    void init(const sf::Font& font, sf::Vector2f size);
    // act / stage: where you are on the path map (stage = map row, stages
    // counts the boss row).
    void setHard(bool on) { hard_ = on; }
    // Ascension: the run's level, this elite fight's trait, the boss's rules (BossRule bits).
    void setAscension(int level, EliteTrait trait, unsigned bossRules) { asc_ = level; trait_ = trait; bossRules_ = bossRules; }
    // The boss's health for the top bar (frac < 0 = no boss on the field).
    void setBoss(float frac) { bossFrac_ = frac; }
    void setBossKind(BossKind k) { bossKind_ = k; }   // the act's boss, for its name
    void update(float dt, int act, int stage, int stages, int enemiesLeft, float coreFrac,
                float comboMultiplier, int score, int gold, const std::optional<ActiveEffect>& effect,
                bool bossWave, bool eliteWave = false);
    // The combo bar: its level and cap in tiers (+0.5x each), and the fuse
    // (1 right after a hit, 0 once it drains).
    void setCombo(float level, float cap, float fuse) { comboLevel_ = level; comboCap_ = cap; comboFuse_ = fuse; }
    void pulseCombo();
    void pulseGold();
    void draw(sf::RenderWindow& window) const;
    // Hover text for the HUD element under `mouse` (UI units). False if none.
    bool tooltipAt(sf::Vector2f mouse, std::string& title, std::string& desc, sf::Color& color) const;

private:
    const sf::Font* font_ = nullptr;
    sf::Vector2f size_;
    int act_ = 1;
    int stage_ = 0;
    int stages_ = 0;
    int enemiesLeft_ = 0;
    int enemiesPeak_ = 0;   // the stage's enemy count at its fullest: the progress bar's 100%
    int score_ = 0;
    int gold_ = 0;
    float goldPop_ = 0.f;
    float goldShown_ = 0.f;   // the counter rolls toward gold_ instead of jumping
    bool bossWave_ = false;
    bool hard_ = false;
    int asc_ = 0;
    EliteTrait trait_ = EliteTrait::None;
    unsigned bossRules_ = 0;
    float bossFrac_ = -1.f;
    BossKind bossKind_ = BossKind::Charger;
    bool eliteWave_ = false;
    float coreFrac_ = 1.f;
    float comboMul_ = 1.f;
    float comboPop_ = 0.f;
    float comboLevel_ = 0.f, comboCap_ = 8.f, comboFuse_ = 0.f;
    float comboShown_ = 0.f;   // the bar eases toward comboLevel_
    float time_ = 0.f;
    float effectAlpha_ = 0.f;
    std::optional<ActiveEffect> effect_;
};

}  // namespace sb
