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
    return 0;
}
