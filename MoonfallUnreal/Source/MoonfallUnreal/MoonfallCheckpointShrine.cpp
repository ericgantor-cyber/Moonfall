#include "MoonfallCheckpointShrine.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
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

 // Dark-fantasy beacon lighting without Tick, dynamic shadow casting,
 // imported assets, or changes to Z and Nyra's original artwork.
 ShrineLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ShrineLight"));
 ShrineLight->SetupAttachment(SceneRoot);
 ShrineLight->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
 ShrineLight->SetAttenuationRadius(420.f);
 ShrineLight->SetCastShadows(false);
 ShrineLight->SetIntensity(0.f);

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
 DiscoveryRange->OnComponentEndOverlap.AddDynamic(this, &AMoonfallCheckpointShrine::OnDiscoveryEndOverlap);
}

void AMoonfallCheckpointShrine::RefreshShrineDisplay(const AMoonfallHeroPawn* Hero)
{
 if (!ShrineLabel) return;
 const bool bDiscovered = IsValid(Hero) && Hero->HasDiscoveredShrine(ShrineId);
 // Every discovered shrine stays active permanently. Only the shrine at
 // the collision-safe checkpoint is the current respawn destination.
 const bool bCurrentRespawn = bDiscovered && Hero->ActiveShrineId == ShrineId
     && Hero->IsCurrentRespawnLocation(GetRespawnLocation());
 // Text distinguishes the current respawn point without suggesting other
 // discovered shrines have been deactivated.
 ShrineLabel->SetText(FText::FromString(bCurrentRespawn ? TEXT("ACTIVE RESPAWN SHRINE")
     : bDiscovered ? TEXT("ACTIVE SHRINE") : TEXT("UNDISCOVERED SHRINE")));
 // High-contrast secondary cue. Permanent activation is independent from
 // which shrine currently owns the respawn checkpoint; text remains primary
 // so the state is still readable without color vision.
 ShrineLabel->SetTextRenderColor(bCurrentRespawn ? FColor(255, 210, 105)
     : bDiscovered ? FColor(115, 245, 210) : FColor(170, 170, 170));
 if (ShrineLight)
 {
  // A discovered shrine stays lit, even if a later shrine becomes the
  // respawn checkpoint. Only the current checkpoint gets the warm highlight.
  ShrineLight->SetLightColor(bCurrentRespawn
      ? FLinearColor(1.f, 0.65f, 0.25f)
      : FLinearColor(0.15f, 0.85f, 0.72f));
  ShrineLight->SetIntensity(bCurrentRespawn ? 850.f : bDiscovered ? 450.f : 0.f);
 }
}

void AMoonfallCheckpointShrine::CheckHeroAtSpawn(AMoonfallHeroPawn* Hero)
{
 if (!IsValid(Hero) || Hero->GetWorld() != GetWorld() || ShrineId.IsNone())
  return;
 // A shrine can begin play before the hero restores its saved checkpoint.
 // The subsequent one-time hero scan must handle that ordering, without
 // relying on a fresh overlap or a per-frame shrine Tick.
 RefreshShrineDisplay(Hero);
 const FVector HeroLocation = Hero->GetActorLocation();
 const FVector ShrineLocation = GetActorLocation();
 if (Hero->HasDiscoveredShrine(ShrineId) || HeroLocation.ContainsNaN()
     || ShrineLocation.ContainsNaN()
     || FVector::DistSquared(HeroLocation, ShrineLocation) > FMath::Square(220.f))
  return;
 TryDiscoverOrQueueRetry(Hero);
}

void AMoonfallCheckpointShrine::TryDiscoverOrQueueRetry(AMoonfallHeroPawn* Hero)
{
 if (!IsValid(Hero) || ShrineId.IsNone() || Hero->GetWorld() != GetWorld()) return;
 if (Hero->HasDiscoveredShrine(ShrineId))
 {
  RefreshShrineDisplay(Hero);
  GetWorldTimerManager().ClearTimer(DiscoveryRetryTimer);
  PendingDiscoveryHero.Reset();
  return;
 }
 if (Hero->DiscoverShrine(this))
 {
  RefreshShrineDisplay(Hero);
  GetWorldTimerManager().ClearTimer(DiscoveryRetryTimer);
  PendingDiscoveryHero.Reset();
  return;
 }
 // If an entry occurs mid-fall or while a transient save cannot complete,
 // an overlap event will not fire again until the hero leaves. Retry at most
 // eight times while nearby, with no per-frame Actor Tick.
 const FVector Location = Hero->GetActorLocation();
 if (Hero->Health <= 0.f || Location.ContainsNaN() ||
     FVector::DistSquared(Location, GetActorLocation()) > FMath::Square(220.f)) return;
 if (!GetWorldTimerManager().IsTimerActive(DiscoveryRetryTimer))
 {
  DiscoveryRetryAttempts = 0;
  PendingDiscoveryHero = Hero;
  GetWorldTimerManager().SetTimer(DiscoveryRetryTimer, this,
      &AMoonfallCheckpointShrine::RetryPendingDiscovery, 1.f, true);
 }
}

void AMoonfallCheckpointShrine::RetryPendingDiscovery()
{
 AMoonfallHeroPawn* Hero = PendingDiscoveryHero.Get();
 if (!IsValid(Hero) || ShrineId.IsNone() || Hero->GetWorld() != GetWorld()
     || Hero->Health <= 0.f || Hero->GetActorLocation().ContainsNaN()
     || FVector::DistSquared(Hero->GetActorLocation(), GetActorLocation()) > FMath::Square(220.f)
     || ++DiscoveryRetryAttempts > 8)
 {
  GetWorldTimerManager().ClearTimer(DiscoveryRetryTimer);
  PendingDiscoveryHero.Reset();
  return;
 }
 TryDiscoverOrQueueRetry(Hero);
}

void AMoonfallCheckpointShrine::BeginPlay()
{
 Super::BeginPlay();
 // Actors created after the hero loads should show saved status immediately.
 // Actors created during startup will respond when the player enters overlap.
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 CheckHeroAtSpawn(Hero);
}

void AMoonfallCheckpointShrine::OnDiscoveryOverlap(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
 bool bFromSweep, const FHitResult& SweepResult)
{
 TryDiscoverOrQueueRetry(Cast<AMoonfallHeroPawn>(OtherActor));
}

void AMoonfallCheckpointShrine::OnDiscoveryEndOverlap(
 UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
 if (OtherActor == PendingDiscoveryHero.Get())
 {
  GetWorldTimerManager().ClearTimer(DiscoveryRetryTimer);
  PendingDiscoveryHero.Reset();
 }
}
