#pragma once
// Pure C++17 layout policy for Moonfall's three tracked quest rows.
// Keeping this independent of Canvas/UE types permits exact geometry tests.
namespace MoonfallQuestLayout {
inline constexpr int MaxRows = 3;
struct FMetrics {
 int Top;
 int PanelHeight;
 int RowStride;
 int RowHeight;
 int TitleOffset;
 int ResidentOffset;
 int RewardOffset;
 int ProgressOffset;
};
constexpr FMetrics Compute(int ViewHeight, int ActiveRows) {
 const bool bCompact = ViewHeight < 520;
 const int Rows = ActiveRows < 0 ? 0 : ActiveRows > MaxRows ? MaxRows : ActiveRows;
 const int Stride = bCompact ? 60 : 70;
 return {
  bCompact ? 18 : 78,
  Rows > 0 ? 85 + Stride * Rows : 124,
  Stride,
  Stride - 4,
  bCompact ? 4 : 6,
  bCompact ? 22 : 28,
  bCompact ? 40 : 49,
  Stride - 8
 };
}
constexpr bool ValidRowSpacing(FMetrics Layout) {
 return Layout.TitleOffset >= 0
  && Layout.ResidentOffset - Layout.TitleOffset >= 18
  && Layout.RewardOffset - Layout.ResidentOffset >= 18
  && Layout.ProgressOffset - Layout.RewardOffset >= 12
  && Layout.ProgressOffset + 3 <= Layout.RowHeight
  && Layout.RowStride > Layout.RowHeight;
}
// The existing health orb is bottom-left. Match its radius/clamp policy.
// Used only for pure policy regression, not to influence runtime drawing.
constexpr bool ClearsHealthOrb(int ViewWidth, int ViewHeight, int ActiveRows) {
 const FMetrics Layout = Compute(ViewHeight, ActiveRows);
 const int Smallest = ViewWidth < ViewHeight ? ViewWidth : ViewHeight;
 float Radius = Smallest * 0.092f;
 Radius = Radius < 50.f ? 50.f : Radius > 80.f ? 80.f : Radius;
 return static_cast<float>(Layout.Top + Layout.PanelHeight) + 8.f
  < ViewHeight - Radius * 2.f - 25.f;
}
static_assert(ValidRowSpacing(Compute(420, 3)), "Compact quest rows must not overlap");
static_assert(ValidRowSpacing(Compute(520, 3)), "Normal quest rows must not overlap");
static_assert(ClearsHealthOrb(600, 420, 3), "Compact three-quest tracker must clear health orb");
static_assert(ClearsHealthOrb(760, 520, 3), "Standard three-quest tracker must clear health orb");
} // namespace MoonfallQuestLayout
