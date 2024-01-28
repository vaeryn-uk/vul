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
		const FVulTime::FVulNowFn NowFn = [&Now]{ return Now; };
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

	{
		float Now = 0.f;
		const FVulTime::FVulNowFn NowFn = [&Now]{ return Now; };
		auto Time = FVulTime(NowFn);

		TestNearlyEqual("Time alpha: 0", Time.Alpha(2), 0);
		Now = 1.f;
		TestNearlyEqual("Time alpha: 0.5", Time.Alpha(2), 0.5f);
		Now = 2.f;
		TestNearlyEqual("Time alpha: 1", Time.Alpha(2), 1.f);
		Now = 4.f;
		TestNearlyEqual("Time alpha: 2", Time.Alpha(2), 2.f);
	}

	return !HasAnyErrors();
}
