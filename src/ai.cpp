#include "game.h" // Include game.h for full Game definition
#include "ai.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <chrono>
#include <SDL2/SDL.h>

AI::AI(const GameConfig& config, Game& game)
    : config(config),
      game(&game),
      framebuffer(),
      drawableWidth(0),
      drawableHeight(0),
      flashUsed(false),
      modeEnabled(false),
      leftTrigger(0.0f),
      rightTrigger(0.0f),
      aButton(false),
      updateReady(false),
      newDirection(0, 0),
      shouldDie(false),
      newPosition(0, 0),
      hasMoved(false),
      hitOpponentHeadResult(false),
      currentTimeSec(0.0f),
      frameCount(0) {}

AI::~AI() {
    if (updateThread.joinable()) {
        updateThread.join();
    }
}

void AI::startUpdate(Player& aiPlayer, const Player& opponent, const Collectible& collectible,
                     const std::vector<Circle>& circles, float dt, std::mt19937& rng, Game& game,
                     const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight, SDL_Color SDLaicolor) {
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

    Vec2 newDir = aiPlayer.direction;
    float rotation = (rightTrigger - leftTrigger) * config.TURN_SPEED * 0.008333f;
    if (std::abs(rotation) > 0) {
        float cosTheta = std::cos(rotation);
        float sinTheta = std::sin(rotation);
        newDir = Vec2(
            aiPlayer.direction.x * cosTheta - aiPlayer.direction.y * sinTheta,
            aiPlayer.direction.x * sinTheta + aiPlayer.direction.y * cosTheta
        ).normalized();
    }

    float speed = config.AI_SPEED;
    Vec2 nextPos = aiPlayer.pos + newDir * speed * 0.008333f;
    nextPos.x = std::max(10.0f, std::min(nextPos.x, game->orthoWidth - 10.0f));
    nextPos.y = std::max(10.0f, std::min(nextPos.y, game->orthoHeight - 10.0f));

    if (aButton && !flashUsed && aiPlayer.canUseNoCollision) {
        game->activateNoCollision(&aiPlayer, currentTimeSec);
        flashUsed = true;
    }

    bool willDie = false;
    bool hitOpponentHead = false;
    if (aiPlayer.hasMoved && !aiPlayer.isInvincible && !aiPlayer.spawnInvincibilityTimer) {
        aiPlayer.pos = nextPos;
        aiPlayer.direction = newDir;
        aiPlayer.trail = aiPlayer.trail;
        aiPlayer.alive = true;
        aiPlayer.willDie = false;
        aiPlayer.noCollisionTimer = aiPlayer.noCollisionTimer;
        aiPlayer.isInvincible = aiPlayer.isInvincible;
        aiPlayer.hitOpponentHead = false;

        game->checkCollision(&aiPlayer, nextPos, currentTimeSec, framebuffer, drawableWidth, drawableHeight);
        if (aiPlayer.willDie) {
            willDie = true;
            hitOpponentHead = aiPlayer.hitOpponentHead;
        }
    }

    aiPlayer.direction = newDir;
    if (willDie) {
        aiPlayer.willDie = true;
        aiPlayer.deathPos = nextPos;
        aiPlayer.hitOpponentHead = hitOpponentHead; // draw
    } else {
        aiPlayer.pos = nextPos;
        aiPlayer.hasMoved = true;
    }

    if (config.ENABLE_DEBUG) {
        float effectiveSpeed = ((nextPos - aiPlayer.pos).magnitude() / 0.008333f);
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
    if (angleDiff > 0.01f) {
        if (cross > 0) {
            leftTrigger = std::min(angleDiff / (config.TURN_SPEED * dt), 1.0f);
        } else {
            rightTrigger = std::min(angleDiff / (config.TURN_SPEED * dt), 1.0f);
        }
    }

    if (!flashUsed && aiPlayer.canUseNoCollision && !aiPlayer.isInvincible && frameCount > (config.FLASH_COOLDOWN / dt)) {
        RaycastResult ray = raycastForward(aiPlayer, game, currentTimeSec, framebuffer, drawableWidth, drawableHeight);
        if (ray.centerLine.hasDanger || ray.leftLine.hasDanger || ray.rightLine.hasDanger) {
            aButton = true;
        }
    }

    std::lock_guard<std::mutex> lock(mutex);
    updateReady = true;

    if (config.ENABLE_DEBUG) {
        SDL_Log("AI input: leftTrigger=%f, rightTrigger=%f, aButton=%d, angleDiff=%f degrees",
                leftTrigger, rightTrigger, aButton, angleDiff * 180.0f / M_PI);
    }
}

AI::LineCheckResult AI::checkLine(const Vec2& start, const Vec2& dir, float maxDistance, const Vec2& playerDir,
                                  Game& game, float currentTimeSec, const std::vector<unsigned char>& framebuffer,
                                  int drawableWidth, int drawableHeight) const {
    LineCheckResult result = {"black", start, false, false, maxDistance};

    for (float d = 0; d <= maxDistance; d += config.RAYCAST_STEP) {
        Vec2 pos = start + dir * d;

        if (pos.x < 10 || pos.x > game.orthoWidth - 10 || pos.y < 10 || pos.y > game.orthoHeight - 10) {
            result = {"wall", pos, result.greenVisible, true, d};
            return result;
        }

        std::string color = getPixelColor(pos, game, currentTimeSec, framebuffer, drawableWidth, drawableHeight);

        Vec2 toPixel = pos - start;
        float distance = toPixel.magnitude();
        if (color == "red" && distance < 10.0f && toPixel.dot(playerDir) < -0.5f) {
            continue;
        }

        if (color == "green") {
            result.greenVisible = true;
        } else if (color == "black" || color == "magenta") {
        } else {
            result = {color, pos, result.greenVisible, true, d};
            return result;
        }
    }

    return result;
}

AI::RaycastResult AI::raycastForward(const Player& aiPlayer, Game& game, float currentTimeSec,
                                     const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    RaycastResult result;
    const float ANGLE_OFFSET = M_PI / 6;
    const float RAYCAST_RANGE = 100.0f;
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
                                 framebuffer, drawableWidth, drawableHeight);
    result.rightDir = rightDir;

    if (config.ENABLE_DEBUG && (result.centerLine.hasDanger || result.leftLine.hasDanger || result.rightLine.hasDanger)) {
        SDL_Log("Line check: center=%s at (%f, %f, dist=%f), left=%s at (%f, %f, dist=%f), right=%s at (%f, %f, dist=%f)",
                result.centerLine.color.c_str(), result.centerLine.hitPos.x, result.centerLine.hitPos.y, result.centerLine.distance,
                result.leftLine.color.c_str(), result.leftLine.hitPos.x, result.leftLine.hitPos.y, result.leftLine.distance,
                result.rightLine.color.c_str(), result.rightLine.hitPos.x, result.rightLine.hitPos.y, result.rightLine.distance);
    }

    return result;
}

