#pragma once
namespace MoonfallIslandQuest
{
    // Discovery progress can precede acceptance; rewards cannot be farmed.
    constexpr int Required(int Id) { return Id==0?2:Id==1?2:Id==2?6:0; }
    struct FJournal {
        unsigned Accepted=0, Completed=0;
        int Talk(int Id,int Progress) {
            if (Required(Id)==0) return 0;
            const unsigned Bit=1u<<Id;
            if (Completed&Bit) return 3;
            if (!(Accepted&Bit)) { Accepted|=Bit; return 1; }
            if (Progress<Required(Id)) return 1;
            Completed|=Bit;
            return 2;
        }
        int State(int Id) const {
            if (!Required(Id)) return 0;
            const unsigned Bit=1u<<Id;
            return Completed&Bit?2:Accepted&Bit?1:0;
        }
        int Count() const {
            int N=0; for (int I=0;I<3;++I) if (Completed&(1u<<I)) ++N; return N;
        }
    };
}
