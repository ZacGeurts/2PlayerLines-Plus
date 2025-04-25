// include/types.h
#ifndef TYPES_H
#define TYPES_H

#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>
#include <map>
#include <cmath> // Added for M_PI in GameConfig

// safty net. These change with game.ini
struct GameConfig {
    int WIDTH = 1920;
    int HEIGHT = 1080;
    float PLAYER_SPEED = 200.0f;
    float TURN_SPEED = 2.0f * M_PI; // Approximately 6.2832
    float CIRCLE_SPEED = 100.0f;
    float CIRCLE_RADIUS = 40.0f;
    float COLLISION_CHECK_SIZE = 10.0f;
    float BOOP_DURATION = 0.5f;
    float EXPLOSION_DURATION = 1.0f;
    float LASER_ZAP_DURATION = 0.5f;
    float BLACK_SQUARE_SIZE = 80.0f;
    float COLLECTIBLE_SIZE = 40.0f;
    float BLACK_CIRCLE_SIZE = 60.0f;
    float EXPLOSION_MAX_RADIUS = 20.0f;
    float PLAYER_SIZE = 10.0f;
    float TRAIL_SIZE = 5.0f;
};

struct Vec2 {
    float x, y;
    Vec2(float x_ = 0, float y_ = 0) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
};

struct Player {
    Vec2 pos;
    Vec2 direction;
    ::SDL_Color color;
    std::vector<Vec2> trail;
    bool alive;
    bool willDie;
    bool hasMoved;
    Vec2 deathPos;
    float noCollisionTimer;
    bool canUseNoCollision;
    bool isInvincible;
    struct Flash* endFlash;
};

struct Circle {
    Vec2 pos;
    Vec2 vel;
    float radius;
    ::SDL_Color color;
    float magentaTimer;
    bool isYellow;
};

struct Collectible {
    Vec2 pos;
    float size;
    float blackCircleSize;
    float blackSquareSize;
};

struct ExplosionParticle {
    Vec2 pos;
    Vec2 vel;
    float time;
};

struct Explosion {
    std::vector<ExplosionParticle> particles;
    float startTime;
    bool soundPlayed;
    float soundStartTime;
};

struct Flash {
    std::vector<ExplosionParticle> particles;
    float startTime;
    bool soundPlayed;
    float soundStartTime;
    ::SDL_Color color;
};

struct BoopAudioData {
    SDL_AudioDeviceID deviceId;
    bool* playing;
    const GameConfig* config;
    float t; // Added for time tracking
};

// 5x5 font map for text rendering (1 = pixel on, 0 = pixel off)
const std::map<char, std::vector<bool>> FONT = {
    {'0', {1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1}},
    {'1', {0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0}},
    {'2', {1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1}},
    {'3', {1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1}},
    {'4', {1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1, 0,0,0,0,1, 0,0,0,0,1}},
    {'5', {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1}},
    {'6', {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1}},
    {'7', {1,1,1,1,1, 0,0,0,0,1, 0,0,0,0,1, 0,0,0,0,1, 0,0,0,0,1}},
    {'8', {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1}},
    {'9', {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1}},
    {'A', {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1}},
    {'B', {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1}},
    {'C', {1,1,1,1,1, 1,0,0,0,0, 1,0,0,0,0, 1,0,0,0,0, 1,1,1,1,1}},
    {'D', {1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1}},
    {'E', {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1}},
    {'F', {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 1,0,0,0,0, 1,0,0,0,0}},
    {'G', {1,1,1,1,1, 1,0,0,0,0, 1,0,1,1,1, 1,0,0,0,1, 1,1,1,1,1}},
    {'H', {1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1}},
    {'I', {1,1,1,1,1, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 1,1,1,1,1}},
    {'J', {1,1,1,1,1, 0,0,0,0,1, 0,0,0,0,1, 0,0,0,0,1, 1,1,1,1,1}},
    {'K', {1,0,0,0,1, 1,0,0,1,0, 1,1,1,0,0, 1,0,0,1,0, 1,0,0,0,1}},
    {'L', {1,0,0,0,0, 1,0,0,0,0, 1,0,0,0,0, 1,0,0,0,0, 1,1,1,1,1}},
    {'M', {1,0,0,0,1, 1,1,0,1,1, 1,0,1,0,1, 1,0,0,0,1, 1,0,0,0,1}},
    {'N', {1,0,0,0,1, 1,1,0,0,1, 1,0,1,0,1, 1,0,0,1,1, 1,0,0,0,1}},
    {'O', {1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1}},
    {'P', {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,0, 1,0,0,0,0}},
    {'Q', {1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,1,1, 1,1,1,1,1}},
    {'R', {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,1,0, 1,0,0,0,1}},
    {'S', {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1}},
    {'T', {1,1,1,1,1, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0}},
    {'U', {1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1}},
    {'V', {1,0,0,0,1, 1,0,0,0,1, 0,1,0,1,0, 0,1,0,1,0, 0,0,1,0,0}},
    {'W', {1,0,0,0,1, 1,0,0,0,1, 1,0,1,0,1, 1,1,0,1,1, 1,0,0,0,1}},
    {'X', {1,0,0,0,1, 0,1,0,1,0, 0,0,1,0,0, 0,1,0,1,0, 1,0,0,0,1}},
    {'Y', {1,0,0,0,1, 0,1,0,1,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0}},
    {'Z', {1,1,1,1,1, 0,0,0,1,0, 0,0,1,0,0, 0,1,0,0,0, 1,1,1,1,1}},
    {'+', {0,0,0,0,0, 0,0,1,0,0, 0,1,1,1,0, 0,0,1,0,0, 0,0,0,0,0}},
    {'-', {0,0,0,0,0, 0,0,0,0,0, 0,1,1,1,0, 0,0,0,0,0, 0,0,0,0,0}},
    {' ', {0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0}},
    {'.', {0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,1,0,0}},
    {',', {0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,1,1,0}},
    {'!', {0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,0,0,0}},
    {'?', {0,1,1,1,0, 0,0,0,1,0, 0,0,1,1,0, 0,0,0,0,0, 0,0,1,0,0}},
    {':', {0,0,0,0,0, 0,0,1,0,0, 0,0,0,0,0, 0,0,1,0,0, 0,0,0,0,0}},
    {';', {0,0,0,0,0, 0,0,1,0,0, 0,0,0,0,0, 0,0,1,0,0, 0,0,1,1,0}}
};

#endif // TYPES_H