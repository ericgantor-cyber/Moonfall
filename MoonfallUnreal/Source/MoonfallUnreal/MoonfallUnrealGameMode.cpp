#include "MoonfallUnrealGameMode.h"
#include "Misc/DateTime.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallCombatHUD.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallEnemy.h"
#include "MoonfallLivingSimulation.h"
#include "MoonfallLivingWorldSubsystem.h"
#include "MoonfallLivingWorldPolicy.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AMoonfallUnrealGameMode::AMoonfallUnrealGameMode()
{
 DefaultPawnClass = AMoonfallHeroPawn::StaticClass();
 HUDClass = AMoonfallCombatHUD::StaticClass();
}

bool AMoonfallUnrealGameMode::GetDungeonEntranceTransform(
 int32 ZoneIndex, int32 DungeonIndex, FTransform& OutTransform) const
{
 if (ZoneIndex < 0 || ZoneIndex >= DungeonZoneCount
     || DungeonIndex < 0 || DungeonIndex >= DungeonsPerZone) return false;
 const int32 Slot = ZoneIndex * DungeonsPerZone + DungeonIndex;
 // Thirty parallel lanes grouped into 6 contiguous regions of 5 dungeons.
 // Each 900-unit-wide layout has a 450-unit gap to its neighbors. Every
 // entrance connects to the same Y=-2500 public road, never a loading screen.
 const float X = (static_cast<float>(Slot) - 14.5f) * DungeonLaneSpacing;
 OutTransform = FTransform(FRotator(0.f, -90.f, 0.f),
   FVector(X, -3100.f, 0.f));
 return true;
}

FString AMoonfallUnrealGameMode::GetNearbyAmbientEventHint(
 const FVector& HeroLocation) const {
 const AActor* Active = ActiveAmbientEncounter.Get();
 if (!IsValid(Active) || Active->IsActorBeingDestroyed()
     || HeroLocation.ContainsNaN() || Active->GetActorLocation().ContainsNaN()
     || FVector::DistSquared2D(HeroLocation,Active->GetActorLocation())
         > FMath::Square(1250.f)) return FString();
 if (const AMoonfallWorldNPC* Visitor = Cast<AMoonfallWorldNPC>(Active)) {
  if (!Visitor->bAmbientVisitor) return FString();
  return FString::Printf(TEXT("ROAD EVENT: %s | APPROACH TO TALK"),
      *Visitor->DisplayName.ToString());
 }
 if (Active->ActorHasTag(FName(TEXT("MoonfallAmbientEncounter"))))
  return TEXT("ROAD EVENT: HOSTILE CREATURE | DODGE OR LEAVE");
 return FString();
}

void AMoonfallUnrealGameMode::RegisterLivingResident(AMoonfallWorldNPC* Resident) {
 if (!IsValid(Resident) || Resident->IsActorBeingDestroyed()
     || Resident->bAmbientVisitor) return;
 // Clean invalid references before checking the bounded cache budget.
 LivingResidents.RemoveAll([](const TWeakObjectPtr<AMoonfallWorldNPC>& Ref) {
  return !Ref.IsValid() || Ref->IsActorBeingDestroyed();
 });
 if (LivingResidents.ContainsByPredicate(
       [Resident](const TWeakObjectPtr<AMoonfallWorldNPC>& Ref) {
        return Ref.Get()==Resident;
       })) return;
 if (LivingResidents.Num() >= MoonfallLivingSimulation::ResidentRegistryBudget) return;
 LivingResidents.Add(Resident);
}

AMoonfallWorldNPC* AMoonfallUnrealGameMode::GetLocalCompanion() const { return LocalCompanion.Get(); }
const AMoonfallWorldNPC* AMoonfallUnrealGameMode::GetLocalCompanionForHero(const AMoonfallHeroPawn* Hero) const {
 // A matching resident ID alone is not ownership during pawn replacement.
 if(!IsLocalCompanionOwner(Hero)||Hero->Health<=0.f||!Hero->IsCompanionPartyRestoreReady())return nullptr;
 const auto* Scout=LocalCompanion.Get();
 return IsValid(Scout)&&!Scout->IsActorBeingDestroyed()&&Scout->GetWorld()==GetWorld()
     &&Scout->ResidentId==Hero->GetCompanionParty().ResidentId?Scout:nullptr;
}

