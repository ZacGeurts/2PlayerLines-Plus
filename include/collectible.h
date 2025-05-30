#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H

#include "types.h"
#include <random>

namespace Game { class Game; } // Forward declaration within Game namespace

class CollectibleManager {
public:
    CollectibleManager(const Game::GameConfig& config);
    Game::Collectible spawnCollectible(std::mt19937& rng, const Game::Game& game) const;
    bool checkCollectibleCollision(const Game::Vec2& playerPos, const Game::Collectible& collectible, const Game::Game& game) const;

private:
    const Game::GameConfig& config;
};

#endif