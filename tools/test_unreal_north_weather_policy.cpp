// Engine-independent C++17 regression for nine Northern Expanse biomes and weather policies.
// No UE actors, runtime weather effects, damage, character art or save changes.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthClimate.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthBiomeTones.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthWeatherRange.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthWeatherPreview.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallNorthPrecipitation.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>

namespace North = MoonfallNorth;
static_assert(North::ZoneCount == 9);
static_assert(North::ValidClimates());
static_assert(North::ValidBiomeTones());
static_assert(North::ValidWeatherPreview());
static_assert(North::ValidPrecipitation());
static_assert(North::ClimateFor(-1) == nullptr);
static_assert(North::ClimateFor(North::ZoneCount) == nullptr);
static_assert(North::PrecipitationInstanceCount == 18);
static_assert(North::WeatherPreviewSteps == 8);
static_assert(North::PositiveMod(-5, 30) == 25);

int main() {
    std::set<int> colors;
    std::set<int> weathers;
    for (int zone = 0; zone < North::ZoneCount; ++zone) {
        const auto* climate = North::ClimateFor(zone);
        assert(climate != nullptr);
        assert(static_cast<int>(climate->Type) == zone);
        assert(std::strlen(climate->Name) >= 5);
        assert(climate->SuggestedWind >= 0 && climate->SuggestedWind <= 100);
        assert(climate->VisibilityFloor >= 80 && climate->VisibilityFloor <= 100);
        assert(climate->HasTextCue);
        assert(climate->Red >= 125 && climate->Green >= 125 && climate->Blue >= 125);
        assert(weathers.insert(static_cast<int>(climate->Type)).second);

        const auto& tone = North::BiomeTones[zone];
        assert(std::strlen(tone.Surface) >= 5);
        assert(tone.Red >= 30 && tone.Red <= 160);
        assert(tone.Green >= 30 && tone.Green <= 160);
        assert(tone.Blue >= 30 && tone.Blue <= 160);
        assert(colors.insert((tone.Red << 16) | (tone.Green << 8) | tone.Blue).second);

        for (int step = 0; step < North::WeatherPreviewSteps * 4; ++step) {
            const int intensity = North::PreviewIntensityPercent(zone, step);
            assert(intensity >= 15 && intensity <= 75);
            assert(intensity == North::PreviewIntensityPercent(zone, step + North::WeatherPreviewSteps));
            assert(North::PreviewBrightness(zone, step) == 200 + intensity * 5);
            assert(North::PreviewBrightness(zone, step) <= 575);
        }

        for (int slot = 0; slot < North::PrecipitationInstanceCount; ++slot) {
            for (int step = 0; step < North::PrecipitationSteps; ++step) {
                const auto drop = North::PrecipitationAt(zone, slot, step);
                const auto repeat = North::PrecipitationAt(zone, slot, step + North::PrecipitationSteps);
                assert(drop.Xcm >= -400 && drop.Xcm < 400);
                assert(drop.Ycm >= -400 && drop.Ycm < 400);
                assert(drop.Zcm >= 120 && drop.Zcm < 620);
                assert(drop.HalfWidthCm >= 2 && drop.HalfWidthCm <= 4);
                assert(drop.HalfDepthCm >= 2 && drop.HalfDepthCm <= 4);
                assert(drop.HalfHeightCm >= 4 && drop.HalfHeightCm <= 14);
                assert(drop.Xcm == repeat.Xcm && drop.Ycm == repeat.Ycm);
                assert(drop.Zcm == repeat.Zcm);
            }
        }
    }
    assert(colors.size() == 9u && weathers.size() == 9u);

    // Invalid user input cannot start preview weather.
    assert(North::PreviewIntensityPercent(-1, 0) == 0);
    assert(North::PreviewIntensityPercent(9, 0) == 0);
    assert(North::PreviewIntensityPercent(0, -1) == 0);
    assert(North::PreviewBrightness(-1, 0) == 0);
    assert(North::PreviewBrightness(0, -1) == 0);
    assert(North::PrecipitationAt(-1, 0, 0).Zcm == 0);
    assert(North::PrecipitationAt(0, 18, 0).Zcm == 0);
    assert(North::PrecipitationAt(0, 0, -1).Zcm == 0);

    // Weather only appears within bounded distances; 250cm exit hysteresis
    // prevents visual flicker at the activation edge.
    assert(North::WithinWeatherRange(false, 2500.f * 2500.f, 2500.f));
    assert(!North::WithinWeatherRange(false, 2600.f * 2600.f, 2500.f));
    assert(North::WithinWeatherRange(true, 2600.f * 2600.f, 2500.f));
    assert(!North::WithinWeatherRange(true, 2800.f * 2800.f, 2500.f));
    assert(!North::WithinWeatherRange(false, 0.f, 799.f));
    assert(!North::WithinWeatherRange(false, 0.f, 8001.f));
    assert(!North::WithinWeatherRange(true, -1.f, 2500.f));
    std::cout << "MOONFALL NORTHERN CLIMATE POLICY PASS (9 zones, 4860 precipitation frames)" << std::endl;
    return 0;
}
