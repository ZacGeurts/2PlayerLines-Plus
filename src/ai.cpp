#include "ai.h"
#include "game.h" // Add to resolve incomplete Game type
#include <cmath>
#include <algorithm>
#include <vector>
#include <chrono>
#include <SDL2/SDL.h>
#include <queue>
#include <unordered_set>

AI::AI(const GameConfig& config, Game& game)
    : config(config),
      game(&game),
      framebuffer(),
      drawableWidth(0),
      drawableHeight(0),
      flashUsed(false),
      modeEnabled(true),
      leftTrigger(0.0f),
      rightTrigger(0.0f),
      aButton(false),
      updateReady(false),
      newDirection(0, 0),
      currentTimeSec(0.0f),
      frameCount(0) {}

AI::~AI() {
    if (updateThread.joinable()) {
        updateThread.join();
    }
}

void AI::startUpdate(Player& aiPlayer, const Player& opponent, const Collectible& collectible,
                     const std::vector<Circle>& circles, float dt, std::mt19937& rng, Game& game,
                     const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight, SDL_Color) {
    if (!modeEnabled || !aiPlayer.alive || aiPlayer.willDie) return;

    if (updateThread.joinable()) {
        updateThread.join();
    }
    this->framebuffer = framebuffer;
    this->drawableWidth = drawableWidth;
    this->drawableHeight = drawableHeight;
    updateThread = std::thread(&AI::simulateControllerInput, this, std::ref(aiPlayer), std::ref(collectible),
                              std::ref(circles), std::ref(opponent), dt, std::ref(rng), std::ref(game),
                              std::ref(framebuffer), drawableWidth, drawableHeight,
                              std::ref(leftTrigger), std::ref(rightTrigger), std::ref(aButton));
}

void AI::waitForUpdate() {
    if (updateThread.joinable()) {
        updateThread.join();
    }
}

void AI::applyUpdate(Player& aiPlayer) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!updateReady) return;

    Vec2 oldPos = aiPlayer.pos;
    Vec2 newDir = aiPlayer.direction;
    float rotation = (rightTrigger - leftTrigger) * config.AI_TURN_SPEED * 0.008333f;
    if (std::abs(rotation) > 0) {
        float cosTheta = std::cos(rotation);
        float sinTheta = std::sin(rotation);
        newDir = Vec2(
            aiPlayer.direction.x * cosTheta - aiPlayer.direction.y * sinTheta,
            aiPlayer.direction.x * sinTheta + aiPlayer.direction.y * cosTheta
        ).normalized();
    }

    float speed = config.PLAYER_SPEED; // Assumed 200.0
    Vec2 nextPos = aiPlayer.pos + newDir * speed * 0.008333f;
    nextPos.x = std::max(config.AI_BERTH, std::min(nextPos.x, game->orthoWidth - config.AI_BERTH));
    nextPos.y = std::max(config.AI_BERTH, std::min(nextPos.y, game->orthoHeight - config.AI_BERTH));

    if (aButton && !flashUsed && aiPlayer.canUseNoCollision && !aiPlayer.isInvincible) {
        game->activateNoCollision(&aiPlayer, currentTimeSec);
        flashUsed = true;
    }

    bool willDie = false;
    bool hitOpponentHead = false;
    if (aiPlayer.hasMoved && !aiPlayer.isInvincible && !aiPlayer.spawnInvincibilityTimer) {
        aiPlayer.pos = nextPos;
        game->checkCollision(&aiPlayer, nextPos, currentTimeSec, framebuffer, drawableWidth, drawableHeight);
        willDie = aiPlayer.willDie;
        hitOpponentHead = aiPlayer.hitOpponentHead;
    } else {
        aiPlayer.pos = nextPos;
        aiPlayer.hasMoved = true;
    }

    aiPlayer.direction = newDir;
    aiPlayer.trail.push_back(aiPlayer.pos);
    if (willDie) {
        aiPlayer.willDie = true;
        aiPlayer.deathPos = nextPos;
        aiPlayer.hitOpponentHead = hitOpponentHead;
    }

    if (config.ENABLE_DEBUG) {
        float effectiveSpeed = ((aiPlayer.pos - oldPos).magnitude() / 0.008333f);
        SDL_Log("AI apply: pos=(%f, %f), dir=(%f, %f), speed=%f, expected=%f, leftTrigger=%f, rightTrigger=%f, aButton=%d",
                aiPlayer.pos.x, aiPlayer.pos.y, newDir.x, newDir.y, effectiveSpeed, speed, leftTrigger, rightTrigger, aButton);
    }

    updateReady = false;
}

