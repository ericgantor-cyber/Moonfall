#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallQuestGuide.h"
#include "MoonfallWorldProgressSave.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
AMoonfallArena::AMoonfallArena() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.25f;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}
void AMoonfallArena::CreateBlock(FVector Position, FVector Scale) {
 UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this);
 Block->SetupAttachment(GetRootComponent());
 Block->RegisterComponent();
 Block->SetRelativeLocation(Position);
 Block->SetRelativeScale3D(Scale);
 Block->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube) Block->SetStaticMesh(Cube);
}
void AMoonfallArena::BeginPlay() {
 Super::BeginPlay();
 if (bContinuousWorld) { LoadWorldProgress(); BuildWorld(); return; }
 CreateBlock(FVector(0,0,-60),FVector(30,30,1));
 CreateBlock(FVector(1500,0,200),FVector(1,30,5));
 CreateBlock(FVector(-1500,0,200),FVector(1,30,5));
 CreateBlock(FVector(0,1500,200),FVector(30,1,5));
 CreateBlock(FVector(0,-1500,200),FVector(30,1,5));
 if (!GetWorld()) return;
 const int32 Count = FMath::Clamp(EnemyCount,0,12);
 for (int32 i=0;i<Count;++i) {
  float Angle = 2.f * PI * i / FMath::Max(1,Count);
  FVector Position = GetActorLocation() + FVector(450.f*FMath::Cos(Angle),450.f*FMath::Sin(Angle),60.f);
  // Defer BeginPlay until elite/boss properties are configured.
  const FTransform SpawnTransform(FRotator::ZeroRotator, Position);
  AMoonfallEnemy* Enemy = Cast<AMoonfallEnemy>(UGameplayStatics::BeginDeferredActorSpawnFromClass(
    this, AMoonfallEnemy::StaticClass(), SpawnTransform));
  if (!Enemy) continue;
  if (i == Count - 1 && Count >= 4) {
   Enemy->bElite = true;
   Enemy->bBoss = true;
   Enemy->Health *= 4.f;
   Enemy->AttackCooldown *= 1.15f;
  }
  UGameplayStatics::FinishSpawningActor(Enemy, SpawnTransform);
 }
}

