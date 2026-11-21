#include "MoonfallExpandedWorld.h"
#include "MoonfallOverworldTile.h"
#include "MoonfallGothicFerry.h"
#include "MoonfallCheckpointShrine.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallEnemy.h"
#include "MoonfallHeroPawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

AMoonfallExpandedWorld::AMoonfallExpandedWorld()
{
 PrimaryActorTick.bCanEverTick = false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("OutdoorWorldRoot")));
 ConnectingRoads = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ProvinceRoads"));
 ConnectingRoads->SetupAttachment(GetRootComponent());
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) ConnectingRoads->SetStaticMesh(Cube.Object);
 ConnectingRoads->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 ConnectingRoads->SetCollisionResponseToAllChannels(ECR_Block);
 ConnectingRoads->SetGenerateOverlapEvents(false);
 ConnectingRoads->SetCanEverAffectNavigation(true);
}

FVector AMoonfallExpandedWorld::RegionCenter(int32 RegionIndex)
{
 // Six separate 1km x 1km territories in a 3-column x 2-row arrangement.
 // Their 200m gaps are linked by actual walkable roads, not load screens.
 // A seventh 1km x 1km island remains offshore for a future real boat route.
 static const FVector Centers[RegionCount] = {
  FVector(-120000.f, 62000.f, 0.f),  // 0 Ashen Wilds
  FVector(0.f, 62000.f, 0.f),        // 1 Moonveil Forest
  FVector(120000.f, 62000.f, 0.f),   // 2 Hollow Depths
  FVector(-120000.f, 182000.f, 0.f), // 3 Iron Foundry
  FVector(0.f, 182000.f, 0.f),       // 4 Frostbound Reach
  FVector(120000.f, 182000.f, 0.f),  // 5 Eclipse Wastes
  FVector(0.f, 302000.f, 0.f)        // 6 Gothic Isle
 };
 return RegionIndex >= 0 && RegionIndex < RegionCount
   ? Centers[RegionIndex] : FVector::ZeroVector;
}

const TCHAR* AMoonfallExpandedWorld::RegionName(int32 RegionIndex)
{
 static const TCHAR* Names[RegionCount] = {
  TEXT("ASHEN WILDS"), TEXT("MOONVEIL FOREST"), TEXT("HOLLOW DEPTHS"),
  TEXT("IRON FOUNDRY"), TEXT("FROSTBOUND REACH"), TEXT("ECLIPSE WASTES"),
  TEXT("GOTHIC ISLE")
 };
 return RegionIndex >= 0 && RegionIndex < RegionCount
   ? Names[RegionIndex] : TEXT("UNKNOWN REGION");
}

FVector AMoonfallExpandedWorld::TileCenter(int32 RegionIndex, int32 Column, int32 Row)
{
 if (RegionIndex < 0 || RegionIndex >= RegionCount
     || Column < 0 || Column >= AMoonfallOverworldTile::TilesPerSide
     || Row < 0 || Row >= AMoonfallOverworldTile::TilesPerSide)
  return FVector::ZeroVector;
 return RegionCenter(RegionIndex) + FVector(
   (Column - 2) * AMoonfallOverworldTile::TileSize,
   (Row - 2) * AMoonfallOverworldTile::TileSize, 0.f);
}

void AMoonfallExpandedWorld::AddRoad(const FVector& Start, const FVector& End, float Width)
{
 const FVector Delta = End - Start;
 const float Length = Delta.Size2D();
 if (!FMath::IsFinite(Length) || Length < 100.f || Width <= 0.f
     || Start.ContainsNaN() || End.ContainsNaN()) return;
 const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
 // Cube has 100cm unscaled edge. Road's top is at Z=0, matching terrain.
 const FVector Midpoint = (Start + End) * 0.5f + FVector(0.f, 0.f, -10.f);
 ConnectingRoads->AddInstance(FTransform(FRotator(0.f, Yaw, 0.f),
   Midpoint, FVector(Length / 100.f, Width / 100.f, 0.2f)));
}

