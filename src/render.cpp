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

void RenderManager::drawCircle(float x, float y, float radius, const SDL_Color& color) const {
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
        currentX += 6 * squareSize; // Character width + 1 pixel spacing
    }
}

void RenderManager::drawPlayer(const Player& player) const {
    if (!player.alive) return;
    drawSquare(player.pos.x - config.PLAYER_SIZE / 2, player.pos.y - config.PLAYER_SIZE / 2, config.PLAYER_SIZE, player.color);
}

void RenderManager::drawTrail(const Player& player, int skipRecent) const {
    if (!player.alive) return;
    float halfSize = config.TRAIL_SIZE / 2.0f;
    for (size_t i = 0; i < player.trail.size() - skipRecent; ++i) {
        const auto& pos = player.trail[i];
        drawSquare(pos.x - halfSize, pos.y - halfSize, config.TRAIL_SIZE, player.color);
    }
}

void RenderManager::drawCollectibleBlackSquare(const Collectible& collectible) const {
    drawSquare(collectible.pos.x - collectible.blackSquareSize / 2, collectible.pos.y - collectible.blackSquareSize / 2, collectible.blackSquareSize, {0, 0, 0, 255});
}

void RenderManager::drawCollectibleBlackCircle(const Collectible& collectible) const {
    drawCircle(collectible.pos.x, collectible.pos.y, collectible.blackCircleSize / 2, {0, 0, 0, 255});
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

    drawCollectibleBlackSquare(game.collectible);
    drawCollectibleBlackCircle(game.collectible);
    drawCollectibleGreenSquare(game.collectible);
    for (const auto& circle : game.circles) {
        drawCircle(circle.pos.x, circle.pos.y, circle.radius, circle.color);
    }
    if (!game.player1.isInvincible) drawTrail(game.player1);
    if (!game.player2.isInvincible) drawTrail(game.player2);
    drawPlayer(game.player1);
    drawPlayer(game.player2);
    if (game.paused) {
        float squareSize = 10.0f;
        std::string scoreText = std::to_string(game.score1) + "-" + std::to_string(game.score2);
        float textWidth = scoreText.size() * squareSize * 6;
        drawText(scoreText, (config.WIDTH - textWidth) / 2, config.HEIGHT / 2 - 25, squareSize, {255, 255, 255, 255});
        drawText("Paused", (config.WIDTH - 6 * squareSize * 6) / 2, config.HEIGHT / 2, squareSize, {255, 255, 255, 255});
    }
}

void RenderManager::renderGameOver(const Game& game) const {
    float currentTimeSec = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();

    // Render explosions to continue effect during game over
    for (const auto& explosion : game.explosions) {
        drawExplosion(explosion, currentTimeSec);
    }

    float squareSize = 10.0f;
    float verticalSpacing = 20.0f;

    std::string winText;
    SDL_Color winColor = {255, 255, 255, 255};
    if (game.score1 >= 100 && game.score2 >= 100) {
        winText = "Draw!";
    } else if (game.score1 >= 100) {
        winText = "Player 1 Wins!";
        winColor = {0, 0, 255, 255};
    } else if (game.score2 >= 100) {
        winText = "Player 2 Wins!";
        winColor = {255, 0, 0, 255};
    }
    float winTextWidth = winText.size() * squareSize * 6;
    float winTextY = config.HEIGHT / 2 - 60;
    drawText(winText, (config.WIDTH - winTextWidth) / 2, winTextY, squareSize, winColor);

    std::string scoreText = std::to_string(game.score1) + "-" + std::to_string(game.score2);
    float scoreTextWidth = scoreText.size() * squareSize * 6;
    float scoreTextY = winTextY + squareSize * 5 + verticalSpacing;
    drawText(scoreText, (config.WIDTH - scoreTextWidth) / 2, scoreTextY, squareSize, {255, 255, 255, 255});

    std::string roundText = "+" + std::to_string(game.roundScore1) + " - +" + std::to_string(game.roundScore2);
    float roundTextWidth = roundText.size() * squareSize * 6;
    float roundTextY = scoreTextY + squareSize * 5 + verticalSpacing;
    drawText(roundText, (config.WIDTH - roundTextWidth) / 2, roundTextY, squareSize, {255, 255, 255, 255});

    int countdown = 5 - static_cast<int>(std::chrono::duration<float>(std::chrono::steady_clock::now() - game.gameOverTime).count());
    if (countdown >= 0) {
        std::string countdownText = std::to_string(std::max(1, countdown));
        float countdownWidth = countdownText.size() * squareSize * 6;
        float countdownY = roundTextY + squareSize * 5 + verticalSpacing;
        drawText(countdownText, (config.WIDTH - countdownWidth) / 2, countdownY, squareSize, {255, 255, 255, 255});
    }
}