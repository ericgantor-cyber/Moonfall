#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MoonfallUnrealGameMode.generated.h"
UCLASS()
class MOONFALLUNREAL_API AMoonfallUnrealGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AMoonfallUnrealGameMode();
 virtual void BeginPlay() override;
 // Default to an open-world test arena. Explicitly opt in for dungeon packs.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Encounter")
 bool bStartInDungeonEncounter = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Encounter")
 FName StartingSpawnBiome = FName(TEXT("Forest"));
};
