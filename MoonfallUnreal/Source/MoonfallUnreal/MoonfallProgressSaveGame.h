#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallProgressSaveGame.generated.h"

// Versioned, bounded player progression snapshot. No equipment, inventory,
// controller settings or world state: those require a dedicated save contract.
UCLASS()
class MOONFALLUNREAL_API UMoonfallProgressSaveGame : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY(SaveGame) int32 Version = 1;
 UPROPERTY(SaveGame) int32 Level = 1;
 UPROPERTY(SaveGame) int32 Experience = 0;
 UPROPERTY(SaveGame) float MaxHealth = 100.f;
 UPROPERTY(SaveGame) float MaxMana = 100.f;
 UPROPERTY(SaveGame) TArray<int32> RootRanks;
 // 24 one-point flags for nodes 3..26, in increasing node order.
 UPROPERTY(SaveGame) TArray<uint8> CoreUnlocks;
 // 111 one-point flags for Astral nodes 27..137.
 UPROPERTY(SaveGame) TArray<uint8> AstralUnlocks;
 // Version 2: validated inventory/gear snapshot. A version-1 slot still
 // loads progression only; no upgrade code touches that legacy inventory.
 UPROPERTY(SaveGame) TArray<int32> InventoryCodes;
 UPROPERTY(SaveGame) int32 EquippedWeapon = 0;
 UPROPERTY(SaveGame) int32 EquippedArmor = 0;
 UPROPERTY(SaveGame) int32 EquippedAccessory = 0;
 UPROPERTY(SaveGame) int32 SalvageMaterials = 0;
 // Preserve pickup order for existing V2 saves without this field.
 // 0 pickup, 1 quality, 2 slot; every value is a valid bag mode.
 UPROPERTY(SaveGame) int32 InventorySortMode = 0;

};
