#include "game.h"
#include "ai.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <random>
#include <chrono>
#include <vector>
#include <algorithm>

namespace Game {

Game::Game(const GameConfig& config)
    : config(config),
      audioManager(config),
      playerColor{255, 0, 255, 255}, // magenta
      aiColor{255, 0, 0, 255}, // red
      explosionColor{255, 255, 0, 255}, // yellow
      window(nullptr),
      glContext(nullptr),
      ai(std::make_unique<AI>(config, *this)),
      splashTexture(0),
      isSplashScreen(true),
      paused(false),
      controllerCount(0),
      player1{ // Explicit initialization
          .pos = {config.WIDTH / 4.0f, config.HEIGHT / 2.0f},
          .direction = {1.0f, 0.0f},
          .color = {255, 0, 255, 255},
          .trail = {},
          .alive = true,
          .willDie = false,
          .hasMoved = false,
          .deathPos = {0.0f, 0.0f},
          .noCollisionTimer = 0.0f,
          .canUseNoCollision = true,
          .isInvincible = false,
          .collectedGreenThisFrame = false,
          .scoredDeathThisFrame = false,
          .spawnInvincibilityTimer = 0.0f,
          .endFlash = nullptr,
          .hitOpponentHead = false
      },
      player2{ // Explicit initialization
          .pos = {3 * config.WIDTH / 4.0f, config.HEIGHT / 2.0f},
          .direction = {-1.0f, 0.0f},
          .color = {255, 0, 0, 255},
          .trail = {},
          .alive = true,
          .willDie = false,
          .hasMoved = false,
          .deathPos = {0.0f, 0.0f},
          .noCollisionTimer = 0.0f,
          .canUseNoCollision = true,
          .isInvincible = false,
          .collectedGreenThisFrame = false,
          .scoredDeathThisFrame = false,
          .spawnInvincibilityTimer = 0.0f,
          .endFlash = nullptr,
          .hitOpponentHead = false
      },
      collectible{{config.WIDTH / 2.0f, config.HEIGHT / 2.0f}, config.COLLECTIBLE_SIZE, true, {0, 255, 0, 255}},
      rng(std::random_device{}()),
      lastBoopTime(0.0f),
      score1(0),
      score2(0),
      roundScore1(0),
      roundScore2(0),
      setScore1(0),
      setScore2(0),
      gameOver(false),
      gameOverScreen(false),
      winnerDeclared(false),
      firstFrame(true),
      lastCircleSpawn(std::chrono::steady_clock::now()),
      gameOverTime(std::chrono::steady_clock::now()),
      lastWinnerVoiceTime(0.0f),
      deathTime(0.0f),
      orthoWidth(config.WIDTH),
      orthoHeight(config.HEIGHT),
      frameRendered(false),
      winningScore(config.WINNING_SCORE),
      greenSquarePoints(config.GREEN_SQUARE_POINTS),
      deathPoints(config.DEATH_POINTS),
      collectibleCollectedThisFrame(false),
      pendingCollectibleRespawn(false) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              config.WIDTH, config.HEIGHT, SDL_WINDOW_OPENGL);
    glContext = SDL_GL_CreateContext(window);
    glOrtho(0, config.WIDTH, 0, config.HEIGHT, -1, 1);
    controllers[0] = controllers[1] = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i) && controllerCount < 2) {
            controllers[controllerCount++] = SDL_GameControllerOpen(i);
        }
    }
    audioManager.startBackgroundMusic();
    reset();
}

