// include/collectible.h
#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H

#include "types.h"
#include <random>

class CollectibleManager {
public:
    CollectibleManager(const GameConfig& config);
    Collectible spawnCollectible(std::mt19937& rng) const;
    bool checkCollectibleCollision(const Vec2& playerPos, const Collectible& collectible) const;

private:
    const GameConfig& config;
};

#endif // COLLECTIBLE_H