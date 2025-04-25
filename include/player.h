// include/player.h
#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "audio.h"
#include "collectible.h"
#include "collision.h"
#include "explosion.h"
#include "circle.h"
#include <SDL2/SDL.h>
#include <random>
#include <vector>

// Forward declaration for Game
struct Game; // Replace #include "game.h" with this

class PlayerManager {
public:
    PlayerManager(const GameConfig& config);
    void updatePlayers(SDL_GameController* controllers[], int controllerCount, Player& player1, Player& player2, Collectible& collectible, std::vector<Explosion>& explosions, std::vector<Flash>& flashes, int& score1, int& score2, int& roundScore1, int& roundScore2, std::mt19937& rng, float dt, float currentTimeSec, AudioManager& audio, CollectibleManager& collectibleManager, ExplosionManager& explosionManager, CircleManager& circleManager, std::vector<Circle>& circles, std::chrono::steady_clock::time_point& lastCircleSpawn, Game* game);

private:
    const GameConfig& config;
};

#endif // PLAYER_H