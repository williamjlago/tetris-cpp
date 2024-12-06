#include <cstdlib>
#include <ctime>
#include <SFML/Graphics.hpp>
#include <vector>
#include <algorithm>
#include <string>
#include <queue>

using namespace std;
std::queue<std::string> messageQueue;

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int TILE_SIZE = 32;
const int UI_WIDTH = 6;

int currentPiece = 0;
int nextPiece = 0;
int msgNum = 0;
float nextMessageDelay = 0.0f;
int currentX = BOARD_WIDTH / 2 - 2; 
int currentY = 0; 
int currentRotation = 0;
bool hasActivePiece = false;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};

sf::Color pieceColors[7] = {
    sf::Color::Cyan,      
    sf::Color::Yellow,    
    sf::Color::Magenta,   
    sf::Color::Green,     
    sf::Color::Red,       
    sf::Color::Blue,      
    sf::Color(255,165,0)  
};

sf::Texture tileTexture;
sf::Font gameFont;

// Line clearing
bool lineClearActive = false;
float lineClearTimer = 0.0f; 
float lineClearTotalTime = 0.5f; 
std::vector<int> fullLines;

// Scoring
int score = 0;

// Message scrolling
std::string currentMessage;
int messageOffset = 0; // how many characters have shifted into the field
float messageCharTimer = 0.0f; // timer for character-by-character movement
float messageCharInterval = 0.05f; // every 0.05s, shift one char
int charWidth = 24; // width of one character in pixels
float messageSpeedInChars = 1.0f; // 1 char per interval

bool checkCollision(int piece, int rotation, int x, int y);
int getBlockValue(int piece, int rotation, int i, int j);
void removeLines(const std::vector<int>& lines);
void spawnPiece();
void lockPiece();
void checkAndStartLineClear();
void finalizeLineClear();
void update(float dt);
void updateScore(int linesCleared);
void showMessage(const std::string &msg);
void render(sf::RenderWindow &window);

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

int main() {
    srand((unsigned)time(0));
    sf::RenderWindow window(sf::VideoMode((BOARD_WIDTH + 2 + UI_WIDTH) * TILE_SIZE, (BOARD_HEIGHT + 2) * TILE_SIZE), "Tetris");

    if (!tileTexture.loadFromFile("tile.png")) {
        return -1;
    }
    tileTexture.setSmooth(true);

    if (!gameFont.loadFromFile("nintendo-nes-font.ttf")) {
        return -1;
    }

    sf::Clock clock;
    float timer = 0;
    float delay = 0.5f;

    nextPiece = rand() % 7;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        timer += dt;
        messageCharTimer += dt;

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

        // Update gravity if not line clearing
        if (!lineClearActive && timer > delay) {
            update(dt);
            timer = 0;
        }

        // Line clearing timing
        if (lineClearActive) {
            lineClearTimer -= dt;
            if (lineClearTimer <= 0.f) {
                finalizeLineClear();
            }
        }

        // Update message scrolling (character by character)
        // Once every messageCharInterval, move text one character to the left
        if (!currentMessage.empty() && messageCharTimer > messageCharInterval) {
            messageCharTimer = 0.0f;
            // Shift the message by one character
            messageOffset += 1;
            // Once the entire message plus some buffer is off the left side, clear it
            int uiStartX = (BOARD_WIDTH + 2)*TILE_SIZE;
            int messageFieldWidth = UI_WIDTH * TILE_SIZE;
            int maxVisibleChars = messageFieldWidth / charWidth;
            // If offset > message.length + maxVisibleChars, message fully scrolled out
            if (messageOffset > (int)currentMessage.size() + maxVisibleChars) {
                currentMessage.clear();
                messageOffset = 0;
            }
        }

        if (currentMessage.empty() && !messageQueue.empty() && nextMessageDelay <= 0) {
            std::string nextMsg = messageQueue.front();
            messageQueue.pop();
            showMessage(nextMsg);
        }

        if (nextMessageDelay > 0) {
            nextMessageDelay -= dt;
        }

        window.clear(sf::Color::Black);
        render(window);
        window.display();
    }

    return 0;
}

