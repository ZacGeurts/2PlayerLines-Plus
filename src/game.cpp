#include "game.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <GL/gl.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <memory>
#include "types.h"
#include "render.h"
#include "collectible.h"
#include "collision.h"
#include "explosion.h"
#include "input.h"
#include "circle.h"
#include "player.h"
#include "ai.h"

float dt = 0.0f; // Define the global dt variable

// Constructor: Initialize game with given configuration
Game::Game(const GameConfig& config)
    : SDLaicolor{255, 0, 0}, // red, per game.h
      SDLplayercolor{255, 0, 255}, // magenta, per game.h
      window(nullptr),
      glContext(nullptr),
      config(config),
      audio(config),
      collectibleManager(config),
      collisionManager(config),
      renderManager(config),
      circleManager(config),
      explosionManager(config),
      inputManager(),
      playerManager(new PlayerManager(config)),
      ai(new AI(config, *this)),
      splashTexture{0},
      isSplashScreen{true},
      paused{false},
      controllers{nullptr, nullptr},
      controllerCount{0},
      player1(),
      player2(),
      circles(),
      collectible(),
      explosions(),
      flashes(),
      rng(std::random_device()()),
      lastBoopTime{0.0f},
      score1{0},
      score2{0},
      roundScore1{0},
      roundScore2{0},
      setScore1{0},
      setScore2{0},
      gameOver{false},
      gameOverScreen{false},
      winnerDeclared{false},
      firstFrame{true},
      lastCircleSpawn{std::chrono::steady_clock::now()},
      gameOverTime{std::chrono::steady_clock::now()},
      lastWinnerVoiceTime{0.0f},
      deathTime{0.0f},
      orthoWidth{static_cast<float>(config.WIDTH)},
      orthoHeight{static_cast<float>(config.HEIGHT)},
      frameRendered{false},
      winningScore{0.0f},
      greenSquarePoints{0.0f},
      deathPoints{0.0f},
      collectibleCollectedThisFrame{false},
      pendingCollectibleRespawn{false} {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        throw std::runtime_error("Failed to initialize SDL");
    }

    // Initialize SDL_image
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
        SDL_Quit();
        throw std::runtime_error("Failed to initialize SDL_image");
    }

    // Create window
    window = SDL_CreateWindow("2PlayerLines-Plus", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              config.WIDTH, config.HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        throw std::runtime_error("Failed to create window");
    }

    // Create OpenGL context
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        throw std::runtime_error("Failed to create OpenGL context");
    }

    // Set up OpenGL
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background

    // Initialize controllers
    for (int i = 0; i < SDL_NumJoysticks() && controllerCount < 2; ++i) {
        if (SDL_IsGameController(i)) {
            controllers[controllerCount] = SDL_GameControllerOpen(i);
            if (controllers[controllerCount]) {
                SDL_Log("Controller %d opened: %s", controllerCount, SDL_GameControllerName(controllers[controllerCount]));
                controllerCount++;
            } else {
                SDL_Log("Failed to open controller %d: %s", i, SDL_GetError());
            }
        }
    }
    if (controllerCount == 0 && config.ENABLE_DEBUG) {
        SDL_Log("No controllers detected");
    }

    // Load splash texture
    SDL_Surface* surface = IMG_Load("assets/splash.png"); // Adjust path as needed
    if (!surface) {
        SDL_Log("IMG_Load failed: %s", IMG_GetError());
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        throw std::runtime_error("Failed to load splash.png");
    }

    glGenTextures(1, &splashTexture);
    glBindTexture(GL_TEXTURE_2D, splashTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    SDL_FreeSurface(surface);
}

