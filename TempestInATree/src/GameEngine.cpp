#include <vector>
#include <memory>
#include <stdlib.h>
#include <string.h>

#include "GameEngine.h"



#ifndef _ASSERT
#define _ASSERT(exp)
#include <spark_wiring_logging.h>
#endif

GameEngine::GameEngine()
{
    // start led index, end led index, count - calculated, player path led index of end
    lanes[0] = {99,  56,  -1, 0};
    lanes[1] = {100, 142, -1, 5};
    lanes[2] = {188, 147, -1, 10};
    lanes[3] = {189, 229, -1, 14};
    lanes[4] = {274, 233, -1, 18};
    lanes[5] = {275, 315, -1, 23};
    lanes[6] = {363, 320, -1, 27};
    _ASSERT(ARRAYSIZE(lanes) == 6 + 1);
    _ASSERT(ARRAYSIZE(lanes) == laneCount);
    // update the count of leds
    for (int i = 0; i < ARRAYSIZE(lanes); ++i)
    {
        lanes[i].count = abs(lanes[i].endIndex - lanes[i].startIndex) + 1;
    }

    // whiteEnemyCount, redEnemyCount, greenEnemyCount, fireRateMultiplier, speedMultiplier, enemySpawnDelta
    levels[0] = { 15,  0, 0,  1.0f, 1.0f, 2.0f };
    levels[1] = { 15, 10, 0,  1.0f, 1.0f, 2.0f };
    levels[2] = { 15, 10, 5,  1.0f, 1.0f, 1.0f };
    levels[3] = { 20, 20, 10, 2.0f, 2.0f, 0.75f };
    levels[4] = { 30, 30, 10, 3.0f, 3.0f, 0.5f };
    _ASSERT(ARRAYSIZE(levels) == 4 + 1);
    _ASSERT(ARRAYSIZE(levels) == levelCount);
}

void GameEngine::Reset(TickCount time)
{
    stepTime = time;

    gameState = GameState::GS_GAME_START_ANIMATION;
    animationEndTime = stepTime + gameStartAnimationDuration;
    levelIndex = 0;
    score = 0;
    livesRemaining = startingLifeCount;
    stepPlayerPosition = 0.5;
    
    memset(shots, 0, sizeof(shots));
    for (int i = 0; i < ARRAYSIZE(shots); ++i)
    {
        shots[i].Invalidate();
    }

    memset(enemies, 0, sizeof(enemies));
    for (int i = 0; i < ARRAYSIZE(enemies); ++i)
    {
        enemies[i].Invalidate();
    }
}

void GameEngine::Start(TickCount time)
{
    Reset(time);
}

void GameEngine::Step(TickCount time, int playerPosition, bool fireButtonPressed)
{
    stepDeltaTicks = time - stepTime;
    stepDeltaSeconds = (float)stepDeltaTicks / (float)TicksPerSecond;
    stepTime = time;

    stepPlayerPosition = (float)(playerPosition) / (float)pathLedCount;
    stepFireButtonPressed = fireButtonPressed;

    switch (gameState)
    {
        case GameState::GS_GAME_START_ANIMATION:
            GameStartAnimation();
            break;
        case GameState::GS_LEVEL_START_ANIMATION:
            LevelStartAnimation();
            break;
        case GameState::GS_PLAYING_LEVEL:
            StepLevel();
            break;
        case GameState::GS_LIFE_LOST_ANIMATION:
            LifeLostAnimation();
            break;
        case GameState::GS_GAME_OVER_ANIMATION:
            GameOverAnimation();
            break;
        case GameState::GS_GAME_OVER:
            break;
    }
}

void GameEngine::GameStartAnimation()
{
    if(stepTime >= animationEndTime)
    {
        gameState = GameState::GS_LEVEL_START_ANIMATION;
        animationEndTime = stepTime + levelStartAnimationDuration;
    }
}

