#include "MoonfallCharacterPreviewActor.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/SpringArmComponent.h"

AMoonfallCharacterPreviewActor::AMoonfallCharacterPreviewActor()
{
 PrimaryActorTick.bCanEverTick = false;
 StageRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewStageRoot"));
 RootComponent = StageRoot;

 RotationPivot = CreateDefaultSubobject<USceneComponent>(TEXT("RotationPivot"));
 RotationPivot->SetupAttachment(StageRoot);
 CharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("OriginalMoonfallCharacterMesh"));
 CharacterMesh->SetupAttachment(RotationPivot);
 CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 CharacterMesh->SetGenerateOverlapEvents(false);

 CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("PreviewCameraArm"));
 CameraArm->SetupAttachment(StageRoot);
 CameraArm->TargetArmLength = 300.f;
 CameraArm->bDoCollisionTest = false;
 CameraArm->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
 Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CharacterPreviewCapture"));
 Capture->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
 Capture->bCaptureEveryFrame = false;
 Capture->bCaptureOnMovement = false;
}

void AMoonfallCharacterPreviewActor::SetPreviewRenderTarget(
 UTextureRenderTarget2D* InRenderTarget)
{
 PreviewRenderTarget = InRenderTarget;
 Capture->TextureTarget = InRenderTarget;
 RefreshPreview();
}

void AMoonfallCharacterPreviewActor::RefreshPreview()
{
 if (!IsValid(Capture) || !IsValid(PreviewRenderTarget))
  return;
 if (Capture->TextureTarget != PreviewRenderTarget)
  Capture->TextureTarget = PreviewRenderTarget;
 Capture->CaptureScene();
}
