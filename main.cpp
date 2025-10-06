/*******************************************************************************************
* "Dodge!" 
*
* GOAL
*   - Move the player (green rounded square) with WASD/Arrow Keys
*   - Avoid the falling red blocks
*   - Score increases the longer you survive
*
* CONTROLS
*   - Move:  WASD or Arrow keys
*   - Start: SPACE (from Menu)
*   - Restart: R (from Game Over)
*   - Back to Menu: ESC (from Game Over)
*
* STRUCTURE
*   - ResetGame() initialises player, enemies, and score (random sizing and speed of enemies)
*   - Update loop handles input, movement, collisions, and scoring
*   - Draw section renders depending on current state -  Weather API Open-meteo used to check weather state
*******************************************************************************************/

#include "raylib.h"
#include <vector>
//#include <string>
#include <cmath>



// --- : Weather bridge (lets JS set the current weather) -----------------
enum class WeatherKind { SUNNY = 0, CLOUDY = 1, RAINY = 2 };
static WeatherKind gWeather = WeatherKind::SUNNY; // default if fetch fails
static const char* WeatherName()
{
    switch (gWeather) {
        case WeatherKind::CLOUDY: return "Cloudy";
        case WeatherKind::RAINY:  return "Rainy";
        default:                  return "Sunny";
    }
}

#ifdef __EMSCRIPTEN__
  #include <emscripten/emscripten.h>
  extern "C" { EMSCRIPTEN_KEEPALIVE void SetWeather(int kind) { gWeather = (WeatherKind)kind; } }
#else
  // Desktop stub (so it compiles/runs without emscripten)
  extern "C" void SetWeather(int kind) { gWeather = (WeatherKind)kind; }
#endif

// -----------------------------------------------------------------------------------------
// Data types 
// -----------------------------------------------------------------------------------------

// Player data: a rectangle for position/size and a movement speed in pixels/sec
struct Player {
    Rectangle rect;      // x, y, width, height
    float speed = 260.0f;
};

// --- : Enemy has a kind so we can draw sun/cloud/rain
struct Enemy {
    Rectangle rect;
    float speedY;
    int kind; // 0 sun, 1 cloud, 2 rain (mirrors WeatherKind)
};

// Simple game-state enum to control which screen/logic is active
enum class GameState { MENU, PLAYING, GAME_OVER };

// -----------------------------------------------------------------------------------------
// screen size 
// -----------------------------------------------------------------------------------------
static const int SCREEN_W = 800;
static const int SCREEN_H = 450;

// -----------------------------------------------------------------------------------------
// Reset everything needed for a new run:
//   - Centre player near bottom
//   - Spawn enemies above the screen with random sizes/speeds
//   - Reset score
// -----------------------------------------------------------------------------------------
void ResetGame(Player &player, std::vector<Enemy> &enemies, int enemyCount, float &score) {
    // Player rectangle: centered horizontally, a bit above the bottom
    player.rect = { SCREEN_W/2.0f - 18.0f, SCREEN_H - 70.0f, 36.0f, 36.0f };

    // Start with a clean enemy list
    enemies.clear();
    enemies.reserve(enemyCount);

    // --- : spawn based on current weather kind
    for (int i = 0; i < enemyCount; ++i) {
        int kind = (int)gWeather;
        float x = (float)GetRandomValue(0, SCREEN_W - 40);
        float y = (float)GetRandomValue(-SCREEN_H, -20);

        float w, h, speed;
        if (kind == 2) {
            // RAIN: thin, long drops
            w = (float)GetRandomValue(3, 6);
            h = (float)GetRandomValue(14, 24);
            speed = 180.0f + (float)GetRandomValue(40, 180);
        } else if (kind == 1) {
            // CLOUD: wider, slower puffs
            w = (float)GetRandomValue(40, 72);
            h = (float)GetRandomValue(24, 40);
            speed = 100.0f + (float)GetRandomValue(20, 80);
        } else {
            // SUN: circles (use rect as bounds), medium
            w = h = (float)GetRandomValue(18, 30);
            speed = 140.0f + (float)GetRandomValue(20, 120);
        }

        enemies.push_back({ Rectangle{ x, y, w, h }, speed, kind });
    }

    // Score is time-based (accumulates while you survive)
    score = 0.0f;
}

