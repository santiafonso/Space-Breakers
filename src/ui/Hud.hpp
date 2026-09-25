#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "core/Math.hpp"
#include "sim/Entities.hpp"

namespace sb {

// Combat HUD: the damage-combo chip, the wave / core-health banner and the
// active power-up bar. Nothing else - lifetime numbers live on the Stats screen.
class Hud {
public:
    void init(const sf::Font& font, sf::Vector2f size);
    // act / stage: where you are on the path map (stage = map row, stages
    // counts the boss row).
    void update(float dt, int act, int stage, int stages, int enemiesLeft, float coreFrac,
                float comboMultiplier, int score, int gold, const std::optional<ActiveEffect>& effect,
                bool bossWave, bool hasReserve, PowerUp reservePu);
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
    int score_ = 0;
    int gold_ = 0;
    float goldPop_ = 0.f;
    float goldShown_ = 0.f;   // the counter rolls toward gold_ instead of jumping
    bool bossWave_ = false;
    float coreFrac_ = 1.f;
    float comboMul_ = 1.f;
    float comboPop_ = 0.f;
    float effectAlpha_ = 0.f;
    std::optional<ActiveEffect> effect_;
    bool hasReserve_ = false;
    PowerUp reservePu_ = PowerUp::Points2x;
};

}  // namespace sb
