#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "MoonfallHeroPawn.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/ConstructorHelpers.h"
AMoonfallArena::AMoonfallArena() {
 PrimaryActorTick.bCanEverTick = false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
 RuinPieces = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RuinPieces"));
 RuinPieces->SetupAttachment(GetRootComponent());
 RuinPieces->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 RuinPieces->SetCastShadow(false);
 RuinPieces->SetMobility(EComponentMobility::Static);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> RuinCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (RuinCube.Succeeded()) RuinPieces->SetStaticMesh(RuinCube.Object);
 DeadTrees = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DeadTrees"));
 DeadTrees->SetupAttachment(GetRootComponent());
 DeadTrees->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 DeadTrees->SetCastShadow(false);
 if (RuinCube.Succeeded()) DeadTrees->SetStaticMesh(RuinCube.Object);
 RitualStones = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RitualStones"));
 RitualStones->SetupAttachment(GetRootComponent());
 RitualStones->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 RitualStones->SetCastShadow(false);
 if (RuinCube.Succeeded()) RitualStones->SetStaticMesh(RuinCube.Object);
 RitualSigil = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RitualSigil"));
 RitualSigil->SetupAttachment(GetRootComponent());
 RitualSigil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 RitualSigil->SetCastShadow(false);
 if (RuinCube.Succeeded()) RitualSigil->SetStaticMesh(RuinCube.Object);
 Moonlight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moonlight"));
 Moonlight->SetupAttachment(GetRootComponent());
 Moonlight->SetRelativeRotation(FRotator(-48.f,135.f,0.f));
 Moonlight->SetIntensity(1.4f);
 Moonlight->SetLightColor(FLinearColor(0.46f,0.57f,0.95f));
 Moonlight->SetCastShadows(false);
 GroundFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("GroundFog"));
 GroundFog->SetupAttachment(GetRootComponent());
 GroundFog->SetFogInscatteringColor(FLinearColor(0.055f,0.075f,0.16f));
 GroundFog->SetFogHeightFalloff(0.34f);
 GroundFog->SetFogDensity(0.012f);
}
void AMoonfallArena::CreateBlock(FVector Position, FVector Scale, bool bGround) {
 UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this);
 Block->SetupAttachment(GetRootComponent());
 Block->SetRelativeLocation(Position);
 Block->SetRelativeScale3D(Scale);
 Block->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 UStaticMesh* DefaultCube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 UStaticMesh* SelectedMesh = bGround ? GroundMeshArt : BoundaryMeshArt;
 if (SelectedMesh || DefaultCube) Block->SetStaticMesh(SelectedMesh ? SelectedMesh : DefaultCube);
 UMaterialInterface* SelectedMaterial = bGround ? GroundMaterial : BoundaryMaterial;
 if (SelectedMaterial) Block->SetMaterial(0,SelectedMaterial);
 Block->RegisterComponent();
}
void AMoonfallArena::CreateDecoration(FVector Position, FVector Scale) {
 // All non-interactive ruins share one instanced mesh component; no per-prop actors or ticks.
 if (RuinPieces && RuinPieces->GetStaticMesh())
  RuinPieces->AddInstance(FTransform(FRotator::ZeroRotator,Position,Scale));
}
void AMoonfallArena::AddFoliage(FVector Position, FVector Scale, FRotator Rotation) {
 if (DeadTrees && DeadTrees->GetStaticMesh())
  DeadTrees->AddInstance(FTransform(Rotation,Position,Scale));
}
void AMoonfallArena::AddRitualStone(FVector Position, FVector Scale, FRotator Rotation) {
 if (RitualStones && RitualStones->GetStaticMesh())
  RitualStones->AddInstance(FTransform(Rotation,Position,Scale));
}
void AMoonfallArena::AddSigilTile(FVector Position, FVector Scale, FRotator Rotation) {
 if (RitualSigil && RitualSigil->GetStaticMesh())
  RitualSigil->AddInstance(FTransform(Rotation, Position, Scale));
}
void AMoonfallArena::CreateAtmosphereLight(FVector Position, FLinearColor Color, float Intensity) {
 UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
 Light->SetupAttachment(GetRootComponent());
 Light->SetRelativeLocation(Position);
 Light->SetIntensity(Intensity);
 Light->SetLightColor(Color);
 Light->SetAttenuationRadius(1250.f);
 Light->SetCastShadows(false);
 Light->RegisterComponent();
}
void AMoonfallArena::BeginPlay() {
 Super::BeginPlay();
 // Atmospheric visibility is independent of map/quest overlays and never spawns ticking actors.
 Moonlight->SetVisibility(bEnableMoonlight);
 GroundFog->SetVisibility(bEnableLowFog);
 GroundFog->SetFogDensity(FMath::Clamp(GroundFogDensity,0.f,0.025f));
 CreateBlock(FVector(0,0,-60),FVector(30,30,1),true);
 CreateBlock(FVector(1500,0,200),FVector(1,30,5),false);
 CreateBlock(FVector(-1500,0,200),FVector(1,30,5),false);
 CreateBlock(FVector(0,1500,200),FVector(30,1,5),false);
 CreateBlock(FVector(0,-1500,200),FVector(30,1,5),false);
 if (RuinMeshArt) RuinPieces->SetStaticMesh(RuinMeshArt);
 if (RuinStoneMaterial) RuinPieces->SetMaterial(0,RuinStoneMaterial);
 if (DeadTreeMeshArt) DeadTrees->SetStaticMesh(DeadTreeMeshArt);
 if (DeadTreeMaterial) DeadTrees->SetMaterial(0,DeadTreeMaterial);
 if (RitualStoneMeshArt) RitualStones->SetStaticMesh(RitualStoneMeshArt);
 if (RitualStoneMaterial) RitualStones->SetMaterial(0,RitualStoneMaterial);
 if (SigilTileMeshArt) RitualSigil->SetStaticMesh(SigilTileMeshArt);
 if (SigilTileMaterial) RitualSigil->SetMaterial(0,SigilTileMaterial);
 // Original deterministic ruined architecture and ground debris, all non-colliding.
 for (int32 i=0; i<6; ++i) {
  const float Angle = i*(2.f*PI/6.f);
  const FVector P(1050.f*FMath::Cos(Angle),1050.f*FMath::Sin(Angle),155.f);
  CreateDecoration(P,FVector(0.7f,0.55f,3.5f));
  CreateDecoration(P+FVector(0.f,0.f,220.f),FVector(1.1f,0.85f,0.35f));
  // Broken lintels and fallen supports add readable silhouettes without actor spawning.
  if (i%2 == 0) {
   CreateDecoration(P+FVector(90.f,45.f,-65.f),FVector(1.4f,0.34f,0.35f));
   CreateDecoration(P+FVector(-85.f,-50.f,-85.f),FVector(0.42f,1.15f,0.3f));
  }
 }
 const int32 DebrisCount = FMath::Clamp(RubbleCount,0,48);
 for (int32 i=0;i<DebrisCount;++i) {
  // Golden-angle distribution is deterministic across runs and avoids per-frame RNG.
  const float Angle = i*2.399963f;
  const float Radius = 530.f+FMath::Sqrt(static_cast<float>(i)+1.f)*115.f;
  const float StoneSize = 0.2f+static_cast<float>(i%4)*0.09f;
  CreateDecoration(FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,-10.f),
                   FVector(StoneSize*2.2f,StoneSize*1.5f,StoneSize*0.45f));
 }
 // Bounded eerie forest border: one non-colliding instance per trunk plus two crooked branches.
 // Keep the center clear for combat, quests, and hit warnings.
 const int32 TreeBudget = FMath::Clamp(DeadTreeCount,0,16);
 for (int32 i=0;i<TreeBudget;++i) {
  const float A = i*2.399963f;
  const float Distance = 1250.f + static_cast<float>(i%3)*55.f;
  const float Height = 2.0f + 0.25f*static_cast<float>(i%4);
  // The fallback cube pivot is centered. Place its bottom at floor height (-10 cm).
  // Imported meshes may have authored root pivots and keep their adjustable art offset.
  const float BaseZ = DeadTreeMeshArt ? 155.f : -10.f + Height*50.f;
  const FVector Base(FMath::Cos(A)*Distance,FMath::Sin(A)*Distance,BaseZ);
  AddFoliage(Base,FVector(0.20f,0.25f,Height),FRotator(0.f,0.f,6.f*FMath::Sin(A)));
  if (!DeadTreeMeshArt) {
   // Default cube fallback gets two sculptural branches, but imported trees stay one mesh.
   AddFoliage(Base+FVector(25.f,0.f,105.f),FVector(0.12f,0.12f,1.0f),FRotator(0.f,35.f,45.f));
   AddFoliage(Base+FVector(-35.f,0.f,145.f),FVector(0.12f,0.12f,0.8f),FRotator(0.f,-20.f,-48.f));
  }
 }
 // Seven weathered ritual stones frame the boss combat space without hiding the player.
 for (int32 i=0;i<7;++i) {
  const float A = i*2.f*PI/7.f;
  AddRitualStone(FVector(FMath::Cos(A)*780.f,FMath::Sin(A)*780.f,4.f),
                 FVector(0.55f,0.17f,0.12f),FRotator(0.f,FMath::RadiansToDegrees(A),0.f));
 }
 // A low-lying ritual ring of 24 tiles, below the combat plane; no gameplay collision.
 // Keep the center empty for boss telegraphs and the player's silhouette.
 if (bShowRitualCircle) {
  constexpr int32 RingTiles = 24;
  for (int32 i=0;i<RingTiles;++i) {
   const float A = (2.f*PI*i)/static_cast<float>(RingTiles);
   const float RadialDistance = i%2 == 0 ? 630.f : 660.f;
   AddSigilTile(FVector(FMath::Cos(A)*RadialDistance,FMath::Sin(A)*RadialDistance,-8.f),
                FVector(0.52f,0.075f,0.03f),
                FRotator(0.f,FMath::RadiansToDegrees(A)+90.f,0.f));
  }
 }
 CreateAtmosphereLight(FVector(-950.f,350.f,420.f),FLinearColor(0.17f,0.31f,0.85f),2700.f);
 CreateAtmosphereLight(FVector(950.f,-350.f,420.f),FLinearColor(0.88f,0.18f,0.09f),2100.f);
 if (!GetWorld()) return;
 // Corrupted crow enemies use the normal enemy damage, XP and loot pipeline.
 for (int32 BirdIndex=0; BirdIndex<2; ++BirdIndex) {
  const FVector BirdPosition = GetActorLocation() + FVector(BirdIndex==0 ? -380.f : 380.f,0.f,520.f);
  const FTransform BirdSpawn(FRotator::ZeroRotator,BirdPosition);
  UClass* CrowSpawnClass = CrowEnemyClass ? CrowEnemyClass.Get() : AMoonfallEnemy::StaticClass();
  AMoonfallEnemy* Crow = Cast<AMoonfallEnemy>(UGameplayStatics::BeginDeferredActorSpawnFromClass(
   this,CrowSpawnClass,BirdSpawn));
  if (!Crow) continue;
  Crow->bCorruptedCrow = true;
  Crow->Health = 52.f;
  Crow->AttackDamage = 7.f;
  UGameplayStatics::FinishSpawningActor(Crow,BirdSpawn);
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
  if (i == Count - 1 && Count >= 4) {
   Enemy->bElite = true;
   Enemy->bBoss = true;
   // Unique arena-quest marker: other dungeon bosses must not become this target.
   Enemy->Tags.AddUnique(FName(TEXT("MoonfallArenaQuestBoss")));
   Enemy->Health *= 4.f;
   Enemy->AttackCooldown *= 1.15f;
  }
  UGameplayStatics::FinishSpawningActor(Enemy, SpawnTransform);
  if (Enemy->bBoss) {
   if (AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
    Hero->BeginArenaBossQuest(Enemy);
  }
 }
}
