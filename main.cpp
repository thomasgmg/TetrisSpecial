#include "raylib.h"

#include <cmath>
#include <cstdio>
#include <ctime>
#include <stdlib.h>

Font font;

int const GRID_HORIZONTAL_SIZE = 16;
int const GRID_VERTICAL_SIZE = 22;
int const BLOCK_SIZE = 27;

int const TOTAL_PIECES_TYPES = 7;

int const screenWidth = 1150;
int const screenHeight = 594;

int level = 1;
int linesClearedTotal = 0;
int const BASE_LINES_PER_LEVEL = 5;
int const LINES_INCREMENT_PER_LEVEL = 5;
float baseFallSpeed = 0.3f;

float gameTime = 0.0f;
float lateralTimer = 0.0f;
float lateralSpeed = 0.1f;
float fallTimer = 0.0f;
float fallSpeed = 0.3f;
float fallVelocity = 0.0f;
float fallAcceleration = 4.5f;

const int GRID_OFFSET_X = (screenWidth - GRID_HORIZONTAL_SIZE * BLOCK_SIZE) / 2;
const int GRID_OFFSET_Y = (screenHeight - GRID_VERTICAL_SIZE * BLOCK_SIZE) / 2;

int grid[GRID_VERTICAL_SIZE][GRID_HORIZONTAL_SIZE] = {0};

bool isMenu = true;
bool gameOver = false;
bool pause = false;
bool isInFreeFall = false;

float pulseTimer = 0.0f;
bool showPulseEffect = false;
float const PULSE_DURATION = 2.5f;

struct Unit
{
    Vector2 position;
};

struct Tetromino
{
    Unit units[4];
    int size = 4;
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

enum GameState
{
    MENU,
    WAITING_TO_START,
    PLAYING,
    LEVEL_UP
};
GameState gameState = MENU;

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
    float currentRadius = maxRadius * progress;

    Vector2 center = {(float)screenWidth / 2, (float)screenHeight / 2};

    // Draw multiple pulsing rings
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

        // Update particle
        p.position.x += p.velocity.x * deltaTime * 60;
        p.position.y += p.velocity.y * deltaTime * 60;
        p.life -= deltaTime;
        p.alpha = p.life / 1.5f;

