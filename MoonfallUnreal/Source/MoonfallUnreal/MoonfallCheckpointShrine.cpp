#include "MoonfallCheckpointShrine.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

AMoonfallCheckpointShrine::AMoonfallCheckpointShrine()
{
 PrimaryActorTick.bCanEverTick = false;
 SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ShrineRoot"));
 SetRootComponent(SceneRoot);

 ShrineBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShrineBase"));
 ShrineBase->SetupAttachment(SceneRoot);
 ShrineBeacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShrineBeacon"));
 ShrineBeacon->SetupAttachment(SceneRoot);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 if (Cylinder.Succeeded()) ShrineBase->SetStaticMesh(Cylinder.Object);
 if (Sphere.Succeeded()) ShrineBeacon->SetStaticMesh(Sphere.Object);
 ShrineBase->SetRelativeScale3D(FVector(0.65f, 0.65f, 0.25f));
 ShrineBeacon->SetRelativeLocation(FVector(0.f, 0.f, 65.f));
 ShrineBeacon->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.22f));
 ShrineBase->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 ShrineBeacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 ShrineBase->SetGenerateOverlapEvents(false);
 ShrineBeacon->SetGenerateOverlapEvents(false);

 ShrineLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ShrineLabel"));
 ShrineLabel->SetupAttachment(SceneRoot);
 ShrineLabel->SetRelativeLocation(FVector(0.f, 0.f, 105.f));
 ShrineLabel->SetWorldSize(26.f);
 ShrineLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 ShrineLabel->SetText(FText::FromString(TEXT("UNDISCOVERED SHRINE")));

 DiscoveryRange = CreateDefaultSubobject<USphereComponent>(TEXT("DiscoveryRange"));
 DiscoveryRange->SetupAttachment(SceneRoot);
 DiscoveryRange->InitSphereRadius(220.f);
 DiscoveryRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 DiscoveryRange->SetCollisionResponseToAllChannels(ECR_Ignore);
 DiscoveryRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
 DiscoveryRange->SetGenerateOverlapEvents(true);
 DiscoveryRange->OnComponentBeginOverlap.AddDynamic(this, &AMoonfallCheckpointShrine::OnDiscoveryOverlap);
}

void AMoonfallCheckpointShrine::BeginPlay()
{
 Super::BeginPlay();
 // A player may already be standing near a shrine spawned at level start.
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 if (IsValid(Hero) && !ShrineId.IsNone()) {
  if (Hero->HasDiscoveredShrine(ShrineId)) {
   ShrineLabel->SetText(FText::FromString(TEXT("DISCOVERED SHRINE")));
  } else if (FVector::DistSquared(GetActorLocation(), Hero->GetActorLocation()) <= FMath::Square(220.f)
             && Hero->DiscoverShrine(this)) {
   ShrineLabel->SetText(FText::FromString(TEXT("SHRINE DISCOVERED")));
  }
 }
}

void AMoonfallCheckpointShrine::OnDiscoveryOverlap(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
 bool bFromSweep, const FHitResult& SweepResult)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(OtherActor);
 if (!IsValid(Hero) || ShrineId.IsNone()) return;
 if (Hero->HasDiscoveredShrine(ShrineId)) {
  ShrineLabel->SetText(FText::FromString(TEXT("DISCOVERED SHRINE")));
 } else if (Hero->DiscoverShrine(this)) {
  ShrineLabel->SetText(FText::FromString(TEXT("SHRINE DISCOVERED")));
 }
}
