#pragma once

#include <cstdint>
#include <optional>

#include "core/Math.hpp"
#include "sim/Entities.hpp"

namespace sb {

// The in-game heads-up display: score, combo multiplier and the active
// power-up bar. Nothing else - the rest lives on the Stats screen.
class Hud {
public:
    void init(const sf::Font& font, sf::Vector2f size);
    void update(float dt, std::uint32_t score, std::uint32_t bestScore, float comboMultiplier,
                const std::optional<ActiveEffect>& effect);
    void pulseCombo();
    void draw(sf::RenderWindow& window) const;

private:
    const sf::Font* font_ = nullptr;
    sf::Vector2f size_;
    std::uint32_t score_ = 0;
    std::uint32_t best_ = 0;
    float scorePop_ = 0.f;
    float comboMul_ = 1.f;
    float comboPop_ = 0.f;
    float effectAlpha_ = 0.f;
    std::optional<ActiveEffect> effect_;
};

}  // namespace sb
