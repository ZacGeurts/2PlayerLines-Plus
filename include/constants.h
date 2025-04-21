#ifndef CONSTANTS_H
#define CONSTANTS_H

const int WIDTH = 1920;
const int HEIGHT = 1080;
const float PLAYER_SPEED = 200.0f;
const float CIRCLE_SPEED = 300.0f;
const float TURN_SPEED = 2.0f * M_PI;
const int PLAYER_SIZE = 4;
const int TRAIL_SIZE = 2;
const int CIRCLE_RADIUS = 45;
const float COLLECTIBLE_SIZE = CIRCLE_RADIUS * 2;
const float BLACK_CIRCLE_SIZE = COLLECTIBLE_SIZE * 2;
const float BLACK_SQUARE_SIZE = COLLECTIBLE_SIZE * 2;
const int COLLISION_CHECK_SIZE = 5;
const int EXPLOSION_DURATION = 1;
const int EXPLOSION_PARTICLES = 50;
const float EXPLOSION_MAX_RADIUS = 100.0f;
const float BOOP_DURATION = 0.1f;

#endif // CONSTANTS_H