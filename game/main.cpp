#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <random>
#include <memory>
#include <algorithm>

const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 600;
const int GRID_ROWS = 5;
const int GRID_COLS = 9;
const float CELL_SIZE = 80.f;
const float OFFSET_X = 50.f;
const float OFFSET_Y = 100.f;

enum class ObjectType { SUNFLOWER, PEASHOOTER, ZOMBIE, SUN, PEA };


class GameObject {
public:
    sf::RectangleShape shape;
    float health;
    bool markedForDeletion;

    GameObject(float x, float y, float w, float h, sf::Color color)
        : health(100), markedForDeletion(false) {
        shape.setSize(sf::Vector2f(w, h));
        shape.setPosition(sf::Vector2f(x, y));
        shape.setFillColor(color);
        shape.setOutlineColor(sf::Color::Black);
        shape.setOutlineThickness(1.f);
    }

    virtual ~GameObject() = default;
    virtual void update(float dt) {}
    virtual void draw(sf::RenderWindow& window) {
        window.draw(shape);
    }

    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }
};

class Plant : public GameObject {
public:
    float lastActionTime;
    float actionCooldown;
    int cost;

    Plant(float x, float y, sf::Color color, int cost, float cooldown)
        : GameObject(x, y, CELL_SIZE - 4, CELL_SIZE - 4, color),
        cost(cost), lastActionTime(0), actionCooldown(cooldown) {
    }
};

class Sunflower : public Plant {
public:
    Sunflower(float x, float y) : Plant(x, y, sf::Color::Yellow, 50, 5.0f) {}
};

class Peashooter : public Plant {
public:
    Peashooter(float x, float y) : Plant(x, y, sf::Color::Green, 100, 1.5f) {}
};


class Pea : public GameObject {
public:
    float speed;
    int damage;

    Pea(float x, float y) : GameObject(x, y, 20, 20, sf::Color::Green),
        speed(300.f), damage(20) {
        shape.setFillColor(sf::Color::Green);
    }

    void update(float dt) override {
        shape.move(sf::Vector2f(speed * dt, 0));
        if (shape.getPosition().x > WINDOW_WIDTH) {
            markedForDeletion = true;
        }
    }
};

class Zombie : public GameObject {
public:
    float speed;
    int damage;
    float attackCooldown;
    float lastAttackTime;
    bool isEating;

    Zombie(float x, float y)
        : GameObject(x, y, CELL_SIZE - 10, CELL_SIZE - 10, sf::Color(100, 0, 0)),
        speed(30.f), damage(10), attackCooldown(1.0f),
        lastAttackTime(0), isEating(false) {
    }

    void update(float dt) override {
        if (!isEating) {
            shape.move(sf::Vector2f(-speed * dt, 0));
        }
        else {
            lastAttackTime += dt;
        }

        if (shape.getPosition().x < 0) {
            markedForDeletion = true;
        }
    }
};


class SunDrop : public GameObject {
public:
    float speed;
    sf::Vector2f targetPos;

    SunDrop(float x, float y, float targetY)
        : GameObject(x, y, 40, 40, sf::Color::Yellow), speed(100.f) {
        shape.setFillColor(sf::Color::Yellow);
        targetPos = sf::Vector2f(x, targetY);
    }

    void update(float dt) override {
        if (shape.getPosition().y < targetPos.y) {
            shape.move(sf::Vector2f(0, speed * dt));
        }
    }
};

class Game {
private:
    sf::RenderWindow window;
    sf::Font font;
    sf::Text scoreText;
    sf::Text messageText;

    std::vector<std::unique_ptr<Plant>> plants;
    std::vector<std::unique_ptr<Zombie>> zombies;
    std::vector<std::unique_ptr<Pea>> peas;
    std::vector<std::unique_ptr<SunDrop>> suns;

    int sunResource;
    float zombieSpawnTimer;
    float zombieSpawnInterval;
    int zombiesKilled;
    int zombiesToWin;
    bool gameOver;
    bool gameWon;
    sf::Clock clock;
    ObjectType selectedPlant;

public:
    Game() : window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }), "Plants vs Zombies - SFML 3"),
        scoreText(font), messageText(font) {

        window.setFramerateLimit(60);

        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
            std::cout << "WARNING: Could not load arial.ttf" << std::endl;
            
        }
        else {
            std::cout << "Font loaded successfully!" << std::endl;
        }

        scoreText.setCharacterSize(24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(sf::Vector2f(10, 10));

        messageText.setCharacterSize(50);
        messageText.setFillColor(sf::Color::Red);
        messageText.setPosition(sf::Vector2f(WINDOW_WIDTH / 4.f, WINDOW_HEIGHT / 2.f));

        
        sunResource = 150;
        zombieSpawnTimer = 0;
        zombieSpawnInterval = 5.0f;
        zombiesKilled = 0;
        zombiesToWin = 10;
        gameOver = false;
        gameWon = false;
        selectedPlant = ObjectType::PEASHOOTER;
    }

    void run() {
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds();
            handleEvents();
            if (!gameOver && !gameWon) {
                update(dt);
            }
            draw();
        }
    }

