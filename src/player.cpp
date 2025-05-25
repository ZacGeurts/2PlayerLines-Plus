#include "player.h"
#include "game.h"
#include <GL/gl.h>
#include <cmath>

PlayerManager::PlayerManager(const GameConfig& config) : config(config) {}

void PlayerManager::updatePlayers(SDL_GameController* controllers[], int controllerCount, Player& player1, Player& player2,
                                  Collectible& collectible, std::vector<Explosion>& explosions, std::vector<Flash>& flashes,
                                  int& score1, int& score2, int& roundScore1, int& roundScore2, std::mt19937& rng,
                                  float dt, float currentTimeSec, AudioManager& audio, CollectibleManager& collectibleManager,
                                  ExplosionManager& explosionManager, CircleManager& circleManager, std::vector<Circle>& circles,
                                  std::chrono::steady_clock::time_point& lastCircleSpawn, Game* game,
                                  const std::vector<unsigned char>& framebuffer, int drawableWidth, int drawableHeight) {
    // Handle controller input for turning
    for (int i = 0; i < controllerCount; ++i) {
        if (!controllers[i] || !SDL_GameControllerGetAttached(controllers[i])) {
            if (config.ENABLE_DEBUG) {
                SDL_Log("Controller %d is null or detached", i);
            }
            continue;
        }
        Player& player = (i == 0) ? player1 : player2;
        if (!player.alive) continue;

        Sint16 leftTrigger = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        Sint16 rightTrigger = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        if (leftTrigger > 0 || rightTrigger > 0) player.hasMoved = true;
        float turn = (rightTrigger - leftTrigger) / 32768.0f * config.TURN_SPEED * dt;
        float angle = atan2(player.direction.y, player.direction.x) + turn;
        player.direction = Vec2(cos(angle), sin(angle));

        // Update no-collision timer
        if (player.noCollisionTimer > 0) {
            player.noCollisionTimer -= dt;
            player.isInvincible = true;
            if (player.noCollisionTimer <= 0) {
                player.noCollisionTimer = 0;
                player.isInvincible = false;
                player.endFlash = new Flash(explosionManager.createFlash(player.pos, rng, currentTimeSec, {255, 0, 255, 255}));
                audio.playLaserZap(currentTimeSec);
                if (config.ENABLE_DEBUG) {
                    SDL_Log("No-collision ended for player %s at (%f, %f), time=%f, magenta flash triggered, canUseNoCollision=%d",
                            (&player == &player1 ? "1" : "2"), player.pos.x, player.pos.y, currentTimeSec, player.canUseNoCollision);
                }
            }
        }
    }

    // Update player positions and check collisions
    for (auto& player : {&player1, &player2}) {
        if (!player->alive) continue;

        Vec2 nextPos = player->pos + player->direction * config.PLAYER_SPEED * dt;

        if (player->willDie) {
            player->alive = false;
            player->deathPos = player->pos;
            if (config.ENABLE_DEBUG) {
                SDL_Log("Player %s set alive=false at (%f, %f), willDie=true",
                        player == &player1 ? "1" : "2", player->pos.x, player->pos.y);
            }
            game->handlePlayerDeath(player, currentTimeSec); // Delegate scoring to Game
            continue;
        }

        // Check pixel collision
        if (player->hasMoved) {
            if (framebuffer.empty()) {
                if (config.ENABLE_DEBUG) {
                    SDL_Log("Warning: Framebuffer empty for player %s collision check", player == &player1 ? "1" : "2");
                }
            } else {
                game->checkCollision(player, nextPos, currentTimeSec, framebuffer, drawableWidth, drawableHeight);
            }
        }

        // Check wall collision only if not already dying
        if (!player->willDie && (nextPos.x < 10 || nextPos.x > game->orthoWidth - 10 ||
                                 nextPos.y < 10 || nextPos.y > game->orthoHeight - 10)) {
            player->willDie = true;
            explosions.emplace_back(explosionManager.createExplosion(nextPos, rng, currentTimeSec));
            audio.playExplosion(currentTimeSec);
            game->deathTime = currentTimeSec;
            if (config.ENABLE_DEBUG) {
                SDL_Log("Player %s hit wall at (%f, %f), explosion triggered, willDie=true, orthoWidth=%f, orthoHeight=%f",
                        player == &player1 ? "1" : "2", nextPos.x, nextPos.y, game->orthoWidth, game->orthoHeight);
            }
        }

        // Update position and trail only if not dying
        if (!player->willDie) {
            player->pos = nextPos;
            player->trail.push_back(player->pos);
        }
    }

    circleManager.updateCircles(dt, circles, rng, currentTimeSec, lastCircleSpawn, *game);
    circleManager.clearTrails(circles, player1, player2);
    explosionManager.updateFlashes(flashes, currentTimeSec);
    explosionManager.cleanupPlayerFlashes(player1, player2, currentTimeSec);
}