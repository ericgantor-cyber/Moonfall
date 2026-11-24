#include "MoonfallWorldNPC.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallRaceCatalog.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "MoonfallLivingSimulation.h"

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
void AMoonfallWorldNPC::BeginPlay() {
 Super::BeginPlay();
 AuthoredHomeLocation = GetActorLocation();
}
void AMoonfallWorldNPC::WitnessNearbyDanger(float Now) {
 if (FMath::IsFinite(Now) && Now >= 0.f) LastDangerAt = Now;
}
void AMoonfallWorldNPC::AdvanceLivingRoutine(float Now, const FVector& HeroLocation) {
 if (bAmbientVisitor || !GetWorld() || !FMath::IsFinite(Now)
     || AuthoredHomeLocation.ContainsNaN() || HeroLocation.ContainsNaN()
     || FVector::DistSquared2D(HeroLocation, GetActorLocation()) < FMath::Square(420.f))
  return;
 // Compact authored-home offsets: no teleport, path solve or NPC Tick.
 const int Shift=(MoonfallLivingSimulation::Phase(Now,180.f)
     + static_cast<int>(GetTypeHash(ResidentId)%4u))%4;
 const float Side=Shift==0?-65.f:(Shift==1?65.f:0.f);
 const float Front=Shift==2?45.f:(Shift==3?-45.f:0.f);
 const FVector Target=AuthoredHomeLocation+FVector(Side,Front,0.f);
 FVector Step=Target-GetActorLocation();
 Step.Z=0.f;
 if (Step.SizeSquared2D() > FMath::Square(1.f))
  AddActorWorldOffset(Step.GetClampedToMaxSize2D(32.f),true);
}
int32 AMoonfallWorldNPC::GetBondWithHero(const AMoonfallHeroPawn* Hero) const {
 return IsValid(Hero) ? Hero->GetResidentBond(ResidentId) : 0;
}
bool AMoonfallWorldNPC::RewardResidentTrust(AMoonfallHeroPawn* Hero, int32 Points) {
 if (!IsValid(Hero) || ResidentId.IsNone() || Points <= 0 || Points > 10
     || !CanHeroInteract(Hero)) return false;
 // One introductory favor per resident; further bond progression belongs
 // to verified quest milestones, not an endlessly reusable conversation key.
 if (Hero->GetResidentBond(ResidentId) != 0) return false;
 return Hero->ChangeResidentBond(ResidentId, Points);
}
int32 AMoonfallWorldNPC::GetContractSalvageCost() const {
 if (bAmbientVisitor) return 0;
 return ResidentId == FName(TEXT("CrossroadsGuide")) ? 10
   : 10 + (MoonfallRaces::Validated(RaceId) % 4) * 5;
}
bool AMoonfallWorldNPC::CanHeroInteract(const AMoonfallHeroPawn* Hero) const {
 if (!IsValid(Hero) || !GetWorld() || Hero->Health <= 0.f
     || Hero->bInventoryOpen || Hero->bOptionsOpen) return false;
 const FVector From = Hero->GetActorLocation();
 const FVector To = GetActorLocation();
 const float Range = FMath::IsFinite(InteractRange)
     ? FMath::Clamp(InteractRange, 50.f, 400.f) : 240.f;
 if (From.ContainsNaN() || To.ContainsNaN()
     || FVector::DistSquared(From, To) > FMath::Square(Range)) return false;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallNPCTalk), false);
 Params.AddIgnoredActor(Hero);
 FHitResult Hit;
 const bool bHit = GetWorld()->LineTraceSingleByChannel(
     Hit, From + FVector(0,0,55), To + FVector(0,0,55), ECC_Visibility, Params);
 return !bHit || Hit.GetActor() == this;
}
FString AMoonfallWorldNPC::GetRaceName() const { return FString(MoonfallRaces::Name(RaceId)); }
FString AMoonfallWorldNPC::GetCurrentActivity() const {
 // Weather-aware daily shifts: queried on demand, never on an NPC Tick.
 const float Now=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
 const int Region=MoonfallLivingSimulation::RegionFromWorldX(GetActorLocation().X);
 const auto Weather=MoonfallLivingSimulation::WeatherFor(
     Region,MoonfallLivingSimulation::Phase(Now,360.f));
 using ERole=MoonfallLivingSimulation::ERole;
 ERole Role=ERole::Citizen;
 if (SocialRole==FName(TEXT("Guard"))) Role=ERole::Guard;
 else if (SocialRole==FName(TEXT("Merchant"))) Role=ERole::Merchant;
 else if (SocialRole==FName(TEXT("Scout"))) Role=ERole::Scout;
 else if (SocialRole==FName(TEXT("Scholar"))) Role=ERole::Scholar;
 else if (SocialRole==FName(TEXT("Tinker"))) Role=ERole::Tinker;
 else if (SocialRole==FName(TEXT("Elder"))) Role=ERole::Elder;
 const int Shift=MoonfallLivingSimulation::Phase(Now,180.f)
      + static_cast<int>(GetTypeHash(ResidentId)%4u);
 switch(MoonfallLivingSimulation::ActivityFor(Role,Shift,Weather)) {
 case MoonfallLivingSimulation::EActivity::Patrol:return TEXT("Patrolling");
 case MoonfallLivingSimulation::EActivity::Travel:return TEXT("Traveling");
 case MoonfallLivingSimulation::EActivity::Rest:return TEXT("Resting");
 case MoonfallLivingSimulation::EActivity::Shelter:return TEXT("Sheltering");
 case MoonfallLivingSimulation::EActivity::Work:
  return Role==ERole::Merchant?TEXT("Trading"):
     (Role==ERole::Guard?TEXT("Watching"):
     (Role==ERole::Scout?TEXT("Reporting"):
     (Role==ERole::Scholar?TEXT("Studying"):TEXT("Working"))));
 }
 return TEXT("Working");
}
bool AMoonfallWorldNPC::SharesRaceWith(const AMoonfallHeroPawn* Hero) const {
 return IsValid(Hero) && Hero->bRaceChosen
    && Hero->RaceId == MoonfallRaces::Validated(RaceId);
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
 if (!CanHeroInteract(Hero)) return false;
 InteractionCount = FMath::Min(InteractionCount + 1, 1000000);
 // The existing custom greeting wins; default dialogue reflects ancestry.
 const FText Spoken = Greeting.EqualTo(FText::FromString(TEXT("The road stretches on. Need directions?")))
    ? FText::FromString(MoonfallRaces::Greeting(RaceId)) : Greeting;
 // Social reaction uses a stable disposition. Negative reputation changes
 // dialogue but does not magically turn a civilian into a hostile actor.
 FText Reaction = Spoken;
 const FString Disposition = GetSocialDisposition(Hero);
 // Shared ancestry earns recognition, not free reputation or blocked quests.
 if (Disposition == TEXT("Neutral") && SharesRaceWith(Hero))
  Reaction = FText::FromString(FString::Printf(
     TEXT("A fellow %s! %s"), MoonfallRaces::Name(RaceId), *Spoken.ToString()));
 if (Disposition == TEXT("Hostile")) Reaction = FText::FromString(TEXT("You are not welcome here."));
 else if (Disposition == TEXT("Wary")) Reaction = FText::FromString(TEXT("Speak carefully, stranger."));
 else if (Disposition == TEXT("Trusted")) Reaction = FText::FromString(TEXT("Welcome back, friend of our people."));
 // Citizens provide contextual opening lines without spawning dialogue actors.
 if (Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Merchant")))
  Reaction = FText::FromString(TEXT("Browse my wares when the market opens."));
 else if (Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Guard")))
  Reaction = FText::FromString(TEXT("Keep the peace inside our walls."));
 // Activity is determined only when interacted with; inhabitants never spam
 // navigation or world perception. Trust/hostility overrides daily routine.
 if (Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Merchant"))
     && (GetCurrentActivity() == TEXT("Resting")
         || GetCurrentActivity() == TEXT("Sheltering")))
  Reaction = FText::FromString(TEXT("The stall is closed while I restock."));
 else if (Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Scout"))
     && (GetCurrentActivity() == TEXT("Reporting")
         || GetCurrentActivity() == TEXT("Sheltering")))
  Reaction = FText::FromString(TEXT("The roads have changed. I am reporting what I saw."));
 // Individual trust changes the greeting without changing faction rank.
 // Negative bonds can come from quest consequences, not conversation farming.
 const int32 Bond = GetBondWithHero(Hero);
 if (Disposition != TEXT("Hostile") && Bond >= 60)
  Reaction = FText::FromString(FString::Printf(
    TEXT("My trusted companion, %s stands with you."), *DisplayName.ToString()));
 else if (Disposition != TEXT("Trusted") && Bond <= -60)
  Reaction = FText::FromString(TEXT("I remember what you did. Keep your distance."));
 // Accepted one-time contracts have useful, contextual status.
 if (!bAmbientVisitor && Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 0) {
  // Firstlight residents can tell their authored story on first meeting
  // while retaining the existing one-time salvage contract acceptance.
  // All Crossroads and other NPC dialogue remains unchanged.
  const FString Offer = TEXT("I have a task for you. Select Accept in our conversation.");
  Reaction = SettlementId == FName(TEXT("Firstlight"))
    ? FText::FromString(Spoken.ToString() + TEXT(" ") + Offer)
    : FText::FromString(Offer);
 }
 else if (!bAmbientVisitor && Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 1)
  Reaction = FText::FromString(FString::Printf(
    TEXT("Bring me %d salvage. Select Turn In when ready."), GetContractSalvageCost()));
 else if (!bAmbientVisitor && Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 2)
  Reaction = FText::FromString(TEXT("Our people remember your help. Thank you."));
 // Short-lived witness memory never changes quests or faction reputation.
 const float Now=GetWorld()?GetWorld()->GetTimeSeconds():-1.f;
 if (FMath::IsFinite(Now) && Now>=LastDangerAt && Now-LastDangerAt<85.f
     && Disposition!=TEXT("Hostile"))
  Reaction=FText::FromString(Reaction.ToString()
      + TEXT(" I heard fighting nearby. Stay alert."));
 LastSpokenLine = Reaction;
 OnDialogueStarted(Hero, Reaction);
 return true;
}

bool AMoonfallWorldNPC::OfferResidentQuest(AMoonfallHeroPawn* Hero) {
 if (bAmbientVisitor || ResidentId.IsNone() || !CanHeroInteract(Hero)
     || Hero->GetFactionReputation(FactionId) <= -50) return false;
 return Hero->AcceptResidentQuest(ResidentId);
}
bool AMoonfallWorldNPC::TurnInResidentQuest(AMoonfallHeroPawn* Hero) {
 if (bAmbientVisitor || ResidentId.IsNone() || !CanHeroInteract(Hero)
     || Hero->GetFactionReputation(FactionId) <= -50) return false;
 return Hero->CompleteResidentQuest(this);
}
