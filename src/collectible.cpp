#include "collectible.h"
#include "constants.h"

Collectible spawnCollectible(std::mt19937& rng) {
    std::uniform_real_distribution<float> distX(BLACK_SQUARE_SIZE / 2, WIDTH - BLACK_SQUARE_SIZE / 2);
    std::uniform_real_distribution<float> distY(BLACK_SQUARE_SIZE / 2, HEIGHT - BLACK_SQUARE_SIZE / 2);
    return Collectible{
        Vec2(distX(rng), distY(rng)),
        COLLECTIBLE_SIZE,
        BLACK_CIRCLE_SIZE,
        BLACK_SQUARE_SIZE
    };
}
