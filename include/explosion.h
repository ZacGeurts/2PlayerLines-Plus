#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "types.h"
#include <random>
#include <vector>

class AudioManager; // Forward declaration for audio

class ExplosionManager {
public:
    ExplosionManager(const GameConfig& config);
    Explosion createExplosion(const Vec2& pos, std::mt19937& rng, float startTime) const;
    Flash createFlash(const Vec2& pos, std::mt19937& rng, float startTime, const SDL_Color& color = {255, 0, 255, 255}) const;
    void updateExplosions(std::vector<Explosion>& explosions, float dt, float currentTimeSec, AudioManager& audio) const; // Added
    void updateFlashes(std::vector<Flash>& flashes, float currentTimeSec) const;
    void cleanupPlayerFlashes(Player& player1, Player& player2, float currentTimeSec) const;

private:
    const GameConfig& config;
};

#endif // EXPLOSION_H