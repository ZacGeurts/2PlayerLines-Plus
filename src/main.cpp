#include "game.h"
#include "types.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <fstream>
#include <sstream>
#include <string>

// Always put hearing safety first. It does not grow back.

// Note: long double and custom rng is Songgen only. Linesplus is a different program (this) and uses the amazing mt19937 RNG.
// We do not include instruments.h songen.h or songgen.cpp as they are a seperate program that we run to play music.

// Hi. Not much to see here in the main.cpp. Just some setup.
// This is the starting point if you want to expand game.ini.
// Next stop is types.h. This overwrites types.h with game.ini. (changing values in GameConfig should be done with game.ini)
// If you add to game.ini, start down here then types.h.
// game.cpp and game.h and types.h are the real starting points.

// linesplus and songgen are 2 different programs.
// songgen.h songgen.cpp instruments.h and instrument files are not free distribution.
// Personal use only. Or it costs a portion of the profit.
// linesplus is free to delete or sell or whatever.
// songgen is free to delete.
// linesplus is MIT license which I think basically means you own it too.
// songgen is not free to do with as you please, unless non-profit personal use.

// Feel free to branch a fork on github of my project. Make it make money.
// A fork will make a backup of my current progress and allow you to have your own backup and space to make changes.
// I accept issue tickets and will weigh code submissions for acceptance into my main branch.
// Issues do not to be detailed as long as I can understand the issue.
// If you make money off songgen, contact me and we can discuss.

// Next to none of the code in these files is AI written.
// My process is to get code from AI, rewrite it, and resubmit it if there are bugs. Sometimes just to have it add comments.
// The code I am adjusting is my own.
// Some code blocks are copied directly from AI, but it is modifying my code with usually a compiler error fix.
// If formatting looks like I copied and pasted directly from AI, I probably did.
// What I usually copy is my code back with the one or two adjustments AI does to tweak a code block faster than me.
// or the fun ones like "alphabetize this"
// The code is prompted and curated how I choose to be the designer of this and other software.
// That being said, there still a few areas I have yet to go over and rewrite, but probably all were at least cursory reviewed.
// AI is dumb and will never see truly what it is you are working on and would like to accomplish.
// Maybe pasting fixed and slightly broken code helps the AI training data.
// If it tells you code, it is in your interest most times to review and update.

// And this code was written from my game.ini
Game::GameConfig loadConfig(const std::string& filename) {
    Game::GameConfig config;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        SDL_Log("Failed to open config file %s, using default values", filename.c_str());
        return config;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key;
        float value;

        if (std::getline(iss, key, '=') && iss >> value) {
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
		// The lines with 'key' are reading the game.ini
            if (key == "WIDTH") config.WIDTH = static_cast<int>(value);
            else if (key == "HEIGHT") config.HEIGHT = static_cast<int>(value);
            else if (key == "PLAYER_SPEED") config.PLAYER_SPEED = value;
            else if (key == "AI_SPEED") config.AI_SPEED = value;
            else if (key == "TURN_SPEED") config.TURN_SPEED = value;
            else if (key == "AI_TURN_SPEED") config.AI_TURN_SPEED = value;
            else if (key == "RAYCAST_STEP") config.RAYCAST_STEP = value;
            else if (key == "CIRCLE_SPEED") config.CIRCLE_SPEED = value;
            else if (key == "CIRCLE_RADIUS") config.CIRCLE_RADIUS = value;
            else if (key == "COLLISION_CHECK_SIZE") config.COLLISION_CHECK_SIZE = value;
            else if (key == "BOOP_DURATION") config.BOOP_DURATION = value;
            else if (key == "EXPLOSION_DURATION") config.EXPLOSION_DURATION = value;
            else if (key == "LASER_ZAP_DURATION") config.LASER_ZAP_DURATION = value;
            else if (key == "WINNER_VOICE_DURATION") config.WINNER_VOICE_DURATION = value;
            else if (key == "GREEN_SQUARE_SIZE") config.GREEN_SQUARE_SIZE = value;
            else if (key == "COLLECTIBLE_SIZE") config.COLLECTIBLE_SIZE = value;
            else if (key == "EXPLOSION_MAX_RADIUS") config.EXPLOSION_MAX_RADIUS = value;
            else if (key == "PLAYER_SIZE") config.PLAYER_SIZE = value;
            else if (key == "TRAIL_SIZE") config.TRAIL_SIZE = value;
            else if (key == "WINNING_SCORE") config.WINNING_SCORE = value;
            else if (key == "GREEN_SQUARE_POINTS") config.GREEN_SQUARE_POINTS = value;
            else if (key == "DEATH_POINTS") config.DEATH_POINTS = value;
            else if (key == "INVINCIBILITY_DURATION") config.INVINCIBILITY_DURATION = value;
            else if (key == "AI_BERTH") config.AI_BERTH = value;
            else if (key == "ENABLE_DEBUG") config.ENABLE_DEBUG = static_cast<bool>(value);
        }
    }

    file.close();
    return config;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        SDL_Log("SDL_image initialization failed: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    Game::GameConfig config = loadConfig("game.ini");
    try {
        Game::Game game(config); // Only config is passed
        game.run();
    } catch (const std::exception& e) {
        SDL_Log("Game error: %s", e.what());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    IMG_Quit();
    SDL_Quit();
    return 0;
}