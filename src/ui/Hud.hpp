#pragma once

#include <cstdint>
#include <optional>

#include "core/Math.hpp"
#include "sim/Entities.hpp"

namespace sb {

// Combat HUD: the damage-combo chip, the wave / core-health banner and the
// active power-up bar. Nothing else - lifetime numbers live on the Stats screen.
class Hud {
public:
    void init(const sf::Font& font, sf::Vector2f size);
    void update(float dt, int wave, int finalWave, int enemiesLeft, float coreFrac,
                float comboMultiplier, const std::optional<ActiveEffect>& effect, bool bossWave);
    void pulseCombo();
    void draw(sf::RenderWindow& window) const;

private:
    const sf::Font* font_ = nullptr;
    sf::Vector2f size_;
    int wave_ = 0;
    int finalWave_ = 0;
    int enemiesLeft_ = 0;
    bool bossWave_ = false;
    float coreFrac_ = 1.f;
    float comboMul_ = 1.f;
    float comboPop_ = 0.f;
    float effectAlpha_ = 0.f;
    std::optional<ActiveEffect> effect_;
};

}  // namespace sb
