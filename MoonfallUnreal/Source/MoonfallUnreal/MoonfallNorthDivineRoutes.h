#pragma once
// Portable, read-only routes to the three Northern Expanse divine sanctums.
// A route uses discovered province connections only, never undiscovered
// shortcuts. No UE actors, coordinates, fast travel, saves or art mutations.
#include "MoonfallNorthDivine.h"
namespace MoonfallNorth {
enum class EDivineSite : unsigned char { Morvella = 0, Z = 1, Nyra = 2 };
struct FDivineSite {
 const char* EncounterId;
 const char* Sanctum;
 int Province;
};
constexpr FDivineSite DivineSite(EDivineSite Site) {
 switch (Site) {
  case EDivineSite::Morvella: return {MorvellaEncounterId, MorvellaSanctum, MorvellaProvince};
  case EDivineSite::Z: return {ZEncounterId, ZSanctum, ZProvince};
  case EDivineSite::Nyra: return {NyraEncounterId, NyraSanctum, NyraProvince};
 }
 return {nullptr, nullptr, -1};
}
constexpr FRoute DiscoveredRoute(int Start, int Goal, std::uint32_t Discovered) {
 FRoute Result{};
 if (!ValidProvinceMask(Discovered) || !ValidZone(Start) || !ValidZone(Goal))
  return Result;
 if ((Discovered & (1u<<Start)) == 0u || (Discovered & (1u<<Goal)) == 0u)
  return Result;
 int Previous[ZoneCount] = {};
 bool Seen[ZoneCount] = {};
 int Queue[ZoneCount] = {};
 for (int Index=0; Index<ZoneCount; ++Index) Previous[Index] = -1;
 int Head=0, Tail=0;
 Queue[Tail++] = Start;
 Seen[Start] = true;
 while (Head<Tail && !Seen[Goal]) {
  const int Current=Queue[Head++];
  for (int Next=0; Next<ZoneCount; ++Next) {
   if (!Seen[Next] && Linked(Current,Next) &&
       (Discovered & (1u<<Next)) != 0u) {
    Seen[Next] = true;
    Previous[Next] = Current;
    Queue[Tail++] = Next;
   }
  }
 }
 if (!Seen[Goal]) return Result;
 int Reverse[ZoneCount] = {};
 int Count=0;
 for (int Current=Goal; Current!=-1; Current=Previous[Current])
  Reverse[Count++] = Current;
 Result.Length=Count;
 for (int Index=0; Index<Count; ++Index)
  Result.Stops[Index]=Reverse[Count-Index-1];
 return Result;
}
constexpr FRoute RouteToDivineSite(EDivineSite Site,
 std::uint32_t Discovered) {
 return DiscoveredRoute(NorthernLandingProvince, DivineSite(Site).Province, Discovered);
}
static_assert(RouteToDivineSite(EDivineSite::Morvella, 1u).Length==0,
 "Do not route to undiscovered Necropolis");
static_assert(RouteToDivineSite(EDivineSite::Z, AllProvinceBits).Length>0,
 "Z must have a connected route");
static_assert(RouteToDivineSite(EDivineSite::Nyra, AllProvinceBits).Length>0,
 "Nyra must have a connected route");
static_assert(DiscoveredRoute(0,0,1u).Length==1,
 "Discovered landing location routes to itself");
static_assert(DiscoveredRoute(0,3,(1u<<0)|(1u<<3)).Length==0,
 "Do not route across undiscovered gaps");
} // namespace MoonfallNorth
