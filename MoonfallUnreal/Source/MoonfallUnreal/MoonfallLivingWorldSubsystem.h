#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MoonfallLivingWorldSubsystem.generated.h"

UENUM(BlueprintType)
enum class EMoonfallEncounterState : uint8 { Unknown, Available, Active, Completed };
USTRUCT(BlueprintType)
struct FMoonfallWorldRumor {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly, Category="Living World") FName RumorId;
 UPROPERTY(BlueprintReadOnly, Category="Living World") FName Origin;
 UPROPERTY(BlueprintReadOnly, Category="Living World") FName LastSpeaker;
 UPROPERTY(BlueprintReadOnly, Category="Living World") int32 Confidence=0;
 UPROPERTY(BlueprintReadOnly, Category="Living World") int32 Hops=0;
 UPROPERTY(BlueprintReadOnly, Category="Living World") bool bDirectWitness=false;
 UPROPERTY(BlueprintReadOnly, Category="Living World") bool bCrime=false;
 UPROPERTY(BlueprintReadOnly, Category="Living World") bool bWitnessReport=false;
 UPROPERTY(BlueprintReadOnly, Category="Living World") FVector Location=FVector::ZeroVector;
 UPROPERTY(BlueprintReadOnly, Category="Living World") double ExpiresAt=0;
};
// World-scoped, authoritative social simulation. Host supplies stable NPC IDs and verified observations.
UCLASS()
class MOONFALLUNREAL_API UMoonfallLivingWorldSubsystem : public UWorldSubsystem {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Moonfall Living World") bool AdjustNpcRelationship(FName Observer, FName Other, int32 Delta);
 UFUNCTION(BlueprintPure, Category="Moonfall Living World") bool GetNpcRelationship(FName Observer, FName Other, int32& Score) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall Living World") bool PublishObservation(FName Witness, FName RumorId, FVector Location, bool bCrime, float LifetimeSeconds=120.f);
 UFUNCTION(BlueprintCallable, Category="Moonfall Living World") bool RelayRumor(FName Speaker, FName Listener, FName RumorId);
 UFUNCTION(BlueprintPure, Category="Moonfall Living World") bool GetRumor(FName Npc, FName RumorId, FMoonfallWorldRumor& Rumor) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall Living World") bool ReportWitnessToGuard(FName Witness, FName Guard, FName RumorId);
 UFUNCTION(BlueprintCallable, Category="Moonfall Living World") bool RegisterEncounter(FName Encounter, bool bRepeatable=false, float CooldownSeconds=300.f);
 UFUNCTION(BlueprintCallable, Category="Moonfall Living World") bool AdvanceEncounter(FName Encounter, EMoonfallEncounterState Next);
 UFUNCTION(BlueprintPure, Category="Moonfall Living World") EMoonfallEncounterState GetEncounterState(FName Encounter) const;
 virtual void Deinitialize() override;
private:
 struct FRelationship { FName Other; int32 Score=0; };
 struct FEncounter { EMoonfallEncounterState State=EMoonfallEncounterState::Available; bool bRepeatable=false; double Cooldown=300; double CompletedAt=0; };
 struct FObservation { FVector Location=FVector::ZeroVector; bool bCrime=false; double ExpiresAt=0; };
 TMap<FName,TArray<FRelationship>> Relationships;
 TMap<FName,TArray<FMoonfallWorldRumor>> Knowledge;
 TMap<FName,FObservation> Observations;
 TMap<FName,FEncounter> Encounters;
 bool CanWrite() const;
 double Now() const;
 bool StoreRumor(FName Npc,const FMoonfallWorldRumor& Rumor);
};
