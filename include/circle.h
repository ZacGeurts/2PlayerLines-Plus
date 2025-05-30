#ifndef CIRCLE_H
#define CIRCLE_H

#include "types.h"
#include <vector>
#include <random>
#include <chrono>

class Game;

class CircleManager {
public:
    CircleManager(const GameConfig& config);
    void spawnInitialCircle(std::mt19937& rng, std::vector<Circle>& circles, const Game& game);
    void updateCircles(float dt, std::vector<Circle>& circles, std::mt19937& rng, float currentTimeSec,
                      std::chrono::steady_clock::time_point& lastCircleSpawn, Game& game);
    void clearTrails(const std::vector<Circle>& circles, Player& player1, Player& player2);

private:
    const GameConfig& config;
};

#endif // CIRCLE_H