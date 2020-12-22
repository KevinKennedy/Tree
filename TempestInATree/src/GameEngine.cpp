#include <vector>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
    levels[1] = { 15, 10, 0,  1.0f, 1.0f, 1.0f };
    levels[2] = { 15, 10, 5,  2.0f, 1.0f, 0.75f };
    levels[3] = { 20, 20, 10, 3.0f, 2.5f, 0.5f };
    levels[4] = { 30, 30, 10, 4.0f, 3.5f, 0.5f };
    _ASSERT(ARRAYSIZE(levels) == 4 + 1);
    _ASSERT(ARRAYSIZE(levels) == levelCount);

    Animator* pGameStartAnimator = new TreeTransitionAnimator(2000, lanes, ARRAYSIZE(lanes), color_black, color_blue);
    Animator* pAttractAnimator = new AttractAnimator(treeBaseStartLedIndex, treeBaseEndLedIndex, pathLeftLedIndex, pathRightLedIndex, lanes, ARRAYSIZE(lanes));
    Animator* pGameOverAnimator = new TreeTransitionAnimator(2000, lanes, ARRAYSIZE(lanes), color_blue, color_black);

    animatedStates[0] = {GameState::GS_ATTRACT_ANIMATION, pAttractAnimator->duration(), true, pAttractAnimator};
    animatedStates[1] = {GameState::GS_GAME_START_ANIMATION, pGameStartAnimator->duration(), false, pGameStartAnimator};
    animatedStates[2] = {GameState::GS_LEVEL_START_ANIMATION, 4000, false, NULL};
    animatedStates[3] = {GameState::GS_LIFE_LOST_ANIMATION, 1000, false, NULL};
    animatedStates[4] = {GameState::GS_GAME_OVER_ANIMATION, pGameOverAnimator->duration(), false, pGameOverAnimator};
    _ASSERT(ARRAYSIZE(animatedStates) == 4 + 1);

    StartAttractAnimation();
}

void GameEngine::Step(TickCount time, int playerPosition, bool fireButtonPressed, bool startButtonPressed)
{
    stepDeltaTicks = time - stepTime;
    stepDeltaSeconds = (float)stepDeltaTicks / (float)TicksPerSecond;
    stepTime = time;

    stepPlayerPosition = (float)(playerPosition) / (float)pathLedCount;
    stepFireButtonPressed = fireButtonPressed;

    if(startButtonPressed)
    {
        if(startButtonWasReleased)
        {
            startButtonWasReleased = false;
            if(GameState::GS_ATTRACT_ANIMATION == gameState)
            {
                BeginAnimatedState(GameState::GS_GAME_START_ANIMATION);
            }
            else
            {
                StartAttractAnimation();
            }
            return;
        }
    }
    else
    {
        startButtonWasReleased = true;
    }
    

    switch (gameState)
    {
        case GameState::GS_ATTRACT_ANIMATION:
        case GameState::GS_GAME_START_ANIMATION:
        case GameState::GS_LEVEL_START_ANIMATION:
        case GameState::GS_LIFE_LOST_ANIMATION:
        case GameState::GS_GAME_OVER_ANIMATION:
            StepAnimatedState();
            break;
        case GameState::GS_PLAYING_LEVEL:
            StepLevel();
            break;
    }
}

void GameEngine::StartAttractAnimation()
{
    BeginAnimatedState(GameState::GS_ATTRACT_ANIMATION);
    currentLevelIndex = 0;
    score = 0;
    livesRemaining = startingLifeCount;
    stepPlayerPosition = 0.5;

    ResetShotsAndEnemies();
}

