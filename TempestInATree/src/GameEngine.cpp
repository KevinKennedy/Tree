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
    gameState = GameState::GS_STARTING_ANIMATION;
    stepTime = time;
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
    stepTime = time;
    stepPlayerPosition = static_cast<float>(playerPosition) / (float)pathLedCount;
    stepFireButtonPressed = fireButtonPressed;

    switch (gameState)
    {
        case GameState::GS_STARTING_ANIMATION:
            // TODO - maybe some animation before starting the next level
            StartLevel();
            break;
        case GameState::GS_PLAYING_LEVEL:
            StepLevel();
            break;
        case GameState::GS_LIFE_LOST_ANIMATION:
            break;
        case GameState::GS_NEXT_LEVEL_ANIMATION:
            break;
        case GameState::GS_GAME_OVER_ANIMATION:
            break;
        case GameState::GS_GAME_OVER:
            break;
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

    // Update everything's position
    // shots
    for (int iShot = 0; iShot < ARRAYSIZE(shots); ++iShot)
    {
        Shot* pShot = shots + iShot;
        if (!pShot->IsValid()) continue;
        pShot->lanePosition = (float)((stepTime - pShot->startTime) / (float)TicksPerSecond) / pShot->speed;
        if (pShot->player)
        {
            pShot->lanePosition = 1.0f - pShot->lanePosition;
        }
    }
    // enemies
    for (int iEnemy = 0; iEnemy < ARRAYSIZE(enemies); ++iEnemy)
    {
        Enemy* pEnemy = enemies + iEnemy;
        if (!pEnemy->IsValid()) continue;
        pEnemy->lanePosition = ((float)(stepTime - pEnemy->startTime) / (float)TicksPerSecond) * pEnemy->speed;
        if (pEnemy->lanePosition > 1.0f) pEnemy->lanePosition = 1.0f;
    }

    // Spawn new enemies
    SpawnNextEnemy();

    // Have enemies shoot if it's time

    // Fire a shot if requested
    if (stepFireButtonPressed)
    {
        if(fireButtonWasReleased)
        {
            int laneIndex = GetClosestLaneToPathPosition(stepPlayerPosition);
            AddShot(true, laneIndex, shotSpeed);
            fireButtonWasReleased = false;
        }
    }
    else
    {
        fireButtonWasReleased = true;
    }
    

    // Check for collisions
        // player shot and enemy - remove the shot and the enemy and count the hit
        // player shot and enemy shot - remove the shot and the enemy shot - is this worth any points?

    // player shot and start of lane - remove the shot
    // enemy shot and end of lane - remove the shot
    for (Shot* pShot = shots; pShot <= shots + ARRAYSIZE(shots); ++pShot)
    {
        if (pShot->IsValid())
        {
            if ((pShot->player && pShot->lanePosition < 0.0f) ||
                (!pShot->player && pShot->lanePosition > 1.0f))
            {
                pShot->Invalidate();
            }
        }
    }
        // enemy shot and player or enemy and player
            // decrenemt life count, remove all shots, move enemies to the start of the lanes, reset the enemy spawn time, start the player died animation

    // Has the player killed all the enemies?  If so, do the next level animation.  If we do spikes, the player may lose lives during that animation

    // is the player out of lives?  Start the game-over animation


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
            pNew->shotDelta = 1 * TicksPerSecond;
            pNew->startTime = stepTime;
            pNew->laneIndex = rand() % ARRAYSIZE(lanes);
            pNew->lanePosition = 0.0f;

            switch (static_cast<EnemyType>(enemyIndex))
            {
                case EnemyType::ET_WHITE:
                    pNew->speed = 0.3f;
                    pNew->color = color_white;
                    pNew->laneSwitching = false;
                    pNew->nextLaneSwitchTime = 0;
                    break;
                case EnemyType::ET_RED:
                    pNew->speed = 0.4f;
                    pNew->color = color_white;
                    pNew->laneSwitching = true;
                    pNew->nextLaneSwitchTime = 0;
                    break;
                case EnemyType::ET_GREEN:
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

void GameEngine::AddShot(bool isPlayer, int laneIndex, float speed)
{
    for (int iShot = 0; iShot < ARRAYSIZE(shots); ++iShot)
    {
        Shot* pShot = shots + iShot;
        if (!pShot->IsValid())
        {
            pShot->player = isPlayer;
            pShot->laneIndex = laneIndex;
            pShot->speed = speed; // lane-lengths per second
            pShot->startTime = stepTime;
            pShot->lanePosition = isPlayer ? 1.0f : 0.0f;
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

    FillLedRange(leds, treeBaseStartLedIndex, treeBaseEndLedIndex, color_red);
    FillLedRange(leds, pathLeftLedIndex, pathRightLedIndex, color_blue);

    leds[LedIndexFromRange(pathLeftLedIndex, pathRightLedIndex, stepPlayerPosition)] = color_yellow;

    for (int laneIndex = 0; laneIndex < ARRAYSIZE(lanes); ++laneIndex)
    {
        const Lane* pLane = lanes + laneIndex;
        FillLedRange(leds, pLane->startIndex, pLane->endIndex, color_blue);
    }

    for (int shotIndex = 0; shotIndex < ARRAYSIZE(shots); ++shotIndex)
    {
        const Shot* pShot = shots + shotIndex;
        if (!pShot->IsValid()) continue;
        LedIndex ledIndex = lanes[pShot->laneIndex].GetLedIndex(pShot->lanePosition);
        leds[ledIndex] = pShot->player ? color_white : color_cyan;
    }

    for (int enemyIndex = 0; enemyIndex < ARRAYSIZE(enemies) && enemies[enemyIndex].IsValid(); ++enemyIndex)
    {
        const Enemy* pEnemy = enemies + enemyIndex;
        if (!pEnemy->IsValid()) continue;
        LedIndex ledIndex = lanes[pEnemy->laneIndex].GetLedIndex(pEnemy->lanePosition);
        leds[ledIndex] = pEnemy->color;
    }
    
}



bool GameEngine::GetIsGameOver() const
{
    return false;
}

