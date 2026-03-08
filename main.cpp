#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>
#include <cmath>
#include <fstream>

namespace {
    const int windowWidth = 800;
    const int windowHeight = 600;
    const float radius = 15.f;
    const char* savePath = "save.dat";

    enum class GameState { MainMenu, PlayMenu, Options, Playing };

    struct GameData {
        float ballX, ballY;
        float velX, velY;
        uint32_t bounceCount;
    };

    bool saveGame(const GameData& d) {
        std::ofstream f(savePath, std::ios::binary);
        if (!f) return false;
        f.write(reinterpret_cast<const char*>(&d.ballX), sizeof(float));
        f.write(reinterpret_cast<const char*>(&d.ballY), sizeof(float));
        f.write(reinterpret_cast<const char*>(&d.velX), sizeof(float));
        f.write(reinterpret_cast<const char*>(&d.velY), sizeof(float));
        f.write(reinterpret_cast<const char*>(&d.bounceCount), sizeof(uint32_t));
        return f.good();
    }

    bool loadGame(GameData& d) {
        std::ifstream f(savePath, std::ios::binary);
        if (!f) return false;
        f.read(reinterpret_cast<char*>(&d.ballX), sizeof(float));
        f.read(reinterpret_cast<char*>(&d.ballY), sizeof(float));
        f.read(reinterpret_cast<char*>(&d.velX), sizeof(float));
        f.read(reinterpret_cast<char*>(&d.velY), sizeof(float));
        f.read(reinterpret_cast<char*>(&d.bounceCount), sizeof(uint32_t));
        return f.good();
    }

    bool hasSavedGame() {
        std::ifstream f(savePath, std::ios::binary);
        return f.good();
    }

    bool isMouseOver(const sf::Text& text, const sf::RenderWindow& window) {
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        return text.getGlobalBounds().contains(mouse);
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Space-Breakers");
    window.setFramerateLimit(60);

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> angleDist(-0.26f, 0.26f);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "No se pudo cargar la fuente (arial.ttf)\n";
        return 1;
    }

    auto makeMenuText = [&font](const std::string& str, unsigned size, float y) {
        sf::Text t(str, font, size);
        t.setFillColor(sf::Color::White);
        sf::FloatRect bounds = t.getLocalBounds();
        t.setPosition(windowWidth / 2.f - bounds.width / 2.f, y);
        return t;
    };

    GameState state = GameState::MainMenu;

    // --- Menú principal
    sf::Text titleMain = makeMenuText("Space-Breakers", 48, 120.f);
    sf::Text playItem = makeMenuText("Jugar", 36, 260.f);
    sf::Text optionsItem = makeMenuText("Opciones", 36, 320.f);
    sf::Text exitItem = makeMenuText("Salir", 36, 380.f);

    // --- Submenú Jugar
    sf::Text titlePlay = makeMenuText("Jugar", 48, 120.f);
    sf::Text newGameItem = makeMenuText("Nueva partida", 36, 260.f);
    sf::Text continueItem = makeMenuText("Continuar", 36, 320.f);
    sf::Text backPlayItem = makeMenuText("Volver", 28, 450.f);

    // --- Opciones
    sf::Text titleOpt = makeMenuText("Opciones", 48, 120.f);
    sf::Text backOptItem = makeMenuText("Volver", 36, 350.f);

    // --- Datos del juego (usados en Playing)
    sf::CircleShape ball(radius);
    ball.setFillColor(sf::Color::Transparent);
    ball.setOutlineThickness(2.f);
    ball.setOutlineColor(sf::Color::Red);

    sf::Vector2f velocity(250.f, 180.f);
    const float cruiseSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    const float cruiseAdjustRate = 2.0f;
    const float throwBoostFactor = 1.8f;

    bool isDragging = false;
    sf::Vector2f dragOffset(0.f, 0.f);
    sf::Vector2f storedVelocity = velocity;
    sf::Vector2f lastDragVelocity(0.f, 0.f);
    sf::Vector2f lastDragMousePos(0.f, 0.f);
    bool hasLastDragMousePos = false;

    unsigned int bounceCount = 0;

    sf::Text counterText;
    counterText.setFont(font);
    counterText.setCharacterSize(24);
    counterText.setFillColor(sf::Color::White);
    counterText.setPosition(10.f, 10.f);

    sf::Clock clock;

    auto resetGame = [&]() {
        ball.setPosition(windowWidth / 2.f, windowHeight / 2.f);
        velocity = sf::Vector2f(250.f, 180.f);
        bounceCount = 0;
        isDragging = false;
        storedVelocity = velocity;
        lastDragVelocity = sf::Vector2f(0.f, 0.f);
        hasLastDragMousePos = false;
    };

