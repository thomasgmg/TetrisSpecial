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

float gameTime = 0.0f;
float lateralTimer = 0.0f;
float lateralSpeed = 0.1f;
float fallTimer = 0.0f;
float fallSpeed = 0.5f;
float fastFallSpeed = 0.01;
float fallVelocity = 0.0f;
float fallAcceleration = 4.5f;

const int GRID_OFFSET_X = (screenWidth - GRID_HORIZONTAL_SIZE * BLOCK_SIZE) / 2;
const int GRID_OFFSET_Y = (screenHeight - GRID_VERTICAL_SIZE * BLOCK_SIZE) / 2;

int grid[GRID_VERTICAL_SIZE][GRID_HORIZONTAL_SIZE] = {0};

bool isMenu = true;
bool gameOver = false;
bool pause = false;
bool isInFreeFall = false;

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
    score += linesCleared * 10;

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
        score += 15;
        justClearedGrid = true;
    }
    else
    {
        justClearedGrid = false;
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
        if (isMenu)
        {
            gameTime += GetFrameTime();

            if (IsKeyPressed(KEY_ENTER))
            {
                isMenu = false;
                gameOver = false;

                for (int y = 0; y < GRID_VERTICAL_SIZE; y++)
                    for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                        grid[y][x] = 0;
                spawnPiece();
            }
        }
        else
        {
            gameTime += GetFrameTime();

            if (IsKeyPressed(KEY_M))
                isMenu = true;
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
    randomNumber = (rand() / (RAND_MAX / 2)) + 1;

    if (randomNumber == 1)
        spawnI(currentPiece);
    // else if (randomNumber == 2)
    //     spawnJ(currentPiece);
    // else if (randomNumber == 3)
    // spawnL(currentPiece);
    else if (randomNumber == 2)
        spawnO(currentPiece);
    // else if (randomNumber == 5)
    //     spawnS(currentPiece);
    // else if (randomNumber == 6)
    //     spawnT(currentPiece);
    // else if (randomNumber == 7)
    //     spawnZ(currentPiece);

    fallSpeed = 0.5f;
    fallTimer = 0.0f;
    isInFreeFall = false;
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
    if (IsKeyPressed('P'))
    {
        pause = !pause;
    }

    if (pause)
    {
        return;
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
            fallSpeed = 0.2f;
        }
    }

    else if (IsKeyPressed(KEY_SPACE))
    {
        isInFreeFall = true;
        fallSpeed = 0.01f;
    }

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
            }
        }
    }
}

void DrawGame()
{
    DrawGrid();
    DrawPiece(&currentPiece);
    DrawText(TextFormat("Score: %i", score), 20, 20, 30, BLACK);

    static float bonusTimer = 0.0f;
    if (justClearedGrid)
    {
        bonusTimer = 2.0f;
        justClearedGrid = false;
    }
    if (bonusTimer > 0)
    {
        DrawText("+15 Grid Clear Bonus!", screenWidth / 2 - 100, screenHeight / 2, 25, BLACK);
        bonusTimer -= GetFrameTime();
    }
}

void UpdateDrawFrame(float gameTime)
{
    BeginDrawing();

    if (isMenu)
    {
        Color topColor = (Color){0, 30, 0, 255};
        Color bottomColor = (Color){0, 255, 0, 255};
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, topColor, bottomColor);

        DrawTextEx(font, "Press ENTER to Start",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Start", 50, 1).x / 2,
                             (float)screenHeight / 2 - 20},
                   40, 1, WHITE);
    }
    else if (gameOver) // Game-over state
    {
        float timeFactor = gameTime * 0.2f;
        Color topColor = {(unsigned char)(127 + 127 * sinf(timeFactor)),
                          (unsigned char)(127 + 127 * sinf(timeFactor + 2.0f)),
                          (unsigned char)(127 + 127 * sinf(timeFactor + 4.0f)), 255};
        Color bottomColor = {(unsigned char)(127 + 127 * cosf(timeFactor)),
                             (unsigned char)(127 + 127 * cosf(timeFactor + 2.0f)),
                             (unsigned char)(127 + 127 * cosf(timeFactor + 4.0f)), 255};
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, topColor, bottomColor);

        DrawTextEx(font, "Game Over",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Game Over", 50, 1).x / 2,
                             (float)screenHeight / 2 - 20},
                   40, 1, WHITE);
    }
    else
    {
        ClearBackground(LIGHTGRAY);
        UpdateGame();
        DrawGame();
        UpdateDrawParticles(GetFrameTime());
        if (pause)
            DrawText("GAME PAUSED", screenWidth / 2 - MeasureText("GAME PAUSED", 40) / 2, screenHeight / 2 - 40, 40,
                     BLACK);
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
