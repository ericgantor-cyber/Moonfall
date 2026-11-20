"""Static checks for opt-in northern climate point-light preview.
Not a native UE5 compile, particle effect, fog system or gameplay test.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"
header=(root/"MoonfallNorthWeatherActor.h").read_text(encoding="utf-8")
source=(root/"MoonfallNorthWeatherActor.cpp").read_text(encoding="utf-8")
assert "AMoonfallNorthWeatherActor : public AActor" in header
assert "bEnableWeatherPreview=false" in header
assert "PrimaryActorTick.bCanEverTick=false;" in source
assert "WeatherLight->SetAttenuationRadius(650.f)" in source
assert "WeatherLight->SetCastShadows(false)" in source
assert "WeatherLight->SetVisibility(false)" in source
assert "WeatherText->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in source
assert "MoonfallNorth::ClimateFor(ProvinceIndex)" in source
assert "MoonfallNorth::PreviewBrightness(" in source
assert "GetWorldTimerManager().SetTimer(" in source
assert "GetWorldTimerManager().ClearTimer(" in source
assert "WeatherText->SetTextRenderColor(Color)" in source
for forbidden in ("SpawnActor", "SaveGameToSlot", "LoadGameFromSlot",
                  "ExponentialHeightFog", "SetFogDensity", "SetVisibilityDistance",
                  "SetGlobalTimeDilation", "Damage", "Niagara", "ParticleSystem"):
    assert forbidden not in source, forbidden
print("Northern climate light preview source safeguards passed")
