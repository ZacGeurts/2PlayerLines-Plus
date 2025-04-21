#ifndef TYPES_H
#define TYPES_H

#include <SDL2/SDL.h>
#include <vector>
#include <map>

struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
};

struct Player {
    Vec2 pos;
    Vec2 direction;
    SDL_Color color;
    std::vector<Vec2> trail;
    bool alive;
    bool willDie;
    bool hasMoved;
    Vec2 deathPos;
};

struct Circle {
    Vec2 pos;
    Vec2 vel;
    float radius;
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
    Vec2 pos;
    std::vector<ExplosionParticle> particles;
    float startTime;
    bool active;
    bool soundPlayed;
};

struct BoopAudioData {
    SDL_AudioDeviceID deviceId;
    bool* playing;
};

extern const std::map<char, std::vector<bool>> FONT;

#endif // TYPES_H