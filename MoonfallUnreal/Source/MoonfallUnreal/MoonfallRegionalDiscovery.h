#pragma once
#include "MoonfallRegionalFishing.h"
namespace MoonfallRegionalDiscovery {
 constexpr unsigned Mask=(1u<<MoonfallNorth::ZoneCount)-1u;
 struct FProgress {
  unsigned Visited=0,Caught=0,Harvested=0;
  static bool Mark(unsigned& Bits,int Id) {
   if(!MoonfallNorth::ValidZone(Id)) return false;
   const unsigned Bit=1u<<Id;
   if(Bits&Bit) return false;
   Bits|=Bit; return true;
  }
  bool Visit(int Id) { return Mark(Visited,Id); }
  bool Fish(int Id) { return Mark(Caught,Id); }
  bool Gather(int Id) { return Mark(Harvested,Id); }
  int Milestones(int Id) const {
   if(!MoonfallNorth::ValidZone(Id)) return 0;
   const unsigned Bit=1u<<Id;
   return int(bool(Visited&Bit))+int(bool(Caught&Bit))+int(bool(Harvested&Bit));
  }
  bool HasVisited(int Id) const { return MoonfallNorth::ValidZone(Id)&&(Visited&(1u<<Id)); }
  bool Complete(int Id) const { return MoonfallNorth::ValidZone(Id)&&((Visited&Caught&Harvested)&(1u<<Id)); }
  static int Count(unsigned Bits) {
   int N=0; for(int I=0;I<MoonfallNorth::ZoneCount;++I) if(Bits&(1u<<I)) ++N;
   return N;
  }
  int Completed() const { return Count(Visited&Caught&Harvested); }
  bool Valid() const { return ((Visited|Caught|Harvested)&~Mask)==0; }
 };
}
