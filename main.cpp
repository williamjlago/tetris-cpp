#include <cstdlib>
#include <ctime>
#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int TILE_SIZE = 32;

int currentPiece = 0;
int currentX = BOARD_WIDTH / 2 - 2; 
int currentY = 0; 
int currentRotation = 0;
bool hasActivePiece = false;

sf::Color pieceColors[7] = {
    sf::Color::Cyan,      // I
    sf::Color::Yellow,    // O
    sf::Color::Magenta,   // T
    sf::Color::Green,     // S
    sf::Color::Red,       // Z
    sf::Color::Blue,      // J
    sf::Color(255,165,0)  // L (orange)
};

sf::Texture tileTexture;

int shapes[7][4][4] = {
    // I
    {
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    },
    // O
    {
        {0,1,1,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // T
    {
        {0,1,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // S
    {
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // Z
    {
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // J
    {
        {1,0,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    // L
    {
        {0,0,1,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    }
};

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};

// Line clearing variables
bool lineClearActive = false;
float lineClearTimer = 0.0f; 
float lineClearTotalTime = 0.3f; // Total flash duration
std::vector<int> fullLines;

void spawnPiece();
bool checkCollision(int piece, int rotation, int x, int y);
void lockPiece();
int getBlockValue(int piece, int rotation, int i, int j);
void update(float dt);
void render(sf::RenderWindow &window);
void checkAndStartLineClear();
void finalizeLineClear();
void removeLines(const std::vector<int>& lines);

int main() {
    srand((unsigned)time(0));
    sf::RenderWindow window(sf::VideoMode((BOARD_WIDTH + 2) * TILE_SIZE, (BOARD_HEIGHT + 2) * TILE_SIZE), "Tetris");

    if (!tileTexture.loadFromFile("tile.png")) {
        return -1;
    }
    tileTexture.setSmooth(true);

    sf::Clock clock;
    float timer = 0;
    float delay = 0.5f;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        timer += dt;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (!hasActivePiece || lineClearActive) continue;

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Left) {
                    if (!checkCollision(currentPiece, currentRotation, currentX - 1, currentY))
                        currentX -= 1;
                } else if (event.key.code == sf::Keyboard::Right) {
                    if (!checkCollision(currentPiece, currentRotation, currentX + 1, currentY))
                        currentX += 1;
                } else if (event.key.code == sf::Keyboard::Down) {
                    int newY = currentY + 1;
                    if (!checkCollision(currentPiece, currentRotation, currentX, newY)) {
                        currentY = newY;
                    } else {
                        lockPiece();
                    }
                } else if (event.key.code == sf::Keyboard::Up) {
                    int newRotation = (currentRotation + 1) % 4;
                    if (!checkCollision(currentPiece, newRotation, currentX, currentY))
                        currentRotation = newRotation;
                }
            }
        }

        // Only update gravity if not line clearing
        if (!lineClearActive && timer > delay) {
            update(dt);
            timer = 0;
        }

        // Handle line clearing timing
        if (lineClearActive) {
            lineClearTimer -= dt;
            if (lineClearTimer <= 0.f) {
                finalizeLineClear();
            }
        }

        window.clear(sf::Color::Black);
        render(window);
        window.display();
    }

    return 0;
}

void spawnPiece() {
    currentPiece = rand() % 7; 
    currentX = BOARD_WIDTH / 2 - 2;
    currentY = 0;
    currentRotation = 0;
    hasActivePiece = true;

    if (checkCollision(currentPiece, currentRotation, currentX, currentY)) {
        // Game Over scenario
        hasActivePiece = false;
    }
}

bool checkCollision(int piece, int rotation, int x, int y) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int block = getBlockValue(piece, rotation, i, j);
            if (block == 1) {
                int newX = x + j;
                int newY = y + i;
                if (newX < 0 || newX >= BOARD_WIDTH || newY < 0 || newY >= BOARD_HEIGHT)
                    return true;
                if (board[newY][newX] != 0)
                    return true;
            }
        }
    }
    return false;
}

void lockPiece() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            int block = getBlockValue(currentPiece, currentRotation, i, j);
            if (block == 1) {
                board[currentY + i][currentX + j] = currentPiece + 1; 
            }
        }
    }

    hasActivePiece = false;
    checkAndStartLineClear();
}