void GameEngine::LevelStartAnimation()
{
    if(stepTime >= animationEndTime)
    {
        StartLevel();
    }
}

void GameEngine::StartLevel()
{
    gameState = GameState::GS_PLAYING_LEVEL;
    nextEmenySpawnTime = stepTime;
    whiteEnemiesRemaining = levels[levelIndex].whiteEnemyCount;
    redEnemiesRemaining = levels[levelIndex].redEnemyCount;
    greenEnemiesRemaining = levels[levelIndex].greenEnemyCount;
    stepFireButtonPressed = false;
    fireButtonWasReleased = false;
}

void GameEngine::StepLevel()
{
    AdvanceGameObjects();

    SpawnNextEnemy();

    FireEnemyShots();

    // Fire a shot if requested
    if (stepFireButtonPressed)
    {
        if(fireButtonWasReleased)
        {
            int laneIndex = GetClosestLaneToPathPosition(stepPlayerPosition);
            AddShot(true, laneIndex, shotSpeed, 1.0f);
            fireButtonWasReleased = false;
        }
    }
    else
    {
        fireButtonWasReleased = true;
    }
    
    HandleCollisions();

    // Has the player killed all the enemies?  If so, do the next level animation.  If we do spikes, the player may lose lives during that animation


}

void GameEngine::AdvanceGameObjects()
{
    // shots
    for (Shot* pShot = shots; pShot < shots + ARRAYSIZE(shots); ++pShot)
    {
        if (!pShot->IsValid()) continue;
        pShot->lanePosition += stepDeltaSeconds * pShot->speed;
    }

    // enemies
    for (Enemy* pEnemy = enemies; pEnemy < enemies + ARRAYSIZE(enemies); ++pEnemy)
    {
        if (!pEnemy->IsValid()) continue;
        if(pEnemy->state == EnemyState::inLane)
        {
            pEnemy->lanePosition = ((float)(stepTime - pEnemy->startTime) / (float)TicksPerSecond) * pEnemy->speed;
            if (pEnemy->lanePosition > 1.0f)
            {
                // Enemy made it to the end of the lane, have it travel towards the player now
                pEnemy->lanePosition = 1.0f;
                pEnemy->state = EnemyState::onPlayerPath;
                pEnemy->startTime = stepTime;
                pEnemy->pathPosition = lanes[pEnemy->laneIndex].GetPathPosition();
            }
        }
        else
        {
            // enemy is in the EnemyState::onPlayerPath state
            float delta = stepDeltaSeconds * pEnemy->speed;
            delta *= (stepPlayerPosition > pEnemy->pathPosition) ? 1.0f : -1.0f;
            pEnemy->pathPosition += delta;
        }
    }
}

