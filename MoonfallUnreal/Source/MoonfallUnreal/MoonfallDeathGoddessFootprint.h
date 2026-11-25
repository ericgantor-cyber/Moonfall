#pragma once
// Unreal-independent, deterministic attack footprints for Morvella Velmora.
// World units are centimeters. The origin and target are locked at windup start.
#include <cmath>

namespace MoonfallDeathGoddessFootprint {
 enum class Shape : int { GraveNeedle = 1, SoulHarvest = 2, CrimsonEclipse = 3, FinalVerdict = 4 };
 inline constexpr double SoulHarvestSafeFraction = 0.40;
 inline constexpr double EclipseCrossHalfWidthFraction = 0.20;
 inline constexpr float GraveNeedleBeyondTarget = 150.f;
 inline constexpr double MaxWorldCoordinate = 1.e12;
 inline bool ValidCoordinate(double V) { return std::isfinite(V) && std::fabs(V) <= MaxWorldCoordinate; }

 inline bool Hit(Shape ShapeId, double OriginX, double OriginY,
                 double TargetX, double TargetY, double HeroX, double HeroY,
                 double Radius) {
  if (!ValidCoordinate(OriginX) || !ValidCoordinate(OriginY)
      || !ValidCoordinate(TargetX) || !ValidCoordinate(TargetY)
      || !ValidCoordinate(HeroX) || !ValidCoordinate(HeroY)
      || !std::isfinite(Radius) || Radius <= 0.0 || Radius > 100000.0) return false;
  const double Dx = HeroX - TargetX, Dy = HeroY - TargetY;
  const double DistanceSquared = Dx * Dx + Dy * Dy;
  switch (ShapeId) {
   case Shape::GraveNeedle: {
    const double CastX = TargetX - OriginX, CastY = TargetY - OriginY;
    const double Length = std::sqrt(CastX * CastX + CastY * CastY);
    if (!std::isfinite(Length) || Length < 0.001f) return false;
    const double DirX = CastX / Length, DirY = CastY / Length;
    const double OffsetX = HeroX - OriginX, OffsetY = HeroY - OriginY;
    const double Along = OffsetX * DirX + OffsetY * DirY;
    const double Across = OffsetX * -DirY + OffsetY * DirX;
    return Along >= 0.f && Along <= Length + GraveNeedleBeyondTarget
      && std::fabs(Across) <= Radius;
   }
   case Shape::SoulHarvest: {
    const double Inner = SoulHarvestSafeFraction * Radius;
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
