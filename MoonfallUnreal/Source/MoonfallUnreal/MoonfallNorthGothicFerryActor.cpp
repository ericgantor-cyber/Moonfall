#include "MoonfallNorthGothicFerryActor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace {
// Unreal engine cubes are 100cm across. All parts are in LOCAL cm, so the
// entire vessel can be oriented or repositioned safely by a level designer.
void AddBox(UInstancedStaticMeshComponent* Group, const FVector& Location,
            const FVector& Dimensions, const FRotator& Rotation = FRotator::ZeroRotator) {
 if (!IsValid(Group) || !Group->GetStaticMesh()) return;
 Group->AddInstance(FTransform(Rotation, Location, Dimensions / 100.f));
}
void Tint(UInstancedStaticMeshComponent* Group, const FLinearColor& Color) {
 if (!IsValid(Group) || !Group->GetStaticMesh()) return;
 if (UMaterialInstanceDynamic* MID = Group->CreateAndSetMaterialInstanceDynamic(0)) {
  // Engine BasicShapeMaterial accepts Color where available; approved authored
  // material instances can replace this optional editor-preview tint later.
  MID->SetVectorParameterValue(TEXT("Color"), Color);
 }
}
}

AMoonfallNorthGothicFerryActor::AMoonfallNorthGothicFerryActor() {
 PrimaryActorTick.bCanEverTick = false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("GothicFerryRoot")));
 // Explicitly use this actor's onboard camera when the controller follows
 // the non-steerable ferry. The preview deck meshes remain nonblocking.
 bFindCameraComponentWhenViewTarget = true;
 VoyageCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("GothicRavenVoyageCamera"));
 VoyageCamera->SetupAttachment(GetRootComponent());
 // Camera is behind the hull and looks toward the raven figurehead (+X).
 VoyageCamera->SetRelativeLocation(FVector(-1750.f, 0.f, 795.f));
 VoyageCamera->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
 VoyageCamera->SetFieldOfView(80.f);
 VoyageCamera->bUsePawnControlRotation = false;
 VoyageCamera->bAutoActivate = true;
 auto MakePieces = [this](const FName& Name) -> UInstancedStaticMeshComponent* {
  UInstancedStaticMeshComponent* Pieces = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
  Pieces->SetupAttachment(GetRootComponent());
  Pieces->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Pieces->SetGenerateOverlapEvents(false);
  Pieces->SetCastShadow(true);
  return Pieces;
 };
 HullPieces = MakePieces(TEXT("BlackHullAndDeck"));
 GothicIronwork = MakePieces(TEXT("GothicIronwork"));
 CrimsonSails = MakePieces(TEXT("TatteredCrimsonSails"));
 RavenFigurehead = MakePieces(TEXT("CarvedRavenFigurehead"));
 CrimsonLanterns = MakePieces(TEXT("CrimsonLanterns"));

 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if (Cube.Succeeded()) {
  for (UInstancedStaticMeshComponent* Parts :
       {HullPieces.Get(), GothicIronwork.Get(), CrimsonSails.Get(),
        RavenFigurehead.Get(), CrimsonLanterns.Get()}) {
   if (IsValid(Parts)) Parts->SetStaticMesh(Cube.Object);
  }
 }
 // Optional built-in colorable material. Materials/art can be replaced without
 // changing the ferry's boarding trigger or its isolated save protections.
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewMaterial(
     TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 if (PreviewMaterial.Succeeded()) {
  for (UInstancedStaticMeshComponent* Parts :
       {HullPieces.Get(), GothicIronwork.Get(), CrimsonSails.Get(),
        RavenFigurehead.Get(), CrimsonLanterns.Get()}) {
   if (IsValid(Parts)) Parts->SetMaterial(0, PreviewMaterial.Object);
  }
 }

 PortLanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortCrimsonLantern"));
 PortLanternLight->SetupAttachment(GetRootComponent());
 PortLanternLight->SetRelativeLocation(FVector(0.f,-225.f,465.f));
 PortLanternLight->SetLightColor(FLinearColor(0.95f,0.035f,0.055f));
 PortLanternLight->SetIntensity(210.f);
 PortLanternLight->SetAttenuationRadius(500.f);
 PortLanternLight->SetCastShadows(false);

 StarboardLanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StarboardCrimsonLantern"));
 StarboardLanternLight->SetupAttachment(GetRootComponent());
 StarboardLanternLight->SetRelativeLocation(FVector(0.f,225.f,465.f));
 StarboardLanternLight->SetLightColor(FLinearColor(0.95f,0.035f,0.055f));
 StarboardLanternLight->SetIntensity(210.f);
 StarboardLanternLight->SetAttenuationRadius(500.f);
 StarboardLanternLight->SetCastShadows(false);
}

