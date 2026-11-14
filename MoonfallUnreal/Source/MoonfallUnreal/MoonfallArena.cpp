#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallCheckpointShrine.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
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
 // World shrines are discovered by walking into their proximity, not by
 // defeating a boss or using an attack key. Stable IDs persist between saves.
 const struct FShrinePlacement { const TCHAR* Id; FVector Offset; } Shrines[] = {
  {TEXT("crossroads_west"), FVector(-1120.f, -720.f, 0.f)},
  {TEXT("crimson_path"), FVector(1080.f, -750.f, 0.f)},
  {TEXT("moonlit_path"), FVector(1040.f, 800.f, 0.f)}
 };
 for (const FShrinePlacement& Placement : Shrines) {
  const FTransform SpawnTransform(FRotator::ZeroRotator, GetActorLocation() + Placement.Offset);
  AMoonfallCheckpointShrine* Shrine = Cast<AMoonfallCheckpointShrine>(
      UGameplayStatics::BeginDeferredActorSpawnFromClass(
          this, AMoonfallCheckpointShrine::StaticClass(), SpawnTransform));
  if (Shrine) {
   Shrine->ShrineId = FName(Placement.Id);
   UGameplayStatics::FinishSpawningActor(Shrine, SpawnTransform);
  }
 }
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
 // Fixed showcase NPC in the existing world. No separate gameplay mode.
 // The player or an interaction UI calls TryTalk; the actor never ticks.
 if (AMoonfallWorldNPC* Guide = GetWorld()->SpawnActor<AMoonfallWorldNPC>(
     AMoonfallWorldNPC::StaticClass(), GetActorLocation() + FVector(-350.f, 650.f, 60.f), FRotator::ZeroRotator)) {
  Guide->RaceId = 2; // Wood Elf
  Guide->SettlementId = FName(TEXT("Crossroads"));
  Guide->ResidentId = FName(TEXT("CrossroadsGuide"));
  Guide->FactionId = FName(TEXT("Wanderers"));
  Guide->SocialRole = FName(TEXT("Guide"));
  Guide->DisplayName = FText::FromString(TEXT("Wandering Guide"));
  Guide->Greeting = FText::FromString(TEXT("Every road belongs to the same world. Stay alert beyond the gate."));
 }
 // Twelve fixed representatives from every playable ancestry, coexisting
 // in one Crossroads settlement. No NPC tick, random spawn or new game mode.
 // Gameplay identity is data-driven; appearance art is left untouched.
 struct FResident { const TCHAR* Name; int32 Race; const TCHAR* Faction;
   const TCHAR* Role; FVector Offset; };
 const FResident Residents[] = {
  {TEXT("Alden"), 0, TEXT("Wanderers"), TEXT("Guard"), FVector(-1100.f,-1050.f,60.f)},
  {TEXT("Starweaver"), 1, TEXT("MoonCoven"), TEXT("Scholar"), FVector(-850.f,-1050.f,60.f)},
  {TEXT("Leafwhisper"), 2, TEXT("VerdantCircle"), TEXT("Scout"), FVector(-600.f,-1050.f,60.f)},
  {TEXT("Anvilkeeper"), 3, TEXT("StoneHearth"), TEXT("Merchant"), FVector(-1100.f,-350.f,60.f)},
  {TEXT("Pip Hearthfoot"), 4, TEXT("Wanderers"), TEXT("Merchant"), FVector(-850.f,-350.f,60.f)},
  {TEXT("Ember Scout"), 5, TEXT("AshenClans"), TEXT("Scout"), FVector(-600.f,-350.f,60.f)},
  {TEXT("Geargrin"), 6, TEXT("Wanderers"), TEXT("Tinker"), FVector(-1100.f,350.f,60.f)},
  {TEXT("Mossfang"), 7, TEXT("AshenClans"), TEXT("Guard"), FVector(-850.f,350.f,60.f)},
  {TEXT("Birchroot"), 8, TEXT("VerdantCircle"), TEXT("Elder"), FVector(-600.f,350.f,60.f)},
  {TEXT("Duskveil"), 9, TEXT("MoonCoven"), TEXT("Scholar"), FVector(-1100.f,1050.f,60.f)},
  {TEXT("Silverwatch"), 10, TEXT("MoonCoven"), TEXT("Guard"), FVector(-850.f,1050.f,60.f)},
  {TEXT("Ashscale"), 11, TEXT("AshenClans"), TEXT("Elder"), FVector(-600.f,1050.f,60.f)}
 };
 for (const FResident& Resident : Residents) {
  if (AMoonfallWorldNPC* Person = GetWorld()->SpawnActor<AMoonfallWorldNPC>(
      AMoonfallWorldNPC::StaticClass(), GetActorLocation() + Resident.Offset, FRotator::ZeroRotator)) {
   Person->DisplayName = FText::FromString(Resident.Name);
   Person->ResidentId = FName(FString(Resident.Name).Replace(TEXT(" "), TEXT("")));
   Person->RaceId = Resident.Race;
   Person->FactionId = FName(Resident.Faction);
   Person->SettlementId = FName(TEXT("Crossroads"));
   Person->SocialRole = FName(Resident.Role);
  }
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
  // Each ordinary combatant has a stable racial origin; boss IDs/art are unaffected.
  Enemy->RaceId = i % 12;
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
}
