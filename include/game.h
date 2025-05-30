#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <vector>
#include <memory>
#include <chrono>
#include <random>
#include "types.h"
#include "audio.h"
#include "ai.h"

namespace Game {

class AI;

class Game {
public:
    Game(const GameConfig& config);
    ~Game();
    void run();
    void update(float dt, float currentTimeSec);
    void render(float currentTimeSec);
    void reset();
    void respawnCircles(float currentTimeSec);
    void activateNoCollision(Player* player, float currentTimeSec, float dt);
    void toggleFullscreen();
    void resumeAfterWinner();
    void checkCollision(Player* player, Vec2 nextPos, float currentTimeSec,
                        const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight, float dt);
    void handleGreenSquareCollection(Player* player, float currentTimeSec);
    void handlePlayerDeath(Player* player, float currentTimeSec);

    // Public members
    const GameConfig& config;
    AudioManager audioManager;
    SDL_Color playerColor;
    SDL_Color aiColor;
    SDL_Color explosionColor;
    SDL_Window* window;
    SDL_GLContext glContext;
    std::unique_ptr<AI> ai;
    GLuint splashTexture;
    bool isSplashScreen;
    bool paused;
    SDL_GameController* controllers[2];
    int controllerCount;
    Player player1;
    Player player2;
    std::vector<Circle> circles;
    Collectible collectible;
    std::vector<Explosion> explosions;
    std::vector<Flash> flashes;
    std::mt19937 rng;
    float lastBoopTime;
    int score1;
    int score2;
    int roundScore1;
    int roundScore2;
    int setScore1;
    int setScore2;
    bool gameOver;
    bool gameOverScreen;
    bool winnerDeclared;
    bool firstFrame;
    std::chrono::steady_clock::time_point lastCircleSpawn;
    std::chrono::steady_clock::time_point gameOverTime;
    float lastWinnerVoiceTime;
    float deathTime;
    float orthoWidth;
    float orthoHeight;
    bool frameRendered;
    float winningScore;
    float greenSquarePoints;
    float deathPoints;
    bool collectibleCollectedThisFrame;
    bool pendingCollectibleRespawn;

private:
    Collectible spawnCollectible(std::mt19937& rng);
    void spawnInitialCircle(std::mt19937& rng, float currentTimeSec);
    void updateCircles(float dt, float currentTimeSec);
    bool handleInput();
};

} // namespace Game

#endif // GAME_H