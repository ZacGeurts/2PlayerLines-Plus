#include "player.h"
#include "game.h"
#include "ai.h" // For AI class definition
#include <SDL2/SDL.h>
#include <random>
#include <chrono>
#include <cmath>

namespace Game {

PlayerManager::PlayerManager(const GameConfig& config) : config(config) {}

void PlayerManager::updatePlayers(SDL_GameController* controllers[], int controllerCount,
                                  Player& player1, Player& player2, Collectible& collectible,
                                  std::vector<Explosion>& explosions, std::vector<Flash>& flashes,
                                  int& score1, int& score2, int& roundScore1, int& roundScore2,
                                  std::mt19937& rng, float dt, float currentTimeSec,
                                  AudioManager& audioManager, CollectibleManager& collectibleManager,
                                  ExplosionManager& explosionManager, CircleManager& circleManager,
                                  std::vector<Circle>& circles, std::chrono::steady_clock::time_point& lastCircleSpawn,
                                  Game* game, const std::vector<unsigned char>& framebuffer,
                                  int drawableWidth, int drawableHeight, SDL_Color aiColor) {
    dt = std::min(dt, 0.008333f); // 120 FPS cap

    for (auto* player : {&player1, &player2}) {
        if (!player->alive) {
            // Placeholder: Assume respawn after 2 seconds
            if (currentTimeSec - game->deathTime > 2.0f) {
                player->alive = true;
                player->pos = (player == &player1) ? Vec2(config.WIDTH / 4.0f, config.HEIGHT / 2.0f) : Vec2(3 * config.WIDTH / 4.0f, config.HEIGHT / 2.0f);
                player->direction = (player == &player1) ? Vec2(1.0f, 0.0f) : Vec2(-1.0f, 0.0f);
                player->trail.clear();
                player->noCollisionTimer = config.INVINCIBILITY_DURATION;
                player->isInvincible = true;
                flashes.emplace_back(explosionManager.createFlash(player->pos, rng, dt, currentTimeSec, {255, 0, 255, 255}));
                audioManager.playLaserZap(currentTimeSec);
                if (config.ENABLE_DEBUG) {
                    SDL_Log("Player %s respawned at (%f, %f), time=%f", player == &player1 ? "1" : "2", player->pos.x, player->pos.y, currentTimeSec);
                }
            }
            continue;
        }

        if (player->willDie) continue;

        Vec2 newDir = player->direction;
        float speed = config.PLAYER_SPEED;

        if (player == &player2 && game->ai && game->ai->getMode()) {
            game->ai->startUpdate(*player, player1, collectible, circles, dt, rng, framebuffer, drawableWidth, drawableHeight, aiColor);
            game->ai->waitForUpdate();
            game->ai->applyUpdate(*player);
        } else {
            int index = (player == &player1) ? 0 : 1;
            float turn = 0.0f;
            bool triggerFlash = false;

            if (index < controllerCount && controllers[index] && SDL_GameControllerGetAttached(controllers[index])) {
                Sint16 leftTrigger = SDL_GameControllerGetAxis(controllers[index], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
                Sint16 rightTrigger = SDL_GameControllerGetAxis(controllers[index], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
                if (leftTrigger > 0 || rightTrigger > 0) {
                    player->hasMoved = true;
                }
                turn = (rightTrigger - leftTrigger) / 32768.0f * config.TURN_SPEED * dt;
                triggerFlash = SDL_GameControllerGetButton(controllers[index], SDL_CONTROLLER_BUTTON_A);
            }

            float angle = atan2(player->direction.y, player->direction.x) + turn;
            newDir = Vec2(cos(angle), sin(angle)).normalized();

            if (triggerFlash && player->canUseNoCollision && !player->isInvincible) {
                game->activateNoCollision(player, currentTimeSec, dt);
                if (config.ENABLE_DEBUG) {
                    SDL_Log("Player %s triggered flash at (%f, %f), time=%f",
                            player == &player1 ? "1" : "2", player->pos.x, player->pos.y, currentTimeSec);
                }
            }
        }

        if (player->noCollisionTimer > 0) {
            player->noCollisionTimer -= dt;
            player->isInvincible = true;
            if (player->noCollisionTimer <= 0) {
                player->noCollisionTimer = 0.0f;
                player->isInvincible = false;
                player->endFlash = std::make_unique<Flash>(explosionManager.createFlash(player->pos, rng, dt, currentTimeSec, {255, 0, 255, 255}));
                flashes.push_back(*player->endFlash);
                audioManager.playLaserZap(currentTimeSec);
                if (config.ENABLE_DEBUG) {
                    SDL_Log("No-collision ended for player %s at (%f, %f), time=%f",
                            player == &player1 ? "1" : "2", player->pos.x, player->pos.y, currentTimeSec);
                }
            }
        }

        Vec2 nextPos = player->pos + newDir * speed * dt;

        if (player->hasMoved && !player->isInvincible && !player->spawnInvincibilityTimer) {
            game->checkCollision(player, nextPos, currentTimeSec, framebuffer, drawableWidth, drawableHeight, dt);
        }

        if (!player->willDie && (nextPos.x < 10 || nextPos.x > game->orthoWidth - 10 ||
                                 nextPos.y < 10 || nextPos.y > game->orthoHeight - 10)) {
            player->willDie = true;
            explosions.emplace_back(explosionManager.createExplosion(nextPos, rng, dt, currentTimeSec, player->color));
            audioManager.playExplosion(currentTimeSec);
            game->deathTime = currentTimeSec;
            if (config.ENABLE_DEBUG) {
                SDL_Log("Player %s hit wall at (%f, %f), explosion triggered",
                        player == &player1 ? "1" : "2", nextPos.x, nextPos.y);
            }
        }

        if (!player->willDie) {
            player->pos = nextPos;
            player->direction = newDir;
            player->trail.push_back(player->pos);
        }

        if (player->willDie) {
            game->handlePlayerDeath(player, currentTimeSec);
            if (player == &player1) {
                score2 += static_cast<int>(game->deathPoints);
                roundScore2 += static_cast<int>(game->deathPoints);
            } else {
                score1 += static_cast<int>(game->deathPoints);
                roundScore1 += static_cast<int>(game->deathPoints);
            }
            if (config.ENABLE_DEBUG) {
                SDL_Log("Player %s died at (%f, %f)", player == &player1 ? "1" : "2", player->pos.x, player->pos.y);
            }
        }

        // Check collectible collision
        if (collectibleManager.checkCollectibleCollision(player->pos, collectible, *game)) {
            player->collectedGreenThisFrame = true;
            if (player == &player1) {
                score1 += static_cast<int>(game->greenSquarePoints);
                roundScore1 += static_cast<int>(game->greenSquarePoints);
            } else {
                score2 += static_cast<int>(game->greenSquarePoints);
                roundScore2 += static_cast<int>(game->greenSquarePoints);
            }
            collectible.active = false;
            game->collectibleCollectedThisFrame = true;
            game->pendingCollectibleRespawn = true;
            audioManager.playBoop(currentTimeSec);
            game->lastCircleSpawn = std::chrono::steady_clock::now();
        }
    }

    // Update managers
    if (game->pendingCollectibleRespawn) {
        auto elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - game->lastCircleSpawn).count();
        if (elapsed > config.COLLECT_COOLDOWN) {
            collectible = collectibleManager.spawnCollectible(rng, *game);
            game->pendingCollectibleRespawn = false;
        }
    }
    circleManager.updateCircles(dt, circles, rng, currentTimeSec, lastCircleSpawn, *game);
    explosionManager.updateExplosions(explosions, dt, currentTimeSec, aiColor);
    explosionManager.updateFlashes(flashes, dt, currentTimeSec, {255, 0, 255, 255});
    explosionManager.cleanupPlayerFlashes(player1, player2, currentTimeSec);
    circleManager.clearTrails(circles, player1, player2);
}

} // namespace Game