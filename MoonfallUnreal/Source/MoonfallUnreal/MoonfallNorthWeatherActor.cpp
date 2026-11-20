#include "MoonfallNorthWeatherActor.h"
#include "MoonfallNorthWeatherPreview.h"
#include "MoonfallNorthPrecipitation.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AMoonfallNorthWeatherActor::AMoonfallNorthWeatherActor() {
 PrimaryActorTick.bCanEverTick=false;
 SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("WeatherRoot")));
 WeatherLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("ClimateAccent"));
 WeatherLight->SetupAttachment(GetRootComponent());
 WeatherLight->SetRelativeLocation(FVector(0.f,0.f,350.f));
 WeatherLight->SetAttenuationRadius(650.f);
 WeatherLight->SetIntensity(0.f);
 WeatherLight->SetVisibility(false);
 WeatherLight->SetCastShadows(false);
 WeatherText=CreateDefaultSubobject<UTextRenderComponent>(TEXT("WeatherLabel"));
 WeatherText->SetupAttachment(GetRootComponent());
 WeatherText->SetRelativeLocation(FVector(0.f,0.f,160.f));
 WeatherText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
 WeatherText->SetWorldSize(55.f);
 WeatherText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 WeatherText->SetGenerateOverlapEvents(false);
 WeatherText->SetVisibility(false);
 WeatherParticles=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WeatherParticles"));
 WeatherParticles->SetupAttachment(GetRootComponent());
 WeatherParticles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 WeatherParticles->SetGenerateOverlapEvents(false);
 WeatherParticles->SetCastShadow(false);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if(Cube.Succeeded()) WeatherParticles->SetStaticMesh(Cube.Object);
}
void AMoonfallNorthWeatherActor::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 RefreshWeatherPreview();
}
void AMoonfallNorthWeatherActor::BeginPlay() {
 Super::BeginPlay();
 WeatherStep=0;
 ParticleStep=0;
 bWeatherActive=false;
 DeactivateWeatherPreview();
 // Proximity is checked at 1Hz; the fast 5Hz animation timer is only
 // scheduled when the local player is inside range.
 if(bEnableWeatherPreview && MoonfallNorth::ValidZone(ProvinceIndex)) {
  CheckWeatherProximity();
  GetWorldTimerManager().SetTimer(ProximityTimer,this,
    &AMoonfallNorthWeatherActor::CheckWeatherProximity,1.0f,true);
 }
}
void AMoonfallNorthWeatherActor::EndPlay(const EEndPlayReason::Type Reason) {
 GetWorldTimerManager().ClearTimer(WeatherTimer);
 GetWorldTimerManager().ClearTimer(ProximityTimer);
 DeactivateWeatherPreview();
 Super::EndPlay(Reason);
}
void AMoonfallNorthWeatherActor::DeactivateWeatherPreview() {
 if(IsValid(WeatherLight)) WeatherLight->SetVisibility(false);
 if(IsValid(WeatherText)) WeatherText->SetVisibility(false);
 if(IsValid(WeatherParticles)) WeatherParticles->ClearInstances();
}
void AMoonfallNorthWeatherActor::CheckWeatherProximity() {
 if(!bEnableWeatherPreview || !MoonfallNorth::ValidZone(ProvinceIndex)) {
  if(bWeatherActive) {
   bWeatherActive=false;
   GetWorldTimerManager().ClearTimer(WeatherTimer);
   DeactivateWeatherPreview();
  }
  return;
 }
 const APawn* Player=UGameplayStatics::GetPlayerPawn(this,0);
 const float Radius=FMath::Clamp(WeatherActivationRadiusCm,800.f,8000.f);
 const bool bShouldActivate=IsValid(Player) &&
   FVector::DistSquared(Player->GetActorLocation(),GetActorLocation()) <= Radius*Radius;
 if(bShouldActivate==bWeatherActive) return;
 bWeatherActive=bShouldActivate;
 if(bWeatherActive) {
  RefreshWeatherPreview();
  GetWorldTimerManager().SetTimer(WeatherTimer,this,
    &AMoonfallNorthWeatherActor::AdvanceWeatherPreview,0.2f,true);
 } else {
  GetWorldTimerManager().ClearTimer(WeatherTimer);
  DeactivateWeatherPreview();
 }
}
void AMoonfallNorthWeatherActor::AdvanceWeatherPreview() {
 ParticleStep=(ParticleStep+1)%MoonfallNorth::PrecipitationSteps;
 // Preserve the original two-second light pulse while animating flakes at 5Hz.
 if(ParticleStep%10==0)
  WeatherStep=(WeatherStep+1)%MoonfallNorth::WeatherPreviewSteps;
 RefreshWeatherPreview();
}
void AMoonfallNorthWeatherActor::RefreshWeatherPreview() {
 if(!IsValid(WeatherLight) || !IsValid(WeatherText) || !IsValid(WeatherParticles)) return;
 const auto* Climate=MoonfallNorth::ClimateFor(ProvinceIndex);
 if(!bEnableWeatherPreview || !Climate || (HasActorBegunPlay() && !bWeatherActive)) {
  WeatherLight->SetVisibility(false);
  WeatherText->SetVisibility(false);
  WeatherParticles->ClearInstances();
  return;
 }
 const FColor Color(Climate->Red,Climate->Green,Climate->Blue);
 WeatherLight->SetLightColor(Color);
 WeatherLight->SetIntensity(static_cast<float>(
   MoonfallNorth::PreviewBrightness(ProvinceIndex,WeatherStep)));
 WeatherLight->SetVisibility(true);
 WeatherText->SetText(FText::FromString(UTF8_TO_TCHAR(Climate->Name)));
 WeatherText->SetTextRenderColor(Color);
 WeatherText->SetVisibility(true);
 RefreshWeatherParticles();
}
void AMoonfallNorthWeatherActor::RefreshWeatherParticles() {
 if(!IsValid(WeatherParticles)) return;
 WeatherParticles->ClearInstances();
 if(!bEnableWeatherPreview || !MoonfallNorth::ValidZone(ProvinceIndex)
     || (HasActorBegunPlay() && !bWeatherActive)
     || !WeatherParticles->GetStaticMesh()) return;
 for(int32 Slot=0;Slot<MoonfallNorth::PrecipitationInstanceCount;++Slot) {
  const auto P=MoonfallNorth::PrecipitationAt(ProvinceIndex,Slot,ParticleStep);
  const FVector Center(static_cast<float>(P.Xcm),static_cast<float>(P.Ycm),
    static_cast<float>(P.Zcm));
  const FVector Scale(static_cast<float>(P.HalfWidthCm)/50.f,
    static_cast<float>(P.HalfDepthCm)/50.f,
    static_cast<float>(P.HalfHeightCm)/50.f);
  WeatherParticles->AddInstance(FTransform(FRotator::ZeroRotator,Center,Scale));
 }
}
