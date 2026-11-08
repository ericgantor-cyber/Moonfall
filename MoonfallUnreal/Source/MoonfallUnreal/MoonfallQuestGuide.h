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
 // A cheap deterministic three-stop routine, never a navmesh chase or crowd sim.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 CurrentRoutine = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quests")
 int32 RegionIndex = 0;
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
};
