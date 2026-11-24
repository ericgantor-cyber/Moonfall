#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MoonfallNpcSocialSave.generated.h"

USTRUCT()
struct FMoonfallSavedNpcRelationship {
 GENERATED_BODY()
 UPROPERTY(SaveGame) FName Observer;
 UPROPERTY(SaveGame) FName Other;
 UPROPERTY(SaveGame) int32 Score=0;
};

UCLASS()
class MOONFALLUNREAL_API UMoonfallNpcSocialSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY(SaveGame) int32 Version=1;
 UPROPERTY(SaveGame) int64 Generation=0;
 UPROPERTY(SaveGame) TArray<FMoonfallSavedNpcRelationship> Entries;
};
