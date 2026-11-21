#pragma once
#include "CoreMinimal.h"

// Shared quality presentation only. IDs 0..5 are part of the existing item
// encoding and save format; changing their order would corrupt gear meaning.
// World of Warcraft-style colors for shared tiers, with Moonfall's distinctive
// jade-teal Emerald at index 4. No new runtime state or per-frame allocation.
namespace MoonfallItemQuality
{
static constexpr int32 Count = 6;

inline bool IsValid(int32 Rarity)
{
 return Rarity >= 0 && Rarity < Count;
}

inline FLinearColor Color(int32 Rarity)
{
 static const FLinearColor Colors[Count] = {
  FLinearColor(1.f,1.f,1.f),             // 0 Common: white
  FLinearColor(0.118f,1.f,0.f),         // 1 Uncommon: green #1EFF00
  FLinearColor(0.f,0.439f,0.867f),      // 2 Rare: blue #0070DD
  FLinearColor(0.639f,0.208f,0.933f),  // 3 Epic: purple #A335EE
  FLinearColor(0.f,0.84f,0.70f),        // 4 Emerald: unique jade-teal
  FLinearColor(1.f,0.502f,0.f)          // 5 Legendary: orange #FF8000
 };
 return IsValid(Rarity) ? Colors[Rarity] : FLinearColor::White;
}

// Ground-loot text expects an 8-bit color. Disable gamma conversion so the
// byte values match the numeric HUD palette and don't drift independently.
inline FColor WorldColor(int32 Rarity)
{
 return Color(Rarity).ToFColor(false);
}

inline const TCHAR* Abbreviation(int32 Rarity)
{
 static const TCHAR* const Marks[Count] = {
  TEXT("C"), TEXT("U"), TEXT("R"), TEXT("E"), TEXT("Em"), TEXT("L")
 };
 return IsValid(Rarity) ? Marks[Rarity] : TEXT("?");
}
}