void AMoonfallNorthGothicFerryActor::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 for (UInstancedStaticMeshComponent* Group :
      {HullPieces.Get(), GothicIronwork.Get(), CrimsonSails.Get(),
       RavenFigurehead.Get(), CrimsonLanterns.Get()}) {
  if (IsValid(Group)) Group->ClearInstances();
 }
 if (PortLanternLight) PortLanternLight->SetVisibility(bShowGothicFerry);
 if (StarboardLanternLight) StarboardLanternLight->SetVisibility(bShowGothicFerry);
 if (!bShowGothicFerry || !IsValid(HullPieces) || !HullPieces->GetStaticMesh()) return;

 // 19m long: tapered layered hull, broad walkable-looking upper deck, stern
 // cabin and a raised bow. All decorative geometry is nonblocking for QA.
 AddBox(HullPieces, FVector(0,0,170), FVector(1770,560,175));
 AddBox(HullPieces, FVector(30,0,252), FVector(1650,650,45));
 AddBox(HullPieces, FVector(-100,0,104), FVector(1420,315,80));
 AddBox(HullPieces, FVector(860,0,205), FVector(355,355,145), FRotator(0,0,0));
 AddBox(HullPieces, FVector(-830,0,265), FVector(270,465,160));
 // Outer hull ribs/black gunwales.
 for (int Side : {-1,1}) {
  AddBox(HullPieces, FVector(-5,Side*307.f,315), FVector(1775,38,120));
  for (int i=0;i<9;++i) {
   const float X = -710.f + 175.f * i;
   AddBox(HullPieces, FVector(X,Side*325.f,252), FVector(31,44,215));
  }
 }

 // Cathedral pavilion, flying roof ridges, needle spires, and 12 rail posts.
 AddBox(HullPieces, FVector(-410,0,420), FVector(660,445,45));
 for (int Side : {-1,1}) {
  for (int i=0;i<4;++i) {
   const float X=-660.f+170.f*i;
   AddBox(GothicIronwork, FVector(X,Side*196.f,510), FVector(30,30,260));
   AddBox(GothicIronwork, FVector(X,Side*196.f,675), FVector(23,23,120));
  }
  AddBox(GothicIronwork, FVector(-410,Side*220.f,650), FVector(700,26,65));
  // Gabled halves imply steep pointed-arch gothic roofline.
  AddBox(HullPieces, FVector(-410,Side*116.f,721),
         FVector(725,260,34), FRotator(Side*-38.f,0,0));
  for (int i=0;i<6;++i) {
   const float X=-820.f+325.f*i;
   AddBox(GothicIronwork, FVector(X,Side*313.f,415), FVector(24,24,180));
   AddBox(GothicIronwork, FVector(X,Side*313.f,521), FVector(18,18,110));
  }
 }
 for (float X : {-735.f,-410.f,-85.f}) {
  AddBox(GothicIronwork, FVector(X,0,835), FVector(23,23,220));
 }
 // Main mast + six crimson torn sail panels. Static and bounded, no physics.
 AddBox(GothicIronwork, FVector(275,0,785), FVector(43,43,1030));
 AddBox(GothicIronwork, FVector(270,0,1190), FVector(32,700,30));
 AddBox(GothicIronwork, FVector(280,0,945), FVector(28,580,25));
 for (int i=0;i<5;++i) {
  const float Y = -200.f + i*102.f;
  const float Height= (i % 2 == 0) ? 450.f : 375.f;
  AddBox(CrimsonSails, FVector(250,Y,950), FVector(24,87,Height));
 }
 AddBox(CrimsonSails, FVector(-565,0,805), FVector(32,135,265));

 // Raven at the forward prow: long beak, raised neck, carved wings,
 // and two bright red eyes. No collision or damage hitbox.
 AddBox(RavenFigurehead, FVector(930,0,460), FVector(105,135,330), FRotator(0,0,18));
 AddBox(RavenFigurehead, FVector(984,0,660), FVector(140,158,145));
 AddBox(RavenFigurehead, FVector(1095,0,623), FVector(160,68,40), FRotator(0,-6,0));
 for (int Side : {-1,1}) {
  AddBox(RavenFigurehead, FVector(942,Side*112.f,550),
         FVector(200,42,185), FRotator(0,Side*33.f,Side*14.f));
 }
 // Hanging crimson lanterns, one pair by pavilion and another at the bow.
 for (float X : {-360.f,685.f}) {
  for (int Side : {-1,1}) {
   AddBox(GothicIronwork, FVector(X,Side*322.f,565), FVector(22,22,270));
   AddBox(CrimsonLanterns, FVector(X,Side*322.f,560), FVector(70,70,90));
   AddBox(GothicIronwork, FVector(X,Side*322.f,615), FVector(78,78,13));
   AddBox(GothicIronwork, FVector(X,Side*322.f,505), FVector(78,78,13));
  }
 }
 for (int Side : {-1,1}) {
  AddBox(CrimsonLanterns, FVector(995,Side*45.f,667), FVector(28,25,22));
 }

 Tint(HullPieces, FLinearColor(0.037f,0.040f,0.052f,1.f));
 Tint(GothicIronwork, FLinearColor(0.105f,0.105f,0.12f,1.f));
 Tint(CrimsonSails, FLinearColor(0.26f,0.015f,0.035f,1.f));
 Tint(RavenFigurehead, FLinearColor(0.020f,0.021f,0.035f,1.f));
 Tint(CrimsonLanterns, FLinearColor(1.f,0.025f,0.045f,1.f));
}
