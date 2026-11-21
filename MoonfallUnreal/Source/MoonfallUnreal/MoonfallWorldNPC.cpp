#include "MoonfallWorldNPC.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallRaceCatalog.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

AMoonfallWorldNPC::AMoonfallWorldNPC() {
 PrimaryActorTick.bCanEverTick = false;
 Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
 SetRootComponent(Body);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 if (Mesh.Succeeded()) Body->SetStaticMesh(Mesh.Object);
 Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Body->SetCollisionResponseToAllChannels(ECR_Ignore);
 Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
 CouncilAssignmentSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CouncilAssignmentSign"));
 CouncilAssignmentSign->SetupAttachment(Body);
 CouncilAssignmentSign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 CouncilAssignmentSign->SetGenerateOverlapEvents(false);
 CouncilAssignmentSign->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
 CouncilAssignmentSign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 CouncilAssignmentSign->SetWorldSize(24.f);
 CouncilAssignmentSign->SetTextRenderColor(FColor(185, 222, 206));
 CouncilAssignmentSign->SetVisibility(false);
 CouncilWorkMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CouncilWorkMarker"));
 CouncilWorkMarker->SetupAttachment(Body);
 if (Mesh.Succeeded()) CouncilWorkMarker->SetStaticMesh(Mesh.Object);
 CouncilWorkMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 CouncilWorkMarker->SetGenerateOverlapEvents(false);
 CouncilWorkMarker->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.16f));
 CouncilWorkMarker->SetRelativeLocation(FVector(50.f, 0.f, 85.f));
 CouncilWorkMarker->SetVisibility(false);
 DisplayName = FText::FromString(TEXT("Wayfarer"));
 Greeting = FText::FromString(TEXT("The road stretches on. Need directions?"));
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
 // Cyclical ambient schedule derived on demand, avoiding a per-NPC Tick.
 const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
 const int32 Shift = FMath::FloorToInt(FMath::Max(0.f, WorldTime) / 300.f) % 3;
 if (SocialRole == FName(TEXT("Guard"))) return Shift == 2 ? TEXT("Patrolling") : TEXT("Watching");
 if (SocialRole == FName(TEXT("Merchant"))) return Shift == 2 ? TEXT("Restocking") : TEXT("Trading");
 if (SocialRole == FName(TEXT("Scout"))) return Shift == 0 ? TEXT("Exploring") : TEXT("Reporting");
 if (SocialRole == FName(TEXT("Scholar"))) return Shift == 2 ? TEXT("Studying") : TEXT("Teaching");
 if (SocialRole == FName(TEXT("Tinker"))) return Shift == 2 ? TEXT("Repairing") : TEXT("Inventing");
 if (SocialRole == FName(TEXT("Elder"))) return Shift == 2 ? TEXT("Reflecting") : TEXT("Advising");
 return Shift == 2 ? TEXT("Resting") : TEXT("Gathering");
}
void AMoonfallWorldNPC::RefreshCouncilAssignmentSign(const AMoonfallHeroPawn* Hero) {
 if (!IsValid(CouncilAssignmentSign)) return;
 const bool bShow = IsValid(Hero) && SettlementId == FName(TEXT("Crossroads"))
     && Hero->CouncilDecisionId >= 0 && Hero->CouncilDecisionId <= 2
     && !ResidentId.IsNone();
 if (!bShow) {
  CouncilAssignmentSign->SetVisibility(false);
  return;
 }
 // Only set the text on a policy transition, not every frame.
 CouncilAssignmentSign->SetText(FText::FromString(GetCouncilAssignment(Hero).Left(36)));
 CouncilAssignmentSign->SetTextRenderColor(
     Hero->CouncilDecisionId == 0 ? FColor(120, 232, 196)
     : Hero->CouncilDecisionId == 1 ? FColor(240, 191, 119)
     : FColor(174, 197, 240));
 CouncilAssignmentSign->SetVisibility(true);
}
void AMoonfallWorldNPC::RefreshCouncilStationActivity(const AMoonfallHeroPawn* Hero) {
 // Guard/scout work cues remain controlled exclusively by the patrol step.
 const bool bMerchant = SocialRole == FName(TEXT("Merchant"));
 const bool bTinker = SocialRole == FName(TEXT("Tinker"));
 if ((!bMerchant && !bTinker) || !IsValid(CouncilWorkMarker)) return;
 const bool bActive = IsValid(Hero) && Hero->CouncilDecisionId >= 0
     && Hero->CouncilDecisionId <= 2 && SettlementId == FName(TEXT("Crossroads"));
 if (!bActive) {
  CouncilWorkMarker->SetVisibility(false);
  CouncilStationVisualStep = 0;
  return;
 }
 // Policy determines the supplies on display, rather than granting free loot.
 // Merchant: public supply crates. Tinker: rebuild tools or border equipment.
 const int32 Policy = Hero->CouncilDecisionId;
 const float Width = bMerchant ? 0.38f : 0.16f;
 const float Height = bMerchant ? 0.10f : (Policy == 1 ? 0.30f : 0.20f);
 CouncilWorkMarker->SetRelativeScale3D(FVector(Width, Width, Height));
 CouncilWorkMarker->SetRelativeLocation(bMerchant
     ? FVector(42.f, 30.f, 25.f)
     : FVector(42.f, 18.f, 48.f));
 CouncilStationVisualStep = (CouncilStationVisualStep + 1) % 4;
 const float Yaw = bMerchant ? 0.f
     : static_cast<float>((CouncilStationVisualStep % 2) * 18 - 9);
 CouncilWorkMarker->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
 CouncilWorkMarker->SetVisibility(true);
}

