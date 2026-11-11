#include "MoonfallKeyPickup.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

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
 // Rendered key never blocks player movement or the aimed interaction ray.
 // Use a generous invisible target for accessible third-person mouse aiming.
 KeyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 AimTarget = CreateDefaultSubobject<USphereComponent>(TEXT("AimTarget"));
 AimTarget->SetupAttachment(KeyRoot);
 AimTarget->InitSphereRadius(65.f);
 AimTarget->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
 AimTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 AimTarget->SetCollisionResponseToAllChannels(ECR_Ignore);
 AimTarget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
 AimTarget->SetGenerateOverlapEvents(false);
 KeyLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("KeyLabel"));
 KeyLabel->SetupAttachment(KeyRoot);
 KeyLabel->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
 KeyLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 KeyLabel->SetWorldSize(20.f);
 KeyLabel->SetText(FText::FromString(TEXT("KEY - F / CLICK")));
 KeyLabel->SetTextRenderColor(FColor(255, 215, 110));
}

void AMoonfallKeyPickup::RefreshKeyLabel()
{
 // A discoverable key should identify itself, not display a generic cube label.
 // Refresh after the level/deferred spawner has assigned its authored KeyId.
 FString Name = TEXT("UNKNOWN KEY");
 if (KeyId == FName(TEXT("wardens_crest")))
  Name = TEXT("WARDENS' CREST");
 else if (KeyId == FName(TEXT("outlaw_seal")))
  Name = TEXT("OUTLAW SEAL");
 else if (KeyId == FName(TEXT("fallen_gate_key")))
  Name = TEXT("FALLEN GATE KEY");
 KeyLabel->SetText(FText::FromString(Name + TEXT(" - F / CLICK")));
}

void AMoonfallKeyPickup::BeginPlay()
{
 Super::BeginPlay();
 RefreshKeyLabel();
 // A level may still contain an old placeholder after the key was saved.
 // The player may not have begun Play yet, so TryCollect also handles this.
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(
   UGameplayStatics::GetPlayerPawn(this, 0));
 if (IsValid(Hero) && Hero->HasQuestKey(KeyId))
  Destroy();
}

bool AMoonfallKeyPickup::TryCollect(AActor* Candidate)
{
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(Candidate);
 if (!IsValid(Hero) || !GetWorld() || !FMath::IsFinite(Hero->Health)
     || Hero->Health <= 0.f || Hero->bInventoryOpen || Hero->bOptionsOpen)
  return false;
 // Enforce physical reach even when a Blueprint calls TryCollect directly.
 // This keeps an inaccessible key from being acquired across walls or floors.
 const FVector HeroAt = Hero->GetActorLocation();
 const FVector KeyAt = GetActorLocation();
 if (HeroAt.ContainsNaN() || KeyAt.ContainsNaN()
     || FMath::Abs(HeroAt.Z - KeyAt.Z) > 150.f
     || FVector::DistSquared2D(HeroAt, KeyAt) > FMath::Square(230.f))
  return false;
 FCollisionQueryParams SightParams(SCENE_QUERY_STAT(MoonfallKeyDirectCollectSight), false);
 SightParams.AddIgnoredActor(Hero);
 SightParams.AddIgnoredActor(this);
 FHitResult Obstruction;
 if (GetWorld()->LineTraceSingleByChannel(Obstruction,
       HeroAt + FVector(0.f, 0.f, 40.f),
       KeyAt + FVector(0.f, 0.f, 40.f), ECC_Visibility, SightParams))
  return false;
 // A saved key may be represented by a leftover actor after streaming.
 // Clearing that duplicate is harmless; a new key is destroyed only after
 // CollectQuestKey has safely committed the save.
 if (Hero->HasQuestKey(KeyId))
 {
  Destroy();
  return true;
 }
 // The hero's whitelist rejects malformed or unrelated key IDs.
 if (!Hero->CollectQuestKey(KeyId)) return false;
 Destroy();
 return true;
}
