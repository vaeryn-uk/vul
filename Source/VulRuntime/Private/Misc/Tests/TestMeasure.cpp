#include "Misc/AutomationTest.h"
#include "Misc/VulMeasure.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestMeasure,
	"VulRuntime.Misc.TestMeasure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

typedef TVulMeasure<float> FTestMeasure;

bool TestMeasure::RunTest(const FString& Parameters)
{
	// Basic-case free-form testing.
	{
		FTestMeasure Measure(10);

		TestEqual("Measure CurrentValue", Measure.CurrentValue(), 10.f);
		TestEqual("Measure MaxValue", Measure.MaxValue(), 10.f);

		TestFalse("Measure CanConsume excess", Measure.CanConsume(20.f));
		TestTrue("Measure CanConsume ok", Measure.CanConsume(5.f));

		TestTrue("Measure Consume ok", Measure.Consume(5.f));
		TestEqual("Measure Consume result", Measure.CurrentValue(), 5.f);
		TestEqual("Measure Consume result pct", Measure.Percent(), .5f);

		TestFalse("Measure Modify clamped min", Measure.Modify(-15));
		TestEqual("Measure Modify clamped min result", Measure.CurrentValue(), 0.f);
		TestEqual("Measure Modify clamped min result pct", Measure.Percent(), .0f);

		TestTrue("Measure Modify clamped max", Measure.Modify(30));
		TestEqual("Measure Modify clamped max result", Measure.CurrentValue(), 10.f);
		TestEqual("Measure Modify clamped max result pct", Measure.Percent(), 1.f);
	}

	{ // Measure copy constructor.
		FTestMeasure M1(10.f);
		auto M2 = M1;

		M2.Modify(-5.f);

		TestEqual("Copy constructor: Copied measure is changed", M2.CurrentValue(), 5.f);
		TestEqual("Copy constructor: Original measure is not changed", M1.CurrentValue(), 10.f);
	}

	{ // Measure assignment operator.
		FTestMeasure M1(10.f);
		auto M2 = M1;
		M2 = M1;

		M2.Modify(-5.f);

		TestEqual("assignment: Copied measure is changed", M2.CurrentValue(), 5.f);
		TestEqual("assignment: Original measure is not changed", M1.CurrentValue(), 10.f);
	}

	{ // Modify max.
		FTestMeasure M1(10.f);

		M1.ModifyMax(FTestMeasure::FMaxNumberModification::MakeFlat(2));
		TestEqual("modifymax 2,0: maximum", M1.MaxValue(), 12.f);
		TestEqual("modifymax 2,0: current", M1.CurrentValue(), 10.f);

		M1.ModifyMax(FTestMeasure::FMaxNumberModification::MakeFlat(2), 1);
		TestEqual("modifymax 2,1: maximum", M1.MaxValue(), 14.f);
		TestEqual("modifymax 2,1: current", M1.CurrentValue(), 12.f);

		M1.ModifyMax(FTestMeasure::FMaxNumberModification::MakeFlat(4), .5);
		TestEqual("modifymax 4,.5: maximum", M1.MaxValue(), 18.f);
		TestEqual("modifymax 4,.5: current", M1.CurrentValue(), 14.f);

		const auto ModificationId = FGuid::NewGuid();
		M1.ModifyMax(FTestMeasure::FMaxNumberModification::MakeFlat(20, ModificationId), 1);
		TestEqual("modifymax 20,1: maximum", M1.MaxValue(), 38.f);
		TestEqual("modifymax 20,1: current", M1.CurrentValue(), 34.f);

		M1.RemoveMax(ModificationId);
		TestEqual("removemax: maximum", M1.MaxValue(), 18.f);
		TestEqual("removemax: current", M1.CurrentValue(), 18.f);
	}

	return !HasAnyErrors();
}
