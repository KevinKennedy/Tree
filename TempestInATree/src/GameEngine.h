#pragma once

#include "Animator.h"

#ifndef ARRASIZE
#define ARRAYSIZE(a) ((int)(sizeof(a) / sizeof(*a)))
#endif

namespace UnitTests
{
    class UnitTests;
}

class GameEngine
{
    friend class UnitTests::UnitTests;

private:
    static const int treeBaseStartLedIndex = 1;
    static const int treeBaseEndLedIndex = 17;
    static const int pathLeftLedIndex = 55;
    static const int pathRightLedIndex = 27;
    static const int pathLedCount = abs(pathRightLedIndex - pathLeftLedIndex) + 1;

    static const int levelCount = 5;

    static const int laneCount = 7;
    static const int maxEnemies = 20; // maximum number of simultanious enemies

    static const int maxActiveShots = 30; // maximum number of simultanious shots (player and enemy)
    const float shotSpeed = 0.5f; // how many seconds it takes for a shot to travel the length of a lane (or, "lane-lengths per second")

    const float shotEnemyCollisionThreshold = 0.05; // how close a shot needs to be to an enemy hit it
    const float shotEnemySideCollisionPositionThreshold = 0.95; // When the enemy is on the player path, how far down the lane the shot still needs to be to have then enemy get destroyed
    const float playerEnemySideDestroyPositionThreshold = 0.1; // When the enemy is on the player path, how close the enemy needs to be to the player to destroy the enemy
    const float playerShotEnemyShotCollisionThreshold = 0.05; // how close the player's shot needs to be to an enemy shot to count a hit
    const float playerEnemyCollisionThreshold = 0.05; // How close a player needs to be to an enemy for the player to die
    const float playerEnemyShotCollisionThreshold = 0.1; // How close a player needs to be to an enemy shot on the player path for the player to die
    const int startingLifeCount = 4;

    const TickCount lifeLostAnimationDuration = 1000;
    const TickCount gameOverAnimationDuration = 1000;

    enum class GameState
    {
        GS_STARTING_ANIMATION,
        GS_PLAYING_LEVEL,
        GS_LIFE_LOST_ANIMATION,
        GS_NEXT_LEVEL_ANIMATION,
        GS_GAME_OVER_ANIMATION,
        GS_GAME_OVER,
    };

    struct Lane
    {
        int startIndex; // side of the lane where the enemies start
        int endIndex; // side of the lane where the player is
        int count; // TODO - are we using this?
        int pathLedIndex; // position on the path.  0 is the start of the path.  The end of the path is abs(pathEndLedIndex - pathStartLedIndex) - test this :-)
        int GetLedIndex(float lanePosition) const { return GameEngine::LedIndexFromRange(startIndex, endIndex, lanePosition); }
        float GetPathPosition() const { return (float)pathLedIndex / (float)GameEngine::pathLedCount;}
    };

    struct Level
    {
        int whiteEnemyCount;
        int redEnemyCount;
        int greenEnemyCount;
        float fireRateMultiplier;
        float speedMultiplier;
        float enemySpawnDelta;
    };

    struct Shot
    {
        bool player; // otherwise enemy
        int laneIndex;
        float speed; // lane-lengths per second
        float lanePosition;

        inline bool IsValid() const { return laneIndex >= 0; }
        inline void Invalidate() { laneIndex = -1; }
    };

    enum class EnemyType
    {
        ET_FIRST = 0,
        ET_WHITE = 0,
        ET_RED,
        ET_GREEN,
        ET_LAST = ET_GREEN,
        ET_COUNT,
    };

    enum class EnemyState
    {
        inLane,
        onPlayerPath
    };

    EnemyType NextEnemyType(EnemyType et)
    {
        return static_cast<EnemyType>((static_cast<int>(et) + 1));
    }

    struct Enemy
    {
        float speed;
        TickCount shotDelta;
        LedColor color;
        bool laneSwitching;
        // enemy type?
        EnemyState state;
        TickCount startTime;

        int laneIndex;
        float lanePosition;
        int shotsRemaining;
        TickCount nextShotTime;
        TickCount nextLaneSwitchTime;

        float pathPosition; // could share with lanePosition

        inline bool IsValid() const { return startTime > 0; }
        inline void Invalidate() { startTime = 0; }
    };


    Lane lanes[laneCount];
    Level levels[levelCount];

    GameState gameState = GameState::GS_GAME_OVER;
    TickCount stepTime = 0;
    TickCount stepDeltaTicks = 0;
    float stepDeltaSeconds = 0.0f;
    int levelIndex = 0;
    int score = 0;
    int livesRemaining = 0;
    float stepPlayerPosition = 0;
    Shot shots[maxActiveShots] = {};
    Enemy enemies[maxEnemies] = {};
    TickCount nextEmenySpawnTime = 0;
    int whiteEnemiesRemaining = 0;
    int redEnemiesRemaining = 0;
    int greenEnemiesRemaining = 0;
    bool stepFireButtonPressed = false;
    bool fireButtonWasReleased = false;

    TickCount lifeLostAnimationStartTime = 0;
    TickCount gameOverAnimationStartTime = 0;


    void Reset(TickCount time);
    void StartLevel();
    void StepLevel();
    void LifeLostAnimation();
    void GameOverAnimation();
    void HandleCollisions();
    void SpawnNextEnemy();
    void FireEnemyShots();
    int& GetEnemiesRemaining(EnemyType et);
    void AddShot(bool isPlayer, int laneIndex, float speed, float startingLanePosition);
    int GetClosestLaneToPathPosition(float pathPosition) const;


public:
    GameEngine();

    void Start(TickCount time);

    void FireShot() { stepFireButtonPressed = true; }

    void Step(TickCount time, int playerPosition, bool fireButtonPressed);

    int GetPathLedCount() const { return pathLedCount; }
    void SetLeds(std::vector<LedColor>& leds) const;

    int GetRemainingLives() const { return livesRemaining; }
    int GetLevel() const { return levelIndex; }
    int GetScore() const { return score; }
    bool GetIsGameOver() const;
    // GetSoundTrigger
    // SetLedsLayoutCheck

    static int LedIndexFromRange(int startIndex, int endIndex, float position)
    {
        float ledDistance = (float)(endIndex - startIndex); // may be negative
        float distancePerLed = 1.0f / ledDistance;
        float adjustment = distancePerLed / 2.0f;
        position += (ledDistance > 0) ? adjustment : -adjustment;
        int indexInTheRange = (int)(ledDistance * position);
        return startIndex + indexInTheRange;
    }

    static void FillLedRange(std::vector<LedColor>& leds, int startLedIndex, int endLedIndex, LedColor color)
    {
        int delta = endLedIndex > startLedIndex ? 1 : -1;
        for (LedIndex ledIndex = startLedIndex; ledIndex != endLedIndex; ledIndex += delta)
        {
            leds[ledIndex] = color;
        }
    }

};



