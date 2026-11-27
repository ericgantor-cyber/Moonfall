#include "MoonfallAquaticProfiles.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallAquaticProfilesTest, "Moonfall.Traversal.AquaticProfiles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMoonfallAquaticProfilesTest::RunTest(const FString& Parameters)
{
    TSet<FString> Names;
    for (int32 Id = 0; Id < 6; ++Id) {
        const auto P = MoonfallAquatic::Profile(Id);
        Names.Add(FString(P.Name));
        TestTrue(TEXT("Positive health and damage"), P.Health > 0.f && P.Damage > 0.f);
        TestTrue(TEXT("Readable windup"), P.Windup >= 0.7f && P.Windup <= 1.5f);
        TestTrue(TEXT("Bounded speed and reach"), P.Speed <= 200.f && P.Reach <= 220.f);
        TestTrue(TEXT("Positive recovery"), P.Recovery > 0.f);
    }
    for (int32 Slot = 0; Slot < 6; ++Slot) {
        int32 Authored = 0;
        for (int32 Roll = 0; Roll < 100; ++Roll) {
            const int32 Id = MoonfallAquatic::SelectEncounter(Slot, Roll);
            if (Id == Slot) ++Authored;
            TestTrue(TEXT("Valid encounter selection"), Id >= 0 && Id <= 5);
            TestTrue(TEXT("Rare restricted to its own grotto"), Id != 4 || Slot == 4);
            TestTrue(TEXT("Leviathan restricted to final grotto"), Id != 5 || Slot == 5);
        }
        TestEqual(TEXT("Exact authored encounter chance"), Authored, Slot == 5 ? 100 : Slot == 4 ? 25 : 70);
    }
    TestEqual(TEXT("Invalid slot fails to default"), MoonfallAquatic::SelectEncounter(9, 50), 0);
    TestEqual(TEXT("Invalid roll fails to default"), MoonfallAquatic::SelectEncounter(5, 100), 0);
    TestEqual(TEXT("Six distinct names"), Names.Num(), 6);
    TestEqual(TEXT("Invalid profile uses default"), MoonfallAquatic::Profile(-1).Health, 60.f);
    TestEqual(TEXT("Ordinary creature has no phase changes"), MoonfallAquatic::Phase(0,0.1f), 1);
    TestEqual(TEXT("Leviathan phase one"), MoonfallAquatic::Phase(5,0.66f), 1);
    TestEqual(TEXT("Leviathan phase two"), MoonfallAquatic::Phase(5,0.65f), 2);
    TestEqual(TEXT("Leviathan phase three"), MoonfallAquatic::Phase(5,0.3f), 3);
    TestTrue(TEXT("Bounded accelerated recovery"), MoonfallAquatic::RecoveryScale(3) >= 0.7f);
    return true;
}
#endif
