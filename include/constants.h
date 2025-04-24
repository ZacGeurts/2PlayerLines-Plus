// constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cmath>

struct GameConfig {
    int WIDTH = 1920;
    int HEIGHT = 1080;
    float PLAYER_SPEED = 200.0f;
    float TURN_SPEED = 2.0f * M_PI;
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

#endif // CONSTANTS_H