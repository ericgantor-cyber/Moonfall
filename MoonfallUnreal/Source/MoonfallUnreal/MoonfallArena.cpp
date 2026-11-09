#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
AMoonfallArena::AMoonfallArena() {
 PrimaryActorTick.bCanEverTick = false;
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
 CreateBlock(FVector(0,0,-60),FVector(30,30,1));
 CreateBlock(FVector(1500,0,200),FVector(1,30,5));
 CreateBlock(FVector(-1500,0,200),FVector(1,30,5));
 CreateBlock(FVector(0,1500,200),FVector(30,1,5));
 CreateBlock(FVector(0,-1500,200),FVector(30,1,5));
 if (!GetWorld()) return;
 const bool bZEncounter = FeaturedDivineBossId == FName(TEXT("z_void_reaper"));
 const bool bNyraEncounter = FeaturedDivineBossId == FName(TEXT("nyra_eclipse_queen"));
 const bool bSingleDivineEncounter = bZEncounter || bNyraEncounter;
 if (bSingleDivineEncounter) {
  // Sparse, fixed arena cover: preserves the open central fighting area.
  // Pillars break sight for ranged spells and offer deliberate positioning.
  // No tick, procedural allocation storm, or destructible prop simulation.
  const float CoverOffset = bZEncounter ? 850.f : 700.f;
  const FVector CoverScale = bZEncounter ? FVector(1.8f,1.8f,3.2f) : FVector(1.3f,2.4f,3.2f);
  CreateBlock(FVector(CoverOffset, CoverOffset, 160.f), CoverScale);
  CreateBlock(FVector(-CoverOffset, CoverOffset, 160.f), CoverScale);
  CreateBlock(FVector(CoverOffset,-CoverOffset, 160.f), CoverScale);
  CreateBlock(FVector(-CoverOffset,-CoverOffset,160.f), CoverScale);
 }
 const int32 Count = FMath::Clamp(EnemyCount,0,12);
 for (int32 i=0;i<Count;++i) {
  float Angle = 2.f * PI * i / FMath::Max(1,Count);
  FVector Position = GetActorLocation() + FVector(450.f*FMath::Cos(Angle),450.f*FMath::Sin(Angle),60.f);
  // Defer BeginPlay until elite/boss properties are configured.
  const FTransform SpawnTransform(FRotator::ZeroRotator, Position);
  AMoonfallEnemy* Enemy = Cast<AMoonfallEnemy>(UGameplayStatics::BeginDeferredActorSpawnFromClass(
    this, AMoonfallEnemy::StaticClass(), SpawnTransform));
  if (!Enemy) continue;
  // The selected preset spawns only the corresponding divine boss in
  // the final slot. NAME_None retains the previous two-god test showcase.
  // Actual regional encounter placement is deferred to world migration.
  const bool bNamedGodSlot = bSingleDivineEncounter && i == Count - 1;
  const bool bLegacyGodSlot = !bSingleDivineEncounter && Count >= 5
    && (i == Count - 2 || i == Count - 1);
  if (bNamedGodSlot || bLegacyGodSlot) {
   const bool bZ = bNamedGodSlot ? bZEncounter : i == Count - 2;
   Enemy->bElite = true;
   Enemy->bBoss = true;
   Enemy->DivineBossId = bZ ? FName(TEXT("z_void_reaper")) : FName(TEXT("nyra_eclipse_queen"));
   Enemy->DivineBossTitle = bZ ? FText::FromString(TEXT("Z, Void Reaper"))
                              : FText::FromString(TEXT("Nyra, Eclipse Queen"));
   Enemy->DivineBossRegion = bZ ? FName(TEXT("ShatteredRuins")) : FName(TEXT("EclipseCitadel"));
   Enemy->Tags.Add(FName(TEXT("DivineWorldBoss")));
   Enemy->Health *= bZ ? 4.f : 4.5f;
   Enemy->AttackCooldown *= 1.15f;
  } else if (!bSingleDivineEncounter && Count == 4 && i == Count - 1) {
   // Keep the old four-enemy prototype setup valid.
   Enemy->bElite = true;
   Enemy->bBoss = true;
   Enemy->Health *= 4.f;
   Enemy->AttackCooldown *= 1.15f;
  }
  UGameplayStatics::FinishSpawningActor(Enemy, SpawnTransform);
 }
 // One 25% roll for the whole encounter, not one roll per group.
 // Stagger groups so a lucky roll does not spawn up to 33 attackers at once.
 if (FMath::FRand() < 0.25f) {
  PendingBonusGroupSizes.Reserve(3);
  for (int32 Group = 0; Group < 3; ++Group) {
   PendingBonusGroupSizes.Add(FMath::RandRange(5, 11));
  }
  GetWorldTimerManager().SetTimer(BonusSpawnTimer, this,
    &AMoonfallArena::SpawnNextBonusGroup, 4.f, false);
 }
}

