#include "collision.h"
#include "constants.h"
#include <GL/gl.h>
#include <SDL2/SDL.h>

bool checkPixelCollision(const Vec2& pos) {
    GLubyte pixel[3];
    glReadPixels((int)pos.x, HEIGHT - (int)pos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    bool collision = !(pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0);
    if (collision) {
        SDL_Log("Collision at (%f, %f): RGB(%d, %d, %d)", pos.x, pos.y, pixel[0], pixel[1], pixel[2]);
    }
    return collision;
}

bool checkAreaCollision(const Vec2& center, int size) {
    int halfSize = size / 2;
    for (int dx = -halfSize; dx <= halfSize; dx++) {
        for (int dy = -halfSize; dy <= halfSize; dy++) {
            Vec2 checkPos(center.x + dx, center.y + dy);
            if (checkPos.x < 0 || checkPos.x >= WIDTH || checkPos.y < 0 || checkPos.y >= HEIGHT) continue;
            if (checkPixelCollision(checkPos)) return true;
        }
    }
    return false;
}

bool checkCollectibleCollision(const Vec2& playerPos, const Collectible& collectible) {
    float halfSize = collectible.size / 2;
    return playerPos.x >= collectible.pos.x - halfSize &&
           playerPos.x <= collectible.pos.x + halfSize &&
           playerPos.y >= collectible.pos.y - halfSize &&
           playerPos.y <= collectible.pos.y + halfSize;
}