#include "game.h"
#include "constants.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <GL/gl.h>
#include <cmath>
#include <algorithm>
#include <string>
#include "render.h"
#include "collectible.h"
#include "explosion.h"
#include "audio.h"
#include "input.h"

Game::Game() : window(nullptr), glContext(nullptr), explosionAudioDevice(0), boopAudioDevice(0), boopPlaying(false), controllerCount(0), isSplashScreen(true), splashTexture(0) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);

    glViewport(0, 0, WIDTH, HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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

    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 4096;
    desired.callback = explosionAudioCallback;
    explosionAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (explosionAudioDevice == 0) {
        SDL_Log("Failed to open explosion audio: %s", SDL_GetError());
    }
    SDL_PauseAudioDevice(explosionAudioDevice, 1);

    SDL_AudioSpec boopDesired, boopObtained;
    SDL_zero(boopDesired);
    boopDesired.freq = 44100;
    boopDesired.format = AUDIO_S16SYS;
    boopDesired.channels = 1;
    boopDesired.samples = 4096;
    boopDesired.callback = boopAudioCallback;
    boopDesired.userdata = &boopData;
    boopAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &boopDesired, &boopObtained, 0);
    if (boopAudioDevice == 0) {
        SDL_Log("Failed to open boop audio: %s", SDL_GetError());
    } else {
        boopData.deviceId = boopAudioDevice;
        boopData.playing = &boopPlaying;
    }
    SDL_PauseAudioDevice(boopAudioDevice, 1);

    for (int i = 0; i < SDL_NumJoysticks() && controllerCount < 2; ++i) {
        if (SDL_IsGameController(i)) {
            controllers[controllerCount] = SDL_GameControllerOpen(i);
            if (controllers[controllerCount]) controllerCount++;
        }
    }

    std::random_device rd;
    rng = std::mt19937(rd());
    std::uniform_real_distribution<float> distX(50, WIDTH - 50);
    std::uniform_real_distribution<float> distY(50, HEIGHT - 50);

    player1.pos = Vec2(200, HEIGHT / 2);
    player1.direction = Vec2(1, 0);
    player1.color = {0, 0, 255, 255};
    player1.alive = true;
    player1.willDie = false;
    player1.hasMoved = false;
    player1.deathPos = Vec2(0, 0);

    player2.pos = Vec2(WIDTH - 200, HEIGHT / 2);
    player2.direction = Vec2(-1, 0);
    player2.color = {255, 0, 0, 255};
    player2.alive = true;
    player2.willDie = false;
    player2.hasMoved = false;
    player2.deathPos = Vec2(0, 0);

    float angle = std::uniform_real_distribution<float>(0, 2 * M_PI)(rng);
    circles.push_back(Circle{Vec2(distX(rng), distY(rng)), Vec2(CIRCLE_SPEED * cos(angle), CIRCLE_SPEED * sin(angle)), CIRCLE_RADIUS});

    collectible = spawnCollectible(rng);
    lastBoopTime = 0.0f;
    score1 = score2 = roundScore1 = roundScore2 = 0;
    gameOver = gameOverScreen = paused = false;
    firstFrame = true;
    lastCircleSpawn = std::chrono::steady_clock::now();
    gameOverTime = lastCircleSpawn;
}

