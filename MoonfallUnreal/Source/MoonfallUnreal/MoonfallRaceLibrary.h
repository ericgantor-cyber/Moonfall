#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MoonfallRaceLibrary.generated.h"

// Original Moonfall fantasy peoples. A race profile is gameplay data, not a
// license to import Tolkien-owned characters, factions or source art.
USTRUCT(BlueprintType)
struct FMoonfallRaceProfile
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Races") FName RaceId = NAME_None;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Races") FText DisplayName;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Races") FName Family = NAME_None;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Races") bool bPlayableCandidate = false;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Races") float VitalityMultiplier = 1.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Races") float PowerMultiplier = 1.f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Races") float SpeedMultiplier = 1.f;
};

UCLASS()
class MOONFALLUNREAL_API UMoonfallRaceLibrary : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 // Stable identifiers for character selection, spawn recipes and enemy BPs.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static TArray<FName> GetRaceIds();

 // Returns false and an unmodified baseline profile for unknown identifiers.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static bool FindRaceProfile(FName RaceId, FMoonfallRaceProfile& Profile);
};
