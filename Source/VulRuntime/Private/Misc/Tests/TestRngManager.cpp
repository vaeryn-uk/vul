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
	TestEqual("stream sequences match", Enum1Sequence2, Enum1Sequence1);

	// Re-seed again and check another stream.
	Rng.Seed("foo");

	TArray<uint32> Enum2Sequence2;
	Enum2Sequence2.Add(Rng.Stream(TestEnum::EnumVal2).GetUnsignedInt());
	Enum2Sequence2.Add(Rng.Stream(TestEnum::EnumVal2).GetUnsignedInt());
	Enum2Sequence2.Add(Rng.Stream(TestEnum::EnumVal2).GetUnsignedInt());

	TestNotEqual("stream sequences differ", Enum2Sequence2, Enum1Sequence2);

	return !HasAnyErrors();
}