Game::~Game() {
    if (splashTexture) glDeleteTextures(1, &splashTexture);
    SDL_CloseAudioDevice(explosionAudioDevice);
    SDL_CloseAudioDevice(boopAudioDevice);
    for (auto& controller : controllers) if (controller) SDL_GameControllerClose(controller);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void Game::checkCollision(Player* player, Vec2 nextPos, float currentTimeSec) {
    // Save current OpenGL state
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    GLdouble projection[16], modelview[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    // Set up framebuffer for collision detection
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, WIDTH, HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw collision objects
    drawTrail(player1, player == &player1 ? 10 : 0); // Increased offset to 10
    drawTrail(player2, player == &player2 ? 10 : 0);
    for (const auto& circle : circles) {
        drawCircle(circle.pos.x, circle.pos.y, circle.radius, {255, 255, 0, 255});
    }
    glFinish();

    // Check collision
    if (checkAreaCollision(nextPos, COLLISION_CHECK_SIZE)) {
        player->willDie = true;
        explosions.emplace_back(createExplosion(nextPos, rng, currentTimeSec));
        SDL_Log("Player %s died at (%f, %f)", player == &player1 ? "1" : "2", nextPos.x, nextPos.y);
    }

    // Restore OpenGL state
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

        running = handleInput(controllers, controllerCount, gameOverScreen, paused, boopAudioDevice, isSplashScreen, this);
        if (!isSplashScreen) {
            if (!gameOverScreen && !gameOver && !paused) {
                update(dt, currentTimeSec);
            } else if (gameOverScreen && std::chrono::duration<float>(currentTime - gameOverTime).count() > 5.0f) {
                reset();
            }

            if (lastBoopTime > 0.0f && currentTimeSec - lastBoopTime >= BOOP_DURATION) {
                if (SDL_GetAudioDeviceStatus(boopAudioDevice) == SDL_AUDIO_PLAYING) {
                    SDL_PauseAudioDevice(boopAudioDevice, 1);
                    boopPlaying = false;
                }
            }
        }

        render();
        SDL_GL_SwapWindow(window);
    }
}

void Game::update(float dt, float currentTimeSec) {
    for (int i = 0; i < controllerCount; ++i) {
        if (!controllers[i]) continue;
        Player& player = (i == 0) ? player1 : player2;
        if (!player.alive) continue;

        Sint16 leftTrigger = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        Sint16 rightTrigger = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        if (leftTrigger > 0 || rightTrigger > 0) player.hasMoved = true;
        float turn = (rightTrigger - leftTrigger) / 32768.0f * TURN_SPEED * dt;
        float angle = atan2(player.direction.y, player.direction.x) + turn;
        player.direction = Vec2(cos(angle), sin(angle));
    }

    for (auto& player : {&player1, &player2}) {
        if (!player->alive) continue;

        Vec2 nextPos = player->pos + player->direction * PLAYER_SPEED * dt;
        if (!player->willDie) {
            if (nextPos.x < 0 || nextPos.x > WIDTH || nextPos.y < 0 || nextPos.y > HEIGHT) {
                player->willDie = true;
            } else if (player->hasMoved) {
                checkCollision(player, nextPos, currentTimeSec);
            }
        } else {
            player->alive = false;
            player->deathPos = player->pos;
            explosions.emplace_back(createExplosion(player->pos, rng, currentTimeSec));
            continue;
        }

        player->pos = nextPos;
        player->trail.push_back(player->pos);

        if (checkCollectibleCollision(player->pos, collectible)) {
            if (player == &player1) {
                score1++;
                roundScore1++;
                if (!boopPlaying && currentTimeSec - lastBoopTime >= BOOP_DURATION) {
                    boopPlaying = true;
                    SDL_PauseAudioDevice(boopAudioDevice, 0);
                    lastBoopTime = currentTimeSec;
                }
            } else {
                score2++;
                roundScore2++;
                if (!boopPlaying && currentTimeSec - lastBoopTime >= BOOP_DURATION) {
                    boopPlaying = true;
                    SDL_PauseAudioDevice(boopAudioDevice, 0);
                    lastBoopTime = currentTimeSec;
                }
            }
            collectible = spawnCollectible(rng);
        }
    }

    for (auto& circle : circles) {
        circle.pos = circle.pos + circle.vel * dt;
        if (circle.pos.x - circle.radius < 0 || circle.pos.x + circle.radius > WIDTH) {
            circle.vel.x = -circle.vel.x;
            circle.pos.x = std::max(circle.radius, std::min(WIDTH - circle.radius, circle.pos.x));
        }
        if (circle.pos.y - circle.radius < 0 || circle.pos.y + circle.radius > HEIGHT) {
            circle.vel.y = -circle.vel.y;
            circle.pos.y = std::max(circle.radius, std::min(HEIGHT - circle.radius, circle.pos.y));
        }

        for (auto& player : {&player1, &player2}) {
            player->trail.erase(
                std::remove_if(player->trail.begin(), player->trail.end(),
                    [&](const Vec2& p) {
                        float dx = circle.pos.x - p.x, dy = circle.pos.y - p.y;
                        return sqrt(dx * dx + dy * dy) < circle.radius;
                    }),
                player->trail.end());
        }
    }

    if (std::chrono::duration<float>(std::chrono::steady_clock::now() - lastCircleSpawn).count() > 5.0f) {
        float angle = std::uniform_real_distribution<float>(0, 2 * M_PI)(rng);
        std::uniform_real_distribution<float> distX(50, WIDTH - 50);
        std::uniform_real_distribution<float> distY(50, HEIGHT - 50);
        circles.push_back(Circle{Vec2(distX(rng), distY(rng)), Vec2(CIRCLE_SPEED * cos(angle), CIRCLE_SPEED * sin(angle)), CIRCLE_RADIUS});
        lastCircleSpawn = std::chrono::steady_clock::now();
    }

    if (!player1.alive || !player2.alive) {
        gameOver = true;
        gameOverScreen = true;
        gameOverTime = std::chrono::steady_clock::now();
        SDL_PauseAudioDevice(explosionAudioDevice, 1);
        SDL_PauseAudioDevice(boopAudioDevice, 1);
        if (!player1.alive && player2.alive) { score2 += 3; roundScore2 += 3; }
        else if (!player2.alive && player1.alive) { score1 += 3; roundScore1 += 3; }
    }
}

void Game::render() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (isSplashScreen) {
        drawSplashScreen(splashTexture);
    } else {
        for (auto& explosion : explosions) {
            float currentTimeSec = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
            if (!explosion.soundPlayed && currentTimeSec - explosion.startTime < EXPLOSION_DURATION) {
                SDL_PauseAudioDevice(explosionAudioDevice, 0);
                explosion.soundPlayed = true;
            }
            drawExplosion(explosion, currentTimeSec);
            if (currentTimeSec - explosion.startTime >= EXPLOSION_DURATION) {
                SDL_PauseAudioDevice(explosionAudioDevice, 1);
            }
        }

        if (gameOverScreen) {
            std::string scoreText = std::to_string(score1) + "-" + std::to_string(score2);
            float squareSize = 10.0f;
            float textWidth = scoreText.size() * squareSize * 6;
            float scoreX = (WIDTH - textWidth) / 2;
            drawText(scoreText, scoreX, HEIGHT / 2 - 40, squareSize, {255, 255, 255, 255});
            std::string roundText = "+" + std::to_string(roundScore1) + " - +" + std::to_string(roundScore2);
            float roundTextWidth = roundText.size() * squareSize * 6;
            drawText(roundText, (WIDTH - roundTextWidth) / 2, HEIGHT / 2 - 25 + 50, squareSize, {255, 255, 255, 255});
            int countdown = 5 - static_cast<int>(std::chrono::duration<float>(std::chrono::steady_clock::now() - gameOverTime).count());
            if (countdown >= 0) {
                drawText(std::to_string(std::max(1, countdown)), (WIDTH - squareSize * 6) / 2, HEIGHT / 2 + 25, squareSize, {255, 255, 255, 255});
            }
        } else {
            drawCollectibleBlackSquare(collectible);
            drawCollectibleBlackCircle(collectible);
            drawCollectibleGreenSquare(collectible);
            for (const auto& circle : circles) drawCircle(circle.pos.x, circle.pos.y, circle.radius, {255, 255, 0, 255});
            drawTrail(player1);
            drawTrail(player2);
            drawPlayer(player1);
            drawPlayer(player2);
            if (firstFrame) {
                std::string scoreText = std::to_string(score1) + "-" + std::to_string(score2);
                float squareSize = 10.0f;
                float textWidth = scoreText.size() * squareSize * 6;
                drawText(scoreText, (WIDTH - textWidth) / 2, HEIGHT / 2 - 25, squareSize, {255, 255, 255, 255});
                firstFrame = false;
            }
            if (paused) {
                std::string pauseText = "PAUSED";
                float squareSize = 10.0f;
                float textWidth = pauseText.size() * squareSize * 6;
                drawText(pauseText, (WIDTH - textWidth) / 2, HEIGHT / 2, squareSize, {255, 255, 255, 255});
            }
        }
    }
}