bool AMoonfallWorldNPC::ConfigureCouncilWorkRoute(const FVector& EndOffset) {
 bCouncilWorkRouteValid = false;
 if (!GetWorld() || SettlementId != FName(TEXT("Crossroads"))
     || !(SocialRole == FName(TEXT("Guard")) || SocialRole == FName(TEXT("Scout")))
     || EndOffset.ContainsNaN() || EndOffset.SizeSquared2D() > FMath::Square(140.f)
     || FMath::Abs(EndOffset.Z) > 1.f) return false;
 const FVector Home = GetActorLocation();
 const FVector Destination = Home + EndOffset;
 if (Home.ContainsNaN() || Destination.ContainsNaN()) return false;
 // Require a genuine, short NavMesh path. If the project has no baked NavMesh,
 // the resident stays put rather than walking through walls or off terrain.
 UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 if (!Nav) return false;
 FNavLocation NavHome;
 FNavLocation NavEnd;
 if (!Nav->ProjectPointToNavigation(Home, NavHome, FVector(70.f,70.f,150.f))
     || !Nav->ProjectPointToNavigation(Destination, NavEnd, FVector(70.f,70.f,150.f))
     || FVector::DistSquared2D(NavHome.Location, Home) > FMath::Square(80.f)
     || FVector::DistSquared2D(NavEnd.Location, Destination) > FMath::Square(80.f))
  return false;
 UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
     GetWorld(), NavHome.Location, NavEnd.Location, this);
 if (!IsValid(Path) || !Path->IsValid() || Path->IsPartial()
     || Path->PathPoints.Num() != 2 || Path->GetPathLength() > 180.f) return false;
 // Movement interpolates a straight segment, not each NavMesh corner. Reject
 // routes that would need to bend around an obstacle or a building.
 FCollisionQueryParams Clearance(SCENE_QUERY_STAT(MoonfallCouncilRoute), false, this);
 FHitResult Obstacle;
 const FVector EyeOffset(0.f, 0.f, 55.f);
 if (GetWorld()->LineTraceSingleByChannel(Obstacle,
      Home + EyeOffset, Destination + EyeOffset, ECC_Visibility, Clearance))
  return false;
 // Avoid arbitrary offsets/height changes after route validation.
 CouncilWorkHome = Home;
 CouncilWorkDestination = Destination;
 bCouncilWorkReturning = false;
 CouncilWorkHoldSeconds = 0.f;
 bCouncilWorkAtPost = false;
 CouncilWorkVisualPhase = 0.f;
 if (IsValid(CouncilWorkMarker)) CouncilWorkMarker->SetVisibility(false);
 bCouncilWorkRouteValid = true;
 return true;
}

