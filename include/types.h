#ifndef TYPES_H
#define TYPES_H

#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>
#include <map>
#include <cmath>
#include <memory>

class AudioManager;
float dt;

struct GameConfig {
    int WIDTH = 1920;
    int HEIGHT = 1080;
    float PLAYER_SPEED = 200.0f;
    float AI_SPEED = 200.0f;
    float TURN_SPEED = 2.0f * M_PI;
    float AI_TURN_SPEED = 180.0f;
    float RAYCAST_STEP = 5.0f;
    float CIRCLE_SPEED = 100.0f;
    float CIRCLE_RADIUS = 40.0f;
    float COLLISION_CHECK_SIZE = 10.0f;
    float BOOP_DURATION = 0.5f;
    float EXPLOSION_DURATION = 1.0f;
    float LASER_ZAP_DURATION = 0.5f;
    float WINNER_VOICE_DURATION = 1.0f;
    float GREEN_SQUARE_SIZE = 80.0f;
    float COLLECTIBLE_SIZE = 80.0f;
    float EXPLOSION_MAX_RADIUS = 20.0f;
    float PLAYER_SIZE = 10.0f;
    float TRAIL_SIZE = 5.0f;
    float WINNING_SCORE = 100.0f;
    float GREEN_SQUARE_POINTS = 1.0f;
    float DEATH_POINTS = 3.0f;
    bool ENABLE_DEBUG = true;
    float COLLECT_COOLDOWN = 0.5f;
    float FLASH_COOLDOWN = 2.5f;
    float CIRCLE_SPAWN_INTERVAL = 5.0f;
	};

struct Vec2 {
    float x, y;
    Vec2(float x_ = 0, float y_ = 0) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator/(float s) const { return s != 0 ? Vec2(x / s, y / s) : Vec2(0, 0); }
    Vec2 operator-() const { return Vec2(-x, -y); }
    float dot(const Vec2& other) const { return x * other.x + y * other.y; }
    float magnitude() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        float mag = magnitude();
        return mag > 0 ? Vec2(x / mag, y / mag) : Vec2(0, 0);
    }
    bool operator==(const Vec2& other) const {
        const float EPSILON = 1e-6f;
        return std::abs(x - other.x) < EPSILON && std::abs(y - other.y) < EPSILON;
    }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
};

struct Flash;

struct Player {
    Vec2 pos;
    Vec2 direction;
	SDL_Color color;
    std::vector<Vec2> trail;
    bool alive;
    bool willDie;
    bool hasMoved;
    Vec2 deathPos;
    float noCollisionTimer;
    bool canUseNoCollision;
    bool isInvincible;
    bool collectedGreenThisFrame;
    bool scoredDeathThisFrame;
    float spawnInvincibilityTimer;
	std::unique_ptr<Flash> endFlash; // Managed by unique_ptr
    bool hitOpponentHead; // draw
};

struct Circle {
    Vec2 pos;
    Vec2 vel;
    Vec2 prevPos;
    float radius;
	SDL_Color SDLcirclecolor;
    float magentaTimer;
    bool isYellow;
};

struct Collectible {
    Vec2 pos;
    float size;
    bool active;
};

struct ExplosionParticle {
    Vec2 pos;
    Vec2 vel;
    float dt;
	float time;
};

struct Explosion {
    std::vector<ExplosionParticle> particles;
	float dt;
    float startTime;
	SDL_Color SDLexplodecolor;
};

struct Flash {
    std::vector<ExplosionParticle> particles;
    float startTime;
	SDL_Color SDLflashcolor = {255, 0, 255}; // magenta
    float maxRadius;
    float duration;
};

struct AudioData {
    SDL_AudioDeviceID deviceId;
    bool* playing;
    const GameConfig* config;
    float t;
    AudioManager* manager;
};

extern const std::map<char, std::vector<bool>> FONT;

#endif // TYPES_H