// Rest of game.cpp remains unchanged (destructor, run, handleInput, update, render, etc.)
// Include all methods from your provided game.cpp:
Game::~Game() {
    audioManager.stopBackgroundMusic();
    for (int i = 0; i < controllerCount; ++i) {
        if (controllers[i]) SDL_GameControllerClose(controllers[i]);
    }
    if (glContext) SDL_GL_DeleteContext(glContext);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::run() {
    bool running = true;
    auto lastTime = std::chrono::steady_clock::now();
    while (running) {
        auto currentTime = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        dt = std::min(dt, 0.01667f); // Cap at ~60 FPS
        float currentTimeSec = std::chrono::duration<float>(currentTime.time_since_epoch()).count();
        lastTime = currentTime;

        running = handleInput();
        if (!paused && !gameOverScreen && !isSplashScreen) {
            update(dt, currentTimeSec);
        }
        render(currentTimeSec);
        SDL_GL_SwapWindow(window);
    }
}

bool Game::handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return false;
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) return false;
            if (event.key.keysym.sym == SDLK_p) paused = !paused;
            if (event.key.keysym.sym == SDLK_f) toggleFullscreen();
            if (event.key.keysym.sym == SDLK_r && (gameOverScreen || winnerDeclared)) {
                reset();
                resumeAfterWinner();
            }
        }
        if (event.type == SDL_CONTROLLERBUTTONDOWN && event.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
            paused = !paused;
        }
    }
    return true;
}

void Game::update(float dt, float currentTimeSec) {
    if (firstFrame) {
        firstFrame = false;
        return;
    }

    if (gameOver) {
        if (std::chrono::duration<float>(std::chrono::steady_clock::now() - gameOverTime).count() > 3.0f) {
            gameOverScreen = true;
        }
        return;
    }

    if (player1.alive) {
        Vec2 nextPos = player1.pos + player1.direction * config.PLAYER_SPEED * dt;
        checkCollision(&player1, nextPos, currentTimeSec, {}, config.WIDTH, config.HEIGHT, dt);
        if (!player1.willDie) player1.pos = nextPos;
        player1.trail.push_back(player1.pos);
    }
    if (player2.alive) {
        ai->startUpdate(player2, player1, collectible, circles, dt, rng, {}, config.WIDTH, config.HEIGHT, aiColor);
        ai->waitForUpdate();
        ai->applyUpdate(player2);
    }

    if (collectible.active) {
        handleGreenSquareCollection(&player1, currentTimeSec);
        handleGreenSquareCollection(&player2, currentTimeSec);
    }
    if (pendingCollectibleRespawn) {
        auto elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - lastCircleSpawn).count();
        if (elapsed > config.COLLECT_COOLDOWN) {
            collectible = spawnCollectible(rng);
            pendingCollectibleRespawn = false;
        }
    }

    updateCircles(dt, currentTimeSec);

    if (score1 >= winningScore || score2 >= winningScore) {
        gameOver = true;
        gameOverTime = std::chrono::steady_clock::now();
        winnerDeclared = true;
        audioManager.playWinnerVoice(currentTimeSec);
    }
}

void Game::render(float currentTimeSec) {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, config.WIDTH, 0, config.HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (isSplashScreen) {
        glColor3f(1.0f, 1.0f, 1.0f);
        isSplashScreen = false;
    } else if (gameOverScreen) {
        glColor3f(1.0f, 1.0f, 1.0f);
    } else {
        glColor3ub(player1.color.r, player1.color.g, player1.color.b);
        glBegin(GL_QUADS);
        glVertex2f(player1.pos.x - 5, player1.pos.y - 5);
        glVertex2f(player1.pos.x + 5, player1.pos.y - 5);
        glVertex2f(player1.pos.x + 5, player1.pos.y + 5);
        glVertex2f(player1.pos.x - 5, player1.pos.y + 5);
        glEnd();
        glColor3ub(player2.color.r, player2.color.g, player2.color.b);
        glBegin(GL_QUADS);
        glVertex2f(player2.pos.x - 5, player2.pos.y - 5);
        glVertex2f(player2.pos.x + 5, player2.pos.y - 5);
        glVertex2f(player2.pos.x + 5, player2.pos.y + 5);
        glVertex2f(player2.pos.x - 5, player2.pos.y + 5);
        glEnd();

        if (collectible.active) {
            glColor3ub(collectible.color.r, collectible.color.g, collectible.color.b);
            glBegin(GL_QUADS);
            float halfSize = collectible.size / 2;
            glVertex2f(collectible.pos.x - halfSize, collectible.pos.y - halfSize);
            glVertex2f(collectible.pos.x + halfSize, collectible.pos.y - halfSize);
            glVertex2f(collectible.pos.x + halfSize, collectible.pos.y + halfSize);
            glVertex2f(collectible.pos.x - halfSize, collectible.pos.y + halfSize);
            glEnd();
        }

        for (const auto& circle : circles) {
            glColor3ub(circle.color.r, circle.color.g, circle.color.b);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(circle.pos.x, circle.pos.y);
            for (int i = 0; i <= 20; ++i) {
                float angle = i * 2 * M_PI / 20;
                glVertex2f(circle.pos.x + circle.radius * cos(angle),
                           circle.pos.y + circle.radius * sin(angle));
            }
            glEnd();
        }

        for (const auto& explosion : explosions) {
            glColor3ub(explosion.color.r, explosion.color.g, explosion.color.b);
            glBegin(GL_POINTS);
            for (const auto& particle : explosion.particles) {
                glVertex2f(particle.pos.x, particle.pos.y);
            }
            glEnd();
        }
    }
}