bool AMoonfallUnrealGameMode::IsLocalCompanionOwner(const AMoonfallHeroPawn* Hero) const {
 return HasAuthority()&&IsValid(Hero)&&Hero==CompanionOwner.Get()&&Hero->GetWorld()==GetWorld()
     &&!Hero->IsActorBeingDestroyed()&&FMath::IsFinite(Hero->Health)&&Hero->IsPlayerControlled();
}
bool AMoonfallUnrealGameMode::RecruitLocalCompanion(AMoonfallHeroPawn* Hero,AMoonfallWorldNPC* Resident){
 if(!HasAuthority()||!IsValid(Hero)||!IsValid(Resident)||Resident->IsActorBeingDestroyed()
     ||Hero->GetWorld()!=GetWorld()||Resident->GetWorld()!=GetWorld()||!Hero->IsPlayerControlled()
     ||Resident->bAmbientVisitor||Resident->ResidentId.IsNone()||!Resident->bUsePersonalityRoutines
     ||!Resident->bAllowLocalScoutGuidance||Resident->SocialRole!=FName(TEXT("Scout"))
     ||Hero->IsRecentlyInCombatForAmbientAI()||!Resident->CanHeroInteract(Hero)
     ||Resident->GetBondWithHero(Hero)<60||Hero->GetFactionReputation(Resident->FactionId)<0
     ||!LivingResidents.ContainsByPredicate([Resident](const auto& Ref){return Ref.Get()==Resident;}))return false;
 if(LocalCompanion.IsValid())return LocalCompanion.Get()==Resident&&IsLocalCompanionOwner(Hero);
 // The party save stores only ResidentId; reject ambiguous authored identities before writing it.
 if(LivingResidents.ContainsByPredicate([Resident](const auto& Ref){
  const auto* Other=Ref.Get();
  return IsValid(Other)&&Other!=Resident&&!Other->IsActorBeingDestroyed()&&Other->ResidentId==Resident->ResidentId;
 }))return false;
 FMoonfallCompanionPartyRecord Next=Hero->GetCompanionParty();
 // A streamed-out companion still occupies the persisted party slot.
 // Never replace that save merely because its weak actor reference is absent.
 if(!Hero->IsCompanionPartyRestoreReady()||(!Next.ResidentId.IsNone()&&Next.ResidentId!=Resident->ResidentId))return false;
 Next.Health=Next.ResidentId==Resident->ResidentId?Next.Health:Resident->CompanionHealth;
 Next.ResidentId=Resident->ResidentId;Next.Stance=static_cast<int32>(EMoonfallCompanionStance::Follow);
 if(!Hero->CommitCompanionParty(Next))return false;
 AttachCompanionParty(Hero,Resident,Next.Stance,Next.Health);
 return true;
}
void AMoonfallUnrealGameMode::AttachCompanionParty(AMoonfallHeroPawn* Hero,AMoonfallWorldNPC* Resident,int32 Stance,float Health){
 if(ReturningCompanion.Get()==Resident)ReturningCompanion.Reset();
 Resident->bFollowerWaypointValid=false;Resident->NextFollowerPathAt=-1.f;
 LocalCompanion=Resident;CompanionOwner=Hero;LocalCompanionStance=static_cast<EMoonfallCompanionStance>(Stance);
 Resident->CompanionHealth=Health;Resident->bCompanionDowned=Health<=0.f;
 bFollowerMoveOrdered=false;FollowerSeparatedAt=-1.f;NextFollowerCatchupAt=-1.f;CompanionThreat.Reset();CompanionThreatAt=-1000.f;CompanionAbilityEndsAt=-1000.f;
 CompanionAbilityReadyUtc=Hero->GetCompanionParty().AbilityReadyUtc;
 const int64 Remaining=FMath::Clamp<int64>(CompanionAbilityReadyUtc-FDateTime::UtcNow().ToUnixTimestamp(),0,30);
 CompanionAbilityReadyAt=GetWorld()->GetTimeSeconds()+static_cast<float>(Remaining);
 GetWorldTimerManager().SetTimer(FollowerPulseTimer,this,&AMoonfallUnrealGameMode::PulseFollower,0.2f,true);
 const uint32 Profile=Hero->GetCompanionProfile(Resident->ResidentId);
 LocalCompanionRole=static_cast<EMoonfallCompanionRole>(MoonfallWorldPolicy::EffectiveCompanionRole(
      MoonfallWorldPolicy::CompanionRank(Resident->GetBondWithHero(Hero)),static_cast<int>(Profile>>18)));
 LocalCompanionTraining=Profile&MoonfallWorldPolicy::CompanionTrainingMask;
}
void AMoonfallUnrealGameMode::TryRestoreCompanionParty(AMoonfallHeroPawn* Hero){
 if(!HasAuthority()||!IsValid(Hero)||!Hero->IsPlayerControlled()||!Hero->IsCompanionPartyRestoreReady()
     ||LocalCompanion.IsValid()||Hero->IsRecentlyInCombatForAmbientAI())return;
 const auto& Saved=Hero->GetCompanionParty();
 if(Saved.ResidentId.IsNone())return;
 AMoonfallWorldNPC* Match=nullptr;
 for(const auto& Ref:LivingResidents){
  auto* Resident=Ref.Get();
  if(!IsValid(Resident)||Resident->IsActorBeingDestroyed()||Resident->ResidentId!=Saved.ResidentId)continue;
  if(Match)return; // Ambiguous authored identities must never duplicate followers.
  Match=Resident;
 }
 if(!IsValid(Match)||Match->GetWorld()!=GetWorld()||Match->bAmbientVisitor||!Match->bUsePersonalityRoutines
     ||!Match->bAllowLocalScoutGuidance||Match->SocialRole!=FName(TEXT("Scout"))
     ||Match->GetBondWithHero(Hero)<60||Hero->GetFactionReputation(Match->FactionId)<0)return;
 AttachCompanionParty(Hero,Match,Saved.Stance,Saved.Health);
}
void AMoonfallUnrealGameMode::SyncCompanionVitals(AMoonfallHeroPawn* Hero){
 if(IsLocalCompanionOwner(Hero)&&LocalCompanion.IsValid())
  Hero->UpdateCompanionVitals(LocalCompanion->ResidentId,LocalCompanion->CompanionHealth,CompanionAbilityReadyUtc);
}
float AMoonfallUnrealGameMode::ConsumeLocalCompanionCover(AMoonfallHeroPawn* Hero,float Damage,const FVector& Source){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||!GetWorld()
     ||LocalCompanionStance!=EMoonfallCompanionStance::Follow
     ||Hero->bInventoryOpen||Hero->bOptionsOpen||Hero->bAtlasOpen||Hero->bTalentWebOpen
     ||Hero->bCharacterStatsOpen||Hero->bSpellBookOpen||Hero->bResidentContextOpen||Source.ContainsNaN())return 0.f;
 auto* Scout=LocalCompanion.Get();
 if(Scout->IsActorBeingDestroyed()||!Scout->bUsePersonalityRoutines||!Scout->bAllowLocalScoutGuidance
     ||Scout->bCompanionDowned||Scout->SocialRole!=FName(TEXT("Scout"))||Scout->GetBondWithHero(Hero)<60
     ||Hero->GetFactionReputation(Scout->FactionId)<0)return 0.f;
 const FVector HeroLocation=Hero->GetActorLocation(),ScoutLocation=Scout->GetActorLocation();
 if(HeroLocation.ContainsNaN()||ScoutLocation.ContainsNaN()||Scout->AuthoredHomeLocation.ContainsNaN()
     ||FVector::DistSquared(HeroLocation,ScoutLocation)>FMath::Square(500.0))return 0.f;
 const float Now=GetWorld()->GetTimeSeconds();
 if(!FMath::IsFinite(Now)||Now<0.f||Now<LastCompanionCoverAt)return 0.f;
 const int Region=MoonfallLivingSimulation::RegionFromWorldX(ScoutLocation.X);
 if(MoonfallLivingSimulation::Severe(MoonfallLivingSimulation::WeatherFor(Region,
       MoonfallLivingSimulation::Phase(Now,360.f))))return 0.f;
 const float Cover=MoonfallWorldPolicy::CompanionCover(Damage,Now-LastCompanionCoverAt,
     FVector::DistSquared(HeroLocation,ScoutLocation),true,
     MoonfallWorldPolicy::CompanionRank(Scout->GetBondWithHero(Hero)),static_cast<int>(LocalCompanionRole),LocalCompanionTraining);
 if(Cover<=0.f)return 0.f;
 // The scout must be between the hero and the reported attack direction.
 const FVector ToSource=(Source-HeroLocation).GetSafeNormal2D();
 const FVector ToScout=(ScoutLocation-HeroLocation).GetSafeNormal2D();
 if(ToSource.IsNearlyZero()||ToScout.IsNearlyZero()||FVector::DotProduct(ToSource,ToScout)<0.5)return 0.f;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallCompanionCover),false);
 Params.AddIgnoredActor(Hero);Params.AddIgnoredActor(Scout);
 FHitResult Hit;
 if(GetWorld()->LineTraceSingleByChannel(Hit,ScoutLocation+FVector(0,0,55),
      HeroLocation+FVector(0,0,55),ECC_Visibility,Params))return 0.f;
 LastCompanionCoverAt=Now;
 const float ArmorScale=1.f-FMath::Min(0.25f,Hero->GetCompanionGearTier(Scout->ResidentId,1)*0.01f);
 const float BraceScale=Scout->GetBondWithHero(Hero)>=80&&LocalCompanionRole==EMoonfallCompanionRole::Protector
    &&CompanionAbilityRole==LocalCompanionRole&&Now<CompanionAbilityEndsAt?0.5f:1.f;
 Scout->CompanionHealth=FMath::Max(0.f,Scout->CompanionHealth-Cover*ArmorScale*BraceScale);
 SyncCompanionVitals(Hero);
 if(Scout->CompanionHealth<=0.f){Scout->bCompanionDowned=true;CompanionAbilityEndsAt=-1000.f;Scout->OnCompanionDowned();}
 return Cover;
}
bool AMoonfallUnrealGameMode::CanReceiveCompanionAttack(const AMoonfallHeroPawn* Hero,const AMoonfallWorldNPC* Scout) const{
 return IsLocalCompanionOwner(Hero)&&IsValid(Scout)&&LocalCompanion.Get()==Scout
    &&Scout->GetWorld()==GetWorld()&&!Scout->IsActorBeingDestroyed()&&!Scout->bCompanionDowned
    &&FMath::IsFinite(Scout->CompanionHealth)&&Scout->CompanionHealth>0.f
    &&Hero->Health>0.f&&LocalCompanionStance==EMoonfallCompanionStance::Follow
    &&Scout->GetBondWithHero(Hero)>=60&&Hero->GetFactionReputation(Scout->FactionId)>=0;
 // Incoming attacks stay live during menus; outgoing player commands retain their locks.
}
AMoonfallWorldNPC* AMoonfallUnrealGameMode::SelectCompanionCounterTarget(AMoonfallHeroPawn* Hero,AMoonfallEnemy* Enemy,float Now,float Range) const{
 auto* Scout=LocalCompanion.Get();
 if(!CanReceiveCompanionAttack(Hero,Scout)||!IsValid(Enemy)||Enemy->GetWorld()!=GetWorld()
     ||Enemy->bBoss||Enemy->bElite||!Enemy->HasRecentCompanionAggression(Scout,Now))return nullptr;
 const FVector From=Enemy->GetActorLocation(),To=Scout->GetActorLocation(),Player=Hero->GetActorLocation();
 if(From.ContainsNaN()||To.ContainsNaN()||Player.ContainsNaN()||FMath::Abs(From.Z-To.Z)>100.0
    ||!MoonfallWorldPolicy::CanCompanionCounter(FVector::DistSquared2D(From,To),Range,FVector::DistSquared(From,Player),true))return nullptr;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallCounterTarget),false,Enemy);
 FHitResult Hit;
 if(GetWorld()->LineTraceSingleByChannel(Hit,From+FVector(0,0,55),To+FVector(0,0,55),ECC_Visibility,Params)
     &&Hit.GetActor()!=Scout)return nullptr;
 return Scout;
}
void AMoonfallUnrealGameMode::ResolveCompanionCounterAttack(AMoonfallHeroPawn* Hero,AMoonfallEnemy* Enemy,
    AMoonfallWorldNPC* Target,const FVector& Direction,float Damage,float Range){
 if(!CanReceiveCompanionAttack(Hero,Target)||!IsValid(Enemy)||Enemy->GetWorld()!=GetWorld()
    ||Enemy->bBoss||Enemy->bElite||Enemy->Health<=0.f||Direction.ContainsNaN())return;
 const FVector From=Enemy->GetActorLocation(),To=Target->GetActorLocation(),Player=Hero->GetActorLocation();
 if(From.ContainsNaN()||To.ContainsNaN()||Player.ContainsNaN()||FMath::Abs(From.Z-To.Z)>100.0
    ||!MoonfallWorldPolicy::CanCompanionCounter(FVector::DistSquared2D(From,To),Range,FVector::DistSquared(From,Player),true)
    ||FVector::DotProduct(Direction,(To-From).GetSafeNormal2D())<0.25)return;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallCounterImpact),false,Enemy);
 FHitResult Hit;
 if(GetWorld()->LineTraceSingleByChannel(Hit,From+FVector(0,0,55),To+FVector(0,0,55),ECC_Visibility,Params)
     &&Hit.GetActor()!=Target)return;
 const float Now=GetWorld()->GetTimeSeconds();
 const bool bBraced=Target->GetBondWithHero(Hero)>=80&&CompanionAbilityRole==EMoonfallCompanionRole::Protector
    &&LocalCompanionRole==CompanionAbilityRole&&Now<CompanionAbilityEndsAt;
 const float Actual=MoonfallWorldPolicy::FollowerIncomingDamage(Damage,Hero->GetCompanionGearTier(Target->ResidentId,1),bBraced);
 if(Actual<=0.f)return;
 Target->CompanionHealth=FMath::Max(0.f,Target->CompanionHealth-Actual);
 SyncCompanionVitals(Hero);
 if(Target->CompanionHealth<=0.f){Target->bCompanionDowned=true;CompanionAbilityEndsAt=-1000.f;Target->OnCompanionDowned();}
}
float AMoonfallUnrealGameMode::GetCompanionAbilityRemaining() const{
 return GetWorld()?FMath::Max(0.f,CompanionAbilityReadyAt-GetWorld()->GetTimeSeconds()):0.f;
}
bool AMoonfallUnrealGameMode::ActivateLocalCompanionAbility(AMoonfallHeroPawn* Hero){
 auto* Scout=LocalCompanion.Get();
 if(!IsLocalCompanionOwner(Hero)||!IsValid(Scout)||Scout->bCompanionDowned||!GetWorld()
    ||Hero->Health<=0.f||!Scout->CanHeroInteract(Hero)||Hero->GetFactionReputation(Scout->FactionId)<0
    ||LocalCompanionStance!=EMoonfallCompanionStance::Follow)return false;
 const int Rank=MoonfallWorldPolicy::CompanionRank(Scout->GetBondWithHero(Hero));
 const int Role=static_cast<int>(LocalCompanionRole);
 const float Now=GetWorld()->GetTimeSeconds();
 if(!MoonfallWorldPolicy::CanActivateCompanionAbility(Rank,Role,Scout->CompanionHealth,Now,CompanionAbilityReadyAt,true))return false;
 const float Cooldown=static_cast<float>(MoonfallWorldPolicy::CompanionAbilityCooldown(Hero->GetCompanionGearTier(Scout->ResidentId,2)));
 auto Next=Hero->GetCompanionParty();
 Next.AbilityReadyUtc=FDateTime::UtcNow().ToUnixTimestamp()+FMath::CeilToInt64(Cooldown);
 if(Role==0)Next.Health=FMath::Min(100.f,Scout->CompanionHealth+25.f);
 if(!Hero->CommitCompanionParty(Next))return false;
 CompanionAbilityReadyUtc=Next.AbilityReadyUtc;
 CompanionAbilityReadyAt=Now+Cooldown;
 CompanionAbilityRole=LocalCompanionRole;
 if(Role==0){Scout->CompanionHealth=FMath::Min(100.f,Scout->CompanionHealth+25.f);CompanionAbilityEndsAt=-1000.f;}
 else CompanionAbilityEndsAt=Now+(Role==1?6.f:8.f);
 return true;
}
void AMoonfallUnrealGameMode::ReportCompanionThreat(AMoonfallHeroPawn* Hero,AMoonfallEnemy* Enemy,float Now){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||!IsValid(Enemy)
    ||Enemy->GetWorld()!=GetWorld()||Enemy->IsActorBeingDestroyed()
    ||Enemy->bBoss||Enemy->bElite||Enemy->Health<=0.f||Hero->Health<=0.f
    ||LocalCompanionStance!=EMoonfallCompanionStance::Follow||LocalCompanion->bCompanionDowned)return;
 const FVector HeroLocation=Hero->GetActorLocation(),Target=Enemy->GetActorLocation(),Scout=LocalCompanion->GetActorLocation();
 if(HeroLocation.ContainsNaN()||Target.ContainsNaN()||Scout.ContainsNaN())return;
 if(!MoonfallWorldPolicy::CanReportCompanionThreat(Now,FVector::DistSquared(HeroLocation,Target),
      FVector::DistSquared(Scout,HeroLocation)))return;
 auto* Existing=CompanionThreat.Get();
 if(IsValid(Existing)&&Existing!=Enemy&&Existing->Health>0.f&&Now>=CompanionThreatAt&&Now-CompanionThreatAt<=2.f
     &&FVector::DistSquared(Existing->GetActorLocation(),HeroLocation)<FVector::DistSquared(Target,HeroLocation))return;
 CompanionThreat=Enemy;CompanionThreatAt=Now;
}
void AMoonfallUnrealGameMode::TryLocalCompanionStrike(AMoonfallHeroPawn* Hero,float Now){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||!GetWorld()
     ||LocalCompanionStance!=EMoonfallCompanionStance::Follow||Hero->Health<=0.f
     ||Hero->bInventoryOpen||Hero->bOptionsOpen||Hero->bAtlasOpen||Hero->bTalentWebOpen
     ||Hero->bCharacterStatsOpen||Hero->bSpellBookOpen||Hero->bResidentContextOpen)return;
 auto* Scout=LocalCompanion.Get();
 if(Scout->IsActorBeingDestroyed()||!Scout->bUsePersonalityRoutines||!Scout->bAllowLocalScoutGuidance
     ||Scout->bCompanionDowned||Scout->SocialRole!=FName(TEXT("Scout"))||Scout->GetBondWithHero(Hero)<60
     ||Hero->GetFactionReputation(Scout->FactionId)<0)return;
 const int Rank=MoonfallWorldPolicy::CompanionRank(Scout->GetBondWithHero(Hero));
 const int Role=static_cast<int>(LocalCompanionRole);
 const float StrikeDamage=FMath::Min(10.f,MoonfallWorldPolicy::CompanionStrikeDamage(Rank,Role)+Hero->GetCompanionGearTier(Scout->ResidentId,0)*0.08f);
 const bool bPrecisionBurst=Rank>=2&&LocalCompanionRole==EMoonfallCompanionRole::Striker
     &&CompanionAbilityRole==LocalCompanionRole&&Now<CompanionAbilityEndsAt;
 const float ExtraPosture=(bPrecisionBurst?8.f:0.f)+MoonfallWorldPolicy::CompanionExtraPosture(Rank,Role)+
     MoonfallWorldPolicy::ActiveCompanionTraining(LocalCompanionTraining,Rank,Role,7);
 float Age=-1.f;
 auto* Enemy=Hero->GetRecentCompanionAssistTarget(Age);
 // Player target wins when viable; otherwise defend against a fresh attack commitment.
 if(!IsValid(Enemy)||Enemy->IsActorBeingDestroyed()||Enemy->GetWorld()!=GetWorld()||Enemy->bBoss||Enemy->bElite||!FMath::IsFinite(Enemy->Health)||Enemy->Health<=StrikeDamage){
  Enemy=CompanionThreat.Get();Age=Now-CompanionThreatAt;
  if(!FMath::IsFinite(Age)||Age<0.f||Age>2.f)return;
 }
 if(!IsValid(Enemy)||Enemy->IsActorBeingDestroyed()||Enemy->GetWorld()!=GetWorld()
     ||Enemy->bBoss||Enemy->bElite||Enemy->Health<=StrikeDamage)return;
 const FVector HeroLocation=Hero->GetActorLocation(),ScoutLocation=Scout->GetActorLocation(),Target=Enemy->GetActorLocation();
 if(HeroLocation.ContainsNaN()||ScoutLocation.ContainsNaN()||Target.ContainsNaN()
     ||Scout->AuthoredHomeLocation.ContainsNaN()
     ||FVector::DistSquared(HeroLocation,ScoutLocation)>FMath::Square(500.0))return;
 if(!MoonfallWorldPolicy::CanCompanionStrike(Age,Now-LastCompanionStrikeAt,
      FVector::DistSquared(ScoutLocation,Target),FVector::DistSquared(HeroLocation,Target),true,
      MoonfallWorldPolicy::CompanionRank(Scout->GetBondWithHero(Hero)),static_cast<int>(LocalCompanionRole),LocalCompanionTraining))return;
 const int Region=MoonfallLivingSimulation::RegionFromWorldX(ScoutLocation.X);
 if(MoonfallLivingSimulation::Severe(MoonfallLivingSimulation::WeatherFor(Region,
       MoonfallLivingSimulation::Phase(Now,360.f))))return;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallCompanionStrike),false);
 Params.AddIgnoredActor(Scout);Params.AddIgnoredActor(Hero);
 FHitResult Hit;
 if(GetWorld()->LineTraceSingleByChannel(Hit,ScoutLocation+FVector(0,0,55),
      Target+FVector(0,0,55),ECC_Visibility,Params)&&Hit.GetActor()!=Enemy)return;
 // Commit cooldown before damage callbacks to prevent reentrant support.
 LastCompanionStrikeAt=Now;
 if(!Enemy->TryCompanionSupportStrike(StrikeDamage))return;
 if(bPrecisionBurst)CompanionAbilityEndsAt=-1000.f;
 if(IsValid(Enemy)&&IsValid(Scout))Enemy->RememberCompanionAttacker(Scout,Now);
 // Precision only adds pressure on a surviving ordinary target that is
 // not already exposed. The enemy owns posture limits and break resolution.
 if(ExtraPosture>0.f&&IsValid(Enemy)&&!Enemy->IsActorBeingDestroyed()&&!Enemy->bBoss&&!Enemy->bElite
     &&Enemy->Health>0.f&&!Enemy->bPostureFinisherAvailable&&Now>=Enemy->PostureBrokenUntil)
  Enemy->ApplyPostureDamage(ExtraPosture);
 if(IsValid(Scout)&&!Scout->IsActorBeingDestroyed())Scout->OnCompanionSupportStrike();
}
bool AMoonfallUnrealGameMode::SetLocalCompanionStance(AMoonfallHeroPawn* Hero,EMoonfallCompanionStance Stance){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||Hero->Health<=0.f)return false;
 switch(Stance){
 case EMoonfallCompanionStance::Follow:case EMoonfallCompanionStance::Hold:case EMoonfallCompanionStance::Passive:
  {
   auto Next=Hero->GetCompanionParty();
   Next.Stance=static_cast<int32>(Stance);
   if(!Hero->CommitCompanionParty(Next))return false;
  }
  LocalCompanionStance=Stance;bFollowerMoveOrdered=false;FollowerSeparatedAt=-1.f;
  LocalCompanion->bFollowerWaypointValid=false;LocalCompanion->NextFollowerPathAt=-1.f;return true;
 default:return false;
 }
}
bool AMoonfallUnrealGameMode::SetLocalCompanionRole(AMoonfallHeroPawn* Hero,EMoonfallCompanionRole Role){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||Hero->Health<=0.f
     ||Hero->IsRecentlyInCombatForAmbientAI()||!LocalCompanion->CanHeroInteract(Hero))return false;
 const int Rank=MoonfallWorldPolicy::CompanionRank(LocalCompanion->GetBondWithHero(Hero));
 if(Hero->GetFactionReputation(LocalCompanion->FactionId)<0||Rank<1)return false;
 switch(Role){
 case EMoonfallCompanionRole::Balanced:break;
 case EMoonfallCompanionRole::Protector:case EMoonfallCompanionRole::Striker:if(Rank<2)return false;break;
 default:return false;
 }
 if(!Hero->CommitCompanionProfile(LocalCompanion->ResidentId,
      MoonfallWorldPolicy::PackCompanionProfile(LocalCompanionTraining,static_cast<int>(Role))))return false;
 if(LocalCompanionRole!=Role)CompanionAbilityEndsAt=-1000.f;
 LocalCompanionRole=Role;return true;
}
bool AMoonfallUnrealGameMode::TrainLocalCompanionTalent(AMoonfallHeroPawn* Hero,int32 Node){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||Hero->Health<=0.f
    ||Hero->IsRecentlyInCombatForAmbientAI()||!LocalCompanion->CanHeroInteract(Hero)
    ||Hero->GetFactionReputation(LocalCompanion->FactionId)<0)return false;
 const int Rank=MoonfallWorldPolicy::CompanionRank(LocalCompanion->GetBondWithHero(Hero));
 const uint32 Next=MoonfallWorldPolicy::TrainCompanion(LocalCompanionTraining,Rank,static_cast<int>(LocalCompanionRole),Node);
 if(Next==LocalCompanionTraining)return false;
 if(!Hero->CommitCompanionProfile(LocalCompanion->ResidentId,
      MoonfallWorldPolicy::PackCompanionProfile(Next,static_cast<int>(LocalCompanionRole))))return false;
 LocalCompanionTraining=Next;return true;
}
bool AMoonfallUnrealGameMode::ResetLocalCompanionTalents(AMoonfallHeroPawn* Hero){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||Hero->Health<=0.f
    ||Hero->IsRecentlyInCombatForAmbientAI()||!LocalCompanion->CanHeroInteract(Hero))return false;
 if(!Hero->CommitCompanionProfile(LocalCompanion->ResidentId,
      MoonfallWorldPolicy::PackCompanionProfile(0,static_cast<int>(LocalCompanionRole))))return false;
 LocalCompanionTraining=0;return true;
}
bool AMoonfallUnrealGameMode::OrderCompanionMove(AMoonfallHeroPawn* Hero,FVector Destination){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||LocalCompanion->bCompanionDowned
    ||Hero->Health<=0.f||Destination.ContainsNaN()||Hero->GetActorLocation().ContainsNaN()
    ||FVector::DistSquared(Destination,Hero->GetActorLocation())>FMath::Square(1000.0))return false;
 auto Next=Hero->GetCompanionParty();
 Next.Stance=static_cast<int32>(EMoonfallCompanionStance::Hold);
 if(!Hero->CommitCompanionParty(Next))return false;
 OrderedFollowerDestination=Destination;bFollowerMoveOrdered=true;
 LocalCompanion->bFollowerWaypointValid=false;LocalCompanion->NextFollowerPathAt=-1.f;
 LocalCompanionStance=EMoonfallCompanionStance::Hold;return true;
}
bool AMoonfallUnrealGameMode::ReviveLocalCompanion(AMoonfallHeroPawn* Hero){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||!LocalCompanion->bCompanionDowned
     ||Hero->IsRecentlyInCombatForAmbientAI()||!LocalCompanion->CanHeroInteract(Hero))return false;
 auto* Scout=LocalCompanion.Get();
 auto Next=Hero->GetCompanionParty();Next.Health=50.f;
 if(!Hero->CommitCompanionParty(Next))return false;
 Scout->CompanionHealth=50.f;Scout->bCompanionDowned=false;Scout->OnCompanionRecovered();return true;
}
void AMoonfallUnrealGameMode::PulseFollower(){
 if(!HasAuthority()||!GetWorld())return;
 const float Now=GetWorld()->GetTimeSeconds();
 if(!FMath::IsFinite(Now))return;
 if(auto* Returning=ReturningCompanion.Get()){
  Returning->LivingIntent=TEXT("Returning Home");
  if(Returning->AdvanceFollowerToward(Returning->AuthoredHomeLocation,Now,40.f)){
   Returning->LivingIntent.Reset();ReturningCompanion.Reset();
  }
 }
 auto* Scout=LocalCompanion.Get();auto* Hero=CompanionOwner.Get();
 if(!IsValid(Scout)||!IsValid(Hero)){
  if(!ReturningCompanion.IsValid())GetWorldTimerManager().ClearTimer(FollowerPulseTimer);
  return;
 }
 if(!IsLocalCompanionOwner(Hero)||!FMath::IsFinite(Hero->Health)||Hero->Health<=0.f||Hero->bInventoryOpen||Hero->bOptionsOpen
    ||Hero->bAtlasOpen||Hero->bTalentWebOpen||Hero->bCharacterStatsOpen||Hero->bSpellBookOpen
    ||Hero->bResidentContextOpen){FollowerSeparatedAt=-1.f;return;}
 if(Scout->bCompanionDowned){FollowerSeparatedAt=-1.f;Scout->LivingIntent=TEXT("Downed");return;}
 if(Scout->GetBondWithHero(Hero)<60||Hero->GetFactionReputation(Scout->FactionId)<0){FollowerSeparatedAt=-1.f;return;}
 const bool bFreshThreat=CompanionThreat.IsValid()&&Now>=CompanionThreatAt&&Now-CompanionThreatAt<=2.f;
 if(!FMath::IsFinite(Scout->CompanionHealth)||Scout->CompanionHealth<=0.f){
  Scout->CompanionHealth=0.f;Scout->bCompanionDowned=true;FollowerSeparatedAt=-1.f;SyncCompanionVitals(Hero);return;
 }
 if(!bFreshThreat&&!Hero->IsRecentlyInCombatForAmbientAI())Scout->CompanionHealth=FMath::Min(100.f,Scout->CompanionHealth+0.8f);
 SyncCompanionVitals(Hero);
 const double Separation=FVector::DistSquared(Hero->GetActorLocation(),Scout->GetActorLocation());
 const bool bRecoverySafe=LocalCompanionStance==EMoonfallCompanionStance::Follow&&!bFollowerMoveOrdered
     &&!bFreshThreat&&!Hero->IsRecentlyInCombatForAmbientAI()&&FMath::IsFinite(Separation)&&Separation>=FMath::Square(1800.0);
 if(!bRecoverySafe)FollowerSeparatedAt=-1.f;
 else {
  if(FollowerSeparatedAt<0.f||Now<FollowerSeparatedAt)FollowerSeparatedAt=Now;
  if(Now>=NextFollowerCatchupAt&&MoonfallWorldPolicy::CanRecoverFollower(Separation,Now-FollowerSeparatedAt,true)){
   NextFollowerCatchupAt=Now+5.f;
   if(Scout->TrySafeFollowerCatchup(Hero)){FollowerSeparatedAt=-1.f;return;}
  }
 }
 if(LocalCompanionStance==EMoonfallCompanionStance::Follow&&CompanionThreat.IsValid()
     &&Now>=CompanionThreatAt&&Now-CompanionThreatAt<=2.f)TryLocalCompanionStrike(Hero,Now);
 if(!IsValid(Scout)||Scout->IsActorBeingDestroyed())return;
 if(bFollowerMoveOrdered){
  Scout->LivingIntent=TEXT("Moving to order");
  if(Scout->AdvanceFollowerToward(OrderedFollowerDestination,Now,45.f))bFollowerMoveOrdered=false;
 }else if(LocalCompanionStance==EMoonfallCompanionStance::Follow){
  const FVector Player=Hero->GetActorLocation();
  const auto* Threat=CompanionThreat.Get();
  const bool bThreat=IsValid(Threat)&&Threat->Health>0.f&&Now>=CompanionThreatAt&&Now-CompanionThreatAt<=2.f
      &&!Threat->GetActorLocation().ContainsNaN()&&!Player.ContainsNaN()
      &&FVector::DistSquared(Player,Threat->GetActorLocation())<=FMath::Square(350.0);
  const auto Tactic=MoonfallWorldPolicy::SelectCompanionTactic(Scout->CompanionHealth,
      MoonfallWorldPolicy::CompanionRank(Scout->GetBondWithHero(Hero)),static_cast<int>(LocalCompanionRole),bThreat);
  using MoonfallWorldPolicy::CompanionTactic;
  if(Tactic==CompanionTactic::Follow){
   Scout->LivingIntent=TEXT("Following");
   Scout->AdvanceFollowerToward(Hero->GetActorLocation(),Now,180.f);
  }else{
   const FVector Toward=(Threat->GetActorLocation()-Player).GetSafeNormal2D();
   const FVector Side(-Toward.Y,Toward.X,0);
   const double Sign=(GetTypeHash(Scout->ResidentId)%2u)?1.0:-1.0;
   FVector Goal=Player;
   if(Tactic==CompanionTactic::Intercept){Goal+=Toward*100.0;Scout->LivingIntent=TEXT("Intercepting");}
   else if(Tactic==CompanionTactic::Flank){Goal+=Toward*100.0+Side*160.0*Sign;Scout->LivingIntent=TEXT("Flanking");}
   else {Goal-=Toward*250.0;Scout->LivingIntent=TEXT("Withdrawing");}
   Scout->AdvanceFollowerToward(Goal,Now,45.f);
  }
 }else Scout->LivingIntent=LocalCompanionStance==EMoonfallCompanionStance::Hold?TEXT("Holding Position"):TEXT("Passive");
}
bool AMoonfallUnrealGameMode::DismissLocalCompanion(AMoonfallHeroPawn* Hero){
 if(!IsLocalCompanionOwner(Hero)||!LocalCompanion.IsValid()||LocalCompanion->bCompanionDowned)return false;
 if(ReturningCompanion.IsValid()&&ReturningCompanion.Get()!=LocalCompanion.Get())return false;
 FMoonfallCompanionPartyRecord Next;Next.AbilityReadyUtc=CompanionAbilityReadyUtc;
 if(!Hero->CommitCompanionParty(Next))return false;
 ReturningCompanion=LocalCompanion;bFollowerMoveOrdered=false;CompanionThreat.Reset();CompanionThreatAt=-1000.f;CompanionAbilityEndsAt=-1000.f;
 ReturningCompanion->bFollowerWaypointValid=false;ReturningCompanion->NextFollowerPathAt=-1.f;
 LocalCompanion.Reset();CompanionOwner.Reset();LocalCompanionStance=EMoonfallCompanionStance::Passive;LocalCompanionRole=EMoonfallCompanionRole::Balanced;LocalCompanionTraining=0;return true;
}
void AMoonfallUnrealGameMode::UnregisterLivingResident(AMoonfallWorldNPC* Resident) {
 if(LocalCompanion.Get()==Resident){LocalCompanion.Reset();CompanionOwner.Reset();LocalCompanionStance=EMoonfallCompanionStance::Passive;LocalCompanionRole=EMoonfallCompanionRole::Balanced;LocalCompanionTraining=0;}
 LivingResidents.RemoveAll([Resident](const TWeakObjectPtr<AMoonfallWorldNPC>& Ref) {
  return !Ref.IsValid() || Ref.Get()==Resident;
 });
}

