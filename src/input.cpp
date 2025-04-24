#include "input.h"
#include "game.h"
#include <chrono>
#include <cstring>

InputManager::InputManager() {
    std::memset(lastFrameButtons, 0, sizeof(lastFrameButtons));
    std::memset(lastFrameKeys, 0, sizeof(lastFrameKeys));
}

bool InputManager::handleInput(SDL_GameController* controllers[], int controllerCount, bool gameOverScreen, bool& isSplashScreen, bool& paused, Game* game) {
    SDL_Event event;
    bool running = true;
    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        } else if (event.type == SDL_KEYDOWN && !lastFrameKeys[event.key.keysym.scancode]) {
            switch (event.key.keysym.sym) {
                case SDLK_f:
                    game->toggleFullscreen();
                    break;
                case SDLK_q:
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_SPACE:
                    if (isSplashScreen) {
                        isSplashScreen = false;
                        game->firstFrame = true;
                    } else if (gameOverScreen) {
                        game->reset();
                    }
                    break;
            }
            lastFrameKeys[event.key.keysym.scancode] = true;
        } else if (event.type == SDL_KEYUP) {
            lastFrameKeys[event.key.keysym.scancode] = false;
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN && !lastFrameButtons[event.cbutton.which][event.cbutton.button]) {
            Uint8 button = event.cbutton.button;
            int controllerIndex = event.cbutton.which;
            lastFrameButtons[controllerIndex][button] = true;

            if (isSplashScreen) {
                if (button == SDL_CONTROLLER_BUTTON_A || button == SDL_CONTROLLER_BUTTON_B ||
                    button == SDL_CONTROLLER_BUTTON_X || button == SDL_CONTROLLER_BUTTON_Y) {
                    isSplashScreen = false;
                    game->firstFrame = true;
                }
            } else if (!gameOverScreen) {
                // Handle pause
                if (button == SDL_CONTROLLER_BUTTON_X || button == SDL_CONTROLLER_BUTTON_Y ||
                    button == SDL_CONTROLLER_BUTTON_B) {
                    paused = !paused;
                }
                // Handle no-collision ability
                Player* player = nullptr;
                if (controllerIndex == 0) player = &game->player1;
                else if (controllerIndex == 1) player = &game->player2;
                if (player && button == SDL_CONTROLLER_BUTTON_A) {
                    float currentTimeSec = std::chrono::duration<float>(std::chrono::steady_clock::now().time_since_epoch()).count();
                    game->activateNoCollision(player, currentTimeSec);
                }
            }
        } else if (event.type == SDL_CONTROLLERBUTTONUP) {
            lastFrameButtons[event.cbutton.which][event.cbutton.button] = false;
        }
    }

    // Update keyboard states for continuous checks
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
        if (!keyboardState[i]) {
            lastFrameKeys[i] = false;
        }
    }

    return running;
}