#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "types.h"
#include <random>
#include <vector>

class ExplosionManager {
public:    
    ExplosionManager(const GameConfig& config);
    void updateExplosions(std::vector<Explosion>& explosions, float dt, float currentTimeSec, SDL_Color color);
    Explosion createExplosion(const Vec2& pos, std::mt19937& rng, float dt, float startTime, SDL_Color color);
    Flash createFlash(const Vec2& pos, std::mt19937& rng, float dt, float startTime, const SDL_Color& color); // Changed to const reference
    void updateFlashes(std::vector<Flash>& flashes, float dt, float currentTimeSec, SDL_Color color);
    void cleanupPlayerFlashes(Player& player1, Player& player2, float currentTimeSec) const;

private:
    const GameConfig& config;
};

#endif // EXPLOSION_H