void Game::reset() {
    std::uniform_real_distribution<float> distX(50, WIDTH - 50);
    std::uniform_real_distribution<float> distY(50, HEIGHT - 50);
    player1 = Player{Vec2(200, HEIGHT / 2), Vec2(1, 0), {0, 0, 255, 255}, {}, true, false, false, Vec2(0, 0)};
    player2 = Player{Vec2(WIDTH - 200, HEIGHT / 2), Vec2(-1, 0), {255, 0, 0, 255}, {}, true, false, false, Vec2(0, 0)};
    float angle = std::uniform_real_distribution<float>(0, 2 * M_PI)(rng);
    circles = {Circle{Vec2(distX(rng), distY(rng)), Vec2(CIRCLE_SPEED * cos(angle), CIRCLE_SPEED * sin(angle)), CIRCLE_RADIUS}};
    collectible = spawnCollectible(rng);
    explosions.clear();
    SDL_PauseAudioDevice(explosionAudioDevice, 1);
    SDL_PauseAudioDevice(boopAudioDevice, 1);
    roundScore1 = roundScore2 = 0;
    gameOver = gameOverScreen = paused = false;
    lastCircleSpawn = std::chrono::steady_clock::now();
    lastBoopTime = 0.0f;
    boopPlaying = false;
}