void AMoonfallArena::SpawnNextBonusGroup() {
 UWorld* World = GetWorld();
 if (!World || NextBonusGroup >= PendingBonusGroupSizes.Num()) return;
 // Avoid fresh enemy spawns during a paused menu, after defeat, or mid-boss fight.
 // Keep the queued group's size rather than rerolling it on retries.
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero) || !FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f) {
  PendingBonusGroupSizes.Empty();
  return;
 }
 if (Hero->bOptionsOpen || Hero->bInventoryOpen) {
  GetWorldTimerManager().SetTimer(BonusSpawnTimer, this,
    &AMoonfallArena::SpawnNextBonusGroup, 4.f, false);
  return;
 }
 // Cap living enemies, including regular, elite, and divine bosses.
 // Never fragment a group: postpone until all its members fit.
 int32 LivingEnemies = 0;
 bool bBossAlive = false;
 for (TActorIterator<AMoonfallEnemy> It(World); It; ++It) {
  if (IsValid(*It) && It->Health > 0.f) {
   ++LivingEnemies;
   bBossAlive |= It->bBoss;
  }
 }
 // Respect divine encounters. Do not add a crowd while a boss is active.
 if (bBossAlive) {
  GetWorldTimerManager().SetTimer(BonusSpawnTimer, this,
    &AMoonfallArena::SpawnNextBonusGroup, 4.f, false);
  return;
 }
 const int32 GroupSize = PendingBonusGroupSizes[NextBonusGroup];
 constexpr int32 MaxLivingEnemies = 24;
 if (LivingEnemies + GroupSize > MaxLivingEnemies) {
  GetWorldTimerManager().SetTimer(BonusSpawnTimer, this,
    &AMoonfallArena::SpawnNextBonusGroup, 4.f, false);
  return;
 }
 // Three widely separated groups, with terrain projected individually.
 // Validate ALL group members before spawning to avoid partial packs.
 // Biome-inspired formation profiles only, not new terrain/maps or enemy assets.
 // Keep the whole cluster within the existing arena footprint.
 struct FSpawnFormation { const TCHAR* Name; float CenterRadius; float MemberRadius; };
 static const FSpawnFormation Profiles[] = {
  {TEXT("Forest"), 950.f, 230.f},
  {TEXT("Cave"), 780.f, 175.f},
  {TEXT("Foundry"), 1030.f, 260.f},
  {TEXT("Swamp"), 820.f, 210.f},
  {TEXT("Desert"), 1080.f, 245.f},
  {TEXT("Tundra"), 1060.f, 230.f},
  {TEXT("Volcanic"), 900.f, 190.f},
  {TEXT("Ruins"), 860.f, 205.f},
  {TEXT("Crypt"), 760.f, 170.f},
  {TEXT("Coast"), 1010.f, 220.f},
  {TEXT("Highlands"), 980.f, 200.f},
  {TEXT("Eclipse"), 920.f, 250.f}
 };
 float GroupCenterRadius = Profiles[0].CenterRadius;
 float MemberRadius = Profiles[0].MemberRadius;
 for (const FSpawnFormation& Profile : Profiles) {
  if (SpawnBiome == FName(Profile.Name)) {
   GroupCenterRadius = Profile.CenterRadius;
   MemberRadius = Profile.MemberRadius;
   break;
  }
 }
 constexpr float SpawnHeight = 90.f;
 constexpr float MinPlayerDistance = 550.f;
 // Rotate the candidate cluster each retry to explore different viable ground.
 const float GroupAngle = (2.f * PI * NextBonusGroup) / 3.f
   + 0.31f * TerrainPlacementAttempts;
 const FVector GroupCenter = GetActorLocation() + FVector(
   GroupCenterRadius * FMath::Cos(GroupAngle),
   GroupCenterRadius * FMath::Sin(GroupAngle), 0.f);
 TArray<FVector> ValidPositions;
 ValidPositions.Reserve(GroupSize);
 FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(MoonfallMobGround), false);
 bool bValidGroup = true;
 for (int32 Member = 0; Member < GroupSize; ++Member) {
  // Rotate pack formation on each terrain retry to test new safe clearances.
  const float Angle = 2.f * PI * Member / GroupSize + 0.17f * TerrainPlacementAttempts;
  const FVector Candidate = GroupCenter + FVector(
    MemberRadius * FMath::Cos(Angle), MemberRadius * FMath::Sin(Angle), 0.f);
  FHitResult GroundHit;
  const bool bHasGround = World->LineTraceSingleByChannel(GroundHit,
    Candidate + FVector(0.f, 0.f, 450.f),
    Candidate - FVector(0.f, 0.f, 500.f), ECC_WorldStatic, GroundParams);
  if (!bHasGround || GroundHit.ImpactNormal.Z < 0.75f) {
   bValidGroup = false;
   break;
  }
  const FVector Position = GroundHit.ImpactPoint + FVector(0.f, 0.f, SpawnHeight);
  if (FVector::DistSquared2D(Position, Hero->GetActorLocation())
      < FMath::Square(MinPlayerDistance)
      || World->OverlapBlockingTestByChannel(Position, FQuat::Identity,
           ECC_Pawn, FCollisionShape::MakeSphere(55.f), GroundParams)) {
   bValidGroup = false;
   break;
  }
  ValidPositions.Add(Position);
 }
 if (!bValidGroup || ValidPositions.Num() != GroupSize) {
  // Try other cluster locations rather than tracing the same blocked geometry forever.
  // If the level has no safe pack-sized ground, fail closed without spawning inside walls.
  constexpr int32 MaxPlacementAttempts = 12;
  if (++TerrainPlacementAttempts >= MaxPlacementAttempts) {
   PendingBonusGroupSizes.Empty();
   GetWorldTimerManager().ClearTimer(BonusSpawnTimer);
   return;
  }
  GetWorldTimerManager().SetTimer(BonusSpawnTimer, this,
    &AMoonfallArena::SpawnNextBonusGroup, 4.f, false);
  return;
 }
 TerrainPlacementAttempts = 0;
 for (const FVector& Position : ValidPositions) {
  World->SpawnActor<AMoonfallEnemy>(AMoonfallEnemy::StaticClass(),
    Position, FRotator::ZeroRotator);
 }
 ++NextBonusGroup;
 if (NextBonusGroup < PendingBonusGroupSizes.Num()) {
  GetWorldTimerManager().SetTimer(BonusSpawnTimer, this,
    &AMoonfallArena::SpawnNextBonusGroup, 8.f, false);
 } else {
  PendingBonusGroupSizes.Empty();
 }
}
