#include "circle.h"
#include "game.h"
#include <algorithm>
#include <random>
#include <chrono>

CircleManager::CircleManager(const Game::GameConfig& config)
    : config(config) {
}

void CircleManager::spawnInitialCircle(std::mt19937& rng, std::vector<Game::Circle>& circles, const Game::Game& game) {
    std::uniform_real_distribution<float> distX(100.0f, game.orthoWidth - 100.0f);
    std::uniform_real_distribution<float> distY(100.0f, game.orthoHeight - 100.0f);
    std::uniform_real_distribution<float> distVel(-config.CIRCLE_SPEED, config.CIRCLE_SPEED);

    Game::Circle circle;
    circle.pos = Game::Vec2(distX(rng), distY(rng));
    circle.prevPos = circle.pos;
    circle.vel = Game::Vec2(distVel(rng), distVel(rng));
    circle.radius = config.CIRCLE_RADIUS;
    circle.color = {255, 0, 255, 255}; // Magenta
    circle.magentaTimer = 0.0f;
    circle.isYellow = false;
    circles.push_back(circle);
}

void CircleManager::updateCircles(float dt, std::vector<Game::Circle>& circles, std::mt19937& rng, float currentTimeSec,
                                 std::chrono::steady_clock::time_point& lastCircleSpawn, Game::Game& game) {
    for (auto& circle : circles) {
        circle.prevPos = circle.pos;
        circle.pos += circle.vel * dt;

        if (circle.pos.x - circle.radius < 10.0f || circle.pos.x + circle.radius > game.orthoWidth - 10.0f) {
            circle.vel.x = -circle.vel.x;
            circle.pos.x = std::clamp(circle.pos.x, 10.0f + circle.radius, game.orthoWidth - 10.0f - circle.radius);
        }
        if (circle.pos.y - circle.radius < 10.0f || circle.pos.y + circle.radius > game.orthoHeight - 10.0f) {
            circle.vel.y = -circle.vel.y;
            circle.pos.y = std::clamp(circle.pos.y, 10.0f + circle.radius, game.orthoHeight - 10.0f - circle.radius);
        }

        circle.magentaTimer += dt;
        if (circle.magentaTimer >= 3.0f && !circle.isYellow) {
            circle.color = {255, 255, 0, 255}; // Yellow
            circle.isYellow = true;
        }
    }

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<float>(now - lastCircleSpawn).count() > config.CIRCLE_SPAWN_INTERVAL) {
        spawnInitialCircle(rng, circles, game);
        lastCircleSpawn = now;
        if (game.config.ENABLE_DEBUG) {
            SDL_Log("New circle spawned at time=%f, total circles=%zu", currentTimeSec, circles.size());
        }
    }
}

void CircleManager::clearTrails(const std::vector<Game::Circle>& circles, Game::Player& player1, Game::Player& player2) {
    auto clearTrail = [this](std::vector<Game::Vec2>& trail, const Game::Circle& circle) {
        std::vector<Game::Vec2> newTrail;
        for (size_t i = 0; i < trail.size(); ++i) {
            bool currentInside = (trail[i] - circle.pos).magnitude() < circle.radius;
            if (!currentInside) {
                newTrail.push_back(trail[i]);
            }
            if (i < trail.size() - 1) {
                bool nextInside = (trail[i + 1] - circle.pos).magnitude() < circle.radius;
                if (currentInside != nextInside) {
                    Game::Vec2 p1 = trail[i];
                    Game::Vec2 p2 = trail[i + 1];
                    float t = 0.5f;
                    Game::Vec2 boundary = p1 + (p2 - p1) * t;
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