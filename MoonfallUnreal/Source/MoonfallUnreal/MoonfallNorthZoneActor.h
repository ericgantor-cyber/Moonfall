#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallNorthZoneActor.generated.h"
class USceneComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;

// Editor-placeable Northern Expanse greybox for one of nine provinces.
// Static non-ticking geometry only. Does not create NPCs, mobs, boss
// spawns, saves, World Partition streaming cells or linked continent travel.
UCLASS()
class MOONFALLUNREAL_API AMoonfallNorthZoneActor : public AActor {
 GENERATED_BODY()
public:
 AMoonfallNorthZoneActor();
 virtual void OnConstruction(const FTransform& Transform) override;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Northern Expanse",
   meta=(ClampMin="0", ClampMax="8", UIMin="0", UIMax="8"))
 int32 ProvinceIndex = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UInstancedStaticMeshComponent> ZoneFloors;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UTextRenderComponent> ZoneSign;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Northern Expanse")
 FVector GetDungeonApproachWorldLocation() const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Northern Expanse")
 FVector GetSouthernEntranceWorldLocation() const;
};