namespace {
constexpr int32 Regions = 6;
constexpr int32 DungeonsPerRegion = 5;
constexpr int32 ChambersPerDungeon = 14;
constexpr float RegionSpacing = 6000.f;
struct FRegionalEnemyProfile {
 const TCHAR* Name;
 float HealthScale;
 float DamageScale;
 float SpeedScale;
 float ReachScale;
 float CooldownScale;
 float PostureScale;
};
// Six biome families, three genuinely different combat stat profiles in each.
// Uses existing melee/chase/posture behavior; never allocates an AI per profile.
constexpr FRegionalEnemyProfile RegionalEnemyProfiles[18] = {
 {TEXT("Ashling Skirmisher"), 0.85f, 0.85f, 1.38f, 0.85f, 0.82f, 0.80f},
 {TEXT("Cinderback Brute"), 1.50f, 1.28f, 0.70f, 1.05f, 1.25f, 1.55f},
 {TEXT("Emberclaw Warden"), 1.80f, 1.40f, 0.95f, 1.15f, 1.10f, 1.70f},
 {TEXT("Mirewing Stalker"), 0.95f, 0.98f, 1.48f, 0.83f, 0.80f, 0.85f},
 {TEXT("Bogroot Crusher"), 1.65f, 1.26f, 0.67f, 1.10f, 1.32f, 1.60f},
 {TEXT("Fenfang Alpha"), 1.85f, 1.42f, 1.05f, 1.12f, 1.10f, 1.80f},
 {TEXT("Frostbite Runner"), 0.84f, 1.03f, 1.50f, 0.82f, 0.75f, 0.82f},
 {TEXT("Glacial Bulwark"), 1.85f, 1.20f, 0.62f, 1.15f, 1.40f, 1.85f},
 {TEXT("Rimehorn Ravager"), 1.75f, 1.52f, 1.07f, 1.20f, 1.05f, 1.55f},
 {TEXT("Stormveil Prowler"), 0.92f, 1.10f, 1.48f, 0.93f, 0.76f, 0.90f},
 {TEXT("Thundershell Sentinel"), 1.70f, 1.26f, 0.69f, 1.13f, 1.25f, 1.70f},
 {TEXT("Tempest Fang"), 1.85f, 1.48f, 1.16f, 1.08f, 0.95f, 1.70f},
 {TEXT("Ironwhisker Scout"), 0.96f, 0.95f, 1.37f, 0.83f, 0.85f, 0.90f},
 {TEXT("Foundrybreaker"), 1.90f, 1.30f, 0.63f, 1.17f, 1.45f, 2.00f},
 {TEXT("Sparkcoil Executioner"), 1.80f, 1.55f, 1.10f, 1.15f, 0.92f, 1.85f},
 {TEXT("Moonshade Pouncer"), 0.90f, 1.14f, 1.55f, 0.84f, 0.74f, 0.85f},
 {TEXT("Voidplate Colossus"), 2.00f, 1.40f, 0.61f, 1.20f, 1.50f, 2.10f},
 {TEXT("Eclipse Reaver"), 1.95f, 1.65f, 1.12f, 1.19f, 0.94f, 1.95f}
};


// Second cohort uses stable IDs 438..455; never remap the original 18 or dungeon IDs.
constexpr int32 ExtraRegionalEncounterBase = 18 + Regions * DungeonsPerRegion * ChambersPerDungeon;
constexpr FRegionalEnemyProfile ExtraRegionalEnemyProfiles[18] = {
 {TEXT("Ashen Viper"), 1.12f, 1.20f, 1.28f, 1.05f, 0.82f, 0.95f},
 {TEXT("Magma Tusk"), 1.80f, 1.35f, 0.68f, 1.15f, 1.32f, 1.75f},
 {TEXT("Coalwing Harrier"), 0.92f, 1.04f, 1.52f, 0.88f, 0.76f, 0.82f},
 {TEXT("Sporefang Hunter"), 1.06f, 1.10f, 1.35f, 1.12f, 0.89f, 1.05f},
 {TEXT("Mossbound Titan"), 1.95f, 1.38f, 0.64f, 1.18f, 1.42f, 1.95f},
 {TEXT("Rotclaw Ambusher"), 1.18f, 1.33f, 1.26f, 0.95f, 0.83f, 1.15f},
 {TEXT("Snowveil Lynx"), 0.91f, 1.22f, 1.52f, 0.88f, 0.78f, 0.83f},
 {TEXT("Frostmaw Behemoth"), 2.00f, 1.42f, 0.62f, 1.20f, 1.48f, 2.05f},
 {TEXT("Iceglass Stalker"), 1.10f, 1.15f, 1.38f, 1.06f, 0.85f, 1.00f},
 {TEXT("Lightning Jackal"), 1.00f, 1.23f, 1.55f, 0.90f, 0.79f, 0.88f},
 {TEXT("Thunderhoof Juggernaut"), 1.92f, 1.45f, 0.66f, 1.16f, 1.40f, 1.92f},
 {TEXT("Cloudpiercer"), 1.08f, 1.35f, 1.33f, 1.12f, 0.90f, 1.06f},
 {TEXT("Rivet Hound"), 1.05f, 1.14f, 1.45f, 0.91f, 0.82f, 0.95f},
 {TEXT("Slagforge Goliath"), 2.00f, 1.46f, 0.61f, 1.22f, 1.48f, 2.10f},
 {TEXT("Voltspine Pursuer"), 1.20f, 1.37f, 1.29f, 1.08f, 0.89f, 1.20f},
 {TEXT("Nightglass Wraith"), 0.96f, 1.28f, 1.51f, 0.87f, 0.77f, 0.86f},
 {TEXT("Abyssal Hornbeast"), 2.05f, 1.51f, 0.63f, 1.19f, 1.47f, 2.12f},
 {TEXT("Dreadmoon Ravager"), 1.35f, 1.55f, 1.25f, 1.10f, 0.88f, 1.35f}
};
FVector RoomCenter(int32 Region, int32 Dungeon, int32 Chamber) {
 return FVector(Region * RegionSpacing - 2200.f + Dungeon * 1100.f,
 3600.f + Chamber * 780.f, 0.f);
}
}
void AMoonfallArena::BuildWorld() {
 // One physical strip of six touching regions with five dungeon corridors per region.
 // The existing small test arena remains available when bContinuousWorld is false.
 for (int32 Region = 0; Region < Regions; ++Region) {
  CreateBlock(FVector(Region * RegionSpacing, 0.f, -60.f), FVector(60.f, 60.f, 1.f));
  // Three distinct residents per region: guide, artisan and scout. No waves.
  for (int32 Role = 0; Role < 3; ++Role) {
   const FVector NpcAt(Region * RegionSpacing + 700.f + Role * 230.f,
     350.f + (Role % 2) * 180.f, 105.f);
   if (AMoonfallQuestGuide* Npc = GetWorld()->SpawnActor<AMoonfallQuestGuide>(
       AMoonfallQuestGuide::StaticClass(), NpcAt, FRotator::ZeroRotator)) {
    Npc->RegionIndex = Region;
    Npc->RoleIndex = Role;
   }
  }
  for (int32 Dungeon = 0; Dungeon < DungeonsPerRegion; ++Dungeon) {
   const FVector Entrance = RoomCenter(Region, Dungeon, 0);
   CreateBlock(FVector(Entrance.X, 8650.f, -60.f), FVector(3.5f, 120.f, 1.f));
  }
 }
 StreamWorld();
}
bool AMoonfallArena::SpawnEncounter(int32 Id, FVector Position, int32 Region, bool Elite, bool Boss) {
 if (!GetWorld() || SpawnedEncounters.Contains(Id) || DefeatedEncounters.Contains(Id)) return false;
 const FTransform Transform(FRotator::ZeroRotator, Position + FVector(0.f, 0.f, 110.f));
 AMoonfallEnemy* Enemy = Cast<AMoonfallEnemy>(
  UGameplayStatics::BeginDeferredActorSpawnFromClass(this, AMoonfallEnemy::StaticClass(), Transform));
 if (!Enemy) return false;
 Enemy->bElite = Elite;
 Enemy->bBoss = Boss;
 if ((Id >= 0 && Id < 18) ||
     (Id >= ExtraRegionalEncounterBase && Id < ExtraRegionalEncounterBase + 18)) {
  const bool bExtra = Id >= ExtraRegionalEncounterBase;
  const int32 ProfileIndex = bExtra ? Id - ExtraRegionalEncounterBase : Id;
  const FRegionalEnemyProfile& Profile = bExtra
    ? ExtraRegionalEnemyProfiles[ProfileIndex] : RegionalEnemyProfiles[ProfileIndex];
  Enemy->ArchetypeName = FName(Profile.Name);
  Enemy->RegionalAttackStyle = ProfileIndex % 6;
  Enemy->bElite = ProfileIndex % 3 == 2;
  Enemy->Health = FMath::Clamp(Enemy->Health * Profile.HealthScale, 1.f, 100000.f);
  Enemy->AttackDamage = FMath::Clamp(Enemy->AttackDamage * Profile.DamageScale, 1.f, 10000.f);
  Enemy->ChaseSpeed = FMath::Clamp(Enemy->ChaseSpeed * Profile.SpeedScale, 60.f, 600.f);
  Enemy->AttackRange = FMath::Clamp(Enemy->AttackRange * Profile.ReachScale, 90.f, 240.f);
  Enemy->AttackCooldown = FMath::Clamp(Enemy->AttackCooldown * Profile.CooldownScale, 0.65f, 3.5f);
  Enemy->MaxPosture = FMath::Clamp(Enemy->MaxPosture * Profile.PostureScale, 40.f, 500.f);
 }

 const float Scale = 1.f + 0.22f * FMath::Clamp(Region, 0, Regions - 1);
 Enemy->Health = FMath::Clamp(Enemy->Health * Scale * (Boss ? 4.f : Elite ? 1.7f : 1.f), 1.f, 100000.f);
 Enemy->AttackDamage = FMath::Clamp(Enemy->AttackDamage * Scale * (Boss ? 1.6f : 1.f), 1.f, 10000.f);
 UGameplayStatics::FinishSpawningActor(Enemy, Transform);
 SpawnedEncounters.Add(Id);
 ActiveEncounters.Add(Id, Enemy); // Observe real combat deaths before saving progress.
 return true;
}
void AMoonfallArena::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (bContinuousWorld) {
  ObserveDefeatedEncounters();
  AdvanceWorkshopProduction();
  // Rewards come from unique persisted guardian defeat IDs, not hit callbacks.
  // Each player pawn applies only the difference, including after save reload.
  if (AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
   Hero->ApplyGuardianQuestRewards(GetCompletedDungeonQuestCount());
  if (bWorldSavePending) SaveWorldProgress();
  StreamWorld();
  if (const APawn* LocalHero = UGameplayStatics::GetPlayerPawn(this, 0))
   StreamResidents(LocalHero->GetActorLocation());
 }
}
void AMoonfallArena::StreamWorld() {
 if (!GetWorld()) return;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero)) return;
 const FVector Here = Hero->GetActorLocation();
 bool BuiltOne = false, SpawnedOne = false;
 // One chamber geometry allocation and at most one hostile spawn per 0.25s scan.
 for (int32 Region = 0; Region < Regions; ++Region) {
  const FVector RegionPos(Region * RegionSpacing, 0.f, 0.f);
  if (FVector::DistSquared2D(Here, RegionPos) < 2200.f * 2200.f
      && !DiscoveredRegions.Contains(Region)) {
   DiscoveredRegions.Add(Region);
   bWorldSavePending = true;
  }
  // Entrance markers are earned through traversal, not proximity streaming.
  for (int32 Dungeon = 0; Dungeon < DungeonsPerRegion; ++Dungeon) {
   const int32 EntranceId = Region * DungeonsPerRegion + Dungeon;
   const FVector Entrance = RoomCenter(Region, Dungeon, 0);
   if (FVector::DistSquared2D(Here, Entrance) < 650.f * 650.f
       && !DiscoveredDungeonEntrances.Contains(EntranceId)) {
    DiscoveredDungeonEntrances.Add(EntranceId);
    if (TrackedDungeonQuestId < 0) TrackedDungeonQuestId = EntranceId;
    bWorldSavePending = true;
   }
  }
  for (int32 Index = 0; Index < 3 && !SpawnedOne; ++Index) {
   const int32 Id = Region * 3 + Index;
   const FVector At(Region * RegionSpacing - 650.f + Index * 650.f,
    700.f + (Index % 2) * 350.f, 0.f);
   if (!SpawnedEncounters.Contains(Id) && FVector::DistSquared2D(Here, At) < 850.f * 850.f)
    SpawnedOne = SpawnEncounter(Id, At, Region, false, false);
  }
  // Three additional native wildlife encounters per biome, using a separate
  // ID range so old saves never reinterpret existing dungeon or monster kills.
  for (int32 Index = 0; Index < 3 && !SpawnedOne; ++Index) {
   const int32 Id = ExtraRegionalEncounterBase + Region * 3 + Index;
   const FVector At(Region * RegionSpacing - 900.f + Index * 900.f,
    -800.f - (Index % 2) * 450.f, 0.f);
   if (!SpawnedEncounters.Contains(Id) && !DefeatedEncounters.Contains(Id)
       && FVector::DistSquared2D(Here, At) < 850.f * 850.f)
    SpawnedOne = SpawnEncounter(Id, At, Region, false, false);
  }
  for (int32 Dungeon = 0; Dungeon < DungeonsPerRegion; ++Dungeon) {
   for (int32 Chamber = 0; Chamber < ChambersPerDungeon; ++Chamber) {
    const int32 Room = (Region * DungeonsPerRegion + Dungeon) * ChambersPerDungeon + Chamber;
    const FVector At = RoomCenter(Region, Dungeon, Chamber);
    const float Distance = FVector::DistSquared2D(Here, At);
    // Visiting a chamber reveals it permanently; merely streaming geometry
    // within draw distance does not expose unexplored locations on the map.
    if (Distance < 450.f * 450.f && !DiscoveredChambers.Contains(Room)) {
     DiscoveredChambers.Add(Room);
     bWorldSavePending = true;
    }
    if (!BuiltOne && !BuiltRooms.Contains(Room) && Distance < 1500.f * 1500.f) {
     CreateBlock(At + FVector(0.f, 0.f, -60.f), FVector(7.f, 6.f, 1.f));
     BuiltRooms.Add(Room);
     BuiltOne = true;
    }
    const int32 Stage = Chamber + 1;
    if (Stage != 3 && Stage != 5 && Stage != 9 && Stage != 12 && Stage != 14) continue;
    // 120 residents and 30 final guardians, plus eighteen overworld monsters.
    const int32 Id = 18 + Room;
    if (!SpawnedOne && BuiltRooms.Contains(Room) && !SpawnedEncounters.Contains(Id)
        && Distance < 850.f * 850.f)
     SpawnedOne = SpawnEncounter(Id, At, Region, Stage >= 9, Stage == 14);
   }
  }
 }
}

