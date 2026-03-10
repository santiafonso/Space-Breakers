#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {
    constexpr float radius = 15.f;
    const char* saveDir = "saves";
    const char* savePath = "saves/save.dat";

    enum class GameState { MainMenu, PlayMenu, Options, Playing, Shop, PauseMenu };

    struct BallData {
        float x;
        float y;
        float vx;
        float vy;
    };

    struct SaveHeader {
        uint32_t version;
        uint32_t points;
        uint32_t totalBounces;
        uint32_t speedLevel;
        uint32_t pointsLevel;
        uint32_t multiballLevel;
        uint32_t ballCount;
    };

    struct FloatingText {
        sf::Text text;
        float lifetime = 0.f;
        sf::Vector2f velocity;
    };

    struct Ball {
        sf::CircleShape shape;
        sf::Vector2f velocity;
        bool isDragging = false;
        sf::Vector2f dragOffset{0.f, 0.f};
        sf::Vector2f storedVelocity{0.f, 0.f};
        sf::Vector2f lastDragVelocity{0.f, 0.f};
        sf::Vector2f lastDragMousePos{0.f, 0.f};
        bool hasLastDragMousePos = false;

        Ball() : shape(radius) {
            shape.setOutlineThickness(2.f);
            shape.setFillColor(sf::Color(80, 170, 255));
            shape.setOutlineColor(sf::Color::White);
        }
    };

    float magnitude(const sf::Vector2f& v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    sf::Vector2f normalizeOrDefault(const sf::Vector2f& v, const sf::Vector2f& fallback) {
        const float len = magnitude(v);
        if (len < 0.0001f) return fallback;
        return v / len;
    }

    uint32_t upgradeCost(uint32_t baseCost, uint32_t level, float growth) {
        return static_cast<uint32_t>(std::round(baseCost * std::pow(growth, static_cast<float>(level))));
    }

    bool hasSavedGame() {
        std::ifstream f(savePath, std::ios::binary);
        return f.good();
    }

    bool saveGame(uint32_t points,
                  uint32_t totalBounces,
                  uint32_t speedLevel,
                  uint32_t pointsLevel,
                  uint32_t multiballLevel,
                  const std::vector<Ball>& balls) {
        std::filesystem::create_directories(saveDir);
        std::ofstream f(savePath, std::ios::binary);
        if (!f) return false;

        SaveHeader header{};
        header.version = 2;
        header.points = points;
        header.totalBounces = totalBounces;
        header.speedLevel = speedLevel;
        header.pointsLevel = pointsLevel;
        header.multiballLevel = multiballLevel;
        header.ballCount = static_cast<uint32_t>(balls.size());

        f.write(reinterpret_cast<const char*>(&header), sizeof(header));
        for (const Ball& ball : balls) {
            const sf::Vector2f pos = ball.shape.getPosition();
            const BallData bd{pos.x, pos.y, ball.velocity.x, ball.velocity.y};
            f.write(reinterpret_cast<const char*>(&bd), sizeof(bd));
        }

        return f.good();
    }

    bool loadGame(uint32_t& points,
                  uint32_t& totalBounces,
                  uint32_t& speedLevel,
                  uint32_t& pointsLevel,
                  uint32_t& multiballLevel,
                  std::vector<Ball>& balls) {
        std::ifstream f(savePath, std::ios::binary);
        if (!f) return false;

        SaveHeader header{};
        f.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!f || header.version != 2 || header.ballCount == 0 || header.ballCount > 32) return false;

        points = header.points;
        totalBounces = header.totalBounces;
        speedLevel = header.speedLevel;
        pointsLevel = header.pointsLevel;
        multiballLevel = header.multiballLevel;

        balls.clear();
        balls.reserve(header.ballCount);
        for (uint32_t i = 0; i < header.ballCount; ++i) {
            BallData bd{};
            f.read(reinterpret_cast<char*>(&bd), sizeof(bd));
            if (!f) return false;
            Ball b;
            b.shape.setPosition(bd.x, bd.y);
            b.velocity = {bd.vx, bd.vy};
            balls.push_back(b);
        }
        return true;
    }

    void centerText(sf::Text& text, float x, float y) {
        const sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f,
                       bounds.top + bounds.height / 2.f);
        text.setPosition(x, y);
    }

    bool isMouseOver(const sf::Text& text, const sf::RenderWindow& window) {
        const sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        return text.getGlobalBounds().contains(mouse);
    }
}

