#pragma once
#include <cmath>
namespace MoonfallOceanGathering
{
    constexpr int NodeCount=12;
    constexpr float RespawnSeconds=90.f;
    constexpr int MaxMaterialPerType=1000000;
    constexpr bool IsResource(int Id) { return Id>=0 && Id<NodeCount; }
    constexpr int MaterialType(int Id) { return IsResource(Id)?Id%4:-1; }
    constexpr bool CanCollect(int Id,bool Swimming,bool /*Grounded*/,bool Manual) {
        return IsResource(Id) && Swimming && Manual;
    }
    struct FCollection {
        double ReadyAt[NodeCount]={};
        int Harvested[4]={};
        int Fish[3]={};
        bool Ready(int Id,double Now) const {
            return IsResource(Id) && std::isfinite(Now) && Now>=0.0 && Now>=ReadyAt[Id]
                && Now+RespawnSeconds>Now && Harvested[MaterialType(Id)]<MaxMaterialPerType;
        }
        bool Collect(int Id,int Event,double Now) {
            if(Event<0||Event>2||!Ready(Id,Now)) return false;
            ReadyAt[Id]=Now+RespawnSeconds;
            ++Harvested[MaterialType(Id)];
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
            return Harvested[Type];
        }
        int Materials() const {
            int N=0; for(int I=0;I<4;++I) N+=Material(I); return N;
        }
        int Catches() const { return Fish[0]+Fish[1]+Fish[2]; }
        int Species(int Id) const { return Id>=0&&Id<3?Fish[Id]:0; }
    };
}
