#ifndef AI_H
#define AI_H

#include "types.h"
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <SDL2/SDL.h>

namespace Game {

class AI {
public:
    struct LineCheckResult {
        Vec2 hitPos;
        float distance;
        bool hasDanger;
        bool greenVisible;
        std::string color;
    };

    struct RaycastResult {
        LineCheckResult centerLine;
        LineCheckResult leftLine;
        LineCheckResult rightLine;
        Vec2 leftDir;
        Vec2 rightDir;
    };

    struct PathNode {
        Vec2 pos;
        float gCost;
        float fCost;
        std::shared_ptr<PathNode> parent;
        bool operator>(const PathNode& other) const { return fCost > other.fCost; }
    };

    AI(const GameConfig& config);
    ~AI();

    void startUpdate(Player& aiPlayer, const Player& opponent, const Collectible& collectible,
                     const std::vector<Circle>& circles, float dt, std::mt19937& rng,
                     const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight, SDL_Color);
    void waitForUpdate();
    void applyUpdate(Player& aiPlayer);
    bool getMode() const { return modeEnabled; }
    void setMode(bool enabled) { modeEnabled = enabled; } // Added setMode
    void resetFlash() { flashUsed = false; }

private:
    void simulateControllerInput(const Player& aiPlayer, const Collectible& collectible,
                                const std::vector<Circle>& circles, const Player& opponent,
                                float dt, std::mt19937& rng,
                                const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight,
                                float& leftTrigger, float& rightTrigger, bool& aButton);

    Vec2 calculateTargetDirection(const Player& aiPlayer, const Collectible& collectible,
                                 const std::vector<Circle>& circles, const Player& opponent,
                                 std::mt19937& rng, float currentTimeSec,
                                 const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);

    RaycastResult raycastForward(const Player& aiPlayer, float currentTimeSec,
                                const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);

    LineCheckResult checkLine(const Vec2& start, const Vec2& dir, float maxDistance,
                             const Vec2& playerDir, float currentTimeSec,
                             const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) const;

    std::string getPixelColor(const Vec2& pos, float currentTimeSec,
                             const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) const;

    float heuristic(const Vec2& a, const Vec2& b) const;

    bool isPositionSafe(const Vec2& pos, const std::vector<Circle>& circles, const Player& opponent);

    std::vector<Vec2> findPathAStar(const Vec2& start, const Vec2& goal,
                                   const std::vector<Circle>& circles, const Player& opponent,
                                   const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight);

    const GameConfig& config;
    std::vector<unsigned char> framebuffer;
    int drawableWidth;
    int drawableHeight;
    bool flashUsed;
    bool modeEnabled;
    float leftTrigger;
    float rightTrigger;
    bool aButton;
    bool updateReady;
    Vec2 newDirection;
    std::thread updateThread;
    std::mutex mutex;
    float currentTimeSec;
    int frameCount;
};

} // namespace Game

#endif // AI_H