    auto applyLoadedGame = [&](const GameData& d) {
        ball.setPosition(d.ballX, d.ballY);
        velocity.x = d.velX;
        velocity.y = d.velY;
        bounceCount = d.bounceCount;
        storedVelocity = velocity;
        isDragging = false;
        hasLastDragMousePos = false;
    };

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                break;
            }

            if (state == GameState::MainMenu) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(playItem, window)) { state = GameState::PlayMenu; }
                    else if (isMouseOver(optionsItem, window)) { state = GameState::Options; }
                    else if (isMouseOver(exitItem, window)) { window.close(); break; }
                }
            }
            else if (state == GameState::PlayMenu) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(newGameItem, window)) {
                        resetGame();
                        state = GameState::Playing;
                    }
                    else if (isMouseOver(continueItem, window)) {
                        GameData loaded;
                        if (loadGame(loaded)) {
                            applyLoadedGame(loaded);
                        } else {
                            resetGame();
                        }
                        state = GameState::Playing;
                    }
                    else if (isMouseOver(backPlayItem, window)) {
                        state = GameState::MainMenu;
                    }
                }
            }
            else if (state == GameState::Options) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (isMouseOver(backOptItem, window)) { state = GameState::MainMenu; }
                }
            }
            else if (state == GameState::Playing) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                    GameData toSave;
                    sf::Vector2f p = ball.getPosition();
                    toSave.ballX = p.x;
                    toSave.ballY = p.y;
                    toSave.velX = velocity.x;
                    toSave.velY = velocity.y;
                    toSave.bounceCount = static_cast<uint32_t>(bounceCount);
                    saveGame(toSave);
                    state = GameState::MainMenu;
                }

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    // Clic en cualquier lugar: la pelota va hasta ahí y la agarras
                    float centerX = std::max(radius, std::min(windowWidth - radius, mousePos.x));
                    float centerY = std::max(radius, std::min(windowHeight - radius, mousePos.y));
                    ball.setPosition(centerX - radius, centerY - radius);
                    isDragging = true;
                    storedVelocity = velocity;
                    velocity = sf::Vector2f(0.f, 0.f);
                    dragOffset = ball.getPosition() - mousePos;
                    lastDragVelocity = sf::Vector2f(0.f, 0.f);
                    lastDragMousePos = mousePos;
                    hasLastDragMousePos = false;
                }
                if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                    if (isDragging) {
                        isDragging = false;
                        float speed = std::sqrt(lastDragVelocity.x*lastDragVelocity.x + lastDragVelocity.y*lastDragVelocity.y);
                        if (speed > 10.f) velocity = lastDragVelocity * throwBoostFactor;
                        else velocity = storedVelocity;
                    }
                }
            }
        }

        float dt = clock.restart().asSeconds();

        if (state == GameState::Playing) {
            if (isDragging) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (hasLastDragMousePos && dt > 0.f) {
                    sf::Vector2f delta = mousePos - lastDragMousePos;
                    lastDragVelocity = delta / dt;
                }
                lastDragMousePos = mousePos;
                hasLastDragMousePos = true;
                ball.setPosition(mousePos + dragOffset);
            } else {
                ball.move(velocity * dt);
            }

            sf::Vector2f pos = ball.getPosition();
            bool bounced = false;

            if (pos.x <= 0.f) { pos.x = 0.f; velocity.x = -velocity.x; bounced = true; }
            else if (pos.x + 2*radius >= windowWidth) { pos.x = windowWidth - 2*radius; velocity.x = -velocity.x; bounced = true; }
            if (pos.y <= 0.f) { pos.y = 0.f; velocity.y = -velocity.y; bounced = true; }
            else if (pos.y + 2*radius >= windowHeight) { pos.y = windowHeight - 2*radius; velocity.y = -velocity.y; bounced = true; }

            ball.setPosition(pos);

            if (bounced) {
                // Solo contar rebote si no estabas arrastrando (ej. presionando contra la pared)
                if (!isDragging) {
                    bounceCount++;
                }
                float speed = std::sqrt(velocity.x*velocity.x + velocity.y*velocity.y);
                float angle = std::atan2(velocity.y, velocity.x) + angleDist(rng);
                velocity.x = speed * std::cos(angle);
                velocity.y = speed * std::sin(angle);
            }

            if (!isDragging) {
                float speed = std::sqrt(velocity.x*velocity.x + velocity.y*velocity.y);
                if (speed > 0.001f) {
                    float factor = std::min(1.f, cruiseAdjustRate * dt);
                    float newSpeed = speed + (cruiseSpeed - speed) * factor;
                    if (newSpeed < 0.f) newSpeed = 0.f;
                    float scale = newSpeed / speed;
                    velocity.x *= scale;
                    velocity.y *= scale;
                }
            }

            counterText.setString("rebounds: " + std::to_string(bounceCount));
        }

        // Dibujar
        window.clear(sf::Color::Black);

        if (state == GameState::MainMenu) {
            window.draw(titleMain);
            window.draw(playItem);
            window.draw(optionsItem);
            window.draw(exitItem);
        }
        else if (state == GameState::PlayMenu) {
            window.draw(titlePlay);
            window.draw(newGameItem);
            if (!hasSavedGame()) {
                continueItem.setFillColor(sf::Color(120, 120, 120));
            } else {
                continueItem.setFillColor(sf::Color::White);
            }
            window.draw(continueItem);
            window.draw(backPlayItem);
        }
        else if (state == GameState::Options) {
            window.draw(titleOpt);
            window.draw(backOptItem);
        }
        else if (state == GameState::Playing) {
            window.draw(ball);
            window.draw(counterText);
        }

        window.display();
    }

    return 0;
}
