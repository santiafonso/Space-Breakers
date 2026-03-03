#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>
#include <cmath>

int main() {
    const int windowWidth = 800;
    const int windowHeight = 600;
    const float radius = 15.f;

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Space-Breakers");
    window.setFramerateLimit(60);

    // Generador de aleatoriedad para los rebotes
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> angleDist(-0.26f, 0.26f); // ~±15 grados

    // Pelota (solo contorno)
    sf::CircleShape ball(radius);
    ball.setFillColor(sf::Color::Transparent);
    ball.setOutlineThickness(2.f);
    ball.setOutlineColor(sf::Color::Red);
    ball.setPosition(windowWidth / 2.f, windowHeight / 2.f);

    // Velocidad inicial (pixeles por segundo)
    sf::Vector2f velocity(250.f, 180.f);
    const float cruiseSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    const float cruiseAdjustRate = 2.0f;   // qué tan rápido vuelve a velocidad crucero
    const float throwBoostFactor = 1.8f;   // multiplica la fuerza del "lanzamiento"

    // Control para arrastrar la pelota con el ratón
    bool isDragging = false;
    sf::Vector2f dragOffset(0.f, 0.f);
    sf::Vector2f storedVelocity = velocity;      // velocidad base por si el lanzamiento es muy débil
    sf::Vector2f lastDragVelocity(0.f, 0.f);     // velocidad calculada a partir del ratón
    sf::Vector2f lastDragMousePos(0.f, 0.f);
    bool hasLastDragMousePos = false;

    // Contador de rebotes
    unsigned int bounceCount = 0;

    // Fuente y texto para mostrar el contador
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "No se pudo cargar la fuente (arial.ttf)\n";
        return 1;
    }

    sf::Text counterText;
    counterText.setFont(font);
    counterText.setCharacterSize(24);
    counterText.setFillColor(sf::Color::White);
    counterText.setPosition(10.f, 10.f);

    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Manejo del ratón para agarrar/soltar la pelota
            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                sf::Vector2f ballPos = ball.getPosition();
                sf::Vector2f ballCenter(ballPos.x + radius, ballPos.y + radius);
                float dx = mousePos.x - ballCenter.x;
                float dy = mousePos.y - ballCenter.y;
                float dist2 = dx * dx + dy * dy;
                if (dist2 <= radius * radius) {
                    isDragging = true;
                    storedVelocity = velocity;
                    velocity = sf::Vector2f(0.f, 0.f);
                    dragOffset = ballPos - mousePos;
                    lastDragVelocity = sf::Vector2f(0.f, 0.f);
                    lastDragMousePos = mousePos;
                    hasLastDragMousePos = false;
                }
            }

            if (event.type == sf::Event::MouseButtonReleased &&
                event.mouseButton.button == sf::Mouse::Left) {
                if (isDragging) {
                    isDragging = false;
                    // Calcular velocidad a partir del último movimiento del ratón
                    float speed = std::sqrt(lastDragVelocity.x * lastDragVelocity.x +
                                            lastDragVelocity.y * lastDragVelocity.y);
                    if (speed > 10.f) {
                        velocity = lastDragVelocity * throwBoostFactor;
                    } else {
                        velocity = storedVelocity;
                    }
                }
            }
        }

        float dt = clock.restart().asSeconds();

        // Si estamos arrastrando, seguir al ratón y medir movimiento; si no, movimiento normal
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
            // Mover la pelota
            ball.move(velocity * dt);
        }

        // Obtener posición actual
        sf::Vector2f pos = ball.getPosition();

        bool bounced = false;

        // Colisión con paredes izquierda y derecha
        if (pos.x <= 0.f) {
            pos.x = 0.f;
            velocity.x = -velocity.x;
            bounced = true;
        } else if (pos.x + 2 * radius >= windowWidth) {
            pos.x = windowWidth - 2 * radius;
            velocity.x = -velocity.x;
            bounced = true;
        }

        // Colisión con paredes superior e inferior
        if (pos.y <= 0.f) {
            pos.y = 0.f;
            velocity.y = -velocity.y;
            bounced = true;
        } else if (pos.y + 2 * radius >= windowHeight) {
            pos.y = windowHeight - 2 * radius;
            velocity.y = -velocity.y;
            bounced = true;
        }

        // Actualizar posición corregida
        ball.setPosition(pos);

        // Si rebotó en alguna pared, aumentar contador y hacer el rebote levemente aleatorio
        if (bounced) {
            bounceCount++;

            // Mantener la velocidad similar, pero con un pequeño cambio de ángulo
            float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
            float angle = std::atan2(velocity.y, velocity.x);
            float randomAngle = angleDist(rng);
            angle += randomAngle;

            velocity.x = speed * std::cos(angle);
            velocity.y = speed * std::sin(angle);
        }

        // Ajustar poco a poco la velocidad hacia la velocidad "crucero"
        if (!isDragging) {
            float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
            if (speed > 0.001f) {
                float factor = std::min(1.f, cruiseAdjustRate * dt);
                float newSpeed = speed + (cruiseSpeed - speed) * factor;
                if (newSpeed < 0.f) newSpeed = 0.f;
                float scale = newSpeed / speed;
                velocity.x *= scale;
                velocity.y *= scale;
            }
        }

        // Actualizar texto del contador
        counterText.setString("rebounds: " + std::to_string(bounceCount));

        // Dibujar
        window.clear(sf::Color::Black);
        window.draw(ball);
        window.draw(counterText);
        window.display();
    }

    return 0;
}