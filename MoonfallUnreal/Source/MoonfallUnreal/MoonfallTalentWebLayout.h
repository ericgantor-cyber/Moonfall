#pragma once
// Pure geometry shared by UE5 Canvas drawing and mouse hit-testing.
// Gameplay ranks and the original 141-node data remain in MoonfallTalentRules.h.
// No widget assets, no Tick scans, no saved input/viewport state.
#include "CoreMinimal.h"
#include "MoonfallTalentRules.h"

namespace MoonfallTalentWeb
{
struct FPanel
{
 float Left, Top, Width, Height, MapLeft, MapTop, MapWidth, MapHeight;
 bool bUsable;
};

inline FPanel Panel(float ViewWidth, float ViewHeight)
{
 const float Width = FMath::Max(0.f, FMath::Min(1100.f, ViewWidth - 24.f));
 const float Height = FMath::Max(0.f, FMath::Min(740.f, ViewHeight - 24.f));
 const float Left = (ViewWidth - Width) * 0.5f;
 const float Top = (ViewHeight - Height) * 0.5f;
 return {Left, Top, Width, Height,
         Left + 16.f, Top + 70.f, FMath::Max(0.f, Width - 283.f),
         FMath::Max(0.f, Height - 135.f), Width >= 535.f && Height >= 355.f};
}

inline const TArray<FVector2D>& Positions()
{
 // Mirrors the tier/branch/lane layout of Godot's moonfall_talent_web.gd.
 // Construct once; never rebuild 141 positions on HUD redraw or mouse motion.
 static const TArray<FVector2D> Points = []()
 {
  TArray<FVector2D> Result;
  TMap<FString, int32> LaneCount;
  Result.Reserve(MoonfallTalents::Catalog().Num());
  static const float LaneAngles[4] = {-26.f, -8.f, 8.f, 26.f};
  for (const MoonfallTalents::FNode& Node : MoonfallTalents::Catalog())
  {
   const FString Branch(Node.Branch);
   const FString Key = Branch + TEXT("_") + FString::FromInt(Node.Tier);
   const int32 Lane = LaneCount.FindRef(Key);
   LaneCount.Add(Key, Lane + 1);
   float Degrees = Branch == TEXT("might") ? -150.f
       : Branch == TEXT("agility") ? -30.f : 90.f;
   if (Node.Tier == 2) Degrees += Lane == 0 ? -13.f : 13.f;
   else if (Node.Tier >= 4) {
    Degrees += LaneAngles[FMath::Min(Lane, 3)];
    if (Node.Tier % 2 == 0) Degrees += Lane % 2 == 0 ? 3.5f : -3.5f;
   }
   float Radius = 105.f + (Node.Tier - 1) * 116.f;
   if (Node.Tier >= 4) Radius += Lane == 1 || Lane == 2 ? 24.f : -12.f;
   const float Radians = FMath::DegreesToRadians(Degrees);
   Result.Add(FVector2D(FMath::Cos(Radians), FMath::Sin(Radians)) * Radius);
  }
  return Result;
 }();
 return Points;
}

struct FLinks { int32 A = INDEX_NONE; int32 B = INDEX_NONE; };
inline const TArray<FLinks>& Dependencies()
{
 // Resolve all prerequisite lookups once rather than scanning 141 IDs
 // for every drawn connection at 60+ HUD frames per second.
 static const TArray<FLinks> Links = []() {
  TMap<FName, int32> Index;
  const auto& Nodes = MoonfallTalents::Catalog();
  for (int32 I=0;I<Nodes.Num();++I) Index.Add(FName(Nodes[I].Id),I);
  TArray<FLinks> Result;
  Result.Reserve(Nodes.Num());
  for (const auto& Node:Nodes) {
   FLinks Pair;
   if (Node.RequiresA[0]) {
    if (const int32* Found = Index.Find(FName(Node.RequiresA))) Pair.A = *Found;
   }
   if (Node.RequiresB[0]) {
    if (const int32* Found = Index.Find(FName(Node.RequiresB))) Pair.B = *Found;
   }
   Result.Add(Pair);
  }
  return Result;
 }();
 return Links;
}

inline FVector2D ScreenPosition(int32 Index, const FPanel& P,
                                float Zoom, const FVector2D& Pan)
{
 const FVector2D Center(P.MapLeft + P.MapWidth * 0.5f,
                        P.MapTop + P.MapHeight * 0.5f);
 if (!Positions().IsValidIndex(Index)) return Center;
 return Center + (Positions()[Index] + Pan) * FMath::Clamp(
     FMath::IsFinite(Zoom) ? Zoom : 0.2f, 0.08f, 2.1f);
}
inline bool InMap(const FPanel& P, const FVector2D& Position)
{
 return Position.X >= P.MapLeft && Position.X <= P.MapLeft + P.MapWidth
     && Position.Y >= P.MapTop && Position.Y <= P.MapTop + P.MapHeight;
}
inline int32 HitTest(float ViewWidth, float ViewHeight, float Zoom,
                     const FVector2D& Pan, float X, float Y)
{
 const FPanel P = Panel(ViewWidth, ViewHeight);
 const FVector2D Mouse(X, Y);
 if (!P.bUsable || !InMap(P, Mouse)) return INDEX_NONE;
 int32 Best = INDEX_NONE;
 float BestDistSq = FMath::Square(17.f);
 for (int32 Index = 0; Index < Positions().Num(); ++Index) {
  const FVector2D Point = ScreenPosition(Index, P, Zoom, Pan);
  const float Dist = FVector2D::DistSquared(Mouse, Point);
  if (Dist <= BestDistSq) { BestDistSq = Dist; Best = Index; }
 }
 return Best;
}
// Up/down walks only the current talent branch. The Godot catalog is
// interleaved by tier, so stepping the raw array jumped unexpectedly from
// Might to Agility or Arcana on a single D-pad press. A bounded loop runs
// only on actual input, never in Tick or HUD rendering.
inline int32 StepWithinBranch(int32 CurrentIndex, int32 Direction)
{
 const auto& Nodes = MoonfallTalents::Catalog();
 if (!Nodes.IsValidIndex(CurrentIndex) || (Direction != -1 && Direction != 1))
  return INDEX_NONE;
 const int32 Count = Nodes.Num();
 int32 Candidate = CurrentIndex;
 for (int32 Examined = 1; Examined < Count; ++Examined)
 {
  Candidate = (Candidate + (Direction > 0 ? 1 : Count - 1)) % Count;
  if (FCString::Strcmp(Nodes[Candidate].Branch, Nodes[CurrentIndex].Branch) == 0)
   return Candidate;
 }
 return CurrentIndex;
}

// Switch between Might, Agility and Arcana near the same tier.
// Runs only on a left/right input event, not every frame. Invalid indices
// and unexpected branch IDs cannot produce a selection.
inline int32 StepBranch(int32 CurrentIndex, int32 Direction)
{
 const auto& Nodes = MoonfallTalents::Catalog();
 if (!Nodes.IsValidIndex(CurrentIndex) || (Direction != -1 && Direction != 1))
  return INDEX_NONE;
 static const TCHAR* Branches[3] = {TEXT("might"), TEXT("agility"), TEXT("arcana")};
 int32 BranchIndex = INDEX_NONE;
 for (int32 Index = 0; Index < 3; ++Index) {
  if (FCString::Strcmp(Nodes[CurrentIndex].Branch, Branches[Index]) == 0) {
   BranchIndex = Index;
   break;
  }
 }
 if (BranchIndex == INDEX_NONE) return INDEX_NONE;
 const int32 TargetBranch = (BranchIndex + (Direction > 0 ? 1 : 2)) % 3;
 const int32 CurrentTier = Nodes[CurrentIndex].Tier;
 int32 Best = INDEX_NONE;
 int32 BestTierDifference = MAX_int32;
 for (int32 Index = 0; Index < Nodes.Num(); ++Index) {
  if (FCString::Strcmp(Nodes[Index].Branch, Branches[TargetBranch]) != 0) continue;
  const int32 Difference = FMath::Abs(Nodes[Index].Tier - CurrentTier);
  if (Difference < BestTierDifference) {
   Best = Index;
   BestTierDifference = Difference;
  }
 }
 return Best;
}

inline void Focus(int32 Index, FVector2D& Pan)
{
 if (Positions().IsValidIndex(Index)) Pan = -Positions()[Index];
}
}
