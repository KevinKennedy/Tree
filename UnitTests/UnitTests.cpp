#include "pch.h"
#include "CppUnitTest.h"
#include <vector>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>

#include "..\TempestInATree\src\Animator.h"
#include "..\TempestInATree\src\GameEngine.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
	TEST_CLASS(UnitTests)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
		}

		TEST_METHOD(LedIndexFromRangeTest)
		{
			// Um, yeah, this may be a little overkill - but handy if we want to optimize this function :-)

			Assert::AreEqual(0, GameEngine::LedIndexFromRange(0, 1, 0.0f));
			Assert::AreEqual(0, GameEngine::LedIndexFromRange(0, 1, 0.4999f));
			Assert::AreEqual(1, GameEngine::LedIndexFromRange(0, 1, 0.5f));
			Assert::AreEqual(1, GameEngine::LedIndexFromRange(0, 1, 0.9f));
			Assert::AreEqual(1, GameEngine::LedIndexFromRange(0, 1, 1.0f));

			Assert::AreEqual(0, GameEngine::LedIndexFromRange(0, 10, 0.0f));
			Assert::AreEqual(0, GameEngine::LedIndexFromRange(0, 10, 0.0499f));
			Assert::AreEqual(1, GameEngine::LedIndexFromRange(0, 10, 0.05f));
			Assert::AreEqual(1, GameEngine::LedIndexFromRange(0, 10, 0.1499f));
			Assert::AreEqual(2, GameEngine::LedIndexFromRange(0, 10, 0.15f));
			Assert::AreEqual(2, GameEngine::LedIndexFromRange(0, 10, 0.2f));
			Assert::AreEqual(2, GameEngine::LedIndexFromRange(0, 10, 0.2499f));
			Assert::AreEqual(3, GameEngine::LedIndexFromRange(0, 10, 0.25f));
			Assert::AreEqual(10, GameEngine::LedIndexFromRange(0, 10, 0.99f));
			Assert::AreEqual(10, GameEngine::LedIndexFromRange(0, 10, 1.0f));

			Assert::AreEqual(7, GameEngine::LedIndexFromRange(5, 15, 0.2499f));


			Assert::AreEqual(1, GameEngine::LedIndexFromRange(1, 0, 0.0f));
			Assert::AreEqual(1, GameEngine::LedIndexFromRange(1, 0, 0.4999f));
			Assert::AreEqual(0, GameEngine::LedIndexFromRange(1, 0, 0.5f));
			Assert::AreEqual(0, GameEngine::LedIndexFromRange(1, 0, 0.9f));
			Assert::AreEqual(0, GameEngine::LedIndexFromRange(1, 0, 1.0f));

			Assert::AreEqual(10, GameEngine::LedIndexFromRange(10, 0, 0.0f));
			Assert::AreEqual(0, GameEngine::LedIndexFromRange(10, 0, 1.0f));

			Assert::AreEqual(15, GameEngine::LedIndexFromRange(15, 5, 0.0f));
			Assert::AreEqual(5, GameEngine::LedIndexFromRange(15, 5, 1.0f));
			Assert::AreEqual(10, GameEngine::LedIndexFromRange(15, 5, 0.5f));
		}

		TEST_METHOD(ShotSmokeTest)
		{
			const float floatTolerance = 0.0001f;
			GameEngine ge;
			TickCount time = 0;
			const TickCount ticksPerShot = (TickCount)((float)TicksPerSecond * ge.shotSpeed);

			ge.Start(time++);
			while (ge.gameState != GameEngine::GameState::GS_PLAYING_LEVEL)
			{
				ge.Step(time++, 0, false);
			}

			// Add a player shot
			Assert::AreEqual(0, PlayerShotCount(ge));
			ge.Step(time++, 0, false); // Step once with button up
			ge.Step(time, 0, true); // fire button down
			Assert::AreEqual(1, PlayerShotCount(ge));
			Assert::AreEqual(1.0f, PlayerShotPosition(ge), floatTolerance);

			// See the player shot 1/4 way up the lane
			time += ticksPerShot / 4;
			ge.Step(time, 0, false);
			Assert::AreEqual(1, PlayerShotCount(ge));
			Assert::AreEqual(0.75f, PlayerShotPosition(ge), floatTolerance);

			// See the player shot all the way up the lane
			time += (ticksPerShot / 4) * 3;
			ge.Step(time, 0, false);
			Assert::AreEqual(1, PlayerShotCount(ge));
			Assert::AreEqual(0.0f, PlayerShotPosition(ge), floatTolerance);

			// Player shot didn't hit anything and is off the lane
			time += 1;
			ge.Step(time, 0, false);
			Assert::AreEqual(0, PlayerShotCount(ge));


			// Player shot near the second lane
			time += 1;
			ge.Step(time, ge.lanes[1].pathLedIndex, true);
			Assert::AreEqual(1, PlayerShotCount(ge));
			Assert::AreEqual(1, ge.shots[0].laneIndex);
			time += ticksPerShot + 1;
			ge.Step(time, 0, false);
			Assert::AreEqual(0, PlayerShotCount(ge));

			// Player shot near the last lane
			time += 1;
			ge.Step(time, ge.pathLedCount - 1, true);
			Assert::AreEqual(1, PlayerShotCount(ge));
			Assert::AreEqual(ARRAYSIZE(ge.lanes) - 1, ge.shots[0].laneIndex);
			time += ticksPerShot + 1;
			ge.Step(time, 0, false);
			Assert::AreEqual(0, PlayerShotCount(ge));


			//for (int i = 0; i < ARRAYSIZE(ge.shots); i++)
			//{
			//	const GameEngine::Shot* pShot = ge.shots + i;
			//	Log("Shot %d: valid:%s player:%s, laneIndex:%d, speed:%f, lanePosition:%f\r\n",
			//		i, pShot->IsValid()?"valid":"invalid", pShot->player?"player":"enemy", pShot->laneIndex, pShot->speed, pShot->lanePosition);
			//}
		}

		TEST_METHOD(EnemySpawnSmokeTest)
		{
			const float floatTolerance = 0.001f;
			GameEngine ge;
			TickCount time = 0;
			const TickCount ticksPerWhiteEnemy = (TickCount)((float)TicksPerSecond / 0.3f);

			ge.Start(time++);
			while (ge.gameState != GameEngine::GameState::GS_PLAYING_LEVEL)
			{
				ge.Step(time++, 0, false);
			}

			Assert::AreEqual(0, EnemyCount(ge));
			ge.Step(time++, 0, false);
			Assert::AreEqual(1, EnemyCount(ge));
			Assert::AreEqual(0.0f, ge.enemies[0].lanePosition, floatTolerance);
			ge.Step(TicksPerSecond, 0, false);
			Assert::AreEqual(ge.enemies[0].speed, ge.enemies[0].lanePosition, floatTolerance);
			for (int i = 0; i < 100 * 5; i++)
			{
				time += 10;
				ge.Step(time, 0, false);
			}
			Assert::AreEqual(1.0f, ge.enemies[0].lanePosition, 0.0f);

		}

		int PlayerShotCount(const GameEngine& ge)
		{
			int retval = 0;
			for (const GameEngine::Shot* pShot = ge.shots; pShot != ge.shots + ARRAYSIZE(ge.shots); pShot++)
			{
				retval += pShot->IsValid() && pShot->player;
			}
			return retval;
		}

		float PlayerShotPosition(const GameEngine& ge)
		{
			for (const GameEngine::Shot* pShot = ge.shots; pShot != ge.shots + ARRAYSIZE(ge.shots); pShot++)
			{
				if (pShot->IsValid() && pShot->player)
				{
					return pShot->lanePosition;
				}
			}
			return -1;
		}

		int EnemyCount(const GameEngine& ge)
		{
			int enemyCount = 0;
			for (const GameEngine::Enemy* pEnemy = ge.enemies; pEnemy < ge.enemies + ARRAYSIZE(ge.enemies); pEnemy++)
			{
				if (pEnemy->IsValid())
				{
					enemyCount++;
				}
			}
			return enemyCount;
		}

		void Log(const char* pszFormat, ...)
		{
			va_list argList;
			va_start(argList, pszFormat);

			char buff[300];
			StringCchVPrintfA(buff, ARRAYSIZE(buff), pszFormat, argList);
			Logger::WriteMessage(buff);
			va_end(argList);
		}
	};
}
