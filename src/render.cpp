#include "render.h"
#include "game.h"
#include <GL/gl.h>
#include <chrono>
#include <cmath>

RenderManager::RenderManager(const GameConfig& config) : config(config) {}

void RenderManager::drawSquare(float x, float y, float size, const SDL_Color& color) const {
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + size, y);
    glVertex2f(x + size, y + size);
    glVertex2f(x, y + size);
    glEnd();
}

void RenderManager::drawBlackCircle(float x, float y, float radius) const {
    glColor4ub(0, 0, 0, 255); // Black, safe color
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 20; ++i) {
        float angle = 2.0f * M_PI * i / 20.0f;
        glVertex2f(x + radius * cos(angle), y + radius * sin(angle));
    }
    glEnd();
}

void RenderManager::drawCircle(float x, float y, float radius, const SDL_Color& color) const {
    // Draw black circle to erase trails
    drawBlackCircle(x, y, radius);
    // Draw colored circle (magenta or yellow)
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 20; ++i) {
        float angle = 2.0f * M_PI * i / 20.0f;
        glVertex2f(x + radius * cos(angle), y + radius * sin(angle));
    }
    glEnd();
}

void RenderManager::drawExplosion(const Explosion& explosion, float currentTimeSec) const {
    float elapsed = currentTimeSec - explosion.startTime;
    if (elapsed > config.EXPLOSION_DURATION) return;
    for (const auto& particle : explosion.particles) {
        float t = particle.time + elapsed / config.EXPLOSION_DURATION;
        if (t > 1.0f) continue;
        Vec2 pos = particle.pos + particle.vel * t * config.EXPLOSION_MAX_RADIUS;
        drawSquare(pos.x - 2, pos.y - 2, 4, {255, 255, 255, 255});
    }
}

void RenderManager::drawText(const std::string& text, float x, float y, float squareSize, const SDL_Color& color) const {
    float currentX = x;
    for (char c : text) {
        if (FONT.find(c) == FONT.end()) continue;
        const auto& pattern = FONT.at(c);
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (pattern[row * 5 + col]) {
                    drawSquare(currentX + col * squareSize, y + row * squareSize, squareSize, color);
                }
            }
        }
        currentX += 6 * squareSize; // Fixed-width: 5 pixels + 1 pixel spacing
    }
}

void RenderManager::drawPlayer(const Player& player) const {
    if (!player.alive) return;
    drawSquare(player.pos.x - config.PLAYER_SIZE / 2, player.pos.y - config.PLAYER_SIZE / 2, config.PLAYER_SIZE, player.color);
}

void RenderManager::drawTrail(const Player& player, int skipRecent) const {
    if (!player.alive || player.trail.size() < 2) return;

    glColor4ub(player.color.r, player.color.g, player.color.b, player.color.a);
    glLineWidth(config.TRAIL_SIZE);
    glBegin(GL_LINES);

    // Draw trail segments, checking for gaps (large distances)
    for (size_t i = 0; i < player.trail.size() - 1 - skipRecent; ++i) {
        const auto& current = player.trail[i];
        const auto& next = player.trail[i + 1];
        // Skip drawing if points are too far apart (indicating a gap from clearTrails)
        float distance = (next - current).magnitude();
        if (distance < 50.0f) { // Threshold to detect gaps
            glVertex2f(current.x, current.y);
            glVertex2f(next.x, next.y);
        }
    }

    glEnd();
    glLineWidth(1.0f);
}

void RenderManager::drawCollectibleGreenSquare(const Collectible& collectible) const {
    drawSquare(collectible.pos.x - collectible.size / 2, collectible.pos.y - collectible.size / 2, collectible.size, {0, 255, 0, 255});
}

