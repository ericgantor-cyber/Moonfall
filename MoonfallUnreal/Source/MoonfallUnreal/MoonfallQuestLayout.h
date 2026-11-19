#pragma once
// Native-free C++17 geometry for seven tracked Moonfall quests.
// At narrow/short resolutions, draw a safe subset without discarding tracked
// objectives. The full seven are visible from 720p upwards.
namespace MoonfallQuestLayout {
inline constexpr int MaxRows = 7;
struct FMetrics {
 int Top;
 int PanelHeight;
 int RowStride;
 int RowHeight;
 int TitleOffset;
 int ResidentOffset;
 int RewardOffset;
 int ProgressOffset;
 int VisibleRows;
};
constexpr int SafeVisibleRows(int ViewWidth, int ViewHeight, int TrackedRows) {
 if (ViewWidth < 600 || ViewHeight < 420 || TrackedRows <= 0) return 0;
 const int Requested = TrackedRows > MaxRows ? MaxRows : TrackedRows;
 const bool bCompact = ViewHeight < 850;
 const int Top = bCompact ? 18 : 78;
 const int Stride = bCompact ? 60 : 70;
 const int ShortSide = ViewWidth < ViewHeight ? ViewWidth : ViewHeight;
 float Radius = ShortSide * 0.092f;
 Radius = Radius < 50.f ? 50.f : Radius > 80.f ? 80.f : Radius;
 const float OrbTop = ViewHeight - 2.f * Radius - 25.f;
 int Visible = 0;
 for (int I = 1; I <= Requested; ++I) {
  if (static_cast<float>(Top + 85 + Stride * I) + 8.f >= OrbTop) break;
  Visible = I;
 }
 return Visible;
}
// Keep the old two-argument policy entry point usable by previous native-free
// tests. The HUD passes its real width as the third argument for exact fit.
constexpr FMetrics Compute(int ViewHeight, int TrackedRows, int ViewWidth = 1280) {
 const bool bCompact = ViewHeight < 850;
 const int Stride = bCompact ? 60 : 70;
 const int Visible = SafeVisibleRows(ViewWidth, ViewHeight, TrackedRows);
 return {
  bCompact ? 18 : 78,
  Visible > 0 ? 85 + Stride * Visible : 124,
  Stride,
  Stride - 4,
  bCompact ? 4 : 6,
  bCompact ? 22 : 28,
  bCompact ? 40 : 49,
  Stride - 8,
  Visible
 };
}
// Move through a tracked list in fixed, nonoverlapping pages on compact
// windows. These are display offsets only, never persisted quest indices.
constexpr int NormalizePageStart(int Start, int Capacity, int Count) {
 return Count <= 0 || Capacity <= 0 || Capacity >= Count
     || Start < 0 || Start >= Count ? 0 : Start;
}
constexpr int NextPageStart(int Start, int Capacity, int Count) {
 if (Count <= 0 || Capacity <= 0 || Capacity >= Count) return 0;
 const int Current = NormalizePageStart(Start, Capacity, Count);
 return Current + Capacity >= Count ? 0 : Current + Capacity;
}
constexpr bool ValidRowSpacing(FMetrics Layout) {
 return Layout.TitleOffset >= 0
  && Layout.ResidentOffset - Layout.TitleOffset >= 18
  && Layout.RewardOffset - Layout.ResidentOffset >= 18
  && Layout.ProgressOffset - Layout.RewardOffset >= 12
  && Layout.ProgressOffset + 3 <= Layout.RowHeight
  && Layout.RowStride > Layout.RowHeight
  && Layout.VisibleRows >= 0 && Layout.VisibleRows <= MaxRows;
}
// The existing lower-left health orb remains unobstructed; this works for
// actual available rows, not all seven if the viewport is too short.
constexpr bool ClearsHealthOrb(int ViewWidth, int ViewHeight, int TrackedRows) {
 const FMetrics Layout = Compute(ViewHeight, TrackedRows, ViewWidth);
 if (ViewWidth < 600 || ViewHeight < 420) return Layout.VisibleRows == 0;
 const int ShortSide = ViewWidth < ViewHeight ? ViewWidth : ViewHeight;
 float Radius = ShortSide * 0.092f;
 Radius = Radius < 50.f ? 50.f : Radius > 80.f ? 80.f : Radius;
 return static_cast<float>(Layout.Top + Layout.PanelHeight) + 8.f
  < ViewHeight - Radius * 2.f - 25.f;
}
static_assert(MaxRows == 7, "Seven quest slots supported");
static_assert(NextPageStart(0, 3, 7) == 3);
static_assert(NextPageStart(3, 3, 7) == 6);
static_assert(NextPageStart(6, 3, 7) == 0);
static_assert(NextPageStart(0, 4, 7) == 4);
static_assert(NextPageStart(4, 4, 7) == 0);
static_assert(NormalizePageStart(6, 7, 7) == 0);
static_assert(NormalizePageStart(-1, 3, 7) == 0);
static_assert(NextPageStart(6, 3, 5) == 0);
static_assert(SafeVisibleRows(600, 420, 7) == 3, "Minimum window shows three safe rows");
static_assert(SafeVisibleRows(640, 480, 7) == 4, "Small window shows four safe rows");
static_assert(SafeVisibleRows(1280, 720, 7) == 7, "720p shows all seven quests");
static_assert(SafeVisibleRows(1920, 1080, 7) == 7, "1080p shows all seven quests");
static_assert(ValidRowSpacing(Compute(420, 7, 600)));
static_assert(ValidRowSpacing(Compute(1080, 7, 1920)));
static_assert(ClearsHealthOrb(600, 420, 7));
static_assert(ClearsHealthOrb(640, 480, 7));
static_assert(ClearsHealthOrb(1280, 720, 7));
static_assert(ClearsHealthOrb(1920, 1080, 7));
} // namespace MoonfallQuestLayout