        // Draw particle
        Color particleColor = p.color;
        particleColor.a = (unsigned char)(p.alpha * 255);
        DrawRectanglePro({p.position.x, p.position.y, p.size, p.size}, {p.size / 2, p.size / 2},
                         GetTime() * 90, // Rotate particles over time
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

    // Award 15 extra points if grid was full and is now empty
    if (isGridEmpty && linesCleared > 0)
    {
        score += 50;
        justClearedGrid = true;
        showPulseEffect = true; // Trigger the pulse effect
        pulseTimer = PULSE_DURATION;
    }
    else
    {
        justClearedGrid = false;
    }

    int linesNeeded = BASE_LINES_PER_LEVEL + (level - 1) * LINES_INCREMENT_PER_LEVEL;

    if (linesClearedTotal >= linesNeeded)
    {
        level++;
        fallSpeed = baseFallSpeed / (1.0f + (level - 1) * 0.1f);
        for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
            for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                grid[y][x] = 0;
        score = 0;
        linesClearedTotal = 0;
        spawnPiece();
        gameState = LEVEL_UP;
    }
    return linesCleared;
}

int main()
{
    srand(time(0));
    InitWindow(screenWidth, screenHeight, "Classic Game: TETRIS");
    spawnPiece();
    SetTargetFPS(60);
    font = LoadFontEx("resources/font.ttf", 96, 0, 0);

    while (!WindowShouldClose())
    {
        gameTime += GetFrameTime();

        switch (gameState)
        {
        case MENU:
            if (IsKeyPressed(KEY_ENTER))
            {
                gameState = WAITING_TO_START;
                for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
                    for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                        grid[y][x] = 0;
                score = 0;
                level = 1;
                linesClearedTotal = 0;
                fallSpeed = baseFallSpeed;
                spawnPiece();
                gameOver = false;
            }
            break;

        case WAITING_TO_START:
            if (IsKeyPressed('S'))
            {
                gameState = PLAYING;
            }
            break;

        case PLAYING:
            if (IsKeyPressed(KEY_M))
            {
                gameState = MENU;
            }
            break;

        case LEVEL_UP:
            if (IsKeyPressed('S'))
            {
                gameState = PLAYING;
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
        spawnS(currentPiece);
    else if (randomNumber == 2)
        spawnJ(currentPiece);
    else if (randomNumber == 3)
        spawnL(currentPiece);
    else if (randomNumber == 4)
        spawnO(currentPiece);
    else if (randomNumber == 5)
        spawnI(currentPiece);
    else if (randomNumber == 6)
        spawnT(currentPiece);
    else if (randomNumber == 7)
        spawnZ(currentPiece);

    fallTimer = 0.0f;
    isInFreeFall = false;
    fallSpeed = baseFallSpeed / (1.0f + (level - 1) * 0.1f);
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

    if (IsKeyPressed('P'))
    {
        pause = !pause;
    }

    if (pause)
    {
        return;
    }

    if (IsKeyPressed(KEY_LEFT))
    {
        if (canMoveHorizontally(currentPiece, -1))
        {
            for (int i = 0; i < currentPiece.size; i++)
            {
                currentPiece.units[i].position.x -= 1;
            }
        }
    }
    if (IsKeyPressed(KEY_RIGHT))
    {
        if (canMoveHorizontally(currentPiece, 1))
        {
            for (int i = 0; i < currentPiece.size; i++)
            {
                currentPiece.units[i].position.x += 1;
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
        {
            fallSpeed = 0.05f;
        }
    }

    else if (IsKeyPressed(KEY_SPACE))
    {
        isInFreeFall = true;
        fallSpeed = 0.01f;
    }

    float levelAdjustedFallSpeed = baseFallSpeed / (1.0f + (level - 1) * 0.1f);

    fallTimer += GetFrameTime();
    if (fallTimer >= fallSpeed)
    {
        fallTimer = 0.0f;
        if (canMoveDown(currentPiece))
        {
            moveDown(currentPiece);
        }
        else
        {
            for (int i = 0; i < currentPiece.size; i++)
            {
                int x = (int)currentPiece.units[i].position.x;
                int y = (int)currentPiece.units[i].position.y;
                grid[y][x] = 1;
            }

            (void)checkAndClearLines();
            if (gameState != LEVEL_UP)
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
    DrawPiece(&currentPiece);
    DrawText(TextFormat("Score: %i", score), 20, 60, 30, BLACK);
    DrawText(TextFormat("Level: %i", level), 20, 20, 30, BLACK);
    DrawText(TextFormat("Lines: %i", linesClearedTotal), 20, 100, 30, BLACK);

    int linesNeeded = BASE_LINES_PER_LEVEL + (level - 1) * LINES_INCREMENT_PER_LEVEL;
    DrawText(TextFormat("Next Level: %i/%i lines", linesClearedTotal, linesNeeded), 20, 140, 20, DARKGRAY);

    if (linesClearedTotal < linesNeeded)
    {
        DrawText("Clear lines to advance!", 20, 180, 20, GRAY);
    }
    else
    {
        DrawText("Level Up!", 20, 180, 20, GREEN);
    }
    float progress = (float)linesClearedTotal / linesNeeded;
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

    switch (gameState)
    {
    case MENU: {
        ClearBackground(BLACK);
        // DrawTextEx(font, "Welcome to TETRIS SPECIAL",
        //            (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Welcome to TERIS SPECIAL", 50, 1).x / 2,
        //                      (float)screenHeight / 2 - 40},
        //            40, 1, WHITE);
        // DrawTextEx(font, "Press ENTER to Start",
        //            (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Start", 50, 1).x / 2 + 30,
        //                      (float)screenHeight / 2 + 40},
        //            30, 1, WHITE);const char* welcomeText = "Welcome to TETRIS SPECIAL";Vector2 textPos =
        //            {(float)screenWidth / 2 - MeasureTextEx(font, welcomeText, 40, 1).x / 2,

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
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Start", 50, 1).x / 2 + 30,
                             (float)screenHeight / 2 + 40},
                   30, 1, WHITE);
        break;
    }

    case WAITING_TO_START:
        ClearBackground(LIGHTGRAY);
        DrawGame();
        DrawTextEx(font, "Press <S> to Begin",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press <S> to Begin", 40, 1).x / 2,
                             (float)screenHeight / 2 - 40},
                   40, 1, BLACK);
        DrawTextEx(font, "Use arrows to move",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press <S> to Begin", 40, 1).x / 2 + 50,
                             (float)screenHeight / 2},
                   25, 1, BLACK);
        DrawTextEx(font, "Press <UP> to rotate",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press <S> to Begin", 40, 1).x / 2 + 40,
                             (float)screenHeight / 2 + 25},
                   25, 1, BLACK);
        DrawTextEx(font, "Press <Down> for fast fall and <Space> for free fall",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press <S> to Begin", 40, 1).x / 2 - 130,
                             (float)screenHeight / 2 + 50},
                   25, 1, BLACK);
        break;

    case PLAYING:
        ClearBackground(LIGHTGRAY);
        UpdateGame();
        DrawGame();
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

    case LEVEL_UP:
        ClearBackground(LIGHTGRAY);
        DrawGame();
        DrawTextEx(
            font, "Level Up! Press <S> to continue",
            (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Level Up! Press <S> to continue", 40, 1).x / 2,
                      (float)screenHeight / 2 - 20},
            40, 1, DARKGREEN);
        break;
    }
    EndDrawing();
}

void UnloadGame()
{
    UnloadFont(font);
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
    if (isInFreeFall)
    {
        return false;
    }

    for (int i = 0; i < currentPiece.size; i++)
    {
        if (currentPiece.units[i].position.y >= GRID_VERTICAL_SIZE - 1)
        {
            return false; // No lateral movement if touching ground
        }
    }

    for (int i = 0; i < currentPiece.size; i++)
    {
        Vector2 newPos = currentPiece.units[i].position;
        newPos.x += amount;

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
