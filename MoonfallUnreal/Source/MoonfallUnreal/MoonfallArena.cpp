#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "MoonfallHeroPawn.h"
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
 if (bContinuousWorld) { BuildWorld(); return; }
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
 if (!GetWorld() || SpawnedEncounters.Contains(Id)) return false;
 const FTransform Transform(FRotator::ZeroRotator, Position + FVector(0.f, 0.f, 110.f));
 AMoonfallEnemy* Enemy = Cast<AMoonfallEnemy>(
  UGameplayStatics::BeginDeferredActorSpawnFromClass(this, AMoonfallEnemy::StaticClass(), Transform));
 if (!Enemy) return false;
 Enemy->bElite = Elite;
 Enemy->bBoss = Boss;
 const float Scale = 1.f + 0.22f * FMath::Clamp(Region, 0, Regions - 1);
 Enemy->Health = FMath::Clamp(Enemy->Health * Scale * (Boss ? 4.f : Elite ? 1.7f : 1.f), 1.f, 100000.f);
 Enemy->AttackDamage = FMath::Clamp(Enemy->AttackDamage * Scale * (Boss ? 1.6f : 1.f), 1.f, 10000.f);
 UGameplayStatics::FinishSpawningActor(Enemy, Transform);
 SpawnedEncounters.Add(Id); // Defeated creatures do not respawn within this session.
 return true;
}
void AMoonfallArena::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (bContinuousWorld) StreamWorld();
}
void AMoonfallArena::StreamWorld() {
 if (!GetWorld()) return;
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero)) return;
 const FVector Here = Hero->GetActorLocation();
 bool BuiltOne = false, SpawnedOne = false;
 // One chamber geometry allocation and at most one hostile spawn per 0.25s scan.
 for (int32 Region = 0; Region < Regions; ++Region) {
  for (int32 Index = 0; Index < 3 && !SpawnedOne; ++Index) {
   const int32 Id = Region * 3 + Index;
   const FVector At(Region * RegionSpacing - 650.f + Index * 650.f,
    700.f + (Index % 2) * 350.f, 0.f);
   if (!SpawnedEncounters.Contains(Id) && FVector::DistSquared2D(Here, At) < 850.f * 850.f)
    SpawnedOne = SpawnEncounter(Id, At, Region, false, false);
  }
  for (int32 Dungeon = 0; Dungeon < DungeonsPerRegion; ++Dungeon) {
   for (int32 Chamber = 0; Chamber < ChambersPerDungeon; ++Chamber) {
    const int32 Room = (Region * DungeonsPerRegion + Dungeon) * ChambersPerDungeon + Chamber;
    const FVector At = RoomCenter(Region, Dungeon, Chamber);
    const float Distance = FVector::DistSquared2D(Here, At);
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
