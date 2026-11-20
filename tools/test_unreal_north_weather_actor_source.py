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
assert "WeatherParticles" in header
assert "WeatherParticles->SetCollisionEnabled(ECollisionEnabled::NoCollision)" in source
assert "WeatherParticles->SetGenerateOverlapEvents(false)" in source
assert "WeatherParticles->SetCastShadow(false)" in source
assert "WeatherParticles->ClearInstances()" in source
assert "MoonfallNorth::PrecipitationInstanceCount" in source
assert "MoonfallNorth::PrecipitationAt(" in source
assert "WeatherParticles->AddInstance(" in source
assert "0.2f,true" in source
assert "ParticleStep%10==0" in source
assert "if(!bWeatherActive || !bEnableWeatherPreview ||" in source
assert "  RefreshWeatherPreview();\n } else {\n  RefreshWeatherParticles();" in source
assert "WeatherStep=(WeatherStep+1)%MoonfallNorth::WeatherPreviewSteps;" in source

# Reject unnecessary animation while nobody is nearby.
assert "WeatherActivationRadiusCm=2500.f" in header
assert "FTimerHandle ProximityTimer" in header
assert "bWeatherActive=false" in header
assert "CheckWeatherProximity()" in source
assert "UGameplayStatics::GetPlayerPawn(this,0)" in source
assert "FVector::DistSquared(" in source
assert "FMath::IsFinite(WeatherActivationRadiusCm)" in source
assert "FMath::Clamp(WeatherActivationRadiusCm,800.f,8000.f)" in source
assert '#include "MoonfallNorthWeatherRange.h"' in source
assert "MoonfallNorth::WithinWeatherRange(bWeatherActive," in source
assert "static_cast<float>(FVector::DistSquared(" in source
assert "&AMoonfallNorthWeatherActor::CheckWeatherProximity,1.0f,true" in source
assert "GetWorldTimerManager().ClearTimer(ProximityTimer)" in source
assert "bShouldActivate==bWeatherActive" in source
assert "bWeatherRuntimeInitialized && !bWeatherActive" in source
assert "bWeatherRuntimeInitialized=true;" in source
assert "bWeatherRuntimeInitialized=false;" in source
assert "void AMoonfallNorthWeatherActor::SetWeatherPreviewEnabled(bool bEnabled)" in source
assert "void AMoonfallNorthWeatherActor::SetWeatherProvinceIndex(int32 NewProvinceIndex)" in source
assert "SetWeatherPreviewEnabled(bEnableWeatherPreview);" in source
assert "SetWeatherPreviewEnabled" in header and "SetWeatherProvinceIndex" in header
assert "bool bFollowPlayerProvince=false" in header
assert "TWeakObjectPtr<AMoonfallNorthContinentActor>" in header
assert "bContinentLookupAttempted=false" in header
assert '#include "MoonfallNorthContinentActor.h"' in source
assert "UGameplayStatics::GetActorOfClass(this,AMoonfallNorthContinentActor::StaticClass())" in source
assert "if(!bContinentLookupAttempted)" in source
assert "NorthernContinent.Get()" in source
assert "FVector::DistSquared2D(" in source
assert "FMath::Square(11500.0)" in source
assert "Continent->GetProvinceCenterWorldLocation(Index)" in source
assert "SetActorLocation(Player->GetActorLocation())" in source
assert "ProvinceIndex=ClosestProvince" in source
assert "ClosestProvince!=INDEX_NONE" in source

assert "WeatherParticles->GetInstanceCount()" in source
assert "WeatherParticles->UpdateInstanceTransform(" in source
assert "if(bInitialBuild) WeatherParticles->ClearInstances();" in source
assert "if(bInitialBuild) {" in source
assert "Slot==MoonfallNorth::PrecipitationInstanceCount-1" in source
assert "GetWorldTimerManager().ClearTimer(WeatherTimer);" in source

assert "DeactivateWeatherPreview()" in source


for forbidden in ("SpawnActor", "SaveGameToSlot", "LoadGameFromSlot",
                  "ExponentialHeightFog", "SetFogDensity", "SetVisibilityDistance",
                  "SetGlobalTimeDilation", "Damage", "Niagara", "ParticleSystem"):
    assert forbidden not in source, forbidden
print("Northern climate light preview source safeguards passed")
