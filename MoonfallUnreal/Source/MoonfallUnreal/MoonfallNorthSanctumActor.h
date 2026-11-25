#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallNorthSanctumActor.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;

// Editor-placeable prototype: turns the three portable sanctum plans into
// actual UE5 static geometry. Does NOT spawn a god, alter saves or world maps.
UCLASS()
class MOONFALLUNREAL_API AMoonfallNorthSanctumActor : public AActor {
 GENERATED_BODY()
public:
 AMoonfallNorthSanctumActor();
 virtual void OnConstruction(const FTransform& Transform) override;

 // 0=Morvella, 1=Z, 2=Nyra. Out of range builds nothing.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Northern Expanse",
   meta=(ClampMin="0", ClampMax="2", UIMin="0", UIMax="2"))
 int32 DivineSiteIndex = 0;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Northern Expanse")
 TObjectPtr<UInstancedStaticMeshComponent> SanctumBlocks;

 // Position-only marker. Encounter spawning is deliberately a separate task.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Northern Expanse")
 FVector GetBossMarkerWorldLocation() const;

 UFUNCTION(BlueprintCallable, Category="Moonfall|Northern Expanse")
 FVector GetEntranceWorldLocation() const;
};
