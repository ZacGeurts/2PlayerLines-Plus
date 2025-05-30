#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <random>
#include <chrono>
#include "types.h"
#include "audio.h"
#include "collectible.h" // For CollectibleManager
#include "explosion.h"   // For ExplosionManager
#include "circle.h"      // For CircleManager

namespace Game {
class Game;

class PlayerManager {
public:
    PlayerManager() = default;
    PlayerManager(const GameConfig& config); // Declare constructor
    void updatePlayers(SDL_GameController* controllers[], int controllerCount,
                       Player& player1, Player& player2, Collectible& collectible,
                       std::vector<Explosion>& explosions, std::vector<Flash>& flashes,
                       int& score1, int& score2, int& roundScore1, int& roundScore2,
                       std::mt19937& rng, float dt, float currentTimeSec,
                       AudioManager& audioManager, CollectibleManager& collectibleManager,
                       ExplosionManager& explosionManager, CircleManager& circleManager,
                       std::vector<Circle>& circles, std::chrono::steady_clock::time_point& lastCircleSpawn,
                       Game* game, const std::vector<unsigned char>& framebuffer,
                       int drawableWidth, int drawableHeight, SDL_Color aiColor);

private:
    const GameConfig& config; // Store config for respawn positions
};

} // namespace Game

#endif // PLAYER_H