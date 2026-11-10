#include "MoonfallKeyGate.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallRewardSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AMoonfallKeyGate::AMoonfallKeyGate()
{
 PrimaryActorTick.bCanEverTick = false;

 GateRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GateRoot"));
 SetRootComponent(GateRoot);
 GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
 GateMesh->SetupAttachment(GateRoot);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
   TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) GateMesh->SetStaticMesh(Cube.Object);
 GateMesh->SetRelativeScale3D(FVector(0.7f, 3.f, 3.f));
 GateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 GateMesh->SetCollisionResponseToAllChannels(ECR_Block);
 GateLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("GateLabel"));
 GateLabel->SetupAttachment(GateRoot);
 GateLabel->SetRelativeLocation(FVector(0.f, 0.f, 185.f));
 GateLabel->SetWorldSize(25.f);
 GateLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 GateLabel->SetText(FText::FromString(TEXT("LOCKED  [F / X]")));
 GateLabel->SetTextRenderColor(FColor(255, 204, 125));


}

void AMoonfallKeyGate::BeginPlay()
{
 Super::BeginPlay();
 // This is replay of a deliberate unlock from a prior session, not a new
 // proximity unlock. Read directly from the versioned save so actor BeginPlay
 // order does not depend on when the hero loads its own reward progress.
 if (GateId.IsNone()) return;
 const FString SlotName(TEXT("Moonfall_RewardProgress_v1"));
 if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0)) return;
 const UMoonfallRewardSaveGame* Saved = Cast<UMoonfallRewardSaveGame>(
   UGameplayStatics::LoadGameFromSlot(SlotName, 0));
 if (!Saved || Saved->SchemaVersion != 2 || !Saved->UnlockedGateIds.Contains(GateId))
  return;
 ApplyOpenState();
 OnSavedGateRestored();
}

bool AMoonfallKeyGate::TryUnlock(AActor* Candidate)
{
 if (bUnlocked) return true;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(Candidate);
 if (!IsValid(Hero) || Hero->Health <= 0.f) return false;
 // Reject a blank/unknown key: the hero's whitelist checks supported IDs.
 if (!Hero->HasQuestKey(RequiredKeyId) && !Hero->HasUnlockedGate(GateId))
 {
  // Explicit interaction gives a clear clue; walking past never opens it.
  const TCHAR* MissingKeyName = TEXT("MATCHING KEY");
  if (RequiredKeyId == FName(TEXT("wardens_crest")))
   MissingKeyName = TEXT("WARDENS' CREST");
  else if (RequiredKeyId == FName(TEXT("outlaw_seal")))
   MissingKeyName = TEXT("OUTLAW SEAL");
  else if (RequiredKeyId == FName(TEXT("fallen_gate_key")))
   MissingKeyName = TEXT("FALLEN GATE KEY");
  GateLabel->SetText(FText::FromString(
    FString::Printf(TEXT("REQUIRES %s  [F / X]"), MissingKeyName)));
  OnGateLocked(RequiredKeyId);
  return false;
 }
 // Commit the door state first. A failed disk save must not make a gate
 // appear permanently solved when the next session still considers it locked.
 if (!Hero->CommitGateUnlock(GateId, RequiredKeyId)) return false;
 ApplyOpenState();
 OnGateUnlocked();
 return true;
}

void AMoonfallKeyGate::ApplyOpenState()
{
 bUnlocked = true;
 GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 GateMesh->SetVisibility(false, true);
 GateLabel->SetVisibility(false);
}