namespace {
const FString WorldProgressSlot = TEXT("MoonfallUnrealWorldProgress_v1");
constexpr int32 WorldEncounterUpperBound = 18 + Regions * DungeonsPerRegion * ChambersPerDungeon;
}
bool AMoonfallArena::IsValidEncounterId(int32 Id) {
 if (Id >= 0 && Id < 18) return true;
 if (Id >= ExtraRegionalEncounterBase && Id < ExtraRegionalEncounterBase + 18) return true;
 if (Id < 18 || Id >= WorldEncounterUpperBound) return false;
 const int32 Stage = ((Id - 18) % ChambersPerDungeon) + 1;
 return Stage == 3 || Stage == 5 || Stage == 9 || Stage == 12 || Stage == 14;
}
void AMoonfallArena::LoadWorldProgress() {
 DefeatedEncounters.Empty();
 DiscoveredChambers.Empty();
 DiscoveredRegions.Empty();
 DiscoveredDungeonEntrances.Empty();
 TrackedDungeonQuestId = -1;
 GuideTrustMemory.Init(0, Regions);
 GuideFearMemory.Init(0, Regions);
 NpcTrustMemory.Init(0, Regions * 3);
 NpcFearMemory.Init(0, Regions * 3);
 LastNpcSocialActionAt.Empty();
 ResidentTrustMemory.Init(0, 1111 - Regions * 3);
 ResidentFearMemory.Init(0, 1111 - Regions * 3);
 WorkshopStock.Init(3, Regions);
 NextWorkshopProductionAt = GetWorld() ? GetWorld()->GetTimeSeconds() + 120.f : 120.f;
 ActiveResidents.Empty();
 PopulationCursor = Regions * 3;
 LastGuideSocialActionAt.Empty();
 if (!UGameplayStatics::DoesSaveGameExist(WorldProgressSlot, 0)) return;
 UMoonfallWorldProgressSave* Saved = Cast<UMoonfallWorldProgressSave>(
  UGameplayStatics::LoadGameFromSlot(WorldProgressSlot, 0));
 if (!Saved || Saved->SchemaVersion != 1) return;
 for (const int32 Id : Saved->DefeatedEncounterIds) {
  if (IsValidEncounterId(Id)) DefeatedEncounters.Add(Id);
 }
 // Schema v1 accepts 36 wildlife and 150 dungeon encounters; no ID reuse.
 if (DefeatedEncounters.Num() > 186) DefeatedEncounters.Empty();
 for (const int32 Region : Saved->DiscoveredRegionIds) {
  if (Region >= 0 && Region < Regions) DiscoveredRegions.Add(Region);
 }
 for (const int32 Entrance : Saved->DiscoveredDungeonEntranceIds) {
  if (Entrance >= 0 && Entrance < Regions * DungeonsPerRegion)
   DiscoveredDungeonEntrances.Add(Entrance);
 }
 // A quest is trackable only when its entrance was actually discovered.
 if (Saved->TrackedDungeonQuestId >= 0 &&
     Saved->TrackedDungeonQuestId < Regions * DungeonsPerRegion &&
     DiscoveredDungeonEntrances.Contains(Saved->TrackedDungeonQuestId))
  TrackedDungeonQuestId = Saved->TrackedDungeonQuestId;
 else if (!DiscoveredDungeonEntrances.IsEmpty()) {
  // Older saves gain a deterministic known objective, never an undiscovered one.
  int32 FirstKnown = Regions * DungeonsPerRegion;
  for (const int32 Entrance : DiscoveredDungeonEntrances)
   FirstKnown = FMath::Min(FirstKnown, Entrance);
  TrackedDungeonQuestId = FirstKnown;
 }
 // Older save slots have no social arrays. Ignore malformed or oversized data.
 if (Saved->GuideTrustMemory.Num() == Regions) {
  for (int32 Region = 0; Region < Regions; ++Region)
   GuideTrustMemory[Region] = FMath::Clamp(Saved->GuideTrustMemory[Region], -100, 100);
 }
 if (Saved->GuideFearMemory.Num() == Regions) {
  for (int32 Region = 0; Region < Regions; ++Region)
   GuideFearMemory[Region] = FMath::Clamp(Saved->GuideFearMemory[Region], 0, 100);
 }
 // Upgrade older regional relationships to the original six guide identities.
 for (int32 Region = 0; Region < Regions; ++Region) {
  NpcTrustMemory[Region * 3] = GuideTrustMemory[Region];
  NpcFearMemory[Region * 3] = GuideFearMemory[Region];
 }
 if (Saved->NpcTrustMemory.Num() == Regions * 3) {
  for (int32 Npc = 0; Npc < Regions * 3; ++Npc)
   NpcTrustMemory[Npc] = FMath::Clamp(Saved->NpcTrustMemory[Npc], -100, 100);
 }
 if (Saved->NpcFearMemory.Num() == Regions * 3) {
  for (int32 Npc = 0; Npc < Regions * 3; ++Npc)
   NpcFearMemory[Npc] = FMath::Clamp(Saved->NpcFearMemory[Npc], 0, 100);
 }
 if (Saved->ResidentTrustMemory.Num() == 1111 - Regions * 3) {
  for (int32 Id = 0; Id < ResidentTrustMemory.Num(); ++Id)
   ResidentTrustMemory[Id] = FMath::Clamp(Saved->ResidentTrustMemory[Id], -100, 100);
 }
 if (Saved->ResidentFearMemory.Num() == 1111 - Regions * 3) {
  for (int32 Id = 0; Id < ResidentFearMemory.Num(); ++Id)
   ResidentFearMemory[Id] = FMath::Clamp(Saved->ResidentFearMemory[Id], 0, 100);
 }
 if (Saved->WorkshopStock.Num() == Regions) {
  for (int32 Region = 0; Region < Regions; ++Region)
   WorkshopStock[Region] = FMath::Clamp(Saved->WorkshopStock[Region], 0, 8);
 }
 for (const int32 Room : Saved->DiscoveredChamberIds) {
  if (Room >= 0 && Room < Regions * DungeonsPerRegion * ChambersPerDungeon)
   DiscoveredChambers.Add(Room);
 }
}
void AMoonfallArena::ObserveDefeatedEncounters() {
 for (auto It = ActiveEncounters.CreateIterator(); It; ++It) {
  AMoonfallEnemy* Enemy = It.Value().Get();
  // Only an observed lethal combat hit is counted as a victory. An actor being
  // garbage-collected, unloaded or deleted is not proof that it was defeated.
  if (IsValid(Enemy) && FMath::IsFinite(Enemy->Health) && Enemy->Health <= 0.f) {
   const int32 Id = It.Key();
   if (IsValidEncounterId(Id) && !DefeatedEncounters.Contains(Id)) {
    DefeatedEncounters.Add(Id);
    bWorldSavePending = true;
   }
   It.RemoveCurrent();
  } else if (!IsValid(Enemy)) {
   It.RemoveCurrent();
  }
 }
}
void AMoonfallArena::SaveWorldProgress() {
 if (!GetWorld() || !bWorldSavePending) return;
 UMoonfallWorldProgressSave* Saved = Cast<UMoonfallWorldProgressSave>(
  UGameplayStatics::CreateSaveGameObject(UMoonfallWorldProgressSave::StaticClass()));
 if (!Saved) return;
 for (const int32 Id : DefeatedEncounters) {
  if (IsValidEncounterId(Id)) Saved->DefeatedEncounterIds.Add(Id);
 }
 Saved->DefeatedEncounterIds.Sort();
 for (const int32 Room : DiscoveredChambers) {
  if (Room >= 0 && Room < Regions * DungeonsPerRegion * ChambersPerDungeon)
   Saved->DiscoveredChamberIds.Add(Room);
 }
 Saved->DiscoveredChamberIds.Sort();
 for (const int32 Region : DiscoveredRegions) {
  if (Region >= 0 && Region < Regions) Saved->DiscoveredRegionIds.Add(Region);
 }
 Saved->DiscoveredRegionIds.Sort();
 for (const int32 Entrance : DiscoveredDungeonEntrances) {
  if (Entrance >= 0 && Entrance < Regions * DungeonsPerRegion)
   Saved->DiscoveredDungeonEntranceIds.Add(Entrance);
 }
 Saved->DiscoveredDungeonEntranceIds.Sort();
 Saved->GuideTrustMemory = GuideTrustMemory;
 Saved->GuideFearMemory = GuideFearMemory;
 Saved->NpcTrustMemory = NpcTrustMemory;
 Saved->NpcFearMemory = NpcFearMemory;
 Saved->ResidentTrustMemory = ResidentTrustMemory;
 Saved->ResidentFearMemory = ResidentFearMemory;
 Saved->WorkshopStock = WorkshopStock;
 Saved->TrackedDungeonQuestId = DiscoveredDungeonEntrances.Contains(TrackedDungeonQuestId)
  ? TrackedDungeonQuestId : -1;
 if (UGameplayStatics::SaveGameToSlot(Saved, WorldProgressSlot, 0))
  bWorldSavePending = false;
}

