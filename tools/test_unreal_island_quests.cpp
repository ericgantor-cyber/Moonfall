#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallIslandQuest.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAquaticRewards.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAquaticTactics.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallOceanEvents.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallOceanGathering.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallOceanCrafting.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallFishing.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallMoney.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallProfessionLedger.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallRegionalFishing.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallRegionalAtlas.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallProfessionUI.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAttributes.h"
#include <limits>
#include <cassert>
int main() {
    {
        using namespace MoonfallAttributes;
        constexpr auto Start=BuildPrimary(1,0,0,0);
        static_assert(Start.Strength==10&&Start.Agility==10&&Start.Intellect==10&&Start.Stamina==10);
        constexpr auto Gear=BuildPrimary(1,5,3,7);
        static_assert(Gear.Strength==20&&Gear.Agility==18&&Gear.Intellect==24&&Gear.Stamina==16);
        constexpr auto Bad=BuildPrimary(-100,-1,-1,-1);
        static_assert(Bad.Strength==10&&Bad.Stamina==10);
        constexpr auto Max=BuildPrimary(2147483647,2147483647,2147483647,2147483647);
        static_assert(Max.Strength==258&&Max.Stamina==258);
        static_assert(PhysicalBonusBps(-1)==0&&PhysicalBonusBps(999)==2000);
        static_assert(FinesseBonusBps(999)==1000&&SpellBonusBps(999)==2000);
        static_assert(HealthBonus(-1)==0&&HealthBonus(999)==500);
    }
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
        for(int id=0;id<BaseNodeCount;++id) {
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
    MoonfallMoney::FWallet Coins;
    assert(Coins.Credit(12345)&&Coins.Gold()==1&&Coins.Silver()==23&&Coins.LooseCopper()==45);
    assert(!Coins.Debit(12346)&&!Coins.Credit(-1)&&Coins.Copper==12345);
    assert(Coins.Debit(345)&&Coins.Copper==12000);
    MoonfallMoney::FWallet Full{MoonfallMoney::MaxCoins};
    assert(!Full.Credit(1)&&!Full.Debit(0));
    assert(MoonfallMoney::EnemyReward(1,false,false)==100);
    assert(MoonfallMoney::EnemyReward(1,true,false)>100&&MoonfallMoney::EnemyReward(1,true,true)>350);
    assert(MoonfallMoney::EnemyReward(101,false,false)==0);
    assert(MoonfallMoney::GearSalePrice(1,0,0)==40);
    assert(MoonfallMoney::GearSalePrice(25,5,3)==2430);
    assert(MoonfallMoney::GearSalePrice(26,5,3)==0);
    assert(MoonfallMoney::GearSalePrice(1,0,1)==0);
    assert(MoonfallMoney::GearSalePrice(1,6,0)==0);
    assert(MoonfallMoney::BoundedRewardCredit(100,0)==100);
    assert(MoonfallMoney::BoundedRewardCredit(100,MoonfallMoney::MaxCoins-1)==1);
    assert(MoonfallMoney::BoundedRewardCredit(100,MoonfallMoney::MaxCoins)==0);
    assert(MoonfallMoney::BoundedRewardCredit(-1,0)==0);
    assert(MoonfallMoney::BoundedRewardCredit(100,-1)==0);
    for(int Capacity=0;Capacity<=1000;++Capacity){
      const bool Valid=Capacity>=100&&Capacity<=500&&Capacity%50==0;
      assert(MoonfallMoney::ValidBagCapacity(Capacity)==Valid);
      assert(!MoonfallMoney::BagHasRoom(Capacity,Capacity));
      assert(MoonfallMoney::BagHasRoom(Capacity-1,Capacity)==Valid);
      assert(!MoonfallMoney::BagHasRoom(10000,Capacity));
      assert((MoonfallMoney::BagUpgradePrice(Capacity)>0)==(Valid&&Capacity<500));
    }
    for(int Count=-1;Count<=1001;++Count)assert(MoonfallMoney::VaultHasRoom(Count)==(Count>=0&&Count<1000));
    assert(MoonfallMoney::VaultPageSelection(0,0,1)==0);
    assert(MoonfallMoney::VaultPageSelection(0,1000,1)==25);
    assert(MoonfallMoney::VaultPageSelection(990,1000,1)==999);
    assert(MoonfallMoney::VaultPageSelection(5,1000,-1)==0);
    assert(MoonfallMoney::VaultPageSelection(999,1000,-1)==974);
    assert(MoonfallMoney::VaultPageSelection(-20,1000,0)==0);
    assert(MoonfallMoney::VaultPageSelection(5000,1000,0)==999);
    for(int Count=1;Count<=1000;++Count){
      for(int Index=0;Index<Count;++Index){
        const int Next=MoonfallMoney::VaultPageSelection(Index,Count,1);
        const int Prev=MoonfallMoney::VaultPageSelection(Index,Count,-1);
        assert(Next>=Index&&Next<Count&&Next-Index<=25);
        assert(Prev>=0&&Prev<=Index&&Index-Prev<=25);
      }
    }
    assert(MoonfallMoney::BagUpgradePrice(100)==1000);
    assert(MoonfallMoney::BagUpgradePrice(450)==11500);
    assert(MoonfallMoney::ContractReward(1,1)==100);
    assert(MoonfallMoney::ContractReward(30,100)==1175);
    assert(MoonfallMoney::ContractReward(0,1)==0&&MoonfallMoney::ContractReward(31,1)==0);
    assert(MoonfallMoney::ContractReward(1,0)==0&&MoonfallMoney::ContractReward(1,101)==0);
    for(int Cost=1;Cost<=30;++Cost)for(int L=1;L<=100;++L){
      const auto Reward=MoonfallMoney::ContractReward(Cost,L);
      assert(Reward>=100&&Reward<=1175);
      assert(MoonfallMoney::ContractCredit(Cost,L,0)==Reward);
      assert(MoonfallMoney::ContractCredit(Cost,L,MoonfallMoney::MaxCoins-1)==1);
      assert(MoonfallMoney::ContractCredit(Cost,L,MoonfallMoney::MaxCoins)==0);
      assert(MoonfallMoney::ContractCredit(Cost,L,-1)==0);
    }
    assert(MoonfallMoney::HistoricalSalePrice(1,1,0,0)==40);
    assert(MoonfallMoney::ValidHistoricalSale(1,25,5,3,2430));
    assert(!MoonfallMoney::ValidHistoricalSale(1,25,5,3,2431));
    assert(!MoonfallMoney::ValidHistoricalSale(2,1,0,0,40));
    assert(!MoonfallMoney::ValidHistoricalSale(1,0,0,0,0));
    for(int InvalidCount : {-1,13,2147483647}) {
      MoonfallMoney::FBuybackHistory Invalid;
      Invalid.Count=InvalidCount;
      assert(Invalid.Get(0).Code==0 && Invalid.Get(12).Code==0);
      assert(!Invalid.Remove(0) && !Invalid.Push(1,50));
      assert(Invalid.Browse(0,1)==0 && Invalid.Browse(0,-1)==0);
      assert(Invalid.Count==InvalidCount);
    }
    MoonfallMoney::FBuybackHistory History;
    assert(!History.Push(0,50)&&!History.Push(1,-1)&&!History.Push(1,40,2));
    for(int I=1;I<=15;++I)assert(History.Push(I,I*10));
    assert(History.Count==12&&History.Get(0).Code==15&&History.Get(11).Code==4);
    assert(History.Browse(0,-1)==11&&History.Browse(11,1)==0);
    assert(!History.Remove(12)&&History.Remove(4)&&History.Get(4).Code==10);
    assert(History.Push(15,150)&&History.Get(0).Code==History.Get(1).Code);
    assert(History.Remove(0)&&History.Get(0).Code==15);
    while(History.Count)assert(History.Remove(0));
    assert(History.Get(0).Code==0&&History.Browse(0,1)==0);
    const MoonfallMoney::FTradeIntent Confirmed{0,1001,0,40,1};
    assert(Confirmed.Matches(Confirmed));
    assert(!Confirmed.Matches(MoonfallMoney::FTradeIntent{1,1001,0,40,1}));
    assert(!Confirmed.Matches(MoonfallMoney::FTradeIntent{0,1002,0,40,1}));
    assert(!Confirmed.Matches(MoonfallMoney::FTradeIntent{0,1001,1,40,1}));
    assert(!Confirmed.Matches(MoonfallMoney::FTradeIntent{0,1001,0,41,1}));
    assert(!Confirmed.Matches(MoonfallMoney::FTradeIntent{0,1001,0,40,2}));
    assert(!MoonfallMoney::FTradeIntent{}.Matches(MoonfallMoney::FTradeIntent{}));
    assert(MoonfallMoney::StockGroups==4);
    assert(MoonfallMoney::NextRestock(10000,0)==11800);
    assert(!MoonfallMoney::SpecialtyAvailable(11799,11800));
    assert(MoonfallMoney::SpecialtyAvailable(11800,11800));
    assert(!MoonfallMoney::SpecialtyAvailable(9999,11800));
    assert(!MoonfallMoney::SpecialtyAvailable(0,0));
    assert(!MoonfallMoney::SpecialtyAvailable(10000,-1));
    assert(MoonfallMoney::NextRestock(MoonfallMoney::MaxUtc,0)==0);
    for(int Step=0;Step<1800;++Step)assert(!MoonfallMoney::SpecialtyAvailable(10000+Step,11800));
    assert(MoonfallMoney::SpecialtySlot(1)==0&&MoonfallMoney::SpecialtySlot(2)==1&&MoonfallMoney::SpecialtySlot(3)==8);
    assert(MoonfallMoney::SpecialtyRarity(25,40)==2&&MoonfallMoney::SpecialtyRarity(24,40)==1);
    assert(MoonfallMoney::SpecialtyRarity(25,39)==1&&MoonfallMoney::SpecialtyRarity(1,100)==0);
    assert(MoonfallMoney::ReputationPricePercent(80)==80&&MoonfallMoney::ReputationPricePercent(-40)==120);
    for(int Standing=-100;Standing<=100;++Standing){
      for(int Tier=1;Tier<=25;++Tier){
        const auto Sale=MoonfallMoney::GearSalePrice(Tier,2,1);
        const auto Buy=MoonfallMoney::FactionPurchasePrice(Tier,2,1,Standing);
        assert(Buy>Sale&&Buy<=Sale*4);
      }
    }
    assert(MoonfallMoney::VendorTier(1)==1&&MoonfallMoney::VendorTier(100)==20);
    assert(MoonfallMoney::VendorTier(0)==0&&MoonfallMoney::VendorTier(101)==0);
    assert(MoonfallMoney::TradeMode(0)==1&&MoonfallMoney::TradeMode(1)==2&&MoonfallMoney::TradeMode(2)==0);
    for(int Tier=1;Tier<=25;++Tier)for(int Rarity=0;Rarity<=5;++Rarity)for(int Trait=0;Trait<=3;++Trait){
      const auto Sell=MoonfallMoney::GearSalePrice(Tier,Rarity,Trait);
      const auto Buy=MoonfallMoney::GearPurchasePrice(Tier,Rarity,Trait);
      assert(Sell==0?Buy==0:Buy>Sell);
      if(Sell>0){
        MoonfallMoney::FWallet Trade{Buy};
        assert(Trade.Debit(Buy)&&Trade.Credit(Sell)&&Trade.Copper<Buy);
        assert(Trade.Debit(Sell)&&Trade.Copper==0);
      }
    }
    using namespace MoonfallFishing;
    assert(!InCastRange(9999.0,850.f) && InCastRange(10000.0,850.f));
    assert(InCastRange(850.0*850.0,850.f) && !InCastRange(851.0*851.0,850.f));
    assert(!InCastRange(std::numeric_limits<double>::quiet_NaN(),850.f));
    assert(OnCastSurface(50,50,2,0,0,100,100,0));
    assert(!OnCastSurface(20,50,2,0,0,100,100,0));
    assert(!OnCastSurface(50,50,4,0,0,100,100,0));
    assert(!OnCastSurface(50,50,2,0,0,40,40,0));
    assert(!OnCastSurface(50,50,2,0,0,100,100,std::numeric_limits<double>::quiet_NaN()));
    // Reel completion is a one-shot event for automatic stock awards.
    assert(EndNotice(EStage::Bite,EStage::Idle,true,10)==EEndNotice::Escaped);
    assert(EndNotice(EStage::Reeling,EStage::Idle,false,10)==EEndNotice::Interrupted);
    assert(EndNotice(EStage::Reeling,EStage::Idle,true,10)==EEndNotice::None);
    assert(EndNotice(EStage::Waiting,EStage::Bite,true,10)==EEndNotice::None);
    assert(EndNotice(EStage::Waiting,EStage::Idle,false,10)==EEndNotice::Interrupted);
    FSession AutoLootCast;
    MoonfallOceanGathering::FCollection AutoLootStock;
    assert(AutoLootCast.Start(1,2));
    assert(!AutoLootCast.Advance(3,true));
    assert(AutoLootCast.Hook(3));
    if(AutoLootCast.Advance(4.5,true)) assert(AutoLootStock.Catch(0,0));
    if(AutoLootCast.Advance(5,true)) assert(AutoLootStock.Catch(0,0));
    assert(AutoLootStock.Catches()==1 && AutoLootStock.Species(0)==1);
    FSession BoundaryCast;
    assert(BoundaryCast.Start(5,2));
    assert(!BoundaryCast.Hook(6.999));
    assert(BoundaryCast.Hook(7) && BoundaryCast.Stage==EStage::Reeling);
    assert(!BoundaryCast.Hook(7.1));
    assert(BoundaryCast.Advance(8.5,true));
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
    for(int Id=0;Id<BaseNodeCount;++Id) assert(BaitMaterials.Collect(Id,0,0));
    assert(BaitWorkshop.CanCraft(3,BaitMaterials,BaitGear) && BaitWorkshop.Bait==0);
    assert(BaitWorkshop.Craft(3,BaitMaterials,BaitGear) && BaitWorkshop.Bait==5);
    assert(BaitWorkshop.AvailableMaterials(BaitMaterials)==10);
    assert(BaitWorkshop.UseBait() && BaitWorkshop.Bait==4);
    BaitWorkshop.Bait=20;
    assert(!BaitWorkshop.Craft(3,BaitMaterials,BaitGear));
    assert(BaitWorkshop.AvailableMaterials(BaitMaterials)==10);
    BaitWorkshop.Bait=0;
    assert(!BaitWorkshop.UseBait());
    // All newly recorded crafts must be successful and consume real stock.
    MoonfallOceanCrafting::FWorkshop Jobs;
    FCollection JobSupplies;
    MoonfallAquaticRewards::FSessionGear JobGear;
    assert(!Jobs.Craft(4,JobSupplies,JobGear) && Jobs.CookingCrafts==0);
    for(int I=0;I<BaseNodeCount;++I) assert(JobSupplies.Collect(I,0,0));
    for(int I=0;I<5;++I) assert(JobSupplies.Catch(0,0));
    assert(Jobs.CanCraft(4,JobSupplies,JobGear) && Jobs.CookingCrafts==0);
    assert(Jobs.Craft(4,JobSupplies,JobGear) && Jobs.Provisions==1 && Jobs.CookingCrafts==1);
    assert(Jobs.ConsumeProvision() && !Jobs.ConsumeProvision() && Jobs.CookingCrafts==1);
    assert(Jobs.Craft(0,JobSupplies,JobGear) && Jobs.AlchemyCrafts==1);
    assert(JobGear.Claim(0,99) && Jobs.Craft(1,JobSupplies,JobGear) && Jobs.SmithingCrafts==1);
    assert(!Jobs.Deliver(-1,JobSupplies,JobGear) && !Jobs.Deliver(9,JobSupplies,JobGear));
    assert(Jobs.Deliver(0,JobSupplies,JobGear) && JobGear.TonicCharges==3);
    const int FishAfterDelivery=Jobs.RemainingFish(JobSupplies);
    assert(!Jobs.Deliver(0,JobSupplies,JobGear) && Jobs.ContractCount()==1);
    assert(Jobs.RemainingFish(JobSupplies)==FishAfterDelivery);
    assert(!Jobs.Craft(4,JobSupplies,JobGear) && Jobs.CookingCrafts==1);
    MoonfallProfessionLedger::FSnapshot NewSave,NewRead;
    NewSave.Gathering=JobSupplies; NewSave.Workshop=Jobs; NewSave.Gear=JobGear;
    assert(NewSave.Regions.Visit(0) && NewSave.Regions.Fish(0) && NewSave.Regions.Gather(0));
    int NewLedger[MoonfallProfessionLedger::Count]={};
    NewSave.Encode(NewLedger);
    assert(NewRead.Decode(NewLedger,MoonfallProfessionLedger::Count));
    assert(NewRead.Workshop.CookingCrafts==1 && NewRead.Workshop.AlchemyCrafts==1);
    assert(NewRead.Workshop.SmithingCrafts==1 && NewRead.Workshop.HasDelivered(0));
    assert(NewRead.Decode(NewLedger,MoonfallProfessionLedger::RegionalCount));
    assert(NewRead.Workshop.CookingCrafts==0 && NewRead.Workshop.ContractCount()==0);
    assert(NewRead.Regions.Complete(0));
    NewLedger[MoonfallProfessionLedger::Count-1]=2;
    assert(!NewRead.Decode(NewLedger,MoonfallProfessionLedger::Count));
    assert(NewRead.Workshop.ContractCount()==0);
    MoonfallProfessionLedger::FSnapshot Snapshot,Restored;
    Snapshot.Gathering=BaitMaterials; Snapshot.Workshop=BaitWorkshop;
    Snapshot.Gathering.Catch(0,0,0,true,0);
    Snapshot.Gear.Claim(0,99);
    assert(Snapshot.Regions.Visit(0) && Snapshot.Regions.Fish(0) && Snapshot.Regions.Gather(0));
    assert(!Snapshot.Regions.Visit(0) && Snapshot.Regions.Complete(0) && Snapshot.Regions.Completed()==1);
    assert(!Snapshot.Regions.Visit(9) && !Snapshot.Regions.Fish(-1));
    int Ledger[MoonfallProfessionLedger::Count]={};
    Snapshot.Encode(Ledger);
    assert(Restored.Decode(Ledger,MoonfallProfessionLedger::Count));
    assert(Restored.Gathering.HasLegendary(0) && Restored.Gathering.TrophyCatches==1);
    assert(Restored.Regions.Complete(0));
    assert(Restored.Regions.Milestones(0)==3 && Restored.Regions.HasVisited(0));
    assert(Restored.Regions.Milestones(-1)==0 && !Restored.Regions.HasVisited(9));
    assert(Restored.Regions.Fish(1) && Restored.Regions.Milestones(1)==1 && !Restored.Regions.HasVisited(1));
    MoonfallProfessionLedger::FSnapshot Migrated;
    assert(Migrated.Decode(Ledger,MoonfallProfessionLedger::LegacyCount));
    assert(Migrated.Regions.Completed()==0 && Migrated.Gathering.HasLegendary(0));
    assert(Restored.Workshop.AvailableMaterials(Restored.Gathering)==10 && Restored.Gear.Count()==1);
    Ledger[4]=100;
    assert(!Restored.Decode(Ledger,MoonfallProfessionLedger::Count));
    assert(Restored.Gathering.HasLegendary(0));
    assert(!Restored.Decode(Ledger,MoonfallProfessionLedger::Count-1));
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
    for(int I=0;I<MoonfallNorth::ZoneCount;++I) {
        assert(MoonfallRegionalFishing::Preferred(I,10)>=0);
        assert(MoonfallRegionalFishing::TrophyBonus(I,10,true)>=1);
        assert(MoonfallRegionalFishing::TrophyBonus(I,10,false)==0);
    }
    assert(MoonfallRegionalFishing::Preferred(8,7)==-1);
    assert(MoonfallRegionalFishing::Preferred(8,8)==0);
    assert(MoonfallRegionalFishing::Preferred(-1,10)==-1);
    FCollection Regional;
    for(int Province=0;Province<9;++Province)
      for(int Type=0;Type<4;++Type) {
        const int Id=RegionalNode(Province,Type);
        assert(Id>=BaseNodeCount && MaterialType(Id)==Type);
        assert(Regional.Collect(Id,0,0));
        assert(!Regional.Collect(Id,0,0));
      }
    assert(Regional.Materials()==36 && RegionalNode(9,0)==-1 && RegionalNode(0,4)==-1);
    for(int I=0;I<MoonfallNorth::ZoneCount;++I) {
      const auto Rect=MoonfallRegionalAtlas::Node(I,800,600);
      assert(MoonfallRegionalAtlas::Hit(Rect.X+Rect.W/2,Rect.Y+Rect.H/2,800,600)==I);
    }
    assert(MoonfallRegionalAtlas::Step(-1,1,5)==0);
    assert(MoonfallRegionalAtlas::Step(0,1,5)==2);
    assert(MoonfallRegionalAtlas::Step(2,1,5)==-1);
    assert(MoonfallRegionalAtlas::Step(-1,-1,5)==2);
    assert(MoonfallRegionalAtlas::Step(-1,1,0)==-1);
    // Every discovery mask must traverse exactly its available provinces,
    // in both directions, before returning to the clear-waypoint slot.
    for(unsigned Mask=0;Mask<(1u<<MoonfallNorth::ZoneCount);++Mask) {
      for(int Direction : {-1,1}) {
        int Current=-1;
        for(int I=0;I<MoonfallNorth::ZoneCount;++I) {
          const int Expected=Direction==1?I:MoonfallNorth::ZoneCount-1-I;
          if(!(Mask&(1u<<Expected))) continue;
          Current=MoonfallRegionalAtlas::Step(Current,Direction,Mask);
          assert(Current==Expected);
        }
        assert(MoonfallRegionalAtlas::Step(Current,Direction,Mask)==-1);
      }
    }
    assert(MoonfallRegionalAtlas::Step(-1,0,~0u)==-1);
    assert(MoonfallRegionalAtlas::Step(999,1,1u)==0);
    assert(MoonfallProfessionUI::Rows(280)==3);
    assert(MoonfallProfessionUI::Rows(860)==25);
    assert(MoonfallProfessionUI::Start(99,3)==22);
    assert(MoonfallProfessionUI::Step(22,3,1)==22);
    assert(MoonfallProfessionUI::Step(22,25,-1)==0);
    assert(MoonfallRegionalAtlas::Node(0,std::numeric_limits<float>::quiet_NaN(),600).W==0);
    return 0;
}
