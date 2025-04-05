#include "raylib.h"

#include "score.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <stdlib.h>
#include <string.h>

Font font;

int const GRID_HORIZONTAL_SIZE = 16;
int const GRID_VERTICAL_SIZE = 22;
int const BLOCK_SIZE = 27;

int const TOTAL_PIECES_TYPES = 7;

int const screenWidth = 1150;
int const screenHeight = 594;

// Settings
bool audioEnabled = true;
Rectangle muteButton = {(float)screenWidth - 80, 35, 80, 40};
bool isMuted = false;

// Themes
bool isGrayBackground = false;
Color pieceColor = MAROON;

// High score entry variables
char playerName[NAME_LEN] = "";
int playerNameLength = 0;
bool isHighScore = false;
float cursorBlinkTimer = 0.0f;
bool showCursor = true;

int const MAX_STARS = 25;
Vector2 stars[MAX_STARS];

int level = 1;
int linesClearedTotal = 0;
int linesClearedThisLevel = 0;
int const BASE_LINES_PER_LEVEL = 1;
int const LINES_INCREMENT_PER_LEVEL = 1;
float baseFallSpeed = 0.3f;

float gameTime = 0.0f;
float lateralTimer = 0.0f;
float lateralSpeed = 0.07f;
float fallTimer = 0.0f;
float fallSpeed = 0.3f;
float fallVelocity = 0.0f;
float fallAcceleration = 4.5f;
float bottomedTimer = 0.0f;

const int GRID_OFFSET_X = (screenWidth - GRID_HORIZONTAL_SIZE * BLOCK_SIZE) / 2;
const int GRID_OFFSET_Y = (screenHeight - GRID_VERTICAL_SIZE * BLOCK_SIZE) / 2;

int grid[GRID_VERTICAL_SIZE][GRID_HORIZONTAL_SIZE] = {0};
bool showGrid = false;

bool isMenu = true;
bool gameOver = false;
bool pause = false;
bool isInFreeFall = false;
bool isPlayerVisible = true;

float pulseTimer = 0.0f;
bool showPulseEffect = false;
float const PULSE_DURATION = 2.5f;

bool isElectrocuted = false;
float electrocutionTimer = 0.0f;
const float ELECTROCUTION_DURATION = 2.0f;

struct Unit
{
    Vector2 position;
};

enum PieceState
{
    NEW,
    FALL,
    FREE_FALL,
    BOTTOMED,
    LOCKED
};

struct Tetromino
{
    Unit units[4];
    int size = 4;
    PieceState pieceState = NEW;
};

struct Particle
{
    Vector2 position;
    Vector2 velocity;
    Color color;
    float alpha;
    float size;
    float life;
};
int const MAX_PARTICLES = 100;
Particle particles[MAX_PARTICLES];
int particleCount = 0;

Sound doorHitSound;
Sound levelStartSound;

enum GameState
{
    HOME,
    HOW_TO_PLAY,
    RULES,
    PLAYING,
    LEVEL_TRANSITION,
    GAME_OVER,
    HIGH_SCORE_ENTRY
};
GameState gameState = HOME;

enum Language
{
    ENGLISH,
    PORTUGUESE,
    GERMAN
};
Language currentLanguage = ENGLISH;

// Flag textures
Texture2D flagPortugal;
Texture2D flagGermany;
Texture2D flagUK;

// Flag buttons (increased size and spacing for better usability)
Rectangle flagButtonPortugal = {20, screenHeight - 85, 100, 70};
Rectangle flagButtonGermany = {140, screenHeight - 85, 100, 70};
Rectangle flagButtonUK = {260, screenHeight - 85, 100, 70};

struct PlayerUnit
{
    Vector2 position;
    Color color;
    float width;
    float height;
};

struct Player
{
    Vector2 position;
    Vector2 velocity;
    PlayerUnit units[12];
    int size = 12;
    float unitSize = 50.0f;
    bool isJumping = false;
};
Player player;

struct Platform
{
    Vector2 position;
    float width = 100.0f;
    float height = 20.0f;
};
Platform platforms[5];
const int NUM_PLATFORMS = 5;

struct Door
{
    Vector2 position;
    float width = 55.0f;
    float height = 120.0f;
};
Door door;

float transitionTimer = 0.0f;
const float TRANSITION_DURATION = 10.0f;

bool doorHit = false;
float doorEffectTimer = 0.0f;
const float DOOR_EFFECT_DURATION = 1.0f;
bool screenShake = false;
float shakeTimer = 0.0f;
const float SHAKE_DURATION = 1.5f;

Tetromino currentPiece;
void spawnI(Tetromino &piece);
void spawnJ(Tetromino &piece);
void spawnL(Tetromino &piece);
void spawnO(Tetromino &piece);
void spawnS(Tetromino &piece);
void spawnT(Tetromino &piece);
void spawnZ(Tetromino &piece);
void spawnPiece();
void UpdateGame();
void DrawGame();
void UnloadGame();
void UpdateDrawFrame(float gameTime);
void DrawPiece(Tetromino *piece);

Vector2 fromGrid(Vector2 position);
Vector2 toGrid(Vector2 position);

Tetromino rotatePiece(Tetromino Piece);
bool canMoveHorizontally(Tetromino currentPiece, int amount);
bool canMoveDown(Tetromino piece);
void moveDown(Tetromino &currentPiece);

int score = 0;
bool justClearedGrid = false;

void gridBackground()
{
    isGrayBackground = !isGrayBackground;
}

