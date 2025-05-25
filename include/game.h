#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <vector>
#include <random>
#include <chrono>
#include "types.h"
#include "audio.h"
#include "collectible.h"
#include "collision.h"
#include "render.h"
#include "circle.h"
#include "explosion.h"
#include "input.h"
#include "player.h"
#include "ai.h"

class Game {
public:
	
	SDL_Color SDLaicolor = {255, 0, 0}; // red
	SDL_Color SDLcirclecolor = {255, 0, 255}; // magenta
	SDL_Color SDLplayercolor = {255, 0, 255}; // magenta
	SDL_Color SDLexplosioncolor = {255, 0, 255}; // magenta
	
	// Forward declaration of PlayerManager
	class PlayerManager;
	
    Game(const GameConfig& config);
    ~Game();
    void run();
    void checkCollision(Player* player, Vec2 nextPos, float currentTimeSec, const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);
    void handleGreenSquareCollection(Player* player, float currentTimeSec);
    void handlePlayerDeath(Player* player, float currentTimeSec);
    void toggleFullscreen();
    void reset();
    void respawnCircles();
    void activateNoCollision(Player* player, float currentTimeSec);
    void resumeAfterWinner();

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
    const GameConfig& config;
    float deathTime;
    float orthoWidth;
    float orthoHeight;
    bool frameRendered;
    float winningScore;
    float greenSquarePoints;
    float deathPoints;

private:
    bool collectibleCollectedThisFrame;
    bool pendingCollectibleRespawn;
    void update(float dt, float currentTimeSec);
    void render();
};

#endif // GAME_H