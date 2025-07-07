#include "ai.h"
#include "game.h"
#include <queue>
#include <cmath>
#include <algorithm>
#include <set>

namespace Game {

/*
 * AI Implementation Overview
 *
 * Purpose:
 * This AI controls Player 2 when enabled (modeEnabled = true), mimicking human player mechanics
 * as defined in player.cpp. It prioritizes collecting green squares (1 point) while avoiding
 * collisions with red, yellow, blue pixels, or screen borders (3 points to opponent).
 *
 * Key Mechanics:
 * - Fixed Speed: Moves at config.AI_SPEED (200.0f) with leftTrigger = 1.0f, rightTrigger = 1.0f.
 * - Fixed Turn Rate: Limited to config.AI_TURN_SPEED (180 deg/sec, converted to radians).
 * - Flash: One 2-second invincibility flash per round (config.INVINCIBILITY_DURATION = 2.0f),
 *   reset after collision via resetFlash(). Cannot flash past screen borders (1920x1080).
 * - Pathfinding: Uses A* algorithm to find a collision-free path to the green square
 *   (collectible.pos), respecting config.AI_BERTH (10.0f) from obstacles and borders.
 * - Raycasting: Detects dangers (red, yellow, blue, borders) using config.RAYCAST_STEP (5.0f).
 * - Safe Colors: Black ("none") and green are safe; red, yellow, blue, and borders are not.
 *
 * Control Flow:
 * - startUpdate: Spawns a thread to compute inputs (leftTrigger, rightTrigger, aButton).
 * - simulateControllerInput: Calculates new direction and position, checks for dangers,
 *   and decides whether to flash.
 * - applyUpdate: Applies computed inputs to player2, updating pos, direction, and flash state.
 * - calculateTargetDirection: Uses A* to find a path to the green square.
 * - raycastForward/checkLine: Detects obstacles and green squares via pixel colors.
 * - getPixelColor: Identifies pixel colors at a position (red, yellow, green, blue, none).
 *
 * Reminder for Updates:
 * - Ensure AI respects player mechanics (no special abilities, same speed/turn constraints).
 * - Verify flash reset in Game::handlePlayerDeath or PlayerManager::updatePlayers after collisions.
 * - Check A* pathfinding performance; adjust gridSize (10.0f) if needed for 1920x1080.
 * - Test boundary handling (config.AI_BERTH = 10.0f) to avoid border collisions.
 * - If adding features, maintain parity with human player (e.g., no dynamic speed changes).
 * - Update getPixelColor thresholds if color detection changes.
 */

AI::AI(const GameConfig& config, Game& game)
    : config(config), game(&game), flashUsed(false), modeEnabled(true), updateReady(false),
      shouldDie(false), hasMoved(false), hitOpponentHeadResult(false), aButton(false),
      leftTrigger(0.0f), rightTrigger(0.0f), currentTimeSec(0.0f), frameCount(0),
      drawableWidth(1920), drawableHeight(1080), aiColor({255, 255, 255, 255}) {}

AI::~AI() {
    if (updateThread.joinable()) {
        updateThread.join();
    }
}

void AI::startUpdate(Player& aiPlayer, const Player& opponent, const Collectible& collectible,
                     const std::vector<Circle>& circles, float dt, std::mt19937& rng,
                     const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight,
                     SDL_Color aiColor) {
    if (!modeEnabled) return; // Only control player2 if AI is enabled
    if (updateThread.joinable()) {
        updateThread.join();
    }
    this->framebuffer = framebuffer;
    this->drawableWidth = 1920; // Fixed resolution
    this->drawableHeight = 1080;
    this->aiColor = aiColor;
    updateThread = std::thread([this, &aiPlayer, &opponent, &collectible, &circles, dt, &rng, &framebuffer]() {
        std::lock_guard<std::mutex> lock(mutex);
        simulateControllerInput(aiPlayer, collectible, circles, opponent, dt, rng, *game,
                               framebuffer, 1920, 1080);
        updateReady = true;
    });
}

void AI::waitForUpdate() {
    if (updateThread.joinable()) {
        updateThread.join();
    }
}

void AI::applyUpdate(Player& aiPlayer) {
    std::lock_guard<std::mutex> lock(mutex);
    if (updateReady) {
        if (hasMoved) {
            aiPlayer.pos = newPosition;
            aiPlayer.direction = newDirection.normalized();
            aiPlayer.hasMoved = true;
        }
        if (aButton && !flashUsed) {
            aiPlayer.activateFlash();
            flashUsed = true;
        }
        aiPlayer.leftTrigger = leftTrigger;
        aiPlayer.rightTrigger = rightTrigger;
        updateReady = false;
    }
}

struct Node {
    Vec2 pos;
    float gCost, hCost;
    Node* parent;
    Node(Vec2 p, float g, float h) : pos(p), gCost(g), hCost(h), parent(nullptr) {}
    float fCost() const { return gCost + hCost; }
};

struct CompareNode {
    bool operator()(const Node* a, const Node* b) const {
        return a->fCost() > b->fCost() || (a->fCost() == b->fCost() && a->hCost > b->hCost);
    }
};

Vec2 AI::calculateTargetDirection(const Player& aiPlayer, const Collectible& collectible,
                                 const std::vector<Circle>& circles, const Player& opponent,
                                 std::mt19937& rng, Game& game, float currentTimeSec,
                                 const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    const float gridSize = 10.0f; // Fixed grid size for 1920x1080
    const float berth = config.AI_BERTH; // 10.0f from game.ini
    Vec2 target = collectible.pos; // Always target green square

    std::priority_queue<Node*, std::vector<Node*>, CompareNode> openList;
    std::set<Vec2> closedList;
    std::vector<Node*> nodes;

    Vec2 startPos = { std::floor(aiPlayer.pos.x / gridSize) * gridSize,
                      std::floor(aiPlayer.pos.y / gridSize) * gridSize };
    Vec2 goalPos = { std::floor(target.x / gridSize) * gridSize,
                     std::floor(target.y / gridSize) * gridSize };

    // Ensure target is within bounds
    goalPos.x = std::max(berth, std::min(1920.0f - berth, goalPos.x));
    goalPos.y = std::max(berth, std::min(1080.0f - berth, goalPos.y));

    Node* startNode = new Node(startPos, 0.0f, (goalPos - startPos).magnitude());
    nodes.push_back(startNode);
    openList.push(startNode);

    const std::vector<Vec2> directions = {
        {0, gridSize}, {0, -gridSize}, {gridSize, 0}, {-gridSize, 0},
        {gridSize, gridSize}, {gridSize, -gridSize}, {-gridSize, gridSize}, {-gridSize, -gridSize}
    };

    while (!openList.empty()) {
        Node* current = openList.top();
        openList.pop();
        if (closedList.find(current->pos) != closedList.end()) continue;
        closedList.insert(current->pos);

        if ((current->pos - goalPos).magnitude() <= gridSize * 1.5f) {
            std::vector<Vec2> path;
            while (current) {
                path.push_back(current->pos);
                current = current->parent;
            }
            for (Node* node : nodes) delete node;
            if (path.size() > 1) {
                Vec2 nextPos = path[path.size() - 2];
                return (nextPos - aiPlayer.pos).normalized();
            }
            return (target - aiPlayer.pos).normalized();
        }

        for (const auto& dir : directions) {
            Vec2 newPos = current->pos + dir;
            if (newPos.x < berth || newPos.x > 1920.0f - berth ||
                newPos.y < berth || newPos.y > 1080.0f - berth) continue;
            if (closedList.find(newPos) != closedList.end()) continue;

            bool collision = false;
            for (const auto& circle : circles) {
                if ((newPos - circle.pos).magnitude() < circle.radius + berth + config.COLLISION_CHECK_SIZE) {
                    collision = true;
                    break;
                }
            }
            if (collision) continue;

            std::string color = getPixelColor(newPos, game, currentTimeSec, framebuffer, 1920, 1080);
            if (color == "red" || color == "yellow" || color == "blue") continue;

            float gCost = current->gCost + dir.magnitude();
            float hCost = (goalPos - newPos).magnitude();
            Node* newNode = new Node(newPos, gCost, hCost);
            newNode->parent = current;
            nodes.push_back(newNode);
            openList.push(newNode);
        }
    }

    for (Node* node : nodes) delete node;
    return (target - aiPlayer.pos).normalized(); // Fallback
}

AI::RaycastResult AI::raycastForward(const Player& aiPlayer, Game& game, float currentTimeSec,
                                    const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    RaycastResult result;
    float maxDistance = 100.0f; // Fixed for 1920x1080
    result.centerLine = checkLine(aiPlayer.pos, aiPlayer.direction, maxDistance, aiPlayer.direction,
                                  game, currentTimeSec, framebuffer, 1920, 1080);
    float angle = 20.0f * M_PI / 180.0f; // Fixed angle for precision
    result.leftDir = { aiPlayer.direction.x * std::cos(angle) - aiPlayer.direction.y * std::sin(angle),
                       aiPlayer.direction.x * std::sin(angle) + aiPlayer.direction.y * std::cos(angle) };
    result.rightDir = { aiPlayer.direction.x * std::cos(-angle) - aiPlayer.direction.y * std::sin(-angle),
                        aiPlayer.direction.x * std::sin(-angle) + aiPlayer.direction.y * std::cos(angle) };
    result.leftLine = checkLine(aiPlayer.pos, result.leftDir, maxDistance, aiPlayer.direction,
                                game, currentTimeSec, framebuffer, 1920, 1080);
    result.rightLine = checkLine(aiPlayer.pos, result.rightDir, maxDistance, aiPlayer.direction,
                                 game, currentTimeSec, framebuffer, 1920, 1080);
    return result;
}

AI::RaycastResult::LineCheckResult AI::checkLine(const Vec2& start, const Vec2& dir, float maxDistance, const Vec2& playerDir,
                                                 Game& game, float currentTimeSec, const std::vector<unsigned char>& framebuffer,
                                                 int drawableWidth, int drawableHeight) const {
    RaycastResult::LineCheckResult result = { "none", start, false, false, maxDistance };
    Vec2 current = start;
    float step = config.RAYCAST_STEP; // 5.0f from game.ini
    float distance = 0.0f;

    while (distance < maxDistance) {
        current = start + dir * distance;
        if (current.x <= config.AI_BERTH || current.x >= 1920.0f - config.AI_BERTH ||
            current.y <= config.AI_BERTH || current.y >= 1080.0f - config.AI_BERTH) {
            result.hasDanger = true;
            result.distance = distance;
            break;
        }
        std::string color = getPixelColor(current, game, currentTimeSec, framebuffer, 1920, 1080);
        if (color == "green") {
            result.greenVisible = true;
            result.distance = distance;
            break; // Prioritize green detection
        } else if (color == "red" || color == "yellow" || color == "blue") {
            result.color = color;
            result.hitPos = current;
            result.hasDanger = true;
            result.distance = distance;
            break;
        }
        distance += step;
    }
    return result;
}

std::string AI::getPixelColor(const Vec2& pos, Game& game, float currentTimeSec,
                              const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) const {
    int x = static_cast<int>(std::clamp(pos.x, 0.0f, 1919.0f));
    int y = static_cast<int>(std::clamp(pos.y, 0.0f, 1079.0f));
    int index = (y * 1920 + x) * 4;
    if (static_cast<size_t>(index + 3) >= framebuffer.size()) return "none";

    unsigned char r = framebuffer[index];
    unsigned char g = framebuffer[index + 1];
    unsigned char b = framebuffer[index + 2];

    if (r > 200 && g < 50 && b < 50) return "red";
    if (r > 200 && g > 200 && b < 50) return "yellow";
    if (r < 50 && g > 200 && b < 50) return "green";
    if (r < 50 && g < 50 && b > 200) return "blue";
    return "none"; // Black or other colors are safe
}

void AI::simulateControllerInput(const Player& aiPlayer, const Collectible& collectible,
                                const std::vector<Circle>& circles, const Player& opponent,
                                float dt, std::mt19937& rng, Game& game,
                                const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    frameCount++;
    currentTimeSec += dt;

    RaycastResult raycast = raycastForward(aiPlayer, game, currentTimeSec, framebuffer, 1920, 1080);
    newDirection = calculateTargetDirection(aiPlayer, collectible, circles, opponent, rng, game,
                                           currentTimeSec, framebuffer, 1920, 1080);

    // Limit turning to AI_TURN_SPEED (180 degrees/sec, converted to radians)
    float maxTurnAngle = (config.AI_TURN_SPEED * M_PI / 180.0f) * dt;
    Vec2 currentDir = aiPlayer.direction.normalized();
    float dot = std::max(-1.0f, std::min(1.0f, currentDir.dot(newDirection.normalized())));
    if (dot < std::cos(maxTurnAngle)) {
        float angle = std::acos(dot);
        float sign = currentDir.x * newDirection.y - currentDir.y * newDirection.x >= 0 ? 1.0f : -1.0f;
        float turnAngle = std::min(maxTurnAngle, angle);
        newDirection = Vec2{
            currentDir.x * std::cos(turnAngle) - currentDir.y * std::sin(turnAngle) * sign,
            currentDir.x * std::sin(turnAngle) * sign + currentDir.y * std::cos(turnAngle)
        }.normalized();
    }

    // Controller inputs: fixed speed, no slowing down
    leftTrigger = 1.0f; // Full speed
    rightTrigger = 1.0f;
    aButton = false;

    // Flash for imminent collision (not near borders)
    if (!flashUsed && raycast.centerLine.hasDanger && raycast.centerLine.distance < 20.0f &&
        (raycast.centerLine.color == "red" || raycast.centerLine.color == "yellow" || raycast.centerLine.color == "blue")) {
        // Check if flash would keep AI within bounds
        Vec2 flashPos = aiPlayer.pos + newDirection * config.AI_SPEED * config.INVINCIBILITY_DURATION;
        if (flashPos.x >= config.AI_BERTH && flashPos.x <= 1920.0f - config.AI_BERTH &&
            flashPos.y >= config.AI_BERTH && flashPos.y <= 1080.0f - config.AI_BERTH) {
            aButton = true;
        }
    }

    // Position update with safety checks
    newPosition = aiPlayer.pos + newDirection * config.AI_SPEED * dt;
    float boundaryMargin = config.AI_BERTH; // 10.0f from game.ini
    std::string newPosColor = getPixelColor(newPosition, game, currentTimeSec, framebuffer, 1920, 1080);
    if (newPosition.x >= boundaryMargin && newPosition.x < 1920.0f - boundaryMargin &&
        newPosition.y >= boundaryMargin && newPosition.y < 1080.0f - boundaryMargin &&
        (newPosColor == "green" || newPosColor == "none")) {
        hasMoved = true;
    } else {
        hasMoved = false;
        // Reverse direction if hitting a border or dangerous color
        newDirection = -newDirection.normalized();
        if (newPosColor == "red" || newPosColor == "yellow" || newPosColor == "blue") {
            // Attempt flash if not used and not near border
            if (!flashUsed) {
                Vec2 flashPos = aiPlayer.pos + newDirection * config.AI_SPEED * config.INVINCIBILITY_DURATION;
                if (flashPos.x >= boundaryMargin && flashPos.x <= 1920.0f - boundaryMargin &&
                    flashPos.y >= boundaryMargin && flashPos.y <= 1080.0f - boundaryMargin) {
                    aButton = true;
                }
            }
        }
    }
}

} // namespace Game