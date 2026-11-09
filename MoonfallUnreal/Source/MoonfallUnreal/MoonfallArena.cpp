#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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
 // Cap living enemies, including regular, elite, and divine bosses.
 // Never fragment a group: postpone until all its members fit.
 int32 LivingEnemies = 0;
 for (TActorIterator<AMoonfallEnemy> It(World); It; ++It) {
  if (IsValid(*It) && It->Health > 0.f) ++LivingEnemies;
 }
 const int32 GroupSize = PendingBonusGroupSizes[NextBonusGroup];
 constexpr int32 MaxLivingEnemies = 24;
 if (LivingEnemies + GroupSize > MaxLivingEnemies) {
  GetWorldTimerManager().SetTimer(BonusSpawnTimer, this,
    &AMoonfallArena::SpawnNextBonusGroup, 4.f, false);
  return;
 }
 // Group centers are separated, and individual mobs orbit their center.
 // Entire clusters stay inside the 3000x3000 arena footprint.
 constexpr float GroupCenterRadius = 950.f;
 constexpr float MemberRadius = 230.f;
 const float GroupAngle = (2.f * PI * NextBonusGroup) / 3.f;
 const FVector GroupCenter = GetActorLocation() + FVector(
   GroupCenterRadius * FMath::Cos(GroupAngle),
   GroupCenterRadius * FMath::Sin(GroupAngle), 60.f);
 for (int32 Member = 0; Member < GroupSize; ++Member) {
  const float Angle = 2.f * PI * Member / GroupSize;
  const FVector Position = GroupCenter + FVector(
    MemberRadius * FMath::Cos(Angle), MemberRadius * FMath::Sin(Angle), 0.f);
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