void GameEngine::SpawnNextEnemy()
{
    if (stepTime < nextEmenySpawnTime) return;

    int enemiesRemaining = 0;
    for (EnemyType et = EnemyType::ET_FIRST; et != EnemyType::ET_COUNT; et = NextEnemyType(et))
    {
        enemiesRemaining += GetEnemiesRemaining(et);
    }

    if (enemiesRemaining <= 0)
    {
        // All enemies have been spawned
        nextEmenySpawnTime = TickCountMax;
        return;
    }

    nextEmenySpawnTime = stepTime + (TickCount)(levels[levelIndex].enemySpawnDelta * TicksPerSecond);

    int newEnemyIndex = 0;
    while(newEnemyIndex < ARRAYSIZE(enemies) && enemies[newEnemyIndex].IsValid()) newEnemyIndex++;
    if(newEnemyIndex >= ARRAYSIZE(enemies))
    {
        // No space for new enemies.  Skip this spawn...
        return;
    }

    int startingEnemyIndex = rand() & static_cast<int>(EnemyType::ET_COUNT);
    for (int enemyIndex = startingEnemyIndex; ; enemyIndex = (enemyIndex + 1) % static_cast<int>(EnemyType::ET_COUNT))
    {
        int& enemyTypeRemainingToSpawn = GetEnemiesRemaining(static_cast<EnemyType>(enemyIndex));
        if (enemyTypeRemainingToSpawn > 0)
        {
            enemyTypeRemainingToSpawn--;


            Enemy* pNew = enemies + newEnemyIndex;
            *pNew = {};
            pNew->shotDelta = (TickCount)(1.5f * (float)TicksPerSecond);
            pNew->state = EnemyState::inLane;
            pNew->startTime = stepTime;
            pNew->laneIndex = rand() % ARRAYSIZE(lanes);
            pNew->lanePosition = 0.0f;
            pNew->pathPosition = 0.0f;

            switch (static_cast<EnemyType>(enemyIndex))
            {
                case EnemyType::ET_WHITE:
                    pNew->shotsRemaining = 1;
                    pNew->speed = 0.2f;
                    pNew->color = color_white;
                    pNew->laneSwitching = false;
                    pNew->nextLaneSwitchTime = 0;
                    break;
                case EnemyType::ET_RED:
                    pNew->shotsRemaining = 2;
                    pNew->speed = 0.3f;
                    pNew->color = color_red;
                    pNew->laneSwitching = true;
                    pNew->nextLaneSwitchTime = 0;
                    break;
                case EnemyType::ET_GREEN:
                    pNew->shotsRemaining = 3;
                    pNew->speed = 0.1f;
                    pNew->color = color_green;
                    pNew->laneSwitching = false;
                    pNew->nextLaneSwitchTime = 0;
                    break;
            }

            pNew->nextShotTime = stepTime + pNew->shotDelta;
            break;
        }
    }
}

void GameEngine::FireEnemyShots()
{
    for(Enemy* pEnemy = enemies; pEnemy < enemies + ARRAYSIZE(enemies); ++pEnemy)
    {
        if(!pEnemy->IsValid()) continue;

        if(pEnemy->nextShotTime <= stepTime && pEnemy->shotsRemaining > 1)
        {
            // Time to shoot
            if(pEnemy->state == EnemyState::inLane)
            {
                AddShot(false, pEnemy->laneIndex, pEnemy->speed * 1.5f, pEnemy->lanePosition); // TODO - maybe we need a shotSpeed for enemies
                pEnemy->shotsRemaining--;
            }
            pEnemy->nextShotTime = stepTime + pEnemy->shotDelta;

        }
    }
}

