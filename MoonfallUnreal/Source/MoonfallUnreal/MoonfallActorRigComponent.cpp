#include "MoonfallActorRigComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"

UMoonfallActorRigComponent::UMoonfallActorRigComponent()
{
 PrimaryComponentTick.bCanEverTick = false;
}

void UMoonfallActorRigComponent::BeginPlay()
{
 Super::BeginPlay();
 RefreshRig();
}

void UMoonfallActorRigComponent::RefreshRig()
{
 RigMesh.Reset();
 AActor* Owner = GetOwner();
 if (IsValid(Owner))
 {
  // Only use an existing authored skeletal mesh on the owning actor.
  // Never synthesize, retarget or overwrite the artist's original assets.
  if (USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>())
  {
   if (IsValid(Mesh) && IsValid(Mesh->GetSkeletalMeshAsset()))
    RigMesh = Mesh;
  }
 }
 bRigAvailable = RigMesh.IsValid();
 bLastCuePlayed = false;
}

bool UMoonfallActorRigComponent::PlayRigCue(FName CueId)
{
 bLastCuePlayed = false;
 // Fail closed on an absent rig or cue but always notify the Blueprint.
 if (!CueId.IsNone() && RigMesh.IsValid())
 {
  const TObjectPtr<UAnimMontage>* Entry = MontageCues.Find(CueId);
  if (Entry && IsValid(Entry->Get()))
  {
   if (UAnimInstance* Anim = RigMesh->GetAnimInstance())
   {
    const float Duration = Anim->Montage_Play(Entry->Get(), 1.f);
    bLastCuePlayed = FMath::IsFinite(Duration) && Duration > 0.f;
   }
  }
 }
 OnRigCue(CueId, bLastCuePlayed);
 return bLastCuePlayed;
}
