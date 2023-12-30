#include "Hexgrid/Addr.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	TestAddr,
	"Vul.VulRuntime.Private.Hexgrid.Tests.TestAddr",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool TestAddr::RunTest(const FString& Parameters)
{
	// Make the test pass by returning true, or fail by returning false.

	TestEqual(TEXT("Project: 0,0"), FVulHexAddr(0, 0).Project(6), FVector(0, 0, 0));
	TestNearlyEqual(TEXT("Project: 1,0"), FVulHexAddr(1, 0).Project(6), FVector(10.3923, 0, 0));
	TestNearlyEqual(TEXT("Project: 1,-1"), FVulHexAddr(1, -1).Project(6), FVector(5.1961, 9, 0));
	TestNearlyEqual(TEXT("Project: 1,-2"), FVulHexAddr(1, -2).Project(6), FVector(0, 18, 0));
	TestNearlyEqual(TEXT("Project: -3,3"), FVulHexAddr(-3, 3).Project(6), FVector(-15.58845, -27, 0));

	return true;
}
