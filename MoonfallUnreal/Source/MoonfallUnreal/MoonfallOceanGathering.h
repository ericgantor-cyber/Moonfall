#pragma once
namespace MoonfallOceanGathering
{
    constexpr int NodeCount=12;
    constexpr bool IsResource(int Id) { return Id>=0 && Id<NodeCount; }
    constexpr int MaterialType(int Id) { return IsResource(Id)?Id%4:-1; }
    constexpr bool CanCollect(int Id,bool Swimming,bool /*Grounded*/,bool Manual) {
        return IsResource(Id) && Swimming && Manual;
    }
    struct FCollection {
        unsigned Claimed=0;
        int Fish[3]={};
        bool Collect(int Id,int Event) {
            if(!IsResource(Id)||Event<0||Event>2) return false;
            const unsigned Bit=1u<<Id;
            if(Claimed&Bit) return false;
            Claimed|=Bit;
            return true;
        }
        bool Catch(int Event,int Roll) {
            if(Event<0||Event>2||Roll<0||Roll>99||Catches()>=30) return false;
            const int SpeciesId=((Roll<50?0:Roll<80?1:2)+Event)%3;
            ++Fish[SpeciesId];
            return true;
        }
        int Material(int Type) const {
            if(Type<0||Type>=4) return 0;
            int N=0; for(int I=Type;I<NodeCount;I+=4) if(Claimed&(1u<<I)) ++N; return N;
        }
        int Materials() const {
            int N=0; for(int I=0;I<4;++I) N+=Material(I); return N;
        }
        int Catches() const { return Fish[0]+Fish[1]+Fish[2]; }
        int Species(int Id) const { return Id>=0&&Id<3?Fish[Id]:0; }
    };
}
