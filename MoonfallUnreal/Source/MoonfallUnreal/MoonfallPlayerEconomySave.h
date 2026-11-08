#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallPlayerEconomySave.generated.h"

// Independent Unreal-only economy slot: never touches the Godot save format.
// Character progression and combat stats are deliberately out of scope.
UCLASS()
class MOONFALLUNREAL_API UMoonfallPlayerEconomySave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 Version = 1;
 UPROPERTY() int32 Salvage = 0;
 UPROPERTY() TArray<int32> InventoryItemCodes;
 UPROPERTY() int32 EquippedWeaponCode = 0;
 UPROPERTY() int32 EquippedArmorCode = 0;
 UPROPERTY() int32 EquippedAccessoryCode = 0;
};