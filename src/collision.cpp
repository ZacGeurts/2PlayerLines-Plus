// src/collision.cpp
#include "collision.h"
#include <GL/gl.h>
#include <SDL2/SDL.h>

CollisionManager::CollisionManager(const GameConfig& config) : config(config) {}

bool CollisionManager::checkPixelCollision(const Vec2& pos) const {
    GLubyte pixel[3];
    glReadPixels((int)pos.x, config.HEIGHT - (int)pos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    bool collision = !( (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) ||
                        (pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 255) );
    if (collision) {
        SDL_Log("Collision at (%f, %f): RGB(%d, %d, %d)", pos.x, pos.y, pixel[0], pixel[1], pixel[2]);
    }
    return collision;
}

bool CollisionManager::checkAreaCollision(const Vec2& center, int size) const {
    int halfSize = size / 2;
    for (int dx = -halfSize; dx <= halfSize; dx++) {
        for (int dy = -halfSize; dy <= halfSize; dy++) {
            Vec2 checkPos(center.x + dx, center.y + dy);
            if (checkPos.x < 0 || checkPos.x >= config.WIDTH || checkPos.y < 0 || checkPos.y >= config.HEIGHT) continue;
            if (checkPixelCollision(checkPos)) return true;
        }
    }
    return false;
}