void AMoonfallExpandedWorld::BuildConnectingRoads()
{
 ConnectingRoads->ClearInstances();
 // Bridge the existing Shattered Sanctuary north gate to Moonveil's
 // south edge, leaving all 30 dungeon entrances/legacy road untouched.
 AddRoad(FVector(0.f, 4400.f, 0.f), FVector(0.f, 15000.f, 0.f), 1600.f);
 const float RowCenters[2] = {62000.f, 182000.f};
 const float Columns[3] = {-120000.f, 0.f, 120000.f};
 for (const float Y : RowCenters) {
  AddRoad(FVector(Columns[0], Y, 0.f), FVector(Columns[1], Y, 0.f), 1600.f);
  AddRoad(FVector(Columns[1], Y, 0.f), FVector(Columns[2], Y, 0.f), 1600.f);
 }
 for (const float X : Columns)
  AddRoad(FVector(X, RowCenters[0], 0.f),
          FVector(X, RowCenters[1], 0.f), 1600.f);
 // Distinct mainland and island shore docks. The Nightferry actor
 // travels visibly through the open-water gap; no false walkable sea bridge.
 AddRoad(FVector(0.f, 228000.f, 0.f), FVector(0.f, 238000.f, 0.f), 2600.f);
 AddRoad(FVector(0.f, 246000.f, 0.f), FVector(0.f, 256000.f, 0.f), 2600.f);
}

