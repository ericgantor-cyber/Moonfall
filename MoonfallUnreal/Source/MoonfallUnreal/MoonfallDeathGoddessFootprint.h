#pragma once
// Unreal-independent, deterministic attack footprints for Morvella Velmora.
// World units are centimeters. The origin and target are locked at windup start.
#include <cmath>

namespace MoonfallDeathGoddessFootprint {
 enum class Shape : int { GraveNeedle = 1, SoulHarvest = 2, CrimsonEclipse = 3, FinalVerdict = 4 };
 inline constexpr float SoulHarvestSafeFraction = 0.40f;
 inline constexpr float EclipseCrossHalfWidthFraction = 0.20f;
 inline constexpr float GraveNeedleBeyondTarget = 150.f;

 inline bool Hit(Shape ShapeId, float OriginX, float OriginY,
                 float TargetX, float TargetY, float HeroX, float HeroY,
                 float Radius) {
  if (!std::isfinite(OriginX) || !std::isfinite(OriginY)
      || !std::isfinite(TargetX) || !std::isfinite(TargetY)
      || !std::isfinite(HeroX) || !std::isfinite(HeroY)
      || !std::isfinite(Radius) || Radius <= 0.f || Radius > 100000.f) return false;
  const float Dx = HeroX - TargetX, Dy = HeroY - TargetY;
  const float DistanceSquared = Dx * Dx + Dy * Dy;
  switch (ShapeId) {
   case Shape::GraveNeedle: {
    const float CastX = TargetX - OriginX, CastY = TargetY - OriginY;
    const float Length = std::sqrt(CastX * CastX + CastY * CastY);
    if (!std::isfinite(Length) || Length < 0.001f) return false;
    const float DirX = CastX / Length, DirY = CastY / Length;
    const float OffsetX = HeroX - OriginX, OffsetY = HeroY - OriginY;
    const float Along = OffsetX * DirX + OffsetY * DirY;
    const float Across = OffsetX * -DirY + OffsetY * DirX;
    return Along >= 0.f && Along <= Length + GraveNeedleBeyondTarget
      && std::fabs(Across) <= Radius;
   }
   case Shape::SoulHarvest: {
    const float Inner = SoulHarvestSafeFraction * Radius;
    return DistanceSquared >= Inner * Inner && DistanceSquared <= Radius * Radius;
   }
   case Shape::CrimsonEclipse:
    return (std::fabs(Dx) <= Radius && std::fabs(Dy) <= Radius * EclipseCrossHalfWidthFraction)
      || (std::fabs(Dy) <= Radius && std::fabs(Dx) <= Radius * EclipseCrossHalfWidthFraction);
   case Shape::FinalVerdict:
    return DistanceSquared <= Radius * Radius;
  }
  return false;
 }
}