void GameEngine::ResetShotsAndEnemies()
{
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

void GameEngine::StartLevel()
{
    gameState = GameState::GS_PLAYING_LEVEL;
    nextEmenySpawnTime = stepTime;
    whiteEnemiesRemaining = levels[currentLevelIndex].whiteEnemyCount;
    redEnemiesRemaining = levels[currentLevelIndex].redEnemyCount;
    greenEnemiesRemaining = levels[currentLevelIndex].greenEnemyCount;
    stepFireButtonPressed = false;
    fireButtonWasReleased = false;
    ResetShotsAndEnemies();
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

    HandleLevelCompleted();
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
        if(!pEnemy->IsValid()) continue;
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

    nextEmenySpawnTime = stepTime + (TickCount)(levels[currentLevelIndex].enemySpawnDelta * TicksPerSecond);

    int newEnemyIndex = 0;
    while(newEnemyIndex < ARRAYSIZE(enemies) && enemies[newEnemyIndex].IsValid()) newEnemyIndex++;
    if(newEnemyIndex >= ARRAYSIZE(enemies))
    {
        // No space for new enemies.  Skip this spawn...
        return;
    }

    int startingEnemyIndex = rand() % static_cast<int>(EnemyType::ET_COUNT);
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
                    pNew->laneSwitching = false;
                    pNew->nextLaneSwitchTime = 0;
                    break;
                case EnemyType::ET_GREEN:
                    pNew->shotsRemaining = 4;
                    pNew->speed = 0.4f;
                    pNew->color = color_green;
                    pNew->laneSwitching = true;
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

    // See if player was hit by an enemy shot
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

    // See if an enemy ran into the player
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

    // Handle the player dieing
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

        BeginAnimatedState(GameState::GS_LIFE_LOST_ANIMATION);
    }
}

void GameEngine::HandleLevelCompleted()
{
    if(whiteEnemiesRemaining > 0 || redEnemiesRemaining > 0 || greenEnemiesRemaining > 0) return;
    for(const Enemy* pEnemy = enemies; pEnemy < enemies + ARRAYSIZE(enemies) ; ++pEnemy)
    {
        if(pEnemy->IsValid())
        {
            return;
        }
    }

    // All enemies have been spawned and there are no enemies on the board
    // so go to the next level
    currentLevelIndex++;
    if(currentLevelIndex >= ARRAYSIZE(levels))
    {
        currentLevelIndex = ARRAYSIZE(levels) - 1;
    }

    BeginAnimatedState(GameState::GS_LEVEL_START_ANIMATION);
}


void GameEngine::BeginAnimatedState(GameState newState)
{
    for(AnimatedState* pAnimatedState = animatedStates; pAnimatedState < animatedStates + ARRAYSIZE(animatedStates); ++pAnimatedState)
    {
        if(pAnimatedState->state == newState)
        {
            gameState = newState;
            animationStartTime = stepTime;
            animationEndTime = stepTime + pAnimatedState->duration;
            pCurrentAnimatedState = pAnimatedState;
            return;
        }
    }
    // ERROR if we're here
}

void GameEngine::StepAnimatedState()
{
    if(NULL != pCurrentAnimatedState)
    {
        if(stepTime >= animationEndTime)
        {
            if(pCurrentAnimatedState->loop)
            {
                animationStartTime = stepTime;
                animationEndTime = stepTime + pCurrentAnimatedState->duration;
            }
            else
            {
                // Animated state just ended.  Go to the next state.
                // We could use lambdas in the AnimatedState structure for this
                // but some of the embedded compilers had trouble with this stuff
                // in the past.
                pCurrentAnimatedState = NULL;

                switch(gameState)
                {
                    case GameState::GS_GAME_START_ANIMATION:
                        StartLevel();
                        break;

                    case GameState::GS_LEVEL_START_ANIMATION:
                        StartLevel();
                        break;

                    case GameState::GS_LIFE_LOST_ANIMATION:
                        if(livesRemaining > 0)
                        {
                            gameState = GameState::GS_PLAYING_LEVEL;
                        }
                        else
                        {
                            BeginAnimatedState(GameState::GS_GAME_OVER_ANIMATION);
                        }
                        break;

                    case GameState::GS_GAME_OVER_ANIMATION:
                        // Go into attract mode
                        StartAttractAnimation();
                        break;
                    
                    default:
                        // ERROR
                        break;
                }
            }
        }
    }
    // ERROR
}

