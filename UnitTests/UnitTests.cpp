#include "pch.h"
#include "CppUnitTest.h"
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
	};
}
