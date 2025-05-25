#include "circle.h"
#include "game.h"
#include <algorithm>
#include <random>
#include <chrono>

CircleManager::CircleManager(const GameConfig& config) : config(config) {}

void CircleManager::spawnInitialCircle(std::mt19937& rng, std::vector<Circle>& circles, const Game& game) {
    std::uniform_real_distribution<float> distX(100.0f, game.orthoWidth - 100.0f);
    std::uniform_real_distribution<float> distY(100.0f, game.orthoHeight - 100.0f);
    std::uniform_real_distribution<float> distVel(-config.CIRCLE_SPEED, config.CIRCLE_SPEED);

    Circle circle;
    circle.pos = Vec2(distX(rng), distY(rng));
    circle.prevPos = circle.pos;
    circle.vel = Vec2(distVel(rng), distVel(rng));
    circle.radius = config.CIRCLE_RADIUS;
    circle.SDLcirclecolor = {255, 0, 255, 255}; // Magenta
    circle.magentaTimer = 0.0f;
    circle.isYellow = false;
    circles.push_back(circle);
}

void CircleManager::updateCircles(float dt, std::vector<Circle>& circles, std::mt19937& rng, float currentTimeSec,
                                 std::chrono::steady_clock::time_point& lastCircleSpawn, Game& game) {
    for (auto& circle : circles) {
        circle.prevPos = circle.pos;
        circle.pos += circle.vel * dt;

        // Bounce off walls
        if (circle.pos.x - circle.radius < 10.0f || circle.pos.x + circle.radius > game.orthoWidth - 10.0f) {
            circle.vel.x = -circle.vel.x;
            circle.pos.x = std::clamp(circle.pos.x, 10.0f + circle.radius, game.orthoWidth - 10.0f - circle.radius);
        }
        if (circle.pos.y - circle.radius < 10.0f || circle.pos.y + circle.radius > game.orthoHeight - 10.0f) {
            circle.vel.y = -circle.vel.y;
            circle.pos.y = std::clamp(circle.pos.y, 10.0f + circle.radius, game.orthoHeight - 10.0f - circle.radius);
        }

        // Update color timer
        circle.magentaTimer += dt;
        if (circle.magentaTimer >= 3.0f && !circle.isYellow) {
            circle.SDLcirclecolor = {255, 255, 0, 255}; // Yellow
            circle.isYellow = true; // Permanent yellow
        }
    }

    // Spawn new circles
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<float>(now - lastCircleSpawn).count() > config.CIRCLE_SPAWN_INTERVAL) {
        spawnInitialCircle(rng, circles, game);
        lastCircleSpawn = now;
        if (game.config.ENABLE_DEBUG) {
            SDL_Log("New circle spawned at time=%f, total circles=%zu", currentTimeSec, circles.size());
        }
    }
}

void CircleManager::clearTrails(const std::vector<Circle>& circles, Player& player1, Player& player2) {
    auto clearTrail = [this](std::vector<Vec2>& trail, const Circle& circle) {
        std::vector<Vec2> newTrail;
        for (size_t i = 0; i < trail.size(); ++i) {
            bool currentInside = (trail[i] - circle.pos).magnitude() < circle.radius;
            if (!currentInside) {
                newTrail.push_back(trail[i]);
            }
            // If transitioning from outside to inside or vice versa, add boundary points
            if (i < trail.size() - 1) {
                bool nextInside = (trail[i + 1] - circle.pos).magnitude() < circle.radius;
                if (currentInside != nextInside) {
                    // Interpolate boundary point
                    Vec2 p1 = trail[i];
                    Vec2 p2 = trail[i + 1];
                    float t = 0.5f; // Approximate intersection
                    Vec2 boundary = p1 + (p2 - p1) * t;
                    if (!nextInside) newTrail.push_back(boundary);
                }
            }
        }
        trail = newTrail;
    };

    for (const auto& circle : circles) {
        clearTrail(player1.trail, circle);
        clearTrail(player2.trail, circle);
    }
}