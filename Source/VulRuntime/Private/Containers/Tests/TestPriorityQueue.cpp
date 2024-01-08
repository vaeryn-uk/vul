#include "Containers/VulPriorityQueue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	PriorityQueueTest,
	"Vul.VulRuntime.Private.Containers.Tests.PriorityQueueTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

struct TestEntry
{
	FString Value;
};

typedef TVulPriorityQueue<TestEntry, float> TestQueue;

bool PriorityQueueTest::RunTest(const FString& Parameters)
{
	auto Queue = TestQueue();

	Queue.Add(TestEntry{TEXT("One")}, 5);
	Queue.Add(TestEntry{TEXT("Two")}, 3);
	Queue.Add(TestEntry{TEXT("Three")}, 2);

	TestEqual(TEXT("Default: Not empty"), Queue.IsEmpty(), false);

	TestEqual(TEXT("Default: 1st element"), Queue.Get().GetValue().Element.Value, TEXT("Three"));
	TestEqual(TEXT("Default: 2nd element"), Queue.Get().GetValue().Element.Value, TEXT("Two"));
	TestEqual(TEXT("Default: 3rd element"), Queue.Get().GetValue().Element.Value, TEXT("One"));

	TestEqual(TEXT("Empty"), Queue.IsEmpty(), true);
	TestEqual(TEXT("Custom: Empty unset"), Queue.Get().IsSet(), false);

	auto CustomQueue = TestQueue(
		[](const float A, const float B)
		{
			return A > B;
		}
	);

	CustomQueue.Add(TestEntry{TEXT("One")}, 5);
	CustomQueue.Add(TestEntry{TEXT("Two")}, 3);
	CustomQueue.Add(TestEntry{TEXT("Three")}, 2);

	TestEqual(TEXT("Custom: Not empty"), CustomQueue.IsEmpty(), false);

	TestEqual(TEXT("Custom: 1st element"), CustomQueue.Get().GetValue().Element.Value, TEXT("One"));
	TestEqual(TEXT("Custom: 2nd element"), CustomQueue.Get().GetValue().Element.Value, TEXT("Two"));
	TestEqual(TEXT("Custom: 3rd element"), CustomQueue.Get().GetValue().Element.Value, TEXT("Three"));

	TestEqual(TEXT("Custom: Empty"), CustomQueue.IsEmpty(), true);
	TestEqual(TEXT("Custom: Empty unset"), CustomQueue.Get().IsSet(), false);

	return true;
}
