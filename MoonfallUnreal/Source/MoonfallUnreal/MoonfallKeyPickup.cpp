#include "MoonfallKeyPickup.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AMoonfallKeyPickup::AMoonfallKeyPickup()
{
 PrimaryActorTick.bCanEverTick = false;
 KeyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("KeyRoot"));
 SetRootComponent(KeyRoot);
 KeyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeyMesh"));
 KeyMesh->SetupAttachment(KeyRoot);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Mesh.Succeeded()) KeyMesh->SetStaticMesh(Mesh.Object);
 KeyMesh->SetRelativeScale3D(FVector(0.18f, 0.32f, 0.10f));
 KeyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 KeyLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("KeyLabel"));
 KeyLabel->SetupAttachment(KeyRoot);
 KeyLabel->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
 KeyLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 KeyLabel->SetWorldSize(20.f);
 KeyLabel->SetText(FText::FromString(TEXT("KEY  [F]")));
 KeyLabel->SetTextRenderColor(FColor(255, 215, 110));
}

bool AMoonfallKeyPickup::TryCollect(AActor* Candidate)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(Candidate);
 if (!IsValid(Hero) || Hero->Health <= 0.f || Hero->bInventoryOpen || Hero->bOptionsOpen)
  return false;
 // The hero's whitelist rejects malformed or unrelated key IDs.
 if (!Hero->CollectQuestKey(KeyId)) return false;
 Destroy();
 return true;
}