private:
    void handleEvents() {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* mbEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mbEvent->button == sf::Mouse::Button::Left) {
                    handleClick(sf::Vector2f(static_cast<float>(mbEvent->position.x),
                        static_cast<float>(mbEvent->position.y)));
                }
            }

            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Num1)
                    selectedPlant = ObjectType::SUNFLOWER;
                if (keyEvent->code == sf::Keyboard::Key::Num2)
                    selectedPlant = ObjectType::PEASHOOTER;
                if (keyEvent->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
        }
    }

    void handleClick(sf::Vector2f pos) {
        if (gameOver || gameWon) return;

        // Клик по солнцу
        for (auto it = suns.begin(); it != suns.end(); ++it) {
            if ((*it)->getBounds().contains(pos)) {
                sunResource += 25;
                (*it)->markedForDeletion = true;
                return;
            }
        }

        // Посадка растения
        int col = static_cast<int>((pos.x - OFFSET_X) / CELL_SIZE);
        int row = static_cast<int>((pos.y - OFFSET_Y) / CELL_SIZE);

        if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
            bool occupied = false;
            for (const auto& p : plants) {
                int pCol = static_cast<int>((p->shape.getPosition().x - OFFSET_X) / CELL_SIZE);
                int pRow = static_cast<int>((p->shape.getPosition().y - OFFSET_Y) / CELL_SIZE);
                if (pCol == col && pRow == row) {
                    occupied = true;
                    break;
                }
            }

            if (!occupied) {
                float x = OFFSET_X + col * CELL_SIZE + 2;
                float y = OFFSET_Y + row * CELL_SIZE + 2;

                if (selectedPlant == ObjectType::SUNFLOWER && sunResource >= 50) {
                    sunResource -= 50;
                    plants.push_back(std::make_unique<Sunflower>(x, y));
                }
                else if (selectedPlant == ObjectType::PEASHOOTER && sunResource >= 100) {
                    sunResource -= 100;
                    plants.push_back(std::make_unique<Peashooter>(x, y));
                }
            }
        }
    }

    void update(float dt) {
        // Спавн зомби
        zombieSpawnTimer += dt;
        if (zombieSpawnTimer > zombieSpawnInterval) {
            zombieSpawnTimer = 0;
            int row = rand() % GRID_ROWS;
            float y = OFFSET_Y + row * CELL_SIZE + 5;
            zombies.push_back(std::make_unique<Zombie>(WINDOW_WIDTH, y));
            if (zombieSpawnInterval > 2.0f) zombieSpawnInterval -= 0.1f;
        }

        // Спавн солнца
        if (rand() % 500 == 0) {
            float x = OFFSET_X + rand() % (static_cast<int>(GRID_COLS) * static_cast<int>(CELL_SIZE));
            suns.push_back(std::make_unique<SunDrop>(x, OFFSET_Y - 50,
                OFFSET_Y + rand() % (static_cast<int>(GRID_ROWS) * static_cast<int>(CELL_SIZE))));
        }

        // Обновление растений
        for (auto& plant : plants) {
            plant->lastActionTime += dt;
            plant->update(dt);

            // Подсолнух генерирует солнце
            if (dynamic_cast<Sunflower*>(plant.get())) {
                if (plant->lastActionTime >= plant->actionCooldown) {
                    plant->lastActionTime = 0;
                    suns.push_back(std::make_unique<SunDrop>(
                        plant->shape.getPosition().x,
                        plant->shape.getPosition().y,
                        plant->shape.getPosition().y + 20));
                }
            }

            // Горохострел стреляет
            if (dynamic_cast<Peashooter*>(plant.get())) {
                if (plant->lastActionTime >= plant->actionCooldown) {
                    int row = static_cast<int>((plant->shape.getPosition().y - OFFSET_Y) / CELL_SIZE);
                    bool zombieOnLine = false;
                    for (const auto& z : zombies) {
                        int zRow = static_cast<int>((z->shape.getPosition().y - OFFSET_Y) / CELL_SIZE);
                        if (zRow == row && z->shape.getPosition().x > plant->shape.getPosition().x) {
                            zombieOnLine = true;
                            break;
                        }
                    }

                    if (zombieOnLine) {
                        plant->lastActionTime = 0;
                        float px = plant->shape.getPosition().x + CELL_SIZE;
                        float py = plant->shape.getPosition().y + CELL_SIZE / 2 - 10;
                        peas.push_back(std::make_unique<Pea>(px, py));
                    }
                }
            }
        }

        // Обновление горошин
        for (auto& pea : peas) {
            pea->update(dt);
        }

        // Обновление зомби и коллизии
        for (auto& zombie : zombies) {
            zombie->isEating = false;

            // Коллизия с растениями
            for (auto& plant : plants) {
                sf::FloatRect zombieBounds = zombie->getBounds();
                sf::FloatRect plantBounds = plant->getBounds();

                if (zombieBounds.findIntersection(plantBounds).has_value()) {
                    zombie->isEating = true;
                    plant->health -= zombie->damage * dt * 10;
                    if (plant->health <= 0) {
                        plant->markedForDeletion = true;
                    }
                }
            }

            // Коллизия с горошинами
            for (auto& pea : peas) {
                if (!pea->markedForDeletion) {
                    sf::FloatRect zombieBounds = zombie->getBounds();
                    sf::FloatRect peaBounds = pea->getBounds();

                    if (zombieBounds.findIntersection(peaBounds).has_value()) {
                        pea->markedForDeletion = true;
                        zombie->health -= pea->damage;
                        if (zombie->health <= 0) {
                            zombie->markedForDeletion = true;
                            zombiesKilled++;
                        }
                    }
                }
            }

            zombie->update(dt);

            if (zombie->shape.getPosition().x < OFFSET_X - 20) {
                gameOver = true;
            }
        }

        // Обновление солнца
        for (auto& sun : suns) {
            sun->update(dt);
        }

        // Очистка
        plants.erase(std::remove_if(plants.begin(), plants.end(),
            [](const std::unique_ptr<Plant>& p) { return p->markedForDeletion; }), plants.end());
        zombies.erase(std::remove_if(zombies.begin(), zombies.end(),
            [](const std::unique_ptr<Zombie>& z) { return z->markedForDeletion; }), zombies.end());
        peas.erase(std::remove_if(peas.begin(), peas.end(),
            [](const std::unique_ptr<Pea>& p) { return p->markedForDeletion; }), peas.end());
        suns.erase(std::remove_if(suns.begin(), suns.end(),
            [](const std::unique_ptr<SunDrop>& s) { return s->markedForDeletion; }), suns.end());

        if (zombiesKilled >= zombiesToWin) {
            gameWon = true;
        }

        scoreText.setString("Sun: " + std::to_string(sunResource) +
            " | Killed: " + std::to_string(zombiesKilled) +
            "/" + std::to_string(zombiesToWin) +
            "\n[1] Sunflower (50)  [2] Peashooter (100)");
    }

    void draw() {
        window.clear(sf::Color(34, 139, 34));

        // Рисуем сетку
        for (int r = 0; r < GRID_ROWS; ++r) {
            for (int c = 0; c < GRID_COLS; ++c) {
                sf::RectangleShape cell(sf::Vector2f(CELL_SIZE, CELL_SIZE));
                cell.setPosition(sf::Vector2f(OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE));
                cell.setFillColor(((r + c) % 2 == 0) ? sf::Color(50, 150, 50) : sf::Color(40, 140, 40));
                cell.setOutlineColor(sf::Color(20, 100, 20));
                cell.setOutlineThickness(1);
                window.draw(cell);
            }
        }

        for (auto& plant : plants) plant->draw(window);
        for (auto& zombie : zombies) zombie->draw(window);
        for (auto& pea : peas) pea->draw(window);
        for (auto& sun : suns) sun->draw(window);

        window.draw(scoreText);

        if (gameOver) {
            messageText.setString("ZOMBIES ATE YOUR BRAINS!");
            messageText.setFillColor(sf::Color::Red);
            window.draw(messageText);
            
        }
        else if (gameWon) {
            messageText.setString("YOU WIN!");
            messageText.setFillColor(sf::Color::Green);
            window.draw(messageText);
        }

        window.display();
    }
};

int main() {
    try {
        Game game;
        game.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}