bool AMoonfallArena::IsDungeonQuestCompleted(int32 EntranceId) const {
 if (EntranceId < 0 || EntranceId >= Regions * DungeonsPerRegion) return false;
 // Every dungeon has 14 chambers; its final guardian is in chamber 14.
 const int32 GuardianEncounterId = 18 + EntranceId * ChambersPerDungeon + (ChambersPerDungeon - 1);
 return DefeatedEncounters.Contains(GuardianEncounterId);
}
int32 AMoonfallArena::GetDungeonResidentDefeatCount(int32 EntranceId) const {
 if (EntranceId < 0 || EntranceId >= Regions * DungeonsPerRegion) return 0;
 const int32 Stages[] = {3, 5, 9, 12};
 int32 Defeated = 0;
 for (const int32 Stage : Stages) {
  const int32 EncounterId = 18 + EntranceId * ChambersPerDungeon + (Stage - 1);
  if (DefeatedEncounters.Contains(EncounterId)) ++Defeated;
 }
 return Defeated;
}
int32 AMoonfallArena::GetCompletedDungeonQuestCount() const {
 int32 Completed = 0;
 for (int32 EntranceId = 0; EntranceId < Regions * DungeonsPerRegion; ++EntranceId) {
  if (IsDungeonQuestCompleted(EntranceId)) ++Completed;
 }
 return Completed;
}
bool AMoonfallArena::TrackDungeonQuest(int32 EntranceId) {
 // Blueprint/UI can only track an entrance the hero has actually found.
 if (!bContinuousWorld || !DiscoveredDungeonEntrances.Contains(EntranceId)) return false;
 if (TrackedDungeonQuestId != EntranceId) {
  TrackedDungeonQuestId = EntranceId;
  bWorldSavePending = true;
 }
 return true;
}

