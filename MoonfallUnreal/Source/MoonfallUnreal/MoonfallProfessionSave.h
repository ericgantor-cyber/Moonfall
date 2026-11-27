#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallProfessionSave.generated.h"
UCLASS()
class MOONFALLUNREAL_API UMoonfallProfessionSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 Version=2;
 UPROPERTY() int64 Sequence=0;
 UPROPERTY() TArray<int32> Ledger;
};
