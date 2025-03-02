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
    for (int y = GRID_VERTICAL_SIZE - 1; y >= 0; y--)
    {
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
            for (int aboveY = y; aboveY > 0; aboveY--)
            {
                for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
                {
                    grid[aboveY][x] = grid[aboveY - 1][x];
                }
            }
            for (int x = 0; x < GRID_HORIZONTAL_SIZE; x++)
            {
                grid[0][x] = 0;
            }
            ++y;
        }
    }
    return linesCleared;
}

int score = 0;

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
    //     spawnL(currentPiece);
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

            int linesCleared = checkAndClearLines();

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
}

void UpdateDrawFrame(float gameTime)
{
    BeginDrawing();

    // Draw animated gradient background
    Color topColor =
        (Color){(unsigned char)(127 + 127 * sinf(gameTime)), 0, (unsigned char)(127 + 127 * cosf(gameTime)), 255};
    Color bottomColor = (Color){0, (unsigned char)(127 + 127 * sinf(gameTime * 1.5f)),
                                (unsigned char)(127 + 127 * cosf(gameTime * 0.5f)), 255};
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, topColor, bottomColor);

    if (isMenu)
    {
        DrawTextEx(font, "Press ENTER to Start",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Press ENTER to Start", 50, 1).x / 2,
                             (float)screenHeight / 2 - 20},
                   40, 1, WHITE);
    }
    else if (gameOver) // Game-over state
    {
        DrawTextEx(font, "Game Over",
                   (Vector2){(float)screenWidth / 2 - MeasureTextEx(font, "Game Over", 50, 1).x / 2,
                             (float)screenHeight / 2 - 20},
                   40, 1, WHITE);
    }
    else
    {
        UpdateGame();
        DrawGame();
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
