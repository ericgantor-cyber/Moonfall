#include "MoonfallArena.h"
#include "MoonfallEnemy.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallCheckpointShrine.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "MoonfallSanctuaryArchitecture.h"
#include "MoonfallLivingSimulation.h"
#include "MoonfallRoadLandmarks.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
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
 if (FPackageName::DoesPackageExist(TEXT("/Game/MoonfallProduction/Meshes/SM_MF_GothicColumn"))) {
  static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Column(
     TEXT("/Game/MoonfallProduction/Meshes/SM_MF_GothicColumn.SM_MF_GothicColumn"));
  ProductionColumn=Column.Get();
 }
 if (FPackageName::DoesPackageExist(TEXT("/Game/MoonfallProduction/Meshes/SM_MF_FirstlightShield"))) {
  static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Shield(
     TEXT("/Game/MoonfallProduction/Meshes/SM_MF_FirstlightShield.SM_MF_FirstlightShield"));
  ProductionShield=Shield.Get();
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
void AMoonfallArena::CreateSanctuaryAtmosphere() {
 if (!bEnableStartingSanctuary || !bEnableSanctuaryAtmosphere || !DungeonRoad) return;
 UStaticMesh* StoneMesh = DungeonRoad->GetStaticMesh();
 if (!StoneMesh) return;
 // Both catalogs are fixed C++17 layouts and share the existing cube mesh.
 // Batch all transforms before registration: two instancing components
 // instead of dozens of ticking scene actors or per-block draw components.
 using MoonfallSanctuaryArchitecture::FPiece;
 // Substitute only the tall sanctuary pillars. The physical arena, road,
 // relics and horizontal arch spans retain their original geometry.
 UInstancedStaticMeshComponent* ImportedColumns=nullptr;
 FBox ColumnBounds(ForceInit);
 if (IsValid(ProductionColumn)) {
  ColumnBounds=ProductionColumn->GetBoundingBox();
  const FVector Size=ColumnBounds.GetSize();
  if (ColumnBounds.IsValid && !ColumnBounds.Min.ContainsNaN() && !ColumnBounds.Max.ContainsNaN()
      && !Size.ContainsNaN() && Size.GetMin()>0.1f && Size.GetMax()<100000.f) {
   ImportedColumns=NewObject<UInstancedStaticMeshComponent>(this,TEXT("SanctuaryImportedColumns"));
   if (IsValid(ImportedColumns)) {
    ImportedColumns->SetupAttachment(GetRootComponent());
    ImportedColumns->SetStaticMesh(ProductionColumn);
    ImportedColumns->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ImportedColumns->SetCollisionResponseToAllChannels(ECR_Ignore);
    ImportedColumns->SetGenerateOverlapEvents(false);
    ImportedColumns->SetCastShadow(false);
    ImportedColumns->SetCanEverAffectNavigation(false);
    ImportedColumns->SetCullDistances(14000,22000);
   }
  }
 }
 const auto BuildDetails = [&](const TCHAR* Name, const FPiece* Pieces,
                               int32 Count) -> UInstancedStaticMeshComponent* {
  UInstancedStaticMeshComponent* Details =
      NewObject<UInstancedStaticMeshComponent>(this, FName(Name));
  if (!IsValid(Details)) return nullptr;
  Details->SetupAttachment(GetRootComponent());
  Details->SetStaticMesh(StoneMesh);
  Details->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Details->SetCollisionResponseToAllChannels(ECR_Ignore);
  Details->SetGenerateOverlapEvents(false);
  Details->SetCastShadow(false);
  Details->SetCullDistances(14000, 22000);
  for (int32 Index = 0; Index < Count; ++Index) {
   const FPiece& Piece = Pieces[Index];
   if (IsValid(ImportedColumns) && Pieces==MoonfallSanctuaryArchitecture::GothicStone
       && Piece.ScaleZ>=4.9f && Piece.ScaleX<=1.f && Piece.ScaleY<=1.f
       && Piece.YawDegrees==0.f) {
    const FVector Size=ColumnBounds.GetSize();
    const FVector Scale(100.f*Piece.ScaleX/Size.X,100.f*Piece.ScaleY/Size.Y,100.f*Piece.ScaleZ/Size.Z);
    const FVector Location=FVector(Piece.X,Piece.Y,Piece.Z)-ColumnBounds.GetCenter()*Scale;
    if (ImportedColumns->AddInstance(FTransform(FRotator::ZeroRotator,Location,Scale))!=INDEX_NONE)
     continue;
   }
   Details->AddInstance(FTransform(
       FRotator(0.f, Piece.YawDegrees, 0.f),
       FVector(Piece.X, Piece.Y, Piece.Z),
       FVector(Piece.ScaleX, Piece.ScaleY, Piece.ScaleZ)));
  }
  AddInstanceComponent(Details);
  Details->RegisterComponent();
  return Details;
 };
 SanctuaryGothicDetails = BuildDetails(TEXT("SanctuaryGothicDetails"),
     MoonfallSanctuaryArchitecture::GothicStone,
     static_cast<int32>(MoonfallSanctuaryArchitecture::GothicCount));
 if (IsValid(ImportedColumns)) {
  AddInstanceComponent(ImportedColumns);
  ImportedColumns->RegisterComponent();
 }
 SanctuaryMoonstoneDetails = BuildDetails(TEXT("SanctuaryMoonstoneDetails"),
     MoonfallSanctuaryArchitecture::MoonstoneRelics,
     static_cast<int32>(MoonfallSanctuaryArchitecture::RelicCount));

 // Imported heraldry is display-only, mounted on the existing outer pillars.
 // No shield equipment, guard mechanics, collision or navigation changes.
 if (IsValid(ProductionShield)) {
  const FBox Bounds=ProductionShield->GetBoundingBox();
  const FVector Size=Bounds.GetSize();
  const double Span=Size.GetMax();
  if (Bounds.IsValid && !Bounds.Min.ContainsNaN() && !Bounds.Max.ContainsNaN()
      && !Size.ContainsNaN() && Size.GetMin()>0.1f && Span<100000.f) {
   UInstancedStaticMeshComponent* Shields=NewObject<UInstancedStaticMeshComponent>(this,TEXT("SanctuaryShieldHeraldry"));
   if (IsValid(Shields)) {
    Shields->SetupAttachment(GetRootComponent());
    Shields->SetStaticMesh(ProductionShield);
    Shields->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Shields->SetCollisionResponseToAllChannels(ECR_Ignore);
    Shields->SetGenerateOverlapEvents(false);
    Shields->SetCastShadow(false);
    Shields->SetCanEverAffectNavigation(false);
    Shields->SetCullDistances(8000,12000);
    const float Scale=60.f/static_cast<float>(Span);
    const FRotator Rotation(90.f,0.f,0.f);
    const FVector Offset=Rotation.RotateVector(Bounds.GetCenter()*Scale);
    for (float X : {-760.f,760.f})
     for (float Y : {2670.f,3170.f,3870.f,4370.f})
      Shields->AddInstance(FTransform(Rotation,FVector(X+50.f,Y,210.f)-Offset,FVector(Scale)));
    AddInstanceComponent(Shields);
    Shields->RegisterComponent();
   }
  }
 }
 // Four soft moon-blue beacons. No shadow casters, extra ticking or FX systems.
 // Lights and all stonework inherit the arena transform if a map is rotated.
 const FVector Lanterns[] = {
  FVector(-465.f, 3480.f, 260.f), FVector(465.f, 3480.f, 260.f),
  FVector(-760.f, 4160.f, 310.f), FVector(760.f, 4160.f, 310.f)
 };
 for (const FVector& Position : Lanterns) {
  UPointLightComponent* MoonLight = NewObject<UPointLightComponent>(this);
  if (!IsValid(MoonLight)) continue;
  MoonLight->SetupAttachment(GetRootComponent());
  MoonLight->SetRelativeLocation(Position);
  MoonLight->SetLightColor(FLinearColor(0.32f, 0.56f, 0.92f));
  MoonLight->SetIntensity(550.f);
  MoonLight->SetAttenuationRadius(660.f);
  MoonLight->SetCastShadows(false);
  AddInstanceComponent(MoonLight);
  MoonLight->RegisterComponent();
  SanctuaryMoonLights.Add(MoonLight);
 }

 // Optional lore is readable in the world, never a forced blocking tutorial.
 struct FStoryPlaque { const TCHAR* Id; const TCHAR* Caption; FVector At; };
 const FStoryPlaque Plaques[] = {
  {TEXT("FirstlightOath"), TEXT("THE FIRSTLIGHT OATH\nTHE SIX ROADS HOLD DIFFERENT TRUTHS"),
      FVector(560.f, 3850.f, 180.f)},
  {TEXT("FirstlightWarden"), TEXT("A BROKEN GATE, AN OPEN ROAD\nFOLLOW THE SOUTHERN PASS"),
      FVector(-560.f, 2940.f, 165.f)},
  {TEXT("FirstlightMoon"), TEXT("THE MOON IS SHATTERED\nLET THE WITNESSES SPEAK"),
      FVector(570.f, 4430.f, 165.f)}
 };
 for (const FStoryPlaque& Plaque : Plaques) {
  UTextRenderComponent* LoreMarker =
      NewObject<UTextRenderComponent>(this, FName(Plaque.Id));
  if (!IsValid(LoreMarker)) continue;
  LoreMarker->SetupAttachment(GetRootComponent());
  LoreMarker->SetText(FText::FromString(Plaque.Caption));
  LoreMarker->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
  LoreMarker->SetWorldSize(29.f);
  LoreMarker->SetTextRenderColor(FColor(172, 215, 246));
  LoreMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  LoreMarker->SetGenerateOverlapEvents(false);
  LoreMarker->SetRelativeLocation(Plaque.At);
  LoreMarker->RegisterComponent();
  SanctuaryGuideSigns.Add(LoreMarker);
 }
}

void AMoonfallArena::CreateRegionalLandmarks() {
 if (!bEnableRegionalLandmarks || !IsValid(DungeonRoad)) return;
 UStaticMesh* Cube=DungeonRoad->GetStaticMesh();
 if (!Cube) return;
 // One mesh batch for six region-specific skylines. Every piece is beyond
 // the road's walkable Y strip, so combat lines and all 30 entrances remain.
 UInstancedStaticMeshComponent* Details=
   NewObject<UInstancedStaticMeshComponent>(this,TEXT("RegionLandmarkDetails"));
 if (!IsValid(Details)) return;
 Details->SetupAttachment(GetRootComponent());
 Details->SetStaticMesh(Cube);
 Details->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Details->SetCollisionResponseToAllChannels(ECR_Ignore);
 Details->SetGenerateOverlapEvents(false);
 Details->SetCastShadow(false);
 Details->SetCullDistances(18000,28000);
 for (int32 Zone=0;Zone<6;++Zone) {
  const float CenterX=(static_cast<float>(Zone*5+2)-14.5f)*1350.f;
  // Forest spires, foundry stacks, frost monoliths, fractured ash towers:
  // authored silhouettes, not procedural collision terrain.
  const float Height=Zone==1?5.2f:(Zone==3?3.6f:(Zone==4?4.8f:2.7f));
  const float Width=Zone==2?2.7f:(Zone==5?1.1f:1.5f);
  // Every instance sits in an authored side gap between dungeon lanes.
  // Compile-time guards forbid future offsets from entering any chamber.
  for (const MoonfallRoadLandmarks::FPlacement& Spec:
       MoonfallRoadLandmarks::Placements) {
   const float H=FMath::Clamp(Height*Spec.Height,1.f,7.f);
   Details->AddInstance(FTransform(FRotator::ZeroRotator,
      FVector(CenterX+Spec.X,Spec.Y,H*50.f),
      FVector(Width,Width,H)));
   // Two sculpted crown tiers per landmark; no new actors, lights or mesh batch.
   const auto& Art=MoonfallRoadLandmarks::ArtProfiles[Zone];
   const float Top=H*100.f;
   Details->AddInstance(FTransform(FRotator(0.f,Art.CrownYaw,0.f),
      FVector(CenterX+Spec.X,Spec.Y,Top+Art.CrownHeight*50.f),
      FVector(Art.CrownWidth,Art.CrownWidth,Art.CrownHeight)));
   Details->AddInstance(FTransform(FRotator::ZeroRotator,
      FVector(CenterX+Spec.X,Spec.Y,Top+Art.CrownHeight*100.f+Art.TipHeight*50.f),
      FVector(Art.TipWidth,Art.TipWidth,Art.TipHeight)));
  }
 }
 AddInstanceComponent(Details);
 Details->RegisterComponent();
 RegionLandmarkDetails=Details;
 // Six bounded, shadow-free weather lights (one per connected region).
 for (int32 Zone=0;Zone<6;++Zone) {
  UPointLightComponent* Light=NewObject<UPointLightComponent>(this);
  if (!IsValid(Light)) continue;
  Light->SetupAttachment(GetRootComponent());
  Light->SetRelativeLocation(FVector(
      (static_cast<float>(Zone*5+2)-14.5f)*1350.f,-2500.f,420.f));
  Light->SetAttenuationRadius(3000.f);
  Light->SetCastShadows(false);
  Light->SetIntensity(300.f);
  AddInstanceComponent(Light);
  Light->RegisterComponent();
  RegionWeatherLights.Add(Light);
 }
 UpdateRegionWeather(0.f);
}
void AMoonfallArena::UpdateRegionWeather(float WorldTime) {
 if (!FMath::IsFinite(WorldTime) || WorldTime<0.f) return;
 const int32 Phase=MoonfallLivingSimulation::Phase(WorldTime,360.f);
 if (LastRegionWeatherPhase==Phase) return;
 LastRegionWeatherPhase=Phase;
 for (int32 Zone=0;Zone<RegionWeatherLights.Num() && Zone<6;++Zone) {
  UPointLightComponent* Light=RegionWeatherLights[Zone];
  if (!IsValid(Light)) continue;
  const auto Weather=MoonfallLivingSimulation::WeatherFor(Zone,Phase);
  FLinearColor Tint(0.66f,0.78f,0.89f);
  float Intensity=300.f;
  switch (Weather) {
   case MoonfallLivingSimulation::EWeather::Ash:
    Tint=FLinearColor(0.94f,0.36f,0.18f);Intensity=420.f;break;
   case MoonfallLivingSimulation::EWeather::Storm:
    Tint=FLinearColor(0.33f,0.43f,0.78f);Intensity=260.f;break;
   case MoonfallLivingSimulation::EWeather::Snow:
    Tint=FLinearColor(0.63f,0.85f,1.f);Intensity=440.f;break;
   case MoonfallLivingSimulation::EWeather::Fog:
    Tint=FLinearColor(0.58f,0.72f,0.78f);Intensity=210.f;break;
   case MoonfallLivingSimulation::EWeather::Rain:
    Tint=FLinearColor(0.42f,0.65f,0.93f);Intensity=310.f;break;
   case MoonfallLivingSimulation::EWeather::Clear:
    Tint=FLinearColor(0.9f,0.78f,0.54f);Intensity=320.f;break;
  }
  Light->SetLightColor(Tint);
  Light->SetIntensity(Intensity);
 }
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
  CreateSanctuaryAtmosphere();
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
 // Bounded silhouette landmarks and nonshadow weather lights, outside the road.
 CreateRegionalLandmarks();
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
 // Two stable, no-Tick sanctuary residents add role-based conversations
 // and use the existing resident contract/bond system without new save fields.
 if (bEnableStartingSanctuary) {
  struct FSanctuaryResident {
   const TCHAR* Id; const TCHAR* Name; const TCHAR* Role;
   const TCHAR* Faction; const TCHAR* Greeting; int32 Race; FVector Position;
  };
  const FSanctuaryResident People[] = {
   {TEXT("FirstlightArchivist"), TEXT("Archivist Veyra"), TEXT("Scholar"),
    TEXT("MoonCoven"), TEXT("Every road remembers a different fragment of the broken oath."),
    1, FVector(510.f, 4040.f, 60.f)},
   {TEXT("FirstlightWatchkeeper"), TEXT("Warden Cael"), TEXT("Guard"),
    TEXT("The northern sentries are for practice. The southern gate leads to the Crossroads."),
    0, FVector(-515.f, 3220.f, 60.f)}
  };
  for (const FSanctuaryResident& Entry : People) {
   const FVector WorldPosition = GetActorTransform().TransformPosition(Entry.Position);
   if (AMoonfallWorldNPC* Person = GetWorld()->SpawnActor<AMoonfallWorldNPC>(
       AMoonfallWorldNPC::StaticClass(), WorldPosition, GetActorRotation())) {
    Person->ResidentId = FName(Entry.Id);
    Person->DisplayName = FText::FromString(Entry.Name);
    Person->SocialRole = FName(Entry.Role);
    Person->FactionId = FName(Entry.Faction);
    Person->SettlementId = FName(TEXT("Firstlight"));
    Person->RaceId = Entry.Race;
    Person->Greeting = FText::FromString(Entry.Greeting);
   }
  }
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
