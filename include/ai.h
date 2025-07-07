#ifndef AI_H
#define AI_H

#include "types.h"
#include <vector>
#include <thread>
#include <mutex>
#include <random>

namespace Game {

class Game; // Forward declaration

class AI {
public:
    struct RaycastResult {
        struct LineCheckResult {
            std::string color;
            Vec2 hitPos;
            bool hasDanger;
            bool greenVisible;
            float distance;
        };
        LineCheckResult centerLine, leftLine, rightLine;
        Vec2 leftDir, rightDir;
    };

    AI(const GameConfig& config, Game& game);
    ~AI();
    void startUpdate(Player& aiPlayer, const Player& opponent, const Collectible& collectible,
                     const std::vector<Circle>& circles, float dt, std::mt19937& rng,
                     const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight,
                     SDL_Color aiColor);
    void waitForUpdate();
    void applyUpdate(Player& aiPlayer);
    bool getMode() const { return modeEnabled; }
    void setMode(bool enabled) { modeEnabled = enabled; } // Added: Toggle AI mode
    void resetFlash() { flashUsed = false; } // Reset flash after collision
    Vec2 calculateTargetDirection(const Player& aiPlayer, const Collectible& collectible,
                                 const std::vector<Circle>& circles, const Player& opponent,
                                 std::mt19937& rng, Game& game, float currentTimeSec,
                                 const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);
    RaycastResult raycastForward(const Player& aiPlayer, Game& game, float currentTimeSec,
                                 const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);
    RaycastResult::LineCheckResult checkLine(const Vec2& start, const Vec2& dir, float maxDistance,
                                             const Vec2& playerDir, Game& game, float currentTimeSec,
                                             const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) const;
    std::string getPixelColor(const Vec2& pos, Game& game, float currentTimeSec,
                              const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) const;
private:
    void simulateControllerInput(const Player& aiPlayer, const Collectible& collectible,
                                const std::vector<Circle>& circles, const Player& opponent,
                                float dt, std::mt19937& rng, Game& game,
                                const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);

    const GameConfig& config;
    Game* game;
    std::thread updateThread;
    std::mutex mutex;
    std::vector<unsigned char> framebuffer;
    bool flashUsed;
    bool modeEnabled;
    bool updateReady;
    bool shouldDie;
    bool hasMoved;
    bool hitOpponentHeadResult;
    bool aButton;
    float leftTrigger;
    float rightTrigger;
    float currentTimeSec;
    int frameCount;
    int drawableWidth;
    int drawableHeight;
    Vec2 newPosition;
    Vec2 newDirection;
    SDL_Color aiColor;
};

} // namespace Game

#endif // AI_H