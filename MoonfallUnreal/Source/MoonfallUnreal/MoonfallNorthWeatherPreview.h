#pragma once
// Deterministic local weather preview timing. This is NOT precipitation,
// sky/fog rendering, hazardous weather, or an effect on combat visibility.
#include "MoonfallNorthClimate.h"
namespace MoonfallNorth {
inline constexpr int WeatherPreviewSteps = 8;
constexpr int PreviewIntensityPercent(int Province, int Step) {
 if (!ValidZone(Province) || Step<0) return 0;
 constexpr int Wave[WeatherPreviewSteps] = {15,25,40,60,75,60,40,25};
 return Wave[Step % WeatherPreviewSteps];
}
constexpr int PreviewBrightness(int Province, int Step) {
 const FClimate* Climate=ClimateFor(Province);
 if (!Climate || Step<0) return 0;
 // Restrained point-light accent: bounded independent of visibility settings.
 return 200 + (PreviewIntensityPercent(Province,Step)*5);
}
constexpr bool ValidWeatherPreview() {
 for(int Province=0; Province<ZoneCount; ++Province)
  for(int Step=0; Step<WeatherPreviewSteps*2; ++Step) {
   const int Intensity=PreviewIntensityPercent(Province,Step);
   if(Intensity<0 || Intensity>75 || PreviewBrightness(Province,Step)>575)
    return false;
  }
 return PreviewIntensityPercent(-1,0)==0
  && PreviewIntensityPercent(ZoneCount,0)==0;
}
static_assert(ValidWeatherPreview(),"Preview must be bounded for all provinces");
} // namespace MoonfallNorth