void AMoonfallUnrealGameMode::BeginPlay()
{
 Super::BeginPlay();
 if (!GetWorld()) return;

 // Preserve an arena already placed by a designer.
 bool bHasArena = false;
 for (TActorIterator<AMoonfallArena> It(GetWorld()); It; ++It)
  if (IsValid(*It) && !It->IsActorBeingDestroyed()) { bHasArena = true; break; }
 if (!bHasArena)
  GetWorld()->SpawnActor<AMoonfallArena>(FVector::ZeroVector, FRotator::ZeroRotator);

 // A single low-frequency pulse powers nearby resident routines, regional
 // atmosphere and bounded, optional roadside encounters.
 GetWorldTimerManager().SetTimer(LivingWorldPulseTimer, this,
     &AMoonfallUnrealGameMode::PulseLivingWorld, 4.f, true, 4.f);
 if (!bGenerateDungeonWorld) return;
 ReadyDungeonCount = 0;
 AuthoredDungeonSlots.SetNumZeroed(TotalDungeons);
 // Inspect authored layouts only once, rather than scanning world actors for
 // each of the thirty spawn requests. Never duplicate an authored dungeon.
 for (TActorIterator<AMoonfallDungeonLayout> It(GetWorld()); It; ++It) {
  // Destroying layouts must not reserve a permanent dungeon slot.
  if (!IsValid(*It) || It->IsActorBeingDestroyed()) continue;
  const int32 Zone = It->ZoneIndex;
  const int32 Dungeon = It->DungeonIndex;
  if (Zone >= 0 && Zone < DungeonZoneCount
      && Dungeon >= 0 && Dungeon < DungeonsPerZone)
   {
    const int32 Slot = Zone * DungeonsPerZone + Dungeon;
    if (!AuthoredDungeonSlots[Slot]) {
     AuthoredDungeonSlots[Slot] = 1;
     ++ReadyDungeonCount;
    }
   }
 }
 NextDungeonSlot = 0;
 NextFailedDungeonRetry = 0;
 FailedDungeonSlots.Reset();
 bDungeonStartupAuditComplete = false;
 bDungeonStartupNotified = false;
 SpawnDungeonBatch();
 if (!bDungeonStartupNotified)
  GetWorldTimerManager().SetTimer(DungeonSpawnTimer, this,
    &AMoonfallUnrealGameMode::SpawnDungeonBatch, 0.1f, true);
}

void AMoonfallUnrealGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 if (GetWorld()) {
  GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);
  GetWorldTimerManager().ClearTimer(LivingWorldPulseTimer);
  GetWorldTimerManager().ClearTimer(FollowerPulseTimer);
 }
 Super::EndPlay(EndPlayReason);
}

void AMoonfallUnrealGameMode::ShareNearbyResidentRumor(
 const TArray<TWeakObjectPtr<AMoonfallWorldNPC>>& Candidates) {
 UWorld* World=GetWorld();
 if (!World || Candidates.Num()<2) return;
 auto* Society=World->GetSubsystem<UMoonfallLivingWorldSubsystem>();
 if (!Society) return;
 // At most one speaker and one line trace per four-second world pulse.
 const int32 SpeakerIndex=SocialSpeakerCursor%Candidates.Num();
 SocialSpeakerCursor=(SocialSpeakerCursor+1)%MoonfallLivingSimulation::ResidentRegistryBudget;
 AMoonfallWorldNPC* Speaker=Candidates[SpeakerIndex].Get();
 if (!IsValid(Speaker) || Speaker->IsActorBeingDestroyed()) return;
 AMoonfallWorldNPC* Listener=nullptr;
 double BestDistance=FMath::Square(300.0);
 for (int32 I=0; I<Candidates.Num(); ++I) {
  auto* Other=Candidates[I].Get();
  if (!IsValid(Other) || Other->IsActorBeingDestroyed() || Other==Speaker
      || Other->ResidentId==Speaker->ResidentId || Other->SettlementId!=Speaker->SettlementId) continue;
  const double Distance=FVector::DistSquared(Speaker->GetActorLocation(),Other->GetActorLocation());
  if (!FMath::IsFinite(Distance) || Distance>=BestDistance) continue;
  // Reject hostile listeners before nearest-neighbor selection. Otherwise
  // the same hostile neighbor can permanently block all friendly exchanges.
  int32 Attitude=0;
  Society->GetNpcRelationship(Speaker->ResidentId,Other->ResidentId,Attitude);
  if (Attitude<=-20) continue;
  if (Society->FindUsefulRumor(Speaker->ResidentId,Other->ResidentId).IsNone()) continue;
  BestDistance=Distance;
  Listener=Other;
 }
 if (!Listener) return;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallResidentSocialLOS),false);
 Params.AddIgnoredActor(Speaker);
 Params.AddIgnoredActor(Listener);
 FHitResult Hit;
 const FVector EyeOffset(0,0,55);
 if (World->LineTraceSingleByChannel(Hit,Speaker->GetActorLocation()+EyeOffset,
      Listener->GetActorLocation()+EyeOffset,ECC_Visibility,Params)) return;
 if (!Society->ShareUsefulRumor(Speaker->ResidentId,Listener->ResidentId)) return;
 // A dedicated ambient event never opens player dialogue, overwrites a
 // quest greeting or changes player reputation. Audio/animation can bind
 // to this event later; only one event is emitted after a real transfer.
 Speaker->OnAmbientNewsShared(Listener->ResidentId,Listener->DisplayName);
}

