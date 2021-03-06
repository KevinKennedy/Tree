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
    static const int totalLedCount = 400;
    static const int treeBaseStartLedIndex = 1;
    static const int treeBaseEndLedIndex = 17;
    static const int pathLeftLedIndex = 55;
    static const int pathRightLedIndex = 27;
    static const int pathLedCount = 29; // abs(pathRightLedIndex - pathLeftLedIndex) + 1;

    static const int levelCount = 5;

    static const int laneCount = 7;
    static const int maxEnemies = 20; // maximum number of simultanious enemies

    static const int maxActiveShots = 30; // maximum number of simultanious shots (player and enemy)
    static const int maxPlayerShots = 4; // maximum number of shots the player can have active on the board at once
    const float shotSpeed = 0.4f; // how lane lengths a player shot travels in a second

    const float shotEnemyCollisionThreshold = 0.05f; // how close a shot needs to be to an enemy hit it
    const float shotEnemySideCollisionPositionThreshold = 0.95f; // When the enemy is on the player path, how far down the lane the shot still needs to be to have then enemy get destroyed
    const float playerEnemySideDestroyPositionThreshold = 0.1f; // When the enemy is on the player path, how close the enemy needs to be to the player to destroy the enemy
    const float playerShotEnemyShotCollisionThreshold = 0.05f; // how close the player's shot needs to be to an enemy shot to count a hit
    const float playerEnemyCollisionThreshold = 0.05f; // How close a player needs to be to an enemy for the player to die
    const float playerEnemyShotCollisionThreshold = 0.1f; // How close a player needs to be to an enemy shot on the player path for the player to die
    const int startingLifeCount = 4;

    enum class GameState
    {
        GS_ATTRACT_ANIMATION,
        GS_GAME_START_ANIMATION,
        GS_PLAYING_LEVEL,
        GS_LEVEL_START_ANIMATION,
        GS_LIFE_LOST_ANIMATION,
        GS_GAME_OVER_ANIMATION,
    };

    struct AnimatedState
    {
        GameState state;
        TickCount duration;
        bool loop;
        Animator* pAnimator;
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

    class AttractAnimator : public Animator
    {
            Animator* pRootAnimator;
        public:
            AttractAnimator(int treeBaseStartLedIndex, int treeBaseEndLedIndex, int pathLeftLedIndex, int pathRightLedIndex, const Lane* pLanes, int laneCount);
	        virtual void Step(TickCount localTime, LedColor* pColors);
    };

    class TreeTransitionAnimator : public Animator
    {
            Animator* pRootAnimator;
        public:
            TreeTransitionAnimator(TickCount duration, const Lane* pLanes, int laneCount, LedColor colorStart, LedColor colorEnd);
	        virtual void Step(TickCount localTime, LedColor* pColors);
    };

    class SparkleAnimator : public Animator
    {
            struct Sparkle
            {
                TickCount startTime = 0;
                LedIndex index = 0;

                bool IsValid() { return startTime > 0; }
                void Invalidate() { startTime = 0; }
            };
            
            const Lane* pLanes_;
            int laneCount_;
            int sparkleCount_;
            TickCount sparkleDuration_;
            TickCount sparkleCycleDuration_;
            LedColor sparkleColor_;

            TickCount lastLocalTime_;
            Sparkle* pSparkles_;

        public:
            SparkleAnimator(TickCount duration, const Lane* pLanes, int laneCount, int sparkleCount, TickCount sparkleDuration,  TickCount sparkleCycleDuration,  LedColor sparkleColor);
	        virtual void Step(TickCount localTime, LedColor* pColors);
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


    AnimatedState animatedStates[5];
    Lane lanes[laneCount];
    Level levels[levelCount];

    GameState gameState = GameState::GS_ATTRACT_ANIMATION;
    TickCount stepTime = 0;
    TickCount stepDeltaTicks = 0;
    float stepDeltaSeconds = 0.0f;
    int currentLevelIndex = 0;
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
    bool startButtonWasReleased = false;

    AnimatedState* pCurrentAnimatedState = NULL;
    TickCount animationStartTime = 0;
    TickCount animationEndTime = 0;


    void StartAttractAnimation();
    void ResetShotsAndEnemies();
    void StartLevel();
    void StepLevel();
    void AdvanceGameObjects();
    void SpawnNextEnemy();
    void FireEnemyShots();
    void HandleCollisions();
    void HandleLevelCompleted();

    void BeginAnimatedState(GameState newState);
    void StepAnimatedState();
    void SetAnimatedStateLeds(LedColor* pLeds) const;

    void SetLevelStartAnimationLeds(TickCount animationTime, LedColor* pLeds) const;
    int& GetEnemiesRemaining(EnemyType et);
    void AddShot(bool isPlayer, int laneIndex, float speed, float startingLanePosition);
    int GetClosestLaneToPathPosition(float pathPosition) const;


public:
    GameEngine();

    void FireShot() { stepFireButtonPressed = true; }

    void Step(TickCount time, int playerPosition, bool fireButtonPressed, bool startButtonPressed);

    int GetPathLedCount() const { return pathLedCount; }
    void SetLeds(LedColor* pLeds) const;

    int GetRemainingLives() const { return livesRemaining; }
    int GetLevel() const { return currentLevelIndex; }
    int GetScore() const { return score; }
    // GetSoundTrigger
    // SetLedsLayoutCheck

    static void LedIndicesToStartAndCount(int a, int b, int& start, int& count)
    {
        if(b >= a)
        {
            start = a;
            count = b - a + 1;
        }
        else
        {
            start = b;
            count = a - b + 1;
        }
    }

    static int LedIndexFromRange(int startIndex, int endIndex, float position)
    {
        float ledDistance = (float)(endIndex - startIndex); // may be negative
        float distancePerLed = 1.0f / ledDistance;
        float adjustment = distancePerLed / 2.0f;
        position += (ledDistance > 0) ? adjustment : -adjustment;
        int indexInTheRange = (int)(ledDistance * position);
        return startIndex + indexInTheRange;
    }

    static void FillLedRange(LedColor* pLeds, int startLedIndex, int endLedIndex, LedColor color)
    {
        int delta = (endLedIndex < startLedIndex) ? -1 : 1;
        for (int ledIndex = startLedIndex; ledIndex != endLedIndex + delta; ledIndex += delta)
        {
            pLeds[(LedIndex)ledIndex] = color;
        }
    }

    static void ColorWipeLed(LedColor* pLeds, int startLedIndex, int endLedIndex, LedColor startColor, LedColor endColor, float t)
    {
        if (startLedIndex > endLedIndex)
        {
            int temp = endLedIndex;
            endLedIndex = startLedIndex;
            startLedIndex = temp;

            LedColor c = endColor;
            endColor = startColor;
            startColor = c;

            t = 1.0f - t;
        }

        int ledCount = (endLedIndex - startLedIndex) + 1;
        int startGroupCount = (int)(t * ((float)ledCount + 0.499f));
        int divider = startLedIndex + startGroupCount;
        for (int ledIndex = startLedIndex; ledIndex != endLedIndex + 1; ++ledIndex)
        {
            pLeds[ledIndex] = (ledIndex < divider) ? startColor : endColor;
        }

    }

};



