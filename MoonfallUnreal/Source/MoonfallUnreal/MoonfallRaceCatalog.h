#pragma once
#include "CoreMinimal.h"

// Stable race IDs shared by heroes, townsfolk and hostile humanoids.
// Never reorder IDs after saves ship. No new assets or game modes.
namespace MoonfallRaces {
 static constexpr int32 Count = 12;
 inline int32 Validated(int32 Id) { return FMath::Clamp(Id, 0, Count - 1); }
 inline const TCHAR* Name(int32 Id) {
  static const TCHAR* Names[Count] = {
   TEXT("Human"), TEXT("High Elf"), TEXT("Wood Elf"), TEXT("Dwarf"),
   TEXT("Halfling"), TEXT("Orc"), TEXT("Goblin"), TEXT("Troll"),
   TEXT("Entkin"), TEXT("Shadowborn"), TEXT("Moonborn"), TEXT("Draconid")
  };
  return Names[Validated(Id)];
 }
 inline const TCHAR* Greeting(int32 Id) {
  static const TCHAR* Greetings[Count] = {
   TEXT("Safe roads, traveler."), TEXT("May the stars guide your magic."),
   TEXT("The forest remembers every step."), TEXT("Stone holds firm. So do we."),
   TEXT("A warm hearth is worth any treasure."), TEXT("Strength needs wisdom, traveler."),
   TEXT("A clever bargain can save a life."), TEXT("Patience makes the hunt."),
   TEXT("The roots tell us the world still lives."), TEXT("Trust is earned in the dark."),
   TEXT("The moon watches every road."), TEXT("Honor burns brighter than fire.")
  };
  return Greetings[Validated(Id)];
 }
 // Conservative relative combat stats for hostile non-boss members.
 inline float HealthScale(int32 Id) {
  static const float Factors[Count] = {1.f, 0.92f, 0.95f, 1.12f, 0.9f, 1.1f,
    0.9f, 1.15f, 1.12f, 0.95f, 0.98f, 1.1f};
  return Factors[Validated(Id)];
 }
 inline float DamageScale(int32 Id) {
  static const float Factors[Count] = {1.f, 1.07f, 1.04f, 1.f, 0.95f, 1.08f,
    1.04f, 1.03f, 0.95f, 1.07f, 1.08f, 1.08f};
  return Factors[Validated(Id)];
 }
}
