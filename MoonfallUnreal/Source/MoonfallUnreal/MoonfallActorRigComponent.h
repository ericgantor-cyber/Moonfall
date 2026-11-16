#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoonfallActorRigComponent.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;

// Optional adapter for Blueprint NPCs and other actors. Unlike combat pawns,
// it never replaces an imported mesh, skeleton, material or animation asset.
UCLASS(ClassGroup=(Moonfall), meta=(BlueprintSpawnableComponent))
class MOONFALLUNREAL_API UMoonfallActorRigComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UMoonfallActorRigComponent();

 // For actors with multiple skeletal meshes, choose the intended rig
 // explicitly. When unset, auto-discovery selects the first valid mesh.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Rig")
 TObjectPtr<USkeletalMeshComponent> PreferredRigMesh = nullptr;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rig")
 TObjectPtr<USkeletalMeshComponent> ActiveRigMesh = nullptr;

 // Assign montages in the actor Blueprint; missing cues do not affect gameplay.
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Rig")
 TMap<FName, TObjectPtr<UAnimMontage>> MontageCues;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rig")
 bool bRigAvailable = false;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Rig")
 bool bLastCuePlayed = false;

 UFUNCTION(BlueprintCallable, Category="Moonfall|Rig")
 void RefreshRig();

 // These helpers expose available cues to Blueprint UI/AI without Tick.
 UFUNCTION(BlueprintPure, Category="Moonfall|Rig")
 bool HasRigCue(FName CueId) const;

 UFUNCTION(BlueprintPure, Category="Moonfall|Rig")
 TArray<FName> GetRigCueIds() const;

 // Report actual montage playback for AI transitions and UI without polling.
 UFUNCTION(BlueprintPure, Category="Moonfall|Rig")
 bool IsRigCuePlaying(FName CueId) const;

 UFUNCTION(BlueprintCallable, Category="Moonfall|Rig")
 bool PlayRigCue(FName CueId);

 // Stop only the requested montage, never unrelated hero/NPC animations.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Rig")
 bool StopRigCue(FName CueId, float BlendOutSeconds = 0.2f);

 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Rig")
 void OnRigCue(FName CueId, bool bPlayed);

protected:
 virtual void BeginPlay() override;

private:
 TWeakObjectPtr<USkeletalMeshComponent> RigMesh;
 // Detect a changed skeletal mesh on the next requested cue, without Tick.
 TWeakObjectPtr<UObject> ObservedRigAsset;
 TWeakObjectPtr<USkeletalMeshComponent> ObservedPreferredRigMesh;
};
