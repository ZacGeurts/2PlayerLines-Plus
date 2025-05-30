#include "explosion.h"
#include <cmath>
#include <algorithm>

ExplosionManager::ExplosionManager(const Game::GameConfig& config) : config(config) {}

Game::Explosion ExplosionManager::createExplosion(const Game::Vec2& pos, std::mt19937& rng, float dt, float startTime, SDL_Color color) {
    Game::Explosion explosion;
    explosion.startTime = startTime;
    std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<float> speedDist(10, 50);
    std::uniform_real_distribution<float> timeDist(0, 1);
    for (int i = 0; i < 200; ++i) {
        float angle = angleDist(rng);
        float speed = speedDist(rng);
        Game::ExplosionParticle particle;
        particle.pos = pos;
        particle.vel = Game::Vec2(cos(angle) * speed, sin(angle) * speed);
        particle.time = timeDist(rng);
        explosion.particles.push_back(particle);
    }
    return explosion;
}

void ExplosionManager::updateExplosions(std::vector<Game::Explosion>& explosions, float dt, float currentTimeSec, SDL_Color color) {
    explosions.erase(
        std::remove_if(explosions.begin(), explosions.end(),
            [&](const Game::Explosion& explosion) {
                float t = currentTimeSec - explosion.startTime;
                if (t >= config.EXPLOSION_DURATION) {
                    return true;
                }
                for (auto& particle : const_cast<Game::Explosion&>(explosion).particles) {
                    particle.pos += particle.vel * dt;
                    particle.time += dt;
                }
                return false;
            }),
        explosions.end());
}

Game::Flash ExplosionManager::createFlash(const Game::Vec2& pos, std::mt19937& rng, float dt, float startTime, const SDL_Color& color) {
    Game::Flash flash;
    flash.color = color; // Changed from SDLflashcolor
    flash.startTime = startTime;
    flash.maxRadius = config.EXPLOSION_MAX_RADIUS;
    flash.duration = config.LASER_ZAP_DURATION;
    std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<float> speedDist(10, 50);
    std::uniform_real_distribution<float> timeDist(0, 1);
    for (int i = 0; i < 20; ++i) {
        float angle = angleDist(rng);
        float speed = speedDist(rng);
        Game::ExplosionParticle particle;
        particle.pos = pos;
        particle.vel = Game::Vec2(cos(angle) * speed, sin(angle) * speed);
        particle.time = timeDist(rng);
        flash.particles.push_back(particle);
    }
    return flash;
}

void ExplosionManager::updateFlashes(std::vector<Game::Flash>& flashes, float dt, float currentTimeSec, SDL_Color color) {
    flashes.erase(
        std::remove_if(flashes.begin(), flashes.end(),
            [&](const Game::Flash& flash) {
                return currentTimeSec - flash.startTime >= flash.duration;
            }),
        flashes.end());
    for (auto& flash : flashes) {
        for (auto& particle : flash.particles) {
            particle.pos += particle.vel * dt;
            particle.time += dt;
        }
    }
}

void ExplosionManager::cleanupPlayerFlashes(Game::Player& player1, Game::Player& player2, float currentTimeSec) const {
    for (auto& player : {&player1, &player2}) {
        if (player->endFlash && currentTimeSec - player->endFlash->startTime > 0.3f) {
            player->endFlash.reset();
        }
    }
}