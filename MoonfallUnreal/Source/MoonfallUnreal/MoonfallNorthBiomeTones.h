#pragma once
// Original, bounded visual surface palette for Northern Expanse.
// Matches Moonfall's darker fantasy direction without copying or altering
// existing hero/character art. This is color blockout, NOT final PBR art.
#include "MoonfallNorthZones.h"
namespace MoonfallNorth {
struct FBiomeTone { int Red, Green, Blue; const char* Surface; };
inline constexpr FBiomeTone BiomeTones[ZoneCount]={
 {79,112,134,"Glacial slate"},
 {41,89,70,"Midnight pine"},
 {114,134,148,"Frost limestone"},
 {85,52,68,"Ashen violet"},
 {58,117,79,"Deep moss"},
 {65,91,127,"Storm basalt"},
 {116,80,55,"Corroded bronze"},
 {76,66,140,"Astral indigo"},
 {80,44,107,"Eclipse amethyst"}
};
constexpr bool ValidBiomeTones() {
 for(int I=0;I<ZoneCount;++I) {
  const auto& C=BiomeTones[I];
  if(!C.Surface || !C.Surface[0])return false;
  if(C.Red<30 || C.Red>160 || C.Green<30 || C.Green>160
     || C.Blue<30 || C.Blue>160)return false;
  for(int J=0;J<I;++J)
   if(C.Red==BiomeTones[J].Red && C.Green==BiomeTones[J].Green
      && C.Blue==BiomeTones[J].Blue)return false;
 }
 return true;
}
static_assert(ValidBiomeTones(),"Northern provinces require distinct dark-fantasy surface tones");
} // namespace MoonfallNorth
