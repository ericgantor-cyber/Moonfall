#include "MoonfallAquaticRewards.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallAquaticRewardsTest, "Moonfall.Traversal.AquaticRewards", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMoonfallAquaticRewardsTest::RunTest(const FString& Parameters)
{
    using namespace MoonfallAquaticRewards;
    for (int Slot=0; Slot<6; ++Slot) {
        int High=0;
        for (int Roll=0; Roll<100; ++Roll) {
            const auto Code=RewardCode(Slot,Roll);
            const auto Item=MoonfallEquipmentLayout::DecodeFullItem(Code);
            TestTrue(TEXT("Reward uses a canonical existing slot"),
                MoonfallEquipmentLayout::ValidFullItem(Code,RewardSlot(Slot)));
            TestEqual(TEXT("Reward carries Focus"),Item.Trait,3);
            TestTrue(TEXT("Reward remains within supported equipment tier"),Item.Tier>=1 && Item.Tier<=6);
            if (Item.Rarity==(Slot==5?5:Slot>=3?4:3)) ++High;
        }
        TestEqual(TEXT("Exact higher-rarity chance"),High,Slot==5?20:Slot>=3?15:20);
    }
    FSessionGear Gear;
    TestFalse(TEXT("Invalid chest"),Gear.Claim(-1,0));
    TestFalse(TEXT("Invalid roll"),Gear.Claim(0,100));
    TestTrue(TEXT("First discovery grants a piece"),Gear.Claim(0,0));
    TestEqual(TEXT("First piece"),Gear.Pieces(),1);
    TestFalse(TEXT("Duplicate claim rejected"),Gear.Claim(0,99));
    TestTrue(TEXT("Second discovery"),Gear.Claim(1,99));
    TestTrue(TEXT("Third discovery"),Gear.Claim(2,99));
    TestEqual(TEXT("Three distinct set pieces"),Gear.Pieces(),3);
    TestTrue(TEXT("Later reward upgrades the same slot"),Gear.Claim(3,0));
    TestEqual(TEXT("Accessory upgraded"),Gear.Rarity(0),4);
    TestTrue(TEXT("Later footwear"),Gear.Claim(4,99));
    TestTrue(TEXT("Final reward"),Gear.Claim(5,0));
    TestEqual(TEXT("Six unique claims"),Gear.Count(),6);
    TestEqual(TEXT("Legendary final piece"),Gear.Rarity(2),5);
    TestEqual(TEXT("No extra set slots"),Gear.Pieces(),3);
    TestEqual(TEXT("Three unique chests award three tonics"),Gear.TonicCharges,3);
    TestTrue(TEXT("Consume one tonic"),Gear.ConsumeTonic());
    TestEqual(TEXT("Two remaining"),Gear.TonicCharges,2);
    TestFalse(TEXT("Claim replay cannot replenish"),Gear.Claim(0,0));
    TestTrue(TEXT("Consume second tonic"),Gear.ConsumeTonic());
    TestTrue(TEXT("Consume third tonic"),Gear.ConsumeTonic());
    TestFalse(TEXT("Empty stock cannot consume"),Gear.ConsumeTonic());
    return true;
}
#endif
