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

void VulTest::Log(FAutomationTestBase* TestInstance, const FString& Message)
{
	TestInstance->AddEvent(FAutomationEvent(
		EAutomationEventType::Warning,
		FString::Printf(TEXT("[VULTEST LOG] %s"), *Message)
	));
}
