// include/circle.h
#ifndef CIRCLE_H
#define CIRCLE_H

#include "types.h"
#include "constants.h"
#include <random>
#include <vector>
#include <chrono> // Added for std::chrono

class CircleManager {
public:
    CircleManager(const GameConfig& config);
    void spawnInitialCircle(std::mt19937& rng, std::vector<Circle>& circles) const;
    void updateCircles(float dt, std::vector<Circle>& circles, std::mt19937& rng, float currentTimeSec, std::chrono::steady_clock::time_point& lastCircleSpawn) const;
    void clearTrails(const std::vector<Circle>& circles, Player& player1, Player& player2) const;

private:
    const GameConfig& config;
};

#endif // CIRCLE_H