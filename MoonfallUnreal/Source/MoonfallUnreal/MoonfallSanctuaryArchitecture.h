#pragma once
// Engine-independent, authored, bounded stonework layout for the Shattered
// Sanctuary. Every piece is visual-only: no physics, overlap or visibility hit.
#include <cstddef>

namespace MoonfallSanctuaryArchitecture {
struct FPiece {
 float X, Y, Z;
 float ScaleX, ScaleY, ScaleZ;
 float YawDegrees;
};
inline constexpr FPiece GothicStone[] = {
 {-760.f, 2670.f, 245.f, 0.82f, 0.85f, 4.9f, 0.f},
 {-760.f, 2670.f, 555.f, 1.1f, 1.1f, 1.1f, 0.f},
 {-1120.f, 2670.f, 175.f, 0.65f, 1.35f, 3.5f, 0.f},
 {760.f, 2670.f, 245.f, 0.82f, 0.85f, 4.9f, 0.f},
 {760.f, 2670.f, 555.f, 1.1f, 1.1f, 1.1f, 0.f},
 {1120.f, 2670.f, 175.f, 0.65f, 1.35f, 3.5f, 0.f},
 {0.f, 2670.f, 510.f, 16.f, 0.68f, 0.45f, 0.f},
 {0.f, 2670.f, 559.f, 15.8f, 0.24f, 0.2f, 0.f},
 {-760.f, 3170.f, 245.f, 0.82f, 0.85f, 4.9f, 0.f},
 {-760.f, 3170.f, 555.f, 1.1f, 1.1f, 1.1f, 0.f},
 {-1120.f, 3170.f, 175.f, 0.65f, 1.35f, 3.5f, 0.f},
 {760.f, 3170.f, 245.f, 0.82f, 0.85f, 4.9f, 0.f},
 {760.f, 3170.f, 555.f, 1.1f, 1.1f, 1.1f, 0.f},
 {1120.f, 3170.f, 175.f, 0.65f, 1.35f, 3.5f, 0.f},
 {0.f, 3170.f, 510.f, 16.f, 0.68f, 0.45f, 0.f},
 {0.f, 3170.f, 559.f, 15.8f, 0.24f, 0.2f, 0.f},
 {-760.f, 3870.f, 245.f, 0.82f, 0.85f, 4.9f, 0.f},
 {-760.f, 3870.f, 555.f, 1.1f, 1.1f, 1.1f, 0.f},
 {-1120.f, 3870.f, 175.f, 0.65f, 1.35f, 3.5f, 0.f},
 {760.f, 3870.f, 245.f, 0.82f, 0.85f, 4.9f, 0.f},
 {760.f, 3870.f, 555.f, 1.1f, 1.1f, 1.1f, 0.f},
 {1120.f, 3870.f, 175.f, 0.65f, 1.35f, 3.5f, 0.f},
 {0.f, 3870.f, 510.f, 16.f, 0.68f, 0.45f, 0.f},
 {0.f, 3870.f, 559.f, 15.8f, 0.24f, 0.2f, 0.f},
 {-760.f, 4370.f, 245.f, 0.82f, 0.85f, 4.9f, 0.f},
 {-760.f, 4370.f, 555.f, 1.1f, 1.1f, 1.1f, 0.f},
 {-1120.f, 4370.f, 175.f, 0.65f, 1.35f, 3.5f, 0.f},
 {760.f, 4370.f, 245.f, 0.82f, 0.85f, 4.9f, 0.f},
 {760.f, 4370.f, 555.f, 1.1f, 1.1f, 1.1f, 0.f},
 {1120.f, 4370.f, 175.f, 0.65f, 1.35f, 3.5f, 0.f},
 {0.f, 4370.f, 510.f, 16.f, 0.68f, 0.45f, 0.f},
 {0.f, 4370.f, 559.f, 15.8f, 0.24f, 0.2f, 0.f},
 {-1030.f, 3500.f, 80.f, 0.52f, 10.5f, 1.6f, 0.f},
 {1030.f, 3500.f, 80.f, 0.52f, 10.5f, 1.6f, 0.f},
 {-450.f, 4550.f, 365.f, 0.65f, 0.7f, 7.3f, 0.f},
 {450.f, 4550.f, 365.f, 0.65f, 0.7f, 7.3f, 0.f}
};
inline constexpr FPiece MoonstoneRelics[] = {
 {-430.f, 3450.f, 147.f, 0.45f, 0.45f, 2.94f, 0.f},
 {-430.f, 3450.f, 323.f, 0.25f, 0.25f, 0.88f, 25.f},
 {430.f, 3450.f, 147.f, 0.45f, 0.45f, 2.94f, 0.f},
 {430.f, 3450.f, 323.f, 0.25f, 0.25f, 0.88f, 25.f},
 {-430.f, 3830.f, 147.f, 0.45f, 0.45f, 2.94f, 0.f},
 {-430.f, 3830.f, 323.f, 0.25f, 0.25f, 0.88f, 25.f},
 {430.f, 3830.f, 147.f, 0.45f, 0.45f, 2.94f, 0.f},
 {430.f, 3830.f, 323.f, 0.25f, 0.25f, 0.88f, 25.f},
 {-1060.f, 2840.f, 29.f, 2.1f, 0.75f, 0.58f, 0.f},
 {1060.f, 2840.f, 29.f, 2.1f, 0.75f, 0.58f, 0.f},
 {-1060.f, 3440.f, 29.f, 2.1f, 0.75f, 0.58f, 0.f},
 {1060.f, 3440.f, 29.f, 2.1f, 0.75f, 0.58f, 0.f},
 {-1060.f, 4080.f, 29.f, 2.1f, 0.75f, 0.58f, 0.f},
 {1060.f, 4080.f, 29.f, 2.1f, 0.75f, 0.58f, 0.f},
 {-1160.f, 2750.f, 35.f, 1.4f, 1.f, 0.7f, -20.f},
 {1160.f, 2750.f, 35.f, 1.4f, 1.f, 0.7f, 20.f},
 {-1160.f, 4270.f, 35.f, 1.4f, 1.f, 0.7f, -20.f},
 {1160.f, 4270.f, 35.f, 1.4f, 1.f, 0.7f, 20.f},
 {-650.f, 4510.f, 530.f, 1.f, 0.8f, 1.2f, -12.f},
 {650.f, 4510.f, 530.f, 1.f, 0.8f, 1.2f, 12.f}
};
inline constexpr std::size_t GothicCount = sizeof(GothicStone) / sizeof(GothicStone[0]);
inline constexpr std::size_t RelicCount = sizeof(MoonstoneRelics) / sizeof(MoonstoneRelics[0]);
inline constexpr std::size_t MaxTotalPieces = 80;

// Keeps a wide uninterrupted central walking corridor to the Crossroads.
// Cross-lane arches are allowed only overhead, above the character's head.
constexpr bool ClearsMainPath(const FPiece& Piece) {
 const float Edge = 50.f * Piece.ScaleX;
 const float Bottom = Piece.Z - 50.f * Piece.ScaleZ;
 return Piece.X - Edge > 340.f || Piece.X + Edge < -340.f
   || Bottom > 335.f;
}
template <std::size_t N>
constexpr bool AllClear(const FPiece (&Pieces)[N]) {
 for (std::size_t i=0; i<N; ++i) {
  const FPiece& P = Pieces[i];
  if (P.ScaleX <= 0.f || P.ScaleY <= 0.f || P.ScaleZ <= 0.f
      || !ClearsMainPath(P)) return false;
 }
 return true;
}
static_assert(GothicCount + RelicCount <= MaxTotalPieces,
 "Sanctuary visual geometry must remain bounded and instanced.");
static_assert(AllClear(GothicStone) && AllClear(MoonstoneRelics),
 "Sanctuary decorations must not obscure the central new-player corridor.");
} // namespace MoonfallSanctuaryArchitecture