std::string AI::getPixelColor(const Vec2& pos, Game& game, float currentTimeSec,
                              const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) const {
    float x_read = (pos.x / game.orthoWidth) * drawableWidth;
    float y_read = (1.0f - pos.y / game.orthoHeight) * drawableHeight;
    x_read = std::max(0.0f, std::min(x_read, static_cast<float>(drawableWidth - 1)));
    y_read = std::max(0.0f, std::min(y_read, static_cast<float>(drawableHeight - 1)));

    size_t index = static_cast<size_t>(static_cast<int>(y_read)) * drawableWidth * 3 + static_cast<size_t>(static_cast<int>(x_read)) * 3;
    if (index + 2 >= framebuffer.size()) {
        if (config.ENABLE_DEBUG) {
            SDL_Log("Framebuffer index out of bounds in getPixelColor: index=%zu, size=%zu", index, framebuffer.size());
        }
        return "black";
    }

    unsigned char r = framebuffer[index];
    unsigned char g = framebuffer[index + 1];
    unsigned char b = framebuffer[index + 2];

    if (r == 0 && g == 0 && b == 0) return "black";
    if (r == 0 && g == 255 && b == 0) return "green";
    if (r == 255 && g == 0 && b == 255) return "magenta";
    if (r == 0 && g == 0 && b == 255) return "blue";
    if (r == 255 && g == 0 && b == 0) return "red";
    if (r == 255 && g == 255 && b == 0) return "yellow";
    return "other";
}

Vec2 AI::calculateTargetDirection(const Player& aiPlayer, const Collectible& collectible,
                                  const std::vector<Circle>& circles, const Player& opponent,
                                  std::mt19937& rng, Game& game, float currentTimeSec,
                                  const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    Vec2 toCollectible = (collectible.pos - aiPlayer.pos).normalized();
    RaycastResult ray = raycastForward(aiPlayer, game, currentTimeSec, framebuffer, drawableWidth, drawableHeight);

    if (!ray.centerLine.hasDanger || ray.centerLine.greenVisible) {
        return toCollectible;
    }

    if (!ray.leftLine.hasDanger && ray.rightLine.hasDanger) {
        return ray.leftDir;
    } else if (!ray.rightLine.hasDanger && ray.leftLine.hasDanger) {
        return ray.rightDir;
    } else if (!ray.leftLine.hasDanger && !ray.rightLine.hasDanger) {
        float leftScore = ray.leftDir.dot(toCollectible);
        float rightScore = ray.rightDir.dot(toCollectible);
        return leftScore > rightScore ? ray.leftDir : ray.rightDir;
    }

    std::uniform_real_distribution<float> dist(-M_PI / 2, M_PI / 2);
    Vec2 newDir = Vec2(
        aiPlayer.direction.x * std::cos(dist(rng)) - aiPlayer.direction.y * std::sin(dist(rng)),
        aiPlayer.direction.x * std::sin(dist(rng)) + aiPlayer.direction.y * std::cos(dist(rng))
    ).normalized();
    return newDir;
}