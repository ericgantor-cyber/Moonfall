#pragma once
// Engine-free, deterministic 2D guardian strike geometry. Units are Unreal cm.
// Four authored footprints reuse the existing enemy attack and defense pipeline.
#include <cmath>
namespace MoonfallGuardianFootprint {
 enum class Shape : int { None=-1, MoonboundRing=0, EmberCharge=1, VenomPool=2, EclipseCross=3 };
 inline const char* Name(Shape s) {
  switch(s) {
   case Shape::MoonboundRing: return "MOONBOUND RING";
   case Shape::EmberCharge: return "EMBER CHARGE";
   case Shape::VenomPool: return "VENOM POOL";
   case Shape::EclipseCross: return "ECLIPSE CROSS";
   default: return "STRIKE";
  }
 }
 inline bool Hit(Shape shape,float originX,float originY,float targetX,float targetY,
                 float facingX,float facingY,float heroX,float heroY,float range) {
  if (!std::isfinite(originX)||!std::isfinite(originY)||!std::isfinite(targetX)||
      !std::isfinite(targetY)||!std::isfinite(facingX)||!std::isfinite(facingY)||
      !std::isfinite(heroX)||!std::isfinite(heroY)||!std::isfinite(range)||
      range<=0.f||range>100000.f) return false;
  const float dx=heroX-originX, dy=heroY-originY;
  const float dist2=dx*dx+dy*dy;
  switch(shape) {
   case Shape::MoonboundRing:
    return dist2 >= (0.34f*range)*(0.34f*range) && dist2 <= range*range;
   case Shape::EmberCharge: {
    const float norm=std::sqrt(facingX*facingX+facingY*facingY);
    if (norm<0.0001f || !std::isfinite(norm)) return false;
    const float fx=facingX/norm,fy=facingY/norm;
    const float forward=dx*fx+dy*fy,side=dx*(-fy)+dy*fx;
    return forward>=0.f && forward<=range*1.25f && std::fabs(side)<=range*0.22f;
   }
   case Shape::VenomPool: {
    const float tx=heroX-targetX,ty=heroY-targetY;
    return tx*tx+ty*ty <= (0.46f*range)*(0.46f*range);
   }
   case Shape::EclipseCross: {
    const float tx=heroX-targetX,ty=heroY-targetY;
    return (std::fabs(tx)<=range && std::fabs(ty)<=range*0.16f) ||
           (std::fabs(ty)<=range && std::fabs(tx)<=range*0.16f);
   }
   default: return false;
  }
 }
}