bool AMoonfallArena::SelectNextDiscoveredDungeonQuestInRegion(int32 RegionIndex) {
 if (!bContinuousWorld || RegionIndex < 0 || RegionIndex >= Regions) return false;
 // Deterministic rotation among visited entrances, with unfinished quests first.
 const int32 Start = RegionIndex * DungeonsPerRegion;
 const int32 Current = TrackedDungeonQuestId;
 for (int32 Pass = 0; Pass < 2; ++Pass) {
  for (int32 Offset = 1; Offset <= DungeonsPerRegion; ++Offset) {
   const int32 CurrentLocal = Current >= Start && Current < Start + DungeonsPerRegion
      ? Current - Start : -1;
   const int32 EntranceId = Start + (CurrentLocal + Offset) % DungeonsPerRegion;
   if (!DiscoveredDungeonEntrances.Contains(EntranceId)) continue;
   if (Pass == 0 && IsDungeonQuestCompleted(EntranceId)) continue;
   return TrackDungeonQuest(EntranceId);
  }
 }
 return false;
}

int32 AMoonfallArena::GetGuideTrust(int32 RegionIndex) const {
 if (RegionIndex < 0 || RegionIndex >= Regions) return 0;
 const int32 Base = GuideTrustMemory.IsValidIndex(RegionIndex)
   ? FMath::Clamp(GuideTrustMemory[RegionIndex], -100, 100) : 0;
 // Recognize only verified guardian victories in this guide's region.
 int32 GuardianReputation = 0;
 for (int32 Dungeon = 0; Dungeon < DungeonsPerRegion; ++Dungeon)
  if (IsDungeonQuestCompleted(RegionIndex * DungeonsPerRegion + Dungeon))
   GuardianReputation += 8;
 return FMath::Clamp(Base + GuardianReputation, -100, 100);
}
int32 AMoonfallArena::GetGuideFear(int32 RegionIndex) const {
 if (RegionIndex < 0 || RegionIndex >= Regions ||
     !GuideFearMemory.IsValidIndex(RegionIndex)) return 0;
 return FMath::Clamp(GuideFearMemory[RegionIndex], 0, 100);
}
bool AMoonfallArena::RecordGuideSocialChoice(int32 RegionIndex, bool bIntimidate) {
 if (!bContinuousWorld || !GetWorld() || RegionIndex < 0 || RegionIndex >= Regions
     || !DiscoveredRegions.Contains(RegionIndex)
     || !GuideTrustMemory.IsValidIndex(RegionIndex)
     || !GuideFearMemory.IsValidIndex(RegionIndex)) return false;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now)) return false;
 const float* Previous = LastGuideSocialActionAt.Find(RegionIndex);
 // Avoid farming relationship points through repeated input presses.
 if (Previous && Now - *Previous < 30.f) return false;
 LastGuideSocialActionAt.Add(RegionIndex, Now);
 GuideTrustMemory[RegionIndex] = FMath::Clamp(
  GuideTrustMemory[RegionIndex] + (bIntimidate ? -10 : 7), -100, 100);
 GuideFearMemory[RegionIndex] = FMath::Clamp(
  GuideFearMemory[RegionIndex] + (bIntimidate ? 12 : -4), 0, 100);
 bWorldSavePending = true;
 return true;
}

