#pragma once

/**
 * Tools for writing tests in UE.
 */
namespace VulTest
{
	/**
	 * Provides an alternative test API for writing Unreal unit tests.
	 */
	struct TestCase
	{
		FString Name;
		FAutomationTestBase* TestInstance;

		template <typename Type>
		bool Equal(Type Actual, Type Expected, const FString Message = FString()) const
		{
			return TestInstance->TestEqual(FormatTestTitle(Message), Actual, Expected);
		}

		/**
		 * Override asserts that two arrays have the same length and each element is equal.
		 */
		template<typename Type>
		bool Equal(TArray<Type> Actual, TArray<Type> Expected, const FString Message = FString()) const
		{
			bool Ok = true;

			if (TestInstance->TestEqual(FormatTestTitle(Message + "Array num"), Actual.Num(), Expected.Num()))
			{
				for (auto N = 0; N < Actual.Num(); ++N)
				{
					if (!TestInstance->TestEqual(
						FormatTestTitle(Message + FString::Printf(TEXT("Item #%d"), N)),
						Actual[N],
						Expected[N]
					))
					{
						Ok = false;
					}
				}
			}

			return Ok;
		}

		template <typename Type>
		bool NotEqual(Type A, Type B, const FString Message = FString()) const
		{
			return TestInstance->TestNotEqual(FormatTestTitle(Message), A, B);
		}

		template <typename Type>
		bool NearlyEqual(Type Actual, Type Expected, const FString Message = FString()) const
		{
			return TestInstance->TestNearlyEqual(FormatTestTitle(Message), Actual, Expected);
		}

	private:
		FString FormatTestTitle(const FString Message) const
		{
			if (!Message.IsEmpty())
			{
				return FString::Printf(TEXT("[VULTEST] %ls: %ls"), *Name, *Message);
			}

			return FString::Printf(TEXT("[VULTEST] %ls"), *Name);
		}
	};

	/**
	 * Shorter alias provided for test case lambdas.
	 */
	typedef const TestCase& TC;

	/**
	 * Groups a series of assertions under one logical grouping.
	 *
	 * Execute assertions in the provided callback using the provided TestCase class.
	 *
	 * Use the alias C for a more concise syntax.
	 */
	void VULTEST_API Case(FAutomationTestBase* TestInstance, const FString& Name, const TFunction<void (TC)>& TestFn);
	constexpr auto C = Case;

	/**
	 * Wraps up a repeatable test, @see DataDriven.
	 */
	template <typename TestData>
	struct DataDrivenTest
	{
		DataDrivenTest(
			const FString& InName,
			FAutomationTestBase* InTestInstance,
			const TFunction<void (const TestCase&, const TestData&)>& InTestFn)
		: Name(InName), TestInstance(InTestInstance), TestFn(InTestFn) {}

		/**
		 * Executes the previously-defined DDT test case with a new set of inputs & expectations.
		 */
		void Run(const FString& DataName, const TestData& Data)
		{
			auto CaseName = FString::Printf(TEXT("%ls: %ls"), *Name, *DataName);

			Case(TestInstance, CaseName, [this, &Data](const TestCase& Case)
			{
				TestFn(Case, Data);
			});
		}

	private:
		FString Name;
		FAutomationTestBase* TestInstance = nullptr;
		TFunction<void (const TestCase&, const TestData&)> TestFn = nullptr;
	};

	/**
	 * Defines a data-driven test, useful when executing the same code & assertions, just with differing
	 * inputs and expected output.
	 *
	 * TestData allows for specifying what inputs/expectations are used for each.
	 *
	 * Call this to setup the test & its logic, then execute @see Run on the returned instance.
	 *
	 * Use the DDT alias for a more concise syntax.
	 */
	template <typename TestData>
	DataDrivenTest<TestData> DataDriven(
		FAutomationTestBase* TestInstance,
		const FString& Name,
		const TFunction<void (TC, const TestData&)>& TestFn)
	{
		return DataDrivenTest<TestData>(Name, TestInstance, TestFn);
	}
	template <typename TestData>
	constexpr auto DDT = DataDriven<TestData>;
}