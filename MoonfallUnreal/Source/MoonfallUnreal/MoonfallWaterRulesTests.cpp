#include "MoonfallWaterRules.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallWaterRulesTest, "Moonfall.Traversal.OxygenRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMoonfallWaterRulesTest::RunTest(const FString& Parameters)
{
    using namespace MoonfallWaterRules;
    TestEqual(TEXT("Submerged drain"), AdvanceAir(100.f, 5.f, true, true, false), 80.f);
    TestEqual(TEXT("Surface refill"), AdvanceAir(50.f, 1.f, true, false, false), 75.f);
    TestEqual(TEXT("Lower bound"), AdvanceAir(1.f, 10.f, true, true, false), 0.f);
    TestEqual(TEXT("Upper bound"), AdvanceAir(99.f, 10.f, true, false, false), 100.f);
    TestEqual(TEXT("Land reset"), AdvanceAir(0.f, 1.f, false, false, false), 100.f);
    TestEqual(TEXT("Actual pause"), AdvanceAir(50.f, 10.f, true, true, true), 50.f);
    TestEqual(TEXT("Negative time ignored"), AdvanceAir(50.f, -1.f, true, true, false), 50.f);
    return true;
}
#endif
