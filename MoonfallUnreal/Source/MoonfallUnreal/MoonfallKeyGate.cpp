#include "MoonfallKeyGate.h"
#include "MoonfallHeroPawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AMoonfallKeyGate::AMoonfallKeyGate()
{
 PrimaryActorTick.bCanEverTick = false;

 GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
 SetRootComponent(GateMesh);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(
   TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) GateMesh->SetStaticMesh(Cube.Object);
 GateMesh->SetRelativeScale3D(FVector(0.7f, 3.f, 3.f));
 GateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 GateMesh->SetCollisionResponseToAllChannels(ECR_Block);

 KeyProximity = CreateDefaultSubobject<UBoxComponent>(TEXT("KeyProximity"));
 KeyProximity->SetupAttachment(GateMesh);
 KeyProximity->SetBoxExtent(FVector(250.f, 190.f, 210.f));
 KeyProximity->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 KeyProximity->SetCollisionResponseToAllChannels(ECR_Ignore);
 KeyProximity->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
 KeyProximity->SetGenerateOverlapEvents(true);
 KeyProximity->OnComponentBeginOverlap.AddDynamic(
   this, &AMoonfallKeyGate::OnKeyProximityBegin);
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
 KeyProximity->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 GateMesh->SetVisibility(false, true);
 OnGateUnlocked();
 return true;
}

void AMoonfallKeyGate::OnKeyProximityBegin(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
 const FHitResult& SweepResult)
{
 TryUnlock(OtherActor);
}
