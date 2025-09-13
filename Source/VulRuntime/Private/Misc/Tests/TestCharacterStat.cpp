#include "Misc/AutomationTest.h"
#include "Misc/VulCharacterStat.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestCharacterStat,
	"VulRuntime.Misc.TestCharacterStat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestCharacterStat::RunTest(const FString& Parameters)
{
	{ // Basic functionality.
		TVulCharacterStat<int, FString> TestStat = 30;

		TestStat.Delta(5, FString("TestBucket1"));
		TestEqual("Bucket1 delta: return value", TestStat.Value(), 35);
		TestEqual("Bucket1 delta: base value unchanged", TestStat.GetBase(), 30);

		TestStat.Clamp("TestBucket2", 0, 3); // Only 3 should be applied from this bucket.
		TestStat.Delta(5, FString("TestBucket2"));
		TestEqual("Bucket2 delta with clamp: return value", TestStat.Value(), 38);

		TestStat.Delta(1, FString("TestBucket2")); // Clamp should stop more being added.
		TestEqual("Bucket2 delta more with clamp: return value", TestStat.Value(), 38);

		TestStat.Set(1, FString("TestBucket2")); // Set: Override existing bucket value
		TestEqual("Bucket2 set: return value", TestStat.Value(), 36);

		TestStat.Set(10, FString("TestBucket2"));
		TestEqual("Bucket2 set with clamp: return value", TestStat.Value(), 38);

		TestEqual("Buckets: base value unchanged", TestStat.GetBase(), 30);

		TestStat.Set(10);
		TestEqual("Buckets: base value changed #1", TestStat.GetBase(), 10);
		TestEqual("Buckets: base value changed #2", TestStat.Value(), 18);
	}
	
	return true;
}
