#pragma once
#include <cmath>
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
// Shared cooldown calculation for live casts and spellbook previews.
// Negative last-cast sentinels are allowed; future timestamps are not.
inline bool TryCooldownRemaining(float base, float now, float lastCast, float haste, float& remaining) {
 remaining=0.f;
 if (!std::isfinite(base) || base<=0.f || !std::isfinite(now) || now<0.f
     || !std::isfinite(lastCast) || lastCast>now || !std::isfinite(haste)
     || haste<0.f || haste>0.20f) return false;
 const float elapsed=now-lastCast;
 if (!std::isfinite(elapsed)) return false;
 const float wait=base/(1.f+haste)-elapsed;
 if (!std::isfinite(wait)) return false;
 remaining=wait>0.f ? wait : 0.f;
 return true;
}
// This is a read-only cast preflight, not a promise that a hostile target exists.
// TryCast remains the only authority on target LOS, damage, and resource spend.
enum class EReadiness { Ready, Cooldown, LowMana, FullHealth, Invalid };
struct FCastState {
 EReadiness State;
 float CooldownRemaining;
 float ManaShortfall;
};
inline FCastState Evaluate(int index, float mana, float maxMana,
 float health, float maxHealth, float power, float now, float lastCast, float haste)
{
 if (!IsValid(index) || !std::isfinite(mana) || !std::isfinite(maxMana)
  || !std::isfinite(health) || !std::isfinite(maxHealth) || !std::isfinite(power)
  || !std::isfinite(now) || !std::isfinite(lastCast) || !std::isfinite(haste)
  || mana < 0.f || maxMana <= 0.f || health <= 0.f || maxHealth <= 0.f
  || health > maxHealth || power <= 0.f)
  return {EReadiness::Invalid,0.f,0.f};
 float wait=0.f;
 if (!TryCooldownRemaining(Spells[index].Cooldown,now,lastCast,haste,wait))
  return {EReadiness::Invalid,0.f,0.f};
 const float shortfall = mana < Spells[index].ManaCost
    ? Spells[index].ManaCost - mana : 0.f;
 if (Spells[index].bHealing && health >= maxHealth)
  return {EReadiness::FullHealth,wait,shortfall};
 if (shortfall > 0.f) return {EReadiness::LowMana,wait,shortfall};
 if (wait > 0.f) return {EReadiness::Cooldown,wait,0.f};
 return {EReadiness::Ready,0.f,0.f};
}
inline float Min(float a, float b) { return a < b ? a : b; }
inline float Max(float a, float b) { return a > b ? a : b; }
inline float Clamp(float a, float lo, float hi) { return Min(hi, Max(lo, a)); }
struct FLayout {
 float Left, Top, Width, Height;
 float TabX, TabY, TabWidth, TabHeight;
 float RowX, RowY, RowWidth, RowGap, RowHeight;
 float EquipLeftX, EquipRightX, EquipY, EquipWidth, EquipHeight;
 float DetailsY;
 float CloseX, CloseY, CloseSize;
};
inline FLayout Layout(float screenWidth, float screenHeight, bool bBookOpen = false) {
 // Inventory remains right-docked. Opening the grimoire creates a wider
 // centered reading surface without altering any keyboard movement bindings.
 const float w = bBookOpen
  ? Min(1080.f,Max(280.f,screenWidth-24.f))
  : Min(720.f,Max(280.f,Min(screenWidth-24.f,screenWidth*0.48f)));
 const float h = Min(860.f,Max(280.f,screenHeight-24.f));
 const float l = bBookOpen ? (screenWidth-w)*0.5f : screenWidth-w-12.f;
 const float t = (screenHeight-h)*0.5f;
 // Reserve space for hand readiness before expanding decorative row height.
 const float gap = bBookOpen ? Clamp((h-280.f)/6.f,15.f,67.f)
  : Clamp((h-198.f)/6.f,15.f,48.f);
 const float equipW = Min(195.f,(w-48.f)*0.5f);
 const float rowY = t+123.f;
 return {l,t,w,h,
  l+18.f,t+80.f,140.f,28.f,
  l+18.f,rowY,w-36.f,gap,Max(12.f,gap-3.f),
  l+18.f,l+w-18.f-equipW,t+h-47.f,equipW,30.f,
  rowY+6.f*gap+9.f,l+w-42.f,t+14.f,28.f};
}
// Full details use two lines of description and two lines of hand status.
inline bool HasFullDetails(const FLayout& g) { return g.DetailsY+90.f <= g.EquipY; }
// Compact windows prioritize both hand statuses over description text.
inline bool HasCompactReadiness(const FLayout& g) { return g.DetailsY+36.f <= g.EquipY; }
inline float ReadinessY(const FLayout& g) {
 return g.DetailsY+(HasFullDetails(g) ? 49.f : 0.f);
}
inline float ReadinessLineGap(const FLayout& g) { return HasFullDetails(g) ? 23.f : 17.f; }
// Hit actions: 0..5 choose spell, 6 equip left, 7 equip right, 8 tab,
// 9 closes the open book; -1 outside. The same geometry drives mouse hit testing and HUD rendering.
inline constexpr int EquipLeft = 6;
inline constexpr int EquipRight = 7;
inline constexpr int Tab = 8;
inline constexpr int Close = 9;
inline constexpr int None = -1;
inline bool Inside(float x, float y, float left, float top, float width, float height) {
 return x >= left && x < left+width && y >= top && y < top+height;
}
inline int Hit(const FLayout& g, float x, float y, bool bBookOpen) {
 if (Inside(x,y,g.TabX,g.TabY,g.TabWidth,g.TabHeight)) return Tab;
 if (!bBookOpen) return None;
 if (Inside(x,y,g.CloseX,g.CloseY,g.CloseSize,g.CloseSize)) return Close;
 for (int i=0;i<SpellCount;++i)
  if (Inside(x,y,g.RowX,g.RowY+i*g.RowGap,g.RowWidth,g.RowHeight)) return i;
 if (Inside(x,y,g.EquipLeftX,g.EquipY,g.EquipWidth,g.EquipHeight)) return EquipLeft;
 if (Inside(x,y,g.EquipRightX,g.EquipY,g.EquipWidth,g.EquipHeight)) return EquipRight;
 return None;
}
}