void GameEngine::HandleCollisions()
{
    // Check for collisions with player shots
    for(Shot* pPlayerShot = shots; pPlayerShot < shots + ARRAYSIZE(shots); ++pPlayerShot)
    {
        if(!pPlayerShot->IsValid() || !pPlayerShot->player) continue;

        // player shot and enemy - remove the shot and the enemy and count the hit
        for(Enemy* pEnemy = enemies; pEnemy < enemies + ARRAYSIZE(enemies); ++pEnemy)
        {
            if(!pEnemy->IsValid()) continue;

            bool enemyHit = false;
            if(EnemyState::inLane == pEnemy->state)
            {
                // Enemy is in a lane so check lane index and how close the shot is to the enemy
                if(pEnemy->laneIndex == pPlayerShot->laneIndex)
                {
                    enemyHit = abs(pEnemy->lanePosition - pPlayerShot->lanePosition) < shotEnemyCollisionThreshold;
                }
            }
            else
            {
                // When an enemy is on the player path, the player needs to have
                // just shot and the player must be close enough to the enemy.
                enemyHit = (pPlayerShot->lanePosition > shotEnemySideCollisionPositionThreshold) &&
                    (abs(pEnemy->pathPosition - stepPlayerPosition) < playerEnemySideDestroyPositionThreshold);
            }

            if(enemyHit)
            {
                // TODO - score the hit
                // TODO - sound maybe some day :-)
                pEnemy->Invalidate();
                pPlayerShot->Invalidate();
                break; // shot can only kill one enemy so don't look at more enemies and go to the next shot
            }
        }
        if(!pPlayerShot->IsValid()) continue;

        // player shot and enemy shot - remove the shot and the enemy shot - is this worth any points?
        for(Shot* pEnemyShot = shots; pEnemyShot < shots + ARRAYSIZE(shots); ++pEnemyShot)
        {
            if(!pEnemyShot->IsValid() || pEnemyShot->player) continue;
            if(pPlayerShot->laneIndex != pEnemyShot->laneIndex) continue;
            if(abs(pPlayerShot->lanePosition - pEnemyShot->lanePosition) < playerShotEnemyShotCollisionThreshold)
            {
                // TODO - score ?
                pPlayerShot->Invalidate();
                pEnemyShot->Invalidate();
                break; // player shot can only hit one enemy shot even if the're very close
            }
        }
        if(!pPlayerShot->IsValid()) continue;

        // Player shot goes away when it hits the start of the lane
        if(pPlayerShot->lanePosition <= 0.0f)
        {
            pPlayerShot->Invalidate();
        }
    }

    bool playerDied = false;

    for (Shot* pEnemyShot = shots; pEnemyShot < shots + ARRAYSIZE(shots); ++pEnemyShot)
    {
        if (!pEnemyShot->IsValid() || pEnemyShot->player) continue;

        // enemy shot and end of lane - see if it hit the player - and remove the shot
        if(pEnemyShot->lanePosition >= 1.0f)
        {
            if(abs(lanes[pEnemyShot->laneIndex].GetPathPosition() - stepPlayerPosition) < playerEnemyShotCollisionThreshold)
            {
                playerDied = true;
            }

            pEnemyShot->Invalidate();
        }
    }


    if(!playerDied)
    {
        for(Enemy* pEnemy = enemies; pEnemy < enemies + ARRAYSIZE(enemies); pEnemy++)
        {
            if(!pEnemy->IsValid()) continue;
            if(pEnemy->state == EnemyState::onPlayerPath)
            {
                if(abs(pEnemy->pathPosition - stepPlayerPosition) < playerEnemyCollisionThreshold)
                {
                    playerDied = true;
                    pEnemy->Invalidate();
                }
            }
        }
    }

    if(playerDied)
    {
        livesRemaining--;
        for(Shot* pShot = shots; pShot < shots + ARRAYSIZE(shots); ++pShot)
        {
            pShot->Invalidate();
        }
        for(Enemy* pEnemy = enemies; pEnemy < enemies + ARRAYSIZE(enemies); ++pEnemy)
        {
            pEnemy->Invalidate();
        }

        gameState = GameState::GS_LIFE_LOST_ANIMATION;
        animationEndTime = stepTime + lifeLostAnimationDuration;
    }
}

void GameEngine::LifeLostAnimation()
{
    if(stepTime >= animationEndTime)
    {
        if(livesRemaining > 0)
        {
            gameState = GameState::GS_PLAYING_LEVEL;
        }
        else
        {
            gameState = GameState::GS_GAME_OVER_ANIMATION;
            animationEndTime = stepTime + gameOverAnimationDuration;
        }
    }
}

void GameEngine::GameOverAnimation()
{
    if(stepTime >= animationEndTime)
    {
        gameState = GameState::GS_GAME_OVER;
    }
}

int& GameEngine::GetEnemiesRemaining(EnemyType et)
{
    switch (et)
    {
        case EnemyType::ET_WHITE: return whiteEnemiesRemaining;
        case EnemyType::ET_RED: return redEnemiesRemaining;
        case EnemyType::ET_GREEN: return greenEnemiesRemaining;
        default: return whiteEnemiesRemaining; // mainly to shut up the compiler warning
    }
}