void UpdateAudioMute()
{
    Vector2 mousePoint = GetMousePosition();
    // Check if the mute button is clicked
    if ((CheckCollisionPointRec(mousePoint, muteButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ||
        (IsKeyPressed('M') && playerNameLength >= NAME_LEN))
    {
        isMuted = !isMuted; // Toggle mute state

        if (isMuted)
        {
            // Mute audio
            SetMasterVolume(0.0f); // Set volume to 0
        }
        else
        {
            // Unmute audio
            SetMasterVolume(1.0f); // Restore full volume
        }
    }
}

bool CheckHighScore(int score)
{
    ScoreEntry *scores = getScores();

    // Check if score is higher than the lowest score on the list
    // or if there are empty slots
    for (int i = 0; i < MAX_SCORES; i++)
    {
        if (strcmp(scores[i].name, "Empty") == 0 || score > scores[i].linesCleared)
        {
            return true;
        }
    }

    return false;
}

void drawHighScore()
{
    gameOver = true; // Load the scores first

    loadScoresFromFile();

    isHighScore = CheckHighScore(linesClearedTotal);

    if (isHighScore)
    {
        // Reset name entry variables
        playerName[0] = '\0';
        playerNameLength = 0;
        cursorBlinkTimer = 0.0f;
        showCursor = true;
        gameState = HIGH_SCORE_ENTRY;
    }
    else
    {
        gameState = GAME_OVER;
    }
}

void StartScreenShake()
{
    screenShake = true;
    shakeTimer = SHAKE_DURATION;
}

void CreateElectrocutionEffect(Vector2 position)
{
    isElectrocuted = true;
    electrocutionTimer = ELECTROCUTION_DURATION;
    StartScreenShake();
}

void CreateDoorHitEffect(Vector2 position)
{
    showPulseEffect = true;
    pulseTimer = 3.5f;
    StartScreenShake();

    if (audioEnabled && !isMuted)
        PlaySound(doorHitSound);

    int particlesToSpawn = 777;
    for (int i = 0; i < particlesToSpawn && particleCount < MAX_PARTICLES; i++)
    {
        Particle &p = particles[particleCount];
        p.position = position;

        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(200, 400);

        p.velocity = {cosf(angle) * speed, sinf(angle) * speed};
        p.color = (i % 3 == 0) ? GOLD : BLACK; // Alternate between gold and black
        p.alpha = 1.0f;
        p.size = (float)GetRandomValue(7, 12);
        p.life = GetRandomValue(5, 35) / 10.0f; // 0.5 to 3.5 seconds
        particleCount++;
    }

    doorHit = true;
    doorEffectTimer = DOOR_EFFECT_DURATION;
}

void drawLevelTransition()
{
    gameState = LEVEL_TRANSITION;
    level++;
    fallSpeed = baseFallSpeed / (1.0f + (level - 1) * 0.1f);
    for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
        for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
            grid[y][x] = 0;
    score = 0;
    linesClearedThisLevel = 0;
    gameOver = false;
    doorHit = false;
    doorEffectTimer = 0.0f;
    showPulseEffect = false;
    pulseTimer = 0.0f;
    particleCount = 0;

    player.position = {(float)screenWidth - 50, 25.0f};
    player.velocity = {0, 0};
    player.isJumping = false;

    player.units[0] = {{0, -27}, {255, 204, 153, 255}, 14.0f, 17.0f}; // Head (skin tone)
    player.units[1] = {{0, 0}, {0, 128, 255, 255}, 18.0f, 25.0f};     // Torso
    player.units[2] = {{-13, -3}, {204, 153, 102, 255}, 4.5f, 24.0f}; // Left arm
    player.units[3] = {{5, -3}, {204, 153, 102, 255}, 4.5f, 24.0f};   // Right arm
    player.units[4] = {{-5, 25}, {0, 0, 255, 255}, 7.0f, 28.0f};      // Left leg
    player.units[5] = {{6, 25}, {0, 0, 255, 255}, 7.0f, 28.0f};       // Right leg
    player.units[6] = {{0, -35}, {0, 0, 0, 255}, 15.0f, 6.0f};        // Hat or hair
    player.units[7] = {{-4, -29}, {0, 0, 0, 255}, 3.0f, 3.0f};        // Left eye
    player.units[8] = {{1, -29}, {0, 0, 0, 255}, 3.0f, 3.0f};         // Right eye
    player.units[9] = {{-2, -24}, {255, 102, 102, 255}, 7.0f, 2.0f};  // Mouth
    player.units[10] = {{-2, -22}, {255, 102, 102, 255}, 5.0f, 1.0f}; // Mouth (smile)
    player.units[11] = {{0, -16}, {255, 204, 153, 255}, 6, 8};        // Neck

    // Initialize platforms
    float startX = screenWidth - platforms[0].width;
    float spacing = 220.0f;
    for (int i = 0; i < NUM_PLATFORMS; i++)
    {
        float platformX = startX - i * spacing;
        float minY = screenHeight - 250;
        float maxY = screenHeight - 50;
        float platformY = (float)GetRandomValue(minY, maxY);
        platforms[i].position = {platformX, platformY};
    }

    door.position = {platforms[NUM_PLATFORMS - 1].position.x - door.width / 2 - 70,
                     platforms[NUM_PLATFORMS - 1].position.y - door.height / 2 + platforms[0].height / 2};

    transitionTimer = TRANSITION_DURATION;
    currentPiece.pieceState = NEW;
    spawnPiece();
}

void UpdateLevelTransition(float deltaTime)
{
    if (pause)
        return;

    if (doorHit)
    {
        currentPiece.pieceState = NEW;
        doorEffectTimer -= deltaTime;
        isPlayerVisible = false;
        if (doorEffectTimer <= 0.0f)
        {
            isPlayerVisible = true;
            doorHit = false;
            gameState = PLAYING;
            spawnPiece();
        }
        return;
    }
    if (isElectrocuted)
    {
        electrocutionTimer -= deltaTime;
        if (electrocutionTimer <= 0.0f)
        {
            gameOver = true;
            isElectrocuted = false;
            drawHighScore();
            return;
        }
        return;
    }

    const float GRAVITY = 500.0f;
    player.velocity.y += GRAVITY * deltaTime;

    const float MOVE_SPEED = 200.0f;
    if (IsKeyDown(KEY_LEFT))
    {
        player.velocity.x = -MOVE_SPEED;
    }
    else if (IsKeyDown(KEY_RIGHT))
    {
        player.velocity.x = MOVE_SPEED;
    }
    else
    {
        player.velocity.x = 0;
    }

    if (IsKeyPressed(KEY_SPACE) && !player.isJumping)
    {
        player.velocity.y = -450.0f;
        player.isJumping = true;
    }

    else if (IsKeyPressed('C'))
    {
        gameState = PLAYING;
        currentPiece.pieceState = NEW;
    }

    player.position.x += player.velocity.x * deltaTime;
    player.position.y += player.velocity.y * deltaTime;

    float boundingWidth = 24.0f;
    float boundingHeight = 50.0f;
    if (player.position.x - boundingWidth / 2 < 0 || player.position.x + boundingWidth / 2 > screenWidth)
    {
        Vector2 playerCenter = {player.position.x, player.position.y};
        CreateElectrocutionEffect(playerCenter);
        return;
    }

    Rectangle playerRect = {player.position.x - boundingWidth / 2, player.position.y - boundingHeight / 2,
                            boundingWidth, boundingHeight};

    for (int i = 0; i < NUM_PLATFORMS; i++)
    {
        Rectangle platformRect = {platforms[i].position.x, platforms[i].position.y, platforms[i].width,
                                  platforms[i].height};

        if (CheckCollisionRecs(playerRect, platformRect))
        {
            float dx = (playerRect.x + playerRect.width / 2) - (platformRect.x + platformRect.width / 2);
            float dy = (playerRect.y + playerRect.height / 2) - (platformRect.y + platformRect.height / 2);
            float overlapX = abs(dx) - (playerRect.width / 2 + platformRect.width / 2);
            float overlapY = abs(dy) - (playerRect.height / 2 + platformRect.height / 2);

            if (overlapX < 0 && overlapY < 0)
            {
                if (abs(overlapX) < abs(overlapY))
                {
                    if (dx > 0)
                    {
                        player.position.x = platformRect.x + platformRect.width + boundingWidth / 2;
                    }
                    else
                    {
                        player.position.x = platformRect.x - boundingWidth / 2;
                    }
                    player.velocity.x = 0;
                }
                else
                {
                    if (dy > 0)
                    {
                        player.position.y = platformRect.y + platformRect.height + boundingHeight / 2;
                        player.velocity.y = 0; // Stop vertical movement
                    }
                    else if (dy < 0 && player.velocity.y > 0)
                    {
                        player.position.y = platformRect.y - boundingHeight / 2;
                        player.velocity.y = 0;
                        player.isJumping = false;
                    }
                }
            }
        }

        Rectangle doorRect = {door.position.x, door.position.y, door.width, door.height};
        if (CheckCollisionRecs(playerRect, doorRect))
        {
            Vector2 doorCenter = {door.position.x + door.width / 2, door.position.y + door.height / 2};
            CreateDoorHitEffect(doorCenter);

            return;
        }

        if (player.position.y - boundingHeight / 2 > screenHeight)
        {
            gameOver = true;
            drawHighScore();
            return;
        }
    }

    transitionTimer -= deltaTime;
    if (transitionTimer <= 0.0f)
    {
        gameOver = true;
        drawHighScore();
    }
}

void DrawPulseEffect(float deltaTime)
{
    if (!showPulseEffect)
        return;

    pulseTimer -= deltaTime;
    if (pulseTimer <= 0)
    {
        showPulseEffect = false;
        return;
    }

    float progress = 1.0f - (pulseTimer / PULSE_DURATION);
    float maxRadius = sqrtf(powf(screenWidth, 2) + powf(screenHeight, 2)) / 2;

    Vector2 center = {(float)screenWidth / 2, (float)screenHeight / 2};

    for (int i = 0; i < 3; i++)
    {
        float ringProgress = progress + (i * 0.3f);
        if (ringProgress > 1.0f)
            continue;

        float ringRadius = maxRadius * ringProgress;
        Color ringColor = DARKBLUE;
        ringColor.a = (unsigned char)((1.0f - ringProgress) * 255 * 0.7f);

        // Draw thick ring using multiple circles
        for (int thickness = -2; thickness <= 2; thickness++)
        {
            DrawCircleLines((int)center.x, (int)center.y, ringRadius + (thickness * 2), ringColor);
        }

        // Add some sparkles at the edge
        if (particleCount < MAX_PARTICLES - 8)
        {
            for (int j = 0; j < 8; j++)
            {
                float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                Particle &p = particles[particleCount];
                p.position = {center.x + cosf(angle) * ringRadius, center.y + sinf(angle) * ringRadius};
                p.velocity = {cosf(angle) * GetRandomValue(100, 200) / 100.0f,
                              sinf(angle) * GetRandomValue(100, 200) / 100.0f};
                p.color = {255, 215, 0, 255}; // Gold sparkles
                p.alpha = 1.0f;
                p.size = (float)GetRandomValue(3, 8);
                p.life = 0.5f;
                particleCount++;
            }
        }
    }
}

void CreateLineClearEffect(int y)
{
    int particlesPerBlock = 4;
    for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
    {
        for (int p = 0; p < particlesPerBlock && particleCount < MAX_PARTICLES; p++)
        {
            Particle &particle = particles[particleCount];
            particle.position = {(float)(GRID_OFFSET_X + x * BLOCK_SIZE + (float)BLOCK_SIZE / 2),
                                 (float)(GRID_OFFSET_Y + y * BLOCK_SIZE + (float)BLOCK_SIZE / 2)};
            particle.velocity = {(float)(GetRandomValue(-200, 200)) / 100.0f,
                                 (float)(GetRandomValue(-200, 200)) / 100.0f};
            particle.color = MAROON;
            particle.alpha = 1.0f;
            particle.size = (float)GetRandomValue(5, 15);
            particle.life = 0.5f + GetRandomValue(0, 100) / 100.0f;
            particleCount++;
        }
    }
}

void UpdateDrawParticles(float deltaTime)
{
    for (int i = particleCount - 1; i >= 0; i--)
    {
        Particle &p = particles[i];

        p.position.x += p.velocity.x * deltaTime * 60;
        p.position.y += p.velocity.y * deltaTime * 60;
        p.life -= deltaTime;
        p.alpha = p.life / 1.5f;

        // Draw particle
        Color particleColor = p.color;
        particleColor.a = (unsigned char)(p.alpha * 255);
        DrawRectanglePro({p.position.x, p.position.y, p.size, p.size}, {p.size / 2, p.size / 2}, GetTime() * 90,
                         particleColor);

        // Remove dead particles
        if (p.life <= 0)
        {
            particles[i] = particles[particleCount - 1];
            particleCount--;
        }
    }
}

void DrawGrid()
{
    int gridWidth = GRID_HORIZONTAL_SIZE * BLOCK_SIZE;
    int gridHeight = GRID_VERTICAL_SIZE * BLOCK_SIZE;
    int gridX = GRID_OFFSET_X;
    int gridY = GRID_OFFSET_Y;

    DrawRectangle(gridX, gridY, gridWidth, gridHeight, isGrayBackground ? GRAY : LIGHTGRAY);

    DrawRectangleLines(gridX, gridY, gridWidth, gridHeight, BLACK);

    if (showGrid)
    {
        for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
        {
            for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
            {
                int screenX = GRID_OFFSET_X + x * BLOCK_SIZE;
                int screenY = GRID_OFFSET_Y + y * BLOCK_SIZE;

                if (grid[y][x] == 1)
                {
                    DrawRectangle(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE, pieceColor);
                }
                else
                {
                    DrawRectangleLines(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE, BLACK);
                }
            }
        }
    }
    for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
    {
        for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
        {
            int screenX = GRID_OFFSET_X + x * BLOCK_SIZE;
            int screenY = GRID_OFFSET_Y + y * BLOCK_SIZE;
            if (grid[y][x] == 1)
            {
                DrawRectangle(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE, pieceColor);
            }
        }
    }
}

void DrawPiece(Tetromino *piece)
{
    for (int i = 0; i < piece->size; i++)
    {
        int screenX = GRID_OFFSET_X + (int)piece->units[i].position.x * BLOCK_SIZE;
        int screenY = GRID_OFFSET_Y + (int)piece->units[i].position.y * BLOCK_SIZE;

        DrawRectangle(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE, pieceColor);
    }
}

int checkAndClearLines()
{
    int linesCleared = 0;

    int y = GRID_VERTICAL_SIZE - 1;
    while (y >= 0)
    {
        if (y < 0 || y >= GRID_VERTICAL_SIZE)
        {
            break;
        }

        bool isFull = true;
        for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
        {
            if (grid[y][x] == 0)
            {
                isFull = false;
                break;
            }
        }

        if (isFull)
        {
            linesCleared++;
            CreateLineClearEffect(y);

            for (int aboveY = y; aboveY > 0; aboveY--)
            {
                for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                {
                    grid[aboveY][x] = grid[aboveY - 1][x];
                }
            }

            // Clear the top line
            for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
            {
                grid[0][x] = 0;
            }
        }
        else
        {
            y--;
        }
    }
    score += linesCleared * 10 * level;
    linesClearedTotal += linesCleared;
    linesClearedThisLevel += linesCleared;

    bool isGridEmpty = true;
    for (int y = 0; y < GRID_VERTICAL_SIZE && isGridEmpty; y++)
    {
        for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
        {
            if (grid[y][x] != 0)
            {
                isGridEmpty = false;
                break;
            }
        }
    }

    if (isGridEmpty && linesCleared > 0)
    {
        score += 50;
        justClearedGrid = true;
        showPulseEffect = true;
        pulseTimer = PULSE_DURATION;
    }
    else
    {
        justClearedGrid = false;
    }

    int linesNeeded = level * level;

    if (linesClearedThisLevel >= linesNeeded)
    {
        drawLevelTransition();
    }
    return linesCleared;
}

int main()
{

    ScoreEntry *latestScores = getScores();

    printf("Top %d Scores:\n", MAX_SCORES);
    for (int i = 0; i < MAX_SCORES; i++)
    {
        printf("%d. %s - %d\n", i + 1, latestScores[i].name, latestScores[i].linesCleared);
    }

    srand(time(0));
    InitWindow(screenWidth, screenHeight, "Classic Game: TETRIS");
    flagPortugal = LoadTexture("resources/flag_portugal.jpeg");
    flagGermany = LoadTexture("resources/flag_germany.jpeg");
    flagUK = LoadTexture("resources/flag_uk.jpeg");

    InitAudioDevice();
    levelStartSound = LoadSound("resources/level-Start-Sound.mp3");
    SetSoundVolume(levelStartSound, 57.0f);
    doorHitSound = LoadSound("resources/next-level.mp3");
    SetSoundVolume(doorHitSound, 0.1f);
    if (isMuted)
    {
        SetMasterVolume(0.0f); // Start muted if isMuted is true
    }
    else
    {
        SetMasterVolume(1.0f); // Start at full volume
    }

    SetTargetFPS(60);

    font = LoadFontEx("resources/font.ttf", 96, 0, 0);

    // initialize stars
    for (int i = 0; i < MAX_STARS; i++)
    {
        stars[i].x = GetRandomValue(0, screenWidth);
        stars[i].y = GetRandomValue(0, screenHeight); // Use screenHeight for Tetris
    }

    while (!WindowShouldClose())
    {
        gameTime += GetFrameTime();

        switch (gameState)
        {
        case HOME:
            if (IsKeyPressed(KEY_ENTER))
            {
                if (audioEnabled && !isMuted)
                    PlaySound(levelStartSound);
                gameState = PLAYING;
                for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
                    for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                        grid[y][x] = 0;
                score = 0;
                level = 1;
                linesClearedTotal = 0;
                linesClearedThisLevel = 0;
                fallSpeed = baseFallSpeed;
                currentPiece.pieceState = NEW;
                spawnPiece();
                gameOver = false;

                for (int i = 0; i < MAX_STARS; i++)
                {
                    stars[i].x = GetRandomValue(0, screenWidth);
                    stars[i].y = GetRandomValue(0, screenHeight);
                }
            }

            if (IsKeyPressed(KEY_SPACE))
                gameState = HOW_TO_PLAY;

            else if (IsKeyPressed('R'))
                gameState = RULES;
            break;

        case HOW_TO_PLAY:
            if (IsKeyPressed(KEY_ENTER))
            {
                if (audioEnabled && !isMuted)
                    PlaySound(levelStartSound);
                gameState = PLAYING;
                for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
                    for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                        grid[y][x] = 0;
                score = 0;
                level = 1;
                linesClearedTotal = 0;
                linesClearedThisLevel = 0;
                fallSpeed = baseFallSpeed;
                currentPiece.pieceState = NEW;
                spawnPiece();
                gameOver = false;

                for (int i = 0; i < MAX_STARS; i++)
                {
                    stars[i].x = GetRandomValue(0, screenWidth);
                    stars[i].y = GetRandomValue(0, screenHeight);
                }
            }
            if (IsKeyPressed('H'))
                gameState = HOME;
            break;

        case RULES:
            if (IsKeyPressed(KEY_ENTER))
            {
                if (audioEnabled && !isMuted)
                    PlaySound(levelStartSound);
                gameState = PLAYING;
                for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
                    for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                        grid[y][x] = 0;
                score = 0;
                level = 1;
                linesClearedTotal = 0;
                linesClearedThisLevel = 0;
                fallSpeed = baseFallSpeed;
                currentPiece.pieceState = NEW;
                spawnPiece();
                gameOver = false;

                for (int i = 0; i < MAX_STARS; i++)
                {
                    stars[i].x = GetRandomValue(0, screenWidth);
                    stars[i].y = GetRandomValue(0, screenHeight);
                }
            }
            if (IsKeyPressed('H'))
                gameState = HOME;
            break;

        case PLAYING:
            if (IsKeyPressed('P'))
                pause = !pause;
            if (IsKeyPressed('H'))
                gameState = HOME;

        case LEVEL_TRANSITION:
            if (IsKeyPressed('P'))
                pause = !pause;
            if (IsKeyPressed('H'))
                gameState = HOME;

        case GAME_OVER:
            if (IsKeyPressed(KEY_ENTER))
            {
                if (audioEnabled && !isMuted)
                    PlaySound(levelStartSound);

                // Not a high score, just reset game
                for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
                    for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                        grid[y][x] = 0;

                score = 0;
                level = 1;
                linesClearedTotal = 0;
                linesClearedThisLevel = 0;
                fallSpeed = baseFallSpeed;
                currentPiece.pieceState = NEW;
                spawnPiece();
                gameOver = false;
                gameState = PLAYING;
            }
            if (IsKeyPressed('H'))
            {
                gameState = HOME;
                gameOver = false;
            }
            break;

        case HIGH_SCORE_ENTRY:
            // Blink the cursor
            cursorBlinkTimer += GetFrameTime();
            if (cursorBlinkTimer > 0.5f)
            {
                cursorBlinkTimer = 0;
                showCursor = !showCursor;
            }
            if (playerNameLength < NAME_LEN)
            {
                int key = GetCharPressed();

                // Check for backspace
                if (IsKeyPressed(KEY_BACKSPACE) && playerNameLength > 0)
                {
                    playerNameLength--;
                    playerName[playerNameLength] = '\0';
                }
                // Add character if not at max length
                else if (key > 0 && playerNameLength < NAME_LEN - 1)
                {
                    // Only allow alphanumeric characters and space
                    if ((key >= 32 && key <= 125) && key != '/')
                    {
                        playerName[playerNameLength] = (char)key;
                        playerNameLength++;
                        playerName[playerNameLength] = '\0';
                    }
                }
            }

            // Handle Enter key presses
            if (IsKeyPressed(KEY_ENTER))
            {
                if (playerNameLength >= NAME_LEN)
                {
                    // Start new game
                    if (audioEnabled && !isMuted)
                        PlaySound(levelStartSound);
                    for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
                        for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                            grid[y][x] = 0;
                    score = 0;
                    level = 1;
                    linesClearedTotal = 0;
                    linesClearedThisLevel = 0;
                    fallSpeed = baseFallSpeed;
                    currentPiece.pieceState = NEW;
                    spawnPiece();
                    gameOver = false;
                    gameState = PLAYING;

                    for (int i = 0; i < MAX_STARS; i++)
                    {
                        stars[i].x = GetRandomValue(0, screenWidth);
                        stars[i].y = GetRandomValue(0, screenHeight);
                    }

                    playerName[0] = '\0';
                    playerNameLength = 0;
                    showCursor = true;
                }
                else // First Enter: submit score
                {
                    // If no name entered, use default
                    if (playerNameLength == 0)
                    {
                        strcpy(playerName, "Anonymous");
                        playerNameLength = strlen(playerName);
                    }
                    insertScore(playerName, linesClearedTotal);
                    saveScoresToFile();
                    playerNameLength = NAME_LEN; // Use as flag to indicate submission
                }
            }
            // Handle keyboard input for name
            if (IsKeyPressed('H') && playerNameLength >= NAME_LEN)
            {
                gameState = HOME;
                gameOver = false;
            }
            break;
        }
        UpdateDrawFrame(gameTime);
    }
    UnloadGame();
    CloseWindow();
    return 0;
}

void spawnPiece(void)
{
    int randomNumber = 0;
    randomNumber = (rand() / (RAND_MAX / 7)) + 1;

    if (randomNumber == 1)
        spawnI(currentPiece);
    else if (randomNumber == 2)
        spawnJ(currentPiece);
    else if (randomNumber == 3)
        spawnL(currentPiece);
    else if (randomNumber == 4)
        spawnO(currentPiece);
    else if (randomNumber == 5)
        spawnS(currentPiece);
    else if (randomNumber == 6)
        spawnT(currentPiece);
    else if (randomNumber == 7)
        spawnZ(currentPiece);

    fallTimer = 0.0f;
    isInFreeFall = false;
    fallSpeed = baseFallSpeed / (1.0f + (level - 1) * 0.1f);

    assert(currentPiece.pieceState == LOCKED || currentPiece.pieceState == NEW);
    currentPiece.pieceState = FALL;
}

void spawnI(Tetromino &piece)
{
    piece.units[0].position = (Vector2){(float)GRID_HORIZONTAL_SIZE / 2, 0};
    piece.units[1].position = (Vector2){piece.units[0].position.x - 1, 0};
    piece.units[2].position = (Vector2){piece.units[0].position.x + 1, 0};
    piece.units[3].position = (Vector2){piece.units[0].position.x + 2, 0};
}

void spawnJ(Tetromino &piece)
{
    piece.units[0].position = (Vector2){(float)GRID_HORIZONTAL_SIZE / 2, 0};
    piece.units[1].position = (Vector2){piece.units[0].position.x, 1};
    piece.units[2].position = (Vector2){piece.units[0].position.x, 2};
    piece.units[3].position = (Vector2){piece.units[0].position.x - 1, 2};
}

void spawnL(Tetromino &piece)
{
    piece.units[0].position = (Vector2){(float)GRID_HORIZONTAL_SIZE / 2, 0};
    piece.units[1].position = (Vector2){piece.units[0].position.x, 1};
    piece.units[2].position = (Vector2){piece.units[0].position.x, 2};
    piece.units[3].position = (Vector2){piece.units[0].position.x + 1, 2};
}

void spawnO(Tetromino &piece)
{
    piece.units[0].position = (Vector2){(float)GRID_HORIZONTAL_SIZE / 2, 0};
    piece.units[1].position = (Vector2){piece.units[0].position.x + 1, 0};
    piece.units[2].position = (Vector2){piece.units[0].position.x, 1};
    piece.units[3].position = (Vector2){piece.units[0].position.x + 1, 1};
}

void spawnS(Tetromino &piece)
{
    piece.units[0].position = (Vector2){(float)GRID_HORIZONTAL_SIZE / 2, 0};
    piece.units[1].position = (Vector2){piece.units[0].position.x + 1, 0};
    piece.units[2].position = (Vector2){piece.units[0].position.x, 1};
    piece.units[3].position = (Vector2){piece.units[0].position.x - 1, 1};
}

void spawnT(Tetromino &piece)
{
    piece.units[0].position = (Vector2){(float)GRID_HORIZONTAL_SIZE / 2, 0};
    piece.units[1].position = (Vector2){piece.units[0].position.x - 1, 1};
    piece.units[2].position = (Vector2){piece.units[0].position.x, 1};
    piece.units[3].position = (Vector2){piece.units[0].position.x + 1, 1};
}

void spawnZ(Tetromino &piece)
{
    piece.units[0].position = (Vector2){(float)GRID_HORIZONTAL_SIZE / 2, 0};
    piece.units[1].position = (Vector2){piece.units[0].position.x - 1, 0};
    piece.units[2].position = (Vector2){piece.units[0].position.x, 1};
    piece.units[3].position = (Vector2){piece.units[0].position.x + 1, 1};
}

void UpdateGame()
{
    if (gameState != PLAYING)
        return;

    if (gameOver)
        return;

    if (IsKeyPressed('P'))
        pause = !pause;

    if (pause)
        return;

    if (IsKeyPressed('G'))
        showGrid = !showGrid;

    if (IsKeyPressed('T'))
        gridBackground();

    if (IsKeyPressed('C'))
        return drawLevelTransition();

    if (IsKeyPressed(KEY_LEFT))
    {
        lateralTimer += GetFrameTime();
        if (lateralTimer >= lateralSpeed)
        {
            if (canMoveHorizontally(currentPiece, -1))
            {
                for (int i = 0; i < currentPiece.size; i++)
                {
                    currentPiece.units[i].position.x -= 1;
                }
            }
        }
    }
    if (IsKeyPressed(KEY_RIGHT))
    {
        lateralTimer += GetFrameTime();
        if (lateralTimer >= lateralSpeed)
        {
            if (canMoveHorizontally(currentPiece, 1))
            {
                for (int i = 0; i < currentPiece.size; i++)
                {
                    currentPiece.units[i].position.x += 1;
                }
            }
        }
    }

    if (IsKeyDown(KEY_LEFT))
    {
        lateralTimer += GetFrameTime();
        if (lateralTimer >= lateralSpeed)
        {
            lateralTimer -= lateralSpeed;
            if (canMoveHorizontally(currentPiece, -1))
            {
                for (int i = 0; i < currentPiece.size; i++)
                {
                    currentPiece.units[i].position.x -= 1;
                }
            }
        }
    }
    else if (IsKeyDown(KEY_RIGHT))
    {
        lateralTimer += GetFrameTime();
        if (lateralTimer >= lateralSpeed)
        {
            lateralTimer -= lateralSpeed;
            if (canMoveHorizontally(currentPiece, 1))
            {
                for (int i = 0; i < currentPiece.size; i++)
                {
                    currentPiece.units[i].position.x += 1;
                }
            }
        }
    }

    else
    {
        lateralTimer = 0.0f;
    }

    if (IsKeyPressed(KEY_UP))
    {
        currentPiece = rotatePiece(currentPiece);
    }

    if (IsKeyPressed(KEY_DOWN))
    {
        fallSpeed = 0.05f;
    }

    if (IsKeyPressed(KEY_SPACE))
    {
        isInFreeFall = true;
        fallSpeed = 0.01f;
    }

    float levelAdjustedFallSpeed = baseFallSpeed / (1.0f + (level - 1) * 0.1f);

    fallTimer += GetFrameTime();
    if ((currentPiece.pieceState != BOTTOMED && fallTimer >= fallSpeed) || currentPiece.pieceState == BOTTOMED)
    {
        fallTimer = 0.0f;

        if (canMoveDown(currentPiece))
            moveDown(currentPiece);
        else
        {
            if (currentPiece.pieceState == BOTTOMED)
            {
                bottomedTimer += GetFrameTime();
            }
            else
            {
                currentPiece.pieceState = BOTTOMED;
            }

            // check timer
            if (bottomedTimer < 0.15f)
            {
                return;
            }
            else
            {
                bottomedTimer = 0.0f;
                currentPiece.pieceState = LOCKED;
            }

            for (int i = 0; i < currentPiece.size; i++)
            {
                int x = (int)currentPiece.units[i].position.x;
                int y = (int)currentPiece.units[i].position.y;
                grid[y][x] = 1;
            }

            checkAndClearLines();
            // TODO don't go here if level up
            if (gameState == PLAYING)
            {
                spawnPiece();
                fallSpeed = levelAdjustedFallSpeed;

                // Check if the new piece overlaps with existing blocks (game over condition)
                for (int i = 0; i < currentPiece.size; i++)
                {
                    int x = (int)currentPiece.units[i].position.x;
                    int y = (int)currentPiece.units[i].position.y;
                    if (grid[y][x] == 1) // New piece can't spawn
                    {
                        gameOver = true;
                        drawHighScore(); // Now call this to handle high score or game over
                        return;
                    }
                }
            }
        }
    }
}

void UpdateLanguageSelection()
{
    Vector2 mousePoint = GetMousePosition();

    if (CheckCollisionPointRec(mousePoint, flagButtonPortugal) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        currentLanguage = PORTUGUESE;
    }
    else if (CheckCollisionPointRec(mousePoint, flagButtonGermany) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        currentLanguage = GERMAN;
    }
    else if (CheckCollisionPointRec(mousePoint, flagButtonUK) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        currentLanguage = ENGLISH;
    }
}

void DrawGame()
{
    DrawGrid();
    DrawText(TextFormat("Score: %i", score), 20, 60, 30, BLACK);
    DrawText(TextFormat("Level: %i", level), 20, 20, 30, BLACK);
    DrawText(TextFormat("Lines: %i", linesClearedThisLevel), 20, 100, 30, BLACK);

    int linesNeeded = level * level;
    DrawText(TextFormat("Next Level: %i/%i lines", linesClearedThisLevel, linesNeeded), 20, 140, 20, DARKGRAY);

    if (linesClearedThisLevel < linesNeeded)
    {
        DrawText("Clear lines to advance!", 20, 180, 20, GRAY);
    }
    else
    {
        DrawText("Level Up!", 20, 180, 20, GREEN);
    }
    float progress = (float)linesClearedThisLevel / linesNeeded;
    if (progress > 1.0f)
        progress = 1.0f;
    DrawRectangle(20, 200, 200, 20, GRAY);
    DrawRectangle(20, 200, 200 * progress, 20, DARKGREEN);

    static float bonusTimer = 0.0f;
    if (justClearedGrid)
    {
        bonusTimer = 1.0f;
        justClearedGrid = false;
    }
    if (bonusTimer > 0)
    {
        DrawText("+50 Grid Clear Bonus!", screenWidth / 2 - 135, screenHeight / 2 + 5, 25, BLACK);
        bonusTimer -= GetFrameTime();
    }
}

void UpdateDrawFrame(float gameTime)
{
    BeginDrawing();
    UpdateLanguageSelection(); // Check for language button clicks

    switch (gameState)
    {
    case HOME: {
        ClearBackground(BLACK);
        Vector2 textPos;
        float glowScale = 1.0f + 0.1f * sinf(gameTime * 2.0f);
        int glowLayers = 3;

        const char *languageText;
        const char *welcomeText;
        const char *startText;
        const char *manualText;
        const char *rulesText;
        const char *hText;
        const char *soundText;
        const char *onOffText;

        switch (currentLanguage)
        {
        case PORTUGUESE:
            languageText = "Selecao da lingua:";
            welcomeText = "Bem-vindo ao TETRIS ESPECIAL";
            startText = "Pressione <ENTER> para Iniciar";
            manualText = "Pressione <ESPACO> para ver Manual";
            rulesText = "Pressione <R> para ver as Regras";
            hText = "(Pode sempre pressionar <H> para voltar a este menu)";
            soundText = "Som:";
            onOffText = isMuted ? "DESLIGADO" : "LIGADO";
            break;
        case GERMAN:
            languageText = "Waehle die Sprache:";
            welcomeText = "Willkommen bei TETRIS SPECIAL";
            startText = "Druecke <ENTER> zum Starten";
            manualText = "Druecke <LEERTASTE> um das Handbuch zu sehen";
            rulesText = "Duecke <R> um das Regelbuch zu sehen";
            hText = "(Man kann jederzeit auf <H> druecken, um zurueck zu diesem Menu zu kommen)";
            soundText = "Ton:";
            onOffText = isMuted ? "AUS" : "AN";
            break;
        case ENGLISH:
        default:
            languageText = "Choose the language:";
            welcomeText = "Welcome to TETRIS SPECIAL";
            startText = "Press <ENTER> to Start";
            manualText = "Press <SPACE> to go to the Manual";
            rulesText = "Press <R> to go to the Rules";
            hText = "(You can always press <H> to go back to this Menu)";
            soundText = "Sound:";
            onOffText = isMuted ? "OFF" : "ON";
            break;
        }

        DrawTextEx(font, "By:", {(float)screenWidth / 2 - 570, (float)screenHeight / 2 - 280}, 25, 1, WHITE);
        DrawTextEx(font, "Thomas Gilb de Moura Guedes (feat. Paulo Moura Guedes)",
                   {(float)screenWidth / 2 - 570, (float)screenHeight / 2 - 250}, 25, 1, WHITE);

        textPos = {(float)screenWidth / 2 - MeasureTextEx(font, welcomeText, 40, 1).x / 2,
                   (float)screenHeight / 2 - 70};

        for (int i = glowLayers; i >= 1; i--)
        {
            float glowSize = 40 + i * 5 * glowScale;
            float glowAlpha = 0.3f - (i * 0.1f);
            Color glowColor = {255, 255, 0, (unsigned char)(glowAlpha * 255)};
            Vector2 glowPos = {(float)screenWidth / 2 - MeasureTextEx(font, welcomeText, glowSize, 1).x / 2,
                               (float)screenHeight / 2 - 70 - i * 2};
            DrawTextEx(font, welcomeText, glowPos, glowSize, 1, glowColor);
        }

        DrawTextEx(font, welcomeText, textPos, 40, 1, WHITE);
        DrawTextEx(font, languageText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, languageText, 25, 1).x / 2 - 430,
                             (float)screenHeight / 2 + 170},
                   25, 1, WHITE);
        DrawTextEx(font, startText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, startText, 30, 1).x / 2,
                             (float)screenHeight / 2 - 20},
                   30, 1, WHITE);
        DrawTextEx(font, manualText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, manualText, 30, 1).x / 2,
                             (float)screenHeight / 2 + 20},
                   30, 1, WHITE);
        DrawTextEx(font, rulesText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, rulesText, 30, 1).x / 2,
                             (float)screenHeight / 2 + 60},
                   30, 1, WHITE);
        DrawTextEx(
            font, hText,
            (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, hText, 30, 1).x / 2, (float)screenHeight / 2 + 100},
            30, 1, WHITE);

        // Draw mute/unmute button

        DrawTextEx(font, soundText, (Vector2){muteButton.x + 7, muteButton.y - 15}, 17, 1, WHITE);
        DrawRectangleRec(muteButton, isMuted ? RED : GREEN);
        DrawTextEx(font, onOffText, (Vector2){muteButton.x + 5, muteButton.y + 10}, 20, 1, WHITE);

        // Draw flag buttons
        Rectangle sourceRectPortugal = {0, 0, (float)flagPortugal.width, (float)flagPortugal.height};
        Rectangle destRectPortugal = {flagButtonPortugal.x, flagButtonPortugal.y, flagButtonPortugal.width,
                                      flagButtonPortugal.height};
        if (currentLanguage == PORTUGUESE)
        {
            // Add green glow effect
            float glowScale = 1.0f + 0.1f * sinf(gameTime * 2.0f);
            for (int i = 3; i >= 1; i--)
            {
                float glowSize = i * 5 * glowScale;
                Color glowColor = {0, 255, 0, (unsigned char)(0.3f * 255 / i)};
                DrawRectangleLinesEx({destRectPortugal.x - glowSize, destRectPortugal.y - glowSize,
                                      destRectPortugal.width + 2 * glowSize, destRectPortugal.height + 2 * glowSize},
                                     2, glowColor);
            }
        }
        DrawTexturePro(flagPortugal, sourceRectPortugal, destRectPortugal, {0, 0}, 0.0f, WHITE);

        Rectangle sourceRectGermany = {0, 0, (float)flagGermany.width, (float)flagGermany.height};
        Rectangle destRectGermany = {flagButtonGermany.x, flagButtonGermany.y, flagButtonGermany.width,
                                     flagButtonGermany.height};
        if (currentLanguage == GERMAN)
        {
            // Add green glow effect
            float glowScale = 1.0f + 0.1f * sinf(gameTime * 2.0f);
            for (int i = 3; i >= 1; i--)
            {
                float glowSize = i * 5 * glowScale;
                Color glowColor = {0, 255, 0, (unsigned char)(0.3f * 255 / i)};
                DrawRectangleLinesEx({destRectGermany.x - glowSize, destRectGermany.y - glowSize,
                                      destRectGermany.width + 2 * glowSize, destRectGermany.height + 2 * glowSize},
                                     2, glowColor);
            }
        }
        DrawTexturePro(flagGermany, sourceRectGermany, destRectGermany, {0, 0}, 0.0f, WHITE);

        Rectangle sourceRectUK = {0, 0, (float)flagUK.width, (float)flagUK.height};
        Rectangle destRectUK = {flagButtonUK.x, flagButtonUK.y, flagButtonUK.width, flagButtonUK.height};
        if (currentLanguage == ENGLISH)
        {
            // Add green glow effect
            float glowScale = 1.0f + 0.1f * sinf(gameTime * 2.0f);
            for (int i = 3; i >= 1; i--)
            {
                float glowSize = i * 5 * glowScale;
                Color glowColor = {0, 255, 0, (unsigned char)(0.3f * 255 / i)};
                DrawRectangleLinesEx({destRectUK.x - glowSize, destRectUK.y - glowSize, destRectUK.width + 2 * glowSize,
                                      destRectUK.height + 2 * glowSize},
                                     2, glowColor);
            }
        }
        DrawTexturePro(flagUK, sourceRectUK, destRectUK, {0, 0}, 0.0f, WHITE);

        DrawRectangleLinesEx(destRectUK, 2, WHITE);
        DrawRectangleLinesEx(destRectGermany, 2, WHITE);
        DrawRectangleLinesEx(destRectPortugal, 2, WHITE);
        break;
    }

    case HOW_TO_PLAY: {
        ClearBackground(BLACK);
        Vector2 textPos;
        float glowScale = 1.0f + 0.1f * sinf(gameTime * 2.0f);
        int glowLayers = 3;

        const char *manualText;
        const char *playText;
        const char *tetrisText = "Tetris:";
        const char *moveText;
        const char *rotateText;
        const char *fallText;
        const char *gridText;
        const char *playerGameText;
        const char *movePlayerText;
        const char *jumpText;
        const char *doorText;
        const char *timeText;

        switch (currentLanguage)
        {
        case PORTUGUESE:
            manualText = "Manual do Usuario";
            playText = "(Pressione ENTER para Jogar)";
            moveText = "Use setas para mover";
            rotateText = "Pressione <CIMA> para girar";
            fallText = "Pressione <BAIXO> para queda rapida e <ESPACO> para queda livre";
            gridText = "Pressione <G> para fazer o tabuleiro desaparecer ou vice-versa";
            playerGameText = "Jogo do Jogador:";
            movePlayerText = "Use setas para mover";
            jumpText = "Use <ESPACO> para pular";
            doorText = "Move o jogador contra a porta para passar de nivel";
            timeText = "So tem um tempo limitado para mover o jogador contra a porta (10s)";
            break;
        case GERMAN:
            manualText = "Benutzerhandbuch";
            playText = "(Druecke ENTER zum Spielen)";
            moveText = "Pfeiltasten zum Bewegen verwenden";
            rotateText = "Druecke <HOCH> zum Drehen";
            fallText = "Druecke <RUNTER> fuer schnelles Fallen und <LEERTASTE> fuer "
                       "freien Fall";
            gridText = "Druecke <G> um das Gitternetz verschwinden zu lassen oder andersherum";
            playerGameText = "Spielerspiel:";
            movePlayerText = "Pfeiltasten zum Bewegen verwenden";
            jumpText = "Verwende <LEERTASTE> zum Springen";
            doorText = "Bewege den Spieler gegen die Tuer, um das Level zu bestehen";
            timeText = "Man hat nur eine begrenzte Zeit den Spieler gegen die Tuer zu bewegen(10s) ";
            break;
        case ENGLISH:
        default:
            manualText = "User Manual";
            playText = "(Press ENTER to Play)";
            moveText = "Use arrows to move";
            rotateText = "Press <UP> to rotate";
            fallText = "Press <DOWN> for fast fall and <SPACE> for free fall";
            gridText = "Press <G> to make the grid disappear or the other way around";
            playerGameText = "Player Game:";
            movePlayerText = "Use arrows to move";
            jumpText = "Use <SPACE> to jump";
            doorText = "Move the player against the door to pass the level";
            timeText = "You only have a limited time to move the player against the door(10s)";
            break;
        }

        textPos = {(float)screenWidth / 2 - MeasureTextEx(font, manualText, 40, 1).x / 2,
                   (float)screenHeight / 2 - 240};

        for (int i = glowLayers; i >= 1; i--)
        {
            float glowSize = 40 + i * 10 * glowScale;
            float glowAlpha = 0.3f - (i * 0.08f);
            Color glowColor = {255, 255, 0, (unsigned char)(glowAlpha * 255)};
            Vector2 glowPos = {(float)screenWidth / 2 - MeasureTextEx(font, manualText, glowSize, 1).x / 2,
                               (float)screenHeight / 2 - 240 - i * 2};
            DrawTextEx(font, manualText, glowPos, glowSize, 1, glowColor);
        }
        DrawTextEx(font, manualText, textPos, 40, 1, WHITE);

        DrawTextEx(font, playText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, playText, 35, 1).x / 2,
                             (float)screenHeight / 2 - 125},
                   35, 1, WHITE);
        DrawTextEx(font, tetrisText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, tetrisText, 35, 1).x / 2,
                             (float)screenHeight / 2 - 65},
                   35, 1, WHITE);
        DrawTextEx(font, moveText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, moveText, 25, 1).x / 2,
                             (float)screenHeight / 2 - 25},
                   25, 1, WHITE);
        DrawTextEx(
            font, rotateText,
            (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, rotateText, 25, 1).x / 2, (float)screenHeight / 2},
            25, 1, WHITE);
        DrawTextEx(font, fallText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, fallText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 25},
                   25, 1, WHITE);
        DrawTextEx(font, gridText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, gridText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 50},
                   25, 1, WHITE);
        DrawTextEx(font, playerGameText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, playerGameText, 35, 1).x / 2,
                             (float)screenHeight / 2 + 100},
                   35, 1, WHITE);
        DrawTextEx(font, movePlayerText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, movePlayerText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 140},
                   25, 1, WHITE);
        DrawTextEx(font, jumpText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, jumpText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 165},
                   25, 1, WHITE);
        DrawTextEx(font, doorText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, doorText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 190},
                   25, 1, WHITE);
        DrawTextEx(font, timeText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, timeText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 215},
                   25, 1, WHITE);
        break;
    }

    case RULES: {
        ClearBackground(BLACK);
        Vector2 textPos;
        float glowScale = 2.0f + 0.1f * sinf(gameTime * 2.0f);
        int glowLayers = 3;

        const char *rulesText;
        const char *playText;
        const char *tetrisText = "Tetris:";
        const char *fullLinesText;
        const char *gridFullText;
        const char *playerGameText;
        const char *dieText;
        const char *hitWallText;
        const char *doorText;

        switch (currentLanguage)
        {
        case PORTUGUESE:
            rulesText = "Regras";
            playText = "(Pressione ENTER para Jogar)";
            fullLinesText = "Se uma linha inteira estiver cheia, ela sera removida";
            gridFullText = "Se os Tetrominos ja nao caberem no tabuleiro, morreras";
            playerGameText = "Jogo do Jogador:";
            dieText = "Se caires entra as plataformas, morreras";
            hitWallText = "Se moveres o jogador contra as paredes, morreras";
            doorText = "Mova o jogador contra a porta para passar de nivel";
            break;
        case GERMAN:
            rulesText = "Regeln";
            playText = "(Druecke Enter zum Spielen)";
            fullLinesText = "Wenn eine ganze Linie voll ist, wird sie geloescht";
            gridFullText = "Wenn die Tetrominos nicht mehr ins Feld passen verlierst du";
            playerGameText = "Spielerspiel";
            dieText = "Wenn du zwischen die Platformen faellst stirbst du";
            hitWallText = "Wenn du den Spieler gegen die Waende bewegst stirbst du";
            doorText = "Bewege den Spieler gegen die Tuer, um das Level zu bestehen";
            break;
        case ENGLISH:
        default:
            rulesText = "Rules";
            playText = "(Press ENTER to play)";
            fullLinesText = "If a line is full it's deleted";
            gridFullText = "If your Tetrominos are stacked to high, you die";
            playerGameText = "Player Game:";
            dieText = "If the player falls between the platforms you die";
            hitWallText = "If you move the player against the walls you'll die";
            doorText = "Move the player against the door to pass the level";
            break;
        }

        textPos = {(float)screenWidth / 2 - MeasureTextEx(font, rulesText, 40, 1).x / 2, (float)screenHeight / 2 - 235};

        for (int i = glowLayers; i >= 1; i--)
        {
            float glowSize = 40 + i * 5 * glowScale;
            float glowAlpha = 0.3f - (i * 0.1f);
            Color glowColor = {255, 255, 0, (unsigned char)(glowAlpha * 255)};
            Vector2 glowPos = {(float)screenWidth / 2 - MeasureTextEx(font, rulesText, glowSize, 1).x / 2,
                               (float)screenHeight / 2 - 240 - i * 2};
            DrawTextEx(font, rulesText, glowPos, glowSize, 1, glowColor);
        }
        DrawTextEx(font, rulesText, textPos, 40, 1, WHITE);

        DrawTextEx(font, playText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, playText, 35, 1).x / 2,
                             (float)screenHeight / 2 - 105},
                   35, 1, WHITE);
        DrawTextEx(font, tetrisText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, tetrisText, 35, 1).x / 2,
                             (float)screenHeight / 2 - 40},
                   35, 1, WHITE);
        DrawTextEx(
            font, playText,
            (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, playText, 25, 1).x / 2, (float)screenHeight / 2}, 25,
            1, WHITE);
        DrawTextEx(font, fullLinesText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, fullLinesText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 25},
                   25, 1, WHITE);
        DrawTextEx(font, gridFullText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, gridFullText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 50},
                   25, 1, WHITE);
        DrawTextEx(font, playerGameText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, playerGameText, 35, 1).x / 2,
                             (float)screenHeight / 2 + 100},
                   35, 1, WHITE);
        DrawTextEx(font, dieText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, dieText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 140},
                   25, 1, WHITE);
        DrawTextEx(font, hitWallText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, hitWallText, 25, 1).x / 2,
                             (float)screenHeight / 2 + 165},
                   25, 1, WHITE);
        DrawTextEx(font, playText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, playText, 35, 1).x / 2,
                             (float)screenHeight / 2 - 105},
                   35, 1, WHITE);
        DrawTextEx(font, doorText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, doorText, 35, 1).x / 2 + 110,
                             (float)screenHeight / 2 + 190},
                   25, 1, WHITE);
        break;
    }

    case PLAYING: {
        ClearBackground(LIGHTGRAY);
        UpdateGame();
        DrawGrid();

        const char *scoreText;
        const char *levelText;
        const char *linesText;
        const char *nextLevelText;
        const char *advanceText;
        const char *levelUpText;
        const char *soundText;
        const char *onOffText;
        const char *pauseText;
        const char *gameOverText;

        switch (currentLanguage)
        {
        case PORTUGUESE:
            scoreText = TextFormat("Pontuacao: %i", score);
            levelText = TextFormat("Nivel: %i", level);
            linesText = TextFormat("Linhas: %i", linesClearedThisLevel);
            nextLevelText = TextFormat("Proximo Nivel: %i/%i linhas", linesClearedThisLevel, level * level);
            advanceText = "Limpe linhas para avancar!";
            levelUpText = "Subir de Nivel!";
            soundText = "Som:";
            onOffText = isMuted ? "DESLIGADO" : "LIGADO";
            pauseText = "JOGO PAUSADO";
            gameOverText = "Fim de Jogo";
            break;
        case GERMAN:
            scoreText = TextFormat("Punkte: %i", score);
            levelText = TextFormat("Stufe: %i", level);
            linesText = TextFormat("Linien: %i", linesClearedThisLevel);
            nextLevelText = TextFormat("Naechste Stufe: %i/%i Linien", linesClearedThisLevel, level * level);
            advanceText = "Loesche Linien zum Fortfahren!";
            levelUpText = "Stufe Aufstieg!";
            soundText = "Ton:";
            onOffText = isMuted ? "AUS" : "AN";
            pauseText = "SPIEL PAUSIERT";
            gameOverText = "Spiel Ende";
            break;
        case ENGLISH:
        default:
            scoreText = TextFormat("Score: %i", score);
            levelText = TextFormat("Level: %i", level);
            linesText = TextFormat("Lines: %i", linesClearedThisLevel);
            nextLevelText = TextFormat("Next Level: %i/%i lines", linesClearedThisLevel, level * level);
            advanceText = "Clear lines to advance!";
            levelUpText = "Level Up!";
            soundText = "Sound:";
            onOffText = isMuted ? "OFF" : "ON";
            pauseText = "GAME PAUSED";
            gameOverText = "Game Over";
            break;
        }

        DrawTextEx(font, scoreText, (Vector2){20, 60}, 30, 1, BLACK);
        DrawTextEx(font, levelText, (Vector2){20, 20}, 30, 1, BLACK);
        DrawTextEx(font, linesText, (Vector2){20, 100}, 30, 1, BLACK);
        DrawTextEx(font, nextLevelText, (Vector2){20, 140}, 27, 1, DARKGRAY);

        int linesNeeded = level * level;
        if (linesClearedThisLevel < linesNeeded)
        {
            DrawTextEx(font, advanceText, (Vector2){20, 180}, 20, 1, GRAY);
        }
        else
        {
            DrawTextEx(font, levelUpText, (Vector2){20, 180}, 20, 1, GREEN);
        }

        float progress = (float)linesClearedThisLevel / linesNeeded;
        if (progress > 1.0f)
            progress = 1.0f;
        DrawRectangle(20, 200, 200, 20, GRAY);
        DrawRectangle(20, 200, 200 * progress, 20, DARKGREEN);

        static float bonusTimer = 0.0f;
        if (justClearedGrid)
        {
            bonusTimer = 1.0f;
            justClearedGrid = false;
        }
        if (bonusTimer > 0)
        {
            const char *bonusText = currentLanguage == PORTUGUESE ? "+50 Bonus de Limpeza de tabuleiro!"
                                    : currentLanguage == GERMAN   ? "+50 Bonus fuer Gitterloesung!"
                                                                  : "+50 Grid Clear Bonus!";
            DrawTextEx(font, bonusText, (Vector2){(float)screenWidth / 2 - 135, (float)screenHeight / 2 + 5}, 25, 1,
                       BLACK);
            bonusTimer -= GetFrameTime();
        }

        DrawPiece(&currentPiece); // Fixed typo from ¤tPiece
        UpdateDrawParticles(GetFrameTime());
        DrawPulseEffect(GetFrameTime());

        if (pause)
        {
            DrawTextEx(font, pauseText,
                       (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, pauseText, 40, 1).x / 2,
                                 (float)screenHeight / 2 - 40},
                       40, 1, BLACK);
        }
        if (gameOver)
        {
            DrawTextEx(font, gameOverText,
                       (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, gameOverText, 40, 1).x / 2,
                                 (float)screenHeight / 2 - 20},
                       40, 1, BLACK);
        }

        // Draw mute/unmute button
        DrawTextEx(font, soundText, (Vector2){muteButton.x + 7, muteButton.y - 15}, 17, 1, WHITE);
        DrawRectangleRec(muteButton, isMuted ? RED : GREEN);
        DrawTextEx(font, onOffText, (Vector2){muteButton.x + 5, muteButton.y + 10}, 20, 1, WHITE);
        break;
    }

    case LEVEL_TRANSITION: {
        Vector2 shakeOffset = {0, 0};
        if (screenShake)
        {
            shakeTimer -= GetFrameTime();
            if (shakeTimer <= 0)
                screenShake = false;
            else
            {
                float shakeIntensity = shakeTimer / SHAKE_DURATION * 10.0f;
                shakeOffset.x = (float)GetRandomValue(-shakeIntensity, shakeIntensity);
                shakeOffset.y = (float)GetRandomValue(-shakeIntensity, shakeIntensity);
            }
        }

        ClearBackground(DARKBLUE);
        for (int i = 0; i < MAX_STARS; i++)
        {
            DrawCircleV(stars[i], GetRandomValue(1, 3), WHITE);
        }

        UpdateLevelTransition(GetFrameTime());

        for (int i = 0; i < NUM_PLATFORMS; i++)
        {
            Rectangle platformRect = {platforms[i].position.x - platforms[i].width / 2 + shakeOffset.x,
                                      platforms[i].position.y - platforms[i].height / 2 + shakeOffset.y,
                                      platforms[i].width, platforms[i].height};
            DrawRectangleRounded(platformRect, 1.2f, 8, MAROON);
        }

        Rectangle doorRect = {door.position.x - door.width / 2 + shakeOffset.x, door.position.y - door.height / 2,
                              door.width + shakeOffset.y, door.height};

        Rectangle shadowRect = {doorRect.x + 10 + shakeOffset.x, doorRect.y + 5 + shakeOffset.y, doorRect.width,
                                doorRect.height};
        DrawRectangleRec(shadowRect, (Color){0, 0, 0, 50});

        Rectangle frameRect = {doorRect.x - 4 + shakeOffset.x, doorRect.y - 4 + shakeOffset.y, doorRect.width + 8,
                               doorRect.height + 8};
        DrawRectangleRec(frameRect, (Color){139, 69, 19, 255});

        DrawRectangleRec(doorRect, (Color){165, 42, 42, 255});

        Rectangle topPanel = {doorRect.x + 4 + shakeOffset.x, doorRect.y + 4 + shakeOffset.y, doorRect.width - 8,
                              (doorRect.height - 12) / 2};
        Rectangle bottomPanel = {doorRect.x + 4 + shakeOffset.x, doorRect.y + doorRect.height / 2 + 2 + shakeOffset.y,
                                 doorRect.width - 8, (doorRect.height - 12) / 2};
        DrawRectangleRec(topPanel, (Color){139, 69, 19, 255});
        DrawRectangleRec(bottomPanel, (Color){139, 69, 19, 255}); // Note: bottomPanel should be used here
                                                                  // instead of customPanel

        Vector2 handlePos = {doorRect.x + doorRect.width - 6 + shakeOffset.x,
                             doorRect.y + doorRect.height * 0.6f + shakeOffset.y};
        DrawCircleV(handlePos, 2.0f, GOLD);
        DrawCircleLines(handlePos.x, handlePos.y, 2.0f, BLACK);

        if (isPlayerVisible)
        {
            for (int i = 0; i < player.size; i++)
            {
                Vector2 unitPos = {player.position.x + player.units[i].position.x - player.unitSize / 2,
                                   player.position.y + player.units[i].position.y - player.unitSize / 2};
                Color drawColor = player.units[i].color;
                if (isElectrocuted)
                {
                    drawColor = (fmod(GetTime(), 0.2f) < 0.1f) ? YELLOW : BLUE;
                    drawColor.a = (unsigned char)(255 * (electrocutionTimer / ELECTROCUTION_DURATION));
                }

                if (i == 0)
                {
                    Rectangle headRect = {unitPos.x - player.units[i].width / 2, unitPos.y - player.units[i].height / 2,
                                          player.units[i].width, player.units[i].height};
                    DrawRectangleRounded(headRect, 0.8f, 8, drawColor);
                }
                else if (i == 2 || i == 3)
                {
                    Rectangle armRect = {unitPos.x, unitPos.y, player.units[i].width, player.units[i].height};
                    DrawRectanglePro(armRect, {player.units[i].width / 2, player.units[i].height / 2}, 35.0f,
                                     drawColor);
                }
                else
                {
                    Rectangle rect = {unitPos.x - player.units[i].width / 2, unitPos.y - player.units[i].height / 2,
                                      player.units[i].width, player.units[i].height};
                    DrawRectangleRec(rect, drawColor);
                }
            }
        }

        UpdateDrawParticles(GetFrameTime());
        DrawPulseEffect(GetFrameTime());

        const char *timeText;
        const char *pauseText;
        switch (currentLanguage)
        {
        case PORTUGUESE:
            timeText = TextFormat("Tempo Restante: %.1f", transitionTimer >= 0 ? transitionTimer : 0.0f);
            pauseText = "JOGO PAUSADO";
            break;
        case GERMAN:
            timeText = TextFormat("Verbleibende Zeit: %.1f", transitionTimer >= 0 ? transitionTimer : 0.0f);
            pauseText = "SPIEL PAUSIERT";
            break;
        case ENGLISH:
        default:
            timeText = TextFormat("Time Left: %.1f", transitionTimer >= 0 ? transitionTimer : 0.0f);
            pauseText = "GAME PAUSED";
            break;
        }

        DrawTextEx(font, timeText, (Vector2){20, 20}, 20, 1, WHITE);
        if (pause)
        {
            DrawTextEx(font, pauseText,
                       (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, pauseText, 40, 1).x / 2,
                                 (float)screenHeight / 2 - 40},
                       40, 1, WHITE);
        }
        break;
    }

    case GAME_OVER: {
        ClearBackground(BLACK);
        const char *gameOverText;
        const char *restartText;
        const char *homeText;
        const char *linesText;
        const char *soundText;
        const char *onOffText;

        switch (currentLanguage)
        {
        case PORTUGUESE:
            gameOverText = "Fim de Jogo";
            restartText = "Pressione [ENTER] para Reiniciar";
            homeText = "Pressione [H] para voltar ao Inicio";
            linesText = TextFormat("Linhas Limpas: %i", linesClearedTotal);
            soundText = "Som:";
            onOffText = isMuted ? "DESLIGADO" : "LIGADO";
            break;
        case GERMAN:
            gameOverText = "Spiel Ende";
            restartText = "Druecke [ENTER] zum Neustart";
            homeText = "Druecke [H] fuer Home";
            linesText = TextFormat("Geloeschte Linien: %i", linesClearedTotal);
            soundText = "Ton:";
            onOffText = isMuted ? "AUS" : "AN";
            break;
        case ENGLISH:
        default:
            gameOverText = "Game Over";
            restartText = "Press [ENTER] to Restart";
            homeText = "Press [H] to return to Home";
            linesText = TextFormat("Lines Cleared: %i", linesClearedTotal);
            soundText = "Sound:";
            onOffText = isMuted ? "OFF" : "ON";
            break;
        }

        DrawTextEx(font, gameOverText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, gameOverText, 50, 1).x / 2,
                             (float)screenHeight / 2 - 50},
                   50, 1, WHITE);
        DrawTextEx(font, restartText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, restartText, 20, 1).x / 2,
                             (float)screenHeight / 2 + 10},
                   20, 1, WHITE);
        DrawTextEx(font, homeText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, homeText, 20, 1).x / 2,
                             (float)screenHeight / 2 + 40},
                   20, 1, WHITE);
        DrawTextEx(font, linesText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, linesText, 25, 1).x / 2 - 10,
                             (float)screenHeight / 2 + 77},
                   25, 1, WHITE);

        // Draw mute/unmute button
        DrawTextEx(font, soundText, (Vector2){muteButton.x + 7, muteButton.y - 15}, 17, 1, WHITE);
        DrawRectangleRec(muteButton, isMuted ? RED : GREEN);
        DrawTextEx(font, onOffText, (Vector2){muteButton.x + 5, muteButton.y + 10}, 20, 1, WHITE);
        break;
    }

    case HIGH_SCORE_ENTRY: {
        ClearBackground(BLACK);

        // Language-specific text
        const char *highScoreText;
        const char *enterNameText;
        const char *confirmText;
        const char *playText;
        const char *linesText;
        const char *soundText;
        const char *onOffText;

        switch (currentLanguage)
        {
        case PORTUGUESE:
            highScoreText = "Novo Recorde!";
            enterNameText = "Insira o seu nome:";
            confirmText = "Pressione <ENTER> para confirmar";
            playText = "Pressione <Enter> outra vez para jogar";
            linesText = TextFormat("Linhas Limpas: %i", linesClearedTotal);
            soundText = "Som:";
            onOffText = isMuted ? "DESLIGADO" : "LIGADO";
            break;
        case GERMAN:
            highScoreText = "Neuer Highscore!";
            enterNameText = "Gib deinen Namen ein:";
            confirmText = "Druecke <ENTER> zum Bestaetigen";
            playText = "Druecke noch einmal <Enter> zum Spielen";
            linesText = TextFormat("Geloeschte Linien: %i", linesClearedTotal);
            soundText = "Ton:";
            onOffText = isMuted ? "AUS" : "AN";
            break;
        case ENGLISH:
        default:
            highScoreText = "New High Score!";
            enterNameText = "Enter your name:";
            confirmText = "Press <ENTER> to confirm";
            playText = "Press <ENTER> again to play";
            linesText = TextFormat("Lines Cleared: %i", linesClearedTotal);
            soundText = "Sound:";
            onOffText = isMuted ? "OFF" : "ON";
            break;
        }

        // Draw high score header
        DrawTextEx(font, highScoreText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, highScoreText, 50, 1).x / 2,
                             (float)screenHeight / 2 - 110},
                   50, 1, GOLD);

        // Draw score
        DrawTextEx(font, linesText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, linesText, 25, 1).x / 2,
                             (float)screenHeight / 2 - 50},
                   25, 1, WHITE);

        // Draw enter name prompt
        DrawTextEx(font, enterNameText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, enterNameText, 25, 1).x / 2,
                             (float)screenHeight / 2 - 10},
                   25, 1, WHITE);

        // Draw input box
        DrawRectangle(screenWidth / 2 - 150, screenHeight / 2 + 20, 300, 40, Color{50, 50, 50, 255});
        DrawRectangleLinesEx((Rectangle){(float)screenWidth / 2 - 150, (float)screenHeight / 2 + 20, 300, 40}, 2, GOLD);

        // Draw current name
        DrawTextEx(font, playerName,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, playerName, 30, 1).x / 2,
                             (float)screenHeight / 2 + 25},
                   30, 1, WHITE);

        // Draw blinking cursor if text is less than max length
        if (showCursor && playerNameLength < NAME_LEN - 1)
        {
            float cursorPosX = (float)screenWidth / 2 - MeasureTextEx(font, playerName, 30, 1).x / 2 +
                               MeasureTextEx(font, playerName, 30, 1).x;
            DrawTextEx(font, "_", (Vector2){cursorPosX, (float)screenHeight / 2 + 25}, 30, 1, WHITE);
        }

        // Draw confirmation text
        DrawTextEx(font, confirmText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, confirmText, 20, 1).x / 2,
                             (float)screenHeight / 2 + 80},
                   25, 1, LIGHTGRAY);
        DrawTextEx(font, playText,
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, playText, 20, 1).x / 2,
                             (float)screenHeight / 2 + 120},
                   25, 1, LIGHTGRAY);

        // Draw mute/unmute button
        DrawTextEx(font, soundText, (Vector2){muteButton.x + 7, muteButton.y - 15}, 17, 1, WHITE);
        DrawRectangleRec(muteButton, isMuted ? RED : GREEN);
        DrawTextEx(font, onOffText, (Vector2){muteButton.x + 5, muteButton.y + 10}, 20, 1, WHITE);

        // Draw top 5 scores on the right side
        const char *highScoresTitle;

        switch (currentLanguage)
        {
        case PORTUGUESE:
            highScoresTitle = "Melhores Pontuacoes:";
            break;
        case GERMAN:
            highScoresTitle = "Bestenliste:";
            break;
        case ENGLISH:
        default:
            highScoresTitle = "Top Scores:";
            break;
        }

        DrawTextEx(font, highScoresTitle, (Vector2){screenWidth - 250, 150}, 25, 1, GOLD);

        ScoreEntry *scores = getScores();
        for (int i = 0; i < MAX_SCORES; i++)
        {
            DrawTextEx(font, TextFormat("%d. %s - %d", i + 1, scores[i].name, scores[i].linesCleared),
                       (Vector2){screenWidth - 250, (float)190 + i * 30}, 20, 1, (i == 0) ? GOLD : WHITE);
        }

        break;
    }
    }

    UpdateAudioMute();
    EndDrawing();
}

