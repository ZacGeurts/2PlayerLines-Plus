#include "explosion.h"
#include "audio.h" // For AudioManager
#include <cmath>
#include <algorithm>

ExplosionManager::ExplosionManager(const GameConfig& config) : config(config) {}

Explosion ExplosionManager::createExplosion(const Vec2& pos, std::mt19937& rng, float startTime) const {
    Explosion explosion;
    explosion.startTime = startTime;
    explosion.soundPlayed = false;
    explosion.soundStartTime = 0.0f;
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

Flash ExplosionManager::createFlash(const Vec2& pos, std::mt19937& rng, float startTime, const SDL_Color& color) const {
    Flash flash;
    flash.startTime = startTime;
    flash.soundPlayed = false;
    flash.soundStartTime = 0.0f;
    flash.color = color;
    flash.maxRadius = config.EXPLOSION_MAX_RADIUS; // Set from config
    flash.duration = config.LASER_ZAP_DURATION; // Use LASER_ZAP_DURATION for flash
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

void ExplosionManager::updateExplosions(std::vector<Explosion>& explosions, float dt, float currentTimeSec, AudioManager& audio) const {
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
                // Handle audio (placeholder, replace with actual audio call if available)
                if (!explosion.soundPlayed && t < config.EXPLOSION_DURATION) {
                    // audio.playExplosionSound(); // Uncomment if AudioManager has this method
                    const_cast<Explosion&>(explosion).soundPlayed = true;
                    const_cast<Explosion&>(explosion).soundStartTime = currentTimeSec;
                }
                return false;
            }),
        explosions.end());
}

void ExplosionManager::updateFlashes(std::vector<Flash>& flashes, float currentTimeSec) const {
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
            delete player->endFlash;
            player->endFlash = nullptr;
        }
    }
}