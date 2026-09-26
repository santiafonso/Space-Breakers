#include "render/Backdrop.hpp"

#include <cmath>

#include "core/Math.hpp"
#include "core/Theme.hpp"
#include "render/Draw.hpp"

namespace sb::backdrop {

namespace {
constexpr float kCell = 40.f;   // grid pitch in UI pixels
}

void draw(sf::RenderTarget& t, sf::Vector2f size) {
    static sf::VertexArray dots(sf::Quads);
    static sf::Vector2f builtFor{-1.f, -1.f};
    if (builtFor != size) {
        builtFor = size;
        dots.clear();
        const sf::Vector2f c = size * 0.5f;
        // Centred on the screen so the grid sits symmetric around the core.
        const int nx = static_cast<int>(c.x / kCell), ny = static_cast<int>(c.y / kCell);
        for (int j = -ny; j <= ny; ++j)
            for (int i = -nx; i <= nx; ++i) {
                // Every fourth crossing is a slightly stronger, bigger mark.
                const bool major = i % 4 == 0 && j % 4 == 0;
                const float h = major ? 1.5f : 1.f;
                const sf::Color col = withAlpha(theme::grid, major ? 0.16f : 0.08f);
                const float x = c.x + static_cast<float>(i) * kCell;
                const float y = c.y + static_cast<float>(j) * kCell;
                dots.append({{x - h, y - h}, col});
                dots.append({{x + h, y - h}, col});
                dots.append({{x + h, y + h}, col});
                dots.append({{x - h, y + h}, col});
            }
    }
    t.draw(dots);
    draw::vignette(t, size, theme::bgDeep, 0.85f);
}

}  // namespace sb::backdrop
