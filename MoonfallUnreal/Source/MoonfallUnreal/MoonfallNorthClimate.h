#pragma once
// Original Northern Expanse per-province climate definitions.
// The labels/tints can be shown in the optional UE5 greybox. Particle storms,
// sky changes, sound and gameplay weather penalties are NOT implemented.
// Readability policy: no screen occlusion or color-only warning.
#include "MoonfallNorthZones.h"
#include <cstdint>
namespace MoonfallNorth {
enum class EWeather : unsigned char {
 SnowFlurries, PineMist, Whiteout, GraveAsh, WarmRain,
 MountainStorm, RustDust, AstralHaze, EclipseWinds
};
struct FClimate {
 EWeather Type;
 const char* Name;
 std::uint8_t Red, Green, Blue;
 int SuggestedWind;    // 0..100, design-only
 int VisibilityFloor;  // percent of original clear viewing distance, 80..100
 bool HasTextCue;
};
inline constexpr FClimate Climates[ZoneCount] = {
 {EWeather::SnowFlurries, "SNOW FLURRIES", 171,218,244,35,90,true},
 {EWeather::PineMist,    "PINE MIST",    157,210,185,20,90,true},
 {EWeather::Whiteout,    "TUNDRA SNOW",  208,232,251,60,85,true},
 {EWeather::GraveAsh,    "GRAVE ASH",    206,173,192,30,90,true},
 {EWeather::WarmRain,    "WARM RAIN",    140,216,153,35,90,true},
 {EWeather::MountainStorm,"HIGH STORM", 174,201,244,75,85,true},
 {EWeather::RustDust,    "RUST DUST",    215,184,147,45,90,true},
 {EWeather::AstralHaze,  "ASTRAL HAZE",  196,173,248,25,90,true},
 {EWeather::EclipseWinds,"ECLIPSE WIND", 218,168,233,60,85,true}
};
constexpr bool ValidClimates() {
 for (int I=0; I<ZoneCount; ++I) {
  const auto& C=Climates[I];
  if (static_cast<int>(C.Type)!=I || !C.Name || !C.Name[0]
    || C.SuggestedWind<0 || C.SuggestedWind>100
    || C.VisibilityFloor<80 || C.VisibilityFloor>100 || !C.HasTextCue)
   return false;
  // Preserve high-contrast sign tints against dark environments.
  if(C.Red<125 || C.Green<125 || C.Blue<125) return false;
 }
 return true;
}
constexpr const FClimate* ClimateFor(int Province) {
 return ValidZone(Province) ? &Climates[Province] : nullptr;
}
static_assert(ValidClimates(),"Every northern biome needs readable climate cues");
static_assert(ClimateFor(-1)==nullptr && ClimateFor(ZoneCount)==nullptr,
 "Invalid province must not resolve weather");
} // namespace MoonfallNorth
