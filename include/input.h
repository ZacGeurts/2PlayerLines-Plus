#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

// Forward declaration to avoid including game.h
class Game;

bool handleInput(SDL_GameController* controllers[], int controllerCount, bool gameOverScreen, bool& paused, SDL_AudioDeviceID boopAudioDevice, bool& isSplashScreen, Game* game);

#endif // INPUT_H