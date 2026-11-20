#pragma once
#include "CoreMinimal.h"

// Shared, pure Canvas geometry for both drawing and mouse hit testing.
// Keep this independent of world saves and actor/asset state.
namespace MoonfallHUDLayout
{
 constexpr int32 Orb = 0;
 constexpr int32 Quest = 1;
 constexpr int32 Boss = 2;
 constexpr int32 PanelCount = 3;
 constexpr float MinScale = 0.70f;
 constexpr float MaxScale = 1.60f;

 struct FPanelRect
 {
  float Left = 0.f, Top = 0.f, Width = 0.f, Height = 0.f, Scale = 1.f;
  bool Contains(float X, float Y) const
  {
   return FMath::IsFinite(X) && FMath::IsFinite(Y)
    && X >= Left && X <= Left + Width && Y >= Top && Y <= Top + Height;
  }
 };

 inline float SafeScale(float Value)
 {
  return FMath::Clamp(FMath::IsFinite(Value) ? Value : 1.f, MinScale, MaxScale);
 }

 inline FVector2D SafeOffset(FVector2D Offset)
 {
  return FVector2D(
   FMath::Clamp(FMath::IsFinite(Offset.X) ? Offset.X : 0.f, -1.f, 1.f),
   FMath::Clamp(FMath::IsFinite(Offset.Y) ? Offset.Y : 0.f, -1.f, 1.f));
 }

 inline FPanelRect Geometry(int32 Panel, float ViewWidth, float ViewHeight,
                            FVector2D Offset, float RequestedScale)
 {
  const float W = FMath::Max(1.f, FMath::IsFinite(ViewWidth) ? ViewWidth : 1.f);
  const float H = FMath::Max(1.f, FMath::IsFinite(ViewHeight) ? ViewHeight : 1.f);
  const float Scale = SafeScale(RequestedScale);
  float Width = 1.f, Height = 1.f, X = 0.f, Y = 0.f;
  if (Panel == Orb) {
   const float Radius = FMath::Clamp(FMath::Min(W,H) * 0.092f, 50.f, 80.f) * Scale;
   Width = Radius * 2.5f;
   Height = Width;
   X = Radius + 25.f - Width * 0.5f;
   Y = H - Radius - 25.f - Height * 0.5f;
  } else if (Panel == Quest) {
   Width = FMath::Min(560.f * Scale, FMath::Max(1.f,W - 40.f));
   Height = 124.f * Scale;
   X = 20.f; Y = 78.f;
  } else if (Panel == Boss) {
   Width = FMath::Min(480.f * Scale, W * 0.68f) + 12.f * Scale;
   Height = 190.f * Scale;
   X = (W - Width) * 0.5f;
   Y = 24.f;
  }
  Width = FMath::Min(Width, FMath::Max(1.f, W - 8.f));
  Height = FMath::Min(Height, FMath::Max(1.f, H - 8.f));
  Offset = SafeOffset(Offset);
  X = FMath::Clamp(X + Offset.X * W, 4.f, FMath::Max(4.f, W - Width - 4.f));
  Y = FMath::Clamp(Y + Offset.Y * H, 4.f, FMath::Max(4.f, H - Height - 4.f));
  return {X, Y, Width, Height, Scale};
 }
}