void GameEngine::SetAnimatedStateLeds(LedColor* pLeds) const
{
    if(NULL != pCurrentAnimatedState)
    {
        if(NULL != pCurrentAnimatedState->pAnimator)
        {
            pCurrentAnimatedState->pAnimator->Step(stepTime - animationStartTime, pLeds);
        }
        else
        {
            switch (pCurrentAnimatedState->state)
            {
                case GameState::GS_LEVEL_START_ANIMATION:
                    SetLevelStartAnimationLeds(stepTime - animationStartTime, pLeds);
                    break;
                
                default:
                    // ERROR
                    break;
            }
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

void GameEngine::AddShot(bool isPlayer, int laneIndex, float speed, float startingLanePosition)
{
    Shot* pShot;
    Shot* pAvaliableShot = NULL;
    int playerShotCount;

    for (pShot = shots, playerShotCount = 0; pShot < shots + ARRAYSIZE(shots); ++pShot)
    {
        if (pShot->IsValid())
        {
            if(isPlayer && pShot->player)
            {
                if(++playerShotCount >= maxPlayerShots)
                {
                    return;
                }
            }
        }
        else
        {
            pAvaliableShot = pShot;
        }
    }

    if(NULL != pAvaliableShot)
    {
        pAvaliableShot->player = isPlayer;
        pAvaliableShot->laneIndex = laneIndex;
        pAvaliableShot->speed = speed * (isPlayer ? -1.0f : 1.0f); // lane-lengths per second
        pAvaliableShot->lanePosition = startingLanePosition;
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


void GameEngine::SetLeds(LedColor* pLeds) const
{
    // assumes caller set leds to all 0 before calling

    LedColor laneColor = color_blue;

    if(GameState::GS_GAME_START_ANIMATION == gameState)
    {
        SetAnimatedStateLeds(pLeds);
        return;
    }
    if(GameState::GS_LEVEL_START_ANIMATION == gameState)
    {
        SetAnimatedStateLeds(pLeds);
        return;
    }
    if(gameState == GameState::GS_LIFE_LOST_ANIMATION)
    {
        laneColor = (stepTime % 200) < 50 ? color_black : color_blue;
    }
    else if(gameState == GameState::GS_GAME_OVER_ANIMATION)
    {
        SetAnimatedStateLeds(pLeds);
        return;
    }
    else if(gameState == GameState::GS_ATTRACT_ANIMATION)
    {
        SetAnimatedStateLeds(pLeds);
        return;
    }


    FillLedRange(pLeds, treeBaseStartLedIndex, treeBaseEndLedIndex, color_red);
    if(livesRemaining > 0)
    {
        FillLedRange(pLeds, treeBaseStartLedIndex, treeBaseStartLedIndex + livesRemaining - 1, color_yellow);
    }
    FillLedRange(pLeds, treeBaseEndLedIndex, treeBaseEndLedIndex - currentLevelIndex, color_cyan);
    
    
    FillLedRange(pLeds, pathLeftLedIndex, pathRightLedIndex, color_blue);

    pLeds[LedIndexFromRange(pathLeftLedIndex, pathRightLedIndex, stepPlayerPosition)] = color_yellow;

    for (const Lane* pLane = lanes; pLane < lanes + ARRAYSIZE(lanes); ++pLane)
    {
        FillLedRange(pLeds, pLane->startIndex, pLane->endIndex, laneColor);
    }

    for (const Shot* pShot = shots; pShot < shots + ARRAYSIZE(shots); ++pShot)
    {
        if (!pShot->IsValid()) continue;
        LedIndex ledIndex = lanes[pShot->laneIndex].GetLedIndex(pShot->lanePosition);
        pLeds[ledIndex] = pShot->player ? color_white : color_cyan;
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

        pLeds[ledIndex] = pEnemy->color;
    }
    
}

void GameEngine::SetLevelStartAnimationLeds(TickCount animationTime, LedColor* pLeds) const
{
    FillLedRange(pLeds, treeBaseStartLedIndex, treeBaseEndLedIndex, color_black);
    FillLedRange(pLeds, pathLeftLedIndex, pathRightLedIndex, color_black);

    const TickCount levelStartAnimationDuration = 4000;
    const TickCount firstPartDuration = levelStartAnimationDuration / 3;
    const TickCount secondPartDuration = levelStartAnimationDuration - firstPartDuration;

    if(animationTime <= firstPartDuration)
    {
        float t = (float)animationTime / (float)firstPartDuration;
        //spark::Log("animation Time: %d    t:%f", animationTime, t);
        for (const Lane* pLane = lanes; pLane < lanes + ARRAYSIZE(lanes); ++pLane)
        {
            ColorWipeLed(pLeds, pLane->startIndex, pLane->endIndex, color_black, color_blue, t);
        }
    }
    else if(animationTime < levelStartAnimationDuration)
    {
        float t = (float)(animationTime - firstPartDuration) / (float)secondPartDuration;

        for (const Lane* pLane = lanes; pLane < lanes + ARRAYSIZE(lanes); ++pLane)
        {
            ColorWipeLed(pLeds, pLane->startIndex, pLane->endIndex, color_blue, color_black, t);
        }
        
    }
    

}

GameEngine::TreeTransitionAnimator::TreeTransitionAnimator(TickCount duration, const Lane* pLanes, int laneCount, LedColor colorStart, LedColor colorEnd)
{
    duration_ = duration;

    // This animator fades each lane in sequence from the start color to the end color
    TickCount laneTime = duration / laneCount;
    TickCount halfLaneTime = laneTime / 2;


    AnimatorGroup* pGroup = new AnimatorGroup();

    for(int laneIndex = 0; laneIndex < laneCount; ++laneIndex)
    {
        const Lane* pLane = pLanes + laneIndex;
        int startLedIndex;
        int ledCount;
        if(pLane->startIndex < pLane->endIndex)
        {
            startLedIndex = pLane->startIndex;
            ledCount = (pLane->endIndex - pLane->startIndex) + 1;
        }
        else
        {
            startLedIndex = pLane->endIndex;
            ledCount = (pLane->startIndex - pLane->endIndex) + 1;
        }
        
        TickCount startColorTime = (laneTime * laneIndex) + halfLaneTime;
        TickCount endColorTime = (laneTime * (laneCount - laneIndex - 1)) + halfLaneTime;
        auto pSolidStart = new SolidColor(startColorTime, startLedIndex, ledCount, colorStart);
        auto pFadeStart = new FadeAnimator(0, startColorTime - halfLaneTime, halfLaneTime,  startLedIndex, ledCount, pSolidStart);
        auto pSolidEnd = new SolidColor(endColorTime, startLedIndex, ledCount, colorEnd);
        auto pFadeEnd = new FadeAnimator(halfLaneTime, endColorTime - halfLaneTime, 0, startLedIndex, ledCount, pSolidEnd);
        pGroup->AddAnimator(pFadeStart, 0);
        pGroup->AddAnimator(pFadeEnd, startColorTime);
    }

    pRootAnimator = pGroup;
}

void GameEngine::TreeTransitionAnimator::Step(TickCount localTime, LedColor* pColors)
{
    pRootAnimator->Step(localTime, pColors);
}

GameEngine::AttractAnimator::AttractAnimator(int treeBaseStartLedIndex, int treeBaseEndLedIndex, int pathLeftLedIndex, int pathRightLedIndex, const Lane* pLanes, int laneCount)
{
    int startLedIndex;
    int ledCount;

    // Attract animator right now is a green tree, red base, ornaments (random colored LEDs), and some twinkles

    duration_ = 1000 * 60 * 60; // hour long duration - meant to loop indefinitely until the game starts
    //duration_ = 4000; // for testing
    const TickCount fadeDuration = 1000;

    AnimatorGroup* pGroup = new AnimatorGroup();
    for(int laneIndex = 0; laneIndex < laneCount; ++laneIndex)
    {
        const Lane* pLane = pLanes + laneIndex;
        LedIndicesToStartAndCount(pLane->startIndex, pLane->endIndex, startLedIndex, ledCount);
        pGroup->AddAnimator(new SolidColor(duration_, startLedIndex, ledCount, color_green), 0);
    }

    LedIndicesToStartAndCount(treeBaseStartLedIndex, treeBaseEndLedIndex, startLedIndex, ledCount);
    pGroup->AddAnimator(new SolidColor(duration_, startLedIndex, ledCount, color_red), 0);
    LedIndicesToStartAndCount(pathLeftLedIndex, pathRightLedIndex, startLedIndex, ledCount);
    pGroup->AddAnimator(new SolidColor(duration_, startLedIndex, ledCount, color_green), 0);
    pGroup->AddAnimator(new SparkleAnimator(duration_, pLanes, laneCount, 20, 2000, 5000, color_white), 0);

    auto pFade = new FadeAnimator(fadeDuration, duration_ - (2 * fadeDuration), fadeDuration,  0, totalLedCount, pGroup);

    pRootAnimator = pFade;
}

void GameEngine::AttractAnimator::Step(TickCount localTime, LedColor* pColors)
{
    pRootAnimator->Step(localTime, pColors);
}

GameEngine::SparkleAnimator::SparkleAnimator(TickCount duration, const Lane* pLanes, int laneCount, int sparkleCount, TickCount sparkleDuration,  TickCount sparkleCycleDuration,  LedColor sparkleColor) :
    pLanes_(pLanes),
    laneCount_(laneCount),
    sparkleCount_(sparkleCount),
    sparkleDuration_(sparkleDuration),
    sparkleCycleDuration_(sparkleCycleDuration),
    sparkleColor_(sparkleColor)
{
    duration_ = duration;
    lastLocalTime_ = 0;
    pSparkles_ = new Sparkle[sparkleCount_];
}

float sqr(float f) { return f * f; }
LedColor CrossFadeColor(float t, LedColor a, LedColor b)
{
	LedColor retval = 0;
    float ti = 1.0f - t;

	retval |= (uint8_t)((float)Red(a)   * ti + (float)Red(b)   * t) << 16;
	retval |= (uint8_t)((float)Green(a) * ti + (float)Green(b) * t) << 8;
	retval |= (uint8_t)((float)Blue(a)  * ti + (float)Blue(b)  * t) << 0;

	return retval;

}

void GameEngine::SparkleAnimator::Step(TickCount localTime, LedColor* pColors)
{
    if(localTime < lastLocalTime_)
    {
        // we're a stateful animator and only expect time to move forward.
        // If localTime goes back, it will need to catch up to the start times
        // for the sparkles before you see any sparkles.
        // So reset the sparkles if we detedt time moving backwards
        for(Sparkle* pSparkle = pSparkles_; pSparkle < pSparkles_ + sparkleCount_; ++pSparkle)
        {
            pSparkle->Invalidate();
        }
    }
    lastLocalTime_ = localTime;

    // Set the LED for any active sparkle
    for(Sparkle* pSparkle = pSparkles_; pSparkle < pSparkles_ + sparkleCount_; ++pSparkle)
    {
        if(!pSparkle->IsValid()) continue;

        if(pSparkle->startTime < localTime && pSparkle->startTime + sparkleDuration_ > localTime)
        {
            // sparkle is active
            float t = (float)(localTime - pSparkle->startTime) / (float)sparkleDuration_;
            float i = t < 0.5f ? sqr(2.0f * t) : sqr(2.0f * (0.5f - (t - 0.5f)));
            pColors[pSparkle->index] = CrossFadeColor(i, pColors[pSparkle->index], sparkleColor_);
        }

        // see if this sparkle is done
        if(pSparkle->startTime + sparkleCycleDuration_ < localTime)
        {
            pSparkle->Invalidate();
        }
    }

    // Create sparkles for any open spots
    for(int sparkleIndex = 0; sparkleIndex < sparkleCount_; ++sparkleIndex)
    {
        Sparkle* pSparkle = pSparkles_ + sparkleIndex;

        if(pSparkle->IsValid()) continue;

        const Lane* pLane = pLanes_ + (rand() % laneCount_);
        int startLedIndex;
        int ledCount;
        LedIndicesToStartAndCount(pLane->startIndex, pLane->endIndex, startLedIndex, ledCount);

        pSparkle->startTime = localTime + (rand() % (int) sparkleCycleDuration_);
        pSparkle->index = startLedIndex + (rand() % ledCount);
    }
}

