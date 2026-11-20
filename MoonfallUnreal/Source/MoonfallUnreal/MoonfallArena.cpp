#include "MoonfallArena.h"
#include "MoonfallDeathGoddess.h"
#include "MoonfallEnemy.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallCheckpointShrine.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/ConstructorHelpers.h"
AMoonfallArena::AMoonfallArena() {
 PrimaryActorTick.bCanEverTick = false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
 DungeonRoad = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DungeonRoad"));
 DungeonRoad->SetupAttachment(GetRootComponent());
 DungeonRoad->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 DungeonRoad->SetCollisionResponseToAllChannels(ECR_Block);
 // Cull distant road tiles visually while their physical collision and
 // NavMesh geometry remain intact for uninterrupted travel across zones.
 DungeonRoad->SetCullDistances(24000, 34000);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) DungeonRoad->SetStaticMesh(Cube.Object);
}
void AMoonfallArena::CreateBlock(FVector Position, FVector Scale) {
 // Reuse the mesh already loaded for the instanced dungeon road rather
 // than issuing a second asset lookup for every sanctuary wall and plinth.
 UStaticMesh* Cube = DungeonRoad ? DungeonRoad->GetStaticMesh() : nullptr;
 if (!Cube) {
  UE_LOG(LogTemp, Warning, TEXT("Moonfall arena block skipped: cube mesh unavailable"));
  return;
 }
 UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this);
 if (!IsValid(Block)) return;
 Block->SetupAttachment(GetRootComponent());
 Block->SetStaticMesh(Cube);
 Block->SetRelativeLocation(Position);
 Block->SetRelativeScale3D(Scale);
 Block->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Block->SetCollisionResponseToAllChannels(ECR_Block);
 // Register once after setting the initial transform, mesh and collision.
 // This avoids repeated render/physics rebuilds during world generation.
 Block->RegisterComponent();
}
FVector AMoonfallArena::GetStartingSanctuaryLocation() const {
 return GetActorTransform().TransformPosition(FVector(0.f, 3500.f, 120.f));
}
void AMoonfallArena::BeginPlay() {
 Super::BeginPlay();
 CreateBlock(FVector(0,0,-60),FVector(30,30,1));
 CreateBlock(FVector(1500,0,200),FVector(1,30,5));
 CreateBlock(FVector(-1500,0,200),FVector(1,30,5));
 // The northern gate only opens if there is actual sanctuary terrain.
 // Opted-out authored arenas must retain their original solid northern wall.
 if (bEnableStartingSanctuary) {
  // Two wall segments leave the same 400-unit walking corridor as the south gate.
  CreateBlock(FVector(-850,1500,200),FVector(13,1,5));
  CreateBlock(FVector(850,1500,200),FVector(13,1,5));
  // One continuous walkable surface, with no level transition or loading door.
  CreateBlock(FVector(0,1850,-20),FVector(5,10,0.4f));
  CreateBlock(FVector(0,3500,-60),FVector(24,23,1));
  // Ruined outer walls stop before the central exit toward the old Crossroads.
  CreateBlock(FVector(-1200,3500,130),FVector(1,23,3.6f));
  CreateBlock(FVector(1200,3500,130),FVector(1,23,3.6f));
  CreateBlock(FVector(-650,4600,130),FVector(10,1,3.6f));
  CreateBlock(FVector(650,4600,130),FVector(10,1,3.6f));
  // Four moonstone relic plinths frame the initial shrine without collision
  // near the new-player spawn. Geometry is static and never ticks.
  CreateBlock(FVector(-900,3850,60),FVector(1.3f,1.3f,2.4f));
  CreateBlock(FVector(900,3850,60),FVector(1.3f,1.3f,2.4f));
  CreateBlock(FVector(-900,2800,60),FVector(1.3f,1.3f,2.4f));
  CreateBlock(FVector(900,2800,60),FVector(1.3f,1.3f,2.4f));
 } else {
  // Avoid an exit into empty world space when the sanctuary is disabled.
  CreateBlock(FVector(0,1500,200),FVector(30,1,5));
 }
 // Southern gate: a 400-unit opening aligns with the walkable dungeon road.
 // Split only this boundary; retain divine encounters, NPCs and shrine layout.
 CreateBlock(FVector(-850,-1500,200),FVector(13,1,5));
 CreateBlock(FVector(850,-1500,200),FVector(13,1,5));
 // Ground bridge between arena Y=-1500 and first dungeon floor edge Y=-2500.
 // Same top surface (Z=0) as the dungeon; no teleport or level load.
 CreateBlock(FVector(0,-2000,-20),FVector(5,10,0.4f));
 // Instanced, flat east/west road. Every one of the 30 entrances overlaps
 // the road by 200 units; the south arena bridge joins it at the center.
 // Only 30 mesh instances, not 30 extra actors or dynamic pathfinding ticks.
 DungeonRoad->ClearInstances();
 for (int32 Slot = 0; Slot < 30; ++Slot) {
  const float X = (static_cast<float>(Slot) - 14.5f) * 1350.f;
  DungeonRoad->AddInstance(FTransform(FRotator::ZeroRotator,
    FVector(X, -2500.f, -20.f), FVector(13.5f, 9.f, 0.4f)));
 }
 // One readable sign for each five-dungeon zone. UI/map code can also obtain
 // exact entrance positions from the game mode without scanning actors.
 // Noncolliding components cost no Tick and do not obstruct the road.
 for (int32 Zone = 0; Zone < 6; ++Zone) {
  UTextRenderComponent* Sign = NewObject<UTextRenderComponent>(
    this, FName(*FString::Printf(TEXT("DungeonZoneSign_%d"), Zone + 1)));
  if (!IsValid(Sign)) continue;
  Sign->SetupAttachment(GetRootComponent());
  Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  static const TCHAR* RegionNames[6] = {TEXT("ASHEN WILDS"), TEXT("MOONVEIL FOREST"), TEXT("HOLLOW DEPTHS"), TEXT("IRON FOUNDRY"), TEXT("FROSTBOUND REACH"), TEXT("ECLIPSE WASTES")};
  Sign->SetText(FText::FromString(FString::Printf(TEXT("%s | DUNGEONS %d-%d"), RegionNames[Zone], Zone * 5 + 1, Zone * 5 + 5)));
  Sign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
  Sign->SetWorldSize(65.f);
  Sign->SetTextRenderColor(FColor(114, 210, 236));
  const float ZoneCenterX = (static_cast<float>(Zone * 5 + 2) - 14.5f) * 1350.f;
  Sign->SetRelativeLocation(FVector(ZoneCenterX, -2500.f, 120.f));
  // Initialize before registration to avoid a post-register transform update.
  Sign->RegisterComponent();
  DungeonZoneSigns.Add(Sign);
 }
 if (bEnableStartingSanctuary) {
  // Clear, readable world-space directions instead of forced modal tutorials.
  // Static text never ticks, does not block combat, and requires no new input.
  struct FStarterSign { const TCHAR* Name; const TCHAR* Message; FVector Position; };
  const FStarterSign Signs[] = {
   {TEXT("SanctuaryWelcome"), TEXT("SHATTERED SANCTUARY\nFIRSTLIGHT SHRINE"), FVector(0.f, 3900.f, 140.f)},
   {TEXT("SanctuaryGuide"), TEXT("MEET THE GUIDE\nRIGHT-CLICK TO TALK"), FVector(100.f, 3290.f, 140.f)},
   {TEXT("SanctuaryControls"), TEXT("LEFT CLICK: ATTACK   RIGHT CLICK: TALK / SPELL\nB: INVENTORY   C: CHARACTER   J: ATLAS   ESC: SETTINGS"), FVector(-520.f, 3000.f, 150.f)},
   {TEXT("SanctuaryPractice"), TEXT("PRACTICE SENTRIES: NORTH CORNERS\nAPPROACH TO TRAIN DODGE AND ATTACK"), FVector(0.f, 4180.f, 140.f)},
   {TEXT("SanctuaryExit"), TEXT("SOUTH GATE: CROSSROADS\nFOLLOW THE ROAD TO ALL SIX REGIONS"), FVector(0.f, 2430.f, 140.f)}
  };
  // Two forgiving practice opponents at the far northern corners.
  // They cannot approach the spawn or shrine: their AI only activates when
  // the hero voluntarily comes close, and they have no pursuit speed.
  const FVector StarterPracticeOffsets[] = {
   FVector(-790.f, 4380.f, 60.f), FVector(790.f, 4380.f, 60.f)
  };
  for (int32 PracticeIndex = 0; PracticeIndex < 2; ++PracticeIndex) {
   const FTransform PracticeTransform(FRotator::ZeroRotator,
     GetActorTransform().TransformPosition(StarterPracticeOffsets[PracticeIndex]));
   AMoonfallEnemy* PracticeEnemy = GetWorld()
     ? GetWorld()->SpawnActorDeferred<AMoonfallEnemy>(
         AMoonfallEnemy::StaticClass(), PracticeTransform, this, nullptr,
         ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding)
     : nullptr;
   if (!IsValid(PracticeEnemy)) continue;
   PracticeEnemy->bBoss = false;
   PracticeEnemy->bElite = false;
   PracticeEnemy->RaceId = PracticeIndex;
   PracticeEnemy->Health = 35.f;
   PracticeEnemy->AttackDamage = 2.f;
   PracticeEnemy->AttackCooldown = 2.f;
   PracticeEnemy->ChaseSpeed = 0.f;
   PracticeEnemy->Tags.AddUnique(FName(TEXT("MoonfallSanctuaryPractice")));
   PracticeEnemy->FinishSpawning(PracticeTransform);
  }
  for (const FStarterSign& Entry : Signs) {
   UTextRenderComponent* Sign = NewObject<UTextRenderComponent>(this, FName(Entry.Name));
   if (!IsValid(Sign)) continue;
   Sign->SetupAttachment(GetRootComponent());
   Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Sign->SetGenerateOverlapEvents(false);
   Sign->SetText(FText::FromString(Entry.Message));
   Sign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
   Sign->SetWorldSize(36.f);
   Sign->SetTextRenderColor(FColor(180, 225, 245));
   Sign->SetRelativeLocation(Entry.Position);
   Sign->RegisterComponent();
   SanctuaryGuideSigns.Add(Sign);
  }
 }
 if (!GetWorld()) return;
 // World shrines are discovered by walking into their proximity, not by
 // defeating a boss or using an attack key. Stable IDs persist between saves.
 struct FShrinePlacement { const TCHAR* Id; FVector Offset; };
 const FShrinePlacement Shrines[] = {
  {TEXT("crossroads_west"), FVector(-1120.f, -720.f, 0.f)},
  {TEXT("crimson_path"), FVector(1080.f, -750.f, 0.f)},
  {TEXT("moonlit_path"), FVector(1040.f, 800.f, 0.f)},
  {TEXT("sanctuary_firstlight"), FVector(0.f, 3650.f, 0.f)}
 };
 for (const FShrinePlacement& Placement : Shrines) {
  if (!bEnableStartingSanctuary && FCString::Strcmp(Placement.Id, TEXT("sanctuary_firstlight")) == 0)
   continue;
  // Only the new sanctuary shrine uses the arena's local coordinates.
  // Keep the legacy region shrine positions exactly as before.
  const bool bFirstlight = FCString::Strcmp(Placement.Id, TEXT("sanctuary_firstlight")) == 0;
  const FVector ShrinePosition = bFirstlight
    ? GetActorTransform().TransformPosition(Placement.Offset)
    : GetActorLocation() + Placement.Offset;
  const FTransform SpawnTransform(FRotator::ZeroRotator, ShrinePosition);
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
 const bool bDeathGoddessEncounter = MoonfallDeathGoddess::IsDeathGoddess(FeaturedDivineBossId);
 const bool bSingleDivineEncounter = bZEncounter || bNyraEncounter || bDeathGoddessEncounter;
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
 const FVector GuidePosition = bEnableStartingSanctuary
   ? GetActorTransform().TransformPosition(FVector(260.f, 3320.f, 60.f))
   : GetActorLocation() + FVector(-350.f, 650.f, 60.f);
 if (AMoonfallWorldNPC* Guide = GetWorld()->SpawnActor<AMoonfallWorldNPC>(
     AMoonfallWorldNPC::StaticClass(), GuidePosition, FRotator::ZeroRotator)) {
  Guide->RaceId = 2; // Wood Elf
  Guide->SettlementId = FName(TEXT("Crossroads"));
  Guide->ResidentId = FName(TEXT("CrossroadsGuide"));
  Guide->FactionId = FName(TEXT("Wanderers"));
  Guide->SocialRole = FName(TEXT("Guide"));
  Guide->DisplayName = FText::FromString(TEXT("Wandering Guide"));
  Guide->Greeting = FText::FromString(TEXT("The Sanctuary is safe. Follow the southern gate to the Crossroads, then the road to all six regions."));
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
   Enemy->DivineBossId = bDeathGoddessEncounter ? MoonfallDeathGoddess::BossId()
     : (bZ ? FName(TEXT("z_void_reaper")) : FName(TEXT("nyra_eclipse_queen")));
   Enemy->DivineBossTitle = bDeathGoddessEncounter ? FText::FromString(TEXT("Morvella Velmora"))
     : (bZ ? FText::FromString(TEXT("Z, Void Reaper"))
           : FText::FromString(TEXT("Nyra, Eclipse Queen")));
   Enemy->DivineBossRegion = bDeathGoddessEncounter ? FName(TEXT("MoonlitNecropolis"))
     : (bZ ? FName(TEXT("ShatteredRuins")) : FName(TEXT("EclipseCitadel")));
   Enemy->Tags.Add(FName(TEXT("DivineWorldBoss")));
   Enemy->Health *= bDeathGoddessEncounter ? 4.25f : (bZ ? 4.f : 4.5f);
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
 // By default, Z and Nyra keep their existing showcase slots while Morvella
 // gains a third, separate reachable location off the sixth-zone public road.
 // The single-Morvella test preset already spawns her and must not duplicate.
 if (bSpawnMorvellaInWorld && bEnableStartingSanctuary && !bDeathGoddessEncounter)
  SpawnMorvellaWorldEncounter();
}

void AMoonfallArena::SpawnMorvellaWorldEncounter() {
 if (!GetWorld()) return;
 // Center of the sixth five-dungeon region: (27 - 14.5) * 1350 cm.
 // A broad spur overlaps the existing road at Y=-2500; the necropolis is
 // directly walkable without loading doors, teleports or extra levels.
 constexpr float X = 16875.f;
 CreateBlock(FVector(X, -1050.f, -20.f), FVector(5.5f, 34.f, 0.4f));
 CreateBlock(FVector(X, 850.f, -20.f), FVector(20.f, 18.f, 0.4f));
 // Low walls and four plinths shape a readable arena. The south approach
 // stays open and the floor remains navigable during every boss phase.
 CreateBlock(FVector(X - 1000.f, 850.f, 130.f), FVector(1.f, 18.f, 3.6f));
 CreateBlock(FVector(X + 1000.f, 850.f, 130.f), FVector(1.f, 18.f, 3.6f));
 CreateBlock(FVector(X, 1750.f, 130.f), FVector(20.f, 1.f, 3.6f));
 const float Sides[] = {-1.f, 1.f};
 for (const float Side : Sides) {
  CreateBlock(FVector(X + Side * 760.f, 375.f, 70.f), FVector(1.2f,1.2f,2.4f));
  CreateBlock(FVector(X + Side * 760.f, 1360.f, 70.f), FVector(1.2f,1.2f,2.4f));
 }
 struct FNecropolisSign { const TCHAR* Name; const TCHAR* Label; FVector Location; };
 const FNecropolisSign Signs[] = {
  {TEXT("MorvellaRoadSign"), TEXT("MOONLIT NECROPOLIS\\nMORVELLA VELMORA  -  GODDESS OF DEATH"),
   FVector(X,-2170.f,145.f)},
  {TEXT("MorvellaSanctumSign"), TEXT("THE CRIMSON REAPER\\nDEATH'S EMBRACE"),
   FVector(X,1540.f,160.f)}
 };
 for (const FNecropolisSign& Entry : Signs) {
  UTextRenderComponent* Sign = NewObject<UTextRenderComponent>(this, FName(Entry.Name));
  if (!IsValid(Sign)) continue;
  Sign->SetupAttachment(GetRootComponent());
  Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Sign->SetGenerateOverlapEvents(false);
  Sign->SetText(FText::FromString(Entry.Label));
  Sign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
  Sign->SetWorldSize(43.f);
  Sign->SetTextRenderColor(FColor(235,180,207));
  Sign->SetRelativeLocation(Entry.Location);
  Sign->RegisterComponent();
  DivineEncounterSigns.Add(Sign);
 }
 // One authored boss, rather than a generic pack or repeated-wave spawn.
 // Designers can provide a Blueprint subclass with the exact approved
 // Morvella artwork. Without that import, a standard visible enemy mesh
 // is used only as a temporary combat proxy.
 UClass* SpawnClass = MorvellaWorldActorClass
  ? MorvellaWorldActorClass.Get() : AMoonfallEnemy::StaticClass();
 const FTransform SpawnTransform(FRotator(0.f,-90.f,0.f),
   GetActorTransform().TransformPosition(FVector(X, 1080.f, 60.f)));
 AMoonfallEnemy* Morvella = GetWorld()->SpawnActorDeferred<AMoonfallEnemy>(
   SpawnClass, SpawnTransform, this, nullptr,
   ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
 if (!IsValid(Morvella)) {
  UE_LOG(LogTemp, Warning, TEXT("Moonfall: Morvella world boss failed to spawn"));
  return;
 }
 Morvella->bBoss = true;
 Morvella->bElite = true;
 Morvella->DivineBossId = MoonfallDeathGoddess::BossId();
 Morvella->DivineBossTitle = FText::FromString(TEXT("Morvella Velmora"));
 Morvella->DivineBossRegion = FName(TEXT("MoonlitNecropolis"));
 Morvella->Tags.AddUnique(FName(TEXT("DivineWorldBoss")));
 Morvella->Tags.AddUnique(FName(TEXT("MoonfallMorvellaWorldBoss")));
 Morvella->Health = 340.f;
 Morvella->AttackDamage = 12.f;
 Morvella->AttackRange = 170.f;
 Morvella->AttackCooldown = 1.45f;
 Morvella->ChaseSpeed = 155.f;
 UGameplayStatics::FinishSpawningActor(Morvella, SpawnTransform);
}
