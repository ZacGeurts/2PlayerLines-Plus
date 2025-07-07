#ifndef TYPES_H
#define TYPES_H

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>
#include <map>
#include <cmath>
#include <memory>

namespace Game {

struct GameConfig {
    int WIDTH{1920};
    int HEIGHT{1080};
    float PLAYER_SPEED{200.0f};
    float AI_SPEED{200.0f};
    float TURN_SPEED{2.0f * static_cast<float>(M_PI)};
    float AI_TURN_SPEED{180.0f};
    float AI_BERTH{10.0f};
    float RAYCAST_STEP{5.0f};
    float CIRCLE_SPEED{100.0f};
    float CIRCLE_RADIUS{40.0f};
    float COLLISION_CHECK_SIZE{10.0f};
    float BOOP_DURATION{0.5f};
    float EXPLOSION_DURATION{1.0f};
    float INVINCIBILITY_DURATION{2.0f};
    float LASER_ZAP_DURATION{0.5f};
    float WINNER_VOICE_DURATION{1.0f};
    float GREEN_SQUARE_SIZE{80.0f};
    float COLLECTIBLE_SIZE{80.0f};
    float EXPLOSION_MAX_RADIUS{40.0f};
    float PLAYER_SIZE{10.0f};
    float TRAIL_SIZE{5.0f};
    float WINNING_SCORE{50.0f};
    float GREEN_SQUARE_POINTS{1.0f};
    float DEATH_POINTS{3.0f};
    bool ENABLE_DEBUG{false};
    float COLLECT_COOLDOWN{0.5f};
    float FLASH_COOLDOWN{2.5f};
    float CIRCLE_SPAWN_INTERVAL{5.0f};
};

struct Vec2 {
    float x{0.0f};
    float y{0.0f};
    Vec2(float x_ = 0.0f, float y_ = 0.0f) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const { return s != 0.0f ? Vec2(x / s, y / s) : Vec2(0.0f, 0.0f); }
    Vec2 operator-() const { return Vec2(-x, -y); }
    float dot(const Vec2& other) const { return x * other.x + y * other.y; }
    float magnitude() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        float mag = magnitude();
        return mag > 0.0f ? Vec2(x / mag, y / mag) : Vec2(0.0f, 0.0f);
    }
    bool operator==(const Vec2& other) const {
        const float EPSILON = 1e-6f;
        return std::abs(x - other.x) < EPSILON && std::abs(y - other.y) < EPSILON;
    }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    // Comparison operator for std::set
    bool operator<(const Vec2& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

struct ExplosionParticle {
    Vec2 pos{0.0f, 0.0f};
    Vec2 vel{0.0f, 0.0f};
    float time{0.0f};
};

struct Explosion {
    std::vector<ExplosionParticle> particles;
    float startTime{0.0f};
    SDL_Color color{255, 255, 255, 255};
};

struct Flash {
    std::vector<ExplosionParticle> particles;
    float startTime{0.0f};
    SDL_Color color{255, 0, 255, 255};
    float maxRadius{0.0f};
    float duration{0.0f};
};

struct Player {
    Vec2 pos{0.0f, 0.0f};
    Vec2 direction{1.0f, 0.0f};
    SDL_Color color{255, 255, 255, 255};
    std::vector<Vec2> trail;
    bool alive{true};
    bool willDie{false};
    bool hasMoved{false};
    Vec2 deathPos{0.0f, 0.0f};
    float noCollisionTimer{0.0f};
    bool canUseNoCollision{true};
    bool isInvincible{false};
    bool collectedGreenThisFrame{false};
    bool scoredDeathThisFrame{false};
    float spawnInvincibilityTimer{0.0f};
    std::unique_ptr<Flash> endFlash;
    bool hitOpponentHead{false};
    float leftTrigger{0.0f};  // 0.0f to 1.0f
    float rightTrigger{0.0f}; // 0.0f to 1.0f
    void activateFlash() {
        if (canUseNoCollision && !isInvincible) {
            isInvincible = true;
            noCollisionTimer = 2.0f; // INVINCIBILITY_DURATION from game.ini
            canUseNoCollision = false;
        }
    }
};

struct Circle {
    Vec2 pos{0.0f, 0.0f};
    Vec2 vel{0.0f, 0.0f};
    Vec2 prevPos{0.0f, 0.0f};
    float radius{0.0f};
    SDL_Color color{255, 255, 255, 255};
    float magentaTimer{0.0f};
    bool isYellow{false};

    Circle() = default;
    Circle(Vec2 pos, Vec2 vel, Vec2 prevPos, float radius, SDL_Color color, float spawnTime, bool isColliding)
        : pos(pos), vel(vel), prevPos(prevPos), radius(radius), color(color), magentaTimer(spawnTime), isYellow(isColliding) {}
};

struct Collectible {
    Vec2 pos{0.0f, 0.0f};
    float size{0.0f};
    bool active{false};
    SDL_Color color{0, 255, 0, 255}; // Green for the square
};

} // namespace Game

#endif // TYPES_H