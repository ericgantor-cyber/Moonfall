#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "MoonfallNorthWeatherActor.generated.h"
class USceneComponent;
class UPointLightComponent;
class UTextRenderComponent;
class UInstancedStaticMeshComponent;

// Optional placeable weather ambience demonstrator; small cube particles, no fog,
// damage, global postprocessing, save writes, god instances or player input.
UCLASS()
class MOONFALLUNREAL_API AMoonfallNorthWeatherActor : public AActor {
 GENERATED_BODY()
public:
 AMoonfallNorthWeatherActor();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Northern Expanse",
   meta=(ClampMin="0",ClampMax="8",UIMin="0",UIMax="8"))
 int32 ProvinceIndex=0;

 // Off by default to preserve existing UE5 maps and player experience.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Northern Expanse")
 bool bEnableWeatherPreview=false;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UPointLightComponent> WeatherLight;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UTextRenderComponent> WeatherText;
 // Maximum 18 noncolliding cube flakes/streaks, updated on one shared timer.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UInstancedStaticMeshComponent> WeatherParticles;
private:
 FTimerHandle WeatherTimer;
 int32 WeatherStep=0;
 int32 ParticleStep=0;
 void RefreshWeatherParticles();
 void RefreshWeatherPreview();
 void AdvanceWeatherPreview();
};
