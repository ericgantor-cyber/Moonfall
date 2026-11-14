#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MoonfallRaceLibrary.h"
#include "MoonfallCharacterCreationWidget.generated.h"

class UHorizontalBox;
class USceneComponent;
class USpringArmComponent;
class UImage;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

// A local, original Moonfall race tile. Its click event carries its race ID.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoonfallRaceChoiceActivated, FName, RaceId);

UCLASS()
class MOONFALLUNREAL_API UMoonfallRaceChoiceButton : public UButton
{
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintAssignable, Category="Moonfall|Character Creation")
 FMoonfallRaceChoiceActivated OnRaceActivated;

 void InitializeChoice(FName InRaceId);

private:
 UPROPERTY(Transient) FName ChoiceRaceId = NAME_None;
 UFUNCTION() void HandleChoiceClicked();
};

// World-of-Warcraft-inspired layout, using original Moonfall text and data:
// race list at left, prominent central preview, details and stats at right.
// The game supplies the optional preview texture; this class never replaces
// existing Z/Nyra images or introduces Warcraft assets or factions.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoonfallRaceSelectionConfirmed, FName, RaceId);

UCLASS(Blueprintable)
class MOONFALLUNREAL_API UMoonfallCharacterCreationWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 // The owning Blueprint can connect this to its existing hero creation flow.
 // Confirmation here never writes a save or spawns an actor.
 UPROPERTY(BlueprintAssignable, Category="Moonfall|Character Creation")
 FMoonfallRaceSelectionConfirmed OnRaceConfirmed;

 // Request the existing character image/model for this race. Owning game
 // decides whether to provide an original Moonfall portrait or preview.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Character Creation")
 void OnRacePreviewRequested(FName RaceId);

 // The owning Blueprint rotates/zooms its existing 3D preview actor or
 // SceneCapture. These events do not transform original sprites or art.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Character Creation|Mouse")
 void OnPreviewRotateRequested(float DeltaYaw);

 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Character Creation|Mouse")
 void OnPreviewZoomRequested(float ZoomSteps);

 UFUNCTION(BlueprintCallable, Category="Moonfall|Character Creation")
 bool InitializeSelection(FName PreferredRaceId, float InBaseVitality,
                          float InBasePower, float InBaseSpeed);

 UFUNCTION(BlueprintCallable, Category="Moonfall|Character Creation")
 void SetPreviewTexture(UTexture2D* ExistingMoonfallTexture);

 // Bind dedicated PREVIEW-ONLY scene components, never the live player hero.
 // When bound, dragging rotates the pivot and scrolling adjusts camera arm
 // distance directly; otherwise the existing Blueprint callbacks are used.
 UFUNCTION(BlueprintCallable, Category="Moonfall|Character Creation|Mouse")
 void BindPreviewRig(USceneComponent* PreviewPivot, USpringArmComponent* PreviewArm);

 UFUNCTION(BlueprintCallable, Category="Moonfall|Character Creation|Mouse")
 void ResetPreviewView();

 UFUNCTION(BlueprintPure, Category="Moonfall|Character Creation")
 FMoonfallRaceSelectionState GetSelectionState() const { return Selection; }

protected:
 virtual void NativeConstruct() override;
 virtual FReply NativeOnKeyDown(const FGeometry& InGeometry,
                                const FKeyEvent& InKeyEvent) override;
 virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                        const FPointerEvent& InMouseEvent) override;
 virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry,
                                      const FPointerEvent& InMouseEvent) override;
 virtual FReply NativeOnMouseMove(const FGeometry& InGeometry,
                                  const FPointerEvent& InMouseEvent) override;
 virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry,
                                   const FPointerEvent& InMouseEvent) override;

private:
 void BuildLayout();
 void RefreshSelection();
 void RefreshRaceList();
 void AdvanceRace(bool bForward);
 void AdvanceFamily(bool bForward);
 void SelectRace(FName RaceId);
 bool IsOverPreview(const FVector2D& ScreenPosition) const;
 void RotatePreview(float DeltaYaw);
 void ZoomPreview(float ZoomSteps);

 UFUNCTION() void HandlePreviousRace();
 UFUNCTION() void HandleNextRace();
 UFUNCTION() void HandlePreviousFamily();
 UFUNCTION() void HandleNextFamily();
 UFUNCTION() void HandleRaceChoice(FName RaceId);
 UFUNCTION() void HandleConfirmRace();
 UFUNCTION() void HandleResetPreview();

 UPROPERTY(Transient) TObjectPtr<UScrollBox> RaceList;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> RaceTitle;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> FamilyTitle;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> StatDetails;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> PreviewCaption;
 UPROPERTY(Transient) TObjectPtr<UImage> PreviewImage;
 UPROPERTY(Transient) TObjectPtr<UTexture2D> PreviewTexture;
 bool bDraggingPreview = false;
 TWeakObjectPtr<USceneComponent> PreviewRotationPivot;
 TWeakObjectPtr<USpringArmComponent> PreviewZoomArm;
 FRotator InitialPreviewRotation = FRotator::ZeroRotator;
 float InitialPreviewArmLength = 300.f;
 UPROPERTY(Transient) FMoonfallRaceSelectionState Selection;
 UPROPERTY(Transient) FName CurrentFamily = NAME_None;
 UPROPERTY(Transient) FName CurrentRaceId = NAME_None;
 UPROPERTY(Transient) float BaseVitality = 100.f;
 UPROPERTY(Transient) float BasePower = 10.f;
 UPROPERTY(Transient) float BaseSpeed = 1.f;
};
