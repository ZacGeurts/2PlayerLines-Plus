#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "game.h"
#include "audio.h"
#include "collectible.h"
#include "collision.h"
#include "explosion.h"
#include "circle.h"
#include <SDL2/SDL.h>
#include <random>
#include <vector>

struct Game; // Forward declaration
SDL_Color SDLplayercolor = {255, 0, 255, 255}; // Magenta,

class PlayerManager {
public:
    PlayerManager(const GameConfig& config);
    void updatePlayers(SDL_GameController* controllers[], int controllerCount, Player& player1, Player& player2, 
                       Collectible& collectible, std::vector<Explosion>& explosions, std::vector<Flash>& flashes, 
                       int& score1, int& score2, int& roundScore1, int& roundScore2, std::mt19937& rng, 
                       float dt, float currentTimeSec, AudioManager& audio, CollectibleManager& collectibleManager, 
                       ExplosionManager& explosionManager, CircleManager& circleManager, std::vector<Circle>& circles, 
                       std::chrono::steady_clock::time_point& lastCircleSpawn, Game* game,
                       const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight, SDL_Color SDLplayercolor);

private:
    const GameConfig& config;
};

#endif // PLAYER_H