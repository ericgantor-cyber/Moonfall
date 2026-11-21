#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallMoralityRules.h"
#include "TimerManager.h"
#include "MoonfallCheckpointShrine.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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
 // Fixed instance batches. They never collide, tick, or obstruct navmesh.
 CouncilProtectionDecor = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CouncilProtectionDecor"));
 CouncilReconstructionDecor = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CouncilReconstructionDecor"));
 CouncilFortificationDecor = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CouncilFortificationDecor"));
 HopeSettlementDecor = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HopeSettlementDecor"));
 ShadowSettlementDecor = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ShadowSettlementDecor"));
 UInstancedStaticMeshComponent* Sets[] = {
  CouncilProtectionDecor, CouncilReconstructionDecor, CouncilFortificationDecor,
  HopeSettlementDecor, ShadowSettlementDecor};
 for (UInstancedStaticMeshComponent* Set : Sets) {
  Set->SetupAttachment(GetRootComponent());
  Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Set->SetGenerateOverlapEvents(false);
  if (Cube.Succeeded()) Set->SetStaticMesh(Cube.Object);
  Set->SetVisibility(false);
 }

}
void AMoonfallArena::StepCouncilWorkRoutes() {
 if (!GetWorld() || CouncilMobileResidents.IsEmpty()) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(
     UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero) || Hero->CouncilDecisionId < 0) return;
 for (const TWeakObjectPtr<AMoonfallWorldNPC>& Resident : CouncilMobileResidents) {
  if (Resident.IsValid()) Resident->AdvanceCouncilWorkStep(Hero, 0.2f);
 }
}
void AMoonfallArena::RefreshCouncilSettlementVisuals() {
 if (!GetWorld() || !CouncilProtectionDecor || !CouncilReconstructionDecor
     || !CouncilFortificationDecor || !HopeSettlementDecor
     || !ShadowSettlementDecor) return;
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
 const int32 Policy = IsValid(Hero) && Hero->CouncilDecisionId >= 0
     && Hero->CouncilDecisionId <= 2 ? Hero->CouncilDecisionId : -1;
 // Save-backed morality silhouettes change only when a threshold is crossed.
 const int32 MoralTier = IsValid(Hero)
     ? MoonfallMorality::WorldTier(Hero->MoralAlignment) : 0;
 // Stationary residents reuse the existing two-second Council refresh.
 // A bounded authored list avoids a recurring global actor scan.
 for (const TWeakObjectPtr<AMoonfallWorldNPC>& Resident : CouncilStationResidents) {
  if (Resident.IsValid()) Resident->RefreshCouncilStationActivity(Hero);
 }
 if (Policy == LastDisplayedCouncilDecision
     && MoralTier == LastDisplayedMoralityTier) return;
 const bool bPolicyChanged = Policy != LastDisplayedCouncilDecision;
 LastDisplayedCouncilDecision = Policy;
 LastDisplayedMoralityTier = MoralTier;
 HopeSettlementDecor->SetVisibility(MoralTier > 0);
 ShadowSettlementDecor->SetVisibility(MoralTier < 0);
 // A policy change is rare. Update authored resident labels once per
 // transition, not in NPC Tick or a per-frame resident scan.
 if (bPolicyChanged) {
  for (TActorIterator<AMoonfallWorldNPC> It(GetWorld()); It; ++It) {
   if (IsValid(*It)) It->RefreshCouncilAssignmentSign(Hero);
  }
 }
 CouncilProtectionDecor->SetVisibility(Policy == 0);
 CouncilReconstructionDecor->SetVisibility(Policy == 1);
 CouncilFortificationDecor->SetVisibility(Policy == 2);
 if (IsValid(CouncilWorldSign)) {
  const TCHAR* Title = Policy == 0 ? TEXT("PEOPLE FIRST | COUNCIL PROTECTION")
      : Policy == 1 ? TEXT("REBUILD THE CROSSROADS | COUNCIL WORKS")
      : Policy == 2 ? TEXT("FORTIFY THE BORDERS | COUNCIL WATCH")
      : TEXT("CROSSROADS COUNCIL | POLICY UNDECIDED");
  const FString MoralSuffix = IsValid(Hero)
      ? FString::Printf(TEXT(" | %s"), ANSI_TO_TCHAR(
          MoonfallMorality::WorldState(Hero->MoralAlignment))) : TEXT("");
  CouncilWorldSign->SetText(FText::FromString(FString(Title) + MoralSuffix));
  CouncilWorldSign->SetTextRenderColor(
      Policy == 0 ? FColor(118, 225, 196)
      : Policy == 1 ? FColor(245, 192, 120)
      : Policy == 2 ? FColor(166, 187, 245) : FColor(194, 194, 203));
 }
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
 // The two morality sets use fixed authored shapes. No player morphs,
 // skeletal replacements, spawned actors, mesh assets, or new collision.
 if (HopeSettlementDecor && ShadowSettlementDecor) {
  const FVector HopePosts[] = {
    FVector(-1160.f,-840.f,60.f), FVector(-1160.f,0.f,60.f),
    FVector(-1160.f,840.f,60.f), FVector(1050.f,0.f,60.f)};
  const FVector ShadowSpikes[] = {
    FVector(1120.f,-850.f,75.f), FVector(1120.f,0.f,75.f),
    FVector(1120.f,850.f,75.f), FVector(-970.f,0.f,75.f)};
  for (const FVector& Pos : HopePosts) {
   HopeSettlementDecor->AddInstance(FTransform(FRotator(0.f,45.f,0.f),
       Pos, FVector(0.60f,0.60f,1.65f)));
  }
  for (const FVector& Pos : ShadowSpikes) {
   ShadowSettlementDecor->AddInstance(FTransform(FRotator(0.f,24.f,0.f),
       Pos, FVector(0.75f,0.75f,2.20f)));
  }
 }
 // Council choices update the Crossroads' appearance, not its collision
 // geometry. Sparse instanced props avoid actors, dynamic materials and Tick.
 if (CouncilProtectionDecor && CouncilReconstructionDecor && CouncilFortificationDecor) {
  const FVector Protection[] = {
   FVector(-1370.f,-1060.f,110.f), FVector(-1370.f,-350.f,110.f),
   FVector(-1370.f,350.f,110.f), FVector(-1370.f,1050.f,110.f)};
  const FVector Reconstruction[] = {
   FVector(1130.f,-1040.f,80.f), FVector(1130.f,-340.f,80.f),
   FVector(1130.f,360.f,80.f), FVector(1130.f,1060.f,80.f)};
  const FVector Fortification[] = {
   FVector(1320.f,-1130.f,160.f), FVector(1320.f,-430.f,160.f),
   FVector(1320.f,270.f,160.f), FVector(1320.f,970.f,160.f)};
  for (const FVector& Pos : Protection)
   CouncilProtectionDecor->AddInstance(FTransform(FRotator(0.f,45.f,0.f),
     Pos, FVector(0.65f,0.65f,2.2f)));
  for (const FVector& Pos : Reconstruction)
   CouncilReconstructionDecor->AddInstance(FTransform(FRotator::ZeroRotator,
     Pos, FVector(2.2f,1.4f,1.6f)));
  for (const FVector& Pos : Fortification)
   CouncilFortificationDecor->AddInstance(FTransform(FRotator::ZeroRotator,
     Pos, FVector(1.4f,1.4f,3.2f)));
 }
 CouncilWorldSign = NewObject<UTextRenderComponent>(this, TEXT("CrossroadsCouncilPolicySign"));
 if (IsValid(CouncilWorldSign)) {
  CouncilWorldSign->SetupAttachment(GetRootComponent());
  CouncilWorldSign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  CouncilWorldSign->SetGenerateOverlapEvents(false);
  CouncilWorldSign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
  CouncilWorldSign->SetWorldSize(46.f);
  CouncilWorldSign->SetRelativeLocation(FVector(840.f,-850.f,260.f));
  CouncilWorldSign->RegisterComponent();
 }
 RefreshCouncilSettlementVisuals();
 if (GetWorld()) {
  FTimerHandle CouncilVisualRefreshTimer;
  GetWorld()->GetTimerManager().SetTimer(CouncilVisualRefreshTimer,
      this, &AMoonfallArena::RefreshCouncilSettlementVisuals, 2.0f, true);
 }
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
   if (Person->SocialRole == FName(TEXT("Merchant"))
       || Person->SocialRole == FName(TEXT("Tinker")))
    CouncilStationResidents.Add(Person);
   // At most four authored guard/scout patrols. A missing/invalid NavMesh
   // disables movement while leaving residents at their quest locations.
   if (CouncilMobileResidents.Num() < 4
       && (Person->SocialRole == FName(TEXT("Guard"))
           || Person->SocialRole == FName(TEXT("Scout")))
       // Guards inspect the eastern watch lane; scouts survey a western lane.
       // These short authored offsets keep residents inside the Crossroads.
       && Person->ConfigureCouncilWorkRoute(
           Person->SocialRole == FName(TEXT("Guard"))
           ? FVector(100.f, 0.f, 0.f) : FVector(-100.f, 0.f, 0.f)))
    CouncilMobileResidents.Add(Person);
  }
 }
 if (!CouncilMobileResidents.IsEmpty()) {
  FTimerHandle CouncilWorkTimer;
  GetWorld()->GetTimerManager().SetTimer(CouncilWorkTimer,
     this, &AMoonfallArena::StepCouncilWorkRoutes, 0.2f, true);
 }
 // BeginPlay builds resident actors after the first policy refresh.
 // Force one post-spawn refresh so a loaded policy is visible immediately,
 // even when it was already resolved at the start of BeginPlay.
 LastDisplayedCouncilDecision = -2;
 RefreshCouncilSettlementVisuals();
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
