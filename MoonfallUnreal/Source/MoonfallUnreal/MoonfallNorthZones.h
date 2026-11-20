#pragma once
// Original Moonfall northern expansion zone catalog, inspired by the geographic
// variety of large fantasy continents, not by any Warcraft names or assets.
// Data only: does not allocate actors, edit saves, create travel routes, or
// change the six legacy region IDs / 30 dungeon progress-bit positions.
#include <cstddef>
#include <cstdint>
namespace MoonfallNorth {
inline constexpr int ZoneCount = 9;
inline constexpr int FirstFutureRegionId = 6;
enum class EBiome : unsigned char {
 GlacialCoast, PineForest, Tundra, Deadlands, LivingBasin,
 HighMountains, RuinedCity, CrystalValley, EclipseCitadel
};
struct FZone {
 const char* Id;
 const char* Name;
 EBiome Biome;
 int SuggestedLevel;
 const char* Hub;
 const char* WorldThreat;
 const char* Dungeon;
 const char* Boss;
 unsigned int NeighborMask;
};
// Nine original interconnected provinces. Bits index this catalog only.
// World placement, navigation meshes, streaming and actual encounters are
// separate implementation tasks and are NOT implied by this data.
inline constexpr FZone Zones[ZoneCount] = {
 {"north_01", "Shiverwake Coast", EBiome::GlacialCoast, 25,
  "Tideglass Haven", "Drowned icebound raiders", "Saltglass Grotto", "The Brine Warden", (1u<<1)|(1u<<2)},
 {"north_02", "Wolfsong Timberlands", EBiome::PineForest, 29,
  "Emberpine Lodge", "Hollow wolves and corrupted treants", "Rootfang Den", "The Thorn Matriarch", (1u<<0)|(1u<<2)|(1u<<4)},
 {"north_03", "Palehorn Tundra", EBiome::Tundra, 31,
  "Greyhorn Outpost", "Stampeding frost beasts", "The White Maw", "Palehorn Colossus", (1u<<0)|(1u<<1)|(1u<<3)|(1u<<4)},
 {"north_04", "Gravewind Barrens", EBiome::Deadlands, 36,
  "Lantern Rest", "Restless oathbound dead", "Hushed Sepulcher", "The Bellkeeper", (1u<<2)|(1u<<5)|(1u<<6)},
 {"north_05", "Verdant Rift", EBiome::LivingBasin, 34,
  "Bloomwatch Refuge", "Predatory ancient flora", "Heartseed Hollow", "The Root Devourer", (1u<<1)|(1u<<2)|(1u<<5)},
 {"north_06", "Skyshard Summits", EBiome::HighMountains, 40,
  "Cloudbreak Hold", "Stormforged giants", "Thunderbrand Vault", "The Skybreaker", (1u<<3)|(1u<<4)|(1u<<7)},
 {"north_07", "The Sunken Crown", EBiome::RuinedCity, 43,
  "Lastlight Enclave", "Clockwork revenants", "Crownfall Archive", "The Regent of Rust", (1u<<3)|(1u<<7)|(1u<<8)},
 {"north_08", "Starveil Vale", EBiome::CrystalValley, 42,
  "Prismwatch", "Fractured astral sentinels", "The Mirror Choir", "The Prism Oracle", (1u<<5)|(1u<<6)|(1u<<8)},
 {"north_09", "Nightspire Dominion", EBiome::EclipseCitadel, 48,
  "Dawn's Edge", "Eclipsebound war hosts", "Obsidian Meridian", "The Uncrowned Star", (1u<<6)|(1u<<7)}
};
constexpr bool ValidZone(int Index) { return Index >= 0 && Index < ZoneCount; }
constexpr bool Linked(int A, int B) {
 return ValidZone(A) && ValidZone(B) && A != B &&
  (Zones[A].NeighborMask & (1u << B)) != 0u;
}
constexpr bool SymmetricLinks() {
 for (int A = 0; A < ZoneCount; ++A) {
  if ((Zones[A].NeighborMask >> ZoneCount) != 0u) return false;
  for (int B = 0; B < ZoneCount; ++B)
   if (Linked(A,B) != Linked(B,A)) return false;
 }
 return true;
}
constexpr bool FullyConnected() {
 // Constant-time-ish graph flood fill for a small immutable 9-zone catalog.
 unsigned int Reached = 1u;
 for (int Pass=0; Pass<ZoneCount; ++Pass)
  for (int Zone=0; Zone<ZoneCount; ++Zone)
   if ((Reached & (1u<<Zone)) != 0u) Reached |= Zones[Zone].NeighborMask;
 return Reached == ((1u<<ZoneCount)-1u);
}
// Morvella's relocated story home. This is an authored location binding,
// not a loaded actor. The legacy sixth-region encounter must be disabled
// until a playable Gravewind Barrens level and travel integration exist.
inline constexpr int MorvellaProvince = 3; // Gravewind Barrens
inline constexpr const char* MorvellaEncounterId = "death_goddess_crimson_reaper";
inline constexpr const char* MorvellaSanctum = "Moonlit Necropolis";
constexpr bool MorvellaHasNorthernHome() {
 return ValidZone(MorvellaProvince)
   && Zones[MorvellaProvince].Biome == EBiome::Deadlands;
}
// Returns the shortest proposed province-to-province route.
// This is a read-only navigation plan, not world travel or fast-travel.
// No heap allocations; invalid destinations return Length=0.
struct FRoute {
 int Length;
 int Stops[ZoneCount];
};
constexpr FRoute ShortestRoute(int Start, int Goal) {
 FRoute Route{};
 if (!ValidZone(Start) || !ValidZone(Goal)) return Route;
 int Prev[ZoneCount] = {};
 bool Seen[ZoneCount] = {};
 int Queue[ZoneCount] = {};
 for (int Index=0; Index<ZoneCount; ++Index) Prev[Index] = -1;
 int Head=0, Tail=0;
 Queue[Tail++] = Start;
 Seen[Start] = true;
 while (Head<Tail && !Seen[Goal]) {
  const int Current=Queue[Head++];
  for (int Next=0; Next<ZoneCount; ++Next) {
   if (!Seen[Next] && Linked(Current,Next)) {
    Seen[Next] = true;
    Prev[Next] = Current;
    Queue[Tail++] = Next;
   }
  }
 }
 if (!Seen[Goal]) return Route;
 int Reverse[ZoneCount] = {};
 int Count=0;
 for (int Current=Goal; Current!=-1; Current=Prev[Current])
  Reverse[Count++] = Current;
 Route.Length = Count;
 for (int Index=0; Index<Count; ++Index)
  Route.Stops[Index] = Reverse[Count-Index-1];
 return Route;
}
static_assert(MorvellaHasNorthernHome(), "Death Goddess must live in Gravewind Barrens");
static_assert(ZoneCount == 9, "Northern continent requires nine regions");
static_assert(SymmetricLinks(), "Province borders must be bidirectional");
static_assert(FullyConnected(), "Every northern zone must be reachable");
static_assert(ShortestRoute(0,0).Length == 1, "Origin must route to itself");
static_assert(ShortestRoute(0,8).Length >= 3, "Final stronghold requires travel");
static_assert(ShortestRoute(-1,8).Length == 0, "Invalid origin must fail");
static_assert(!Linked(-1,0) && !Linked(0,ZoneCount), "Reject invalid regions");
} // namespace MoonfallNorth
