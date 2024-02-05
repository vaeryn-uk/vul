#include "TestCase.h"
#include "Misc/AutomationTest.h"
#include "Misc/VulNumber.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestNumber,
	"VulRuntime.Misc.TestNumber",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

typedef TVulNumber<int> TestType;
typedef TVulNumberModification<int> TestMod;

bool TestNumber::RunTest(const FString& Parameters)
{
	// Test basic modifications & clamping.
	{
		struct Data { int Base; int Expected; TArray<TestMod> Modifications; TestType::FClamp Clamp; };
		auto Ddt = VulTest::DDT<Data>(this, TEXT("Int Modifications"), [](VulTest::TC TC, Data Data)
		{
			auto Number = TestType(Data.Base, Data.Clamp);

			for (const auto Modification : Data.Modifications)
			{
				Number.Modify(Modification);
			}

			TC.Equal(Number.Value(), Data.Expected);
		});

		Ddt.Run("No modifications", {10, 10});
		Ddt.Run("Single flat", {10, 12, {TestMod::MakeFlat(2)}});
		Ddt.Run("Single pct", {10, 11, {TestMod::MakePercent(1.1)}});
		Ddt.Run("flat+pct", {10, 24, {TestMod::MakeFlat(10), TestMod::MakePercent(1.2)}});
		Ddt.Run("pct+flat", {10, 22, {TestMod::MakePercent(1.2), TestMod::MakeFlat(10)}});
		Ddt.Run("basepct +ve", {10, 11, {TestMod::MakeBasePercent(0.1)}});
		Ddt.Run("basepct -ve", {10, 0, {TestMod::MakeBasePercent(-1)}});
		Ddt.Run("flat+pct+basepct", {10, 24, {TestMod::MakePercent(1.2), TestMod::MakeFlat(10), TestMod::MakeBasePercent(0.2)}});
		Ddt.Run("basepct+flat+pct", {10, 20, {TestMod::MakeBasePercent(-0.2), TestMod::MakePercent(1.5), TestMod::MakeFlat(8)}});

		// Integers should always round down as we don't support any rounding logic.
		// These tests are here to make this implicit behaviour an explicit behaviour.
		Ddt.Run("int-rounding-up", {10, 15, {TestMod::MakePercent(1.55)}});
		Ddt.Run("int-rounding-down", {10, 15, {TestMod::MakePercent(1.545)}});

		auto Zero = MakeShared<TestType>(0);
		auto Twelve = MakeShared<TestType>(12);

		Ddt.Run("clamp-min", {10, 0, {TestMod::MakeFlat(-30)}, TestType::FClamp({Zero, Twelve})});
		Ddt.Run("clamp-max", {10, 12, {TestMod::MakeFlat(30)}, TestType::FClamp({Zero, Twelve})});

		// We can modify the clamp in place.
		Twelve->Modify(TestMod::MakePercent(1.5));
		Ddt.Run("clamp-modified", {10, 18, {TestMod::MakeFlat(30)}, TestType::FClamp({Zero, Twelve})});

		Ddt.Run("flat-min-clamp", {10, 15, {TestMod::MakeFlat(8).WithClamp(0, 5)}});
		Ddt.Run("flat-max-clamp", {10, 8, {TestMod::MakeFlat(-3).WithClamp(-2, 5)}});
		Ddt.Run("pct-min-clamp", {10, 12, {TestMod::MakePercent(1.5).WithClamp(0, 2)}});
		Ddt.Run("pct-max-clamp", {10, 8, {TestMod::MakePercent(.5).WithClamp(-2, 5)}});
	}

	// Test modification removal.
	{
		const auto ToRemove = FGuid::NewGuid();

		auto Number = TestType(10);
		Number.Modify(TestMod::MakeFlat(5));
		Number.Modify(TestMod::MakePercent(2.f, ToRemove));
		Number.Modify(TestMod::MakeBasePercent(1.f));

		TestEqual("Modification revocation", Number.Value(), 40);

		Number.Remove(ToRemove);

		TestEqual("Modification revocation", Number.Value(), 25);
	}

	// Test watches
	{
		auto Number = TestType(10);

		int CapturedOld = 0;
		int CapturedNew = 0;
		bool ValidFn = true;

		Number.Watch().Add([&ValidFn] { return ValidFn; }, [&](int New, int Old)
		{
			CapturedNew = New;
			CapturedOld = Old;
		});

		Number.Modify(TestMod::MakeFlat(5));

		TestEqual("Number watch #1: old", CapturedOld, 10);
		TestEqual("Number watch #1: new", CapturedNew, 15);

		Number.Modify(TestMod::MakePercent(2.f));

		TestEqual("Number watch #2: old", CapturedOld, 15);
		TestEqual("Number watch #2: new", CapturedNew, 30);

		ValidFn = false;

		Number.Modify(TestMod::MakePercent(2.f));

		// Despite modification, the watch should not have been called (therefore old captured values are still there).
		TestEqual("Number watch #3: old", CapturedOld, 15);
		TestEqual("Number watch #3: new", CapturedNew, 30);
	}

	// Make the test pass by returning true, or fail by returning false.
	return true;
}