void Game::toggleFullscreen() {
    Uint32 fullscreenFlag = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (fullscreenFlag) {
        if (SDL_SetWindowFullscreen(window, 0) < 0) {
            SDL_Log("Failed to switch to windowed mode: %s", SDL_GetError());
        }
        SDL_SetWindowSize(window, WIDTH, HEIGHT);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        glViewport(0, 0, WIDTH, HEIGHT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        SDL_Log("Switched to windowed mode: %dx%d", WIDTH, HEIGHT);
    } else {
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
            SDL_Log("Failed to switch to fullscreen mode: %s", SDL_GetError());
            return;
        }
        int drawableWidth, drawableHeight;
        SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
        SDL_DisplayMode displayMode;
        if (SDL_GetCurrentDisplayMode(0, &displayMode) < 0) {
            SDL_Log("Failed to get display mode: %s", SDL_GetError());
        } else {
            SDL_Log("Display mode: %dx%d, Drawable size: %dx%d", displayMode.w, displayMode.h, drawableWidth, drawableHeight);
        }
        float gameAspect = static_cast<float>(WIDTH) / HEIGHT;
        float drawableAspect = static_cast<float>(drawableWidth) / drawableHeight;
        int viewportWidth, viewportHeight, viewportX, viewportY;
        if (drawableAspect > gameAspect) {
            viewportHeight = drawableHeight;
            viewportWidth = static_cast<int>(drawableHeight * gameAspect);
            viewportX = (drawableWidth - viewportWidth) / 2;
            viewportY = 0;
        } else {
            viewportWidth = drawableWidth;
            viewportHeight = static_cast<int>(drawableWidth / gameAspect);
            viewportX = 0;
            viewportY = (drawableHeight - viewportHeight) / 2;
        }
        glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        SDL_Log("Fullscreen viewport: %d,%d,%dx%d", viewportX, viewportY, viewportWidth, viewportHeight);
    }
}