void AMoonfallWorldNPC::AdvanceCouncilWorkStep(const AMoonfallHeroPawn* Hero, float StepSeconds) {
 if (!bCouncilWorkRouteValid || !IsValid(Hero) || Hero->CouncilDecisionId < 0
     || Hero->CouncilDecisionId > 2 || Hero->Health <= 0.f
     || !FMath::IsFinite(StepSeconds) || StepSeconds <= 0.f || StepSeconds > 0.25f
     || Hero->bInventoryOpen || Hero->bOptionsOpen || Hero->bAtlasOpen
     || (Hero->bResidentContextOpen && Hero->FocusedResident.Get() == this)) {
  if (IsValid(CouncilWorkMarker)) CouncilWorkMarker->SetVisibility(false);
  return;
 }
 // Arrival pauses are advanced only by this shared, bounded 0.2s timer.
 // While paused, no actor transform updates or collision queries are needed.
 // Do not animate through nearby conversations or reveal work props while
 // the player is watching an NPC stand still. Resume the pause afterwards.
 const FVector PlayerForPause = Hero->GetActorLocation();
 if (PlayerForPause.ContainsNaN()
     || FVector::DistSquared2D(GetActorLocation(), PlayerForPause) < FMath::Square(300.f)) {
  if (IsValid(CouncilWorkMarker)) CouncilWorkMarker->SetVisibility(false);
  return;
 }
 if (CouncilWorkHoldSeconds > 0.f) {
  CouncilWorkHoldSeconds = FMath::Max(0.f, CouncilWorkHoldSeconds - StepSeconds);
  const bool bWorking = bCouncilWorkAtPost && CouncilWorkHoldSeconds > 0.f;
  if (IsValid(CouncilWorkMarker)) {
   CouncilWorkMarker->SetVisibility(bWorking);
   if (bWorking) {
    // A small bobbing/rotating work glyph, animated by the existing 5Hz
    // arena timer, never a per-NPC Tick or skeletal mesh replacement.
    CouncilWorkVisualPhase = FMath::Fmod(
        CouncilWorkVisualPhase + StepSeconds * 2.f, 2.f * PI);
    const bool bScout = SocialRole == FName(TEXT("Scout"));
    // Scouts sweep a low compass marker side-to-side, guards keep a tall
    // rotating watch beacon. Both use existing primitive mesh geometry.
    CouncilWorkMarker->SetRelativeScale3D(bScout
        ? FVector(0.18f, 0.18f, 0.06f)
        : FVector(0.10f, 0.10f, 0.22f));
    CouncilWorkMarker->SetRelativeLocation(bScout
        ? FVector(50.f, 10.f * FMath::Sin(CouncilWorkVisualPhase), 80.f)
        : FVector(50.f, 0.f, 85.f + 8.f * FMath::Sin(CouncilWorkVisualPhase)));
    CouncilWorkMarker->SetRelativeRotation(
        FRotator(0.f, bScout ? 45.f * FMath::Sin(CouncilWorkVisualPhase)
        : FMath::RadiansToDegrees(CouncilWorkVisualPhase), 0.f));
   }
  }
  if (CouncilWorkHoldSeconds <= 0.f) {
   bCouncilWorkReturning = !bCouncilWorkReturning;
   bCouncilWorkAtPost = false;
  }
  return;
 }
 if (IsValid(CouncilWorkMarker)) CouncilWorkMarker->SetVisibility(false);
 const FVector Current = GetActorLocation();
 const FVector Player = Hero->GetActorLocation();
 if (Current.ContainsNaN() || Player.ContainsNaN()
     || FVector::DistSquared2D(Current, Player) < FMath::Square(300.f)) return;
 const FVector Target = bCouncilWorkReturning ? CouncilWorkHome : CouncilWorkDestination;
 const FVector ToTarget = Target - Current;
 if (ToTarget.SizeSquared2D() < FMath::Square(8.f)) {
  bCouncilWorkAtPost = !bCouncilWorkReturning;
  CouncilWorkHoldSeconds = bCouncilWorkAtPost ? 5.f : 2.f;
  return;
 }
 const float Distance = FMath::Sqrt(ToTarget.SizeSquared2D());
 const float Travel = FMath::Min(Distance, 55.f * StepSeconds);
 const FVector Next = Current + FVector(ToTarget.X, ToTarget.Y, 0.f) * (Travel / Distance);
 // Never leave the initial 140-unit authored workplace corridor.
 if (FVector::DistSquared2D(Next, CouncilWorkHome) > FMath::Square(145.f)) {
  bCouncilWorkRouteValid = false;
  return;
 }
 // Root mesh is query-only for player dialogue. Explicitly test the
 // movement corridor too, so a swept move cannot phase through scenery.
 FCollisionQueryParams Clearance(SCENE_QUERY_STAT(MoonfallCouncilStep), false, this);
 FHitResult Obstacle;
 const FVector EyeOffset(0.f, 0.f, 55.f);
 if (GetWorld()->LineTraceSingleByChannel(Obstacle,
     Current + EyeOffset, Next + EyeOffset, ECC_Visibility, Clearance)) {
  bCouncilWorkRouteValid = false;
  return;
 }
 FHitResult Blocked;
 if (!SetActorLocation(Next, true, &Blocked, ETeleportType::None) || Blocked.bBlockingHit)
  bCouncilWorkRouteValid = false;
}

FString AMoonfallWorldNPC::GetCouncilWorkPhase() const {
 if (!bCouncilWorkRouteValid) return TEXT("At assigned station");
 if (CouncilWorkHoldSeconds > 0.f)
  return bCouncilWorkAtPost ? TEXT("Working at post") : TEXT("Resting at home");
 return bCouncilWorkReturning ? TEXT("Returning to station") : TEXT("Walking to work");
}