Game::~Game() {
    if (splashTexture) {
        glDeleteTextures(1, &splashTexture);
    }
    delete ai;
    delete playerManager;
    for (int i = 0; i < controllerCount; ++i) {
        if (controllers[i]) {
            SDL_GameControllerClose(controllers[i]);
        }
    }
    if (glContext) {
        SDL_GL_DeleteContext(glContext);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    IMG_Quit();
    SDL_Quit();
}


// Check collision for a player at the next position
void Game::checkCollision(Player* player, Vec2 nextPos, float currentTimeSec, const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    if (!player->alive || player->willDie) return;

    // Geometric check for green square collection
    if (collectible.active && !collectibleCollectedThisFrame) {
        // Normalize coordinates to ortho space
        Vec2 normalizedPlayerPos = nextPos;
        Vec2 normalizedCollectiblePos = collectible.pos;
        float halfSize = collectible.size / 2.0f;

        bool withinX = normalizedPlayerPos.x >= normalizedCollectiblePos.x - halfSize &&
                       normalizedPlayerPos.x <= normalizedCollectiblePos.x + halfSize;
        bool withinY = normalizedPlayerPos.y >= normalizedCollectiblePos.y - halfSize &&
                       normalizedPlayerPos.y <= normalizedCollectiblePos.y + halfSize;

        if (withinX && withinY) {
            handleGreenSquareCollection(player, currentTimeSec);
        } else if (config.ENABLE_DEBUG) {
            SDL_Log("Green square check failed for player %s: playerPos=(%f, %f), collectible=(%f, %f, size=%f), withinX=%d, withinY=%d, active=%d, drawable=(%d, %d)",
                    player == &player1 ? "1" : "2", normalizedPlayerPos.x, normalizedPlayerPos.y,
                    normalizedCollectiblePos.x, normalizedCollectiblePos.y, collectible.size,
                    withinX, withinY, collectible.active, drawableWidth, drawableHeight);
        }
    }

    // Framebuffer-based collision check - black green magenta safe colors
    Vec2 checkPos = nextPos + player->direction * (config.PLAYER_SIZE / 2.0f);
    checkPos.x = std::max(10.0f, std::min(checkPos.x, orthoWidth - 10.0f));
    checkPos.y = std::max(10.0f, std::min(checkPos.y, orthoHeight - 10.0f));

    float x_read = (checkPos.x / orthoWidth) * drawableWidth;
    float y_read = (1 - checkPos.y / orthoHeight) * drawableHeight;

    x_read = std::max(0.0f, std::min(x_read, static_cast<float>(drawableWidth - 1)));
    y_read = std::max(0.0f, std::min(y_read, static_cast<float>(drawableHeight - 1)));

    int index = (static_cast<int>(y_read) * drawableWidth + static_cast<int>(x_read)) * 3;
    unsigned char r = framebuffer[index];
    unsigned char g = framebuffer[index + 1];
    unsigned char b = framebuffer[index + 2];

    bool isBlack = r == 0 && g == 0 && b == 0;
    bool isGreen = r == 0 && g == 255 && b == 0;
    bool isMagenta = r == 255 && g == 0 && b == 255;

    if (config.ENABLE_DEBUG) {
        SDL_Log("Collision check at gamePos=(%f, %f), screenPos=(%f, %f), color=(%d, %d, %d), isBlack=%d, isGreen=%d, isMagenta=%d, collectible.active=%d",
                checkPos.x, checkPos.y, x_read, y_read, r, g, b, isBlack, isGreen, isMagenta, collectible.active);
    }

    if (player->noCollisionTimer > 0) return;

    if (!isBlack && !isMagenta && !isGreen) {
        player->willDie = true;
        explosions.emplace_back(explosionManager.createExplosion(nextPos, rng, dt, currentTimeSec, SDLexplosioncolor));
        audio.playExplosion(currentTimeSec);
        deathTime = currentTimeSec;
        if (config.ENABLE_DEBUG) {
            SDL_Log("Player %s died at (%f, %f) due to unsafe color (R=%d, G=%d, B=%d), hasMoved=%d",
                    player == &player1 ? "1" : "2", nextPos.x, nextPos.y, r, g, b, player->hasMoved);
        }
    }
}

// Handle collection of green square
void Game::handleGreenSquareCollection(Player* player, float currentTimeSec) {
    if (!player->collectedGreenThisFrame && !collectibleCollectedThisFrame && collectible.active) {
        player->collectedGreenThisFrame = true;
        collectibleCollectedThisFrame = true;
        collectible.active = false; // Disable collectible until respawn
        pendingCollectibleRespawn = true; // Defer respawn to Game::update
        int points = 1; // Green square always awards 1 point
        if (player == &player1) {
            score1 += points;
            roundScore1 += points;
        } else {
            score2 += points;
            roundScore2 += points;
        }
        audio.playBoop(currentTimeSec);
        if (config.ENABLE_DEBUG) {
            SDL_Log("Green square collected by player %s at pos=(%f, %f), time=%f, score1=%d, score2=%d, collectible.active=%d",
                    player == &player1 ? "1" : "2", player->pos.x, player->pos.y, currentTimeSec, score1, score2, collectible.active);
        }
    } else if (config.ENABLE_DEBUG) {
        SDL_Log("Green square collection skipped for player %s at time=%f, already collected=%d, collectible.active=%d",
                player == &player1 ? "1" : "2", currentTimeSec, collectibleCollectedThisFrame, collectible.active);
    }
}

// Handle player death
void Game::handlePlayerDeath(Player* player, float currentTimeSec) {
    if (!player->scoredDeathThisFrame) {
        player->scoredDeathThisFrame = true;
        bool simultaneousDeath = (player1.willDie || !player1.alive) && (player2.willDie || !player2.alive);
        if (!simultaneousDeath) {
            if (player == &player1 && player2.alive) {
                score2 += static_cast<int>(deathPoints);
                roundScore2 += static_cast<int>(deathPoints);
                if (config.ENABLE_DEBUG) {
                    SDL_Log("Player 1 died, score2 += %d, total score2=%d, player1.alive=%d, player2.alive=%d",
                            static_cast<int>(deathPoints), score2, player1.alive, player2.alive);
                }
            } else if (player == &player2 && player1.alive) {
                score1 += static_cast<int>(deathPoints);
                roundScore1 += static_cast<int>(deathPoints);
                if (config.ENABLE_DEBUG) {
                    SDL_Log("Player 2 died, score1 += %d, total score1=%d, player1.alive=%d, player2.alive=%d",
                            static_cast<int>(deathPoints), score1, player1.alive, player2.alive);
                }
            }
        } else if (config.ENABLE_DEBUG) {
            SDL_Log("Simultaneous death detected, no points awarded, player1.alive=%d, player2.alive=%d",
                    player1.alive, player2.alive);
        }
    } else if (config.ENABLE_DEBUG) {
        SDL_Log("Death scoring skipped for player %s at time=%f, already scored this frame",
                player == &player1 ? "1" : "2", currentTimeSec);
    }
}

void Game::run() {
    bool running = true;
    auto lastTime = std::chrono::steady_clock::now();
    while (running) {
        auto currentTime = std::chrono::steady_clock::now();
        dt = std::chrono::duration<float>(currentTime - lastTime).count();
        float currentTimeSec = std::chrono::duration<float>(currentTime.time_since_epoch()).count();
        lastTime = currentTime;

        frameRendered = false;

        running = inputManager.handleInput(controllers, controllerCount, gameOverScreen, isSplashScreen, paused, this);

        if (!isSplashScreen && running) {
            if (!gameOverScreen && !gameOver && !paused && !winnerDeclared) {
                // Read framebuffer
                int drawableWidth, drawableHeight;
                SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
                size_t framebufferSize = static_cast<size_t>(drawableWidth) * drawableHeight * 3;
                if (framebufferSize > std::vector<unsigned char>().max_size()) {
                    SDL_Log("Error: Framebuffer size %zu exceeds max vector size %zu",
                            framebufferSize, std::vector<unsigned char>().max_size());
                    throw std::length_error("Framebuffer too large for vector");
                }
                std::vector<unsigned char> framebuffer(framebufferSize);
                glReadPixels(0, 0, drawableWidth, drawableHeight, GL_RGB, GL_UNSIGNED_BYTE, framebuffer.data());

                // Update players (AI handled in PlayerManager)
                playerManager->updatePlayers(controllers, controllerCount, player1, player2, collectible, explosions, flashes,
                                             score1, score2, roundScore1, roundScore2, rng, dt, currentTimeSec, audio,
                                             collectibleManager, explosionManager, circleManager, circles, lastCircleSpawn, this,
                                             framebuffer, drawableWidth, drawableHeight, SDLplayercolor);

                update(dt, currentTimeSec);
            } else if (gameOverScreen && !winnerDeclared && std::chrono::duration<float>(currentTime - gameOverTime).count() > 5.0f) {
                reset();
            } else if (winnerDeclared && currentTimeSec - lastWinnerVoiceTime >= config.WINNER_VOICE_DURATION) {
                audio.playWinnerVoice(currentTimeSec);
                lastWinnerVoiceTime = currentTimeSec;
            }
        }

        render();
        SDL_GL_SwapWindow(window);
        frameRendered = true;
    }
}

void Game::update(float dt, float currentTimeSec) {
    // Reset per-frame flags
    player1.collectedGreenThisFrame = false;
    player2.collectedGreenThisFrame = false;
    player1.scoredDeathThisFrame = false;
    player2.scoredDeathThisFrame = false;
    collectibleCollectedThisFrame = false;

    // Update AI noCollisionTimer
    if (ai->getMode() && player2.noCollisionTimer > 0) {
        player2.noCollisionTimer -= dt;
        if (player2.noCollisionTimer <= 0) {
            player2.noCollisionTimer = 0;
            player2.isInvincible = false;
            player2.canUseNoCollision = true;
            player2.endFlash = std::make_unique<Flash>(
                explosionManager.createFlash(player2.pos, rng, dt, currentTimeSec, {255, 0, 255, 255}));
            audio.playLaserZap(currentTimeSec);
            if (config.ENABLE_DEBUG) {
                SDL_Log("AI no-collision ended at time %f, canUseNoCollision=%d",
                        currentTimeSec, player2.canUseNoCollision);
            }
        }
    }

    // Handle deferred collectible respawn
    if (pendingCollectibleRespawn) {
        collectible = collectibleManager.spawnCollectible(rng, *this);
        collectible.active = true;
        pendingCollectibleRespawn = false;
        if (config.ENABLE_DEBUG) {
            SDL_Log("Deferred collectible respawned at pos=(%f, %f), active=%d",
                    collectible.pos.x, collectible.pos.y, collectible.active);
        }
    }

    // Update circles (handled by CircleManager)
    circleManager.updateCircles(dt, circles, rng, currentTimeSec, lastCircleSpawn, *this);

    // Clear trails under circles
    circleManager.clearTrails(circles, player1, player2);

    if (score1 >= config.WINNING_SCORE) {
        setScore1 += 1;
        winnerDeclared = true;
        gameOverScreen = true;
        audio.playWinnerVoice(currentTimeSec);
        lastWinnerVoiceTime = currentTimeSec;
        score1 = score2 = 0;
    } else if (score2 >= config.WINNING_SCORE) {
        setScore2 += 1;
        winnerDeclared = true;
        gameOverScreen = true;
        audio.playWinnerVoice(currentTimeSec);
        lastWinnerVoiceTime = currentTimeSec;
        score1 = score2 = 0;
    } else if (!player1.alive || !player2.alive) {
        gameOver = true;
        gameOverScreen = true;
        gameOverTime = std::chrono::steady_clock::now();
    }
}

void Game::render() {
    glClear(GL_COLOR_BUFFER_BIT);
    int drawableWidth, drawableHeight;
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
    glViewport(0, 0, drawableWidth, drawableHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, orthoWidth, orthoHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (config.ENABLE_DEBUG) {
        SDL_Log("Rendering: orthoWidth=%f, orthoHeight=%f, drawableWidth=%d, drawableHeight=%d",
                orthoWidth, orthoHeight, drawableWidth, drawableHeight);
    }

    if (isSplashScreen) {
        if (splashTexture == 0 && config.ENABLE_DEBUG) {
            SDL_Log("Warning: splashTexture is 0, cannot render splash screen");
        }
        renderManager.renderSplashScreen(splashTexture);
    } else if (gameOverScreen) {
        renderManager.renderGameOver(*this, orthoWidth, orthoHeight);
    } else {
        float currentTimeSec = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
        renderManager.renderGame(*this, currentTimeSec);
    }

    frameRendered = true;
}

// Reset game state
void Game::reset() {
    player1 = Player{
        Vec2(200, orthoHeight / 2),      // pos
        Vec2(1, 0),                      // direction
        SDL_Color{0, 0, 255, 255},      // color
        {},                              // trail
        true,                            // alive
        false,                           // willDie
        false,                           // hasMoved
        Vec2(0, 0),                      // deathPos
        0.0f,                            // noCollisionTimer
        true,                            // canUseNoCollision
        false,                           // isInvincible
        false,                           // collectedGreenThisFrame
        false,                           // scoredDeathThisFrame
        0.0f,                            // spawnInvincibilityTimer
        nullptr,                         // endFlash
        false                            // hitOpponentHead
    };
    player2 = Player{
        Vec2(orthoWidth - 200, orthoHeight / 2), // pos
        Vec2(-1, 0),                      // direction
        SDL_Color{255, 0, 0, 255},       // color
        {},                              // trail
        true,                            // alive
        false,                           // willDie
        false,                           // hasMoved
        Vec2(0, 0),                      // deathPos
        0.0f,                            // noCollisionTimer
        true,                            // canUseNoCollision
        false,                           // isInvincible
        false,                           // collectedGreenThisFrame
        false,                           // scoredDeathThisFrame
        0.0f,                            // spawnInvincibilityTimer
        nullptr,                         // endFlash
        false                            // hitOpponentHead
    };

    player1.trail.clear();
    player2.trail.clear();

    circles.clear();
    circleManager.spawnInitialCircle(rng, circles, *this);
    collectible = collectibleManager.spawnCollectible(rng, *this);
    collectible.active = true; // Ensure collectible is active
    explosions.clear();
    flashes.clear();
    ai->resetFlash();

    roundScore1 = roundScore2 = 0;
    if (winnerDeclared) {
        score1 = score2 = 0;
    }
    gameOver = gameOverScreen = winnerDeclared = paused = false;
    lastCircleSpawn = std::chrono::steady_clock::now();
    lastBoopTime = 0.0f;
    deathTime = 0.0f;
    lastWinnerVoiceTime = 0.0f;
    frameRendered = false;
    collectibleCollectedThisFrame = false;
    pendingCollectibleRespawn = false;

    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();
    if (config.ENABLE_DEBUG) {
        SDL_Log("Game reset, new collectible pos=(%f, %f), size=%f, active=%d",
                collectible.pos.x, collectible.pos.y, collectible.size, collectible.active);
    }
}

// Respawn circles to match current ortho dimensions
void Game::respawnCircles() {
    circles.clear();
    circleManager.spawnInitialCircle(rng, circles, *this);
}

// Activate no-collision mode for a player
void Game::activateNoCollision(Player* player, float currentTimeSec) {
    if (player->canUseNoCollision && player->noCollisionTimer <= 0 && player->alive) {
        player->noCollisionTimer = 2.0f; // 2 seconds
        player->canUseNoCollision = false; // you used it
        player->isInvincible = true;
        flashes.emplace_back(explosionManager.createFlash(player->pos, rng, dt, currentTimeSec, SDLexplosioncolor));
        audio.playLaserZap(currentTimeSec);
        if (config.ENABLE_DEBUG) {
            SDL_Log("Player flash activated at time %f, noCollisionTimer=%f", currentTimeSec, player->noCollisionTimer);
        }
    }
}

// Toggle between fullscreen and windowed mode
void Game::toggleFullscreen() {
    Uint32 fullscreenFlag = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    int drawableWidth, drawableHeight;

    if (fullscreenFlag) {
        if (SDL_SetWindowFullscreen(window, 0) < 0) {
            if (config.ENABLE_DEBUG) SDL_Log("Failed to switch to windowed mode: %s", SDL_GetError());
            return;
        }
        SDL_SetWindowSize(window, config.WIDTH, config.HEIGHT);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
    } else {
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
            if (config.ENABLE_DEBUG) SDL_Log("Failed to switch to fullscreen mode: %s", SDL_GetError());
            return;
        }
        SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
    }

    glViewport(0, 0, drawableWidth, drawableHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, orthoWidth, orthoHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (config.ENABLE_DEBUG) {
        SDL_Log("Fullscreen toggle: orthoWidth=%f, orthoHeight=%f, drawableWidth=%d, drawableHeight=%d",
                orthoWidth, orthoHeight, drawableWidth, drawableHeight);
    }

    collectible = collectibleManager.spawnCollectible(rng, *this);
    respawnCircles();
}

// Resume game after a winner is declared
void Game::resumeAfterWinner() {
    winnerDeclared = false;
    gameOverScreen = false;
    reset();
}