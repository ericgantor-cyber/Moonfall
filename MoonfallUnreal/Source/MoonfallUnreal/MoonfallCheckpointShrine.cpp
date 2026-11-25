#include "MoonfallCheckpointShrine.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
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
 // The imported reliquary replaces the primitive display only if present
 // and usable. Discovery volume, respawn height, label and light stay fixed.
 if (FPackageName::DoesPackageExist(TEXT("/Game/MoonfallProduction/Meshes/SM_MF_LunarReliquary"))) {
  static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Reliquary(
      TEXT("/Game/MoonfallProduction/Meshes/SM_MF_LunarReliquary.SM_MF_LunarReliquary"));
  if (UStaticMesh* Imported=Reliquary.Get()) {
   const FBox Bounds=Imported->GetBoundingBox();
   const FVector Size=Bounds.GetSize();
   const float Span=static_cast<float>(Size.GetMax());
   if (Bounds.IsValid && !Bounds.Min.ContainsNaN() && !Bounds.Max.ContainsNaN()
       && FMath::IsFinite(Span) && Span>0.1f && Span<100000.f
       && ShrineBase->SetStaticMesh(Imported)) {
    const float Scale=80.f/Span;
    const FVector Center=Bounds.GetCenter();
    ShrineBase->SetRelativeScale3D(FVector(Scale));
    ShrineBase->SetRelativeLocation(FVector(-Center.X*Scale,-Center.Y*Scale,-12.5f-Bounds.Min.Z*Scale));
    ShrineBeacon->SetVisibility(false);
   }
  }
 }
 ShrineBase->SetGenerateOverlapEvents(false);
 ShrineBeacon->SetGenerateOverlapEvents(false);

 // Dark-fantasy beacon lighting without Tick, dynamic shadow casting,
 // extra animated effects, or changes to Z and Nyra's original artwork.
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

FString AMoonfallCheckpointShrine::GetMapDisplayName() const
{
 FString Title = MapDisplayName.ToString().TrimStartAndEnd();
 if (Title.IsEmpty()) {
  if (ShrineId == FName(TEXT("sanctuary_firstlight"))) Title = TEXT("Firstlight Shrine");
  else if (ShrineId == FName(TEXT("crossroads_west"))) Title = TEXT("Crossroads West Shrine");
  else if (ShrineId == FName(TEXT("crimson_path"))) Title = TEXT("Crimson Path Shrine");
  else if (ShrineId == FName(TEXT("moonlit_path"))) Title = TEXT("Moonlit Path Shrine");
  else {
   Title = ShrineId.IsNone() ? TEXT("Unnamed") : ShrineId.ToString().Replace(TEXT("_"), TEXT(" "));
   Title += TEXT(" Shrine");
  }
 }
 Title.ReplaceInline(TEXT("\n"), TEXT(" "));
 Title.ReplaceInline(TEXT("\r"), TEXT(" "));
 return Title.Left(36);
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
 const FString ShrineState = bCurrentRespawn ? TEXT("ACTIVE RESPAWN SHRINE")
     : bDiscovered ? TEXT("ACTIVE SHRINE") : TEXT("UNDISCOVERED SHRINE");
 ShrineLabel->SetText(FText::FromString(GetMapDisplayName() + TEXT("\n") + ShrineState));
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
 if (!GetWorld() || IsActorBeingDestroyed() || !IsValid(Hero)
     || Hero->IsActorBeingDestroyed() || Hero->GetWorld() != GetWorld() || ShrineId.IsNone())
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
 if (!GetWorld() || IsActorBeingDestroyed() || !IsValid(Hero)
     || Hero->IsActorBeingDestroyed() || ShrineId.IsNone() || Hero->GetWorld() != GetWorld()) return;
 if (!FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || Hero->GetActorLocation().ContainsNaN() || GetActorLocation().ContainsNaN()) return;
 if (Hero->HasDiscoveredShrine(ShrineId))
 {
  RefreshShrineDisplay(Hero);
  GetWorldTimerManager().ClearTimer(DiscoveryRetryTimer);
  PendingDiscoveryHero.Reset();
  Hero->RestAtDiscoveredPreviewShrine(this);
  return;
 }
 if (Hero->DiscoverShrine(this))
 {
  // The discovery event may synchronously destroy either actor.
  if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld()) return;
  if (IsValid(Hero) && !Hero->IsActorBeingDestroyed()) RefreshShrineDisplay(Hero);
  GetWorldTimerManager().ClearTimer(DiscoveryRetryTimer);
  PendingDiscoveryHero.Reset();
  if (IsValid(Hero) && !Hero->IsActorBeingDestroyed()) Hero->RestAtDiscoveredPreviewShrine(this);
  return;
 }
 // If an entry occurs mid-fall or while a transient save cannot complete,
 // an overlap event will not fire again until the hero leaves. Retry at most
 // eight times while nearby, with no per-frame Actor Tick.
 if (!IsValid(this) || IsActorBeingDestroyed() || !GetWorld()
     || !IsValid(Hero) || Hero->IsActorBeingDestroyed()) return;
 const FVector Location = Hero->GetActorLocation();
 if (!FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f || Location.ContainsNaN()
     || GetActorLocation().ContainsNaN()
     || FVector::DistSquared(Location, GetActorLocation()) > FMath::Square(220.f)) return;
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
 if (!GetWorld() || IsActorBeingDestroyed()) return;
 if (!IsValid(Hero) || Hero->IsActorBeingDestroyed() || ShrineId.IsNone() || Hero->GetWorld() != GetWorld()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || Hero->GetActorLocation().ContainsNaN() || GetActorLocation().ContainsNaN()
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
