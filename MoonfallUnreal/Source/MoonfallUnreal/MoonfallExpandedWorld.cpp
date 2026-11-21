#include "MoonfallExpandedWorld.h"
#include "MoonfallOverworldTile.h"
#include "MoonfallGothicFerry.h"
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

void AMoonfallExpandedWorld::BeginPlay()
{
 Super::BeginPlay();
 BuildConnectingRoads();
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
 // Restrict this first-pass water state to the actual ferry corridor.
 // The 26m-wide piers are centered on X=0; nearby land must not be
 // misclassified as sea just because it falls within the same Y band.
 const bool bSeaGap = FMath::Abs(Position.X) < 1300.f
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
  } else if (bLeaveWater || bOnFerry) {
   Movement->SetMovementMode(MOVE_Falling);
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

 // Unload beyond a wider hysteresis radius, preventing churn at tile edges.
 TArray<int32> ExpiredIds;
 for (const TPair<int32, TObjectPtr<AMoonfallOverworldTile>>& Entry : ResidentTiles) {
  const AMoonfallOverworldTile* Tile = Entry.Value.Get();
  if (!IsValid(Tile) || FVector::DistSquared2D(PlayerPosition,
       Tile->GetActorLocation()) > FMath::Square(UnloadRadius))
   ExpiredIds.Add(Entry.Key);
 }
 for (const int32 Id : ExpiredIds) {
  if (TObjectPtr<AMoonfallOverworldTile>* Tile = ResidentTiles.Find(Id))
   if (IsValid(Tile->Get())) Tile->Get()->Destroy();
  ResidentTiles.Remove(Id);
 }

 // Only sort candidate tiles near the hero. The complete world is NEVER
 // instantiated at startup. One update creates at most three tiles.
 TArray<TPair<float, int32>> Candidates;
 for (int32 Region = 0; Region < RegionCount; ++Region) {
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
