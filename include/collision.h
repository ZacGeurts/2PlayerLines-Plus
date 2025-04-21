#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"

bool checkPixelCollision(const Vec2& pos);
bool checkAreaCollision(const Vec2& center, int size);
bool checkCollectibleCollision(const Vec2& playerPos, const Collectible& collectible);

#endif // COLLISION_H