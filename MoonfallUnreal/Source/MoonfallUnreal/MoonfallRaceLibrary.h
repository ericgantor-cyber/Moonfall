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

// Read-only UI preview. Does not modify character stats or save data.
USTRUCT(BlueprintType)
struct FMoonfallRaceStatPreview
{
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Races") float Vitality = 0.f;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Races") float Power = 0.f;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Races") float Speed = 0.f;
};

UCLASS()
class MOONFALLUNREAL_API UMoonfallRaceLibrary : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 // Stable identifiers for character selection, spawn recipes and enemy BPs.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static TArray<FName> GetRaceIds();

 // Only candidates marked playable. Does not alter save IDs or character art.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static TArray<FName> GetPlayableRaceIds();

 // Return complete character-selection metadata in one Blueprint call.
 // This is a catalog lookup, not a player race-selection or save mutation.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static TArray<FMoonfallRaceProfile> GetPlayableRaceProfiles();

 // Filter the original roster by family, including nonplayable enemy races.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static TArray<FName> GetRaceIdsByFamily(FName Family);

 // Full profile data for NPC/enemy catalog browsing by race family.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static TArray<FMoonfallRaceProfile> GetRaceProfilesByFamily(FName Family);

 // Unique family IDs in catalog order, for race browsing and NPC filtering.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static TArray<FName> GetRaceFamilies();

 // Reject nonplayable/unknown races for character-selection validation.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static bool FindPlayableRaceProfile(FName RaceId, FMoonfallRaceProfile& Profile);

 // Preview race-adjusted base stats without changing a hero or save file.
 // Invalid race IDs, nonplayable races, or negative/nonfinite inputs fail closed.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static bool PreviewPlayableRaceStats(FName RaceId, float BaseVitality, float BasePower,
                                     float BaseSpeed, FMoonfallRaceStatPreview& Preview);

 // Returns false and an unmodified baseline profile for unknown identifiers.
 UFUNCTION(BlueprintPure, Category="Moonfall|Races")
 static bool FindRaceProfile(FName RaceId, FMoonfallRaceProfile& Profile);
};
