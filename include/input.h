#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

// Forward declaration to avoid including game.h
class Game;

class InputManager {
public:
    InputManager();
    bool handleInput(SDL_GameController* controllers[], int controllerCount, bool gameOverScreen, bool& isSplashScreen, bool& paused, Game* game);
private:
    bool lastFrameButtons[2][SDL_CONTROLLER_BUTTON_MAX]; // Track controller button states
    bool lastFrameKeys[SDL_NUM_SCANCODES];              // Track keyboard key states
    bool musicMuted;                                    // Track music mute state
    void handleAIModeInput(SDL_KeyboardEvent& keyEvent, Game* game); // Handle AI mode
};

#endif // INPUT_H