#pragma once
// Portable weather proximity hysteresis shared by native tests and UE5.
// The caller bounds the activation radius and handles invalid player data.
namespace MoonfallNorth {
inline constexpr float WeatherExitBufferCm=250.f;
constexpr bool WithinWeatherRange(bool PreviouslyActive, float SquaredDistance,
                                  float RadiusCm) {
 const float Limit=RadiusCm+(PreviouslyActive?WeatherExitBufferCm:0.f);
 return RadiusCm>=800.f && RadiusCm<=8000.f &&
   SquaredDistance>=0.f && SquaredDistance<=Limit*Limit;
}
static_assert(WithinWeatherRange(false,2500.f*2500.f,2500.f),
 "Weather must activate at the range boundary");
static_assert(!WithinWeatherRange(false,2600.f*2600.f,2500.f),
 "Inactive weather must not activate past its boundary");
static_assert(WithinWeatherRange(true,2600.f*2600.f,2500.f),
 "Active weather must not flicker near the boundary");
static_assert(!WithinWeatherRange(true,2800.f*2800.f,2500.f),
 "Weather must eventually deactivate beyond its exit buffer");
} // namespace MoonfallNorth
