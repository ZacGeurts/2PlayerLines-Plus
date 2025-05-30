#ifndef EXPLOSION_H
#define EXPLOSION_H

#include "types.h"
#include <random>
#include <vector>
#include <SDL2/SDL.h>

class ExplosionManager {
public:    
    ExplosionManager(const Game::GameConfig& config);
    void updateExplosions(std::vector<Game::Explosion>& explosions, float dt, float currentTimeSec, SDL_Color color);
    Game::Explosion createExplosion(const Game::Vec2& pos, std::mt19937& rng, float dt, float startTime, SDL_Color color);
    Game::Flash createFlash(const Game::Vec2& pos, std::mt19937& rng, float dt, float startTime, const SDL_Color& color);
    void updateFlashes(std::vector<Game::Flash>& flashes, float dt, float currentTimeSec, SDL_Color color);
    void cleanupPlayerFlashes(Game::Player& player1, Game::Player& player2, float currentTimeSec) const;

private:
    const Game::GameConfig& config;
};

#endif // EXPLOSION_H