int main() {
    const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    const unsigned windowWidth = std::max(960u, std::min(1280u, desktop.width > 80 ? desktop.width - 80 : desktop.width));
    const unsigned windowHeight = std::max(540u, std::min(720u, desktop.height > 120 ? desktop.height - 120 : desktop.height));

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Space-Breakers v2", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> angleDist(-0.22f, 0.22f);
    std::uniform_real_distribution<float> sideDist(-1.f, 1.f);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Could not load font (arial.ttf)\n";
        return 1;
    }

    auto makeCenteredText = [&](const std::string& str, unsigned size, float y) {
        sf::Text t(str, font, size);
        t.setFillColor(sf::Color::White);
        centerText(t, windowWidth / 2.f, y);
        return t;
    };

    auto recenterMenu = [&](std::vector<sf::Text*> texts) {
        for (sf::Text* text : texts) {
            centerText(*text, windowWidth / 2.f, text->getPosition().y);
        }
    };

    GameState state = GameState::MainMenu;
    GameState optionsReturnState = GameState::MainMenu;

    const float titleY = windowHeight * 0.17f;
    const float menuY1 = windowHeight * 0.38f;
    const float menuGap = windowHeight * 0.08f;

    sf::Text titleMain = makeCenteredText("Space-Breakers", 44, titleY);
    sf::Text playItem = makeCenteredText("Play", 34, menuY1);
    sf::Text optionsItem = makeCenteredText("How to play", 34, menuY1 + menuGap);
    sf::Text exitItem = makeCenteredText("Quit", 34, menuY1 + menuGap * 2.f);

    sf::Text titlePlay = makeCenteredText("Play", 42, titleY);
    sf::Text newGameItem = makeCenteredText("New Game", 34, menuY1);
    sf::Text continueItem = makeCenteredText("Continue", 34, menuY1 + menuGap);
    sf::Text backPlayItem = makeCenteredText("Back", 30, windowHeight * 0.73f);

    sf::Text titleOpt = makeCenteredText("How to play", 42, windowHeight * 0.15f);
    sf::Text opt1 = makeCenteredText("- Click anywhere to grab every ball and drag them", 22, windowHeight * 0.32f);
    sf::Text opt2 = makeCenteredText("- Each bounce gives points", 22, windowHeight * 0.37f);
    sf::Text opt3 = makeCenteredText("- Press TAB to open upgrades", 22, windowHeight * 0.42f);
    sf::Text opt4 = makeCenteredText("- Press ESC to open pause menu", 22, windowHeight * 0.47f);
    sf::Text backOptItem = makeCenteredText("Back", 30, windowHeight * 0.68f);

    sf::Text pauseTitle = makeCenteredText("Paused", 44, windowHeight * 0.17f);
    sf::Text resumeItem = makeCenteredText("Resume", 34, menuY1);
    sf::Text pauseOptionsItem = makeCenteredText("How to play", 34, menuY1 + menuGap);
    sf::Text saveGameItem = makeCenteredText("Save Game", 34, menuY1 + menuGap * 2.f);
    sf::Text backToMenuItem = makeCenteredText("Back to Main Menu", 34, menuY1 + menuGap * 3.f);
    sf::Text quitFromPauseItem = makeCenteredText("Quit Game", 34, menuY1 + menuGap * 4.f);

    sf::Text shopTitle = makeCenteredText("Upgrades", 42, windowHeight * 0.13f);
    sf::Text shopBack = makeCenteredText("Back to Game", 30, windowHeight * 0.83f);
    sf::Text upgradeSpeed("", font, 26);
    sf::Text upgradePoints("", font, 26);
    sf::Text upgradeMultiball("", font, 26);
    sf::Text shopInfo("Click to buy  |  TAB returns to play", font, 18);
    shopInfo.setFillColor(sf::Color(180, 180, 180));
    centerText(upgradeSpeed, windowWidth / 2.f, windowHeight * 0.36f);
    centerText(upgradePoints, windowWidth / 2.f, windowHeight * 0.52f);
    centerText(upgradeMultiball, windowWidth / 2.f, windowHeight * 0.68f);
    centerText(shopInfo, windowWidth / 2.f, windowHeight * 0.92f);

    sf::Text hudPoints("", font, 22);
    sf::Text hudBounces("", font, 22);
    hudPoints.setPosition(12.f, 8.f);
    hudBounces.setPosition(12.f, 36.f);

    sf::RectangleShape overlayDim(sf::Vector2f(static_cast<float>(windowWidth), static_cast<float>(windowHeight)));
    overlayDim.setFillColor(sf::Color(0, 0, 0, 160));

    std::vector<Ball> balls;
    std::vector<FloatingText> floatingTexts;
    uint32_t points = 0;
    uint32_t totalBounces = 0;
    uint32_t speedLevel = 0;
    uint32_t pointsLevel = 0;
    uint32_t multiballLevel = 0;

    constexpr float baseCruiseSpeed = 300.f;
    constexpr float cruiseAdjustRate = 2.2f;
    constexpr float throwBoostFactor = 1.75f;

    auto currentCruiseSpeed = [&]() {
        return baseCruiseSpeed * std::pow(1.18f, static_cast<float>(speedLevel));
    };

    auto pointsPerBounce = [&]() {
        return 1u + pointsLevel;
    };

    auto lerpColor = [](const sf::Color& a, const sf::Color& b, float t) {
        t = std::clamp(t, 0.f, 1.f);
        auto lerp = [&](sf::Uint8 x, sf::Uint8 y) -> sf::Uint8 {
            return static_cast<sf::Uint8>(x + (y - x) * t);
        };
        return sf::Color(lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a));
    };

    auto recolorBall = [&](Ball& ball) {
        const float speed = magnitude(ball.velocity);
        const sf::Color slowFill(80, 170, 255);
        const sf::Color midFill(120, 255, 170);
        const sf::Color fastFill(255, 220, 90);
        const sf::Color ultraFill(255, 90, 90);

        sf::Color fill;
        sf::Color outline;
        if (speed < 220.f) {
            const float t = speed / 220.f;
            fill = lerpColor(slowFill, midFill, t);
            outline = lerpColor(sf::Color::White, sf::Color(180, 255, 220), t);
        } else if (speed < 420.f) {
            const float t = (speed - 220.f) / 200.f;
            fill = lerpColor(midFill, fastFill, t);
            outline = lerpColor(sf::Color(180, 255, 220), sf::Color::Yellow, t);
        } else {
            const float t = std::min(1.f, (speed - 420.f) / 300.f);
            fill = lerpColor(fastFill, ultraFill, t);
            outline = lerpColor(sf::Color::Yellow, sf::Color::Red, t);
        }
        ball.shape.setFillColor(fill);
        ball.shape.setOutlineColor(outline);
    };

    auto spawnFloatingText = [&](const std::string& value, sf::Vector2f pos) {
        FloatingText ft;
        ft.text.setFont(font);
        ft.text.setCharacterSize(18);
        ft.text.setString(value);
        ft.text.setFillColor(sf::Color::Yellow);
        centerText(ft.text, pos.x, pos.y);
        ft.lifetime = 0.8f;
        ft.velocity = {0.f, -38.f};
        floatingTexts.push_back(ft);
    };

    auto createBall = [&](sf::Vector2f center, sf::Vector2f vel) {
        Ball b;
        b.shape.setPosition(center.x - radius, center.y - radius);
        b.velocity = vel;
        recolorBall(b);
        balls.push_back(b);
    };

    auto clampBallInside = [&](Ball& ball) {
        sf::Vector2f pos = ball.shape.getPosition();
        pos.x = std::clamp(pos.x, 0.f, static_cast<float>(windowWidth) - 2.f * radius);
        pos.y = std::clamp(pos.y, 0.f, static_cast<float>(windowHeight) - 2.f * radius);
        ball.shape.setPosition(pos);
    };

    auto syncBallCountWithUpgrade = [&]() {
        const std::size_t targetCount = 1u + multiballLevel;
        while (balls.size() < targetCount) {
            const sf::Vector2f basePos(
                windowWidth / 2.f + 28.f * static_cast<float>(balls.size()),
                windowHeight / 2.f + 18.f * static_cast<float>(balls.size())
            );
            const sf::Vector2f dir = normalizeOrDefault({sideDist(rng), sideDist(rng)}, {1.f, 0.3f});
            createBall(basePos, dir * currentCruiseSpeed());
        }
    };

    auto resetGame = [&]() {
        points = 0;
        totalBounces = 0;
        speedLevel = 0;
        pointsLevel = 0;
        multiballLevel = 0;
        balls.clear();
        floatingTexts.clear();
        createBall({windowWidth / 2.f, windowHeight / 2.f}, {250.f, 180.f});
    };

    auto tryLoadGame = [&]() {
        if (!loadGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls)) {
            resetGame();
            return;
        }
        floatingTexts.clear();
        syncBallCountWithUpgrade();
        for (Ball& ball : balls) {
            clampBallInside(ball);
            recolorBall(ball);
        }
    };

    auto updateGameplay = [&](float dt) {
        for (Ball& ball : balls) {
            if (ball.isDragging) {
                const sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (ball.hasLastDragMousePos && dt > 0.f) {
                    const sf::Vector2f delta = mousePos - ball.lastDragMousePos;
                    ball.lastDragVelocity = delta / dt;
                }
                ball.lastDragMousePos = mousePos;
                ball.hasLastDragMousePos = true;

                sf::Vector2f desiredPos = mousePos + ball.dragOffset;
                desiredPos.x = std::clamp(desiredPos.x, 0.f, static_cast<float>(windowWidth) - 2.f * radius);
                desiredPos.y = std::clamp(desiredPos.y, 0.f, static_cast<float>(windowHeight) - 2.f * radius);
                ball.shape.setPosition(desiredPos);
                recolorBall(ball);
                continue;
            }

            ball.shape.move(ball.velocity * dt);
            clampBallInside(ball);

            sf::Vector2f pos = ball.shape.getPosition();
            bool bounced = false;

            if (pos.x <= 0.f) {
                pos.x = 0.f;
                ball.velocity.x = std::abs(ball.velocity.x);
                bounced = true;
            } else if (pos.x + 2.f * radius >= static_cast<float>(windowWidth)) {
                pos.x = static_cast<float>(windowWidth) - 2.f * radius;
                ball.velocity.x = -std::abs(ball.velocity.x);
                bounced = true;
            }

            if (pos.y <= 0.f) {
                pos.y = 0.f;
                ball.velocity.y = std::abs(ball.velocity.y);
                bounced = true;
            } else if (pos.y + 2.f * radius >= static_cast<float>(windowHeight)) {
                pos.y = static_cast<float>(windowHeight) - 2.f * radius;
                ball.velocity.y = -std::abs(ball.velocity.y);
                bounced = true;
            }

            ball.shape.setPosition(pos);

            if (bounced) {
                ++totalBounces;
                points += pointsPerBounce();
                spawnFloatingText("+" + std::to_string(pointsPerBounce()), pos + sf::Vector2f(radius, 0.f));

                const float speed = magnitude(ball.velocity);
                const float angle = std::atan2(ball.velocity.y, ball.velocity.x) + angleDist(rng);
                ball.velocity.x = speed * std::cos(angle);
                ball.velocity.y = speed * std::sin(angle);
            }

            const float speed = magnitude(ball.velocity);
            if (speed > 0.001f) {
                const float factor = std::min(1.f, cruiseAdjustRate * dt);
                const float newSpeed = speed + (currentCruiseSpeed() - speed) * factor;
                ball.velocity *= newSpeed / speed;
            } else {
                const sf::Vector2f dir = normalizeOrDefault({sideDist(rng), sideDist(rng)}, {1.f, 0.2f});
                ball.velocity = dir * currentCruiseSpeed();
            }
            recolorBall(ball);
        }

        for (std::size_t i = 0; i < balls.size(); ++i) {
            for (std::size_t j = i + 1; j < balls.size(); ++j) {
                const sf::Vector2f a = balls[i].shape.getPosition() + sf::Vector2f(radius, radius);
                const sf::Vector2f b = balls[j].shape.getPosition() + sf::Vector2f(radius, radius);
                const sf::Vector2f delta = b - a;
                const float dist = magnitude(delta);
                const float minDist = 2.f * radius;
                if (dist > 0.f && dist < minDist) {
                    const sf::Vector2f normal = delta / dist;
                    const float overlap = minDist - dist;
                    balls[i].shape.move(-normal * overlap * 0.5f);
                    balls[j].shape.move(normal * overlap * 0.5f);
                    clampBallInside(balls[i]);
                    clampBallInside(balls[j]);
                    std::swap(balls[i].velocity, balls[j].velocity);
                }
            }
        }

        for (auto it = floatingTexts.begin(); it != floatingTexts.end();) {
            it->lifetime -= dt;
            it->text.move(it->velocity * dt);
            sf::Color c = it->text.getFillColor();
            c.a = static_cast<sf::Uint8>(255.f * std::max(0.f, it->lifetime / 0.8f));
            it->text.setFillColor(c);
            if (it->lifetime <= 0.f) it = floatingTexts.erase(it);
            else ++it;
        }
    };

    resetGame();

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                window.close();
                break;
            }

            if (state == GameState::MainMenu) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(playItem, window)) state = GameState::PlayMenu;
                    else if (isMouseOver(optionsItem, window)) { optionsReturnState = GameState::MainMenu; state = GameState::Options; }
                    else if (isMouseOver(exitItem, window)) window.close();
                }
            } else if (state == GameState::PlayMenu) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(newGameItem, window)) {
                        resetGame();
                        saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                        state = GameState::Playing;
                    } else if (isMouseOver(continueItem, window) && hasSavedGame()) {
                        tryLoadGame();
                        state = GameState::Playing;
                    } else if (isMouseOver(backPlayItem, window)) {
                        state = GameState::MainMenu;
                    }
                }
            } else if (state == GameState::Options) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(backOptItem, window)) state = optionsReturnState;
                }
            } else if (state == GameState::Shop) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab) state = GameState::Playing;
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    const uint32_t speedCost = upgradeCost(15, speedLevel, 1.55f);
                    const uint32_t pointsCost = upgradeCost(25, pointsLevel, 1.70f);
                    const uint32_t multiballCost = upgradeCost(80, multiballLevel, 2.30f);
                    if (isMouseOver(shopBack, window)) state = GameState::Playing;
                    else if (isMouseOver(upgradeSpeed, window) && points >= speedCost) { points -= speedCost; ++speedLevel; }
                    else if (isMouseOver(upgradePoints, window) && points >= pointsCost) { points -= pointsCost; ++pointsLevel; }
                    else if (isMouseOver(upgradeMultiball, window) && points >= multiballCost) { points -= multiballCost; ++multiballLevel; syncBallCountWithUpgrade(); }
                }
            } else if (state == GameState::PauseMenu) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) state = GameState::Playing;
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(resumeItem, window)) state = GameState::Playing;
                    else if (isMouseOver(pauseOptionsItem, window)) { optionsReturnState = GameState::PauseMenu; state = GameState::Options; }
                    else if (isMouseOver(saveGameItem, window)) {
                        saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                    }
                    else if (isMouseOver(backToMenuItem, window)) {
                        saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                        state = GameState::MainMenu;
                    }
                    else if (isMouseOver(quitFromPauseItem, window)) window.close();
                }
            } else if (state == GameState::Playing) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab) state = GameState::Shop;
                else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) state = GameState::PauseMenu;

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    const sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    const float clampedX = std::clamp(mousePos.x, radius, static_cast<float>(windowWidth) - radius);
                    const float clampedY = std::clamp(mousePos.y, radius, static_cast<float>(windowHeight) - radius);
                    const sf::Vector2f snappedTopLeft{clampedX - radius, clampedY - radius};
                    for (Ball& ball : balls) {
                        ball.shape.setPosition(snappedTopLeft);
                        ball.isDragging = true;
                        ball.storedVelocity = ball.velocity;
                        ball.velocity = {0.f, 0.f};
                        ball.dragOffset = ball.shape.getPosition() - mousePos;
                        ball.lastDragVelocity = {0.f, 0.f};
                        ball.lastDragMousePos = mousePos;
                        ball.hasLastDragMousePos = false;
                        recolorBall(ball);
                    }
                }

                if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                    for (Ball& ball : balls) {
                        if (!ball.isDragging) continue;
                        ball.isDragging = false;
                        const float speed = magnitude(ball.lastDragVelocity);
                        ball.velocity = (speed > 10.f) ? ball.lastDragVelocity * throwBoostFactor : ball.storedVelocity;
                        ball.hasLastDragMousePos = false;
                        recolorBall(ball);
                    }
                }
            }
        }

        float dt = std::min(clock.restart().asSeconds(), 0.03f);
        if (state == GameState::Playing || state == GameState::Shop || state == GameState::PauseMenu) {
            updateGameplay(dt);
            hudPoints.setString("Points: " + std::to_string(points));
            hudBounces.setString("Total bounces: " + std::to_string(totalBounces));
        }

        const uint32_t speedCost = upgradeCost(15, speedLevel, 1.55f);
        const uint32_t pointsCost = upgradeCost(25, pointsLevel, 1.70f);
        const uint32_t multiballCost = upgradeCost(80, multiballLevel, 2.30f);

        upgradeSpeed.setString("[1] Speed level " + std::to_string(speedLevel) +
                               "  ->  +18% base speed\nCost: " + std::to_string(speedCost) + " pts");
        upgradePoints.setString("[2] Points per bounce level " + std::to_string(pointsLevel) +
                                "  ->  gain +1 extra point per bounce\nCost: " + std::to_string(pointsCost) + " pts");
        upgradeMultiball.setString("[3] Multiball level " + std::to_string(multiballLevel) +
                                   "  ->  add one new ball\nCost: " + std::to_string(multiballCost) + " pts");
        upgradeSpeed.setFillColor(points >= speedCost ? sf::Color::White : sf::Color(120, 120, 120));
        upgradePoints.setFillColor(points >= pointsCost ? sf::Color::White : sf::Color(120, 120, 120));
        upgradeMultiball.setFillColor(points >= multiballCost ? sf::Color::White : sf::Color(120, 120, 120));
        centerText(upgradeSpeed, windowWidth / 2.f, windowHeight * 0.36f);
        centerText(upgradePoints, windowWidth / 2.f, windowHeight * 0.52f);
        centerText(upgradeMultiball, windowWidth / 2.f, windowHeight * 0.68f);

        recenterMenu({&titleMain, &playItem, &optionsItem, &exitItem,
                      &titlePlay, &newGameItem, &continueItem, &backPlayItem,
                      &titleOpt, &opt1, &opt2, &opt3, &opt4, &backOptItem,
                      &pauseTitle, &resumeItem, &pauseOptionsItem, &saveGameItem, &backToMenuItem,
                      &quitFromPauseItem, &shopTitle, &shopBack, &shopInfo});

        window.clear(sf::Color(12, 12, 22));

        if (state == GameState::MainMenu) {
            window.draw(titleMain);
            window.draw(playItem);
            window.draw(optionsItem);
            window.draw(exitItem);
        } else if (state == GameState::PlayMenu) {
            continueItem.setFillColor(hasSavedGame() ? sf::Color::White : sf::Color(120, 120, 120));
            window.draw(titlePlay);
            window.draw(newGameItem);
            window.draw(continueItem);
            window.draw(backPlayItem);
        } else if (state == GameState::Options) {
            window.draw(titleOpt);
            window.draw(opt1);
            window.draw(opt2);
            window.draw(opt3);
            window.draw(opt4);
            window.draw(backOptItem);
        } else {
            for (const Ball& ball : balls) window.draw(ball.shape);
            for (const FloatingText& ft : floatingTexts) window.draw(ft.text);
            window.draw(hudPoints);
            window.draw(hudBounces);

            if (state == GameState::Shop) {
                sf::Text shopPoints("Available points: " + std::to_string(points), font, 28);
                shopPoints.setFillColor(sf::Color::Yellow);
                centerText(shopPoints, windowWidth / 2.f, windowHeight * 0.23f);
                window.draw(overlayDim);
                window.draw(shopTitle);
                window.draw(shopPoints);
                window.draw(upgradeSpeed);
                window.draw(upgradePoints);
                window.draw(upgradeMultiball);
                window.draw(shopBack);
                window.draw(shopInfo);
            } else if (state == GameState::PauseMenu) {
                window.draw(overlayDim);
                window.draw(pauseTitle);
                window.draw(resumeItem);
                window.draw(pauseOptionsItem);
                window.draw(saveGameItem);
                window.draw(backToMenuItem);
                window.draw(quitFromPauseItem);
            }
        }

        window.display();
    }

    saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
    return 0;
}