int main() {
    // -------------------------------------------------------------------------------------
    // Window + timing setup
    // -------------------------------------------------------------------------------------
    InitWindow(SCREEN_W, SCREEN_H, "Dodge");
    SetTargetFPS(60); // lock to 60 FPS; GetFrameTime() still gives real delta-time

    // -------------------------------------------------------------------------------------
    // Game state + entities + score
    // -------------------------------------------------------------------------------------
    GameState state = GameState::MENU;   // start at menu

    Player player{};                     // will be positioned in ResetGame()
    std::vector<Enemy> enemies;          // container for falling enemies
    float score = 0.0f;                  // current run score (seconds * 60)
    int bestScore = 0;                   // best score across runs (integer)

    const int ENEMY_COUNT = 10;          // how many enemies to manage

    // Initialize the first run (even though start is MENU, this sets baseline)
    ResetGame(player, enemies, ENEMY_COUNT, score);

    // -------------------------------------------------------------------------------------
    // Main game loop
    //   - Runs until the user closes the window (Esc or close button)
    // -------------------------------------------------------------------------------------
    while (!WindowShouldClose()) {
        // =============================================================================
        // UPDATE (handle input, move entities, detect collisions, update score)
        // =============================================================================

        if (state == GameState::MENU) {
            // On menu, wait for SPACE/ENTER to start a new game
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                ResetGame(player, enemies, ENEMY_COUNT, score);
                state = GameState::PLAYING;
            }
        }
        else if (state == GameState::PLAYING) {
            // ------------------------------
            // 1) Player input and movement
            // ------------------------------

            // Movement direction vector (unit-length after normalisation)
            Vector2 move{0, 0};

            // Support both Arrows and WASD
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move.x += 1;
            if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) move.x -= 1;
            if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) move.y += 1;
            if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) move.y -= 1;

            // Normalise diagonal movement so speed stays consistent in all directions
            if (move.x != 0 || move.y != 0) {
                float len = sqrtf(move.x*move.x + move.y*move.y);
                move.x /= len; 
                move.y /= len;
            }

            // Move the player by (speed * deltaTime)
            float dt = GetFrameTime();
            player.rect.x += move.x * player.speed * dt;
            player.rect.y += move.y * player.speed * dt;

            // Keep player fully on screen (clamp)
            if (player.rect.x < 0) player.rect.x = 0;
            if (player.rect.y < 0) player.rect.y = 0;
            if (player.rect.x + player.rect.width > SCREEN_W) 
                player.rect.x = SCREEN_W - player.rect.width;
            if (player.rect.y + player.rect.height > SCREEN_H) 
                player.rect.y = SCREEN_H - player.rect.height;

            // Debug keys (desktop/web) to cycle weather quickly (optional)
            if (IsKeyPressed(KEY_F1)) { gWeather = WeatherKind::SUNNY; }
            if (IsKeyPressed(KEY_F2)) { gWeather = WeatherKind::CLOUDY; }
            if (IsKeyPressed(KEY_F3)) { gWeather = WeatherKind::RAINY; }

            // ------------------------------
            // 2) Enemies: fall + recycle + collide
            // ------------------------------
            for (auto &e : enemies) {
                // Fall down by speed * dt
                e.rect.y += e.speedY * dt;

                // If this enemy goes below the bottom, recycle it above the screen
                if (e.rect.y > SCREEN_H + 10) {
                    e.rect.y = (float)GetRandomValue(-200, -20);              // back above
                    e.rect.x = (float)GetRandomValue(0, SCREEN_W - (int)e.rect.width); // new X

                    // keep same kind, new speed within kind range
                    if (e.kind == 2) e.speedY = 180.0f + (float)GetRandomValue(40, 180);
                    else if (e.kind == 1) e.speedY = 100.0f + (float)GetRandomValue(20, 80);
                    else e.speedY = 140.0f + (float)GetRandomValue(20, 120);
                }

                // Collision: if any enemy overlaps the player, game over
                if (CheckCollisionRecs(player.rect, e.rect)) {
                    state = GameState::GAME_OVER;

                    // Update best score if current score is higher
                    if ((int)score > bestScore) bestScore = (int)score;
                }
            }

            // ------------------------------
            // 3) Scoring
            // ------------------------------
            // Score increases as long as you survive.
            // add 60 per second to feel like "points per second" 
            score += 60.0f * dt;

        }
        else if (state == GameState::GAME_OVER) {
            // From the GAME OVER screen, allow restart or return to menu
            if (IsKeyPressed(KEY_R)) {
                ResetGame(player, enemies, ENEMY_COUNT, score);
                state = GameState::PLAYING;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                // ESC to go back to the MENU from GAME OVER
                state = GameState::MENU;
            }
        }

        // =============================================================================
        // DRAW (render the current state)
        // =============================================================================
        BeginDrawing();

        // --- : background colour depends on weather 
        Color bg = {18,18,18,255};
        if (gWeather == WeatherKind::SUNNY)      bg = Color{ 20, 24, 34, 255 };  // bluish
        else if (gWeather == WeatherKind::CLOUDY) bg = Color{ 35, 35, 45, 255 }; // dark grey
        else if (gWeather == WeatherKind::RAINY)  bg = Color{ 15, 18, 30, 255 }; // deep blue
        ClearBackground(bg);

        if (state == GameState::MENU) {
            // -------------- MENU SCREEN --------------
            const char *title = "DODGE THE WEATHER";
            int titleSize = 60;
            int tw = MeasureText(title, titleSize);

            DrawText(title, SCREEN_W/2 - tw/2, 90, titleSize, RAYWHITE);
            DrawText("Move with WASD or Arrow Keys", 220, 200, 20, GRAY);
            DrawText("Avoid the falling blocks",      280, 230, 20, GRAY);
            DrawText("Press SPACE to start",          280, 280, 24, LIGHTGRAY);

            DrawText(TextFormat("Best: %d", bestScore), 10, 10, 20, GRAY);
        }

        if (state == GameState::PLAYING) {
            // -------------- GAMEPLAY RENDER --------------

            // Draw player (rounded green square)
            DrawRectangleRounded(player.rect, 0.2f, 6, Color{ 80, 200, 120, 255 });

            // --- : Draw enemies by type (sun/cloud/rain)
            for (auto &e : enemies) {
                if (e.kind == 2) {
                    // RAIN: blue thin rect
                    DrawRectangleRec(e.rect, Color{ 70, 140, 255, 255 });
                } else if (e.kind == 1) {
                    // CLOUD: three overlapping white circles inside the rect area
                    float cx = e.rect.x + e.rect.width*0.5f;
                    float cy = e.rect.y + e.rect.height*0.6f;
                    float r1 = e.rect.height*0.55f;
                    float r2 = r1*0.85f, r3 = r1*0.85f;
                    DrawCircle((int)cx,               (int)cy,   (int)r1, RAYWHITE);
                    DrawCircle((int)(cx - r1*0.9f),   (int)(cy+2),(int)r2, RAYWHITE);
                    DrawCircle((int)(cx + r1*0.9f),   (int)(cy+2),(int)r3, RAYWHITE);
                } else {
                    // SUN: yellow circle
                    float r = e.rect.width * 0.5f;
                    DrawCircle((int)(e.rect.x + r), (int)(e.rect.y + r), (int)r, Color{ 250, 210, 60, 255 });
                }
            }

            // HUD: Score and FPS
            DrawText(TextFormat("Score: %d", (int)score), 10, 10, 22, RAYWHITE);
            DrawText(TextFormat("London weather: %s", WeatherName()), 10, 40, 20, RAYWHITE);

        }

        if (state == GameState::GAME_OVER) {
            // -------------- GAME OVER OVERLAY --------------

            // Dim the current frame
            DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Color{0, 0, 0, 130});

            // Big title
            const char* over = "GAME OVER";
            DrawText(over, SCREEN_W/2 - MeasureText(over, 50)/2, 120, 50, RAYWHITE);

            // Show score and best score
            DrawText(TextFormat("Score: %d", (int)score), SCREEN_W/2 - 80, 190, 30, LIGHTGRAY);
            DrawText(TextFormat("Best:  %d", bestScore),  SCREEN_W/2 - 80, 225, 24, GRAY);

            // Hints
            DrawText("Press R to Restart", SCREEN_W/2 - 120, 270, 22, RAYWHITE);
            DrawText("Press ESC for Menu", SCREEN_W/2 - 120, 300, 20, GRAY);
        }

        EndDrawing();
    }

    // -------------------------------------------------------------------------------------
    // 
    // -------------------------------------------------------------------------------------
    CloseWindow();
    return 0;
}
