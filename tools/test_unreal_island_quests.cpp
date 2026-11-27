#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallIslandQuest.h"
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
    return 0;
}
