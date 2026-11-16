#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallProgressSave.generated.h"

// Isolated experimental save, never shared with production Moonfall.
UCLASS()
class MOONFALLPROTOTYPE_API UMoonfallProgressSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 PermanentBossVictories = 0;

    // Prototype-only collectible list; maximum one per victory, 100 entries.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    TArray<FString> BossRelics;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 HeroLevel = 1;
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 HeroXP = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 HeroPoints = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 TamerPoints = 0;

    // Exactly 18 fixed, single-clear enemies across six connected regions.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 DefeatedEnemyMask = 0;

    // Backward compatible: old saves default to undiscovered; the current region
    // is revealed after the checkpoint position is restored.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 DiscoveredRegionMask = 0;

    // 48 x 16 exploration cells. Older saves load an empty array safely.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    TArray<uint8> DiscoveredMapCells;

    // Manual navigation pin; deliberately separate from checkpoint and quest flags.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    bool bHasWaypoint = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    FVector SavedWaypoint = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 ClaimedChamberMask = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 DefeatedGuardianMask = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    bool bBossDefeated = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    int32 SelectedHeroIndex = 0;

    // Isolated exploration checkpoint; older saves default to no checkpoint.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    bool bHasCheckpoint = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    FVector SavedHeroPosition = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    float SavedHealth = 100.f;

    // Separate version flag keeps older saves' default full resource bars.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    bool bHasResourceCheckpoint = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    float SavedMana = 180.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Prototype")
    float SavedStamina = 150.f;

};
