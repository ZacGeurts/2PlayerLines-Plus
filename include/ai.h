#ifndef AI_H
#define AI_H

#include "player.h"
#include "collectible.h"
#include "circle.h"
#include <vector>
#include <random>

class GameConfig;

class AI {
public:
    enum class Difficulty { OFF, EASY, MEDIUM, HARD, EXPERT };

    AI(const GameConfig& config);
    void update(Player& aiPlayer, const Player& opponent, const Collectible& collectible,
                const std::vector<Circle>& circles, float dt, std::mt19937& rng);
    void setDifficulty(Difficulty diff);
    Difficulty getDifficulty() const;

private:
    const GameConfig& config;
    Difficulty difficulty;
    float reactionTime;
    float predictionDistance;
    float avoidanceRadius;

    Vec2 calculateTargetDirection(const Player& aiPlayer, const Player& opponent,
                                  const Collectible& collectible, const std::vector<Circle>& circles,
                                  std::mt19937& rng) const;
    bool isPathSafe(const Vec2& start, const Vec2& direction, float distance,
                    const std::vector<Circle>& circles, const Player& opponent) const;
};

#endif // AI_H