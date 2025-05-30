#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"

namespace Game { class Game; } // Forward declaration

class CollisionManager {
public:
    CollisionManager(const Game::GameConfig& config);
    bool checkPixelCollision(const Game::Vec2& pos, const Game::Game& game) const;
    bool checkAreaCollision(const Game::Vec2& center, int size, const Game::Game& game) const;

private:
    const Game::GameConfig& config;
};

#endif // COLLISION_H