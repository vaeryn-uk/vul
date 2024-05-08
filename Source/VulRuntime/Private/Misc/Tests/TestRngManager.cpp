#include "VulTest/Public/TestCase.h"
#include "Misc/AutomationTest.h"
#include "Types.h"
#include "Misc/VulRngManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestRngManager,
	"VulRuntime.Misc.TestRngManager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestRngManager::RunTest(const FString& Parameters)
{
	VulTest::Case(this, "seed behaviour", [](VulTest::TC TC)
	{
		auto Rng = TVulRngManager<TestEnum>();

		Rng.Seed("foo");

		TArray<uint32> Enum1Sequence1;
		Enum1Sequence1.Add(Rng.Stream(TestEnum::EnumVal1).GetUnsignedInt());
		Enum1Sequence1.Add(Rng.Stream(TestEnum::EnumVal1).GetUnsignedInt());
		Enum1Sequence1.Add(Rng.Stream(TestEnum::EnumVal1).GetUnsignedInt());

		// Re-seed.
		Rng.Seed("foo");

		TArray<uint32> Enum1Sequence2;
		Enum1Sequence2.Add(Rng.Stream(TestEnum::EnumVal1).GetUnsignedInt());
		Enum1Sequence2.Add(Rng.Stream(TestEnum::EnumVal1).GetUnsignedInt());
		Enum1Sequence2.Add(Rng.Stream(TestEnum::EnumVal1).GetUnsignedInt());

		// The sequence should be the same because we re-seeded with the same value.
		TC.Equal(Enum1Sequence2, Enum1Sequence1, "same streams match");

		// Re-seed again and check another stream.
		Rng.Seed("foo");

		TArray<uint32> Enum2Sequence2;
		Enum2Sequence2.Add(Rng.Stream(TestEnum::EnumVal2).GetUnsignedInt());
		Enum2Sequence2.Add(Rng.Stream(TestEnum::EnumVal2).GetUnsignedInt());
		Enum2Sequence2.Add(Rng.Stream(TestEnum::EnumVal2).GetUnsignedInt());

		TC.NotEqual(Enum2Sequence2, Enum1Sequence2, "different streams differ");
	});

	VulTest::Case(this, "seedless stream", [](VulTest::TC TC)
	{
		auto Rng = TVulRngManager<TestEnum>();

		TArray<uint32> Sequence1;
		TArray<uint32> Sequence2;

		Rng.Seed("foo");
		Sequence1.Add(Rng.SeedlessStream().GetUnsignedInt());
		Sequence1.Add(Rng.SeedlessStream().GetUnsignedInt());
		Sequence1.Add(Rng.SeedlessStream().GetUnsignedInt());

		Rng.Seed("foo");
		Sequence2.Add(Rng.SeedlessStream().GetUnsignedInt());
		Sequence2.Add(Rng.SeedlessStream().GetUnsignedInt());
		Sequence2.Add(Rng.SeedlessStream().GetUnsignedInt());

		// Technically this could fail even if not seeded, but chances are minuscule.
		TC.NotEqual(Sequence1, Sequence2, "seedless not reset");
	});

	VulTest::Case(this, "shuffle", [](VulTest::TC TC)
	{
		const TArray Original = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
		TArray Shuffled = Original;

		TVulRngManager<TestEnum>().SeedlessStream().Shuffle(Shuffled);

		// Technically this could fail even if not seeded, but chances are minuscule.
		TC.NotEqual(Original, Shuffled, "differs");
	});

	return !HasAnyErrors();
}
