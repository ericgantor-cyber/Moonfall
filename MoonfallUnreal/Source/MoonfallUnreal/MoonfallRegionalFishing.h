#pragma once
#include "MoonfallNorthZones.h"
namespace MoonfallRegionalFishing {
 struct FProfile { int Preferred; int SchoolBonus; int MinRank; const char* Habitat; };
 inline constexpr FProfile Profiles[MoonfallNorth::ZoneCount]={
  {0,2,1,"Glacial coast"}, {2,1,2,"Forest rivers"}, {0,2,3,"Tundra pools"},
  {1,2,4,"Barrens springs"}, {2,3,4,"Living basin"}, {1,2,5,"Mountain lakes"},
  {1,3,6,"Sunken ruins"}, {2,3,7,"Crystal waters"}, {0,3,8,"Eclipse pools"}
 };
 constexpr bool Valid(int Region) { return MoonfallNorth::ValidZone(Region); }
 constexpr bool Eligible(int Region,int Rank) { return Valid(Region)&&Rank>=Profiles[Region].MinRank; }
 constexpr int Preferred(int Region,int Rank) { return Eligible(Region,Rank)?Profiles[Region].Preferred:-1; }
 constexpr int TrophyBonus(int Region,int Rank,bool School) {
  return School&&Eligible(Region,Rank)?Profiles[Region].SchoolBonus:0;
 }
 constexpr bool ValidProfiles() {
  for(int I=0;I<MoonfallNorth::ZoneCount;++I)
   if(Profiles[I].Preferred<0||Profiles[I].Preferred>2||Profiles[I].MinRank<1||Profiles[I].MinRank>10||
    Profiles[I].SchoolBonus<0||Profiles[I].SchoolBonus>3) return false;
  return true;
 }
 static_assert(ValidProfiles(),"Regional fishing catalog must fit existing species and ranks");
}