void RenderManager::renderSplashScreen(GLuint texture) const {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(config.WIDTH, 0);
    glTexCoord2f(1, 1); glVertex2f(config.WIDTH, config.HEIGHT);
    glTexCoord2f(0, 1); glVertex2f(0, config.HEIGHT);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void RenderManager::renderGame(const Game& game, float currentTimeSec) const {
    int drawableWidth, drawableHeight;
    SDL_GL_GetDrawableSize(game.window, &drawableWidth, &drawableHeight);
    glViewport(0, 0, drawableWidth, drawableHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, game.orthoWidth, game.orthoHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (const auto& explosion : game.explosions) {
        drawExplosion(explosion, currentTimeSec);
    }
    for (const auto& flash : game.flashes) {
        float elapsed = currentTimeSec - flash.startTime;
        if (elapsed > 0.3f) continue;
        for (const auto& particle : flash.particles) {
            float t = particle.time + elapsed / 0.3f;
            if (t > 1.0f) continue;
            Vec2 pos = particle.pos + particle.vel * t * 20.0f;
            drawSquare(pos.x - 2, pos.y - 2, 4, flash.color);
        }
    }
    // Draw trails before circles to allow circles to overwrite them
    if (!game.player1.isInvincible) drawTrail(game.player1);
    if (!game.player2.isInvincible) drawTrail(game.player2);
    drawCollectibleGreenSquare(game.collectible);
    for (const auto& circle : game.circles) {
        drawCircle(circle.pos.x, circle.pos.y, circle.radius, circle.SDLcolor);
    }
    drawPlayer(game.player1);
    drawPlayer(game.player2);

    if (game.paused) {
        float squareSize = 8.0f;
        drawText("PAUSED", config.WIDTH / 2 - 6 * 6 * squareSize / 2, config.HEIGHT / 2 - 50, squareSize, {255, 255, 255, 255});
        std::string totalText = std::to_string(game.score1) + "-" + std::to_string(game.score2);
        float totalTextWidth = totalText.size() * 6 * squareSize;
        drawText(totalText, config.WIDTH / 2 - totalTextWidth / 2, config.HEIGHT / 2, squareSize, {255, 255, 255, 255});
        drawText("W:" + std::to_string(game.setScore1), 10, 10, squareSize, {0, 0, 255, 255});
        std::string setScore2Text = "W:" + std::to_string(game.setScore2);
        float setScore2Width = setScore2Text.size() * 6 * squareSize;
        drawText(setScore2Text, config.WIDTH - setScore2Width - 10, 10, squareSize, {255, 0, 0, 255});
    }
}

void RenderManager::renderGameOver(const Game& game, float orthoWidth, float orthoHeight) const {
    float currentTimeSec = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
    int drawableWidth, drawableHeight;
    SDL_GL_GetDrawableSize(game.window, &drawableWidth, &drawableHeight);
    glViewport(0, 0, drawableWidth, drawableHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, orthoWidth, orthoHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (const auto& explosion : game.explosions) {
        drawExplosion(explosion, currentTimeSec);
    }

    float squareSize = 8.0f;
    drawText("W:" + std::to_string(game.setScore1), 10, 10, squareSize, {0, 0, 255, 255});
    std::string setScore2Text = "W:" + std::to_string(game.setScore2);
    float setScore2Width = setScore2Text.size() * 6 * squareSize;
    drawText(setScore2Text, orthoWidth - setScore2Width - 10, 10, squareSize, {255, 0, 0, 255});

    std::string winText;
    SDL_Color winColor = {255, 255, 255, 255};
    if (game.score1 >= config.WINNING_SCORE && game.score2 >= config.WINNING_SCORE) {
        winText = "OVERALL DRAW!";
    } else if (game.score1 >= config.WINNING_SCORE) {
        winText = "PLAYER ONE WINS THE SET";
        winColor = {0, 0, 255, 255};
    } else if (game.score2 >= config.WINNING_SCORE) {
        winText = "PLAYER TWO WINS THE SET";
        winColor = {255, 0, 0, 255};
    } else if (!game.player1.alive && !game.player2.alive) {
        winText = "DRAW!";
    } else if (!game.player1.alive) {
        winText = "P2 WIN";
        winColor = {255, 0, 0, 255};
    } else if (!game.player2.alive) {
        winText = "P1 WIN";
        winColor = {0, 0, 255, 255};
    }
    float winTextWidth = winText.size() * 6 * squareSize;
    float winTextY = orthoHeight / 2 - 60;
    drawText(winText, orthoWidth / 2 - winTextWidth / 2, winTextY, squareSize, winColor);

    std::string totalText = std::to_string(game.score1) + "-" + std::to_string(game.score2);
    float totalTextWidth = totalText.size() * 6 * squareSize;
    float totalTextY = winTextY + squareSize * 5 + 20;
    drawText(totalText, orthoWidth / 2 - totalTextWidth / 2, totalTextY, squareSize, {255, 255, 255, 255});

    std::string roundText = "+" + std::to_string(game.roundScore1) + " - +" + std::to_string(game.roundScore2);
    float roundTextWidth = roundText.size() * 6 * squareSize;
    float roundTextY = totalTextY + squareSize * 5 + 20;
    drawText(roundText, orthoWidth / 2 - roundTextWidth / 2, roundTextY, squareSize, {255, 255, 255, 255});

    int countdown = 5 - static_cast<int>(std::chrono::duration<float>(std::chrono::steady_clock::now() - game.gameOverTime).count());
    if (countdown >= 0) {
        std::string countdownText = std::to_string(std::max(1, countdown));
        float countdownWidth = countdownText.size() * 6 * squareSize;
        float countdownY = roundTextY + squareSize * 5 + 20;
        drawText(countdownText, orthoWidth / 2 - countdownWidth / 2, countdownY, squareSize, {255, 255, 255, 255});
    }
}