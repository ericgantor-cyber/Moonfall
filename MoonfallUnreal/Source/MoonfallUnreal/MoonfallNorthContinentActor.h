#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallNorthContinentActor.generated.h"
class USceneComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;

// Optional editor-placeable one-piece connected Northern Expanse greybox.
// No combat actors, travel teleporters, level streaming or save changes.
// Place in a separate authored UE5 test map, away from the legacy arena.
UCLASS()
class MOONFALLUNREAL_API AMoonfallNorthContinentActor : public AActor {
 GENERATED_BODY()
public:
 AMoonfallNorthContinentActor();
 virtual void OnConstruction(const FTransform& Transform) override;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UInstancedStaticMeshComponent> ProvincePads;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UInstancedStaticMeshComponent> ProvinceRoads;
 // Decorative only. Distinct province landmarks must not block roads/navmesh.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UInstancedStaticMeshComponent> ProvinceLandmarks;
 // One nonblocking region/hub sign per province, generated in editor.
 // Names identify built locations, never hidden quest or divine lore.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TArray<TObjectPtr<UTextRenderComponent>> ProvinceSigns;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Northern Expanse")
 FVector GetProvinceCenterWorldLocation(int32 ProvinceIndex) const;
};
