// include/collision.h
#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"
#include "constants.h"

class CollisionManager {
public:
    CollisionManager(const GameConfig& config);
    bool checkPixelCollision(const Vec2& pos) const;
    bool checkAreaCollision(const Vec2& center, int size) const;

private:
    const GameConfig& config;
};

#endif // COLLISION_H