void AI::simulateControllerInput(const Player& aiPlayer, const Collectible& collectible,
                                const std::vector<Circle>& circles, const Player& opponent,
                                float dt, std::mt19937& rng, Game& game,
                                const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight,
                                float& leftTrigger, float& rightTrigger, bool& aButton) {
    leftTrigger = 0.0f;
    rightTrigger = 0.0f;
    aButton = false;
    currentTimeSec = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
    frameCount++;

    dt = std::min(dt, 0.008333f);

    size_t expectedSize = static_cast<size_t>(drawableWidth) * drawableHeight * 3;
    if (framebuffer.size() < expectedSize) {
        if (config.ENABLE_DEBUG) {
            SDL_Log("Invalid framebuffer size: got %zu, expected %zu", framebuffer.size(), expectedSize);
        }
        return;
    }

    Vec2 targetDir = calculateTargetDirection(aiPlayer, collectible, circles, opponent, rng, game, currentTimeSec,
                                              framebuffer, drawableWidth, drawableHeight);

    float angleDiff = std::acos(std::clamp(aiPlayer.direction.dot(targetDir), -1.0f, 1.0f));
    float cross = aiPlayer.direction.x * targetDir.y - aiPlayer.direction.y * targetDir.x;
    float turnSpeedRad = config.AI_TURN_SPEED * M_PI / 180.0f;
    if (angleDiff > 0.01f) {
        float triggerValue = std::min(angleDiff / (turnSpeedRad * dt), 1.0f);
        if (cross > 0) {
            leftTrigger = triggerValue;
        } else {
            rightTrigger = triggerValue;
        }
    }

    std::lock_guard<std::mutex> lock(mutex);
    updateReady = true;

    if (config.ENABLE_DEBUG) {
        SDL_Log("AI input: leftTrigger=%f, rightTrigger=%f, aButton=%d, angleDiff=%f degrees",
                leftTrigger, rightTrigger, aButton, angleDiff * 180.0 / M_PI);
    }
}

float AI::heuristic(const Vec2& a, const Vec2& b) const {
    return (a - b).magnitude();
}

bool AI::isPositionSafe(const Vec2& pos, const std::vector<Circle>& circles, const Player& opponent, Game& game) {
    if (pos.x < config.AI_BERTH || pos.x > game.orthoWidth - config.AI_BERTH ||
        pos.y < config.AI_BERTH || pos.y > game.orthoHeight - config.AI_BERTH) {
        return false;
    }

    for (const auto& circle : circles) {
        if ((pos - circle.pos).magnitude() < circle.radius + config.AI_BERTH) {
            return false;
        }
    }

    if ((pos - opponent.pos).magnitude() < config.AI_BERTH * 2.0f && opponent.alive) {
        return false;
    }

    return true;
}

