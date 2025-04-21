#ifndef RENDER_H
#define RENDER_H

#include <GL/gl.h> // Added for GLuint
#include "types.h"
#include <string>

void drawSquare(float x, float y, float size, const SDL_Color& color);
void drawCircle(float x, float y, float radius, const SDL_Color& color);
void drawExplosion(const Explosion& explosion, float currentTime);
void drawText(const std::string& text, float x, float y, float squareSize, const SDL_Color& color);
void drawPlayer(const Player& player);
void drawTrail(const Player& player, int skipRecent = 0);
void drawCollectibleBlackSquare(const Collectible& collectible);
void drawCollectibleBlackCircle(const Collectible& collectible);
void drawCollectibleGreenSquare(const Collectible& collectible);
void drawSplashScreen(GLuint texture);

#endif // RENDER_H