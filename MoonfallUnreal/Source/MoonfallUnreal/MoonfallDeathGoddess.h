#pragma once
#include "CoreMinimal.h"

// Pure, allocation-free encounter rules for the optional Death Goddess.
// This does not alter the original Z/Nyra encounter IDs or artwork.
namespace MoonfallDeathGoddess {
 inline FName BossId() {
  static const FName Id(TEXT("death_goddess_crimson_reaper"));
  return Id;
 }
 inline bool IsDeathGoddess(const FName Id) { return Id == BossId(); }
 // Boundary HP values belong to the *next* phase. Invalid ratios do not
 // escalate a boss; calling combat logic validates health separately.
 inline int32 PhaseForHealthRatio(const float Ratio) {
  if (!FMath::IsFinite(Ratio)) return 1;
  if (Ratio <= 0.25f) return 4;
  if (Ratio <= 0.50f) return 3;
  if (Ratio <= 0.75f) return 2;
  return 1;
 }
}