void UnloadGame()
{
    UnloadFont(font);
    UnloadSound(levelStartSound);
    UnloadSound(doorHitSound);
    UnloadTexture(flagPortugal);
    UnloadTexture(flagGermany);
    UnloadTexture(flagUK);
    CloseAudioDevice();
}

Vector2 fromGrid(Vector2 position)
{
    return {position.x * BLOCK_SIZE, position.y * BLOCK_SIZE};
}

Vector2 toGrid(Vector2 position)
{
    return {position.x / BLOCK_SIZE, position.y / BLOCK_SIZE};
}

bool canMoveHorizontally(Tetromino currentPiece, int amount)
{
    if (isInFreeFall && currentPiece.pieceState != BOTTOMED)
    {
        return false;
    }

    for (int i = 0; i < currentPiece.size; i++)
    {
        // check Y
        if (currentPiece.pieceState != BOTTOMED && currentPiece.units[i].position.y >= GRID_VERTICAL_SIZE - 1)
        {
            return false; // No lateral movement if touching ground
        }
    }

    for (int i = 0; i < currentPiece.size; i++)
    {
        Vector2 newPos = currentPiece.units[i].position;
        newPos.x += amount;

        // check X
        if (newPos.x < 0 || newPos.x >= GRID_HORIZONTAL_SIZE)
        {
            return false;
        }
        if (grid[(int)currentPiece.units[i].position.y][(int)newPos.x] == 1)
        {
            return false;
        }
    }

    return true;
}

