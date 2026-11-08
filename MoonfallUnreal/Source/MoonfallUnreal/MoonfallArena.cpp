#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallWorldProgressSave.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
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
  if (bWorldSavePending) SaveWorldProgress();
  StreamWorld();
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
 if (UGameplayStatics::SaveGameToSlot(Saved, WorldProgressSlot, 0))
  bWorldSavePending = false;
}
