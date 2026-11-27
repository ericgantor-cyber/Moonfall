#pragma once
#include <cmath>
namespace MoonfallOceanGathering
{
    constexpr int NodeCount=12;
    constexpr float RespawnSeconds=90.f;
    constexpr int MaxMaterialPerType=1000000;
    constexpr int MaxSessionCatches=1000000; // arithmetic safety guard, not a gameplay quota
    constexpr int GatheringRank(int Harvests) { return Harvests<0?1:Harvests>=36?10:1+Harvests/4; }
    constexpr bool IsResource(int Id) { return Id>=0 && Id<NodeCount; }
    constexpr int MaterialType(int Id) { return IsResource(Id)?Id%4:-1; }
    constexpr bool CanCollect(int Id,bool Swimming,bool /*Grounded*/,bool Manual) {
        return IsResource(Id) && Swimming && Manual;
    }
    struct FCollection {
        double ReadyAt[NodeCount]={};
        int Harvested[4]={};
        int Fish[3]={};
        int TrophyCatches=0;
        int TrophySpecies[3]={};
        int LastSpecies=-1;
        bool LastTrophy=false;
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
        bool CanCatch() const { return Catches()<MaxSessionCatches; }
        bool Catch(int Event,int Roll,int Preferred=-1,bool Trophy=false) {
            if(Event<0||Event>2||Roll<0||Roll>99||Preferred<-1||Preferred>2||!CanCatch()) return false;
            const int SpeciesId=Preferred>=0&&Roll<75?Preferred:((Roll<50?0:Roll<80?1:2)+Event)%3;
            ++Fish[SpeciesId];
            LastSpecies=SpeciesId;
            LastTrophy=Trophy;
            if(Trophy) { ++TrophyCatches; ++TrophySpecies[SpeciesId]; }
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
        int Trophies(int Id) const { return Id>=0&&Id<3?TrophySpecies[Id]:0; }
        int DiscoveredSpecies() const { return int(Fish[0]>0)+int(Fish[1]>0)+int(Fish[2]>0); }
    };
}
