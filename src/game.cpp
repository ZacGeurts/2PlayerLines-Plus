#include "game.h"
#include "constants.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <GL/gl.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <stdexcept>
#include "render.h"
#include "collectible.h"
#include "collision.h"
#include "explosion.h"
#include "input.h"
#include "circle.h"
#include "player.h"

Game::Game(const GameConfig& config) : window(nullptr), glContext(nullptr), audio(config), collectibleManager(config), collisionManager(config), renderManager(config), circleManager(config), explosionManager(config), inputManager(), playerManager(config), splashTexture(0), isSplashScreen(true), paused(false), controllerCount(0), player1(), player2(), circles(), collectible(), explosions(), flashes(), rng(), lastBoopTime(0.0f), score1(0), score2(0), roundScore1(0), roundScore2(0), gameOver(false), gameOverScreen(false), firstFrame(true), lastCircleSpawn(), gameOverTime(), config(config), deathTime(0.0f) {
    // Set high-DPI hint
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    // Create window
    window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config.WIDTH, config.HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        throw std::runtime_error("Window creation failed");
    }

    // Create OpenGL context
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_Log("Failed to create GL context: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        throw std::runtime_error("GL context creation failed");
    }
    SDL_GL_SetSwapInterval(1);

    // Set up OpenGL viewport and projection
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, config.WIDTH, config.HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Load splash screen texture
    SDL_Surface* splashSurface = IMG_Load("splash.png");
    if (!splashSurface) {
        SDL_Log("Failed to load splash.png: %s", IMG_GetError());
    } else {
        glGenTextures(1, &splashTexture);
        glBindTexture(GL_TEXTURE_2D, splashTexture);
        GLenum format = (splashSurface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, splashSurface->w, splashSurface->h, 0, format, GL_UNSIGNED_BYTE, splashSurface->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        SDL_FreeSurface(splashSurface);
    }

    // Initialize game controllers
    for (int i = 0; i < SDL_NumJoysticks() && controllerCount < 2; ++i) {
        if (SDL_IsGameController(i)) {
            controllers[controllerCount] = SDL_GameControllerOpen(i);
            if (controllers[controllerCount]) controllerCount++;
        }
    }

    // Initialize random number generator
    std::random_device rd;
    rng = std::mt19937(rd());

    // Initialize players
    player1.pos = Vec2(200, config.HEIGHT / 2);
    player1.direction = Vec2(1, 0);
    player1.color = {0, 0, 255, 255};
    player1.alive = true;
    player1.willDie = false;
    player1.hasMoved = false;
    player1.deathPos = Vec2(0, 0);
    player1.noCollisionTimer = 0.0f;
    player1.canUseNoCollision = true;
    player1.isInvincible = false;
    player1.endFlash = nullptr;

    player2.pos = Vec2(config.WIDTH - 200, config.HEIGHT / 2);
    player2.direction = Vec2(-1, 0);
    player2.color = {255, 0, 0, 255};
    player2.alive = true;
    player2.willDie = false;
    player2.hasMoved = false;
    player2.deathPos = Vec2(0, 0);
    player2.noCollisionTimer = 0.0f;
    player2.canUseNoCollision = true;
    player2.isInvincible = false;
    player2.endFlash = nullptr;

    // Initialize game state
    circleManager.spawnInitialCircle(rng, circles);
    collectible = collectibleManager.spawnCollectible(rng);
    lastBoopTime = 0.0f;
    score1 = score2 = roundScore1 = roundScore2 = 0;
    gameOver = gameOverScreen = false;
    firstFrame = true;
    lastCircleSpawn = std::chrono::steady_clock::now();
    gameOverTime = lastCircleSpawn;
}

Game::~Game() {
    if (splashTexture) glDeleteTextures(1, &splashTexture);
    if (player1.endFlash) delete player1.endFlash;
    if (player2.endFlash) delete player2.endFlash;
    for (auto& controller : controllers) if (controller) SDL_GameControllerClose(controller);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
}

void Game::checkCollision(Player* player, Vec2 nextPos, float currentTimeSec) {
    if (player->noCollisionTimer > 0) return;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    GLdouble projection[16], modelview[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, config.WIDTH, config.HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, config.WIDTH, config.HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    renderManager.drawTrail(player1, player == &player1 ? 10 : 0);
    renderManager.drawTrail(player2, player == &player2 ? 10 : 0);
    for (const auto& circle : circles) {
        renderManager.drawCircle(circle.pos.x, circle.pos.y, circle.radius, circle.color);
    }
    glFinish();

    if (collisionManager.checkAreaCollision(nextPos, config.COLLISION_CHECK_SIZE)) {
        explosions.emplace_back(explosionManager.createExplosion(nextPos, rng, currentTimeSec));
        audio.playExplosion(currentTimeSec);
        player->willDie = true;
        deathTime = currentTimeSec; // Track death time
        SDL_Log("Player %s died at (%f, %f), explosion triggered", player == &player1 ? "1" : "2", nextPos.x, nextPos.y);
    }

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projection);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(modelview);
}

void Game::run() {
    bool running = true;
    auto lastTime = std::chrono::steady_clock::now();
    while (running) {
        auto currentTime = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        float currentTimeSec = std::chrono::duration<float>(currentTime.time_since_epoch()).count();
        lastTime = currentTime;

        running = inputManager.handleInput(controllers, controllerCount, gameOverScreen, isSplashScreen, paused, this);
        if (!isSplashScreen) {
            if (!gameOverScreen && !gameOver && !paused) {
                update(dt, currentTimeSec);
            } else if (gameOverScreen && std::chrono::duration<float>(currentTime - gameOverTime).count() > 5.0f) {
                reset();
            }
        }

        render();
        SDL_GL_SwapWindow(window);
    }
}

