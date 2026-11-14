#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallCharacterPreviewActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

// Isolated 3D mannequin stage for the character creation menu.
// No model or image assets are created or modified by this actor.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallCharacterPreviewActor : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallCharacterPreviewActor();

 UFUNCTION(BlueprintPure, Category="Moonfall|Character Preview")
 USceneComponent* GetPreviewPivot() const { return RotationPivot; }

 UFUNCTION(BlueprintPure, Category="Moonfall|Character Preview")
 USpringArmComponent* GetPreviewArm() const { return CameraArm; }

 UFUNCTION(BlueprintPure, Category="Moonfall|Character Preview")
 UTextureRenderTarget2D* GetPreviewRenderTarget() const { return PreviewRenderTarget; }

 UFUNCTION(BlueprintCallable, Category="Moonfall|Character Preview")
 void SetPreviewRenderTarget(UTextureRenderTarget2D* InRenderTarget);

 UFUNCTION(BlueprintCallable, Category="Moonfall|Character Preview")
 void RefreshPreview();

protected:
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Character Preview")
 TObjectPtr<USceneComponent> StageRoot;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Character Preview")
 TObjectPtr<USceneComponent> RotationPivot;

 // Attach Moonfall-approved original character meshes or Blueprint components.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Character Preview")
 TObjectPtr<USkeletalMeshComponent> CharacterMesh;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Character Preview")
 TObjectPtr<USpringArmComponent> CameraArm;

 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Character Preview")
 TObjectPtr<USceneCaptureComponent2D> Capture;

 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Moonfall|Character Preview")
 TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;
};
