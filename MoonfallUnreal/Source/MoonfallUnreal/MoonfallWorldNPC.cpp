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
FString AMoonfallWorldNPC::GetCurrentActivity() const {
 // Cyclical ambient schedule derived on demand, avoiding a per-NPC Tick.
 const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
 const int32 Shift = FMath::FloorToInt(FMath::Max(0.f, WorldTime) / 300.f) % 3;
 if (SocialRole == FName(TEXT("Guard"))) return Shift == 2 ? TEXT("Patrolling") : TEXT("Watching");
 if (SocialRole == FName(TEXT("Merchant"))) return Shift == 2 ? TEXT("Restocking") : TEXT("Trading");
 if (SocialRole == FName(TEXT("Scout"))) return Shift == 0 ? TEXT("Exploring") : TEXT("Reporting");
 return Shift == 2 ? TEXT("Resting") : TEXT("Gathering");
}
FString AMoonfallWorldNPC::GetSocialDisposition(const AMoonfallHeroPawn* Hero) const {
 const int32 Reputation = IsValid(Hero) ? Hero->GetFactionReputation(FactionId) : 0;
 if (Reputation <= -50) return TEXT("Hostile");
 if (Reputation <= -15) return TEXT("Wary");
 if (Reputation >= 50) return TEXT("Trusted");
 if (Reputation >= 15) return TEXT("Friendly");
 return TEXT("Neutral");
}
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
 // Social reaction uses a stable disposition. Negative reputation changes
 // dialogue but does not magically turn a civilian into a hostile actor.
 FText Reaction = Spoken;
 const FString Disposition = GetSocialDisposition(Hero);
 if (Disposition == TEXT("Hostile")) Reaction = FText::FromString(TEXT("You are not welcome here."));
 else if (Disposition == TEXT("Wary")) Reaction = FText::FromString(TEXT("Speak carefully, stranger."));
 else if (Disposition == TEXT("Trusted")) Reaction = FText::FromString(TEXT("Welcome back, friend of our people."));
 // Citizens provide contextual opening lines without spawning dialogue actors.
 if (Disposition == TEXT("Neutral") && SocialRole == FName(TEXT("Merchant")))
  Reaction = FText::FromString(TEXT("Browse my wares when the market opens."));
 else if (Disposition == TEXT("Neutral") && SocialRole == FName(TEXT("Guard")))
  Reaction = FText::FromString(TEXT("Keep the peace inside our walls."));
 // Activity is determined only when interacted with; inhabitants never spam
 // navigation or world perception. Trust/hostility overrides daily routine.
 if (Disposition == TEXT("Neutral") && SocialRole == FName(TEXT("Merchant"))
     && GetCurrentActivity() == TEXT("Restocking"))
  Reaction = FText::FromString(TEXT("The stall is closed while I restock."));
 else if (Disposition == TEXT("Neutral") && SocialRole == FName(TEXT("Scout"))
     && GetCurrentActivity() == TEXT("Reporting"))
  Reaction = FText::FromString(TEXT("The roads have changed. I am reporting what I saw."));
 OnDialogueStarted(Hero, Reaction);
 return true;
}
