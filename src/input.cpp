#include "input.h"
#include "game.h"
#include <chrono>
#include <cstring>

InputManager::InputManager() : musicMuted(false) {
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
                case SDLK_n:
                    //game->renderManager->togglePostProcessing();
                    break;
                case SDLK_i:
                    //game->toggleFPSDisplay();
                    break;
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_p:
                    if (!isSplashScreen && !gameOverScreen && !game->winnerDeclared) {
                        paused = !paused;
                        SDL_Log("Game %s", paused ? "paused" : "unpaused");
                    }
                    break;
                case SDLK_m:
                    musicMuted = !musicMuted;
                    if (musicMuted) {
                        game->audio.stopBackgroundMusic();
                        SDL_Log("Music muted (sent Ctrl+C to songgen)");
                    } else {
                        game->audio.startBackgroundMusic();
                        SDL_Log("Music unmuted (restarted songgen)");
                    }
                    break;
                default:
                    handleAIModeInput(event.key, game);
                    break;
            }
            lastFrameKeys[event.key.keysym.scancode] = true;
        } else if (event.type == SDL_KEYUP) {
            lastFrameKeys[event.key.keysym.scancode] = false;
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN && !lastFrameButtons[event.cbutton.which][event.cbutton.button]) {
            Uint8 button = event.cbutton.button;
            int controllerIndex = event.cbutton.which;
            lastFrameButtons[controllerIndex][button] = true;

            if (game->winnerDeclared) {
                if (button == SDL_CONTROLLER_BUTTON_A) {
                    game->resumeAfterWinner();
                }
                continue;
            }

            if (isSplashScreen) {
                if (button == SDL_CONTROLLER_BUTTON_A || button == SDL_CONTROLLER_BUTTON_B ||
                    button == SDL_CONTROLLER_BUTTON_X || button == SDL_CONTROLLER_BUTTON_Y) {
                    isSplashScreen = false;
                    game->firstFrame = true;
                    SDL_Log("Splash screen exited via controller button %d", button);
					game->reset();
                }
            } else if (!gameOverScreen) {
                if (button == SDL_CONTROLLER_BUTTON_X || button == SDL_CONTROLLER_BUTTON_Y ||
                    button == SDL_CONTROLLER_BUTTON_B) {
                    paused = !paused;
                    SDL_Log("Game %s", paused ? "paused" : "unpaused");
                }
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

    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
        if (!keyboardState[i]) {
            lastFrameKeys[i] = false;
        }
    }

    return running;
}

void InputManager::handleAIModeInput(SDL_KeyboardEvent& keyEvent, Game* game) {
    switch (keyEvent.keysym.sym) {
        case SDLK_1:            
            game->ai->setMode(true);
            SDL_Log("AI Mode set to ON (One-player mode)");
            break;
        case SDLK_2:
            game->ai->setMode(false);
            SDL_Log("AI Mode set to OFF (Two-player mode)");
            break;
    }
}