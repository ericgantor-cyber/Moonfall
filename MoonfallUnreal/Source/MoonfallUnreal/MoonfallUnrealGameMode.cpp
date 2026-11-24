#include "MoonfallUnrealGameMode.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallArena.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallCombatHUD.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallEnemy.h"
#include "MoonfallLivingSimulation.h"
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
 SpawnDungeonBatch();
 if (NextDungeonSlot < TotalDungeons)
  GetWorldTimerManager().SetTimer(DungeonSpawnTimer, this,
    &AMoonfallUnrealGameMode::SpawnDungeonBatch, 0.1f, true);
}

void AMoonfallUnrealGameMode::PulseLivingWorld() {
 UWorld* World=GetWorld();
 if (!World) return;
 const float Now=World->GetTimeSeconds();
 if (!FMath::IsFinite(Now) || Now<0.f) return;
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
       It && Scanned<MoonfallLivingSimulation::ResidentBudget; ++It,++Scanned) {
   if (IsValid(*It) && !(*It)->IsActorBeingDestroyed() && !(*It)->bAmbientVisitor)
    LivingResidents.Add(*It);
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
 if (!bHeroReady) return;
 for (const TWeakObjectPtr<AMoonfallWorldNPC>& Ref:LivingResidents) {
  AMoonfallWorldNPC* NPC=Ref.Get();
  if (!IsValid(NPC) || NPC->IsActorBeingDestroyed()) continue;
  const float DistSq=FVector::DistSquared2D(HeroLocation,NPC->GetActorLocation());
  if (!FMath::IsFinite(DistSq) || DistSq>FMath::Square(1600.f)) continue;
  if (bCombatNearby && DistSq<FMath::Square(700.f))
   NPC->WitnessNearbyDanger(Now);
  NPC->AdvanceLivingRoutine(Now,HeroLocation);
 }
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
  FActorSpawnParameters Params;
  Params.Owner=this;
  Params.SpawnCollisionHandlingOverride=
     ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
  AMoonfallWorldNPC* Visitor=World->SpawnActor<AMoonfallWorldNPC>(
      AMoonfallWorldNPC::StaticClass(),SpawnTransform,Params);
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
void AMoonfallUnrealGameMode::SpawnDungeonBatch()
{
 if (!GetWorld()) return;
 // Keep the startup frame responsive. Thirty layouts take no Tick of their own;
 // the short timer stops entirely when all slots have been considered.
 constexpr int32 MaxLayoutsPerBatch = 2;
 int32 Processed = 0;
 while (NextDungeonSlot < TotalDungeons && Processed < MaxLayoutsPerBatch) {
  const int32 Slot = NextDungeonSlot++;
  ++Processed;
  if (AuthoredDungeonSlots.IsValidIndex(Slot) && AuthoredDungeonSlots[Slot])
   continue;
  const int32 Zone = Slot / DungeonsPerZone;
  const int32 DungeonIndex = Slot % DungeonsPerZone;
  FTransform DungeonTransform;
  if (!GetDungeonEntranceTransform(Zone, DungeonIndex, DungeonTransform))
   continue;
  AMoonfallDungeonLayout* Layout = GetWorld()->SpawnActorDeferred<AMoonfallDungeonLayout>(
    AMoonfallDungeonLayout::StaticClass(), DungeonTransform, nullptr, nullptr,
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (!IsValid(Layout)) continue;
  Layout->ZoneIndex = Zone;
  Layout->DungeonIndex = DungeonIndex;
  Layout->bRandomizeChambers = true;
  Layout->FinishSpawning(DungeonTransform);
  if (IsValid(Layout) && !Layout->IsActorBeingDestroyed()) ++ReadyDungeonCount;
 }
 if (NextDungeonSlot >= TotalDungeons) {
  GetWorldTimerManager().ClearTimer(DungeonSpawnTimer);
  if (ReadyDungeonCount != TotalDungeons)
   UE_LOG(LogTemp, Warning, TEXT("Moonfall dungeon world: only %d/%d layouts ready"),
     ReadyDungeonCount, TotalDungeons);
  OnDungeonWorldReady(ReadyDungeonCount);
 }
}
