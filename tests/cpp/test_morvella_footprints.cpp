// Build on Linux: g++ -std=c++17 -Wall -Wextra -Werror -I. \
 // tests/cpp/test_morvella_footprints.cpp -o /tmp/test_morvella_footprints
#include "MoonfallUnreal/Source/MoonfallUnreal/MoonfallDeathGoddessFootprint.h"
#include <cassert>
#include <limits>
using namespace MoonfallDeathGoddessFootprint;
int main() {
 const float Nan = std::numeric_limits<float>::quiet_NaN();
 const auto HitAt = [](Shape ShapeId, float X, float Y) {
  return Hit(ShapeId, 0.f, 0.f, 300.f, 0.f, X, Y, 100.f);
 };
 // Needle starts at locked caster origin, extends 150cm beyond locked mark.
 assert(HitAt(Shape::GraveNeedle, 300.f, 50.f));
 assert(HitAt(Shape::GraveNeedle, 450.f, 0.f));
 assert(!HitAt(Shape::GraveNeedle, 300.f, 101.f));
 assert(!HitAt(Shape::GraveNeedle, 451.f, 0.f));
 assert(!HitAt(Shape::GraveNeedle, -10.f, 0.f));
 // Soul Harvest leaves a 40cm safe center and a safe outside.
 assert(!HitAt(Shape::SoulHarvest, 300.f, 0.f));
 assert(HitAt(Shape::SoulHarvest, 350.f, 0.f));
 assert(!HitAt(Shape::SoulHarvest, 401.f, 0.f));
 // Crimson Eclipse is two intersecting lanes, with safe diagonal corners.
 assert(HitAt(Shape::CrimsonEclipse, 390.f, 10.f));
 assert(HitAt(Shape::CrimsonEclipse, 310.f, 90.f));
 assert(!HitAt(Shape::CrimsonEclipse, 360.f, 360.f));
 // Final Verdict remains a clearly marked circle.
 assert(HitAt(Shape::FinalVerdict, 300.f, 0.f));
 assert(!HitAt(Shape::FinalVerdict, 401.f, 0.f));
 for (const auto ShapeId: {Shape::GraveNeedle, Shape::SoulHarvest,
                            Shape::CrimsonEclipse, Shape::FinalVerdict}) {
  assert(!Hit(ShapeId,0.f,0.f,300.f,0.f,Nan,0.f,100.f));
  assert(!Hit(ShapeId,0.f,0.f,300.f,0.f,300.f,0.f,Nan));
  assert(!Hit(ShapeId,0.f,0.f,300.f,0.f,300.f,0.f,-1.f));
 }
 assert(!Hit(Shape::GraveNeedle,0.f,0.f,0.f,0.f,0.f,0.f,100.f));
 return 0;
}
