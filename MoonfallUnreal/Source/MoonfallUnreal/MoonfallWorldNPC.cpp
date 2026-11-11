#include "MoonfallWorldNPC.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallRaceCatalog.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

AMoonfallWorldNPC::AMoonfallWorldNPC() {
 PrimaryActorTick.bCanEverTick = false;
 Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
 SetRootComponent(Body);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 if (Mesh.Succeeded()) Body->SetStaticMesh(Mesh.Object);
 Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Body->SetCollisionResponseToAllChannels(ECR_Ignore);
 Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
 DisplayName = FText::FromString(TEXT("Wayfarer"));
 Greeting = FText::FromString(TEXT("The road stretches on. Need directions?"));
}
FString AMoonfallWorldNPC::GetRaceName() const { return FString(MoonfallRaces::Name(RaceId)); }
bool AMoonfallWorldNPC::TryTalk(AMoonfallHeroPawn* Hero) {
 if (!IsValid(Hero) || !GetWorld() || Hero->Health <= 0.f
     || Hero->bInventoryOpen || Hero->bOptionsOpen) return false;
 const FVector From = Hero->GetActorLocation();
 const FVector To = GetActorLocation();
 const float Range = FMath::IsFinite(InteractRange)
     ? FMath::Clamp(InteractRange, 50.f, 400.f) : 240.f;
 if (From.ContainsNaN() || To.ContainsNaN()
     || FVector::DistSquared(From, To) > FMath::Square(Range)) return false;
 // Only interact through unobstructed space; no dialogue through walls.
 FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallNPCTalk), false);
 Params.AddIgnoredActor(Hero);
 FHitResult Hit;
 const bool bHit = GetWorld()->LineTraceSingleByChannel(
     Hit, From + FVector(0,0,55), To + FVector(0,0,55), ECC_Visibility, Params);
 if (bHit && Hit.GetActor() != this) return false;
 InteractionCount = FMath::Min(InteractionCount + 1, 1000000);
 // The existing custom greeting wins; default dialogue reflects ancestry.
 const FText Spoken = Greeting.EqualTo(FText::FromString(TEXT("The road stretches on. Need directions?")))
    ? FText::FromString(MoonfallRaces::Greeting(RaceId)) : Greeting;
 OnDialogueStarted(Hero, Spoken);
 return true;
}
