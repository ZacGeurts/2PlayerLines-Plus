#include "explosion.h"
#include "constants.h"
#include <cmath>

Explosion createExplosion(const Vec2& pos, std::mt19937& rng, float startTime) {
    std::vector<ExplosionParticle> particles;
    std::uniform_real_distribution<float> angleDist(0, 2 * M_PI);
    std::uniform_real_distribution<float> speedDist(0.5f, 1.5f);
    for (int i = 0; i < EXPLOSION_PARTICLES; ++i) {
        float angle = angleDist(rng);
        float speed = speedDist(rng);
        Vec2 vel(cos(angle) * speed, sin(angle) * speed);
        particles.push_back({pos, vel, 0.0f});
    }
    return {pos, particles, startTime, true, false};
}
