#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallNorthWeatherActor.generated.h"
class USceneComponent;
class UPointLightComponent;
class UTextRenderComponent;

// Optional placeable weather ambience demonstrator; no particles, fog,
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
private:
 FTimerHandle WeatherTimer;
 int32 WeatherStep=0;
 void RefreshWeatherPreview();
 void AdvanceWeatherPreview();
};