void AMoonfallExpandedWorld::SpawnGothicIsleServices()
{
 // Two real, persistently identified discovery/respawn shrines and five
 // existing quest/dialogue residents, using the same gameplay classes as
 // the mainland. Their positions are on the five island district centers.
 // No new saves, character art, actor Tick or map transitions are required.
 UWorld* World = GetWorld();
 if (!World) return;
 const FVector Isle = RegionCenter(6);
 struct FIslandShrineSpec {
  const TCHAR* Id;
  const TCHAR* Name;
  FVector Offset;
 };
 const FIslandShrineSpec Shrines[] = {
  {TEXT("gothic_ravens_landing"), TEXT("Raven's Landing Shrine"), FVector(650.f, -40000.f, 0.f)},
  {TEXT("gothic_blackbell"), TEXT("Blackbell Cathedral Shrine"), FVector(650.f, 0.f, 0.f)}
 };
 // Inventory existing services once, rather than walking the actor
 // registry separately for each new landmark.
 TSet<FName> ExistingShrineIds;
 for (TActorIterator<AMoonfallCheckpointShrine> It(World); It; ++It)
  if (IsValid(*It) && !It->ShrineId.IsNone())
   ExistingShrineIds.Add(It->ShrineId);
 for (const FIslandShrineSpec& Spec : Shrines) {
  const FName StableId(Spec.Id);
  if (ExistingShrineIds.Contains(StableId)) continue;
  const FTransform Transform(FRotator::ZeroRotator, Isle + Spec.Offset);
  AMoonfallCheckpointShrine* Shrine = World->SpawnActorDeferred<AMoonfallCheckpointShrine>(
    AMoonfallCheckpointShrine::StaticClass(), Transform, this, nullptr,
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (!IsValid(Shrine)) continue;
  Shrine->ShrineId = StableId;
  Shrine->MapDisplayName = FText::FromString(Spec.Name);
  Shrine->FinishSpawning(Transform);
  if (IsValid(Shrine) && !Shrine->IsActorBeingDestroyed()) {
   ExistingShrineIds.Add(StableId);
   if (AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(
       UGameplayStatics::GetPlayerPawn(this, 0)))
    Shrine->CheckHeroAtSpawn(Hero);
  }
 }
 struct FIslandResidentSpec {
  const TCHAR* Id;
  const TCHAR* Name;
  const TCHAR* Greeting;
  const TCHAR* StoryClue;
  const TCHAR* Role;
  const TCHAR* Faction;
  FVector Offset;
  int32 Race;
 };
 const FIslandResidentSpec Residents[] = {
  {TEXT("GothicRavenHarbormaster"), TEXT("Mara Vesper"),
   TEXT("The tide has carried stranger things than you. Welcome to Raven's Landing."),
   TEXT("The harbor manifests name six cities, but every ship arrived on the same impossible night."),
   TEXT("Scout"), TEXT("GothicHarbor"), FVector(-620.f, -40000.f, 60.f), 1},
  {TEXT("GothicBlackbellKeeper"), TEXT("Brother Corvin"),
   TEXT("The Blackbell rings for the living and the lost."),
   TEXT("Six witnesses are carved inside the bell. The seventh name was erased."),
   TEXT("Elder"), TEXT("BlackbellOrder"), FVector(-620.f, 0.f, 60.f), 3},
  {TEXT("GothicGraveWarden"), TEXT("Veyra Ash"),
   TEXT("Do not disturb the names beneath these stones."),
   TEXT("These graves hold no bodies, only the names of those who carried evidence across the sea."),
   TEXT("Guard"), TEXT("Gravewardens"), FVector(40000.f, -620.f, 60.f), 4},
  {TEXT("GothicDuskwatchSentinel"), TEXT("Rook Thorn"),
   TEXT("Duskwatch keeps its lanterns lit until dawn."),
   TEXT("The wreckers copied a forged signal from the same command that corrupted the foundry."),
   TEXT("Guard"), TEXT("Duskwatch"), FVector(-40000.f, -620.f, 60.f), 6},
  {TEXT("GothicThornwraithScholar"), TEXT("Elias Briar"),
   TEXT("The grove remembers every footstep."),
   TEXT("The roots shelter a star chart pointing toward this island, not toward the eclipse."),
   TEXT("Scholar"), TEXT("Thornwraith"), FVector(-620.f, 40000.f, 60.f), 8}
 };
 TSet<FName> ExistingResidentIds;
 for (TActorIterator<AMoonfallWorldNPC> It(World); It; ++It)
  if (IsValid(*It) && !It->ResidentId.IsNone())
   ExistingResidentIds.Add(It->ResidentId);
 for (const FIslandResidentSpec& Spec : Residents) {
  const FName StableId(Spec.Id);
  if (ExistingResidentIds.Contains(StableId)) continue;
  const FTransform Transform(FRotator::ZeroRotator, Isle + Spec.Offset);
  AMoonfallWorldNPC* NPC = World->SpawnActorDeferred<AMoonfallWorldNPC>(
    AMoonfallWorldNPC::StaticClass(), Transform, this, nullptr,
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (!IsValid(NPC)) continue;
  NPC->ResidentId = StableId;
  NPC->DisplayName = FText::FromString(Spec.Name);
  NPC->Greeting = FText::FromString(Spec.Greeting);
  NPC->CompletedStoryClue = FText::FromString(Spec.StoryClue);
  NPC->SocialRole = FName(Spec.Role);
  NPC->FactionId = FName(Spec.Faction);
  NPC->SettlementId = FName(TEXT("GothicIsle"));
  NPC->RaceId = Spec.Race;
  NPC->FinishSpawning(Transform);
  if (IsValid(NPC) && !NPC->IsActorBeingDestroyed())
   ExistingResidentIds.Add(StableId);
 }
}

void AMoonfallExpandedWorld::UpdateGothicIsleEncounters(const FVector& PlayerPosition)
{
 // Five district patrols and one separate cathedral guardian reuse the
 // existing enemy AI, XP and loot without an island-wide enemy swarm.
 // A defeated encounter stays cleared until the player leaves its district.
 UWorld* World = GetWorld();
 if (!World || PlayerPosition.ContainsNaN()) return;
 // Keep range-based cleanup active even during defeat, but never spawn
 // a new hostile encounter around a dead or unloaded player pawn.
 const AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(
   UGameplayStatics::GetPlayerPawn(this, 0));
 const bool bHeroReady = IsValid(Hero) && FMath::IsFinite(Hero->Health)
   && Hero->Health > 0.f;
 struct FIslandEncounterSpec {
  const TCHAR* Tag;
  FVector Offset;
  int32 Race;
  bool bElite;
  bool bBoss;
 };
 const FIslandEncounterSpec Encounters[] = {
  {TEXT("GothicHarborReaver"), FVector(3600.f, -37500.f, 90.f), 1, false, false},
  {TEXT("GothicBlackbellPenitent"), FVector(3600.f, 2500.f, 90.f), 3, true, false},
  {TEXT("GothicGraveSentinel"), FVector(37500.f, 3600.f, 90.f), 4, true, false},
  {TEXT("GothicDuskwatchOutcast"), FVector(-37500.f, 3600.f, 90.f), 6, false, false},
  {TEXT("GothicThornwraithStalker"), FVector(3600.f, 37500.f, 90.f), 8, true, false},
  // Away from the cathedral NPC, but still within the same walkable tile.
  {TEXT("GothicBlackbellWarden"), FVector(12500.f, 12500.f, 90.f), 3, false, true}
 };
 const FVector Isle = RegionCenter(6);
 for (int32 Index = 0; Index < UE_ARRAY_COUNT(Encounters); ++Index) {
  const FVector Site = Isle + Encounters[Index].Offset;
  const float DistanceSquared = FVector::DistSquared2D(PlayerPosition, Site);
  if (DistanceSquared > FMath::Square(18000.f)) {
   // This encounter is no longer streamed in: reclaim its AI actor and
   // permit a fresh encounter only after the hero returns later.
   if (TWeakObjectPtr<AMoonfallEnemy>* Existing = IslandEncounters.Find(Index)) {
    if (AMoonfallEnemy* Enemy = Existing->Get(); IsValid(Enemy) && Enemy->Health > 0.f)
     Enemy->Destroy();
   }
   IslandEncounters.Remove(Index);
   ClearedIslandEncounters.Remove(Index);
   IslandSpawnRetryAt.Remove(Index);
   continue;
  }
  if (TWeakObjectPtr<AMoonfallEnemy>* Existing = IslandEncounters.Find(Index)) {
   AMoonfallEnemy* Enemy = Existing->Get();
   if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed() || Enemy->Health <= 0.f) {
    // Destroying and invalid encounters must be removed from the live
    // registry. Preserve visit-based clearing so an enemy killed and
    // destroyed between refreshes cannot immediately respawn.
    ClearedIslandEncounters.Add(Index);
    IslandEncounters.Remove(Index);
   }
  }
  if (!bHeroReady || DistanceSquared > FMath::Square(11000.f)
      || ClearedIslandEncounters.Contains(Index)
      || IslandEncounters.Contains(Index)) continue;
  // The island is streamed in 200m tiles. Never instantiate a hostile
  // enemy over terrain that has not finished loading beneath its site.
  // The next 0.35s refresh will retry after the terrain tile is ready.
  const int32 SiteColumn = FMath::FloorToInt(
    (Encounters[Index].Offset.X + 50000.f) / AMoonfallOverworldTile::TileSize);
  const int32 SiteRow = FMath::FloorToInt(
    (Encounters[Index].Offset.Y + 50000.f) / AMoonfallOverworldTile::TileSize);
  if (SiteColumn < 0 || SiteColumn >= AMoonfallOverworldTile::TilesPerSide
      || SiteRow < 0 || SiteRow >= AMoonfallOverworldTile::TilesPerSide) continue;
  const int32 SiteTileId = 6 * TilesPerRegion
    + SiteRow * AMoonfallOverworldTile::TilesPerSide + SiteColumn;
  const TObjectPtr<AMoonfallOverworldTile>* GroundTile = ResidentTiles.Find(SiteTileId);
  if (!GroundTile || !IsValid(GroundTile->Get())
      || GroundTile->Get()->IsActorBeingDestroyed()) continue;
  // Collision failures must not trigger a spawn attempt every 0.35s.
  const float Now = World->GetTimeSeconds();
  if (!FMath::IsFinite(Now)) continue;
  if (const float* RetryAt = IslandSpawnRetryAt.Find(Index)) {
   if (Now < *RetryAt) continue;
  }
  const FTransform Transform(FRotator::ZeroRotator, Site);
  AMoonfallEnemy* Enemy = World->SpawnActorDeferred<AMoonfallEnemy>(
    AMoonfallEnemy::StaticClass(), Transform, this, nullptr,
    ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
  if (!IsValid(Enemy)) {
   IslandSpawnRetryAt.Add(Index, Now + 2.5f);
   continue;
  }
  // This is a generic island boss, NOT a dungeon-ledger or divine boss.
  // No original hero or boss character artwork is modified.
  Enemy->RaceId = Encounters[Index].Race;
  Enemy->bElite = Encounters[Index].bElite;
  Enemy->bBoss = Encounters[Index].bBoss;
  Enemy->Tags.AddUnique(FName(Encounters[Index].Tag));
  Enemy->FinishSpawning(Transform);
  if (IsValid(Enemy) && !Enemy->IsActorBeingDestroyed()) {
   IslandSpawnRetryAt.Remove(Index);
   IslandEncounters.Add(Index, Enemy);
  } else IslandSpawnRetryAt.Add(Index, Now + 2.5f);
 }
}

void AMoonfallExpandedWorld::BeginPlay()
{
 Super::BeginPlay();
 BuildConnectingRoads();
 SpawnGothicIsleServices();
 // One boat serves both shore docks and reverses on the next boarding.
 // Preserve a designer-authored ferry rather than duplicating it.
 bool bFerryAlreadyExists = false;
 for (TActorIterator<AMoonfallGothicFerry> It(GetWorld()); It; ++It)
  if (IsValid(*It)) { bFerryAlreadyExists = true; break; }
 if (!bFerryAlreadyExists && GetWorld())
  GetWorld()->SpawnActor<AMoonfallGothicFerry>(
    AMoonfallGothicFerry::DockLocation(false), FRotator::ZeroRotator);
 RefreshNearbyTiles();
 // The timer owns all scenery streaming; the manager and each tile do NOT Tick.
 GetWorldTimerManager().SetTimer(StreamingTimer, this,
   &AMoonfallExpandedWorld::RefreshNearbyTiles, 0.35f, true, 0.1f);
 // Water movement stays responsive without requiring a per-frame actor Tick.
 GetWorldTimerManager().SetTimer(SeaSwimmingTimer, this,
   &AMoonfallExpandedWorld::UpdateSeaSwimming, 0.1f, true);
}

void AMoonfallExpandedWorld::EndPlay(const EEndPlayReason::Type Reason)
{
 if (GetWorld()) {
  GetWorldTimerManager().ClearTimer(StreamingTimer);
  GetWorldTimerManager().ClearTimer(SeaSwimmingTimer);
 }
 SeaSwimmer.Reset();
 for (const TPair<int32, TWeakObjectPtr<AMoonfallEnemy>>& Entry : IslandEncounters) {
  if (AMoonfallEnemy* Enemy = Entry.Value.Get(); IsValid(Enemy) && Enemy->Health > 0.f)
   Enemy->Destroy();
 }
 IslandEncounters.Empty();
 ClearedIslandEncounters.Empty();
 IslandSpawnRetryAt.Empty();
 ResidentTiles.Empty();
 Super::EndPlay(Reason);
}

void AMoonfallExpandedWorld::UpdateSeaSwimming()
{
 // Short 80m sea gap between mainland and Gothic Isle, with the water
 // surface slightly below the docks. Leaving the ferry is the player's
 // choice: do not move, teleport or attach an overboard character.
 AMoonfallHeroPawn* Hero = Cast<AMoonfallHeroPawn>(
   UGameplayStatics::GetPlayerPawn(this, 0));
 if (!IsValid(Hero)) {
  SeaSwimmer.Reset();
  return;
 }
 UCharacterMovementComponent* Movement = Hero->GetCharacterMovement();
 if (!IsValid(Movement)) {
  SeaSwimmer.Reset();
  return;
 }
 // Dead heroes belong to the existing death/respawn system. Never revive
 // their motion or teleport them as part of sea detection.
 if (!FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f) {
  SeaSwimmer.Reset();
  return;
 }
 const FVector Position = Hero->GetActorLocation();
 if (Position.ContainsNaN()) {
  SeaSwimmer.Reset();
  return;
 }
 // The 80m crossing has open water outside the 26m-wide piers.
 // Allow swimming a reasonable distance to either side of the vessel,
 // but keep the Y boundary inside the sea gap to avoid marking land wet.
 const bool bSeaGap = FMath::Abs(Position.X) < 5000.f
   && Position.Y > 238000.f && Position.Y < 246000.f;
 const bool bInWater = bSeaGap && Position.Z < -40.f;
 // Hysteresis: enter once submerged, but do not flip straight back to
 // falling when natural swimming briefly reaches the water surface.
 const bool bLeaveWater = !bSeaGap || Position.Z > 60.f;
 // Existing deck movement wins over water. It is never overridden
 // merely because the boat sails across the water.
 const bool bOnFerry = Hero->GetMovementBase()
   && Cast<AMoonfallGothicFerry>(Hero->GetMovementBase()->GetOwner()) != nullptr;
 if (bInWater && !bOnFerry && Movement->MovementMode == MOVE_Falling) {
  Movement->SetMovementMode(MOVE_Swimming);
  SeaSwimmer = Hero;
 } else if (SeaSwimmer.Get() == Hero) {
  // Other gameplay systems may cancel swimming on their own. Drop our
  // tracking state rather than overriding their chosen movement mode.
  if (Movement->MovementMode != MOVE_Swimming) {
   SeaSwimmer.Reset();
  } else if (bOnFerry) {
   // The player is already physically standing on the vessel.
   // Return to walking on that existing moving base, never force a rescue.
   Movement->SetMovementMode(MOVE_Walking);
   SeaSwimmer.Reset();
  } else if (bLeaveWater) {
   // If shoreline collision already gives the hero a non-ferry movement
   // base, keep their walking movement. Otherwise fall normally until
   // ground contact. Neither path repositions the player.
   const bool bOnShoreSurface = IsValid(Hero->GetMovementBase())
     && !bOnFerry;
   Movement->SetMovementMode(bOnShoreSurface ? MOVE_Walking : MOVE_Falling);
   SeaSwimmer.Reset();
  }
 }
}

void AMoonfallExpandedWorld::RefreshNearbyTiles()
{
 UWorld* World = GetWorld();
 if (!World) return;
 const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
 if (!IsValid(Player)) return;
 const FVector PlayerPosition = Player->GetActorLocation();
 if (PlayerPosition.ContainsNaN()) return;
 UpdateGothicIsleEncounters(PlayerPosition);

 // Remove expired tiles in place: one map walk, no temporary ID array
 // or second lookup. The wider radius still prevents tile-edge churn.
 for (auto It = ResidentTiles.CreateIterator(); It; ++It) {
  AMoonfallOverworldTile* Tile = It.Value().Get();
  // Tiles already being destroyed must not hold a resident slot, even
  // when still within the unload radius.
  if (IsValid(Tile) && !Tile->IsActorBeingDestroyed()
      && FVector::DistSquared2D(PlayerPosition,
       Tile->GetActorLocation()) <= FMath::Square(UnloadRadius))
   continue;
  if (IsValid(Tile) && !Tile->IsActorBeingDestroyed()) Tile->Destroy();
  It.RemoveCurrent();
 }
 // Unloading and island encounters must run even at capacity. Only skip
 // new-candidate generation when there is no room to spawn another tile.
 if (ResidentTiles.Num() >= MaxResidentTiles) return;

 // Only sort candidate tiles near the hero. The complete world is NEVER
 // instantiated at startup. One update creates at most three tiles.
 TArray<TPair<float, int32>> Candidates;
 for (int32 Region = 0; Region < RegionCount; ++Region) {
  // A region's 5x5 tile centers extend at most two tile widths from
  // its center on each axis. If even the closest possible tile center
  // is outside LoadRadius, skip all 25 candidate tests for that region.
  const FVector Center = RegionCenter(Region);
  const float MaxTileCenterOffset = 2.f * AMoonfallOverworldTile::TileSize;
  const float CandidateReach = LoadRadius + MaxTileCenterOffset;
  if (FMath::Abs(PlayerPosition.X - Center.X) > CandidateReach
      || FMath::Abs(PlayerPosition.Y - Center.Y) > CandidateReach)
   continue;
  for (int32 Row = 0; Row < AMoonfallOverworldTile::TilesPerSide; ++Row) {
   for (int32 Column = 0; Column < AMoonfallOverworldTile::TilesPerSide; ++Column) {
    const int32 Id = Region * TilesPerRegion
      + Row * AMoonfallOverworldTile::TilesPerSide + Column;
    if (ResidentTiles.Contains(Id)) continue;
    const float D2 = FVector::DistSquared2D(PlayerPosition,
      TileCenter(Region, Column, Row));
    if (D2 <= FMath::Square(LoadRadius))
     Candidates.Emplace(D2, Id);
   }
  }
 }
 Candidates.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) {
  return A.Key == B.Key ? A.Value < B.Value : A.Key < B.Key;
 });
 int32 Spawns = 0;
 for (const TPair<float, int32>& Candidate : Candidates) {
  if (Spawns >= MaxSpawnsPerRefresh || ResidentTiles.Num() >= MaxResidentTiles)
   break;
  const int32 Id = Candidate.Value;
  const int32 Region = Id / TilesPerRegion;
  const int32 Index = Id % TilesPerRegion;
  const int32 Column = Index % AMoonfallOverworldTile::TilesPerSide;
  const int32 Row = Index / AMoonfallOverworldTile::TilesPerSide;
  const FTransform TileTransform(FRotator::ZeroRotator, TileCenter(Region, Column, Row));
  AMoonfallOverworldTile* Tile = World->SpawnActorDeferred<AMoonfallOverworldTile>(
    AMoonfallOverworldTile::StaticClass(), TileTransform, this, nullptr,
    ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (!IsValid(Tile)) continue;
  Tile->RegionIndex = Region;
  Tile->Column = Column;
  Tile->Row = Row;
  Tile->FinishSpawning(TileTransform);
  if (IsValid(Tile) && !Tile->IsActorBeingDestroyed()) {
   ResidentTiles.Add(Id, Tile);
   ++Spawns;
  }
 }
}
