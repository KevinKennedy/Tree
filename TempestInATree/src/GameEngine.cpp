#include "GameEngine.h"

#include <vector>
#include <memory>
#include <stdlib.h>
#include <string.h>

#define ARRAYSIZE(a) ((int)(sizeof(a) / sizeof(*a)))

#ifndef _ASSERT
#define _ASSERT(exp)
#endif

GameEngine::GameEngine()
{
    // start led index, end led index, count - calculated, player path led index of end
    lanes[0] = {215, 195, -1, 0};
    lanes[1] = {170, 190, -1, 5};
    lanes[2] = {170, 150, -1, 10};
    lanes[3] = {125, 145, -1, 15};
    lanes[4] = {125, 105, -1, 20};
    lanes[5] = { 80, 100, -1, 25};
    lanes[6] = { 80, 60,  -1, 29};
    _ASSERT(ARRAYSIZE(lanes) == 6 + 1);
    _ASSERT(ARRAYSIZE(lanes) == laneCount);
    // update the count of leds
    for (int i = 0; i < ARRAYSIZE(lanes); ++i)
    {
        lanes[i].count = abs(lanes[i].endIndex - lanes[i].startIndex) + 1;
    }

    // whiteEnemyCount, redEnemyCount, greenEnemyCount, fireRateMultiplier, speedMultiplier
    levels[0] = { 15,  0, 0,  1.0f, 1.0f };
    levels[1] = { 15, 10, 0,  1.0f, 1.0f };
    levels[2] = { 15, 10, 5,  1.0f, 1.0f };
    levels[3] = { 20, 20, 10, 2.0f, 2.0f };
    levels[4] = { 30, 30, 10, 3.0f, 3.0f };
    _ASSERT(ARRAYSIZE(levels) == 4 + 1);
    _ASSERT(ARRAYSIZE(levels) == levelCount);
}

void GameEngine::Reset(TickCount time)
{
    gameState = GS_STARTING_ANIMATION;
    stepTime = time;
    levelIndex = 0;
    score = 0;
    livesRemaining = startingLifeCount;
    playerPosition = 0.5;
    
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

    switch (gameState)
    {
        case GS_STARTING_ANIMATION:
            // TODO - maybe some animation before starting the next level
            StartLevel();
            break;
        case GS_PLAYING_LEVEL:
            StepLevel();
            break;
        case GS_LIFE_LOST_ANIMATION:
            break;
        case GS_NEXT_LEVEL_ANIMATION:
            break;
        case GS_GAME_OVER_ANIMATION:
            break;
        case GS_GAME_OVER:
            break;
    }
}

void GameEngine::StartLevel()
{
    gameState = GS_PLAYING_LEVEL;
}

void GameEngine::StepLevel()
{

    // Update everything's position

    // Spawn new enemies

    // Fire a shot if requested

    // Check for collisions
        // player shot and enemy - remove the shot and the enemy and count the hit
        // player shot and enemy shot - remove the shot and the enemy shot - is this worth any points?
        // enemy shot and player or enemy and player
            // decrenemt life count, remove all shots, move enemies to the start of the lanes, reset the enemy spawn time, start the player died animation

    // Has the player killed all the enemies?  If so, do the next level animation.  If we do spikes, the player may lose lives during that animation

    // is the player out of lives?  Start the game-over animation


}

void GameEngine::SetLeds(std::vector<LedColor>& leds) const
{
    // assumes caller set leds to all 0 before calling

    FillLedRange(leds, treeBaseStartLedIndex, treeBaseEndLedIndex, color_red);
    FillLedRange(leds, pathStartLedIndex, pathEndLedIndex, color_blue);

    leds[LedIndexFromRange(pathStartLedIndex, pathEndLedIndex, playerPosition)] = color_yellow;

    for (int laneIndex = 0; laneIndex < ARRAYSIZE(lanes); ++laneIndex)
    {
        const Lane* pLane = lanes + laneIndex;
        FillLedRange(leds, pLane->startIndex, pLane->endIndex, color_blue);
    }

    for (int shotIndex = 0; shotIndex < ARRAYSIZE(shots) && shots[shotIndex].IsValid(); ++shotIndex)
    {
        const Shot* pShot = shots + shotIndex;
        LedIndex ledIndex = lanes[pShot->laneIndex].GetLedIndex(pShot->progress);
        leds[ledIndex] = pShot->player ? color_white : color_cyan;
    }

    for (int enemyIndex = 0; enemyIndex < ARRAYSIZE(enemies) && enemies[enemyIndex].IsValid(); ++enemyIndex)
    {
        const Enemy* pEnemy = enemies + enemyIndex;
        LedIndex ledIndex = lanes[pEnemy->laneIndex].GetLedIndex(pEnemy->progress);
        leds[ledIndex] = pEnemy->color;
    }
    
}



bool GameEngine::GetIsGameOver() const
{
    return false;
}

