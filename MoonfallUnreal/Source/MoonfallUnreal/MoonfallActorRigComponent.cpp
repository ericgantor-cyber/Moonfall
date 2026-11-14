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
 ActiveRigMesh = nullptr;
 AActor* Owner = GetOwner();
 if (IsValid(Owner))
 {
  // An explicitly selected rig must belong to the same actor. A rig
  // reference on another actor is never safe to animate from this component.
  if (PreferredRigMesh != nullptr)
  {
   if (IsValid(PreferredRigMesh)
       && PreferredRigMesh->GetOwner() == Owner
       && IsValid(PreferredRigMesh->GetSkeletalMeshAsset()))
    RigMesh = PreferredRigMesh;
   // Fail closed for invalid explicit selections rather than silently
   // animating an unrelated mesh such as an NPC's clothing or equipment.
  }
  else
  {
   // Auto-discovery is intentionally bounded to the first valid mesh.
   // The first skeletal component can be empty on Blueprint actors.
   TArray<USkeletalMeshComponent*> Candidates;
   Owner->GetComponents<USkeletalMeshComponent>(Candidates);
   for (USkeletalMeshComponent* Mesh : Candidates)
   {
    if (IsValid(Mesh) && IsValid(Mesh->GetSkeletalMeshAsset()))
    {
     RigMesh = Mesh;
     break;
    }
   }
  }
 }
 ActiveRigMesh = RigMesh.Get();
 ObservedPreferredRigMesh = PreferredRigMesh;
 ObservedRigAsset = RigMesh.IsValid() ? RigMesh->GetSkeletalMeshAsset() : nullptr;
 bRigAvailable = RigMesh.IsValid();
 bLastCuePlayed = false;
}

bool UMoonfallActorRigComponent::PlayRigCue(FName CueId)
{
 // Refresh on demand after swapping an NPC mesh or the preferred rig.
 // This component never polls, spawns geometry, or alters imported assets.
 AActor* Owner = GetOwner();
 const bool bOwnerChanged = RigMesh.IsValid() && RigMesh->GetOwner() != Owner;
 const bool bPreferredChanged = PreferredRigMesh.Get() != ObservedPreferredRigMesh.Get();
 const bool bAssetChanged = RigMesh.IsValid()
   && RigMesh->GetSkeletalMeshAsset() != ObservedRigAsset.Get();
 if (bOwnerChanged || bPreferredChanged || bAssetChanged || !RigMesh.IsValid())
  RefreshRig();
 bLastCuePlayed = false;
 // Fail closed on an absent rig or cue but always notify the Blueprint.
 if (!CueId.IsNone() && IsValid(Owner) && RigMesh.IsValid()
     && RigMesh->GetOwner() == Owner
     && IsValid(RigMesh->GetSkeletalMeshAsset()))
 {
  const TObjectPtr<UAnimMontage>* Entry = MontageCues.Find(CueId);
  if (Entry && IsValid(Entry->Get()))
  {
   // Animation instances may be pending destruction during a rig hot-swap.
   // Check UObject validity before calling into Montage_Play.
   UAnimInstance* Anim = RigMesh->GetAnimInstance();
   if (IsValid(Anim))
   {
    const float Duration = Anim->Montage_Play(Entry->Get(), 1.f);
    bLastCuePlayed = FMath::IsFinite(Duration) && Duration > 0.f;
   }
  }
 }
 OnRigCue(CueId, bLastCuePlayed);
 return bLastCuePlayed;
}
