#include "input.h"
#include "game.h"

bool handleInput(SDL_GameController* controllers[], int controllerCount, bool gameOverScreen, bool& paused, SDL_AudioDeviceID boopAudioDevice, bool& isSplashScreen, Game* game) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return false;
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_f:
                    game->toggleFullscreen();
                    break;
                case SDLK_q:
                case SDLK_ESCAPE:
                    return false; // Quit the program
            }
        }
        if (event.type == SDL_CONTROLLERBUTTONDOWN) {
            if (isSplashScreen) {
                Uint8 button = event.cbutton.button;
                if (button == SDL_CONTROLLER_BUTTON_A || button == SDL_CONTROLLER_BUTTON_B ||
                    button == SDL_CONTROLLER_BUTTON_X || button == SDL_CONTROLLER_BUTTON_Y) {
                    isSplashScreen = false;
                }
            } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
                if (!gameOverScreen) {
                    paused = !paused;
                    if (paused) {
                        SDL_PauseAudioDevice(boopAudioDevice, 1);
                    }
                }
            }
        }
    }
    return true;
}