FString AMoonfallWorldNPC::GetCouncilAssignment(const AMoonfallHeroPawn* Hero) const {
 // Crossroads is the only authored settlement with Council decorations.
 // Other NPCs retain their ordinary schedule until their regions are built.
 if (!IsValid(Hero) || SettlementId != FName(TEXT("Crossroads"))
     || Hero->CouncilDecisionId < 0 || Hero->CouncilDecisionId > 2)
  return GetCurrentActivity();
 const int32 Policy = Hero->CouncilDecisionId;
 if (Policy == 0) {
  if (SocialRole == FName(TEXT("Guard"))) return TEXT("Protecting Residents");
  if (SocialRole == FName(TEXT("Scout"))) return TEXT("Escorting Travelers");
  if (SocialRole == FName(TEXT("Elder"))) return TEXT("Organizing Aid");
  if (SocialRole == FName(TEXT("Merchant"))) return TEXT("Supplying Families");
  return TEXT("Supporting Neighbors");
 }
 if (Policy == 1) {
  if (SocialRole == FName(TEXT("Tinker"))) return TEXT("Repairing Machinery");
  if (SocialRole == FName(TEXT("Merchant"))) return TEXT("Stocking Building Supplies");
  if (SocialRole == FName(TEXT("Guard"))) return TEXT("Watching Worksites");
  if (SocialRole == FName(TEXT("Scholar"))) return TEXT("Planning Reconstruction");
  return TEXT("Helping Rebuild");
 }
 if (SocialRole == FName(TEXT("Guard"))) return TEXT("Monitoring the Borders");
 if (SocialRole == FName(TEXT("Scout"))) return TEXT("Surveying Outer Roads");
 if (SocialRole == FName(TEXT("Tinker"))) return TEXT("Maintaining Defenses");
 if (SocialRole == FName(TEXT("Elder"))) return TEXT("Coordinating Patrols");
 return TEXT("Preparing Supplies");
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
     && GetCurrentActivity() == TEXT("Restocking"))
  Reaction = FText::FromString(TEXT("The stall is closed while I restock."));
 else if (Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Scout"))
     && GetCurrentActivity() == TEXT("Reporting"))
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
 if (Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 0)
  Reaction = FText::FromString(TEXT("I have a task for you. Select Accept in our conversation."));
 else if (Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 1)
  Reaction = FText::FromString(FString::Printf(
    TEXT("Bring me %d salvage. Select Turn In when ready."), GetContractSalvageCost()));
 else if (Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 2)
  Reaction = FText::FromString(TEXT("Our people remember your help. Thank you."));
 // Personal recognition after completed contracts; no trust farming.
 if (Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 2) {
  const FString Rank = Hero->GetResidentRelationshipRank(ResidentId);
  if (Rank == TEXT("Devoted"))
   Reaction = FText::FromString(TEXT("You have my lasting loyalty, whatever road you choose."));
  else if (Rank == TEXT("Trusted"))
   Reaction = FText::FromString(TEXT("Your deeds earned my trust. I will remember them."));
 }
 // Residents acknowledge the single saved Council policy after their
 // contract is complete; no free rewards, spawns or new faction mutations.
 if (Disposition != TEXT("Hostile") && Hero->GetResidentQuestState(ResidentId) == 2
     && Hero->CouncilDecisionId >= 0) {
  switch (Hero->CouncilDecisionId) {
  case 0:
   Reaction = FText::FromString(TEXT("The council chose to protect our people. I remember that promise."));
   break;
  case 1:
   Reaction = FText::FromString(TEXT("The council funded rebuilding. There is work ahead for us."));
   break;
  case 2:
   Reaction = FText::FromString(TEXT("The council ordered stronger borders. The roads may grow safer."));
   break;
  default: break;
  }
 }
 // Fable-inspired moral reputation is independent of faction reputation.
 // Mention sustained deeds without changing trust tiers, quests or dialogue AI.
 if (Disposition != TEXT("Hostile") && Hero->MoralAlignment >= 60)
  Reaction = FText::FromString(Reaction.ToString()
      + TEXT(" People have heard of your kindness."));
 else if (Disposition != TEXT("Hostile") && Hero->MoralAlignment <= -60)
  Reaction = FText::FromString(Reaction.ToString()
      + TEXT(" People fear what you might do next."));
 LastSpokenLine = Reaction;
 OnDialogueStarted(Hero, Reaction);
 return true;
}

bool AMoonfallWorldNPC::OfferResidentQuest(AMoonfallHeroPawn* Hero) {
 if (ResidentId.IsNone() || !CanHeroInteract(Hero)
     || Hero->GetFactionReputation(FactionId) <= -50) return false;
 return Hero->AcceptResidentQuest(ResidentId);
}
bool AMoonfallWorldNPC::TurnInResidentQuest(AMoonfallHeroPawn* Hero) {
 if (ResidentId.IsNone() || !CanHeroInteract(Hero)
     || Hero->GetFactionReputation(FactionId) <= -50) return false;
 return Hero->CompleteResidentQuest(this);
}
