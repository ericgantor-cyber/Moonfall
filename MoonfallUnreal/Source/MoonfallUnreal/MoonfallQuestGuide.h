#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallQuestGuide.generated.h"

class UStaticMeshComponent;

// Stationary, non-hostile landmark. Existing Unreal gameplay manages quest state.
UCLASS()
class MOONFALLUNREAL_API AMoonfallQuestGuide : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallQuestGuide();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 // Low-cost local day/shift routine, no AI controller or distant actor simulation.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 DayPhase = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 CurrentMood = 0;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC")
 void ReactToPlayer(int32 Trust, int32 Fear);
 // Allows conversation without NPC sliding away from the hero.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") float ConversationPausedUntil = 0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 CurrentRoutine = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quests")
 int32 RegionIndex = 0;
 // Three distinct civilian roles per region. 0 quest guide, 1 craftsperson,
 // 2 scout. Unique ID is stable across saves: Region*3 + Role.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 RoleIndex = 0;
 // 0..17 are existing authored characters; 18..1110 are streamed residents.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 PopulationNpcId = -1;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") int32 GetPersistentNpcId() const { return PopulationNpcId >= 18 && PopulationNpcId < 1111 ? PopulationNpcId : FMath::Clamp(RegionIndex, 0, 5) * 3 + FMath::Clamp(RoleIndex, 0, 2); }
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") FText GetPersonalityLabel() const;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quests")
 UStaticMeshComponent* Body = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quests") UStaticMeshComponent* Head = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quests") UStaticMeshComponent* LeftArm = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quests") UStaticMeshComponent* RightArm = nullptr;
 // A deterministic named placeholder, replaceable with authored regional NPC assets.
 UFUNCTION(BlueprintPure, Category="Moonfall|Quests") FText GetGuideTitle() const;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") FText GetRoutineLabel() const;
private:
 FVector HomeLocation = FVector::ZeroVector;
 float LastVisualMoodUpdate = -1.f;
};