int32 AMoonfallArena::GetNpcTrust(int32 NpcId) const {
 if (NpcId >= Regions * 3 && NpcId < 1111) {
  const int32 Idx = NpcId - Regions * 3;
  if (!ResidentTrustMemory.IsValidIndex(Idx)) return 0;
  return FMath::Clamp(ResidentTrustMemory[Idx], -100, 100);
 }
 if (NpcId < 0 || NpcId >= Regions * 3 || !NpcTrustMemory.IsValidIndex(NpcId)) return 0;
 int32 Deeds = 0;
 const int32 Region = NpcId / 3;
 for (int32 Dungeon = 0; Dungeon < DungeonsPerRegion; ++Dungeon)
  if (IsDungeonQuestCompleted(Region * DungeonsPerRegion + Dungeon))
   Deeds += (NpcId % 3 == 2 ? 10 : NpcId % 3 == 1 ? 5 : 8);
 return FMath::Clamp(NpcTrustMemory[NpcId] + Deeds, -100, 100);
}
int32 AMoonfallArena::GetNpcFear(int32 NpcId) const {
 if (NpcId >= Regions * 3 && NpcId < 1111) {
  const int32 Idx = NpcId - Regions * 3;
  return ResidentFearMemory.IsValidIndex(Idx) ? FMath::Clamp(ResidentFearMemory[Idx], 0, 100) : 0;
 }
 if (NpcId < 0 || NpcId >= Regions * 3 || !NpcFearMemory.IsValidIndex(NpcId)) return 0;
 return FMath::Clamp(NpcFearMemory[NpcId], 0, 100);
}
bool AMoonfallArena::RecordNpcSocialChoice(int32 NpcId, bool bIntimidate) {
 if (NpcId >= Regions * 3 && NpcId < 1111) {
  const int32 Index = NpcId - Regions * 3;
  const int32 Region = Index % Regions;
  if (!bContinuousWorld || !GetWorld() || !DiscoveredRegions.Contains(Region)
      || !ResidentTrustMemory.IsValidIndex(Index) || !ResidentFearMemory.IsValidIndex(Index))
   return false;
  const float Now = GetWorld()->GetTimeSeconds();
  if (!FMath::IsFinite(Now)) return false;
  const float* Previous = LastNpcSocialActionAt.Find(NpcId);
  if (Previous && Now - *Previous < 30.f) return false;
  LastNpcSocialActionAt.Add(NpcId, Now);
  ResidentTrustMemory[Index] = FMath::Clamp(
   ResidentTrustMemory[Index] + (bIntimidate ? -10 : 7), -100, 100);
  ResidentFearMemory[Index] = FMath::Clamp(
   ResidentFearMemory[Index] + (bIntimidate ? 12 : -4), 0, 100);
  // Two neighbors in the same virtual town hear about the interaction.
  // A fixed one-hop update, never a cascading social graph traversal.
  for (int32 Offset : {-Regions, Regions}) {
   const int32 Neighbor = Index + Offset;
   if (!ResidentTrustMemory.IsValidIndex(Neighbor) ||
       Neighbor % Regions != Region) continue;
   ResidentTrustMemory[Neighbor] = FMath::Clamp(
     ResidentTrustMemory[Neighbor] + (bIntimidate ? -2 : 1), -100, 100);
  }
  bWorldSavePending = true;
  return true;
 }
 if (!bContinuousWorld || !GetWorld() || NpcId < 0 || NpcId >= Regions * 3
     || !DiscoveredRegions.Contains(NpcId / 3)
     || !NpcTrustMemory.IsValidIndex(NpcId) || !NpcFearMemory.IsValidIndex(NpcId)) return false;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now)) return false;
 const float* Previous = LastNpcSocialActionAt.Find(NpcId);
 if (Previous && Now - *Previous < 30.f) return false;
 LastNpcSocialActionAt.Add(NpcId, Now);
 // Personalities react differently: artisans appreciate kindness, guards fear
 // intimidation less, and scouts value heroic deeds more.
 const int32 Role = NpcId % 3;
 const int32 TrustDelta = bIntimidate ? (Role == 2 ? -13 : -10)
   : (Role == 1 ? 10 : 7);
 const int32 FearDelta = bIntimidate ? (Role == 2 ? 6 : 12) : -4;
 NpcTrustMemory[NpcId] = FMath::Clamp(NpcTrustMemory[NpcId] + TrustDelta, -100, 100);
 NpcFearMemory[NpcId] = FMath::Clamp(NpcFearMemory[NpcId] + FearDelta, 0, 100);
 // Local word of mouth: other residents react according to their established
 // bond with the target. Only the original social choice triggers gossip.
 const int32 Region = NpcId / 3;
 for (int32 OtherRole = 0; OtherRole < 3; ++OtherRole) {
  const int32 OtherId = Region * 3 + OtherRole;
  if (OtherId == NpcId || !NpcTrustMemory.IsValidIndex(OtherId)) continue;
  const int32 Bond = GetNpcBond(NpcId, OtherId);
  const int32 GossipDelta = bIntimidate
    ? (Bond >= 0 ? -3 : 1) : (Bond >= 0 ? 2 : -1);
  NpcTrustMemory[OtherId] = FMath::Clamp(NpcTrustMemory[OtherId] + GossipDelta, -100, 100);
 }
 // Keep the six legacy guide values synced when an artisan/scout's story spreads.
 GuideTrustMemory[Region] = NpcTrustMemory[Region * 3];
 GuideFearMemory[Region] = NpcFearMemory[Region * 3];
 if (Role == 0) {
  GuideTrustMemory[NpcId / 3] = NpcTrustMemory[NpcId];
  GuideFearMemory[NpcId / 3] = NpcFearMemory[NpcId];
 }
 bWorldSavePending = true;
 return true;
}

