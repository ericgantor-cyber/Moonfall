#include "MoonfallWaterRules.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallWaterRulesTest, "Moonfall.Traversal.OxygenRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMoonfallWaterRulesTest::RunTest(const FString& Parameters)
{
    using namespace MoonfallWaterRules;
    TestTrue(TEXT("Half breath after 90 seconds"), FMath::IsNearlyEqual(AdvanceAir(100.f, 90.f, true, true, false), 50.f));
    TestTrue(TEXT("Three-minute breath"), FMath::IsNearlyZero(AdvanceAir(100.f, 180.f, true, true, false)));
    TestEqual(TEXT("Surface refill"), AdvanceAir(50.f, 1.f, true, false, false), 75.f);
    TestEqual(TEXT("Lower bound"), AdvanceAir(1.f, 10.f, true, true, false), 0.f);
    TestEqual(TEXT("Upper bound"), AdvanceAir(99.f, 10.f, true, false, false), 100.f);
    TestEqual(TEXT("Land reset"), AdvanceAir(0.f, 1.f, false, false, false), 100.f);
    TestEqual(TEXT("Actual pause"), AdvanceAir(50.f, 10.f, true, true, true), 50.f);
    TestEqual(TEXT("Negative time ignored"), AdvanceAir(50.f, -1.f, true, true, false), 50.f);
    TestEqual(TEXT("Unselected ancestry"), BreathMultiplier(false, 9), 1.f);
    TestEqual(TEXT("Invalid ancestry"), BreathMultiplier(true, 99), 1.f);
    TestEqual(TEXT("Shadowborn breath"), BreathMultiplier(true, 9), 2.f);
    TestTrue(TEXT("Shadowborn half breath after three minutes"), FMath::IsNearlyEqual(AdvanceAir(100.f,180.f,true,true,false,2.f),50.f));
    TestTrue(TEXT("Shadowborn six minutes"), FMath::IsNearlyZero(AdvanceAir(100.f,360.f,true,true,false,2.f)));
    float ExtendedExposure = 0.f;
    TestFalse(TEXT("Extended breath fractional depletion"), AdvanceDrowning(ExtendedExposure,EffectiveDrain(2.f)*0.125f,0.25f,true,false,2.f));
    TestTrue(TEXT("Extended exposure uses matching rate"), FMath::IsNearlyEqual(ExtendedExposure,0.125f));
    float Exposure = 0.f;
    TestFalse(TEXT("Air remains"), AdvanceDrowning(Exposure, 2.f, 0.25f, true, false));
    TestEqual(TEXT("No exposure before depletion"), Exposure, 0.f);
    TestFalse(TEXT("Partial empty frame"), AdvanceDrowning(Exposure, DrainPerSecond * 0.125f, 0.25f, true, false));
    TestEqual(TEXT("Only empty portion counted"), Exposure, 0.125f);
    TestFalse(TEXT("Pause preserves exposure"), AdvanceDrowning(Exposure, 0.f, 0.25f, true, true));
    TestEqual(TEXT("Paused exposure unchanged"), Exposure, 0.125f);
    for (int32 I = 0; I < 3; ++I)
        TestFalse(TEXT("No premature drowning pulse"), AdvanceDrowning(Exposure, 0.f, 0.25f, true, false));
    TestTrue(TEXT("One second without air triggers pulse"), AdvanceDrowning(Exposure, 0.f, 0.25f, true, false));
    TestEqual(TEXT("Pulse preserves remainder"), Exposure, 0.125f);
    TestFalse(TEXT("Surface cancels damage"), AdvanceDrowning(Exposure, 0.f, 0.25f, false, false));
    TestEqual(TEXT("Surface resets exposure"), Exposure, 0.f);
    return true;
}
#endif
