#include "collision.h"
#include "game.h"
#include <GL/gl.h>
#include <SDL2/SDL.h>

CollisionManager::CollisionManager(const GameConfig& config) : config(config) {}

bool CollisionManager::checkPixelCollision(const Vec2& pos, const Game& game) const {
    GLubyte pixel[3];
    float pixelY = game.orthoHeight - pos.y;
    glReadPixels((int)pos.x, (int)pixelY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    // Safe colors: black (0, 0, 0), magenta (255, 0, 255), green (0, 255, 0)
    // Deadly colors: yellow (255, 255, 0), player lines (e.g., red, blue), walls (non-black, non-safe)
    bool collision = !( (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) ||   // Black (background)
                        (pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 255) || // Magenta
                        (pixel[0] == 0 && pixel[1] == 255 && pixel[2] == 0) );   // Green
    if (collision) {
        SDL_Log("Collision at (%f, %f), pixelY=%f: RGB(%d, %d, %d), orthoHeight=%f", 
                pos.x, pos.y, pixelY, pixel[0], pixel[1], pixel[2], game.orthoHeight);
    }
    return collision;
}

bool CollisionManager::checkAreaCollision(const Vec2& center, int size, const Game& game) const {
    int halfSize = size / 2;
    for (int dx = -halfSize; dx <= halfSize; dx++) {
        for (int dy = -halfSize; dy <= halfSize; dy++) {
            Vec2 checkPos(center.x + dx, center.y + dy);
            if (checkPos.x < 0 || checkPos.x >= game.orthoWidth || checkPos.y < 0 || checkPos.y >= game.orthoHeight) {
                SDL_Log("Wall collision at (%f, %f), orthoWidth=%f, orthoHeight=%f", 
                        checkPos.x, checkPos.y, game.orthoWidth, game.orthoHeight);
                return true; // Wall collision
            }
            if (checkPixelCollision(checkPos, game)) return true; // Yellow, lines, or other deadly colors
        }
    }
    return false;
}