void Game::update(float dt, float currentTimeSec) {
    playerManager.updatePlayers(controllers, controllerCount, player1, player2, collectible, explosions, flashes, score1, score2, roundScore1, roundScore2, rng, dt, currentTimeSec, audio, collectibleManager, explosionManager, circleManager, circles, lastCircleSpawn, this);

    if (score1 >= 100 || score2 >= 100) {
        gameOver = true;
        gameOverScreen = true;
        gameOverTime = std::chrono::steady_clock::now();
        SDL_Log("Game over: score1=%d, score2=%d", score1, score2);
    } else if (!player1.alive || !player2.alive) {
        gameOver = true;
        gameOverScreen = true;
        gameOverTime = std::chrono::steady_clock::now();
        SDL_Log("Game over: player1.alive=%d, player2.alive=%d", player1.alive, player2.alive);
    }
}

void Game::render() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (isSplashScreen) {
        renderManager.renderSplashScreen(splashTexture);
    } else if (gameOverScreen) {
        renderManager.renderGameOver(*this);
    } else {
        float currentTimeSec = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
        renderManager.renderGame(*this, currentTimeSec);
    }
}

void Game::reset() {
    std::uniform_real_distribution<float> distX(50, config.WIDTH - 50);
    std::uniform_real_distribution<float> distY(50, config.HEIGHT - 50);
    player1 = Player{Vec2(200, config.HEIGHT / 2), Vec2(1, 0), {0, 0, 255, 255}, {}, true, false, false, Vec2(0, 0), 0.0f, true, false, nullptr};
    player2 = Player{Vec2(config.WIDTH - 200, config.HEIGHT / 2), Vec2(-1, 0), {255, 0, 0, 255}, {}, true, false, false, Vec2(0, 0), 0.0f, true, false, nullptr};
    circles.clear();
    circleManager.spawnInitialCircle(rng, circles);
    collectible = collectibleManager.spawnCollectible(rng);
    explosions.clear();
    flashes.clear();
    roundScore1 = roundScore2 = 0;
    gameOver = gameOverScreen = false;
    paused = false;
    lastCircleSpawn = std::chrono::steady_clock::now();
    lastBoopTime = 0.0f;
    deathTime = 0.0f;
    SDL_Log("Game reset");
}

void Game::activateNoCollision(Player* player, float currentTimeSec) {
    if (player->canUseNoCollision && player->noCollisionTimer <= 0 && player->alive) {
        player->noCollisionTimer = 2.0f;
        player->canUseNoCollision = false;
        player->isInvincible = true;
        flashes.emplace_back(explosionManager.createFlash(player->pos, rng, currentTimeSec, {255, 255, 255, 255}));
        audio.playLaserZap(currentTimeSec);
        SDL_Log("No-collision activated for player at (%f, %f), white flash triggered", player->pos.x, player->pos.y);
    }
}

void Game::toggleFullscreen() {
    Uint32 fullscreenFlag = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (fullscreenFlag) {
        if (SDL_SetWindowFullscreen(window, 0) < 0) {
            SDL_Log("Failed to switch to windowed mode: %s", SDL_GetError());
        }
        SDL_SetWindowSize(window, config.WIDTH, config.HEIGHT);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        glViewport(0, 0, config.WIDTH, config.HEIGHT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, config.WIDTH, config.HEIGHT, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        SDL_Log("Switched to windowed mode: %dx%d", config.WIDTH, config.HEIGHT);
    } else {
        SDL_DisplayMode displayMode;
        if (SDL_GetCurrentDisplayMode(0, &displayMode) < 0) {
            SDL_Log("Failed to get display mode: %s", SDL_GetError());
            displayMode.w = config.WIDTH;
            displayMode.h = config.HEIGHT;
        }
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
            SDL_Log("Failed to switch to fullscreen mode: %s", SDL_GetError());
            return;
        }

        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        int drawableWidth, drawableHeight;
        SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0) {
            SDL_Log("Failed to get display DPI: %s", SDL_GetError());
            ddpi = 96.0f;
        }
        float scaleFactor = ddpi / 96.0f;
        SDL_Log("Display mode: %dx%d, Drawable size: %dx%d, Window size: %dx%d, DPI: %f, Scale factor: %f",
                displayMode.w, displayMode.h, drawableWidth, drawableHeight, windowWidth, windowHeight, ddpi, scaleFactor);

        glViewport(0, 0, windowWidth, windowHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float gameAspect = static_cast<float>(config.WIDTH) / config.HEIGHT;
        float windowAspect = static_cast<float>(windowWidth) / windowHeight;
        float orthoWidth = config.WIDTH;
        float orthoHeight = config.HEIGHT;
        if (windowAspect > gameAspect) {
            orthoWidth = config.HEIGHT * windowAspect;
        } else {
            orthoHeight = config.WIDTH / windowAspect;
        }
        glOrtho(0, orthoWidth, orthoHeight, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        SDL_Log("Fullscreen viewport: 0,0,%dx%d, Ortho: 0,0,%fx%f", windowWidth, windowHeight, orthoWidth, orthoHeight);
    }
}