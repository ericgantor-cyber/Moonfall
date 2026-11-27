#include "MoonfallWorldNPC.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallUnrealGameMode.h"
#include "MoonfallRaceCatalog.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MoonfallLivingSimulation.h"
#include "MoonfallNpcDecisionPolicy.h"
#include "MoonfallLivingWorldSubsystem.h"
#include "MoonfallLivingWorldPolicy.h"

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
 if (!bAmbientVisitor) {
  if (UWorld* World=GetWorld())
   if (AMoonfallUnrealGameMode* Mode=World->GetAuthGameMode<AMoonfallUnrealGameMode>())
    Mode->RegisterLivingResident(this);
 }
}
void AMoonfallWorldNPC::EndPlay(const EEndPlayReason::Type EndPlayReason) {
 if (UWorld* World=GetWorld())
  if (AMoonfallUnrealGameMode* Mode=World->GetAuthGameMode<AMoonfallUnrealGameMode>())
   Mode->UnregisterLivingResident(this);
 Super::EndPlay(EndPlayReason);
}
bool AMoonfallWorldNPC::AdvanceFollowerToward(const FVector& Goal,float Now,float StopDistance){
 if(!HasAuthority()||!GetWorld()||Goal.ContainsNaN()||GetActorLocation().ContainsNaN()
     ||!FMath::IsFinite(Now)||!FMath::IsFinite(StopDistance)||StopDistance<0.f)return false;
 const FVector Start=GetActorLocation();
 const double Distance=FVector::Dist2D(Start,Goal);
 const bool bSameFloor=FMath::Abs(Start.Z-Goal.Z)<=150.0;
 if(!FMath::IsFinite(Distance))return false;
 if(Distance<=StopDistance&&bSameFloor)return true;
 // At most one synchronous nav query per second for the active follower.
 if(Now>=NextFollowerPathAt){
  NextFollowerPathAt=Now+1.f;bFollowerWaypointValid=false;
  if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())){
   FNavLocation Projected;
   if(Nav->ProjectPointToNavigation(Goal,Projected)){
    auto* Path=Nav->FindPathToLocationSynchronously(GetWorld(),Start,Projected.Location,this);
    if(IsValid(Path)&&Path->IsValid()&&Path->PathPoints.Num()>1&&!Path->PathPoints[1].ContainsNaN()){
     FollowerWaypoint=Path->PathPoints[1];bFollowerWaypointValid=true;
    }
   }
  }
 }
 const FVector Target=bFollowerWaypointValid?FollowerWaypoint:Goal;
 FVector Step=Target-Start;Step.Z=0;
 if(Step.SizeSquared2D()<25.0){bFollowerWaypointValid=false;return false;}
 const double StepLimit=bSameFloor?FMath::Min(60.0,FMath::Max(0.0,Distance-StopDistance)):60.0;
 const FVector End=Start+Step.GetClampedToMaxSize2D(StepLimit);
 FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallFollowerMove),false,this);
 FHitResult Obstacle;
 if(GetWorld()->LineTraceSingleByChannel(Obstacle,Start+FVector(0,0,55),End+FVector(0,0,55),ECC_Visibility,Params)){
  bFollowerWaypointValid=false;return false;
 }
 FHitResult Ground,OldGround;
 if(!GetWorld()->LineTraceSingleByChannel(Ground,End+FVector(0,0,150),End-FVector(0,0,150),ECC_Visibility,Params)
     ||!GetWorld()->LineTraceSingleByChannel(OldGround,Start+FVector(0,0,150),Start-FVector(0,0,150),ECC_Visibility,Params)
     ||Ground.ImpactNormal.Z<0.65||FMath::Abs(Ground.ImpactPoint.Z-OldGround.ImpactPoint.Z)>35.0)return false;
 FVector Move=End-Start;Move.Z=Ground.ImpactPoint.Z-OldGround.ImpactPoint.Z;
 AddActorWorldOffset(Move,true);
 if(!Move.IsNearlyZero())SetActorRotation(FRotator(0.f,Move.Rotation().Yaw,0.f));
 return FVector::Dist2D(GetActorLocation(),Goal)<=StopDistance&&FMath::Abs(GetActorLocation().Z-Goal.Z)<=150.0;
}
bool AMoonfallWorldNPC::TrySafeFollowerCatchup(const AMoonfallHeroPawn* Hero){
 if(!HasAuthority()||!IsValid(Hero)||Hero->GetWorld()!=GetWorld()||!GetWorld()||bCompanionDowned
    ||!Hero->GetCharacterMovement()||!Hero->GetCharacterMovement()->IsMovingOnGround()
    ||Hero->IsRecentlyInCombatForAmbientAI())return false;
 const FVector Player=Hero->GetActorLocation();
 const FVector Forward=Hero->GetActorForwardVector().GetSafeNormal2D();
 if(Player.ContainsNaN()||Forward.ContainsNaN()||Forward.IsNearlyZero())return false;
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 if(!Nav)return false;
 FVector Origin,Extent;GetActorBounds(true,Origin,Extent);
 if(Origin.ContainsNaN()||Extent.ContainsNaN()||Extent.X<=0||Extent.Y<=0||Extent.Z<=0||Extent.GetMax()>200)return false;
 const FVector Offset=Origin-GetActorLocation();
 const FVector Side(-Forward.Y,Forward.X,0);
 const FVector Candidates[]={Player-Forward*260.0,Player-Forward*220.0+Side*150.0,Player-Forward*220.0-Side*150.0};
 FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(MoonfallFollowerCatchup),false,this);
 GroundParams.AddIgnoredActor(Hero);
 FCollisionQueryParams OccupancyParams(SCENE_QUERY_STAT(MoonfallFollowerOccupancy),false,this);
 FCollisionObjectQueryParams Objects;
 Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);Objects.AddObjectTypesToQuery(ECC_Pawn);
 for(const FVector& Candidate:Candidates){
  FNavLocation Projected;
  if(!Nav->ProjectPointToNavigation(Candidate,Projected,FVector(100,100,150))||Projected.Location.ContainsNaN()
     ||FVector::DistSquared2D(Projected.Location,Player)>FMath::Square(400.0)
     ||FVector::DistSquared2D(Projected.Location,Player)<FMath::Square(150.0))continue;
  FHitResult Ground;
  if(!GetWorld()->LineTraceSingleByChannel(Ground,Projected.Location+FVector(0,0,100),
     Projected.Location-FVector(0,0,100),ECC_Visibility,GroundParams)||Ground.ImpactNormal.Z<0.7
     ||FMath::Abs(Ground.ImpactPoint.Z-Player.Z)>180.0)continue;
  const FVector Center=Ground.ImpactPoint+FVector(0,0,Extent.Z+8.0);
  const FVector Destination=Center-Offset;
  if(GetWorld()->OverlapAnyTestByObjectType(Center,FQuat::Identity,Objects,
       FCollisionShape::MakeBox(Extent+FVector(5,5,2)),OccupancyParams))continue;
  FHitResult Sight;
  if(GetWorld()->LineTraceSingleByChannel(Sight,Center,Player,ECC_Visibility,GroundParams))continue;
  if(!TeleportTo(Destination,GetActorRotation(),false,false))continue;
  bFollowerWaypointValid=false;NextFollowerPathAt=-1.f;LivingIntent=TEXT("Rejoining");
  return true;
 }
 return false;
}
int32 AMoonfallWorldNPC::GetLocalRegionalDanger() const
{
 if(bAmbientVisitor||!bUsePersonalityRoutines||!GetWorld()||
    AuthoredHomeLocation.ContainsNaN()||AuthoredHomeLocation.Y>-1800.f||
    SettlementId==FName(TEXT("Firstlight")))return -1;
 const auto* Mode=GetWorld()->GetAuthGameMode<AMoonfallUnrealGameMode>();
 return IsValid(Mode)?Mode->GetRegionalDanger(
  MoonfallLivingSimulation::RegionFromWorldX(AuthoredHomeLocation.X)):-1;
}
void AMoonfallWorldNPC::WitnessNearbyDanger(float Now) {
 if (FMath::IsFinite(Now) && Now >= 0.f) LastDangerAt = Now;
}
void AMoonfallWorldNPC::AdvanceLivingRoutine(float Now, const FVector& HeroLocation,
                                               bool bInConversation, int32 HeroBond, bool bAllowLocalGuide, bool bHoldCompanion) {
 if (bAmbientVisitor || !GetWorld() || !FMath::IsFinite(Now)
     || AuthoredHomeLocation.ContainsNaN() || HeroLocation.ContainsNaN()
     || GetActorLocation().ContainsNaN())
  return;
 // Compact authored-home offsets: no teleport, path solve or NPC Tick.
 const int Shift=(MoonfallLivingSimulation::Phase(Now,180.f)
     + static_cast<int>(GetTypeHash(ResidentId)%4u))%4;
 const int Region=MoonfallLivingSimulation::RegionFromWorldX(GetActorLocation().X);
 const auto Weather=MoonfallLivingSimulation::WeatherFor(Region,
      MoonfallLivingSimulation::Phase(Now,360.f));
 const bool bGuard=SocialRole==FName(TEXT("Guard"));
 const auto Alert=MoonfallLivingSimulation::ReactionFor(
      bGuard?MoonfallLivingSimulation::ERole::Guard
            :MoonfallLivingSimulation::ERole::Citizen,Now-LastDangerAt);
 // Guards hold their assigned watch points during danger. Residents
 // seek their authored shelter/home rather than wandering into combat.
 const bool bSheltering=(MoonfallLivingSimulation::Severe(Weather) && !bGuard)
      || Alert==MoonfallLivingSimulation::EAlertReaction::SeekShelter;
 const bool bHoldPost=Alert==MoonfallLivingSimulation::EAlertReaction::HoldPost;
 using RegionRoutine=MoonfallLivingSimulation::ERegionalRoutine;
 using RegionRole=MoonfallLivingSimulation::ERole;
 const RegionRole Role=bGuard?RegionRole::Guard:SocialRole==FName(TEXT("Merchant"))?RegionRole::Merchant:
     SocialRole==FName(TEXT("Scout"))?RegionRole::Scout:RegionRole::Citizen;
 const auto Regional=MoonfallLivingSimulation::RegionalRoutine(Role,GetLocalRegionalDanger(),
     !bInConversation&&!bSheltering&&!bHoldPost);
 const bool bRegionalHome=Regional==RegionRoutine::ReturnHome;
 const bool bRegionalWatch=Regional==RegionRoutine::GuardWatch;
 const bool bNearHero=FVector::DistSquared2D(HeroLocation,GetActorLocation())
     < FMath::Square(420.f);
 bool bRoutineHome=false;
 LivingIntent.Reset();
 bReviewingCombatReport=false;
 FMoonfallWorldRumor GuardReport;
 if (bUsePersonalityRoutines && bGuard && !bInConversation && Alert==MoonfallLivingSimulation::EAlertReaction::Normal) {
  if (const auto* Society=GetWorld()->GetSubsystem<UMoonfallLivingWorldSubsystem>()) {
   bReviewingCombatReport=Society->GetNearbyCombatReport(ResidentId,GetActorLocation(),GuardReport);
  }
 }
 if (bUsePersonalityRoutines) {
  using namespace MoonfallNpcPolicy;
  Input Context;
  // Four existing 180-second shifts form one simulated day. Start at dawn.
  Context.hour=FMath::Fmod(FMath::Fmod(FMath::Max(Now,0.f),720.f)/30.f+6.f,24.f);
  const bool bMerchant=SocialRole==FName(TEXT("Merchant"));
  const bool bScout=SocialRole==FName(TEXT("Scout"));
  // Stable per-resident variation, independent of tick/frame or spawn order.
  const float Temperament=static_cast<float>(GetTypeHash(ResidentId)%101u)/100.f;
  Context.courage=bGuard?.95f:(bScout?.7f:.2f+Temperament*.4f);
  Context.aggression=bGuard?.7f:.1f;
  Context.sociability=bMerchant?.8f:.3f+Temperament*.5f;
  Context.diligence=bMerchant?.9f:.4f+Temperament*.5f;
  Context.canDefend=bGuard;
  Context.danger=Alert==MoonfallLivingSimulation::EAlertReaction::Normal?0.f:.7f;
  Context.storm=MoonfallLivingSimulation::Severe(Weather)&&!bGuard;
  Context.sheltered=false;
  Context.playerNearby=bNearHero;
  Context.trust=FMath::Clamp(HeroBond/100.f,-1.f,1.f);
  const Action Intent=Decide(Context);
  // Keep the existing quest roles and guard shift rotation intact. Intent
  // names report decisions, not unimplemented activities or combat.
  if (bInConversation) LivingIntent=TEXT("Conversing");
  else if (bHoldPost) LivingIntent=TEXT("On Alert");
  else if (bSheltering) LivingIntent=TEXT("Seeking Shelter");
  else if (bGuard) LivingIntent=bReviewingCombatReport?TEXT("Reviewing Reports"):bRegionalWatch?TEXT("Regional Watch"):TEXT("On Duty");
  else if (bRegionalHome) { LivingIntent=TEXT("Staying Near Home"); bRoutineHome=true; }
  else if (Regional==RegionRoutine::CautiousWork) LivingIntent=TEXT("Cautious Work");
  else switch(Intent) {
   case Action::Sleep: LivingIntent=TEXT("Resting"); bRoutineHome=true; break;
   case Action::Eat: LivingIntent=TEXT("Meal Break"); bRoutineHome=true; break;
   case Action::Talk: LivingIntent=TEXT("Ready to Talk"); break;
   case Action::Socialize: LivingIntent=TEXT("Socializing"); break;
   case Action::Work: LivingIntent=TEXT("Working"); break;
   default: LivingIntent=TEXT("Idle"); break;
  }
  // Merchants/scouts retain their authored shifts so quest greetings do not
  // change. Ordinary residents return home when resting or on a meal break.
  if (bMerchant || bScout || bGuard || bInConversation) bRoutineHome=false;
 }
 if(bHoldCompanion&&!bInConversation&&!bSheltering&&!bHoldPost
     &&Alert==MoonfallLivingSimulation::EAlertReaction::Normal){
  LivingIntent=TEXT("Holding Position");return;
 }
 const bool bGuide=MoonfallWorldPolicy::CanLocalGuide(HeroBond,0,
     FVector::DistSquared(HeroLocation,AuthoredHomeLocation),
     FVector::DistSquared(HeroLocation,GetActorLocation()),
     !bInConversation&&!bSheltering&&!bHoldPost&&Alert==MoonfallLivingSimulation::EAlertReaction::Normal,
     bAllowLocalGuide&&MoonfallLivingSimulation::RegionalRoutineAllowsGuide(Regional)&&bUsePersonalityRoutines&&bAllowLocalScoutGuidance&&SocialRole==FName(TEXT("Scout")));
 if (MoonfallLivingSimulation::PauseResidentMotion(
      bInConversation,bNearHero,bSheltering||bRegionalHome,bHoldPost||bRegionalWatch) && !bGuide) return;
 float Side=(bSheltering || bHoldPost || bRoutineHome)?0.f:(Shift==0?-65.f:(Shift==1?65.f:0.f));
 float Front=(bSheltering || bHoldPost || bRoutineHome)?0.f:(Shift==2?45.f:(Shift==3?-45.f:0.f));
 if(bRegionalWatch){Side=0.f;Front=0.f;}
 // Credible reports bias a guard toward one of the original four watch
 // offsets. This does not pathfind to the reported incident or accuse anyone.
 if (bGuard && bUsePersonalityRoutines && bReviewingCombatReport && !bHoldPost
     && !bSheltering && !bInConversation && !bNearHero && !MoonfallLivingSimulation::Severe(Weather)) {
  const FVector Direction=GuardReport.Location-AuthoredHomeLocation;
  using MoonfallWorldPolicy::WatchPoint;
  switch (MoonfallWorldPolicy::GuardWatchPoint(Direction.X,Direction.Y,GuardReport.Confidence,true)) {
   case WatchPoint::West: Side=-65.f; Front=0.f; break;
   case WatchPoint::East: Side=65.f; Front=0.f; break;
   case WatchPoint::North: Side=0.f; Front=45.f; break;
   case WatchPoint::South: Side=0.f; Front=-45.f; break;
   default: break;
  }
 }
 FVector Target=AuthoredHomeLocation+FVector(Side,Front,0.f);
 if(bGuide){
  FVector Direction=HeroLocation-AuthoredHomeLocation;Direction.Z=0.f;
  const double Distance=Direction.Size2D();
  Target=AuthoredHomeLocation+Direction.GetSafeNormal2D()*FMath::Clamp(Distance-240.0,0.0,180.0);
  LivingIntent=TEXT("Guiding Nearby");
 }
 FVector Step=Target-GetActorLocation();
 Step.Z=0.f;
 if (Step.SizeSquared2D() > FMath::Square(1.f)){
  if(bGuide||bRegionalHome||bRegionalWatch){
   // Guide and regional return steps must stay on clear, level ground.
   const FVector Start=GetActorLocation();
   const FVector End=Start+Step.GetClampedToMaxSize2D(32.f);
   FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallLocalGuide),false,this);
   FHitResult Hit;
   if(GetWorld()->LineTraceSingleByChannel(Hit,Start+FVector(0,0,55),End+FVector(0,0,55),ECC_Visibility,Params))return;
   if(!GetWorld()->LineTraceSingleByChannel(Hit,End+FVector(0,0,120),End-FVector(0,0,120),ECC_Visibility,Params)
       ||Hit.ImpactNormal.Z<0.7)return;
   FHitResult ExistingGround;
   if(!GetWorld()->LineTraceSingleByChannel(ExistingGround,Start+FVector(0,0,120),Start-FVector(0,0,120),ECC_Visibility,Params)
       ||FMath::Abs(Hit.ImpactPoint.Z-ExistingGround.ImpactPoint.Z)>10.0)return;
  }
  const float RoutineStep=MoonfallLivingSimulation::RegionalRoutineStepLimit(Regional);
  AddActorWorldOffset(Step.GetClampedToMaxSize2D(RoutineStep),true);
 }
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
 // Match the HUD and dialogue Tick: invalid range/health must not open a
 // hidden conversation, and only actors in the same live world may interact.
 if (!IsValid(Hero) || !GetWorld() || Hero->GetWorld()!=GetWorld()
     || IsActorBeingDestroyed() || Hero->IsActorBeingDestroyed()
     || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f
     || Hero->bInventoryOpen || Hero->bOptionsOpen || Hero->bAtlasOpen
     || Hero->bTalentWebOpen || !FMath::IsFinite(InteractRange)) return false;
 const FVector From = Hero->GetActorLocation();
 const FVector To = GetActorLocation();
 const float Range = FMath::Clamp(InteractRange, 50.f, 400.f);
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
 const auto Alert=MoonfallLivingSimulation::ReactionFor(Role,Now-LastDangerAt);
 if(Role==ERole::Scout&&(LivingIntent==TEXT("Following")||LivingIntent==TEXT("Holding Position")||LivingIntent==TEXT("Passive")||LivingIntent==TEXT("Returning Home")||LivingIntent==TEXT("Moving to order")||LivingIntent==TEXT("Intercepting")||LivingIntent==TEXT("Flanking")||LivingIntent==TEXT("Withdrawing")||LivingIntent==TEXT("Downed")))return LivingIntent;
 if(Alert==MoonfallLivingSimulation::EAlertReaction::HoldPost) return TEXT("On Alert");
 if(Alert==MoonfallLivingSimulation::EAlertReaction::SeekShelter) return TEXT("Seeking Shelter");
 const bool bRegionalIntent=LivingIntent==TEXT("Regional Watch")||
     LivingIntent==TEXT("Staying Near Home")||LivingIntent==TEXT("Cautious Work");
 const auto Regional=MoonfallLivingSimulation::RegionalRoutine(Role,GetLocalRegionalDanger(),
     !MoonfallLivingSimulation::Severe(Weather)||Role==ERole::Guard);
 if(bRegionalIntent&&Regional!=MoonfallLivingSimulation::ERegionalRoutine::Normal)return LivingIntent;
 // Only ordinary residents expose the new activity in the existing HUD.
 // Fresh danger/weather and authored specialist roles take precedence.
 if (!bAmbientVisitor && bUsePersonalityRoutines && Role==ERole::Guard && bReviewingCombatReport) {
  if (const auto* Society=GetWorld()?GetWorld()->GetSubsystem<UMoonfallLivingWorldSubsystem>():nullptr) {
   FMoonfallWorldRumor Report;
   if (Society->GetNearbyCombatReport(ResidentId,GetActorLocation(),Report)) return TEXT("Reviewing Reports");
  }
 }
 if (!bAmbientVisitor && bUsePersonalityRoutines && Role==ERole::Scout
     && !MoonfallLivingSimulation::Severe(Weather) && (LivingIntent==TEXT("Guiding Nearby")||LivingIntent==TEXT("Holding Position"))) return LivingIntent;
 if (!bAmbientVisitor && bUsePersonalityRoutines && Role==ERole::Citizen
     && !MoonfallLivingSimulation::Severe(Weather) && !LivingIntent.IsEmpty() && !bRegionalIntent
     && LivingIntent!=TEXT("Seeking Shelter"))
  return LivingIntent;
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
EMoonfallNpcMood AMoonfallWorldNPC::GetEmotionalState(const AMoonfallHeroPawn* Hero) const {
 const float Now=GetWorld()?GetWorld()->GetTimeSeconds():-1.f;
 const bool bDanger=FMath::IsFinite(Now)&&Now>=LastDangerAt&&Now-LastDangerAt<85.f;
 bool bReport=false;
 if(!bAmbientVisitor&&bUsePersonalityRoutines&&GetWorld()){
  if(const auto* Society=GetWorld()->GetSubsystem<UMoonfallLivingWorldSubsystem>()){
   FMoonfallWorldRumor Report;
   bReport=Society->GetNearbyCombatReport(ResidentId,GetActorLocation(),Report);
  }
 }
 const auto ResolvedMood=MoonfallWorldPolicy::ResolveMood(GetBondWithHero(Hero),
     IsValid(Hero)?Hero->GetFactionReputation(FactionId):0,bDanger,bReport,SocialRole==FName(TEXT("Guard")));
 using MoonfallWorldPolicy::Mood;
 switch(ResolvedMood){
  case Mood::Uneasy:return EMoonfallNpcMood::Uneasy;
  case Mood::Afraid:return EMoonfallNpcMood::Afraid;
  case Mood::Vigilant:return EMoonfallNpcMood::Vigilant;
  case Mood::Wary:return EMoonfallNpcMood::Wary;
  case Mood::Resentful:return EMoonfallNpcMood::Resentful;
  case Mood::Warm:return EMoonfallNpcMood::Warm;
  case Mood::Loyal:return EMoonfallNpcMood::Loyal;
  default:return EMoonfallNpcMood::Calm;
 }
}
bool AMoonfallWorldNPC::TryTalk(AMoonfallHeroPawn* Hero) {
 if (!CanHeroInteract(Hero)) return false;
 InteractionCount = FMath::Min(InteractionCount + 1, 1000000);
 LastConversationMood=GetEmotionalState(Hero);
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
 if (!bAmbientVisitor && Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Merchant")))
  Reaction = FText::FromString(TEXT("Browse my wares when the market opens."));
 else if (!bAmbientVisitor && Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Guard")))
  Reaction = FText::FromString(TEXT("Keep the peace inside our walls."));
 // Activity is determined only when interacted with; inhabitants never spam
 // navigation or world perception. Trust/hostility overrides daily routine.
 if (!bAmbientVisitor && Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Merchant"))
     && (GetCurrentActivity() == TEXT("Resting")
         || GetCurrentActivity() == TEXT("Sheltering")))
  Reaction = FText::FromString(TEXT("The stall is closed while I restock."));
 else if (!bAmbientVisitor && Disposition == TEXT("Neutral") && !SharesRaceWith(Hero) && SocialRole == FName(TEXT("Scout"))
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
 // Modest social warmth or personal caution changes otherwise neutral
 // greetings, but never overrides authored contracts, explicit hostility,
 // ancestry recognition or strong bond dialogue.
 if(!bAmbientVisitor && bUsePersonalityRoutines && Disposition==TEXT("Neutral")
     && !SharesRaceWith(Hero) && Bond>-60 && Bond<60 && SocialRole==FName(TEXT("Citizen"))
     && Greeting.EqualTo(FText::FromString(TEXT("The road stretches on. Need directions?")))){
  if(LastConversationMood==EMoonfallNpcMood::Warm)
   Reaction=FText::FromString(TEXT("Good to see you again. How fares the road?"));
  else if(LastConversationMood==EMoonfallNpcMood::Wary)
   Reaction=FText::FromString(TEXT("I would rather keep our distance for now."));
 }
 // Accepted one-time contracts have useful, contextual status.
 if (!bAmbientVisitor && Disposition != TEXT("Hostile")
     && Hero->SupportsResidentContract(ResidentId)
     && Hero->GetResidentQuestState(ResidentId) == 0) {
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
 else if (!bAmbientVisitor && Disposition!=TEXT("Hostile") && bUsePersonalityRoutines) {
  if (const auto* Society=GetWorld()?GetWorld()->GetSubsystem<UMoonfallLivingWorldSubsystem>():nullptr) {
   FMoonfallWorldRumor Report;
   if (Society->GetNearbyCombatReport(ResidentId,GetActorLocation(),Report)) {
    const TCHAR* Warning=SocialRole==FName(TEXT("Guard"))
        ? TEXT(" We have reports of fighting nearby. Nothing is confirmed yet.")
        : TEXT(" Someone mentioned fighting nearby. I cannot say for certain.");
    Reaction=FText::FromString(Reaction.ToString()+Warning);
   }
  }
 }
 if(!bAmbientVisitor&&Disposition!=TEXT("Hostile")&&GetLocalRegionalDanger()>=65)
  Reaction=FText::FromString(Reaction.ToString()+TEXT(" The province is on alert. We are keeping close to our posts."));
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