void spawnPiece() {
    currentPiece = nextPiece;
    nextPiece = rand() % 7;

    currentX = BOARD_WIDTH / 2 - 2;
    currentY = 0;
    currentRotation = 0;
    hasActivePiece = true;

    if (checkCollision(currentPiece, currentRotation, currentX, currentY)) {
        hasActivePiece = false;
        messageQueue.push("GAME OVER!");
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

int getBlockValue(int piece, int rotation, int i, int j) {
    switch (rotation) {
        case 0: return shapes[piece][i][j];
        case 1: return shapes[piece][3-j][i];
        case 2: return shapes[piece][3-i][3-j];
        case 3: return shapes[piece][j][3-i];
    }
    return 0;
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
        lineClearTimer = lineClearTotalTime; 
    }
}

void finalizeLineClear() {
    int count = (int)fullLines.size();
    removeLines(fullLines);
    fullLines.clear();
    lineClearActive = false;

    updateScore(count);

    // Show a message based on how many lines were cleared
    if (count > 0) {
        if (count == 1) {
            msgNum = rand() % 7;
            switch(msgNum) {
                case 0:
                    messageQueue.push("CLEARED!");
                    break;
                case 1:
                    messageQueue.push("NICE!");
                    break;
                case 2:
                    messageQueue.push("AWESOME!");
                    break;
                case 3:
                    messageQueue.push("TUBULAR!");
                    break;
                case 4:
                    messageQueue.push("RADICAL!");
                    break;
                case 5:
                    messageQueue.push("OH YEAH!");
                    break;
                case 6:
                    messageQueue.push("SWEET!");  
                    break;             
            }
        }
        else if (count == 2) {
            messageQueue.push("DOUBLE!");
            messageQueue.push("3x PTS!");
        }
        else if (count == 3) {
            messageQueue.push("TRIPLE!");
            messageQueue.push("5x PTS!");
        }

        else if (count == 4) {
            messageQueue.push("TETRIS!");
            messageQueue.push("8x PTS!");
        }
    }
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

void updateScore(int linesCleared) {
    int points = 0;
    switch (linesCleared) {
        case 1: points = 100; break;
        case 2: points = 300; break;
        case 3: points = 500; break;
        case 4: points = 800; break;
        default: break;
    }
    score += points;
}

void showMessage(const std::string &msg) {
    currentMessage = msg;
    messageOffset = 0; // reset offset so message starts from right side
    // The message starts off the right side. We'll consider that the message is initially invisible until scrolled in.
}

void render(sf::RenderWindow &window) {
    sf::Sprite sprite(tileTexture);

    // Draw border around main board
    sprite.setColor(sf::Color(128,128,128));
    for (int x = -1; x <= BOARD_WIDTH; x++) {
        // Top
        sprite.setPosition((x + 1)*TILE_SIZE, 0);
        window.draw(sprite);
        // Bottom
        sprite.setPosition((x + 1)*TILE_SIZE, (BOARD_HEIGHT + 1)*TILE_SIZE);
        window.draw(sprite);
    }
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        // Left
        sprite.setPosition(0, (y + 1)*TILE_SIZE);
        window.draw(sprite);
        // Right
        sprite.setPosition((BOARD_WIDTH + 1)*TILE_SIZE, (y + 1)*TILE_SIZE);
        window.draw(sprite);
    }

    bool flashing = lineClearActive && !fullLines.empty();
    float elapsedFlashTime = lineClearTotalTime - lineClearTimer; 
    float flashInterval = 0.07f; 
    int flashIndex = (int)(elapsedFlashTime / flashInterval);
    bool whiteFlash = (flashIndex % 2 == 0);

    // Draw locked pieces
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j] != 0) {
                int pieceIndex = board[i][j] - 1;
                int posX = (j + 1) * TILE_SIZE;
                int posY = (i + 1) * TILE_SIZE;
                if (flashing && std::find(fullLines.begin(), fullLines.end(), i) != fullLines.end()) {
                    if (whiteFlash) {
                        sf::RectangleShape rect(sf::Vector2f(TILE_SIZE, TILE_SIZE));
                        rect.setFillColor(sf::Color::White);
                        rect.setPosition(posX, posY);
                        window.draw(rect);
                    } else {
                        sprite.setPosition(posX, posY);
                        sprite.setColor(pieceColors[pieceIndex]);
                        window.draw(sprite);
                    }
                } else {
                    sprite.setPosition(posX, posY);
                    sprite.setColor(pieceColors[pieceIndex]);
                    window.draw(sprite);
                }
            }
        }
    }

    // Draw current piece if no line clearing is active
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

    // UI Area
    int uiStartX = (BOARD_WIDTH + 2) * TILE_SIZE; 
    int uiStartY = TILE_SIZE;

    // NEXT label
    {
        sf::Text nextText("NEXT", gameFont, 24);
        nextText.setFillColor(sf::Color::White);
        nextText.setPosition(uiStartX, uiStartY);
        window.draw(nextText);

        // Draw next piece
        int nextPieceStartY = uiStartY + 40; 
        sf::Sprite nextSprite(tileTexture);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                int block = shapes[nextPiece][i][j];
                if (block == 1) {
                    int posX = uiStartX + j*TILE_SIZE;
                    int posY = nextPieceStartY + i*TILE_SIZE;
                    nextSprite.setPosition(posX, posY);
                    nextSprite.setColor(pieceColors[nextPiece]);
                    window.draw(nextSprite);
                }
            }
        }
    }

    // Score
    {
        int scoreY = uiStartY + 200;
        sf::Text scoreText("SCORE:\n" + std::to_string(score), gameFont, 24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(uiStartX, scoreY);
        window.draw(scoreText);
    }

    // Message Field
    {
        int messageFieldY = uiStartY + 320;
        int messageFieldWidth = UI_WIDTH * TILE_SIZE; 
        int messageFieldHeight = 32; 
        int messageFieldX = uiStartX;

        // Draw border tiles above and below the message field, similar to main board
        for (int x = 0; x < UI_WIDTH; x++) {
            sprite.setPosition(messageFieldX + x*TILE_SIZE, messageFieldY - TILE_SIZE);
            sprite.setColor(sf::Color(128,128,128));
            window.draw(sprite);
        }
        for (int x = 0; x < UI_WIDTH; x++) {
            sprite.setPosition(messageFieldX + x*TILE_SIZE, messageFieldY + messageFieldHeight);
            sprite.setColor(sf::Color(128,128,128));
            window.draw(sprite);
        }

        // Background for message field
        sf::RectangleShape msgBg(sf::Vector2f((float)messageFieldWidth, (float)messageFieldHeight));
        msgBg.setFillColor(sf::Color(0,0,0,200));
        msgBg.setPosition((float)messageFieldX, (float)messageFieldY);
        window.draw(msgBg);

        if (!currentMessage.empty()) {

            int maxVisibleChars = messageFieldWidth / charWidth;
            nextMessageDelay = 0.5f;

            int startIndex = (int)currentMessage.size() - messageOffset;
            if (startIndex < 0) startIndex = 0;

            // The visible substring is up to startIndex+maxVisibleChars, but capped by message length
            int endIndex = startIndex + maxVisibleChars;
            if (endIndex > (int)currentMessage.size()) endIndex = (int)currentMessage.size();

            std::string visibleText;
            if (startIndex < endIndex) {
                visibleText = currentMessage.substr(startIndex, endIndex - startIndex);
            }

            int textLengthPx = (int)visibleText.size() * charWidth;
            int textPosX = messageFieldX + (messageFieldWidth - textLengthPx);
            int textPosY = messageFieldY + 5; // slight padding

            sf::Text msgText(visibleText, gameFont, 24);
            msgText.setFillColor(sf::Color::White);
            msgText.setPosition((float)textPosX, (float)textPosY);
            window.draw(msgText);
        }
    }
}
