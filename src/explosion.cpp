// src/explosion.cpp
#include "explosion.h"
#include <cmath>
#include <algorithm> // Added for std::remove_if

ExplosionManager::ExplosionManager(const GameConfig& config) : config(config) {}

Explosion ExplosionManager::createExplosion(const Vec2& pos, std::mt19937& rng, float startTime) const {
    Explosion explosion;
    explosion.startTime = startTime;
    explosion.soundPlayed = false;
    explosion.soundStartTime = 0.0f;
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

void ExplosionManager::updateFlashes(std::vector<Flash>& flashes, float currentTimeSec) const {
    flashes.erase(
        std::remove_if(flashes.begin(), flashes.end(),
            [&](const Flash& flash) {
                return currentTimeSec - flash.startTime >= 0.3f;
            }),
        flashes.end());
}

void ExplosionManager::cleanupPlayerFlashes(Player& player1, Player& player2, float currentTimeSec) const {
    for (auto& player : {&player1, &player2}) {
        if (player->endFlash && currentTimeSec - player->endFlash->startTime > 0.3f) {
            delete player->endFlash;
            player->endFlash = nullptr;
        }
    }
}