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
 GateLabel->SetText(FText::FromString(TEXT("LOCKED - CLICK")));
 GateLabel->SetTextRenderColor(FColor(255, 204, 125));


}

void AMoonfallKeyGate::BeginPlay()
{
 Super::BeginPlay();
 // Restore only a previously earned manual unlock. Reuse validated
 // player progress when the hero initialized first; avoid a save-file read
 // per gate in large dungeons. If the gate starts first, fall back to disk.
 if (GateId.IsNone()) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(
   UGameplayStatics::GetPlayerPawn(this, 0));
 if (IsValid(Hero) && Hero->bRewardSaveInitialized)
 {
  // Invalid/future saves must not unlock doors using uncertain live state.
  if (Hero->bRewardSaveLoadFailed || !Hero->HasUnlockedGate(GateId))
   return;
 }
 else
 {
  const FString& SlotName = UMoonfallRewardSaveGame::GetRewardSlotName();
  if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0)) return;
  const UMoonfallRewardSaveGame* Saved = Cast<UMoonfallRewardSaveGame>(
    UGameplayStatics::LoadGameFromSlot(SlotName, 0));
  if (!Saved || (Saved->SchemaVersion != 2 && Saved->SchemaVersion != 3 && Saved->SchemaVersion != 4 && Saved->SchemaVersion != 5)
      || !Saved->UnlockedGateIds.Contains(GateId))
   return;
 }
 ApplyOpenState();
 OnSavedGateRestored();
}

bool AMoonfallKeyGate::TryUnlock(AActor* Candidate)
{
 if (bUnlocked) return true;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(Candidate);
 if (!IsValid(Hero) || !GetWorld() || !FMath::IsFinite(Hero->Health)
     || Hero->Health <= 0.f || Hero->bOptionsOpen || Hero->bInventoryOpen)
  return false;
 // Blueprint callers cannot unlock a gate remotely, through a different
 // floor, or while a gameplay-blocking menu is open. This does not add
 // proximity auto-unlocking: a separate explicit interaction is still needed.
 const FVector HeroPosition = Hero->GetActorLocation();
 const FVector GatePosition = GetActorLocation();
 if (HeroPosition.ContainsNaN() || GatePosition.ContainsNaN()
     || FMath::Abs(HeroPosition.Z - GatePosition.Z) > 175.f
     || FVector::DistSquared2D(HeroPosition, GatePosition) > FMath::Square(275.f))
  return false;
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
    FString::Printf(TEXT("REQUIRES %s - CLICK"), MissingKeyName)));
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

