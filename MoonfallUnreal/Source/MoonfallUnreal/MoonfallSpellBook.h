#pragma once
// UE5-independent spell metadata and Canvas layout. No invented spell effects:
// all six entries correspond to existing TryCast schools and their exact costs.
namespace MoonfallSpellBook {
struct FSpell {
 const char* Name;
 const char* School;
 const char* Description;
 float ManaCost;
 float Cooldown;
 bool bHealing;
};
inline constexpr FSpell Spells[6] = {
 {"Ember Bolt", "Flame", "Focused fire strike; light posture pressure.", 16.f, 0.8f, false},
 {"Frost Shard", "Frost", "Precise frost strike; modest posture pressure.", 13.f, 0.65f, false},
 {"Eclipse Lance", "Arcane", "Heavy arcane strike; strong posture pressure.", 22.f, 1.2f, false},
 {"Mending Light", "Restoration", "Restores 24 health times spell power. No cast at full health.", 25.f, 2.f, true},
 {"Storm Spark", "Storm", "Powerful ranged strike; strong posture pressure.", 18.f, 1.f, false},
 {"Thorn Dart", "Nature", "Quick nature strike; moderate posture pressure.", 14.f, 0.9f, false}
};
inline constexpr int SpellCount = 6;
static_assert(sizeof(Spells) / sizeof(Spells[0]) == SpellCount, "Spell book schools must match TryCast");
inline constexpr bool IsValid(int index) { return index >= 0 && index < SpellCount; }
inline float Min(float a, float b) { return a < b ? a : b; }
inline float Max(float a, float b) { return a > b ? a : b; }
inline float Clamp(float a, float lo, float hi) { return Min(hi, Max(lo, a)); }
struct FLayout {
 float Left, Top, Width, Height;
 float TabX, TabY, TabWidth, TabHeight;
 float RowX, RowY, RowWidth, RowGap, RowHeight;
 float EquipLeftX, EquipRightX, EquipY, EquipWidth, EquipHeight;
 float DetailsY;
};
inline FLayout Layout(float screenWidth, float screenHeight) {
 const float w = Min(720.f, Max(280.f, Min(screenWidth - 24.f, screenWidth * 0.48f)));
 const float h = Min(860.f, Max(280.f, screenHeight - 24.f));
 const float l = screenWidth - w - 12.f;
 const float t = (screenHeight - h) * 0.5f;
 const float gap = Clamp((h - 198.f) / 6.f, 15.f, 48.f);
 const float equipW = Min(175.f, (w - 48.f) * 0.5f);
 return {l,t,w,h,
  l+18.f,t+80.f,130.f,28.f,
  l+18.f,t+123.f,w-36.f,gap,Max(12.f,gap-3.f),
  l+18.f,l+w-18.f-equipW,t+h-47.f,equipW,30.f,
  t+123.f+6.f*gap+9.f};
}
// Hit actions: 0..5 choose spell, 6 equip left, 7 equip right, 8 tab,
// -1 outside. The same geometry drives mouse hit testing and HUD rendering.
inline constexpr int EquipLeft = 6;
inline constexpr int EquipRight = 7;
inline constexpr int Tab = 8;
inline constexpr int None = -1;
inline bool Inside(float x, float y, float left, float top, float width, float height) {
 return x >= left && x < left+width && y >= top && y < top+height;
}
inline int Hit(const FLayout& g, float x, float y, bool bBookOpen) {
 if (Inside(x,y,g.TabX,g.TabY,g.TabWidth,g.TabHeight)) return Tab;
 if (!bBookOpen) return None;
 for (int i=0;i<SpellCount;++i)
  if (Inside(x,y,g.RowX,g.RowY+i*g.RowGap,g.RowWidth,g.RowHeight)) return i;
 if (Inside(x,y,g.EquipLeftX,g.EquipY,g.EquipWidth,g.EquipHeight)) return EquipLeft;
 if (Inside(x,y,g.EquipRightX,g.EquipY,g.EquipWidth,g.EquipHeight)) return EquipRight;
 return None;
}
}