std::vector<Vec2> AI::findPathAStar(const Vec2& start, const Vec2& goal, const std::vector<Circle>& circles,
                                    const Player& opponent, Game& game, const std::vector<unsigned char>& framebuffer,
                                    int drawableWidth, int drawableHeight) {
    const float GRID_SIZE = 6.0f; // Finer grid
    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openList;
    std::unordered_set<uint64_t> closedList;

    auto hashPos = [&](const Vec2& pos) {
        int x = static_cast<int>(pos.x / GRID_SIZE);
        int y = static_cast<int>(pos.y / GRID_SIZE);
        return static_cast<uint64_t>(x) << 32 | static_cast<uint64_t>(y);
    };

    PathNode startNode{start, 0.0f, heuristic(start, goal), nullptr};
    openList.push(startNode);

    std::vector<Vec2> directions = {
        {GRID_SIZE, 0}, {-GRID_SIZE, 0}, {0, GRID_SIZE}, {0, -GRID_SIZE},
        {GRID_SIZE, GRID_SIZE}, {GRID_SIZE, -GRID_SIZE}, {-GRID_SIZE, GRID_SIZE}, {-GRID_SIZE, -GRID_SIZE}
    };

    while (!openList.empty()) {
        PathNode current = openList.top();
        openList.pop();

        uint64_t currentHash = hashPos(current.pos);
        if (closedList.find(currentHash) != closedList.end()) continue;
        closedList.insert(currentHash);

        if ((current.pos - goal).magnitude() < GRID_SIZE) {
            std::vector<Vec2> path;
            auto node = std::make_shared<PathNode>(current);
            while (node) {
                path.push_back(node->pos);
                node = node->parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& dir : directions) {
            Vec2 newPos = current.pos + dir;
            if (!isPositionSafe(newPos, circles, opponent, game)) continue;

            std::string color = getPixelColor(newPos, game, currentTimeSec, framebuffer, drawableWidth, drawableHeight);
            if (color == "red" || color == "yellow" || color == "blue") continue;

            float gCost = current.gCost + dir.magnitude();
            float hCost = heuristic(newPos, goal);
            PathNode newNode{newPos, gCost, gCost + hCost, std::make_shared<PathNode>(current)};

            openList.push(newNode);
        }
    }

    return {};
}

Vec2 AI::calculateTargetDirection(const Player& aiPlayer, const Collectible& collectible,
                                 const std::vector<Circle>& circles, const Player& opponent,
                                 std::mt19937& rng, Game& game, float currentTimeSec,
                                 const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    const float AI_BERTH = config.AI_BERTH;
    const float COLLECTIBLE_HALF_SIZE = config.COLLECTIBLE_SIZE / 2.0f;
    const float VISUAL_HALF_SIZE = collectible.size / 2.0f;
    const float RAYCAST_RANGE = 700.0f;
    const float ANGLE_STEP = M_PI / 180; // 1-degree increments
    const float MAX_TURN_ANGLE = config.AI_TURN_SPEED * 0.008333f;
    const float WALL_THRESHOLD = AI_BERTH * 9.0f; // 90 units
    const float DANGER_THRESHOLD = 7.0f;
    const float OPPONENT_AVOIDANCE = 90.0f;

    if (!collectible.active) {
        return aiPlayer.direction;
    }

    // Wall proximity
    bool nearWall = false;
    Vec2 wallAvoidanceDir = Vec2(0.0f, 0.0f);
    float wallDistance = std::numeric_limits<float>::max();
    if (aiPlayer.pos.x < WALL_THRESHOLD) {
        wallAvoidanceDir += Vec2(1.0f, 0.0f);
        wallDistance = std::min(wallDistance, aiPlayer.pos.x - AI_BERTH);
        nearWall = true;
    } else if (aiPlayer.pos.x > game.orthoWidth - WALL_THRESHOLD) {
        wallAvoidanceDir += Vec2(-1.0f, 0.0f);
        wallDistance = std::min(wallDistance, game.orthoWidth - aiPlayer.pos.x - AI_BERTH);
        nearWall = true;
    }
    if (aiPlayer.pos.y < WALL_THRESHOLD) {
        wallAvoidanceDir += Vec2(0.0f, 1.0f);
        wallDistance = std::min(wallDistance, aiPlayer.pos.y - AI_BERTH);
        nearWall = true;
    } else if (aiPlayer.pos.y > game.orthoHeight - WALL_THRESHOLD) {
        wallAvoidanceDir += Vec2(0.0f, -1.0f);
        wallDistance = std::min(wallDistance, game.orthoHeight - aiPlayer.pos.y - AI_BERTH);
        nearWall = true;
    }

    // A* pathfinding
    std::vector<Vec2> path = findPathAStar(aiPlayer.pos, collectible.pos, circles, opponent, game, framebuffer, drawableWidth, drawableHeight);
    Vec2 toCollectible = (collectible.pos - aiPlayer.pos).normalized();
    Vec2 targetDir = toCollectible;

    if (!path.empty() && path.size() > 1) {
        targetDir = (path[1] - aiPlayer.pos).normalized();
    }

    bool nearCollectible = std::abs(aiPlayer.pos.x - collectible.pos.x) <= COLLECTIBLE_HALF_SIZE &&
                          std::abs(aiPlayer.pos.y - collectible.pos.y) <= COLLECTIBLE_HALF_SIZE;

    // Opponent avoidance
    Vec2 toOpponent = opponent.pos - aiPlayer.pos;
    float opponentDistance = toOpponent.magnitude();
    if (opponentDistance < OPPONENT_AVOIDANCE && opponent.alive) {
        Vec2 avoidDir = -toOpponent.normalized();
        targetDir = (targetDir + avoidDir * 1.0f).normalized();
    }

    // Flash if trapped
    if (nearWall && wallDistance < AI_BERTH * 0.9f && !flashUsed && aiPlayer.canUseNoCollision && !aiPlayer.isInvincible) {
        aButton = true;
        return targetDir;
    }

    // Wall avoidance override
    if (nearWall && wallDistance < AI_BERTH * 4.5f && !nearCollectible) {
        wallAvoidanceDir = wallAvoidanceDir.normalized();
        float angleDiff = std::acos(std::clamp(aiPlayer.direction.dot(wallAvoidanceDir), -1.0f, 1.0f));
        float cross = aiPlayer.direction.x * wallAvoidanceDir.y - aiPlayer.direction.y * wallAvoidanceDir.x;
        float turnAngle = cross > 0 ? std::min(angleDiff, MAX_TURN_ANGLE) : -std::min(angleDiff, MAX_TURN_ANGLE);
        return Vec2(
            aiPlayer.direction.x * std::cos(turnAngle) - aiPlayer.direction.y * std::sin(turnAngle),
            aiPlayer.direction.x * std::sin(turnAngle) + aiPlayer.direction.y * std::cos(turnAngle)
        ).normalized();
    }

    // Forward raycast
    RaycastResult forwardRay = raycastForward(aiPlayer, game, currentTimeSec, framebuffer, drawableWidth, drawableHeight);

    // Pursue collectible if safe
    if (nearCollectible || (!forwardRay.centerLine.hasDanger || forwardRay.centerLine.greenVisible)) {
        float angleDiff = std::acos(std::clamp(aiPlayer.direction.dot(targetDir), -1.0f, 1.0f));
        float cappedAngle = std::min(angleDiff, MAX_TURN_ANGLE);
        float cross = aiPlayer.direction.x * targetDir.y - aiPlayer.direction.y * targetDir.x;
        float turnAngle = cross > 0 ? cappedAngle : -cappedAngle;
        return Vec2(
            aiPlayer.direction.x * std::cos(turnAngle) - aiPlayer.direction.y * std::sin(turnAngle),
            aiPlayer.direction.x * std::sin(turnAngle) + aiPlayer.direction.y * std::cos(turnAngle)
        ).normalized();
    }

    // Flash for obstacles
    float minDistance = std::min({forwardRay.centerLine.distance, forwardRay.leftLine.distance, forwardRay.rightLine.distance});
    if (!flashUsed && aiPlayer.canUseNoCollision && !aiPlayer.isInvincible && minDistance < DANGER_THRESHOLD) {
        aButton = true;
        return targetDir;
    }

    // Pathfinding sweep
    std::vector<std::pair<Vec2, float>> safeDirections;
    for (float angle = -M_PI / 2; angle <= M_PI / 2; angle += ANGLE_STEP) {
        if (std::abs(angle) > MAX_TURN_ANGLE) continue;
        Vec2 testDir = Vec2(
            aiPlayer.direction.x * std::cos(angle) - aiPlayer.direction.y * std::sin(angle),
            aiPlayer.direction.x * std::sin(angle) + aiPlayer.direction.y * std::cos(angle)
        ).normalized();

        LineCheckResult line = checkLine(aiPlayer.pos + aiPlayer.direction * 10.0f, testDir, RAYCAST_RANGE,
                                        aiPlayer.direction, game, currentTimeSec, framebuffer, drawableWidth, drawableHeight);

        if (!line.hasDanger || line.greenVisible) {
            float score = testDir.dot(toCollectible) * (line.greenVisible ? 30.0f : 1.0f) * (1.0f - line.distance / RAYCAST_RANGE);
            Vec2 testPos = aiPlayer.pos + testDir * std::min(line.distance, RAYCAST_RANGE);
            if (std::abs(testPos.x - collectible.pos.x) <= VISUAL_HALF_SIZE &&
                std::abs(testPos.y - collectible.pos.y) <= VISUAL_HALF_SIZE) {
                score *= 35.0f;
            } else if (std::abs(testPos.x - collectible.pos.x) <= COLLECTIBLE_HALF_SIZE &&
                       std::abs(testPos.y - collectible.pos.y) <= COLLECTIBLE_HALF_SIZE) {
                score *= 15.0f;
            }
            if (testPos.x < AI_BERTH * 2.0f || testPos.x > game.orthoWidth - AI_BERTH * 2.0f ||
                testPos.y < AI_BERTH * 2.0f || testPos.y > game.orthoHeight - AI_BERTH * 2.0f) {
                score *= 0.1f;
            }
            if ((testPos - opponent.pos).magnitude() < OPPONENT_AVOIDANCE) {
                score *= 0.2f;
            }
            safeDirections.emplace_back(testDir, score);
        }
    }

    if (!safeDirections.empty()) {
        auto bestDir = std::max_element(safeDirections.begin(), safeDirections.end(),
                                        [](const auto& a, const auto& b) { return a.second < b.second; });
        return bestDir->first;
    }

    // Fallback: Safe direction
    if (nearWall) {
        wallAvoidanceDir = wallAvoidanceDir.normalized();
        float angleDiff = std::acos(std::clamp(aiPlayer.direction.dot(wallAvoidanceDir), -1.0f, 1.0f));
        float cross = aiPlayer.direction.x * wallAvoidanceDir.y - aiPlayer.direction.y * wallAvoidanceDir.x;
        float turnAngle = cross > 0 ? std::min(angleDiff, MAX_TURN_ANGLE) : -std::min(angleDiff, MAX_TURN_ANGLE);
        return Vec2(
            aiPlayer.direction.x * std::cos(turnAngle) - aiPlayer.direction.y * std::sin(turnAngle),
            aiPlayer.direction.x * std::sin(turnAngle) + aiPlayer.direction.y * std::cos(turnAngle)
        ).normalized();
    }

    std::uniform_real_distribution<float> dist(-MAX_TURN_ANGLE, MAX_TURN_ANGLE);
    float randomAngle = dist(rng);
    return Vec2(
        aiPlayer.direction.x * std::cos(randomAngle) - aiPlayer.direction.y * std::sin(randomAngle),
        aiPlayer.direction.x * std::sin(randomAngle) + aiPlayer.direction.y * std::cos(randomAngle)
    ).normalized();
}

AI::RaycastResult AI::raycastForward(const Player& aiPlayer, Game& game, float currentTimeSec,
                                     const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    RaycastResult result;
    const float ANGLE_OFFSET = M_PI / 18; // 10 degrees
    const float RAYCAST_RANGE = 700.0f;
    const float HEAD_OFFSET = 10.0f;

    Vec2 start = aiPlayer.pos + aiPlayer.direction * HEAD_OFFSET;

    result.centerLine = checkLine(start, aiPlayer.direction, RAYCAST_RANGE, aiPlayer.direction, game, currentTimeSec,
                                  framebuffer, drawableWidth, drawableHeight);

    float cosA = std::cos(-ANGLE_OFFSET);
    float sinA = std::sin(-ANGLE_OFFSET);
    Vec2 leftDir = Vec2(
        aiPlayer.direction.x * cosA - aiPlayer.direction.y * sinA,
        aiPlayer.direction.x * sinA + aiPlayer.direction.y * cosA
    ).normalized();
    result.leftLine = checkLine(start, leftDir, RAYCAST_RANGE, aiPlayer.direction, game, currentTimeSec,
                                framebuffer, drawableWidth, drawableHeight);
    result.leftDir = leftDir;

    cosA = std::cos(ANGLE_OFFSET);
    sinA = std::sin(ANGLE_OFFSET);
    Vec2 rightDir = Vec2(
        aiPlayer.direction.x * cosA - aiPlayer.direction.y * sinA,
        aiPlayer.direction.x * sinA + aiPlayer.direction.y * cosA
    ).normalized();
    result.rightLine = checkLine(start, rightDir, RAYCAST_RANGE, aiPlayer.direction, game, currentTimeSec,
                                 framebuffer, drawableWidth, drawableHeight); // Fixed: drawableWidth, drawableHeight
    result.rightDir = rightDir;

    if (config.ENABLE_DEBUG && (result.centerLine.hasDanger || result.leftLine.hasDanger || result.rightLine.hasDanger)) {
        SDL_Log("Line check: center=%s at (%f, %f, dist=%f), left=%s at (%f, %f, dist=%f), right=%s at (%f, %f, dist=%f)",
                result.centerLine.color.c_str(), result.centerLine.hitPos.x, result.centerLine.hitPos.y, result.centerLine.distance,
                result.leftLine.color.c_str(), result.leftLine.hitPos.x, result.leftLine.hitPos.y, result.leftLine.distance,
                result.rightLine.color.c_str(), result.rightLine.hitPos.x, result.rightLine.hitPos.y, result.rightLine.distance);
    }

    return result;
}

AI::LineCheckResult AI::checkLine(const Vec2& start, const Vec2& dir, float maxDistance, const Vec2& playerDir,
                                  Game& game, float currentTimeSec, const std::vector<unsigned char>& framebuffer,
                                  int drawableWidth, int drawableHeight) const {
    LineCheckResult result;
    result.distance = maxDistance;
    result.hasDanger = false;
    result.greenVisible = false;
    result.hitPos = start;
    result.color = "none";

    const float STEP_SIZE = 2.0f; // Check every 2 units
    int steps = static_cast<int>(maxDistance / STEP_SIZE);
    Vec2 normDir = dir.normalized();

    for (int i = 0; i <= steps; ++i) {
        float distance = i * STEP_SIZE;
        Vec2 pos = start + normDir * distance;

        // Check wall collision
        if (pos.x < config.AI_BERTH || pos.x > game.orthoWidth - config.AI_BERTH ||
            pos.y < config.AI_BERTH || pos.y > game.orthoHeight - config.AI_BERTH) {
            result.distance = distance;
            result.hasDanger = true;
            result.hitPos = pos;
            result.color = "wall";
            break;
        }

        // Check pixel color
        std::string color = getPixelColor(pos, game, currentTimeSec, framebuffer, drawableWidth, drawableHeight);
        if (color == "green") {
            result.greenVisible = true;
            result.distance = distance;
            result.hitPos = pos;
            result.color = color;
            break;
        } else if (color == "red" || color == "blue" || color == "yellow") {
            result.hasDanger = true;
            result.distance = distance;
            result.hitPos = pos;
            result.color = color;
            break;
        }
    }

    if (config.ENABLE_DEBUG && result.hasDanger) {
        SDL_Log("checkLine: start=(%f, %f), dir=(%f, %f), hit=%s at (%f, %f), distance=%f",
                start.x, start.y, dir.x, dir.y, result.color.c_str(), result.hitPos.x, result.hitPos.y, result.distance);
    }

    return result;
}

std::string AI::getPixelColor(const Vec2& pos, Game& game, float currentTimeSec,
                              const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) const {
    float x_read = (pos.x / game.orthoWidth) * drawableWidth;
    float y_read = ((game.orthoHeight - pos.y) / game.orthoHeight) * drawableHeight;
    x_read = std::max(0.0f, std::min(x_read, static_cast<float>(drawableWidth - 1)));
    y_read = std::max(0.0f, std::min(y_read, static_cast<float>(drawableHeight - 1)));

    size_t index = static_cast<size_t>(static_cast<int>(y_read)) * drawableWidth * 3 + static_cast<size_t>(static_cast<int>(x_read)) * 3;
    if (index + 2 >= framebuffer.size()) {
        if (config.ENABLE_DEBUG) {
            SDL_Log("Framebuffer index out of bounds: index=%zu, size=%zu, pos=(%f, %f), x_read=%f, y_read=%f",
                    index, framebuffer.size(), pos.x, pos.y, x_read, y_read);
        }
        return "black";
    }

    unsigned char r = framebuffer[index];
    unsigned char g = framebuffer[index + 1];
    unsigned char b = framebuffer[index + 2];

    if (config.ENABLE_DEBUG) {
        SDL_Log("getPixelColor: pos=(%f, %f), x_read=%f, y_read=%f, color=(%d, %d, %d)",
                pos.x, pos.y, x_read, y_read, r, g, b);
    }

    if (r == 0 && g == 0 && b == 0) return "black";
    if (r == 0 && g == 255 && b == 0) return "green";
    if (r == 255 && g == 0 && b == 255) return "magenta";
    if (r == 0 && g == 0 && b == 255) return "blue";
    if (r == 255 && g == 0 && b == 0) return "red";
    if (r == 255 && g == 255 && b == 0) return "yellow";
    return "other";
}