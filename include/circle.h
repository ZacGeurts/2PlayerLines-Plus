#ifndef CIRCLE_H
#define CIRCLE_H

#include "types.h"
#include <vector>
#include <random>
#include <chrono>

namespace Game {
class Game; // Forward declaration
}

class CircleManager {
public:
    CircleManager(const Game::GameConfig& config);
    void spawnInitialCircle(std::mt19937& rng, std::vector<Game::Circle>& circles, const Game::Game& game);
    void updateCircles(float dt, std::vector<Game::Circle>& circles, std::mt19937& rng, float currentTimeSec,
                      std::chrono::steady_clock::time_point& lastCircleSpawn, Game::Game& game);
    void clearTrails(const std::vector<Game::Circle>& circles, Game::Player& player1, Game::Player& player2);

private:
    const Game::GameConfig& config;
};

#endif // CIRCLE_H