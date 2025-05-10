#include "collectible.h"
#include "game.h"
#include <GL/gl.h>
#include <SDL2/SDL.h>

CollectibleManager::CollectibleManager(const GameConfig& config) : config(config) {}

Collectible CollectibleManager::spawnCollectible(std::mt19937& rng, const Game& game) const {
    std::uniform_real_distribution<float> distX(config.COLLECTIBLE_SIZE, game.orthoWidth - config.COLLECTIBLE_SIZE);
    std::uniform_real_distribution<float> distY(config.COLLECTIBLE_SIZE, game.orthoHeight - config.COLLECTIBLE_SIZE);
    Collectible collectible{{distX(rng), distY(rng)}, config.GREEN_SQUARE_SIZE};
    return collectible;
}

bool CollectibleManager::checkCollectibleCollision(const Vec2& playerPos, const Collectible& collectible, const Game& game) const {
    float halfSize = config.COLLECTIBLE_SIZE / 2.0f;
    unsigned char pixel[3];
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            Vec2 checkPos(playerPos.x + dx, playerPos.y + dy);
            if (checkPos.x < collectible.pos.x - halfSize || checkPos.x > collectible.pos.x + halfSize ||
                checkPos.y < collectible.pos.y - halfSize || checkPos.y > collectible.pos.y + halfSize) {
                continue;
            }
            float pixelY = game.orthoHeight - checkPos.y;
            glReadPixels((int)checkPos.x, (int)pixelY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
            SDL_Log("Collectible collision check at (%f, %f), pixelY=%f: R=%d, G=%d, B=%d, orthoHeight=%f",
                    checkPos.x, checkPos.y, pixelY, pixel[0], pixel[1], pixel[2], game.orthoHeight);
            if (pixel[0] == 0 && pixel[1] == 255 && pixel[2] == 0) {
                SDL_Log("Green square collected at (%f, %f)", checkPos.x, checkPos.y);
                return true;
            }
        }
    }
    return false;
}