int getBlockValue(int piece, int rotation, int i, int j) {
    switch (rotation) {
        case 0: return shapes[piece][i][j];
        case 1: return shapes[piece][3-j][i];
        case 2: return shapes[piece][3-i][3-j];
        case 3: return shapes[piece][j][3-i];
    }
    return 0;
}

void update(float dt) {
    if (!hasActivePiece) {
        spawnPiece();
    }

    if (!lineClearActive && hasActivePiece) {
        int newY = currentY + 1;
        if (!checkCollision(currentPiece, currentRotation, currentX, newY)) {
            currentY = newY;
        } else {
            lockPiece();
        }
    }
}

void checkAndStartLineClear() {
    fullLines.clear();
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        bool full = true;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            fullLines.push_back(i);
        }
    }

    if (!fullLines.empty()) {
        lineClearActive = true;
        lineClearTimer = lineClearTotalTime; // 0.5f total
    }
}

void finalizeLineClear() {
    removeLines(fullLines);
    fullLines.clear();
    lineClearActive = false;
}

void removeLines(const std::vector<int>& lines) {
    std::vector<int> sortedLines = lines;
    std::sort(sortedLines.begin(), sortedLines.end());

    for (int idx = 0; idx < (int)sortedLines.size(); idx++) {
        int line = sortedLines[idx];
        for (int row = line; row > 0; row--) {
            for (int col = 0; col < BOARD_WIDTH; col++) {
                board[row][col] = board[row-1][col];
            }
        }
        for (int col = 0; col < BOARD_WIDTH; col++) {
            board[0][col] = 0;
        }
    }
}

void render(sf::RenderWindow &window) {
    sf::Sprite sprite(tileTexture);

    // Draw border
    sprite.setColor(sf::Color(128,128,128));
    for (int x = -1; x <= BOARD_WIDTH; x++) {
        // Top border
        sprite.setPosition((x + 1)*TILE_SIZE, 0);
        window.draw(sprite);
        // Bottom border
        sprite.setPosition((x + 1)*TILE_SIZE, (BOARD_HEIGHT + 1)*TILE_SIZE);
        window.draw(sprite);
    }
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        // Left border
        sprite.setPosition(0, (y + 1)*TILE_SIZE);
        window.draw(sprite);
        // Right border
        sprite.setPosition((BOARD_WIDTH + 1)*TILE_SIZE, (y + 1)*TILE_SIZE);
        window.draw(sprite);
    }

    // Line clear strobe effect
    bool flashing = lineClearActive && !fullLines.empty();
    float elapsedFlashTime = lineClearTotalTime - lineClearTimer; 
    // Flash every 0.07s
    float flashInterval = 0.07f;
    int flashIndex = (int)(elapsedFlashTime / flashInterval);
    bool whiteFlash = (flashIndex % 2 == 0); // Even intervals: white, odd: normal

    // Draw locked pieces
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j] != 0) {
                int pieceIndex = board[i][j] - 1;
                int posX = (j + 1) * TILE_SIZE;
                int posY = (i + 1) * TILE_SIZE;
                if (flashing && std::find(fullLines.begin(), fullLines.end(), i) != fullLines.end()) {
                    // Flashing line
                    if (whiteFlash) {
                        // Draw a pure white rectangle
                        sf::RectangleShape rect(sf::Vector2f(TILE_SIZE, TILE_SIZE));
                        rect.setFillColor(sf::Color::White);
                        rect.setPosition(posX, posY);
                        window.draw(rect);
                    } else {
                        // Normal color
                        sprite.setPosition(posX, posY);
                        sprite.setColor(pieceColors[pieceIndex]);
                        window.draw(sprite);
                    }
                } else {
                    // Normal locked tile
                    sprite.setPosition(posX, posY);
                    sprite.setColor(pieceColors[pieceIndex]);
                    window.draw(sprite);
                }
            }
        }
    }

    // Draw current piece if no line clearing is active (or you could choose to hide it during flashing)
    if (hasActivePiece && !lineClearActive) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                int block = getBlockValue(currentPiece, currentRotation, i, j);
                if (block == 1) {
                    int posX = (currentX + j + 1)*TILE_SIZE;
                    int posY = (currentY + i + 1)*TILE_SIZE;
                    sprite.setPosition(posX, posY);
                    sprite.setColor(pieceColors[currentPiece]);
                    window.draw(sprite);
                }
            }
        }
    }
}
