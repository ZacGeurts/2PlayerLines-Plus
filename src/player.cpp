#include "player.h"
#include "game.h"
#include <SDL2/SDL.h>
#include <cmath>

PlayerManager::PlayerManager(const GameConfig& config) : config(config) {}

void PlayerManager::updatePlayers(SDL_GameController* controllers[], int controllerCount, Player& player1, Player& player2,
                                 Collectible& collectible, std::vector<Explosion>& explosions, std::vector<Flash>& flashes,
                                 int& score1, int& score2, int& roundScore1, int& roundScore2, std::mt19937& rng,
                                 float dt, float currentTimeSec, AudioManager& audio, CollectibleManager& collectibleManager,
                                 ExplosionManager& explosionManager, CircleManager& circleManager, std::vector<Circle>& circles,
                                 std::chrono::steady_clock::time_point& lastCircleSpawn, Game* game,
                                 const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight, SDL_Color SDLaicolor) {
    dt = std::min(dt, 0.008333f); // 120 FPS cap

    for (auto* player : {&player1, &player2}) {
        if (!player->alive && game->shouldRespawnPlayer(player, currentTimeSec)) {
            player->alive = true;
            player->pos = game->getSpawnPosition();
            player->direction = Vec2(1.0f, 0.0f);
            player->trail.clear();
            player->noCollisionTimer = config.INVINCIBILITY_DURATION;
            player->isInvincible = true;
            flashes.emplace_back(
                explosionManager.createFlash(player->pos, rng, dt, currentTimeSec, {255, 0, 255, 255}));
            audio.playLaserZap(currentTimeSec);
            if (config.ENABLE_DEBUG) {
                SDL_Log("Invincibility started for player %s at (%f, %f), time=%f, magenta flash triggered",
                        player == &player1 ? "1" : "2", player->pos.x, player->pos.y, currentTimeSec);
            }
        }

        if (!player->alive || player->willDie) continue;

        Vec2 newDir = player->direction;
        float speed = config.PLAYER_SPEED; // Assumed 200.0f to match AI_SPEED

        if (player == &player2 && game->ai && game->ai->getMode()) {
            // AI-controlled player2
            game->ai->startUpdate(*player, player1, collectible, circles, dt, rng, *game,
                                  framebuffer, drawableWidth, drawableHeight, game->SDLaicolor);
            game->ai->waitForUpdate();
            game->ai->applyUpdate(*player);
        } else {
            // Human-controlled player
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
                game->activateNoCollision(player, currentTimeSec);
                if (config.ENABLE_DEBUG) {
                    SDL_Log("Player %s triggered flash at (%f, %f), time=%f",
                            player == &player1 ? "1" : "2", player->pos.x, player->pos.y, currentTimeSec);
                }
            }
        }

        // Update no-collision timer
        if (player->noCollisionTimer > 0) {
            player->noCollisionTimer -= dt;
            player->isInvincible = true;
            if (config.ENABLE_DEBUG) {
                SDL_Log("Player %s noCollisionTimer: %f", player == &player1 ? "1" : "2", player->noCollisionTimer);
            }
            if (player->noCollisionTimer <= 0) {
                player->noCollisionTimer = 0.0f;
                player->isInvincible = false;
                player->endFlash = std::make_unique<Flash>(
                    explosionManager.createFlash(player->pos, rng, dt, currentTimeSec, {255, 0, 255, 255}));
                flashes.emplace_back(*player->endFlash);
                audio.playLaserZap(currentTimeSec);
                if (config.ENABLE_DEBUG) {
                    SDL_Log("No-collision ended for player %s at (%f, %f), time=%f, magenta flash triggered",
                            player == &player1 ? "1" : "2", player->pos.x, player->pos.y, currentTimeSec);
                }
            }
        }

        Vec2 nextPos = player->pos + newDir * speed * dt;

        // Check pixel collision
        if (player->hasMoved && !player->isInvincible && !player->spawnInvincibilityTimer) {
            if (framebuffer.empty()) {
                if (config.ENABLE_DEBUG) {
                    SDL_Log("Warning: Framebuffer empty for player %s collision check", player == &player1 ? "1" : "2");
                }
            } else {
                game->checkCollision(player, nextPos, currentTimeSec, framebuffer, drawableWidth, drawableHeight);
            }
        }

        // Check wall collision
        if (!player->willDie && (nextPos.x < 10 || nextPos.x > game->orthoWidth - 10 ||
                                 nextPos.y < 10 || nextPos.y > game->orthoHeight - 10)) {
            player->willDie = true;
            explosions.emplace_back(explosionManager.createExplosion(nextPos, rng, dt, currentTimeSec, player->color));
            audio.playExplosion(currentTimeSec);
            game->deathTime = currentTimeSec;
            if (config.ENABLE_DEBUG) {
                SDL_Log("Player %s hit wall at (%f, %f), explosion triggered, willDie=true, orthoWidth=%f, orthoHeight=%f",
                        player == &player1 ? "1" : "2", nextPos.x, nextPos.y, game->orthoWidth, game->orthoHeight);
            }
        }

        // Update position and trail
        if (!player->willDie) {
            player->pos = nextPos;
            player->direction = newDir;
            player->trail.push_back(player->pos);
        }

        if (player->willDie) {
            game->handlePlayerDeath(player, currentTimeSec);
            if (player == &player1) {
                score2 += game->deathPoints;
                roundScore2 += game->deathPoints;
            } else {
                score1 += game->deathPoints;
                roundScore1 += game->deathPoints;
            }
            if (config.ENABLE_DEBUG) {
                SDL_Log("Player %s died at (%f, %f), willDie=true",
                        player == &player1 ? "1" : "2", player->pos.x, player->pos.y);
            }
        }
    }

    circleManager.updateCircles(dt, circles, rng, currentTimeSec, lastCircleSpawn, *game);
    circleManager.clearTrails(circles, player1, player2);
    explosionManager.updateFlashes(flashes, dt, currentTimeSec, {255, 0, 255, 255});
    explosionManager.cleanupPlayerFlashes(player1, player2, currentTimeSec);
}