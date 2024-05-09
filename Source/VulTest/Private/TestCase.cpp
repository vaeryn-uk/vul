#include "TestCase.h"

void VulTest::TestCase::Log(const FString& Message) const
{
	TestInstance->AddEvent(FAutomationEvent(
		EAutomationEventType::Warning,
		FString::Printf(TEXT("[VULTEST] %ls LOG: %ls"), *Name, *Message)
	));
}

FString VulTest::TestCase::FormatTestTitle(const FString Message) const
{
	if (!Message.IsEmpty())
	{
		return FString::Printf(TEXT("[VULTEST] %ls: %ls"), *Name, *Message);
	}

	return FString::Printf(TEXT("[VULTEST] %ls"), *Name);
}

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
