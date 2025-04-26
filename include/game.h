#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "audio.h"
#include "collectible.h"
#include "collision.h"
#include "render.h"
#include "circle.h"
#include "explosion.h"
#include "input.h"
#include "ai.h"
#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <random>
#include <vector>
#include <chrono>

// Forward declaration for PlayerManager
class PlayerManager;

#include "player.h"

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
    AI ai;
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
    int setScore1, setScore2;
    bool gameOver;
    bool gameOverScreen;
    bool winnerDeclared; // Added for winner state
    bool firstFrame;
    std::chrono::steady_clock::time_point lastCircleSpawn;
    std::chrono::steady_clock::time_point gameOverTime;
    float lastWinnerVoiceTime; // Added for winner voice timing
    GameConfig config;
    float deathTime;

    Game(const GameConfig& config);
    ~Game();
    void checkCollision(Player* player, Vec2 nextPos, float currentTimeSec);
    void run();
    void update(float dt, float currentTimeSec);
    void render();
    void reset();
    void activateNoCollision(Player* player, float currentTimeSec);
    void toggleFullscreen();
    void resumeAfterWinner(); // Added for resuming after winner
};

#endif // GAME_H