#include "Misc/AutomationTest.h"
#include "Time/VulTime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestTime,
	"VulRuntime.Time.TestTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestTime::RunTest(const FString& Parameters)
{
	{
		float Now = 0.f;
		const FVulTime::FVulNowFn NowFn = [&Now]()
		{
			return Now;
		};

		auto Time = FVulTime(NowFn);

		Now = 0.5f;
		TestTrue("Timer is within", Time.IsWithin(1.f));
		TestFalse("Timer not after", Time.IsAfter(1.f));

		Now = 1.5f;
		TestFalse("Timer not within", Time.IsWithin(1.f));
		TestTrue("Timer is after", Time.IsAfter(1.f));
	}

	{
		FVulTime Time;

		TestFalse("Invalid timer not within", Time.IsWithin(1.f));
		TestFalse("Invalid timer not after", Time.IsAfter(1.f));
	}

	return !HasAnyErrors();
}
