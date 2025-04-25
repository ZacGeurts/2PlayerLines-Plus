// src/collectible.cpp
#include "collectible.h"
#include <cmath>

CollectibleManager::CollectibleManager(const GameConfig& config) : config(config) {}

Collectible CollectibleManager::spawnCollectible(std::mt19937& rng) const {
    std::uniform_real_distribution<float> distX(config.BLACK_SQUARE_SIZE / 2, config.WIDTH - config.BLACK_SQUARE_SIZE / 2);
    std::uniform_real_distribution<float> distY(config.BLACK_SQUARE_SIZE / 2, config.HEIGHT - config.BLACK_SQUARE_SIZE / 2);
    return Collectible{
        Vec2(distX(rng), distY(rng)),
        config.COLLECTIBLE_SIZE,
        config.BLACK_CIRCLE_SIZE,
        config.BLACK_SQUARE_SIZE
    };
}

bool CollectibleManager::checkCollectibleCollision(const Vec2& playerPos, const Collectible& collectible) const {
    float dx = playerPos.x - collectible.pos.x;
    float dy = playerPos.y - collectible.pos.y;
    return sqrt(dx * dx + dy * dy) < collectible.size / 2;
}