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
    const int treeBaseStartLedIndex = 1;
    const int treeBaseEndLedIndex = 17;
    const int pathLeftLedIndex = 55;
    const int pathRightLedIndex = 27;
    const int pathLedCount = abs(pathRightLedIndex - pathLeftLedIndex) + 1;

    static const int levelCount = 5;

    static const int laneCount = 7;
    static const int maxEnemies = 20; // maximum number of simultanious enemies

    static const int maxActiveShots = 30; // maximum number of simultanious shots (player and enemy)
    const float shotSpeed = 0.5f; // how many seconds it takes for a shot to travel the length of a lane (or, "lane-lengths per second")

    const int startingLifeCount = 4;

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
        TickCount startTime;
        float lanePosition;

        inline bool IsValid() const { return startTime > 0; }
        inline void Invalidate() { startTime = 0; }
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
        TickCount startTime;
        int laneIndex;
        float lanePosition;
        TickCount nextShotTime;
        TickCount nextLaneSwitchTime;

        inline bool IsValid() const { return startTime > 0; }
        inline void Invalidate() { startTime = 0; }
    };


    Lane lanes[laneCount];
    Level levels[levelCount];

    GameState gameState = GameState::GS_GAME_OVER;
    TickCount stepTime = 0;
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


    void Reset(TickCount time);
    void StartLevel();
    void StepLevel();
    void SpawnNextEnemy();
    int& GetEnemiesRemaining(EnemyType et);
    void AddShot(bool isPlayer, int laneIndex, float speed);
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



