#ifndef AI_H
#define AI_H

#include <vector>
#include <random>
#include <mutex>
#include <thread>
#include "types.h"
#include "game.h"

// Forward declaration of Game
class Game;

class AI {
public:
    AI(const GameConfig& config, Game& game);
    ~AI();
    void startUpdate(Player& aiPlayer, const Player& opponent, const Collectible& collectible,
                     const std::vector<Circle>& circles, float dt, std::mt19937& rng, Game& game,
                     const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight, SDL_Color SDLaicolor);
    void waitForUpdate();
    void applyUpdate(Player& aiPlayer);
    void resetFlash() { flashUsed = false; }
    bool getMode() const { return modeEnabled; }
    void setMode(bool enabled) { modeEnabled = enabled; }

private:
    struct LineCheckResult {
        std::string color;
        Vec2 hitPos;
        bool greenVisible;
        bool hasDanger;
        float distance;
    };

    struct RaycastResult {
        LineCheckResult centerLine;
        LineCheckResult leftLine;
        Vec2 leftDir;
        LineCheckResult rightLine;
        Vec2 rightDir;
    };

    const GameConfig& config;
    Game* game; // Store Game pointer
    std::vector<unsigned char> framebuffer;
    int drawableWidth;
    int drawableHeight;
    bool flashUsed;
    bool modeEnabled;
    std::mutex mutex;
    std::thread updateThread;
    float leftTrigger;
    float rightTrigger;
    bool aButton;
    bool updateReady;
    Vec2 newDirection;
    bool shouldDie;
    Vec2 newPosition;
    bool hasMoved;
    bool hitOpponentHeadResult;
    float currentTimeSec;
    size_t frameCount;

    void simulateControllerInput(const Player& aiPlayer, const Collectible& collectible,
                                const std::vector<Circle>& circles, const Player& opponent,
                                float dt, std::mt19937& rng, Game& game,
                                const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight,
                                float& leftTrigger, float& rightTrigger, bool& aButton);
    Vec2 calculateTargetDirection(const Player& aiPlayer, const Collectible& collectible,
                                  const std::vector<Circle>& circles, const Player& opponent,
                                  std::mt19937& rng, Game& game, float currentTimeSec,
                                  const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);
    RaycastResult raycastForward(const Player& aiPlayer, Game& game, float currentTimeSec,
                                 const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);
    LineCheckResult checkLine(const Vec2& start, const Vec2& dir, float maxDistance, const Vec2& playerDir,
                              Game& game, float currentTimeSec, const std::vector<unsigned char>& framebuffer,
                              int drawableWidth, int drawableHeight) const;
    std::string getPixelColor(const Vec2& pos, Game& game, float currentTimeSec,
                              const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) const;
};

#endif // AI_H