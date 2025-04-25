// src/player.cpp
#include "player.h"
#include "game.h"
#include <GL/gl.h>

PlayerManager::PlayerManager(const GameConfig& config) : config(config) {}

void PlayerManager::updatePlayers(SDL_GameController* controllers[], int controllerCount, Player& player1, Player& player2, Collectible& collectible, std::vector<Explosion>& explosions, std::vector<Flash>& flashes, int& score1, int& score2, int& roundScore1, int& roundScore2, std::mt19937& rng, float dt, float currentTimeSec, AudioManager& audio, CollectibleManager& collectibleManager, ExplosionManager& explosionManager, CircleManager& circleManager, std::vector<Circle>& circles, std::chrono::steady_clock::time_point& lastCircleSpawn, Game* game) {
    for (int i = 0; i < controllerCount; ++i) {
        if (!controllers[i]) continue;
        Player& player = (i == 0) ? player1 : player2;
        if (!player.alive) continue;

        Sint16 leftTrigger = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        Sint16 rightTrigger = SDL_GameControllerGetAxis(controllers[i], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        if (leftTrigger > 0 || rightTrigger > 0) player.hasMoved = true;
        float turn = (rightTrigger - leftTrigger) / 32768.0f * config.TURN_SPEED * dt;
        float angle = atan2(player.direction.y, player.direction.x) + turn;
        player.direction = Vec2(cos(angle), sin(angle));

        if (player.noCollisionTimer > 0) {
            player.noCollisionTimer -= dt;
            player.isInvincible = true;
            if (player.noCollisionTimer <= 0) {
                player.isInvincible = false;
                player.endFlash = new Flash(explosionManager.createFlash(player.pos, rng, currentTimeSec, {255, 0, 255, 255}));
                audio.playLaserZap(currentTimeSec);
                SDL_Log("No-collision wore off for player %s at (%f, %f), magenta flash triggered", (&player == &player1 ? "1" : "2"), player.pos.x, player.pos.y);
            }
        } else {
            player.isInvincible = false;
        }
    }

    for (auto& player : {&player1, &player2}) {
        if (!player->alive) continue;

        Vec2 nextPos = player->pos + player->direction * config.PLAYER_SPEED * dt;
        if (!player->willDie) {
            if (nextPos.x < 0 || nextPos.x > config.WIDTH || nextPos.y < 0 || nextPos.y > config.HEIGHT) {
                player->willDie = true;
                explosions.emplace_back(explosionManager.createExplosion(nextPos, rng, currentTimeSec));
                audio.playExplosion(currentTimeSec);
                game->deathTime = currentTimeSec;
                SDL_Log("Player %s hit edge at (%f, %f), explosion triggered", player == &player1 ? "1" : "2", nextPos.x, nextPos.y);
            } else if (player->hasMoved) {
                game->checkCollision(player, nextPos, currentTimeSec);
            }
        }
        if (player->willDie) {
            player->alive = false;
            player->deathPos = player->pos;
            if (player == &player1 && player2.alive) {
                score2 += 3;
                roundScore2 += 3;
                SDL_Log("Player 1 died, score2 += 3");
            } else if (player == &player2 && player1.alive) {
                score1 += 3;
                roundScore1 += 3;
                SDL_Log("Player 2 died, score1 += 3");
            }
            continue;
        }

        player->pos = nextPos;
        player->trail.push_back(player->pos);

        if (collectibleManager.checkCollectibleCollision(player->pos, collectible)) {
            if (player == &player1) {
                score1++;
                roundScore1++;
                audio.playBoop(currentTimeSec);
                SDL_Log("Player 1 collected at (%f, %f), boop triggered", player->pos.x, player->pos.y);
            } else {
                score2++;
                roundScore2++;
                audio.playBoop(currentTimeSec);
                SDL_Log("Player 2 collected at (%f, %f), boop triggered", player->pos.x, player->pos.y);
            }
            collectible = collectibleManager.spawnCollectible(rng);
        }
    }

    circleManager.updateCircles(dt, circles, rng, currentTimeSec, lastCircleSpawn);
    circleManager.clearTrails(circles, player1, player2);
    explosionManager.updateFlashes(flashes, currentTimeSec);
    explosionManager.cleanupPlayerFlashes(player1, player2, currentTimeSec);
}