int32 AMoonfallArena::GetNpcBond(int32 NpcId, int32 OtherNpcId) const {
 if (NpcId < 0 || NpcId >= Regions * 3 || OtherNpcId < 0 ||
     OtherNpcId >= Regions * 3 || NpcId == OtherNpcId) return 0;
 // Bonds are symmetric and predetermined by roles, not random Tick outcomes.
 if (NpcId / 3 != OtherNpcId / 3) return 0;
 const int32 A = NpcId % 3;
 const int32 B = OtherNpcId % 3;
 if ((A == 0 && B == 1) || (A == 1 && B == 0)) return 60; // trusted partners
 if ((A == 0 && B == 2) || (A == 2 && B == 0)) return 35; // watchful allies
 return -25; // artisan/scout rivalry
}
int32 AMoonfallArena::GetRegionalFactionStanding(int32 RegionIndex) const {
 if (RegionIndex < 0 || RegionIndex >= Regions) return 0;
 int32 Sum = 0;
 for (int32 Role = 0; Role < 3; ++Role) {
  const int32 NpcId = RegionIndex * 3 + Role;
  Sum += GetNpcTrust(NpcId);
  Sum -= GetNpcFear(NpcId) / 2;
 }
 // Active social history from generated residents contributes without letting
 // neutral virtual residents drown out the original three named NPCs.
 int32 ActiveCount = 0;
 int32 ResidentSum = 0;
 for (int32 Index = RegionIndex; Index < ResidentTrustMemory.Num(); Index += Regions) {
  if (!ResidentFearMemory.IsValidIndex(Index)) break;
  const int32 Trust = ResidentTrustMemory[Index], Fear = ResidentFearMemory[Index];
  if (Trust == 0 && Fear == 0) continue;
  ResidentSum += Trust - Fear / 2;
  ++ActiveCount;
 }
 const int32 NamedStanding = FMath::RoundToInt(Sum / 3.f);
 if (ActiveCount == 0) return FMath::Clamp(NamedStanding, -100, 100);
 const int32 ResidentStanding = FMath::RoundToInt(
   static_cast<float>(ResidentSum) / ActiveCount);
 return FMath::Clamp(FMath::RoundToInt(
   NamedStanding * 0.7f + ResidentStanding * 0.3f), -100, 100);
}