void GameEngine::AddShot(bool isPlayer, int laneIndex, float speed, float startingLanePosition)
{
    for (Shot* pShot = shots; pShot < shots + ARRAYSIZE(shots); ++pShot)
    {
        if (!pShot->IsValid())
        {
            pShot->player = isPlayer;
            pShot->laneIndex = laneIndex;
            pShot->speed = speed * (isPlayer ? -1.0f : 1.0f); // lane-lengths per second
            pShot->lanePosition = startingLanePosition;
            break;
        }
    }
}

int GameEngine::GetClosestLaneToPathPosition(float pathPosition) const
{
    int ledIndex = LedIndexFromRange(pathLeftLedIndex, pathRightLedIndex, pathPosition);
    if(pathLeftLedIndex < pathRightLedIndex)
    {
        ledIndex -= pathLeftLedIndex;
    }
    else
    {
        ledIndex = -(ledIndex - pathLeftLedIndex);
    }
    
    int closestLaneIndex = -1;
    int closestLaneDistance = INT32_MAX;
    for (int laneIndex = 0; laneIndex < ARRAYSIZE(lanes); ++laneIndex)
    {
        int laneDistance = abs(lanes[laneIndex].pathLedIndex - ledIndex);
        if (laneDistance < closestLaneDistance)
        {
            closestLaneDistance = laneDistance;
            closestLaneIndex = laneIndex;
        }
    }

    // should fail if closestLaneIndex is still -1 but that should never happen right? :-)

    return closestLaneIndex;
}


void GameEngine::SetLeds(std::vector<LedColor>& leds) const
{
    // assumes caller set leds to all 0 before calling

    LedColor laneColor = color_blue;

    if(GameState::GS_GAME_START_ANIMATION == gameState)
    {
        laneColor = color_pink;
    }
    if(GameState::GS_LEVEL_START_ANIMATION == gameState)
    {
        laneColor = color_yellow;
    }
    if(gameState == GameState::GS_LIFE_LOST_ANIMATION)
    {
        laneColor = (stepTime % 200) < 50 ? color_black : color_blue;
    }
    else if(gameState == GameState::GS_GAME_OVER_ANIMATION)
    {
        laneColor = color_red;
    }
    else if(gameState == GameState::GS_GAME_OVER)
    {
        laneColor = color_green;
    }


    FillLedRange(leds, treeBaseStartLedIndex, treeBaseStartLedIndex + livesRemaining, color_yellow); // off-by-one but simpler logic :-)
    FillLedRange(leds, treeBaseStartLedIndex + livesRemaining, treeBaseEndLedIndex, color_red);
    FillLedRange(leds, pathLeftLedIndex, pathRightLedIndex, color_blue);

    leds[LedIndexFromRange(pathLeftLedIndex, pathRightLedIndex, stepPlayerPosition)] = color_yellow;

    for (const Lane* pLane = lanes; pLane < lanes + ARRAYSIZE(lanes); ++pLane)
    {
        FillLedRange(leds, pLane->startIndex, pLane->endIndex, laneColor);
    }

    for (const Shot* pShot = shots; pShot < shots + ARRAYSIZE(shots); ++pShot)
    {
        if (!pShot->IsValid()) continue;
        LedIndex ledIndex = lanes[pShot->laneIndex].GetLedIndex(pShot->lanePosition);
        leds[ledIndex] = pShot->player ? color_white : color_cyan;
    }

    for (const Enemy* pEnemy = enemies; pEnemy < enemies + ARRAYSIZE(enemies); ++pEnemy)
    {
        if (!pEnemy->IsValid()) continue;

        LedIndex ledIndex;
        if(pEnemy->state == EnemyState::inLane)
        {
            ledIndex = lanes[pEnemy->laneIndex].GetLedIndex(pEnemy->lanePosition);
        }
        else
        {
            ledIndex = LedIndexFromRange(pathLeftLedIndex, pathRightLedIndex, pEnemy->pathPosition);
        }

        leds[ledIndex] = pEnemy->color;
    }
    
}



bool GameEngine::GetIsGameOver() const
{
    return false;
}

