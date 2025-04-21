#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "types.h"
#include <random>

Explosion createExplosion(const Vec2& pos, std::mt19937& rng, float startTime);

#endif // EXPLOSION_H