FVector AMoonfallArena::GetResidentHome(int32 ResidentId) {
 // IDs 18..1110 distribute evenly across six regions, without storing locations.
 const int32 Local = (ResidentId - Regions * 3) / Regions;
 const int32 Region = (ResidentId - Regions * 3) % Regions;
 const int32 Column = Local % 15;
 const int32 Row = Local / 15;
 return FVector(Region * RegionSpacing - 1900.f + Column * 270.f,
   -1650.f + Row * 260.f, 105.f);
}
void AMoonfallArena::StreamResidents(const FVector& PlayerLocation) {
 if (!GetWorld()) return;
 // Unload far actors first. Defeated enemies and the 18 authored NPCs are unaffected.
 for (auto It = ActiveResidents.CreateIterator(); It; ++It) {
  AMoonfallQuestGuide* Npc = It.Value().Get();
  if (!IsValid(Npc)) { It.RemoveCurrent(); continue; }
  if (FVector::DistSquared2D(Npc->GetActorLocation(), PlayerLocation) > FMath::Square(1650.f)) {
   Npc->Destroy();
   It.RemoveCurrent();
  }
 }
 constexpr int32 MaxStreamedResidents = 24;
 if (ActiveResidents.Num() >= MaxStreamedResidents) return;
 // Scan no more than 80 records per quarter-second tick; spawn at most two.
 int32 Spawned = 0;
 for (int32 Checked = 0; Checked < 80 && Spawned < 2
      && ActiveResidents.Num() < MaxStreamedResidents; ++Checked) {
  const int32 Id = PopulationCursor;
  PopulationCursor = Regions * 3 + (PopulationCursor - Regions * 3 + 1) % (1111 - Regions * 3);
  if (ActiveResidents.Contains(Id)) continue;
  const FVector Home = GetResidentHome(Id);
  if (FVector::DistSquared2D(Home, PlayerLocation) > FMath::Square(1200.f)) continue;
  const FTransform SpawnTransform(FRotator::ZeroRotator, Home);
  AMoonfallQuestGuide* Npc = Cast<AMoonfallQuestGuide>(
   UGameplayStatics::BeginDeferredActorSpawnFromClass(this,
    AMoonfallQuestGuide::StaticClass(), SpawnTransform));
  if (!Npc) continue;
  Npc->PopulationNpcId = Id;
  Npc->RegionIndex = (Id - Regions * 3) % Regions;
  Npc->RoleIndex = 1 + ((Id - Regions * 3) / Regions) % 2;
  UGameplayStatics::FinishSpawningActor(Npc, SpawnTransform);
  ActiveResidents.Add(Id, Npc);
  ++Spawned;
 }
}

int32 AMoonfallArena::GetRegionalSupplyRating(int32 RegionIndex) const {
 if (RegionIndex < 0 || RegionIndex >= Regions) return 0;
 int32 Guardians = 0;
 for (int32 Dungeon = 0; Dungeon < DungeonsPerRegion; ++Dungeon)
  if (IsDungeonQuestCompleted(RegionIndex * DungeonsPerRegion + Dungeon))
   ++Guardians;
 // Supplies improve after verified guardians fall, with no fabricated stock drops.
 return FMath::Clamp(20 + Guardians * 15 + FMath::Max(0, GetRegionalFactionStanding(RegionIndex)) / 5, 0, 100);
}
int32 AMoonfallArena::GetRegionalServicePrice(int32 RegionIndex, int32 RoleIndex) const {
 if (!bContinuousWorld || RegionIndex < 0 || RegionIndex >= Regions ||
     (RoleIndex != 1 && RoleIndex != 2)) return 0;
 const int32 Standing = GetRegionalFactionStanding(RegionIndex);
 const int32 Supply = GetRegionalSupplyRating(RegionIndex);
 const int32 Base = RoleIndex == 1 ? 12 : 10;
 const int32 StandingAdjustment = Standing < 0 ? (-Standing + 14) / 15
     : -(Standing / 25);
 const int32 SupplyDiscount = Supply / 30;
 // Prices stay positive; no negative-price or free-item farming.
 return FMath::Clamp(Base + StandingAdjustment - SupplyDiscount, 4, 25);
}

int32 AMoonfallArena::GetWorkshopStock(int32 RegionIndex) const {
 if (RegionIndex < 0 || RegionIndex >= Regions || !WorkshopStock.IsValidIndex(RegionIndex)) return 0;
 return FMath::Clamp(WorkshopStock[RegionIndex], 0, 8);
}
int32 AMoonfallArena::GetCraftingPrice(int32 RegionIndex) const {
 if (!bContinuousWorld || RegionIndex < 0 || RegionIndex >= Regions) return 0;
 const int32 Base = 9 + (100 - GetRegionalSupplyRating(RegionIndex)) / 20;
 const int32 ReputationPenalty = FMath::Max(0, -GetRegionalFactionStanding(RegionIndex)) / 20;
 return FMath::Clamp(Base + ReputationPenalty, 7, 20);
}
bool AMoonfallArena::PurchaseWorkshopStock(int32 RegionIndex) {
 if (!bContinuousWorld || RegionIndex < 0 || RegionIndex >= Regions ||
     !DiscoveredRegions.Contains(RegionIndex) ||
     !WorkshopStock.IsValidIndex(RegionIndex) || WorkshopStock[RegionIndex] <= 0) return false;
 --WorkshopStock[RegionIndex];
 bWorldSavePending = true;
 // Commit the limited workshop stock *before* the hero gains crafted gear.
 // If this world save fails, roll back and reject the purchase; otherwise a
 // restart could mint duplicate crafted equipment from stale workshop stock.
 SaveWorldProgress();
 if (bWorldSavePending) {
  ++WorkshopStock[RegionIndex];
  return false;
 }
 return true;
}
bool AMoonfallArena::RefundWorkshopStock(int32 RegionIndex) {
 if (!bContinuousWorld || !WorkshopStock.IsValidIndex(RegionIndex)) return false;
 // A refund never increases stock beyond the valid persisted maximum.
 const int32 OldCount = WorkshopStock[RegionIndex];
 WorkshopStock[RegionIndex] = FMath::Clamp(OldCount + 1, 0, 8);
 if (WorkshopStock[RegionIndex] == OldCount) return false;
 bWorldSavePending = true;
 SaveWorldProgress();
 return !bWorldSavePending;
}
void AMoonfallArena::AdvanceWorkshopProduction() {
 if (!GetWorld() || !bContinuousWorld || WorkshopStock.Num() != Regions) return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now) || Now < NextWorkshopProductionAt) return;
 // No backfill loop or catch-up burst; one bounded workshop cycle.
 NextWorkshopProductionAt = Now + 120.f;
 for (int32 Region = 0; Region < Regions; ++Region) {
  if (!DiscoveredRegions.Contains(Region)) continue;
  const int32 Capacity = FMath::Clamp(2 + GetRegionalSupplyRating(Region) / 20, 2, 7);
  if (WorkshopStock[Region] < Capacity) {
   WorkshopStock[Region] = FMath::Min(Capacity, WorkshopStock[Region] + 1);
   bWorldSavePending = true;
  }
 }
}
