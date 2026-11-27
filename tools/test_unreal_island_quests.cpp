#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallIslandQuest.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAquaticRewards.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAquaticTactics.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallOceanEvents.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallOceanGathering.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallOceanCrafting.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallFishing.h"
#include <limits>
#include <cassert>
int main() {
    using namespace MoonfallIslandQuest;
    FJournal J;
    assert(J.Talk(-1,99)==0);
    assert(J.Talk(3,99)==0);
    for (int i=0;i<3;++i) {
        assert(J.State(i)==0);
        assert(J.Talk(i,0)==1);
        assert(J.State(i)==1);
        assert(J.Talk(i,Required(i)-1)==1);
        assert(J.Talk(i,Required(i))==2);
        assert(J.State(i)==2);
        assert(J.Talk(i,Required(i))==3);
        assert(J.Count()==i+1);
    }
    FJournal Early;
    assert(Early.Talk(2,6)==1);
    assert(Early.Count()==0);
    assert(Early.Talk(2,6)==2);
    assert(Early.Count()==1);
    assert(J.Reputation()==90);
    assert(J.Credits==6);
    assert(J.TonicPrice()==1);
    assert(Early.Reputation()==30 && Early.Credits==2);
    assert(Early.TonicPrice()==2);
    int Stock=3;
    assert(!Early.BuyTonic(Stock) && Early.Credits==2);
    Stock=0;
    assert(Early.BuyTonic(Stock) && Stock==1 && Early.Credits==0);
    assert(!Early.BuyTonic(Stock));
    assert(Early.Talk(2,6)==3 && Early.Credits==0);
    Stock=2;
    assert(J.BuyTonic(Stock) && Stock==3 && J.Credits==5);
    assert(!J.BuyTonic(Stock) && J.Credits==5);
    Stock=-1;
    assert(!J.BuyTonic(Stock) && J.Credits==5);
    using namespace MoonfallAquaticRewards;
    for (int index=0;index<6;++index) {
        int high=0;
        for (int roll=0;roll<100;++roll) {
            const int code=RewardCode(index,roll);
            const auto item=MoonfallEquipmentLayout::DecodeFullItem(code);
            assert(MoonfallEquipmentLayout::ValidFullItem(code,RewardSlot(index)));
            assert(item.Trait==3 && item.Tier==index+1);
            if(item.Rarity==(index==5?5:index>=3?4:3)) ++high;
        }
        assert(high==(index==5?20:index>=3?15:20));
    }
    FSessionGear Gear;
    assert(!Gear.Claim(-1,0));
    assert(!Gear.Claim(0,100));
    for(int index=0;index<6;++index) {
        assert(Gear.Claim(index,0));
        assert(!Gear.Claim(index,99));
    }
    assert(Gear.Count()==6 && Gear.Pieces()==3 && Gear.TonicCharges==3);
    assert(Gear.Rarity(0)==4 && Gear.Rarity(1)==4 && Gear.Rarity(2)==5);
    assert(Gear.ConsumeTonic() && Gear.ConsumeTonic() && Gear.ConsumeTonic());
    assert(!Gear.ConsumeTonic());
    using namespace MoonfallAquaticTactics;
    for (int id=0;id<6;++id) {
        const auto B=Behavior(id);
        assert(B.PatrolRadius>0.f && B.PatrolRadius<=105.f);
        assert(B.PatrolRate>0.f && B.PatrolRate<=0.85f);
        assert(B.RecoveryDistance>=0.f && B.RecoveryDistance<=85.f);
        assert(B.StrafeDistance>=0.f && B.StrafeDistance<=85.f);
        assert(B.RecoverySpeedScale>=0.f && B.RecoverySpeedScale<=0.7f);
    }
    assert(Behavior(1).RecoverySpeedScale==0.f);
    assert(Behavior(5).RecoverySpeedScale==0.f);
    assert(Behavior(2).StrafeDistance>Behavior(3).StrafeDistance);
    assert(Behavior(3).RecoveryDistance>Behavior(2).RecoveryDistance);
    assert(Behavior(-1).PatrolRadius==Behavior(0).PatrolRadius);
    assert(MayPatrol(true,false,false,850.f));
    assert(!MayPatrol(true,false,false,851.f));
    assert(!MayPatrol(false,false,false,500.f));
    assert(!MayPatrol(true,true,false,500.f));
    assert(!MayPatrol(true,false,true,500.f));
    assert(!MayPatrol(true,false,false,-1.f));
    assert(!MayPatrol(true,false,false,std::numeric_limits<float>::quiet_NaN()));
    assert(MayReposition(true,true,false));
    assert(!MayReposition(false,true,false));
    assert(!MayReposition(true,false,false));
    assert(!MayReposition(true,true,true));
    using namespace MoonfallOceanEvents;
    for(int second=0;second<180;++second) assert(Activity(second)==second/60);
    assert(Activity(-1)==0 && Activity(180)==0);
    for(int id=0;id<6;++id) {
        assert(PatrolScale(0,id)==1.f);
        assert(PatrolScale(1,id)>=1.f && PatrolScale(1,id)<=1.45f);
        assert(AwarenessRadius(2,id)<=540.f);
    }
    assert(PatrolScale(1,5)==1.f && PatrolScale(1,1)==1.f);
    assert(AwarenessRadius(2,5)==480.f);
    FSightings Sightings;
    assert(!Sightings.Discover(-1) && !Sightings.Discover(6));
    for(int id=0;id<6;++id) {
        assert(Sightings.Discover(id));
        assert(Sightings.Has(id));
        assert(!Sightings.Discover(id));
        assert(Sightings.Count()==id+1);
    }
    using namespace MoonfallOceanGathering;
    for(int event=0;event<3;++event) {
        FCollection Collection;
        for(int id=0;id<NodeCount;++id) {
            assert(CanCollect(id,true,false,true));
            assert(!CanCollect(id,true,false,false));
            assert(!CanCollect(id,false,true,true));
            assert(Collection.Collect(id,event,0));
            assert(!Collection.Collect(id,event,0));
            assert(MaterialType(id)==id%4);
        }
        assert(Collection.Materials()==12);
        for(int type=0;type<4;++type) assert(Collection.Material(type)==3);
        assert(!Collection.Collect(NodeCount,event,0));
        assert(!Collection.Collect(-1,event,0));
        for(int roll : {0,50,80}) assert(Collection.Catch(event,roll));
        assert(Collection.Catches()==3);
        assert(Collection.Species(0)==1 && Collection.Species(1)==1 && Collection.Species(2)==1);
        for(int i=3;i<100;++i) assert(Collection.Catch(event,0));
        assert(Collection.Catches()==100 && Collection.CanCatch());
        FCollection Limit;
        Limit.Fish[0]=MaxSessionCatches-1;
        assert(Limit.Catch(event,0));
        assert(Limit.Catches()==MaxSessionCatches && !Limit.Catch(event,0));
    }
    FCollection Invalid;
    assert(!Invalid.Collect(4,3,0) && Invalid.Catches()==0);
    assert(!CanCollect(4,false,false,true));
    assert(!Invalid.Catch(3,0) && !Invalid.Catch(0,100));
    MoonfallOceanCrafting::FWorkshop Workshop;
    MoonfallOceanGathering::FCollection Supplies;
    MoonfallAquaticRewards::FSessionGear Loan;
    assert(!Workshop.Craft(-1,Supplies,Loan));
    assert(!Workshop.Craft(3,Supplies,Loan));
    assert(!Workshop.Craft(0,Supplies,Loan));
    for(int id=0;id<4;++id) assert(Supplies.Collect(id,0,0));
    for(int roll : {0,50,80}) assert(Supplies.Catch(0,roll));
    assert(Workshop.AvailableMaterials(Supplies)==4);
    assert(Workshop.RemainingFish(Supplies)==3);
    Loan.TonicCharges=3;
    assert(!Workshop.Craft(0,Supplies,Loan));
    assert(Workshop.AvailableMaterials(Supplies)==4);
    Loan.TonicCharges=2;
    assert(Workshop.HasMaterial(0,Supplies));
    assert(Workshop.CanCraft(0,Supplies,Loan));
    assert(Workshop.CanCraft(0,Supplies,Loan));
    assert(Loan.TonicCharges==2);
    assert(Workshop.AvailableMaterials(Supplies)==4);
    assert(Workshop.RemainingFish(Supplies)==3);
    assert(!Workshop.CanCraft(1,Supplies,Loan));
    assert(Workshop.Craft(0,Supplies,Loan));
    assert(!Workshop.HasMaterial(0,Supplies));
    assert(!Workshop.CanCraft(0,Supplies,Loan));
    assert(Loan.TonicCharges==3);
    assert(!Workshop.Craft(0,Supplies,Loan));
    assert(!Workshop.Craft(1,Supplies,Loan));
    assert(Workshop.AvailableMaterials(Supplies)==3);
    assert(Loan.Claim(0,99));
    assert(Loan.Claim(1,99));
    assert(Workshop.Craft(1,Supplies,Loan));
    assert(Loan.Rarity(0)==3);
    assert(!Workshop.Craft(1,Supplies,Loan));
    assert(Workshop.Craft(2,Supplies,Loan));
    assert(Loan.Rarity(1)==3);
    assert(Workshop.AvailableMaterials(Supplies)==0);
    assert(Workshop.RemainingFish(Supplies)==1);
    assert(Supplies.Materials()==4 && Supplies.Catches()==3);
    assert(GatheringRank(-1)==1 && GatheringRank(0)==1 && GatheringRank(4)==2);
    assert(GatheringRank(35)==9 && GatheringRank(36)==10 && GatheringRank(1000)==10);
    assert(Workshop.RemainingSpecies(0,Supplies)==0);
    assert(Workshop.RemainingSpecies(1,Supplies)==0);
    assert(Workshop.RemainingSpecies(2,Supplies)==1);
    assert(Workshop.RemainingSpecies(-1,Supplies)==0);
    MoonfallOceanCrafting::FWorkshop MaxWorkshop;
    Loan.Codes[0]=MoonfallEquipmentLayout::EncodeFullItem(2,3,5,4);
    assert(!MaxWorkshop.Craft(1,Supplies,Loan));
    assert(MaxWorkshop.AvailableMaterials(Supplies)==4);
    // Replenishment increases cumulative earned stock, never refunds spent stock.
    assert(!Supplies.Collect(0,0,89.99));
    assert(!Supplies.Collect(0,0,-1));
    assert(!Supplies.Collect(0,0,std::numeric_limits<double>::quiet_NaN()));
    assert(Supplies.Collect(0,0,90));
    assert(!Supplies.Collect(0,0,90));
    assert(Supplies.Material(0)==2);
    assert(Workshop.AvailableMaterials(Supplies)==1);
    assert(Workshop.HasMaterial(0,Supplies));
    Loan.TonicCharges=2;
    assert(Workshop.Craft(0,Supplies,Loan));
    assert(Workshop.AvailableMaterials(Supplies)==0);
    assert(!Supplies.Collect(0,0,179.99));
    assert(Supplies.Collect(0,1,180));
    assert(Workshop.AvailableMaterials(Supplies)==1);
    using namespace MoonfallFishing;
    FSession Cast;
    assert(!Cast.Start(0,1));
    assert(Cast.Start(10,3));
    assert(!Cast.Hook(11));
    assert(!Cast.Advance(12,true) && Cast.Stage==EStage::Waiting);
    assert(!Cast.Advance(13,true) && Cast.Stage==EStage::Bite);
    assert(Cast.Hook(15) && Cast.Stage==EStage::Reeling);
    assert(!Cast.Advance(16,true));
    assert(Cast.Advance(16.5,true) && Cast.Stage==EStage::Idle);
    assert(!Cast.Advance(17,true));
    assert(Cast.Start(20,2));
    assert(!Cast.Advance(22,true));
    assert(!Cast.Advance(27,true) && Cast.Stage==EStage::Bite);
    assert(!Cast.Advance(32,true) && Cast.Stage==EStage::Idle);
    assert(Cast.Start(30,3));
    assert(!Cast.Advance(31,false) && Cast.Stage==EStage::Idle);
    assert(!Cast.Start(std::numeric_limits<double>::quiet_NaN(),3));
    for(int Id=0;Id<SchoolCount;++Id) {
        auto A=SchoolPosition(Id,0,6000,4000),B=SchoolPosition(Id,30,6000,4000);
        assert(A.X>0 && A.X<6000 && A.Y>0 && A.Y<4000);
        assert(A.X!=B.X || A.Y!=B.Y);
        assert(SchoolSpecies(Id)==Id%3);
    }
    assert(SchoolSpecies(-1)==-1 && SchoolSpecies(8)==-1);
    assert(SchoolPosition(0,-1,6000,4000).X==0);
    assert(SchoolWait(7)==3.5 && SchoolWait(2)==2);
    assert(ReelProgress(0,1,2)==0 && ReelProgress(1.5,1,2)==0.5 && ReelProgress(3,1,2)==1);
    assert(ReelProgress(1,2,2)==0 && ReelProgress(std::numeric_limits<double>::quiet_NaN(),1,2)==0);
    assert(RippleRadius(0,false,false)==6 && RippleRadius(0.5,false,false)==12);
    assert(RippleRadius(0.5,true,true)==18 && RippleRadius(0.9,false,true)==10);
    FSchoolStock SchoolStock;
    for(int I=0;I<SchoolCapacity;++I) assert(SchoolStock.Take(0,10));
    assert(!SchoolStock.Available(0,99.99) && !SchoolStock.Take(0,99.99));
    assert(SchoolStock.Available(1,10) && SchoolStock.Available(0,100));
    assert(SchoolStock.Take(0,100) && SchoolStock.Used[0]==1);
    assert(!SchoolStock.Take(-1,100) && !SchoolStock.Take(8,100));
    assert(!SchoolStock.Take(1,std::numeric_limits<double>::quiet_NaN()));
    assert(TrophyChance(0,false,0)==2 && TrophyChance(27,true,1)==13);
    MoonfallOceanGathering::FCollection Trophy;
    assert(Trophy.Catch(0,0,1,true) && Trophy.LastSpecies==1 && Trophy.LastTrophy && Trophy.TrophyCatches==1);
    assert(Trophy.Catch(0,0) && !Trophy.LastTrophy && Trophy.TrophyCatches==1 && Trophy.Catches()==2);
    assert(Trophy.Trophies(1)==1 && Trophy.Trophies(0)==0 && Trophy.Trophies(3)==0);
    assert(Trophy.DiscoveredSpecies()==2);
    assert(LegendarySpecies(17,0,0,0)==-1);
    assert(LegendarySpecies(18,0,0,2)==0);
    assert(LegendarySpecies(18,1,1,0)==1);
    assert(LegendarySpecies(18,2,2,0)==2);
    assert(LegendarySpecies(18,2,1,0)==-1 && LegendarySpecies(18,0,0,3)==-1);
    assert(LegendarySpecies(18,-1,0,0)==-1 && LegendarySpecies(18,0,0,-1)==-1);
    FCollection LegendaryLog;
    assert(LegendaryLog.Catch(0,99,-1,false,0));
    assert(LegendaryLog.LastLegendary && LegendaryLog.LastSpecies==0 && LegendaryLog.TrophyCatches==1);
    assert(LegendaryLog.Catch(0,99,-1,false,0));
    assert(!LegendaryLog.LastLegendary && LegendaryLog.Legendaries()==1);
    assert(LegendaryLog.Catch(1,0,1,false,1) && LegendaryLog.Catch(2,0,2,false,2));
    assert(LegendaryLog.Legendaries()==3 && LegendaryLog.Catches()==4);
    assert(!LegendaryLog.Catch(0,0,-1,false,3));
    assert(TrophyChance(27,true,1,true)==18);
    assert(LegendarySpecies(18,0,0,4,true)==0 && LegendarySpecies(18,0,0,5,true)==-1);
    MoonfallOceanCrafting::FWorkshop BaitWorkshop;
    FCollection BaitMaterials;
    MoonfallAquaticRewards::FSessionGear BaitGear;
    assert(!BaitWorkshop.Craft(3,BaitMaterials,BaitGear));
    for(int Id=0;Id<NodeCount;++Id) assert(BaitMaterials.Collect(Id,0,0));
    assert(BaitWorkshop.CanCraft(3,BaitMaterials,BaitGear) && BaitWorkshop.Bait==0);
    assert(BaitWorkshop.Craft(3,BaitMaterials,BaitGear) && BaitWorkshop.Bait==5);
    assert(BaitWorkshop.AvailableMaterials(BaitMaterials)==10);
    assert(BaitWorkshop.UseBait() && BaitWorkshop.Bait==4);
    BaitWorkshop.Bait=20;
    assert(!BaitWorkshop.Craft(3,BaitMaterials,BaitGear));
    assert(BaitWorkshop.AvailableMaterials(BaitMaterials)==10);
    BaitWorkshop.Bait=0;
    assert(!BaitWorkshop.UseBait());
    assert(Workshop.RemainingMaterial(0,Supplies)==1 && Workshop.RemainingMaterial(-1,Supplies)==0);
    MoonfallOceanGathering::FCollection SchoolCatch;
    assert(SchoolCatch.Catch(0,0,2) && SchoolCatch.Species(2)==1);
    assert(!SchoolCatch.Catch(0,0,3));
    assert(Rank(-1)==1 && Rank(0)==1 && Rank(3)==2 && Rank(27)==10 && Rank(30)==10);
    assert(CastWait(1,0)==7.0 && CastWait(0,30)==2.0 && CastWait(1,30)<5.0);
    assert(BiteSeconds(0)==10.0 && BiteSeconds(30)==14.5);
    assert(RodTier(0)==1 && RodTier(9)==2 && RodTier(18)==3 && RodTier(27)==4);
    assert(CastReach(0)==850.f && CastReach(27)==1150.f);
    assert(ToolTier(0)==1 && ToolTier(12)==2 && ToolTier(24)==3 && ToolTier(36)==4);
    assert(RecoverySeconds(0)==90.f && RecoverySeconds(36)==60.f);
    FCollection AdvancedGathering;
    AdvancedGathering.Harvested[0]=36;
    assert(AdvancedGathering.Collect(0,0,100));
    assert(AdvancedGathering.RemainingCooldown(0,100)==60);
    assert(!AdvancedGathering.Collect(0,0,159.99));
    assert(AdvancedGathering.Collect(0,0,160));
    assert(Cast.Start(40,2,30));
    assert(!Cast.Advance(42,true) && Cast.Stage==EStage::Bite);
    assert(Cast.Hook(56) && Cast.Stage==EStage::Reeling);
    assert(Cast.Advance(57.5,true));
    assert(!Cast.Advance(58,true));
    assert(!Cast.Start(std::numeric_limits<double>::max(),3));
    assert(Cast.Start(60,2));
    assert(!Cast.Advance(62,true));
    assert(!Cast.Hook(61.99));
    assert(!Cast.Advance(59,true) && Cast.Stage==EStage::Idle);
    assert(Cast.Start(70,2));
    assert(!Cast.Advance(std::numeric_limits<double>::max(),true) && Cast.Stage==EStage::Idle);
    return 0;
}
