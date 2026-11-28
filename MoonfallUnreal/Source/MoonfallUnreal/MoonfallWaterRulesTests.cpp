#include "MoonfallWaterRules.h"
#include "MoonfallWaterCurrent.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallWaterRulesTest, "Moonfall.Traversal.OxygenRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMoonfallWaterRulesTest::RunTest(const FString& Parameters)
{
    using namespace MoonfallWaterRules;
    TestTrue(TEXT("First pocket query"),RefreshPocketCandidates(1.0,-1.0,0.0));
    TestFalse(TEXT("Stationary eye reuses candidates"),RefreshPocketCandidates(1.05,1.0,0.0));
    TestTrue(TEXT("Moving eye refreshes candidates"),RefreshPocketCandidates(1.01,1.0,100.0));
    TestTrue(TEXT("Timed pocket refresh"),RefreshPocketCandidates(1.2,1.0,0.0));
    TestTrue(TEXT("Clock reset refresh"),RefreshPocketCandidates(0.5,1.0,0.0));
    TestEqual(TEXT("Dive from surface"),ResolveVerticalIntent(-1,true,0.f),-1);
    TestEqual(TEXT("Rise below surface"),ResolveVerticalIntent(1,true,100.f),1);
    TestEqual(TEXT("Rise stops at surface"),ResolveVerticalIntent(1,true,0.f),0);
    TestEqual(TEXT("Menu cancels dive"),ResolveVerticalIntent(-1,false,100.f),0);
    TestEqual(TEXT("Invalid command rejected"),ResolveVerticalIntent(2,true,100.f),0);
    TestEqual(TEXT("Land pose is neutral"),SwimPosePitch(false,false,false,100.f),0.f);
    TestEqual(TEXT("Surface pose is neutral"),SwimPosePitch(true,false,false,100.f),0.f);
    TestEqual(TEXT("Reduced motion is neutral"),SwimPosePitch(true,true,true,-250.f),0.f);
    TestEqual(TEXT("Idle submerged pose"),SwimPosePitch(true,true,false,0.f),-10.f);
    TestEqual(TEXT("Bounded descent pose"),SwimPosePitch(true,true,false,-10000.f),-25.f);
    TestEqual(TEXT("Bounded ascent pose"),SwimPosePitch(true,true,false,10000.f),10.f);
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
    TestEqual(TEXT("No equipped Focus gear"),GearBreathMultiplier(false,9,-1),1.f);
    TestEqual(TEXT("Common Focus is not a canonical affix"),GearBreathMultiplier(false,0,0),1.f);
    TestTrue(TEXT("Legendary Focus adds thirty percent"),FMath::IsNearlyEqual(GearBreathMultiplier(false,0,5),1.30f));
    TestTrue(TEXT("Shadowborn legendary breath cap"),FMath::IsNearlyEqual(GearBreathMultiplier(true,9,5),2.6f));
    TestTrue(TEXT("Legendary Focus footwear"),FMath::IsNearlyEqual(GearSwimMultiplier(5),1.30f));
    TestEqual(TEXT("Invalid rarity cannot add speed"),GearSwimMultiplier(6),1.f);
    TestTrue(TEXT("HUD rate honors gear cap"),FMath::IsNearlyEqual(EffectiveDrain(2.6f),100.f/468.f));
    TestTrue(TEXT("Gear breath duration matches air depletion"),FMath::IsNearlyZero(AdvanceAir(100.f,468.f,true,true,false,2.6f),0.001f));
    TestEqual(TEXT("One set piece has no breath bonus"),SetBreathMultiplier(1.f,1),1.f);
    TestTrue(TEXT("Two pieces extend breath"),FMath::IsNearlyEqual(SetBreathMultiplier(1.f,2),1.10f));
    TestTrue(TEXT("Three pieces increase speed"),FMath::IsNearlyEqual(SetSwimMultiplier(1.30f,3),1.40f));
    TestTrue(TEXT("Set breath remains bounded"),FMath::IsNearlyEqual(SetBreathMultiplier(2.6f,3),2.86f));
    TestTrue(TEXT("Drain reflects set bonus"),FMath::IsNearlyEqual(EffectiveDrain(2.86f),100.f/514.8f));
    TestTrue(TEXT("Critical air triggers tonic"),ShouldUseTonic(10.f,0.f,true,false));
    TestFalse(TEXT("No early tonic"),ShouldUseTonic(11.f,0.f,true,false));
    TestFalse(TEXT("No tonic stacking"),ShouldUseTonic(0.f,1.f,true,false));
    TestFalse(TEXT("Air pocket does not consume tonic"),ShouldUseTonic(0.f,0.f,false,false));
    TestFalse(TEXT("Paused world does not consume tonic"),ShouldUseTonic(0.f,0.f,true,true));
    TestEqual(TEXT("Two minute duration"),AdvanceTonic(TonicDuration,60.f,false),60.f);
    TestEqual(TEXT("Pause preserves tonic"),AdvanceTonic(60.f,20.f,true),60.f);
    TestEqual(TEXT("Tonic expires"),AdvanceTonic(1.f,2.f,false),0.f);
    float FrameAir=50.f, FrameExposure=0.75f;
    TestFalse(TEXT("Tonic expiration splits protected and unprotected time"),
        AdvanceBreathFrame(FrameAir,FrameExposure,0.25f,true,true,false,0.125f,1.f));
    TestTrue(TEXT("Only unprotected fraction drains"),FMath::IsNearlyEqual(FrameAir,50.f+25.f*0.125f-DrainPerSecond*0.125f));
    TestEqual(TEXT("Protection clears drowning exposure"),FrameExposure,0.f);
    FrameAir=0.f; FrameExposure=0.75f;
    TestFalse(TEXT("Air pocket immediately stops drowning"),AdvanceBreathFrame(FrameAir,FrameExposure,0.25f,true,false,false,0.f,1.f));
    TestEqual(TEXT("Air pocket refill"),FrameAir,6.25f);
    TestEqual(TEXT("Air pocket clears exposure"),FrameExposure,0.f);
    FrameAir=0.f; FrameExposure=0.75f;
    TestTrue(TEXT("Unprotected frame can still drown"),AdvanceBreathFrame(FrameAir,FrameExposure,0.25f,true,true,false,0.f,1.f));
    TestFalse(TEXT("Paused frame preserves air"),AdvanceBreathFrame(FrameAir,FrameExposure,0.25f,true,true,true,0.f,1.f));
    TestEqual(TEXT("Paused air unchanged"),FrameAir,0.f);
    using MoonfallWaterCurrent::Input;
    TestTrue(TEXT("West current flows north"),FMath::IsNearlyEqual(Input(FVector(-700,0,-200),false,false).Y,0.18));
    TestTrue(TEXT("East current flows south"),FMath::IsNearlyEqual(Input(FVector(700,0,-200),false,false).Y,-0.18));
    TestTrue(TEXT("No current in air pocket"),Input(FVector(-700,0,-200),false,true).IsNearlyZero());
    TestTrue(TEXT("No current while ascending"),Input(FVector(-700,0,-200),true,false).IsNearlyZero());
    TestTrue(TEXT("No current in grotto"),Input(FVector(-1400,-850,-200),false,false).IsNearlyZero());
    TestTrue(TEXT("No surface drift"),Input(FVector(-700,0,-20),false,false).IsNearlyZero());
    TestTrue(TEXT("No downward current"),Input(FVector(-700,0,-200),false,false).Z==0.0);
    return true;
}
#endif
