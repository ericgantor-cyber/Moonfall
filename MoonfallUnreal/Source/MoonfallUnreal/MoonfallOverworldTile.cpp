#include "MoonfallOverworldTile.h"
#include "MoonfallExpandedWorld.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallOverworldTile::AMoonfallOverworldTile()
{
 PrimaryActorTick.bCanEverTick = false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TileRoot")));
 Terrain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WalkableGround"));
 Terrain->SetupAttachment(GetRootComponent());
 Terrain->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Terrain->SetCollisionResponseToAllChannels(ECR_Block);
 Terrain->SetGenerateOverlapEvents(false);
 Terrain->SetCanEverAffectNavigation(true);

 Scenery = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SceneryInstances"));
 Scenery->SetupAttachment(GetRootComponent());
 Scenery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Scenery->SetGenerateOverlapEvents(false);
 Scenery->SetCanEverAffectNavigation(false);
 Buildings = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SettlementInstances"));
 Buildings->SetupAttachment(GetRootComponent());
 Buildings->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Buildings->SetGenerateOverlapEvents(false);
 Buildings->SetCanEverAffectNavigation(false);

 RegionSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RegionLandmark"));
 RegionSign->SetupAttachment(GetRootComponent());
 RegionSign->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 RegionSign->SetGenerateOverlapEvents(false);
 RegionSign->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 RegionSign->SetWorldSize(275.f);
 RegionSign->SetVisibility(false);

 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 CubeAsset = Cube.Object;
 ConeAsset = Cone.Object;
 CylinderAsset = Cylinder.Object;
 SphereAsset = Sphere.Object;
 Terrain->SetStaticMesh(CubeAsset);
 Buildings->SetStaticMesh(CubeAsset);
 // 200m square with its upper face at the same Z=0 walkable level as the
 // sanctuary and thirty dungeon lanes. Only the floor affects navigation.
 Terrain->SetRelativeLocation(FVector(0.f, 0.f, -60.f));
 Terrain->SetRelativeScale3D(FVector(TileSize / 100.f, TileSize / 100.f, 1.2f));
}

void AMoonfallOverworldTile::BeginPlay()
{
 Super::BeginPlay();
 BuildBlockout();
}

