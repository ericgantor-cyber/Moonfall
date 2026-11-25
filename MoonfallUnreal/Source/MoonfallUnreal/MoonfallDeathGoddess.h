#pragma once
#include "CoreMinimal.h"
#include "MoonfallDeathGoddessFootprint.h"

// Pure, allocation-free encounter rules for the optional Death Goddess.
// This does not alter the original Z/Nyra encounter IDs or artwork.
namespace MoonfallDeathGoddess {
 inline FName BossId() {
  static const FName Id(TEXT("death_goddess_crimson_reaper"));
  return Id;
 }
 inline bool IsDeathGoddess(const FName Id) { return Id == BossId(); }
 // Four reusable, one-hit marked spells. No hidden actor spawns or
 // instant kills. Existing line-of-sight and dodge/parry rules still apply.
 struct FSignature {
  FName Id;
  float Radius;
  float WarningSeconds;
  float DamageMultiplier;
  bool bCenteredOnCaster;
 };
 inline FSignature SignatureForPhase(const int32 Phase) {
  switch (FMath::Clamp(Phase, 1, 4)) {
   case 1: return {FName(TEXT("GraveNeedle")), 110.f, 1.55f, 1.3f, false};
   case 2: return {FName(TEXT("SoulHarvest")), 345.f, 1.7f, 1.35f, true};
   case 3: return {FName(TEXT("CrimsonEclipse")), 310.f, 1.35f, 1.55f, false};
   default: return {FName(TEXT("FinalVerdict")), 400.f, 1.85f, 1.9f, false};
  }
 }
 inline const TCHAR* WarningForPhase(const int32 Phase) {
  switch (FMath::Clamp(Phase, 1, 4)) {
   case 1: return TEXT("GRAVE NEEDLE");
   case 2: return TEXT("SOUL HARVEST");
   case 3: return TEXT("CRIMSON ECLIPSE");
   default: return TEXT("FINAL VERDICT");
  }
 }
 // Move IDs, rather than the *current* HP phase, lock the collision shape
 // for each cast. Interrupted casts never use these hit calculations.
 inline MoonfallDeathGoddessFootprint::Shape FootprintForMove(FName Move) {
  using Shape = MoonfallDeathGoddessFootprint::Shape;
  if (Move == FName(TEXT("GraveNeedle"))) return Shape::GraveNeedle;
  if (Move == FName(TEXT("SoulHarvest"))) return Shape::SoulHarvest;
  if (Move == FName(TEXT("CrimsonEclipse"))) return Shape::CrimsonEclipse;
  if (Move == FName(TEXT("FinalVerdict"))) return Shape::FinalVerdict;
  return static_cast<Shape>(0);
 }
 inline bool HitsSignature(FName Move, const FVector& Origin, const FVector& Target,
                           const FVector& Hero, const float Radius) {
  if (Origin.ContainsNaN() || Target.ContainsNaN() || Hero.ContainsNaN()) return false;
  return MoonfallDeathGoddessFootprint::Hit(FootprintForMove(Move),
    Origin.X, Origin.Y, Target.X, Target.Y, Hero.X, Hero.Y, Radius);
 }
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
