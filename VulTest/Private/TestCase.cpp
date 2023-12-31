#include "TestCase.h"

void VulTest::Case(
	FAutomationTestBase* TestInstance,
	const FString& Name,
	const TFunction<void(const TestCase&)>& TestFn)
{
	TestCase Wrapper;
	Wrapper.Name = Name;
	Wrapper.TestInstance = TestInstance;

	TestFn(Wrapper);
}