void AMoonfallOverworldTile::BuildBlockout()
{
 if (RegionIndex < 0 || RegionIndex >= AMoonfallExpandedWorld::RegionCount
     || Column < 0 || Column >= TilesPerSide || Row < 0 || Row >= TilesPerSide)
  return;
 // These are deliberate, region-specific PROXY silhouettes. Real meshes,
 // materials, cliffs and foliage can replace them without changing art or
 // breaking the physical region/road layout.
 UStaticMesh* PropShape = CubeAsset;
 switch (RegionIndex) {
 case 0: PropShape = ConeAsset; break;      // Ashen volcanic spires
 case 1: PropShape = ConeAsset; break;      // Moonveil ancient evergreens
 case 2: PropShape = CylinderAsset; break;  // Hollow stone columns
 case 3: PropShape = CylinderAsset; break;  // Foundry smokestacks
 case 4: PropShape = ConeAsset; break;      // Frostbound ice peaks
 case 5: PropShape = SphereAsset; break;    // Eclipse floating monuments
 case 6: PropShape = ConeAsset; break;      // Offshore gothic ruins
 default: break;
 }
 Scenery->SetStaticMesh(PropShape ? PropShape : CubeAsset);
 FRandomStream Random(3907 + RegionIndex * 911 + Column * 131 + Row * 37);
 constexpr int32 PropsPerTile = 18;
 for (int32 Index = 0; Index < PropsPerTile; ++Index) {
  const float X = Random.FRandRange(-8500.f, 8500.f);
  const float Y = Random.FRandRange(-8500.f, 8500.f);
  // Continuous cross-shaped walking lanes pass through EVERY tile.
  // No decorative prop has collision, but keep these routes visually open.
  if (FMath::Abs(X) < 1800.f || FMath::Abs(Y) < 1800.f) continue;
  const float Width = Random.FRandRange(4.f, 9.f);
  float Height = Random.FRandRange(9.f, 19.f);
  if (RegionIndex == 0 || RegionIndex == 4) Height *= 1.7f;
  if (RegionIndex == 3) Height *= 1.9f;
  const float Z = RegionIndex == 5 ? Random.FRandRange(700.f, 1700.f) : Height * 50.f;
  Scenery->AddInstance(FTransform(
    FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f),
    FVector(X, Y, Z), FVector(Width, Width, Height)));
 }
 // Gothic Isle inherits the mainland's 25 streamed, walkable tile
 // foundation but gets its own recognizable outposts and district silhouettes.
 // Everything here is instanced scenery, NOT a claim of functional shops,
 // NPCs, shrines, dungeons or combat encounters.
 if (RegionIndex == 6) {
  // Lightweight inhabited/ruined silhouettes on every tile; keep the
  // cross-shaped walk lanes open for both player and navigation.
  constexpr int32 IslandOutpostProps = 6;
  for (int32 Index = 0; Index < IslandOutpostProps; ++Index) {
   const float SideX = Index % 2 == 0 ? -1.f : 1.f;
   const float SideY = (Index / 2) % 2 == 0 ? -1.f : 1.f;
   const float X = SideX * (4300.f + float(Index % 3) * 1050.f);
   const float Y = SideY * (4400.f + float((Index + 1) % 3) * 1000.f);
   const float Height = 17.f + float((Column * 3 + Row + Index) % 5) * 7.f;
   Buildings->AddInstance(FTransform(
     FRotator(0.f, float((Index * 45 + Column * 11 + Row * 17) % 360), 0.f),
     FVector(X, Y, Height * 50.f),
     FVector(8.f + float(Index % 3) * 2.f, 9.f, Height)));
  }
  // Five district centers: the isle has distinct equivalents of the
  // mainland's landmark hubs, not copies of mainland towns.
  const bool bCathedral = Column == 2 && Row == 2;
  const bool bHarbor = Column == 2 && Row == 0;
  const bool bGraveyard = Column == 4 && Row == 2;
  const bool bWatch = Column == 0 && Row == 2;
  const bool bBlackwood = Column == 2 && Row == 4;
  const TCHAR* Landmark = bCathedral ? TEXT("BLACKBELL CATHEDRAL")
    : bHarbor ? TEXT("RAVEN'S LANDING")
    : bGraveyard ? TEXT("ASHEN GRAVES")
    : bWatch ? TEXT("DUSKWATCH KEEP")
    : bBlackwood ? TEXT("THORNWRAITH GROVE") : nullptr;
  if (Landmark) {
   // Use the same central landmark convention as the mainland.
   // The surrounding instances are unmistakable gothic proxies,
   // to be replaced by original authored island assets later.
   for (int32 Index = 0; Index < 7; ++Index) {
    const float Angle = (2.f * PI * Index) / 7.f;
    const float Distance = 3400.f + float(Index % 2) * 1250.f;
    const float Height = bCathedral ? 55.f : (bWatch ? 42.f : 23.f)
      + float(Index % 3) * 7.f;
    Buildings->AddInstance(FTransform(
      FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f),
      FVector(Distance * FMath::Cos(Angle),
              Distance * FMath::Sin(Angle), Height * 50.f),
      FVector(bHarbor ? 11.f : 13.f, 12.f, Height)));
   }
   RegionSign->SetText(FText::FromString(Landmark));
   RegionSign->SetRelativeLocation(FVector(0.f, 0.f, 3000.f));
   RegionSign->SetVisibility(true);
  }
 }
 // Each province gets a recognizable town/landmark cluster at its center.
 // Island gets ruins rather than pretending it already has full NPC services.
 if (Column == 2 && Row == 2) {
  for (int32 Index = 0; Index < 8; ++Index) {
   const float Angle = (2.f * PI * Index) / 8.f;
   const float Distance = Index % 2 == 0 ? 4700.f : 6700.f;
   const float BuildingHeight = RegionIndex == 3 ? 34.f : 14.f + float(Index % 4) * 4.f;
   Buildings->AddInstance(FTransform(FRotator::ZeroRotator,
     FVector(Distance * FMath::Cos(Angle), Distance * FMath::Sin(Angle),
       BuildingHeight * 50.f),
     FVector(14.f + float(Index % 3) * 4.f, 16.f, BuildingHeight)));
  }
  if (RegionIndex != 6) {
   RegionSign->SetText(FText::FromString(AMoonfallExpandedWorld::RegionName(RegionIndex)));
   RegionSign->SetRelativeLocation(FVector(0.f, 0.f, 2300.f));
   RegionSign->SetVisibility(true);
  }
 }
}
