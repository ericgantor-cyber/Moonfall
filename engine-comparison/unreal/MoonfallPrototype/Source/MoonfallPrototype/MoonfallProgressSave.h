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
};
