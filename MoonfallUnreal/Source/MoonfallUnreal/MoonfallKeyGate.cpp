#include "MoonfallKeyGate.h"
#include "MoonfallHeroPawn.h"
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

bool AMoonfallKeyGate::TryUnlock(AActor* Candidate)
{
 if (bUnlocked) return true;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(Candidate);
 if (!IsValid(Hero) || Hero->Health <= 0.f) return false;
 // Reject a blank/unknown key: the hero's whitelist checks supported IDs.
 if (!Hero->HasQuestKey(RequiredKeyId))
 {
  OnGateLocked(RequiredKeyId);
  return false;
 }
 bUnlocked = true;
 GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 GateMesh->SetVisibility(false, true);
 GateLabel->SetVisibility(false);
 OnGateUnlocked();
 return true;
}

