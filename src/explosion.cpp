#include "explosion.h" // linesplus

#include <cmath>
#include <algorithm>

ExplosionManager::ExplosionManager(const GameConfig& config) : config(config) {}

Explosion ExplosionManager::createExplosion(const Vec2& pos, std::mt19937& rng, float dt, float startTime, SDL_Color color) {
    Explosion explosion;
    explosion.startTime = startTime;
    std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<float> speedDist(10, 50);
    std::uniform_real_distribution<float> timeDist(0, 1);
    for (int i = 0; i < 200; ++i) {
        float angle = angleDist(rng);
        float speed = speedDist(rng);
        ExplosionParticle particle;
        particle.pos = pos;
        particle.vel = Vec2(cos(angle) * speed, sin(angle) * speed);
        particle.time = timeDist(rng);
        explosion.particles.push_back(particle);
    }
    return explosion;
}

void ExplosionManager::updateExplosions(std::vector<Explosion>& explosions, float dt, float currentTimeSec, SDL_Color color) {
    explosions.erase(
        std::remove_if(explosions.begin(), explosions.end(),
            [&](const Explosion& explosion) {
                float t = currentTimeSec - explosion.startTime;
                if (t >= config.EXPLOSION_DURATION) {
                    return true; // Remove expired explosions
                }
                // Update particle positions
                for (auto& particle : const_cast<Explosion&>(explosion).particles) {
                    particle.pos += particle.vel * dt;
                    particle.time += dt;
                }
                return false;
            }),
        explosions.end());
}

Flash ExplosionManager::createFlash(const Vec2& pos, std::mt19937& rng, float dt, float startTime, const SDL_Color& color) {
    Flash flash;
    flash.SDLflashcolor = color;
    flash.startTime = startTime;
    flash.maxRadius = config.EXPLOSION_MAX_RADIUS;
    flash.duration = config.LASER_ZAP_DURATION;
    std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<float> speedDist(10, 50);
    std::uniform_real_distribution<float> timeDist(0, 1);
    for (int i = 0; i < 20; ++i) {
        float angle = angleDist(rng);
        float speed = speedDist(rng);
        ExplosionParticle particle;
        particle.pos = pos;
        particle.vel = Vec2(cos(angle) * speed, sin(angle) * speed);
        particle.time = timeDist(rng);
        flash.particles.push_back(particle);
    }
    return flash;
}

void ExplosionManager::updateFlashes(std::vector<Flash>& flashes, float dt, float currentTimeSec, SDL_Color SDLflashcolor) {
    flashes.erase(
        std::remove_if(flashes.begin(), flashes.end(),
            [&](const Flash& flash) {
                return currentTimeSec - flash.startTime >= flash.duration; // Use flash.duration
            }),
        flashes.end());
    // Update particle positions for remaining flashes
    for (auto& flash : flashes) {
        for (auto& particle : flash.particles) {
            particle.pos += particle.vel * (currentTimeSec - particle.time);
            particle.time = currentTimeSec;
        }
    }
}

void ExplosionManager::cleanupPlayerFlashes(Player& player1, Player& player2, float currentTimeSec) const {
    for (auto& player : {&player1, &player2}) {
        if (player->endFlash && currentTimeSec - player->endFlash->startTime > 0.3f) {
            player->endFlash.reset();
        }
    }
}