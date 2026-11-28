#pragma once
namespace MoonfallOceanEvents
{
    // Caller supplies a finite, nonnegative, wrapped world-clock second.
    // 60 seconds calm, 60 seconds migration, 60 seconds hunting.
    constexpr int Activity(int Second) {
        return Second<0||Second>=180?0:Second/60;
    }
    constexpr float PatrolScale(int Event,int Id) {
        if (Id==5 || Id==1) return 1.f;
        return Event==1?1.45f:Event==2?1.15f:1.f;
    }
    constexpr float AwarenessRadius(int Event,int Id) {
        return Id==5||Id==1?480.f:Event==2?540.f:480.f;
    }
    struct FSightings {
        unsigned Seen=0;
        bool Discover(int Id) {
            if(Id<0||Id>5) return false;
            const unsigned Bit=1u<<Id;
            if(Seen&Bit) return false;
            Seen|=Bit;
            return true;
        }
        bool Has(int Id) const { return Id>=0&&Id<=5&&(Seen&(1u<<Id)); }
        int Count() const {
            int N=0; for(int I=0;I<6;++I) if(Has(I)) ++N; return N;
        }
    };
}