Tetromino rotatePiece(Tetromino piece)
{
    Tetromino rotatedPiece = piece;
    Vector2 pivot = piece.units[0].position;

    for (int i = 1; i < rotatedPiece.size; i++)
    {
        float relativeX = rotatedPiece.units[i].position.x - pivot.x;
        float relativeY = rotatedPiece.units[i].position.y - pivot.y;

        float newRelativeX = relativeY;
        float newRelativeY = -relativeX;

        rotatedPiece.units[i].position.x = pivot.x + newRelativeX;
        rotatedPiece.units[i].position.y = pivot.y + newRelativeY;

        if (rotatedPiece.units[i].position.x < 0 || rotatedPiece.units[i].position.x >= GRID_HORIZONTAL_SIZE ||
            rotatedPiece.units[i].position.y < 0 || rotatedPiece.units[i].position.y >= GRID_VERTICAL_SIZE)
        {
            return piece;
        }
    }
    for (int i = 0; i < rotatedPiece.size; i++)
    {
        int x = (int)rotatedPiece.units[i].position.x;
        int y = (int)rotatedPiece.units[i].position.y;

        if (grid[y][x] == 1)
        {
            return piece;
        }
    }
    return rotatedPiece;
}

bool canMoveDown(Tetromino piece)
{
    for (int i = 0; i < piece.size; i++)
    {
        float newY = piece.units[i].position.y + 1;
        int x = (int)piece.units[i].position.x;

        if (newY >= GRID_VERTICAL_SIZE || grid[(int)newY][x] == 1)
        {
            return false;
        }
    }
    return true;
}

void moveDown(Tetromino &currentPiece)
{
    for (int i = 0; i < currentPiece.size; i++)
    {
        currentPiece.units[i].position.y += 1; // Move down
    }
}
