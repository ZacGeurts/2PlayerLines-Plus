#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "constants.h"
#include "audio.h"
#include "collectible.h"
#include "collision.h"
#include "render.h"
#include "circle.h"
#include "explosion.h"
#include "input.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <random>
#include <vector>
#include <chrono>

// Forward declaration for PlayerManager
class PlayerManager; // Resolves undefined type

#include "player.h" // Provides full PlayerManager definition

struct Game {
    SDL_Window* window;
    SDL_GLContext glContext;
    AudioManager audio;
    CollectibleManager collectibleManager;
    CollisionManager collisionManager;
    RenderManager renderManager;
    CircleManager circleManager;
    ExplosionManager explosionManager;
    InputManager inputManager;
    PlayerManager playerManager;
    GLuint splashTexture;
    bool isSplashScreen;
    bool paused;
    SDL_GameController* controllers[2];
    int controllerCount;
    Player player1, player2;
    std::vector<Circle> circles;
    Collectible collectible;
    std::vector<Explosion> explosions;
    std::vector<Flash> flashes;
    std::mt19937 rng;
    float lastBoopTime;
    int score1, score2;
    int roundScore1, roundScore2;
    bool gameOver;
    bool gameOverScreen;
    bool firstFrame;
    std::chrono::steady_clock::time_point lastCircleSpawn;
    std::chrono::steady_clock::time_point gameOverTime;
    GameConfig config;
    float deathTime; // Added to track last death time

    Game(const GameConfig& config);
    ~Game();
    void checkCollision(Player* player, Vec2 nextPos, float currentTimeSec);
    void run();
    void update(float dt, float currentTimeSec);
    void render();
    void reset();
    void activateNoCollision(Player* player, float currentTimeSec);
    void toggleFullscreen();
};

#endif // GAME_H