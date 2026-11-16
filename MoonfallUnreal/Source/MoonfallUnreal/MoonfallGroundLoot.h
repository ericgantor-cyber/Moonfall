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
 // Validated dungeon coordinates bind pickup to the persisted V9 claim.
 void InitializeDungeonReward(int32 Zone, int32 Dungeon, int32 Chamber);
 int32 GetItemCode() const { return ItemCode; }
 bool TryCollect();
private:
 UPROPERTY(VisibleAnywhere) USceneComponent* SceneRoot;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* PickupMesh;
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* RarityBeacon;
 UPROPERTY(VisibleAnywhere) UTextRenderComponent* ItemLabel;
 UPROPERTY(VisibleAnywhere) int32 ItemCode = 0;
 int32 RewardZone = INDEX_NONE;
 int32 RewardDungeon = INDEX_NONE;
 int32 RewardChamber = INDEX_NONE;
 bool bCollected = false;
};
