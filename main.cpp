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
    const int windowWidth = 1920;
    const int windowHeight = 1080;
    const float radius = 15.f;
    const char* saveDir = "saves";
    const char* savePath = "saves/save_v2.dat";

    enum class GameState { MainMenu, PlayMenu, Options, Playing, Shop };

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
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineThickness(2.f);
            shape.setOutlineColor(sf::Color::Red);
        }
    };

    bool isMouseOver(const sf::Text& text, const sf::RenderWindow& window) {
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        return text.getGlobalBounds().contains(mouse);
    }

    float magnitude(const sf::Vector2f& v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    sf::Vector2f normalizeOrDefault(const sf::Vector2f& v, const sf::Vector2f& fallback) {
        float len = magnitude(v);
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
            sf::Vector2f pos = ball.shape.getPosition();
            BallData bd{pos.x, pos.y, ball.velocity.x, ball.velocity.y};
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
}

int main() {
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Space-Breakers v2");
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

    auto makeMenuText = [&](const std::string& str, unsigned size, float y) {
        sf::Text t(str, font, size);
        t.setFillColor(sf::Color::White);
        sf::FloatRect bounds = t.getLocalBounds();
        t.setPosition(windowWidth / 2.f - bounds.width / 2.f, y);
        return t;
    };

    auto makeButtonText = [&](const std::string& str, unsigned size, float x, float y) {
        sf::Text t(str, font, size);
        t.setFillColor(sf::Color::White);
        t.setPosition(x, y);
        return t;
    };

    GameState state = GameState::MainMenu;

    sf::Text titleMain = makeMenuText("Space-Breakers", 54, 120.f);
    sf::Text playItem = makeMenuText("Play", 36, 260.f);
    sf::Text optionsItem = makeMenuText("Options", 36, 320.f);
    sf::Text exitItem = makeMenuText("Quit", 36, 380.f);

    sf::Text titlePlay = makeMenuText("Play", 48, 120.f);
    sf::Text newGameItem = makeMenuText("New Game", 36, 260.f);
    sf::Text continueItem = makeMenuText("Continue", 36, 320.f);
    sf::Text backPlayItem = makeMenuText("Back", 28, 450.f);

    sf::Text titleOpt = makeMenuText("Options", 48, 110.f);
    sf::Text opt1 = makeMenuText("- Click anywhere to grab every ball and drag them", 24, 230.f);
    sf::Text opt2 = makeMenuText("- Each bounce gives points", 24, 270.f);
    sf::Text opt3 = makeMenuText("- Press ESC to open the upgrade screen", 24, 310.f);
    sf::Text opt4 = makeMenuText("- Buy upgrades to increase speed, score, and ball count", 24, 350.f);
    sf::Text backOptItem = makeMenuText("Back", 32, 470.f);

    sf::Text shopTitle = makeMenuText("Upgrades", 42, 60.f);
    sf::Text shopBack = makeButtonText("Back to Game", 30, 60.f, 610.f);
    sf::Text upgradeSpeed = makeButtonText("", 28, 80.f, 180.f);
    sf::Text upgradePoints = makeButtonText("", 28, 80.f, 290.f);
    sf::Text upgradeMultiball = makeButtonText("", 28, 80.f, 400.f);
    sf::Text shopInfo = makeButtonText("Click to buy  |  ESC or Back to Game returns to play", 20, 80.f, 520.f);
    shopInfo.setFillColor(sf::Color(180, 180, 180));

    sf::Text hudTopLeft("", font, 22);
    hudTopLeft.setPosition(12.f, 8.f);
    sf::Text hudSecondLine("", font, 22);
    hudSecondLine.setPosition(12.f, 36.f);
    sf::Text hudThirdLine("", font, 22);
    hudThirdLine.setPosition(12.f, 64.f);

    sf::Text shopButton("Upgrades", font, 22);
    shopButton.setPosition(windowWidth - 120.f, 10.f);
    shopButton.setFillColor(sf::Color(200, 200, 255));

    std::vector<Ball> balls;
    std::vector<FloatingText> floatingTexts;
    uint32_t points = 0;
    uint32_t totalBounces = 0;
    uint32_t speedLevel = 0;
    uint32_t pointsLevel = 0;
    uint32_t multiballLevel = 0;

    const float baseCruiseSpeed = 300.f;
    const float cruiseAdjustRate = 2.2f;
    const float throwBoostFactor = 1.75f;

    auto currentCruiseSpeed = [&]() {
        return baseCruiseSpeed * std::pow(1.18f, static_cast<float>(speedLevel));
    };

    auto pointsPerBounce = [&]() {
        return 1u + pointsLevel;
    };

    auto spawnFloatingText = [&](const std::string& value, sf::Vector2f pos) {
        FloatingText ft;
        ft.text.setFont(font);
        ft.text.setCharacterSize(20);
        ft.text.setString(value);
        ft.text.setFillColor(sf::Color::Yellow);
        ft.text.setPosition(pos);
        ft.lifetime = 0.9f;
        ft.velocity = {0.f, -40.f};
        floatingTexts.push_back(ft);
    };

    auto recolorBall = [&](Ball& ball) {
        float speed = magnitude(ball.velocity);
        if (speed < 260.f) ball.shape.setOutlineColor(sf::Color::Green);
        else if (speed < 420.f) ball.shape.setOutlineColor(sf::Color::Yellow);
        else ball.shape.setOutlineColor(sf::Color::Red);
    };

    auto createBall = [&](sf::Vector2f center, sf::Vector2f vel) {
        Ball b;
        b.shape.setPosition(center.x - radius, center.y - radius);
        b.velocity = vel;
        recolorBall(b);
        balls.push_back(b);
    };

    auto resetGame = [&]() {
        points = 0;
        totalBounces = 0;
        speedLevel = 0;
        pointsLevel = 0;
        multiballLevel = 0;
        floatingTexts.clear();
        balls.clear();
        createBall({windowWidth / 2.f, windowHeight / 2.f}, {250.f, 180.f});
    };

    auto syncBallCountWithUpgrade = [&]() {
        const std::size_t targetCount = 1u + multiballLevel;
        while (balls.size() < targetCount) {
            sf::Vector2f basePos(windowWidth / 2.f + 30.f * static_cast<float>(balls.size()),
                                 windowHeight / 2.f + 20.f * static_cast<float>(balls.size()));
            sf::Vector2f dir(normalizeOrDefault({sideDist(rng), sideDist(rng)}, {1.f, 0.3f}));
            createBall(basePos, dir * currentCruiseSpeed());
        }
    };

    auto tryLoadGame = [&]() {
        if (!loadGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls)) {
            resetGame();
            return;
        }
        floatingTexts.clear();
        syncBallCountWithUpgrade();
        for (Ball& b : balls) recolorBall(b);
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
                    else if (isMouseOver(optionsItem, window)) state = GameState::Options;
                    else if (isMouseOver(exitItem, window)) window.close();
                }
            } else if (state == GameState::PlayMenu) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(newGameItem, window)) {
                        resetGame();
                        saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                        state = GameState::Playing;
                    } else if (isMouseOver(continueItem, window)) {
                        tryLoadGame();
                        state = GameState::Playing;
                    } else if (isMouseOver(backPlayItem, window)) {
                        state = GameState::MainMenu;
                    }
                }
            } else if (state == GameState::Options) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(backOptItem, window)) state = GameState::MainMenu;
                }
            } else if (state == GameState::Shop) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                    saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                    state = GameState::Playing;
                }

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    uint32_t speedCost = upgradeCost(15, speedLevel, 1.55f);
                    uint32_t pointsCost = upgradeCost(25, pointsLevel, 1.70f);
                    uint32_t multiballCost = upgradeCost(80, multiballLevel, 2.30f);

                    if (isMouseOver(shopBack, window)) {
                        saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                        state = GameState::Playing;
                    } else if (isMouseOver(upgradeSpeed, window) && points >= speedCost) {
                        points -= speedCost;
                        ++speedLevel;
                    } else if (isMouseOver(upgradePoints, window) && points >= pointsCost) {
                        points -= pointsCost;
                        ++pointsLevel;
                    } else if (isMouseOver(upgradeMultiball, window) && points >= multiballCost) {
                        points -= multiballCost;
                        ++multiballLevel;
                        syncBallCountWithUpgrade();
                    }
                }
            } else if (state == GameState::Playing) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                    saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                    state = GameState::Shop;
                }

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(shopButton, window)) {
                        saveGame(points, totalBounces, speedLevel, pointsLevel, multiballLevel, balls);
                        state = GameState::Shop;
                    } else {
                        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                        float clampedX = std::clamp(mousePos.x, radius, windowWidth - radius);
                        float clampedY = std::clamp(mousePos.y, radius, windowHeight - radius);
                        sf::Vector2f snappedTopLeft{clampedX - radius, clampedY - radius};

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
                }

                if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                    for (Ball& ball : balls) {
                        if (!ball.isDragging) continue;
                        ball.isDragging = false;
                        float speed = magnitude(ball.lastDragVelocity);
                        if (speed > 10.f) ball.velocity = ball.lastDragVelocity * throwBoostFactor;
                        else ball.velocity = ball.storedVelocity;
                        ball.hasLastDragMousePos = false;
                        recolorBall(ball);
                    }
                }
            }
        }

        float dt = clock.restart().asSeconds();
        dt = std::min(dt, 0.03f);

        if (state == GameState::Playing) {
            for (Ball& ball : balls) {
                if (ball.isDragging) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    if (ball.hasLastDragMousePos && dt > 0.f) {
                        sf::Vector2f delta = mousePos - ball.lastDragMousePos;
                        ball.lastDragVelocity = delta / dt;
                    }
                    ball.lastDragMousePos = mousePos;
                    ball.hasLastDragMousePos = true;

                    sf::Vector2f desiredPos = mousePos + ball.dragOffset;
                    desiredPos.x = std::clamp(desiredPos.x, 0.f, windowWidth - 2.f * radius);
                    desiredPos.y = std::clamp(desiredPos.y, 0.f, windowHeight - 2.f * radius);
                    ball.shape.setPosition(desiredPos);
                    recolorBall(ball);
                    continue;
                }

                ball.shape.move(ball.velocity * dt);

                sf::Vector2f pos = ball.shape.getPosition();
                bool bounced = false;

                if (pos.x <= 0.f) {
                    pos.x = 0.f;
                    ball.velocity.x = -ball.velocity.x;
                    bounced = true;
                } else if (pos.x + 2.f * radius >= windowWidth) {
                    pos.x = windowWidth - 2.f * radius;
                    ball.velocity.x = -ball.velocity.x;
                    bounced = true;
                }

                if (pos.y <= 0.f) {
                    pos.y = 0.f;
                    ball.velocity.y = -ball.velocity.y;
                    bounced = true;
                } else if (pos.y + 2.f * radius >= windowHeight) {
                    pos.y = windowHeight - 2.f * radius;
                    ball.velocity.y = -ball.velocity.y;
                    bounced = true;
                }

                ball.shape.setPosition(pos);

                if (bounced) {
                    ++totalBounces;
                    points += pointsPerBounce();
                    spawnFloatingText("+" + std::to_string(pointsPerBounce()), pos + sf::Vector2f(radius, 0.f));

                    float speed = magnitude(ball.velocity);
                    float angle = std::atan2(ball.velocity.y, ball.velocity.x) + angleDist(rng);
                    ball.velocity.x = speed * std::cos(angle);
                    ball.velocity.y = speed * std::sin(angle);
                }

                float speed = magnitude(ball.velocity);
                if (speed > 0.001f) {
                    float target = currentCruiseSpeed();
                    float factor = std::min(1.f, cruiseAdjustRate * dt);
                    float newSpeed = speed + (target - speed) * factor;
                    ball.velocity *= newSpeed / speed;
                } else {
                    sf::Vector2f dir = normalizeOrDefault({sideDist(rng), sideDist(rng)}, {1.f, 0.2f});
                    ball.velocity = dir * currentCruiseSpeed();
                }

                recolorBall(ball);
            }

            for (std::size_t i = 0; i < balls.size(); ++i) {
                for (std::size_t j = i + 1; j < balls.size(); ++j) {
                    sf::Vector2f a = balls[i].shape.getPosition() + sf::Vector2f(radius, radius);
                    sf::Vector2f b = balls[j].shape.getPosition() + sf::Vector2f(radius, radius);
                    sf::Vector2f delta = b - a;
                    float dist = magnitude(delta);
                    float minDist = 2.f * radius;

                    if (dist > 0.f && dist < minDist) {
                        sf::Vector2f normal = delta / dist;
                        float overlap = minDist - dist;
                        balls[i].shape.move(-normal * (overlap * 0.5f));
                        balls[j].shape.move(normal * (overlap * 0.5f));
                        std::swap(balls[i].velocity, balls[j].velocity);
                    }
                }
            }

            for (auto it = floatingTexts.begin(); it != floatingTexts.end();) {
                it->lifetime -= dt;
                it->text.move(it->velocity * dt);
                sf::Color c = it->text.getFillColor();
                c.a = static_cast<sf::Uint8>(255.f * std::max(0.f, it->lifetime / 0.9f));
                it->text.setFillColor(c);
                if (it->lifetime <= 0.f) it = floatingTexts.erase(it);
                else ++it;
            }

            hudTopLeft.setString("Points: " + std::to_string(points) + "   |   Total bounces: " + std::to_string(totalBounces));
            hudSecondLine.setString("Balls: " + std::to_string(balls.size()) + "   |   Points/bounce: " + std::to_string(pointsPerBounce()));
            hudThirdLine.setString("Base speed: " + std::to_string(static_cast<int>(currentCruiseSpeed())) +
                                   "   |   ESC = upgrades");
        }

        if (state == GameState::Shop) {
            uint32_t speedCost = upgradeCost(15, speedLevel, 1.55f);
            uint32_t pointsCost = upgradeCost(25, pointsLevel, 1.70f);
            uint32_t multiballCost = upgradeCost(80, multiballLevel, 2.30f);

            upgradeSpeed.setString(
                "[1] Speed level " + std::to_string(speedLevel) +
                "  ->  +18% velocidad base\nCosto: " + std::to_string(speedCost) +
                " pts");

            upgradePoints.setString(
                "[2] Points per bounce level " + std::to_string(pointsLevel) +
                "  ->  ganas +1 punto extra por rebote\nCosto: " + std::to_string(pointsCost) +
                " pts");

            upgradeMultiball.setString(
                "[3] Multiball level " + std::to_string(multiballLevel) +
                "  ->  agrega una pelota nueva\nCosto: " + std::to_string(multiballCost) +
                " pts");

            auto paintAffordable = [&](sf::Text& t, uint32_t cost) {
                t.setFillColor(points >= cost ? sf::Color::White : sf::Color(120, 120, 120));
            };

            paintAffordable(upgradeSpeed, speedCost);
            paintAffordable(upgradePoints, pointsCost);
            paintAffordable(upgradeMultiball, multiballCost);
        }

        window.clear(sf::Color(12, 12, 22));

        if (state == GameState::MainMenu) {
            window.draw(titleMain);
            window.draw(playItem);
            window.draw(optionsItem);
            window.draw(exitItem);
        } else if (state == GameState::PlayMenu) {
            window.draw(titlePlay);
            window.draw(newGameItem);
            continueItem.setFillColor(hasSavedGame() ? sf::Color::White : sf::Color(120, 120, 120));
            window.draw(continueItem);
            window.draw(backPlayItem);
        } else if (state == GameState::Options) {
            window.draw(titleOpt);
            window.draw(opt1);
            window.draw(opt2);
            window.draw(opt3);
            window.draw(opt4);
            window.draw(backOptItem);
        } else if (state == GameState::Playing) {
            for (const Ball& ball : balls) window.draw(ball.shape);
            for (const FloatingText& ft : floatingTexts) window.draw(ft.text);
            window.draw(hudTopLeft);
            window.draw(hudSecondLine);
            window.draw(hudThirdLine);
            window.draw(shopButton);
        } else if (state == GameState::Shop) {
            sf::Text shopPoints("Available points: " + std::to_string(points), font, 28);
            shopPoints.setPosition(80.f, 110.f);
            shopPoints.setFillColor(sf::Color::Yellow);

            window.draw(shopTitle);
            window.draw(shopPoints);
            window.draw(upgradeSpeed);
            window.draw(upgradePoints);
            window.draw(upgradeMultiball);
            window.draw(shopInfo);
            window.draw(shopBack);
        }

        window.display();
    }

    return 0;
}
