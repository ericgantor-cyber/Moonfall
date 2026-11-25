#pragma once
// Portable C++17 policy for the Gothic Raven's safe dry-land arrivals.
// Ground MUST block Visibility and Pawn; an editor marker or visual-only
// surface is not a collision-proof landing. Used by UE5 and CI unit tests.
namespace MoonfallNorth {
inline constexpr float DryLandingMinNormalZ = 0.7f;
inline constexpr float DryLandingMaxAnchorDistanceCm = 400.f;
inline constexpr float DryLandingCapsuleClearanceCm = 3.f;
constexpr bool ValidDryLandingSample(bool bGroundHit, bool bPawnBlocks,
                                    float NormalZ, float MarkerZ,
                                    float GroundZ, float CapsuleHalfHeight) {
 if (!bGroundHit || !bPawnBlocks) return false;
 // Relational guards also reject NaN and +/-infinity in ordinary C++17.
 if (!(NormalZ >= DryLandingMinNormalZ && NormalZ <= 1.f)
     || !(CapsuleHalfHeight > 0.f && CapsuleHalfHeight < 500.f))
  return false;
 const float CapsuleCenterZ = GroundZ + CapsuleHalfHeight
                              + DryLandingCapsuleClearanceCm;
 const float Difference = CapsuleCenterZ - MarkerZ;
 return Difference >= -DryLandingMaxAnchorDistanceCm
     && Difference <= DryLandingMaxAnchorDistanceCm;
}
static_assert(ValidDryLandingSample(true,true,1.f,130.f,0.f,88.f),
              "Walkable dock accepts the full capsule");
static_assert(ValidDryLandingSample(true,true,0.7f,130.f,0.f,88.f),
              "Borderline slope is permitted");
static_assert(!ValidDryLandingSample(true,false,1.f,130.f,0.f,88.f),
              "Visibility-only geometry is not a landing");
static_assert(!ValidDryLandingSample(true,true,0.69f,130.f,0.f,88.f),
              "Steep slopes cannot receive a passenger");
static_assert(!ValidDryLandingSample(false,true,1.f,130.f,0.f,88.f),
              "A missing dock is not safe");
} // namespace MoonfallNorth
