#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallIslandQuest.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAquaticRewards.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAquaticTactics.h"
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
    return 0;
}
