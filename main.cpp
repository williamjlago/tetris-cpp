#include <SFML/Graphics.hpp>

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int TILE_SIZE = 30;

int shapes[7][4][4] = {
    // Define the 7 tetromino shapes here
};

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};

void handleInput();
void update();
void render(sf::RenderWindow &window);

void handleInput() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        // Move shape left
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        // Move shape right
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        // Move shape down faster
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        // Rotate shape
    }
}

void update() {
    // Move current shape down
    // Check for collisions
    // Lock shape in place if it can't move down
    // Clear full lines
    // Spawn new shape if needed
}


int main() {
    sf::RenderWindow window(sf::VideoMode(BOARD_WIDTH * TILE_SIZE, BOARD_HEIGHT * TILE_SIZE), "Tetris");

    sf::Clock clock;
    float timer = 0, delay = 0.5;

    while (window.isOpen()) {
        float time = clock.restart().asSeconds();
        timer += time;

        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            // Add more event handling as needed
        }

        handleInput();

        // Update game logic
        if (timer > delay) {
            update();
            timer = 0;
        }

        // Render
        window.clear(sf::Color::Black);
        render(window);
        window.display();
    }

    return 0;
}