#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallIslandQuest.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallAquaticRewards.h"
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
    return 0;
}
