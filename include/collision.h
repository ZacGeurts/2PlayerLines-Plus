#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"

// Forward declaration
struct Game;

class CollisionManager {
public:
    CollisionManager(const GameConfig& config);
    bool checkPixelCollision(const Vec2& pos, const Game& game) const;
    bool checkAreaCollision(const Vec2& center, int size, const Game& game) const;

private:
    const GameConfig& config;
};

#endif // COLLISION_H