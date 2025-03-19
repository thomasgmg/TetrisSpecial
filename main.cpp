#include "raylib.h"

#include "score.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <stdlib.h>

// Settings
bool const AUDIO_ENABLED = true;

Font font;

int const GRID_HORIZONTAL_SIZE = 16;
int const GRID_VERTICAL_SIZE = 22;
int const BLOCK_SIZE = 27;

int const TOTAL_PIECES_TYPES = 7;

int const screenWidth = 1150;
int const screenHeight = 594;

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

bool isMenu = true;
bool gameOver = false;
bool pause = false;
bool isInFreeFall = false;
bool isPlayer = true;

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
    MENU,
    MANUAL,
    PLAYING,
    LEVEL_UP,
    LEVEL_TRANSITION,
    GAME_OVER
};
GameState gameState = MENU;

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

void UpdateLevelTransition(float deltaTime)
{
    if (pause)
        return;

    if (doorHit)
    {
        currentPiece.pieceState = NEW;
        doorEffectTimer -= deltaTime;
        isPlayer = false;
        if (doorEffectTimer <= 0.0f)
        {
            isPlayer = true;
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
            gameState = GAME_OVER;
            isElectrocuted = false;
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
        player.velocity.y = -400.0f;
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
            gameState = GAME_OVER;
            return;
        }
    }

    transitionTimer -= deltaTime;
    if (transitionTimer <= 0.0f)
    {
        gameOver = true;
        gameState = GAME_OVER;
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
    for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
    {
        for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
        {
            int screenX = GRID_OFFSET_X + x * BLOCK_SIZE;
            int screenY = GRID_OFFSET_Y + y * BLOCK_SIZE;

            if (grid[y][x] == 1)
            {
                DrawRectangle(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE, MAROON);
            }
            else
            {
                DrawRectangleLines(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE, GRAY);
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

        DrawRectangle(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE, MAROON);
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
        level++;
        fallSpeed = baseFallSpeed / (1.0f + (level - 1) * 0.01f);
        for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
            for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                grid[y][x] = 0;

        score = 0;
        linesClearedThisLevel = 0;
        gameState = LEVEL_TRANSITION;
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
        player.units[6] = {{0, -35}, {0, 0, 0, 255}, 15.0f, 6.0f};        // Hat or hair (red)
        player.units[7] = {{-4, -29}, {0, 0, 0, 255}, 3.0f, 3.0f};        // Left eye (black)
        player.units[8] = {{1, -29}, {0, 0, 0, 255}, 3.0f, 3.0f};         // Right eye (black)
        player.units[9] = {{-2, -24}, {255, 102, 102, 255}, 7.0f, 2.0f};  // Mouth (pinkish-red)
        player.units[10] = {{-2, -22}, {255, 102, 102, 255}, 5.0f, 1.0f}; // Mouth (smile)
        player.units[11] = {{0, -16}, {255, 204, 153, 255}, 6, 8};        // Neck

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
    }
    return linesCleared;
}

int main()
{
    loadScoresFromFile();

    insertScore("PMG", 60);
    insertScore("Thomas", 70);

    saveScoresToFile();

    ScoreEntry *latestScores = getScores();

    printf("Top %d Scores:\n", MAX_SCORES);
    for (int i = 0; i < MAX_SCORES; i++)
    {
        printf("%d. %s - %d\n", i + 1, latestScores[i].name, latestScores[i].linesCleared);
    }

    srand(time(0));
    InitWindow(screenWidth, screenHeight, "Classic Game: TETRIS");

    if (AUDIO_ENABLED)
    {
        InitAudioDevice();
        levelStartSound = LoadSound("resources/level-Start-Sound.mp3");
        SetSoundVolume(levelStartSound, 57.0f);
        doorHitSound = LoadSound("resources/next-level.mp3");
        SetSoundVolume(doorHitSound, 0.1f);
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
        case MENU:
            if (IsKeyPressed(KEY_ENTER))
            {
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

            else if (IsKeyPressed(KEY_SPACE))
                gameState = MANUAL;
            break;

        case MANUAL:
            if (IsKeyPressed(KEY_ENTER))
            {
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

        case PLAYING:
            if (IsKeyPressed('M'))
                gameState = MENU;
            break;

        case LEVEL_TRANSITION:
            if (IsKeyPressed('P'))
                pause = !pause;
            if (IsKeyPressed('M'))
                gameState = MENU;

        case LEVEL_UP:
            if (IsKeyPressed('S'))
            {
                gameState = PLAYING;
            }
            break;

        case GAME_OVER:
            if (IsKeyPressed(KEY_ENTER))
            {
                PlaySound(levelStartSound);
                insertScore("Player", linesClearedTotal); // Save lines cleared
                saveScoresToFile();

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
            if (IsKeyPressed(KEY_M))
            {
                insertScore("Player", linesClearedTotal); // Save lines cleared
                saveScoresToFile();
                gameState = MENU;
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

    if (IsKeyPressed('C'))
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
        return;
    }

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
                bool isGameOver = false;
                for (int i = 0; i < currentPiece.size; i++)
                {
                    if (currentPiece.units[i].position.y <= 0)
                    {
                        isGameOver = true;
                        break;
                    }
                }
                if (isGameOver)
                {
                    gameOver = true;
                    gameState = GAME_OVER;
                }
                else
                {
                    spawnPiece();
                    fallSpeed = levelAdjustedFallSpeed;
                }
            }
        }
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

    const char *welcomeText = "Welcome to TETRIS SPECIAL";
    const char *manualText = "User Manual";

    switch (gameState)
    {
    case MENU: {
        ClearBackground(BLACK);
        Vector2 textPos = {(float)screenWidth / 2 - MeasureTextEx(font, welcomeText, 40, 1).x / 2,
                           (float)screenHeight / 2 - 40};

        float glowScale = 1.0f + 0.1f * sinf(gameTime * 2.0f);
        int glowLayers = 3;

        for (int i = glowLayers; i >= 1; i--)
        {
            float glowSize = 40 + i * 5 * glowScale;
            float glowAlpha = 0.3f - (i * 0.1f);
            Color glowColor = {255, 255, 0, (unsigned char)(glowAlpha * 255)};

            Vector2 glowPos = {(float)screenWidth / 2 - MeasureTextEx(font, welcomeText, glowSize, 1).x / 2,
                               (float)screenHeight / 2 - 40 - i * 2}; // Slight offset upward

            DrawTextEx(font, welcomeText, glowPos, glowSize, 1, glowColor);
        }

        // Main text on top
        DrawTextEx(font, welcomeText, textPos, 40, 1, WHITE);

        // "Press ENTER to Start" (unchanged)
        DrawTextEx(font, "Press ENTER to Start",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Start", 50, 1).x / 2 + 70,
                             (float)screenHeight / 2 + 40},
                   30, 1, WHITE);
        DrawTextEx(font, "Press SPACE to go to the Manual",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Start", 50, 1).x / 2,
                             (float)screenHeight / 2 + 80},
                   30, 1, WHITE);
        break;
    }

    case MANUAL: {
        ClearBackground(BLACK);

        Vector2 textPos = {(float)screenWidth / 2 - MeasureTextEx(font, manualText, 40, 1).x / 2,
                           (float)screenHeight / 2 - 240};

        float glowScale = 1.0f + 0.1f * sinf(gameTime * 2.0f);
        int glowLayers = 3;

        for (int i = glowLayers; i >= 1; i--)
        {
            float glowSize = 40 + i * 5 * glowScale;
            float glowAlpha = 0.3f - (i * 0.1f);
            Color glowColor = {255, 255, 0, (unsigned char)(glowAlpha * 255)};

            Vector2 glowPos = {(float)screenWidth / 2 - MeasureTextEx(font, manualText, glowSize, 1).x / 2,
                               (float)screenHeight / 2 - 240 - i * 2}; // Slight offset upward

            DrawTextEx(font, manualText, glowPos, glowSize, 1, glowColor);
        }
        DrawTextEx(font, manualText, textPos, 40, 1, WHITE);

        DrawTextEx(font, "(Press ENTER to Play)",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2,
                             (float)screenHeight / 2 - 105},
                   35, 1, WHITE);

        DrawTextEx(font, "Tetris:",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2 + 100,
                             (float)screenHeight / 2 - 40},
                   35, 1, WHITE);
        DrawTextEx(font, "Use arrows to move",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2 + 50,
                             (float)screenHeight / 2},
                   25, 1, WHITE);
        DrawTextEx(font, "Press <UP> to rotate",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2 + 40,
                             (float)screenHeight / 2 + 25},
                   25, 1, WHITE);
        DrawTextEx(font, "Press <Down> for fast fall and <Space> for free fall",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2 - 130,
                             (float)screenHeight / 2 + 50},
                   25, 1, WHITE);

        DrawTextEx(font, "Player Game:",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2 + 70,
                             (float)screenHeight / 2 + 100},
                   35, 1, WHITE);
        DrawTextEx(font, "Use arrows to move",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2 + 50,
                             (float)screenHeight / 2 + 140},
                   25, 1, WHITE);
        DrawTextEx(font, "Use <Space> to jump",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2 + 50,
                             (float)screenHeight / 2 + 165},
                   25, 1, WHITE);
        DrawTextEx(font, "Move the player against the door to pass the level",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Play", 40, 1).x / 2 - 120,
                             (float)screenHeight / 2 + 190},
                   25, 1, WHITE);
        break;
    }

    case PLAYING:
        ClearBackground(LIGHTGRAY);
        UpdateGame();
        DrawGame();
        DrawPiece(&currentPiece);
        UpdateDrawParticles(GetFrameTime());
        DrawPulseEffect(GetFrameTime());
        if (pause)
            DrawText("GAME PAUSED", screenWidth / 2 - MeasureText("GAME PAUSED", 40) / 2, screenHeight / 2 - 40, 40,
                     BLACK);
        if (gameOver)
        {
            DrawTextEx(font, "Game Over",
                       (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Game Over", 50, 1).x / 2,
                                 (float)screenHeight / 2 - 20},
                       40, 1, BLACK);
        }
        break;

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

        // Draw platforms (rounded rectangles)
        for (int i = 0; i < NUM_PLATFORMS; i++)
        {
            Rectangle platformRect = {platforms[i].position.x - platforms[i].width / 2 + shakeOffset.x,  // Center x
                                      platforms[i].position.y - platforms[i].height / 2 + shakeOffset.y, // Center y
                                      platforms[i].width, platforms[i].height};
            DrawRectangleRounded(platformRect, 1.2f, 8, MAROON);
        }

        Rectangle doorRect = {door.position.x - door.width / 2 + shakeOffset.x, door.position.y - door.height / 2,
                              door.width + shakeOffset.y, door.height};

        // Add a subtle shadow for depth
        Rectangle shadowRect = {doorRect.x + 10 + shakeOffset.x, doorRect.y + 5 + shakeOffset.y, doorRect.width,
                                doorRect.height};
        DrawRectangleRec(shadowRect, (Color){0, 0, 0, 50}); // Faint black shadow

        // Door frame (slightly larger, darker outline)
        Rectangle frameRect = {doorRect.x - 4 + shakeOffset.x, doorRect.y - 4 + shakeOffset.y, doorRect.width + 8,
                               doorRect.height + 8};
        DrawRectangleRec(frameRect, (Color){139, 69, 19, 255});

        // Main door body
        DrawRectangleRec(doorRect, (Color){165, 42, 42, 255});

        Rectangle topPanel = {doorRect.x + 4 + shakeOffset.x, doorRect.y + 4 + shakeOffset.y, doorRect.width - 8,
                              (doorRect.height - 12) / 2};
        Rectangle bottomPanel = {doorRect.x + 4 + shakeOffset.x, doorRect.y + doorRect.height / 2 + 2 + shakeOffset.y,
                                 doorRect.width - 8, (doorRect.height - 12) / 2};
        DrawRectangleRec(topPanel, (Color){139, 69, 19, 255});
        DrawRectangleRec(bottomPanel, (Color){139, 69, 19, 255});

        Vector2 handlePos = {doorRect.x + doorRect.width - 6 + shakeOffset.x,
                             doorRect.y + doorRect.height * 0.6f + shakeOffset.y};
        DrawCircleV(handlePos, 2.0f, GOLD);
        DrawCircleLines(handlePos.x, handlePos.y, 2.0f, BLACK);

        if (isPlayer)
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
                else if (i == 2)
                {
                    Rectangle armRect = {unitPos.x, unitPos.y, player.units[i].width, player.units[i].height};
                    DrawRectanglePro(armRect, {player.units[i].width / 2, player.units[i].height / 2}, 35.0f,
                                     drawColor);
                }
                else if (i == 3)
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

        DrawText(TextFormat("Time Left: %.1f", transitionTimer >= 0 ? transitionTimer : 0.0f), 20, 20, 20, WHITE);

        if (pause)
        {
            DrawText("GAME PAUSED", screenWidth / 2 - MeasureText("GAME PAUSED", 40) / 2, screenHeight / 2 - 40, 40,
                     WHITE);
        }
        break;
    }

    case LEVEL_UP:
        ClearBackground(LIGHTGRAY);
        DrawGame();
        DrawTextEx(
            font, "Level Up! Press <S> to continue",
            (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Level Up! Press <S> to continue", 40, 1).x / 2,
                      (float)screenHeight / 2 - 20},
            40, 1, DARKGREEN);
        DrawText(TextFormat("Timer: %.1f / %.1f", transitionTimer, TRANSITION_DURATION), 20, 20, 20, WHITE);
        break;

    case GAME_OVER:
        ClearBackground(BLACK);
        DrawTextEx(font, "Game Over",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Game Over", 50, 1).x / 2,
                             (float)screenHeight / 2 - 50},
                   50, 1, WHITE);
        DrawText("Press [ENTER] to Restart", screenWidth / 2 - MeasureText("Press [ENTER] to Restart", 20) / 2,
                 screenHeight / 2 + 10, 20, WHITE);
        DrawText("Press [M] to return to Menu", screenWidth / 2 - MeasureText("Press [M] to return to Menu", 20) / 2,
                 screenHeight / 2 + 40, 20, WHITE);
        DrawText(TextFormat("Lines Cleared: %i", linesClearedTotal),
                 screenWidth / 2 - MeasureText("Lines Cleared: XX", 20) / 2 - 10, screenHeight / 2 + 77, 25, WHITE);
        break;
    }
    EndDrawing();
}

void UnloadGame()
{
    UnloadFont(font);
    UnloadSound(levelStartSound);
    UnloadSound(doorHitSound);
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