void AMoonfallUnrealGameMode::PulseLivingWorld() {
 UWorld* World=GetWorld();
 if (!World) return;
 const float Now=World->GetTimeSeconds();
 if (!FMath::IsFinite(Now) || Now<0.f) return;
 if(auto* SocialMemory=World->GetSubsystem<UMoonfallLivingWorldSubsystem>())SocialMemory->FlushSocialMemory();
 if (!LivingArena.IsValid()) {
  for (TActorIterator<AMoonfallArena> It(World); It; ++It) {
   if (IsValid(*It) && !(*It)->IsActorBeingDestroyed()) {
    LivingArena=*It; break;
   }
  }
 }
 if (LivingArena.IsValid()) LivingArena->UpdateRegionWeather(Now);
 if (!bLivingResidentCacheInitialized) {
  // One small setup scan for the permanent Firstlight/Crossroads residents.
  bLivingResidentCacheInitialized=true;
  int32 Scanned=0;
  for (TActorIterator<AMoonfallWorldNPC> It(World);
       It && Scanned<MoonfallLivingSimulation::ResidentRegistryBudget; ++It,++Scanned) {
   if (IsValid(*It) && !(*It)->IsActorBeingDestroyed() && !(*It)->bAmbientVisitor)
    RegisterLivingResident(*It);
  }
 }
 AMoonfallHeroPawn* Hero=Cast<AMoonfallHeroPawn>(
     UGameplayStatics::GetPlayerPawn(this,0));
 // Even if the player dies, disconnects, or has no valid pawn this pulse,
 // the director must retire old actors. Otherwise one invalid hero could
 // permanently occupy the one-event slot after SetLifeSpan(0.f).
 const bool bHeroActive=IsValid(Hero) && Hero->IsPlayerControlled()
     && FMath::IsFinite(Hero->Health) && Hero->Health>0.f;
 const FVector HeroLocation=bHeroActive ? Hero->GetActorLocation() : FVector::ZeroVector;
 const bool bHeroReady=bHeroActive && !HeroLocation.ContainsNaN();
 const bool bCombatNearby=bHeroReady && Hero->IsRecentlyInCombatForAmbientAI();
 // Retire old ambient actors on the existing four-second pulse, not via
 // a fixed SetLifeSpan that could destroy a conversation or active fight.
 if (AActor* Active=ActiveAmbientEncounter.Get()) {
  if (!IsValid(Active) || Active->IsActorBeingDestroyed()) {
   ActiveAmbientEncounter.Reset();
   ActiveAmbientExpiresAt=-1.f;
  } else {
   // Conversations and active nearby battles hold their event open.
   // A missing/dead hero cannot hold an expired event indefinitely.
   const bool bTalkingToVisitor=bHeroReady && Hero->bResidentContextOpen
       && Hero->FocusedResident.Get()==Active;
   const bool bNearbyMonsterBattle=bCombatNearby
       && Active->ActorHasTag(FName(TEXT("MoonfallAmbientEncounter")))
       && !Active->GetActorLocation().ContainsNaN()
       && FVector::DistSquared2D(HeroLocation,Active->GetActorLocation())
           < FMath::Square(1200.f);
   if (MoonfallLivingSimulation::ShouldRetireAmbient(
           Now,ActiveAmbientExpiresAt,bTalkingToVisitor,bNearbyMonsterBattle)) {
    Active->Destroy();
    ActiveAmbientEncounter.Reset();
    ActiveAmbientExpiresAt=-1.f;
   }
  }
 } else {
  ActiveAmbientExpiresAt=-1.f;
 }
 if(!LocalCompanion.IsValid()||!CompanionOwner.IsValid()
     ||(bHeroReady&&CompanionOwner.Get()!=Hero)){
  LocalCompanion.Reset();CompanionOwner.Reset();LocalCompanionStance=EMoonfallCompanionStance::Passive;LocalCompanionRole=EMoonfallCompanionRole::Balanced;LocalCompanionTraining=0;
 }
 if (!bHeroReady) return;
 TryRestoreCompanionParty(Hero);
 // Scan a bounded registry of <=80 weak refs, but run the expensive
 // weather / movement / danger routines for no more than 20 NPCs.
 // With <=20 nearby, all respond on every pulse instead of waiting for
 // their distant settlement's registry slots. Larger local crowds rotate.
 auto* Society=World->GetSubsystem<UMoonfallLivingWorldSubsystem>();
 TArray<TWeakObjectPtr<AMoonfallWorldNPC>> SocialCandidates;
 SocialCandidates.Reserve(MoonfallLivingSimulation::ResidentBudget);
 const int32 CombatPhase=MoonfallLivingSimulation::Phase(Now,20.f);
 const bool bNewCombatObservation=bCombatNearby && CombatPhase!=LastSocialCombatPhase;
 // Nearby combat is a non-crime danger observation, not proof of a crime.
 const FName CombatRumorId=bNewCombatObservation
     ? FName(*FString::Printf(TEXT("NearbyCombat_%d"),CombatPhase%1024)) : NAME_None;
 bool bPublishedCombat=false;
 bool bGuideBudgetUsed=false;
 const bool bGuideSafe=!bCombatNearby&&!Hero->bInventoryOpen&&!Hero->bOptionsOpen
     &&!Hero->bAtlasOpen&&!Hero->bCharacterStatsOpen&&!Hero->bSpellBookOpen
     &&!Hero->bResidentContextOpen&&!Hero->bTalentWebOpen;
 NextResidentPulseIndex=MoonfallLivingSimulation::ProcessNearbyResidents(
     NextResidentPulseIndex,LivingResidents.Num(),
     [&](int Index) {
      AMoonfallWorldNPC* NPC=LivingResidents[Index].Get();
      if (!IsValid(NPC) || NPC->IsActorBeingDestroyed()) return false;
      const float DistSq=FVector::DistSquared2D(HeroLocation,NPC->GetActorLocation());
      return FMath::IsFinite(DistSq) && DistSq<=FMath::Square(1600.f);
     },
     [&](int Index) {
      AMoonfallWorldNPC* NPC=LivingResidents[Index].Get();
      if (!IsValid(NPC) || NPC->IsActorBeingDestroyed()) return;
      const float DistSq=FVector::DistSquared2D(HeroLocation,NPC->GetActorLocation());
      if (bCombatNearby && DistSq<FMath::Square(700.f)) {
       NPC->WitnessNearbyDanger(Now);
       if (Society && bNewCombatObservation && !NPC->ResidentId.IsNone())
        bPublishedCombat=Society->PublishObservation(NPC->ResidentId,CombatRumorId,HeroLocation,false,180.f)
            || bPublishedCombat;
      }
      const bool bTalking=Hero->bResidentContextOpen && Hero->FocusedResident.Get()==NPC;
      const int32 Bond=NPC->GetBondWithHero(Hero);
      const bool bRecruited=LocalCompanion.Get()==NPC&&IsLocalCompanionOwner(Hero);
      if(bRecruited||ReturningCompanion.Get()==NPC)return;
      const bool bStanceAllowsGuide=!LocalCompanion.IsValid()
          ||(bRecruited&&LocalCompanionStance==EMoonfallCompanionStance::Follow);
      const bool bHold=bRecruited&&LocalCompanionStance==EMoonfallCompanionStance::Hold
          &&bGuideSafe&&Bond>=60&&Hero->GetFactionReputation(NPC->FactionId)>=0
          &&FVector::DistSquared(HeroLocation,NPC->AuthoredHomeLocation)<=FMath::Square(450.0);
      const bool bGuide=!bGuideBudgetUsed&&bStanceAllowsGuide&&NPC->bAllowLocalScoutGuidance&&NPC->bUsePersonalityRoutines
          &&NPC->SocialRole==FName(TEXT("Scout"))
          &&MoonfallWorldPolicy::CanLocalGuide(Bond,Hero->GetFactionReputation(NPC->FactionId),
             FVector::DistSquared(HeroLocation,NPC->AuthoredHomeLocation),
             FVector::DistSquared(HeroLocation,NPC->GetActorLocation()),bGuideSafe,true);
      if(bGuide)bGuideBudgetUsed=true;
      NPC->AdvanceLivingRoutine(Now,HeroLocation,
          Hero->bResidentContextOpen && Hero->FocusedResident.Get()==NPC,
          Bond,bGuide,bHold);
      if (!bCombatNearby && !bTalking && NPC->bUsePersonalityRoutines && !NPC->ResidentId.IsNone()
          && !NPC->SettlementId.IsNone()
          && MoonfallLivingSimulation::ReactionFor(MoonfallLivingSimulation::ERole::Citizen,
               Now-NPC->LastDangerAt)==MoonfallLivingSimulation::EAlertReaction::Normal
          && NPC->LivingIntent!=TEXT("Seeking Shelter") && NPC->LivingIntent!=TEXT("Resting"))
       SocialCandidates.Add(NPC);
     });
 if (bCombatNearby) TryLocalCompanionStrike(Hero,Now);
 if (bPublishedCombat) LastSocialCombatPhase=CombatPhase;
 if (!bCombatNearby && !Hero->bResidentContextOpen) ShareNearbyResidentRumor(SocialCandidates);
 // Event cooldowns and region eligibility are deterministic. We spawn at
 // most one short-lived ambient actor, never inside the starting sanctuary.
 if (Now<NextAmbientEncounterAt) return;
 const int Region=MoonfallLivingSimulation::RegionFromWorldX(HeroLocation.X);
 const bool bSafeZone=Hero->IsNearStartingSanctuary() ||
     HeroLocation.Y > -1800.f;
 const bool bNearRoad=FMath::Abs(HeroLocation.Y+2500.f)<260.f;
 // New encounters wait until gameplay is active, including stats/Spell Book,
 // and never surprise the player while fighting an existing enemy.
 const bool bModal=Hero->bInventoryOpen || Hero->bOptionsOpen ||
     Hero->bAtlasOpen || Hero->bCharacterStatsOpen ||
     Hero->bSpellBookOpen || Hero->bResidentContextOpen ||
     Hero->FocusedResident.IsValid() || bCombatNearby;
 if (!MoonfallLivingSimulation::MayEncounter(Region,bSafeZone,bNearRoad,
     bModal,ActiveAmbientEncounter.IsValid())) return;
 const int Phase=MoonfallLivingSimulation::Phase(Now,150.f);
 if (Phase==LastAmbientEventPhase) return;
 // Sparse road event placement uses one ground ray only when committing an
 // encounter. No per-frame traces, NPC crowds or persistent world-save data.
 // Prefer the direction the hero is traveling on the east/west road.
 // At region edges, choose the opposite side without crossing regions or
 // paying for multiple ground traces. No per-player saved travel state.
 const float CandidateOffset=MoonfallLivingSimulation::RoadEncounterOffset(
     HeroLocation.X,Hero->GetVelocity().X);
 if (CandidateOffset==0.f) return;
 const FVector Candidate(HeroLocation.X+CandidateOffset,-2500.f,HeroLocation.Z+600.f);
 if (MoonfallLivingSimulation::RegionFromWorldX(Candidate.X)!=Region) return;
 FCollisionQueryParams FloorQuery(SCENE_QUERY_STAT(MoonfallAmbientRoadGround),false);
 FloorQuery.AddIgnoredActor(Hero);
 FHitResult Ground;
 if (!World->LineTraceSingleByChannel(Ground,Candidate,
       Candidate-FVector(0.f,0.f,1400.f),ECC_Visibility,FloorQuery)
     || !Ground.bBlockingHit || Ground.ImpactNormal.Z<0.7f
     || !LivingArena.IsValid() || !IsValid(LivingArena->DungeonRoad)
     || Ground.GetComponent()!=LivingArena->DungeonRoad) return;
 const FVector SpawnPosition=Ground.ImpactPoint+FVector(0.f,0.f,85.f);
 if (SpawnPosition.ContainsNaN()) return;
 const FTransform SpawnTransform(FRotator::ZeroRotator,SpawnPosition);
 const auto CurrentWeather=MoonfallLivingSimulation::WeatherFor(
     Region,MoonfallLivingSimulation::Phase(Now,360.f));
 const auto Event=MoonfallLivingSimulation::EncounterForWeather(
     Region,Phase,CurrentWeather);
 AActor* Spawned=nullptr;
 if (Event==MoonfallLivingSimulation::EEvent::Monster) {
  // A single short-lived, ordinary enemy; no divine IDs or special loot.
  AMoonfallEnemy* Enemy=World->SpawnActorDeferred<AMoonfallEnemy>(
    AMoonfallEnemy::StaticClass(),SpawnTransform,this,nullptr,
    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
  if (IsValid(Enemy)) {
   Enemy->bBoss=false;
   Enemy->bElite=false;
   Enemy->Health=35.f;
   Enemy->AttackDamage=4.f;
   Enemy->Tags.AddUnique(FName(TEXT("MoonfallAmbientEncounter")));
   Enemy->FinishSpawning(SpawnTransform);
   Spawned=Enemy;
  }
 } else {
  // Deferred spawn sets bAmbientVisitor before BeginPlay, so a temporary
  // traveler never registers as a permanent world resident.
  AMoonfallWorldNPC* Visitor=World->SpawnActorDeferred<AMoonfallWorldNPC>(
      AMoonfallWorldNPC::StaticClass(),SpawnTransform,this,nullptr,
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
  if (IsValid(Visitor)) {
   Visitor->bAmbientVisitor=true;
   Visitor->ResidentId=NAME_None;
   Visitor->SettlementId=FName(TEXT("Roadside"));
   Visitor->FactionId=FName(TEXT("Wanderers"));
   switch (Event) {
    case MoonfallLivingSimulation::EEvent::Merchant:
     Visitor->DisplayName=FText::FromString(TEXT("Roadside Trader"));
     Visitor->SocialRole=FName(TEXT("Merchant"));
     Visitor->Greeting=FText::FromString(TEXT("I bring supplies and stories from the next province."));
     break;
    case MoonfallLivingSimulation::EEvent::Patrol:
     Visitor->DisplayName=FText::FromString(TEXT("Passing Patrol"));
     Visitor->SocialRole=FName(TEXT("Guard"));
     Visitor->Greeting=FText::FromString(TEXT("We patrol these roads. Keep an eye on the shadows."));
     break;
    case MoonfallLivingSimulation::EEvent::Rescue:
     Visitor->DisplayName=FText::FromString(TEXT("Stranded Wayfarer"));
     Visitor->SocialRole=FName(TEXT("Scout"));
     Visitor->Greeting=FText::FromString(TEXT("I lost my caravan near the old boundary stones."));
     break;
    case MoonfallLivingSimulation::EEvent::Dispute:
     Visitor->DisplayName=FText::FromString(TEXT("Disputing Traveler"));
     Visitor->SocialRole=FName(TEXT("Citizen"));
     Visitor->Greeting=FText::FromString(TEXT("Two caravans claim this road. Who can settle it?"));
     break;
    default:break;
   }
   // Each of the six lands has a distinct local history. This is
   // conversation flavour, not a fabricated quest or reputation reward.
   static const TCHAR* const RegionRumors[6] = {
    TEXT("Ashen Wilds: red dust hides ruined watchfires."),
    TEXT("Moonveil Forest: old trees mark paths no map remembers."),
    TEXT("Hollow Depths: the earth echoes beneath abandoned roads."),
    TEXT("Iron Foundry: broken furnaces glow beyond the ridge."),
    TEXT("Frostbound Reach: ice seals the passes at dusk."),
    TEXT("Eclipse Wastes: even the moonlight seems to falter.")
   };
   if (Region>=0 && Region<6) {
    FString Story=Visitor->Greeting.ToString()+TEXT(" ")+RegionRumors[Region];
    if (MoonfallLivingSimulation::Severe(CurrentWeather))
     Story+=TEXT(" The weather has made travel dangerous.");
    Visitor->Greeting=FText::FromString(Story);
   }
   Visitor->FinishSpawning(SpawnTransform);
   if (IsValid(Visitor) && !Visitor->IsActorBeingDestroyed())
    Spawned=Visitor;
  }
 }
 if (!IsValid(Spawned)) return;
 // The director owns this one event until its expiry. Waiting on a real
 // conversation or current monster battle never introduces a second event.
 Spawned->SetLifeSpan(0.f);
 ActiveAmbientEncounter=Spawned;
 ActiveAmbientExpiresAt=Now+100.f;
 LastAmbientEventPhase=Phase;
 NextAmbientEncounterAt=Now+150.f;
}
void AMoonfallUnrealGameMode::AuditLiveDungeonSlots(bool bQueueMissing)
{
 if (!GetWorld()) return;
 TArray<uint8> LiveSlots;
 LiveSlots.SetNumZeroed(TotalDungeons);
 for (TActorIterator<AMoonfallDungeonLayout> It(GetWorld()); It; ++It) {
  if (!IsValid(*It) || It->IsActorBeingDestroyed()) continue;
  if (It->ZoneIndex < 0 || It->ZoneIndex >= DungeonZoneCount
      || It->DungeonIndex < 0 || It->DungeonIndex >= DungeonsPerZone) continue;
  LiveSlots[It->ZoneIndex * DungeonsPerZone + It->DungeonIndex] = 1;
 }
 ReadyDungeonCount = 0;
 for (int32 Slot = 0; Slot < TotalDungeons; ++Slot) {
  if (LiveSlots[Slot]) ++ReadyDungeonCount;
  else if (bQueueMissing) FailedDungeonSlots.AddUnique(Slot);
 }
}

void AMoonfallUnrealGameMode::SpawnDungeonBatch()
{
 if (!GetWorld() || bDungeonStartupNotified) return;
 // Keep the startup frame responsive. Thirty layouts take no Tick of their own;
 // the short timer stops entirely when all slots have been considered.
 constexpr int32 MaxLayoutsPerBatch = 2;
 int32 Processed = 0;
 // Retry each failed primary slot once, on a later timer pass.
 const bool bRetryPass = NextDungeonSlot >= TotalDungeons;
 while (Processed < MaxLayoutsPerBatch &&
     (bRetryPass ? NextFailedDungeonRetry < FailedDungeonSlots.Num() : NextDungeonSlot < TotalDungeons)) {
  const int32 Slot = bRetryPass ? FailedDungeonSlots[NextFailedDungeonRetry++] : NextDungeonSlot++;
  if (Slot < 0 || Slot >= TotalDungeons) break;
  ++Processed;
  const int32 Zone = Slot / DungeonsPerZone;
  const int32 DungeonIndex = Slot % DungeonsPerZone;
  // A streamed/authored layout may arrive or vanish after the initial scan.
  // Recheck both primary and retry attempts before generating geometry.
  bool bExisting = false;
  for (TActorIterator<AMoonfallDungeonLayout> It(GetWorld()); It; ++It) {
   if (IsValid(*It) && !It->IsActorBeingDestroyed()
       && It->ZoneIndex == Zone && It->DungeonIndex == DungeonIndex) {
    bExisting = true;
    break;
   }
  }
  if (bExisting) {
   if (!AuthoredDungeonSlots[Slot]) ++ReadyDungeonCount;
   continue;
  }
  if (AuthoredDungeonSlots[Slot]) {
   AuthoredDungeonSlots[Slot] = 0;
   ReadyDungeonCount = FMath::Max(0, ReadyDungeonCount - 1);
  }
  FTransform DungeonTransform;
  if (!GetDungeonEntranceTransform(Zone, DungeonIndex, DungeonTransform)) {
   if (!bRetryPass) FailedDungeonSlots.AddUnique(Slot);
   continue;
  }
  AMoonfallDungeonLayout* Layout = GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>(
    AMoonfallDungeonLayout::StaticClass(), DungeonTransform, nullptr, nullptr,
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (!IsValid(Layout)) {
   if (!bRetryPass) FailedDungeonSlots.AddUnique(Slot);
   continue;
  }
  Layout->ZoneIndex = Zone;
  Layout->DungeonIndex = DungeonIndex;
  Layout->bRandomizeChambers = true;
  Layout->FinishSpawning(DungeonTransform);
  if (IsValid(Layout) && !Layout->IsActorBeingDestroyed()) ++ReadyDungeonCount;
  else if (!bRetryPass) FailedDungeonSlots.AddUnique(Slot);
 }
 if (NextDungeonSlot >= TotalDungeons && !bDungeonStartupAuditComplete) {
  bDungeonStartupAuditComplete = true;
  AuditLiveDungeonSlots(true);
 }
 if (NextDungeonSlot >= TotalDungeons && NextFailedDungeonRetry >= FailedDungeonSlots.Num()) {
  // Recount distinct live slots after the bounded retry phase. Do not
  // claim a destroyed or duplicate actor as a completed dungeon.
  AuditLiveDungeonSlots(false);
  bDungeonStartupNotified = true;
  GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);
  if (ReadyDungeonCount != TotalDungeons)
   UE_LOG(LogTemp, Warning, TEXT("Moonfall dungeon world: only %d/%d layouts ready"),
     ReadyDungeonCount, TotalDungeons);
  OnDungeonWorldReady(ReadyDungeonCount);
 }
}
