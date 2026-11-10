#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallRewardSaveGame.generated.h"

// Compact, bounded records for uncollected world loot.
USTRUCT()
struct FMoonfallWorldLootRecord
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid DropId;
 UPROPERTY(SaveGame) int32 ItemCode = 0;
 UPROPERTY(SaveGame) FVector Location = FVector::ZeroVector;
 // v5: level identity prevents a drop from appearing in the wrong map.
 UPROPERTY(SaveGame) FName LevelId = NAME_None;
};

// Separate, versioned reward progress. Does not replace the equipment save.
UCLASS()
class MOONFALLUNREAL_API UMoonfallRewardSaveGame : public USaveGame
{
 GENERATED_BODY()
public:
 // Keep the legacy slot filename for v1/v2 backward compatibility.
 // The hero and level gates must never silently read different slots.
 static const FString& GetRewardSlotName()
 {
  static const FString Slot(TEXT("Moonfall_RewardProgress_v1"));
  return Slot;
 }
 UPROPERTY(SaveGame) int32 SchemaVersion = 5;
 UPROPERTY(SaveGame) int64 Gold = 0;
 UPROPERTY(SaveGame) int64 ConfirmedKills = 0;
 UPROPERTY(SaveGame) TArray<FName> CollectedKeys;
 UPROPERTY(SaveGame) TArray<FName> CompletedFactionQuests;
 // Stable authored gate IDs. v1 saves load with this list empty.
 UPROPERTY(SaveGame) TArray<FName> UnlockedGateIds;
 // v3: equipment actually collected into the bag.
 UPROPERTY(SaveGame) TArray<int32> InventoryItemCodes;
 UPROPERTY(SaveGame) int32 Salvage = 0;
 // v3 also retains the deliberate loadout; inventory entries remain canonical.
 UPROPERTY(SaveGame) int32 EquippedWeaponCode = 0;
 UPROPERTY(SaveGame) int32 EquippedArmorCode = 0;
 UPROPERTY(SaveGame) int32 EquippedAccessoryCode = 0;
 // v4: persisted world drops; v5 adds their source level IDs.
 UPROPERTY(SaveGame) TArray<FMoonfallWorldLootRecord> PendingWorldLoot;
};
