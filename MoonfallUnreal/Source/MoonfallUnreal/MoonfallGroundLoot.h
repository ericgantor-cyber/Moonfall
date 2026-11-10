#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallGroundLoot.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class MOONFALLUNREAL_API AMoonfallGroundLoot : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallGroundLoot();
 void InitializeLoot(int32 InItemCode);
 int32 GetItemCode() const { return ItemCode; }
 void SetPersistentDropId(const FGuid& Id) { PersistentDropId = Id; }
 const FGuid& GetPersistentDropId() const { return PersistentDropId; }
 bool TryCollect();
private:
 UPROPERTY(VisibleAnywhere) USceneComponent* SceneRoot;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* PickupMesh;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* RarityBeacon;
 UPROPERTY(VisibleAnywhere) UTextRenderComponent* ItemLabel;
 UPROPERTY(VisibleAnywhere) int32 ItemCode = 0;
 FGuid PersistentDropId;
 bool bCollected = false;
};