void Game::reset() {
    player1 = {
        .pos = {config.WIDTH / 4.0f, config.HEIGHT / 2.0f},
        .direction = {1.0f, 0.0f},
        .color = playerColor,
        .trail = {},
        .alive = true,
        .willDie = false,
        .hasMoved = false,
        .deathPos = {0.0f, 0.0f},
        .noCollisionTimer = 0.0f,
        .canUseNoCollision = true,
        .isInvincible = false,
        .collectedGreenThisFrame = false,
        .scoredDeathThisFrame = false,
        .spawnInvincibilityTimer = 0.0f,
        .endFlash = nullptr,
        .hitOpponentHead = false
    };
    player2 = {
        .pos = {3 * config.WIDTH / 4.0f, config.HEIGHT / 2.0f},
        .direction = {-1.0f, 0.0f},
        .color = aiColor,
        .trail = {},
        .alive = true,
        .willDie = false,
        .hasMoved = false,
        .deathPos = {0.0f, 0.0f},
        .noCollisionTimer = 0.0f,
        .canUseNoCollision = true,
        .isInvincible = false,
        .collectedGreenThisFrame = false,
        .scoredDeathThisFrame = false,
        .spawnInvincibilityTimer = 0.0f,
        .endFlash = nullptr,
        .hitOpponentHead = false
    };
    circles.clear();
    explosions.clear();
    flashes.clear();
    collectible = spawnCollectible(rng);
    score1 = score2 = roundScore1 = roundScore2 = 0;
    gameOver = gameOverScreen = winnerDeclared = false;
    lastCircleSpawn = std::chrono::steady_clock::now();
    spawnInitialCircle(rng, 0.0f);
}

Collectible Game::spawnCollectible(std::mt19937& rng) {
    std::uniform_real_distribution<float> distX(config.AI_BERTH, config.WIDTH - config.AI_BERTH);
    std::uniform_real_distribution<float> distY(config.AI_BERTH, config.HEIGHT - config.AI_BERTH);
    return {{distX(rng), distY(rng)}, config.COLLECTIBLE_SIZE, true, {0, 255, 0, 255}};
}

void Game::spawnInitialCircle(std::mt19937& rng, float currentTimeSec) {
    std::uniform_real_distribution<float> distX(config.AI_BERTH, config.WIDTH - config.AI_BERTH);
    std::uniform_real_distribution<float> distY(config.AI_BERTH, config.HEIGHT - config.AI_BERTH);
    std::uniform_real_distribution<float> distVel(-config.CIRCLE_SPEED, config.CIRCLE_SPEED);
    circles.emplace_back(Vec2{distX(rng), distY(rng)}, Vec2{distVel(rng), distVel(rng)},
                        Vec2{}, config.CIRCLE_RADIUS, SDL_Color{255, 255, 255, 255}, 0.0f, false);
}

