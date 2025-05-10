#ifndef RENDER_H
#define RENDER_H

#include <GL/gl.h>
#include "types.h"
#include <string>
#include <SDL2/SDL.h>

// Forward declaration of Game struct
struct Game;

class RenderManager {
public:
    RenderManager(const GameConfig& config);
    void renderSplashScreen(GLuint texture) const;
    void renderGame(const Game& game, float currentTimeSec) const;
    void renderGameOver(const Game& game, float orthoWidth, float orthoHeight) const;
    void drawCircle(float x, float y, float radius, const SDL_Color& color) const;
	void drawBlackCircle(float x, float y, float radius) const;
    void drawTrail(const Player& player, int skipRecent = 0) const;
    void drawText(const std::string& text, float x, float y, float squareSize, const SDL_Color& color) const;

private:
    void drawSquare(float x, float y, float size, const SDL_Color& color) const;
    void drawExplosion(const Explosion& explosion, float currentTimeSec) const;
    void drawPlayer(const Player& player) const;
    void drawCollectibleGreenSquare(const Collectible& collectible) const;

    const GameConfig& config;
};

#endif // RENDER_H