#include "render.h"
#include "constants.h"
#include <GL/gl.h>

void drawSquare(float x, float y, float size, const SDL_Color& color) {
    glColor3ub(color.r, color.g, color.b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + size, y);
    glVertex2f(x + size, y + size);
    glVertex2f(x, y + size);
    glEnd();
}

void drawCircle(float x, float y, float radius, const SDL_Color& color) {
    glColor3ub(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < 360; i++) {
        float rad = i * M_PI / 180.0f;
        glVertex2f(x + cos(rad) * radius, y + sin(rad) * radius);
    }
    glEnd();
}

void drawExplosion(const Explosion& explosion, float currentTime) {
    float elapsed = currentTime - explosion.startTime;
    if (elapsed > EXPLOSION_DURATION) return;

    for (const auto& particle : explosion.particles) {
        float t = particle.time + elapsed / EXPLOSION_DURATION;
        if (t > 1.0f) continue;
        Vec2 pos = particle.pos + particle.vel * t * EXPLOSION_MAX_RADIUS;
        SDL_Color color = {
            (Uint8)(255 * (1.0f - t)),
            (Uint8)(255 * t),
            0,
            255
        };
        drawSquare(pos.x - 2, pos.y - 2, 4, color);
    }
}

void drawText(const std::string& text, float x, float y, float squareSize, const SDL_Color& color) {
    glColor3ub(color.r, color.g, color.b);
    float charWidth = squareSize * 6;
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (FONT.find(c) == FONT.end()) continue;
        const auto& pattern = FONT.at(c);
        float startX = x + i * charWidth;
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (pattern[row * 5 + col]) {
                    drawSquare(startX + col * squareSize, y + row * squareSize, squareSize, color);
                }
            }
        }
    }
}

void drawPlayer(const Player& player) {
    drawSquare(player.pos.x - PLAYER_SIZE / 2, player.pos.y - PLAYER_SIZE / 2, PLAYER_SIZE, player.color);
}

void drawTrail(const Player& player, int skipRecent) {
    glColor3ub(player.color.r, player.color.g, player.color.b);
    glBegin(GL_QUADS);
    size_t start = std::max<size_t>(0, player.trail.size() - skipRecent);
    for (size_t i = 0; i < start; ++i) {
        const auto& p = player.trail[i];
        float halfSize = TRAIL_SIZE / 2.0f;
        glVertex2f(p.x - halfSize, p.y - halfSize);
        glVertex2f(p.x + halfSize, p.y - halfSize);
        glVertex2f(p.x + halfSize, p.y + halfSize);
        glVertex2f(p.x - halfSize, p.y + halfSize);
    }
    glEnd();
}

void drawCollectibleBlackSquare(const Collectible& collectible) {
    drawSquare(collectible.pos.x - collectible.blackSquareSize / 2, collectible.pos.y - collectible.blackSquareSize / 2, collectible.blackSquareSize, {0, 0, 0, 255});
}

void drawCollectibleBlackCircle(const Collectible& collectible) {
    drawCircle(collectible.pos.x, collectible.pos.y, collectible.blackCircleSize / 2, {0, 0, 0, 255});
}

void drawCollectibleGreenSquare(const Collectible& collectible) {
    drawSquare(collectible.pos.x - collectible.size / 2, collectible.pos.y - collectible.size / 2, collectible.size, {0, 255, 0, 255});
}

void drawSplashScreen(GLuint texture) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);      // Bottom-left
    glTexCoord2f(1.0f, 0.0f); glVertex2f(WIDTH, 0.0f);     // Bottom-right
    glTexCoord2f(1.0f, 1.0f); glVertex2f(WIDTH, HEIGHT);   // Top-right
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, HEIGHT);    // Top-left
    glEnd();
    glDisable(GL_TEXTURE_2D);
}