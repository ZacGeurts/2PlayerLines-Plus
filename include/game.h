#ifndef GAME_H
#define GAME_H

#include <GL/gl.h>
#include "types.h"
#include "audio.h"
#include "render.h"
#include "collision.h"
#include "collectible.h"
#include "explosion.h"
#include <random>
#include <chrono>
#include <vector>

// Forward declaration of Game class
class Game;

#include "input.h"

class Game {
public:
    Game();
    ~Game();
    void run();
    void toggleFullscreen();

private:
    void update(float dt, float currentTimeSec);
    void render();
    void reset();
    void checkCollision(Player* player, Vec2 nextPos, float currentTimeSec);

    SDL_Window* window;
    SDL_GLContext glContext;
    SDL_AudioDeviceID explosionAudioDevice;
    SDL_AudioDeviceID boopAudioDevice;
    bool boopPlaying;
    BoopAudioData boopData;

    SDL_GameController* controllers[2];
    int controllerCount;

    std::mt19937 rng;
    Player player1, player2;
    std::vector<Circle> circles;
    Collectible collectible;
    std::vector<Explosion> explosions;
    float lastBoopTime;
    int score1, score2;
    int roundScore1, roundScore2;
    bool gameOver;
    bool gameOverScreen;
    bool paused;
    bool firstFrame;
    bool isSplashScreen;
    GLuint splashTexture;
    std::chrono::steady_clock::time_point lastCircleSpawn;
    std::chrono::steady_clock::time_point gameOverTime;
};

#endif // GAME_H