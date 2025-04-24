// src/circle.cpp
#include "circle.h"
#include <cmath>
#include <algorithm> // Added for std::remove_if

CircleManager::CircleManager(const GameConfig& config) : config(config) {}

void CircleManager::spawnInitialCircle(std::mt19937& rng, std::vector<Circle>& circles) const {
    std::uniform_real_distribution<float> distX(50, config.WIDTH - 50);
    std::uniform_real_distribution<float> distY(50, config.HEIGHT - 50);
    float angle = std::uniform_real_distribution<float>(0, 2 * M_PI)(rng);
    Circle initialCircle;
    initialCircle.pos = Vec2(distX(rng), distY(rng));
    initialCircle.vel = Vec2(config.CIRCLE_SPEED * cos(angle), config.CIRCLE_SPEED * sin(angle));
    initialCircle.radius = config.CIRCLE_RADIUS;
    initialCircle.color = {255, 0, 255, 255};
    initialCircle.magentaTimer = 3.0f;
    initialCircle.isYellow = true;
    circles.push_back(initialCircle);
}

void CircleManager::updateCircles(float dt, std::vector<Circle>& circles, std::mt19937& rng, float currentTimeSec, std::chrono::steady_clock::time_point& lastCircleSpawn) const {
    for (auto& circle : circles) {
        circle.pos = circle.pos + circle.vel * dt;
        if (circle.pos.x - circle.radius < 0 || circle.pos.x + circle.radius > config.WIDTH) {
            circle.vel.x = -circle.vel.x;
            circle.pos.x = std::max(circle.radius, std::min(config.WIDTH - circle.radius, circle.pos.x));
        }
        if (circle.pos.y - circle.radius < 0 || circle.pos.y + circle.radius > config.HEIGHT) {
            circle.vel.y = -circle.vel.y;
            circle.pos.y = std::max(circle.radius, std::min(config.HEIGHT - circle.radius, circle.pos.y));
        }

        if (circle.isYellow && circle.magentaTimer > 0) {
            circle.magentaTimer -= dt;
            if (circle.magentaTimer <= 0) {
                circle.color = {255, 255, 0, 255};
            }
        }
    }

    if (std::chrono::duration<float>(std::chrono::steady_clock::now() - lastCircleSpawn).count() > 5.0f) {
        std::uniform_real_distribution<float> distX(50, config.WIDTH - 50);
        std::uniform_real_distribution<float> distY(50, config.HEIGHT - 50);
        float angle = std::uniform_real_distribution<float>(0, 2 * M_PI)(rng);
        Circle newCircle;
        newCircle.pos = Vec2(distX(rng), distY(rng));
        newCircle.vel = Vec2(config.CIRCLE_SPEED * cos(angle), config.CIRCLE_SPEED * sin(angle));
        newCircle.radius = config.CIRCLE_RADIUS;
        newCircle.color = {255, 0, 255, 255};
        newCircle.magentaTimer = 3.0f;
        newCircle.isYellow = true;
        circles.push_back(newCircle);
        lastCircleSpawn = std::chrono::steady_clock::now();
    }
}

void CircleManager::clearTrails(const std::vector<Circle>& circles, Player& player1, Player& player2) const {
    for (const auto& circle : circles) {
        for (auto& player : {&player1, &player2}) {
            player->trail.erase(
                std::remove_if(player->trail.begin(), player->trail.end(),
                    [&](const Vec2& p) {
                        float dx = circle.pos.x - p.x, dy = circle.pos.y - p.y;
                        return sqrt(dx * dx + dy * dy) < circle.radius;
                    }),
                player->trail.end());
        }
    }
}