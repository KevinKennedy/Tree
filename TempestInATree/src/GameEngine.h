#pragma once


#include <vector>
#include <memory>

#include "Animator.h"


class GameEngine
{
private:
    const int treeBaseStartLedIndex = 0;
    const int treeBaseEndLedIndex = 25;
    const int pathStartLedIndex = 30;
    const int pathEndLedIndex = 59;

    static const int levelCount = 5;

    static const int laneCount = 7;
    static const int maxEnemies = 20; // maximum number of simultanious enemies

    static const int maxActiveShots = 30; // maximum number of simultanious shots (player and enemy)
    const float shotSpeed = 0.5f; // how many seconds it takes for a shot to travel the length of a lane (or, "lane-lengths per second")

    const int startingLifeCount = 4;

    enum GameState
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
    };

    struct Shot
    {
        bool player; // otherwise enemy
        int laneIndex;
        float speed; // lane-lengths per second
        float progress;

        inline bool IsValid() const { return laneIndex >= 0; }
        inline void Invalidate() { laneIndex = -1; }
    };

    struct Enemy
    {
        int laneIndex;
        float progress;
        float speed;
        TickCount shotDelta;
        LedColor color;
        bool laneSwitching;
        // enemy type?
        TickCount nextShotTime;
        TickCount nextLaneSwitchTime;

        inline bool IsValid() const { return laneIndex >= 0; }
        inline void Invalidate() { laneIndex = -1; }
    };


    Lane lanes[laneCount];
    Level levels[levelCount];

    GameState gameState = GameState::GS_GAME_OVER;
    TickCount stepTime = 0;
    int levelIndex = 0;
    int score = 0;
    int livesRemaining = 0;
    float playerPosition = 0;
    Shot shots[maxActiveShots] = {};
    Enemy enemies[maxEnemies] = {};

    void Reset(TickCount time);
    void StartLevel();
    void StepLevel();

public:
    GameEngine();

    void Start(TickCount time);

    void Step(TickCount time, int playerPosition, bool fireButtonPressed);

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