void Game::updateCircles(float dt, float currentTimeSec) {
    for (auto& circle : circles) {
        circle.prevPos = circle.pos;
        circle.pos += circle.vel * dt;
        if (circle.pos.x < config.AI_BERTH || circle.pos.x > config.WIDTH - config.AI_BERTH) {
            circle.vel.x = -circle.vel.x;
            circle.pos.x = std::clamp(circle.pos.x, config.AI_BERTH, config.WIDTH - config.AI_BERTH);
        }
        if (circle.pos.y < config.AI_BERTH || circle.pos.y > config.HEIGHT - config.AI_BERTH) {
            circle.vel.y = -circle.vel.y;
            circle.pos.y = std::clamp(circle.pos.y, config.AI_BERTH, config.HEIGHT - config.AI_BERTH);
        }
    }

    auto elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - lastCircleSpawn).count();
    if (elapsed > config.CIRCLE_SPAWN_INTERVAL) {
        spawnInitialCircle(rng, currentTimeSec);
        lastCircleSpawn = std::chrono::steady_clock::now();
    }
}

void Game::checkCollision(Player* player, Vec2 nextPos, float currentTimeSec,
                          const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight, float dt) {
    if (!player->alive || player->isInvincible || player->spawnInvincibilityTimer > 0) return;

    for (const auto& circle : circles) {
        if ((nextPos - circle.pos).magnitude() < config.CIRCLE_RADIUS + config.PLAYER_SIZE) {
            player->willDie = true;
            player->deathPos = nextPos;
            explosions.push_back(Explosion{{}, currentTimeSec, explosionColor});
            audioManager.playExplosion(currentTimeSec);
            return;
        }
    }

    player->pos = nextPos;
}

void Game::handleGreenSquareCollection(Player* player, float currentTimeSec) {
    if (!collectible.active || !player->alive) return;

    if (std::abs(player->pos.x - collectible.pos.x) <= config.COLLECTIBLE_SIZE / 2 &&
        std::abs(player->pos.y - collectible.pos.y) <= config.COLLECTIBLE_SIZE / 2) {
        player->collectedGreenThisFrame = true;
        if (player == &player1) {
            score1 += greenSquarePoints;
            roundScore1 += greenSquarePoints;
        } else {
            score2 += greenSquarePoints;
            roundScore2 += greenSquarePoints;
        }
        collectible.active = false;
        collectibleCollectedThisFrame = true;
        pendingCollectibleRespawn = true;
        audioManager.playBoop(currentTimeSec);
        lastCircleSpawn = std::chrono::steady_clock::now();
    }
}

void Game::handlePlayerDeath(Player* player, float currentTimeSec) {
    if (!player->willDie) return;

    player->alive = false;
    if (player == &player1) {
        score2 += deathPoints;
        roundScore2 += deathPoints;
    } else {
        score1 += deathPoints;
        roundScore1 += deathPoints;
    }
    audioManager.playExplosion(currentTimeSec);
}

void Game::respawnCircles(float currentTimeSec) {
    circles.clear();
    spawnInitialCircle(rng, currentTimeSec);
}

void Game::activateNoCollision(Player* player, float currentTimeSec, float dt) {
    if (!player->canUseNoCollision || player->isInvincible) return;

    player->canUseNoCollision = false;
    player->noCollisionTimer = config.INVINCIBILITY_DURATION;
    flashes.emplace_back(Flash{{}, currentTimeSec, {255, 0, 255, 255}, config.EXPLOSION_MAX_RADIUS, config.LASER_ZAP_DURATION});
    audioManager.playLaserZap(currentTimeSec);
}

void Game::toggleFullscreen() {
    Uint32 flags = SDL_GetWindowFlags(window);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(window, 0);
    } else {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    collectible = spawnCollectible(rng);
}

void Game::resumeAfterWinner() {
    gameOver = false;
    winnerDeclared = false;
    reset();
}

} // namespace Game