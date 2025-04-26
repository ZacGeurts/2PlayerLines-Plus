#include "ai.h"
#include "game.h"
#include <cmath>
#include <algorithm>

AI::AI(const GameConfig& config) : config(config), difficulty(Difficulty::OFF), reactionTime(0.0f),
                                   predictionDistance(100.0f), avoidanceRadius(50.0f) {}

void AI::update(Player& aiPlayer, const Player& opponent, const Collectible& collectible,
                const std::vector<Circle>& circles, float dt, std::mt19937& rng) {
    if (difficulty == Difficulty::OFF || !aiPlayer.alive) return;

    // Update difficulty-specific parameters
    switch (difficulty) {
        case Difficulty::EASY:
            reactionTime = 0.5f;
            predictionDistance = 50.0f;
            avoidanceRadius = 30.0f;
            break;
        case Difficulty::MEDIUM:
            reactionTime = 0.3f;
            predictionDistance = 75.0f;
            avoidanceRadius = 40.0f;
            break;
        case Difficulty::HARD:
            reactionTime = 0.15f;
            predictionDistance = 100.0f;
            avoidanceRadius = 50.0f;
            break;
        case Difficulty::EXPERT:
            reactionTime = 0.05f;
            predictionDistance = 150.0f;
            avoidanceRadius = 60.0f;
            break;
        default:
            return;
    }

    // Simulate reaction time
    static float timeSinceLastDecision = 0.0f;
    timeSinceLastDecision += dt;
    if (timeSinceLastDecision < reactionTime) return;
    timeSinceLastDecision = 0.0f;

    // Calculate desired direction
    Vec2 targetDir = calculateTargetDirection(aiPlayer, opponent, collectible, circles, rng);
    
    // Smoothly adjust AI player's direction
    float turnSpeed = config.AI_TURN_SPEED ? config.AI_TURN_SPEED : 180.0f; // Degrees per second
    float maxTurn = turnSpeed * dt * (3.14159f / 180.0f); // Convert to radians
    Vec2 currentDir = aiPlayer.direction;
    Vec2 newDir = currentDir + (targetDir - currentDir) * std::min(1.0f, maxTurn);
    if (newDir.magnitude() > 0) {
        aiPlayer.direction = newDir.normalized();
    }

    // Update position
    float speed = config.PLAYER_SPEED ? config.PLAYER_SPEED : 100.0f;
    Vec2 nextPos = aiPlayer.pos + aiPlayer.direction * speed * dt;

    // Keep AI within bounds
    if (nextPos.x < 10) nextPos.x = 10;
    if (nextPos.x > config.WIDTH - 10) nextPos.x = config.WIDTH - 10;
    if (nextPos.y < 10) nextPos.y = 10;
    if (nextPos.y > config.HEIGHT - 10) nextPos.y = config.HEIGHT - 10;

    aiPlayer.pos = nextPos;
    aiPlayer.hasMoved = true;
}

void AI::setDifficulty(Difficulty diff) {
    difficulty = diff;
}

AI::Difficulty AI::getDifficulty() const {
    return difficulty;
}

Vec2 AI::calculateTargetDirection(const Player& aiPlayer, const Player& opponent,
                                 const Collectible& collectible, const std::vector<Circle>& circles,
                                 std::mt19937& rng) const {
    // Primary goal: move toward collectible
    Vec2 toCollectible = (collectible.pos - aiPlayer.pos).normalized();
    Vec2 targetDir = toCollectible;

    // Check if path to collectible is safe
    if (!isPathSafe(aiPlayer.pos, toCollectible, predictionDistance, circles, opponent)) {
        // Try to avoid obstacles by testing alternative directions
        std::vector<Vec2> directions;
        for (int i = 0; i < 8; ++i) {
            float angle = i * (2 * 3.14159f / 8);
            directions.emplace_back(std::cos(angle), std::sin(angle));
        }
        std::shuffle(directions.begin(), directions.end(), rng);

        for (const auto& dir : directions) {
            if (isPathSafe(aiPlayer.pos, dir, predictionDistance, circles, opponent)) {
                targetDir = dir;
                break;
            }
        }
    }

    return targetDir;
}

bool AI::isPathSafe(const Vec2& start, const Vec2& direction, float distance,
                    const std::vector<Circle>& circles, const Player& opponent) const {
    Vec2 end = start + direction * distance;

    // Check for collision with circles
    for (const auto& circle : circles) {
        float distToCircle = (circle.pos - start).magnitude();
        if (distToCircle < circle.radius + avoidanceRadius) {
            return false;
        }
    }

    // Check for collision with opponent's trail (simplified)
    if (opponent.alive) {
        float distToOpponent = (opponent.pos - start).magnitude();
        if (distToOpponent < avoidanceRadius) {
            return false;
        }
    }

    // Check screen boundaries
    if (end.x < 10 || end.x > config.WIDTH - 10 || end.y < 10 || end.y > config.HEIGHT - 10) {
        return false;
    }

    return true;
}