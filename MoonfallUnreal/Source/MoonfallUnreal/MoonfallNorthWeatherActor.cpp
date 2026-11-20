#include "MoonfallNorthWeatherActor.h"
#include "MoonfallNorthWeatherPreview.h"
#include "Components/SceneComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "TimerManager.h"

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
}
void AMoonfallNorthWeatherActor::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);
 RefreshWeatherPreview();
}
void AMoonfallNorthWeatherActor::BeginPlay() {
 Super::BeginPlay();
 WeatherStep=0;
 RefreshWeatherPreview();
 if(bEnableWeatherPreview && MoonfallNorth::ValidZone(ProvinceIndex)) {
  GetWorldTimerManager().SetTimer(WeatherTimer,this,
    &AMoonfallNorthWeatherActor::AdvanceWeatherPreview,2.0f,true);
 }
}
void AMoonfallNorthWeatherActor::EndPlay(const EEndPlayReason::Type Reason) {
 GetWorldTimerManager().ClearTimer(WeatherTimer);
 Super::EndPlay(Reason);
}
void AMoonfallNorthWeatherActor::AdvanceWeatherPreview() {
 WeatherStep=(WeatherStep+1)%MoonfallNorth::WeatherPreviewSteps;
 RefreshWeatherPreview();
}
void AMoonfallNorthWeatherActor::RefreshWeatherPreview() {
 if(!IsValid(WeatherLight) || !IsValid(WeatherText)) return;
 const auto* Climate=MoonfallNorth::ClimateFor(ProvinceIndex);
 if(!bEnableWeatherPreview || !Climate) {
  WeatherLight->SetVisibility(false);
  WeatherText->SetVisibility(false);
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
}
