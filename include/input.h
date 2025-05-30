#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

namespace Game { class Game; } // Forward declaration

class InputManager {
public:
    InputManager();
    bool handleInput(SDL_GameController* controllers[], int controllerCount, bool gameOverScreen, bool& isSplashScreen, bool& paused, Game::Game* game);
private:
    bool lastFrameButtons[2][SDL_CONTROLLER_BUTTON_MAX];
    bool lastFrameKeys[SDL_NUM_SCANCODES];
    bool musicMuted;
    void handleAIModeInput(SDL_KeyboardEvent& keyEvent, Game::Game* game);
};

#endif // INPUT_H