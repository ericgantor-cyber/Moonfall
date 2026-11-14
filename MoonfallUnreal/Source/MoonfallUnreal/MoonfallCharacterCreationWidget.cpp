#include "MoonfallCharacterCreationWidget.h"
#include "MoonfallCharacterPreviewActor.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"

namespace
{
 UTextBlock* AddLabel(UWidgetTree* Tree, UVerticalBox* Panel,
                      const FString& Text, int32 FontSize = 18)
 {
  UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
  Label->SetText(FText::FromString(Text));
  FSlateFontInfo Font = Label->GetFont();
  Font.Size = FontSize;
  Label->SetFont(Font);
  Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.78f, 0.50f, 1.f)));
  Panel->AddChildToVerticalBox(Label);
  return Label;
 }

 UButton* AddAction(UWidgetTree* Tree, UVerticalBox* Panel, const FString& Name)
 {
  UButton* Button = Tree->ConstructWidget<UButton>();
  UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
  Text->SetText(FText::FromString(Name));
  Button->SetBackgroundColor(FLinearColor(0.23f, 0.19f, 0.12f, 1.f));
  Button->AddChild(Text);
  Panel->AddChildToVerticalBox(Button);
  return Button;
 }
}

void UMoonfallRaceChoiceButton::InitializeChoice(FName InRaceId)
{
 ChoiceRaceId = InRaceId;
 OnClicked.RemoveDynamic(this, &UMoonfallRaceChoiceButton::HandleChoiceClicked);
 OnClicked.AddDynamic(this, &UMoonfallRaceChoiceButton::HandleChoiceClicked);
}

void UMoonfallRaceChoiceButton::HandleChoiceClicked()
{
 OnRaceActivated.Broadcast(ChoiceRaceId);
}

void UMoonfallCharacterCreationWidget::NativeConstruct()
{
 Super::NativeConstruct();
 SetIsFocusable(true);
 if (WidgetTree && !WidgetTree->RootWidget)
  BuildLayout();
 RefreshSelection();
}

void UMoonfallCharacterCreationWidget::BuildLayout()
{
 if (!WidgetTree)
  return;

 UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
 Frame->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.065f, 0.97f));
 Frame->SetPadding(FMargin(24.f));
 UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>();
 Frame->AddChild(Root);
 WidgetTree->RootWidget = Frame;

 UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>();
 UVerticalBox* Center = WidgetTree->ConstructWidget<UVerticalBox>();
 UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();
 UHorizontalBoxSlot* LeftSlot = Root->AddChildToHorizontalBox(Left);
 UHorizontalBoxSlot* CenterSlot = Root->AddChildToHorizontalBox(Center);
 UHorizontalBoxSlot* RightSlot = Root->AddChildToHorizontalBox(Right);
 LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 CenterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

 AddLabel(WidgetTree, Left, TEXT("CHOOSE YOUR RACE"), 24);
 RaceList = WidgetTree->ConstructWidget<UScrollBox>();
 UVerticalBoxSlot* RaceSlot = Left->AddChildToVerticalBox(RaceList);
 RaceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

 AddLabel(WidgetTree, Center, TEXT("MOONFALL"), 30);
 AddLabel(WidgetTree, Center, TEXT("CHOOSE YOUR HERO"), 22);
 PreviewImage = WidgetTree->ConstructWidget<UImage>();
 // Reserve a usable interaction region before a portrait or render target loads.
 PreviewImage->SetDesiredSizeOverride(FVector2D(480.f, 600.f));
 UVerticalBoxSlot* PortraitSlot = Center->AddChildToVerticalBox(PreviewImage);
 PortraitSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 PreviewCaption = AddLabel(WidgetTree, Center, TEXT("Original Moonfall preview supplied by game"), 16);
 UButton* PreviousRace = AddAction(WidgetTree, Center, TEXT("Previous Race"));
 UButton* NextRace = AddAction(WidgetTree, Center, TEXT("Next Race"));
 PreviousRace->OnClicked.AddDynamic(this, &UMoonfallCharacterCreationWidget::HandlePreviousRace);
 NextRace->OnClicked.AddDynamic(this, &UMoonfallCharacterCreationWidget::HandleNextRace);
 UButton* Confirm = AddAction(WidgetTree, Center, TEXT("CONTINUE"));
 Confirm->OnClicked.AddDynamic(this, &UMoonfallCharacterCreationWidget::HandleConfirmRace);
 UButton* ResetView = AddAction(WidgetTree, Center, TEXT("RESET VIEW"));
 ResetView->OnClicked.AddDynamic(this, &UMoonfallCharacterCreationWidget::HandleResetPreview);

 AddLabel(WidgetTree, Right, TEXT("RACE DETAILS"), 24);
 RaceTitle = AddLabel(WidgetTree, Right, TEXT(""), 23);
 FamilyTitle = AddLabel(WidgetTree, Right, TEXT(""), 17);
 StatDetails = AddLabel(WidgetTree, Right, TEXT(""), 17);
 UButton* PreviousFamily = AddAction(WidgetTree, Right, TEXT("Previous Family"));
 UButton* NextFamily = AddAction(WidgetTree, Right, TEXT("Next Family"));
 PreviousFamily->OnClicked.AddDynamic(this, &UMoonfallCharacterCreationWidget::HandlePreviousFamily);
 NextFamily->OnClicked.AddDynamic(this, &UMoonfallCharacterCreationWidget::HandleNextFamily);

 // Preserve a preview source bound before NativeConstruct rebuilt the layout.
 if (PreviewTarget)
  SetPreviewRenderTarget(PreviewTarget);
 else
  SetPreviewTexture(PreviewTexture);
}

bool UMoonfallCharacterCreationWidget::InitializeSelection(
 FName PreferredRaceId, float InBaseVitality, float InBasePower, float InBaseSpeed)
{
 FMoonfallRaceSelectionState Next;
 if (!UMoonfallRaceLibrary::BuildPlayableRaceSelectionState(
      PreferredRaceId, NAME_None, InBaseVitality, InBasePower, InBaseSpeed, Next))
  return false;
 BaseVitality = InBaseVitality;
 BasePower = InBasePower;
 BaseSpeed = InBaseSpeed;
 CurrentRaceId = Next.RaceId;
 CurrentFamily = NAME_None;
 RefreshSelection();
 return true;
}

void UMoonfallCharacterCreationWidget::SetPreviewTexture(UTexture2D* ExistingMoonfallTexture)
{
 PreviewTexture = ExistingMoonfallTexture;
 PreviewTarget = nullptr;
 if (!PreviewImage)
  return;
 if (PreviewTexture)
 {
  PreviewImage->SetBrushFromTexture(PreviewTexture);
  PreviewImage->SetVisibility(ESlateVisibility::Visible);
 }
 else
 {
  PreviewImage->SetBrush(FSlateBrush());
  PreviewImage->SetVisibility(ESlateVisibility::HitTestInvisible);
 }
 if (PreviewCaption)
  PreviewCaption->SetText(FText::FromString(
    PreviewTexture ? TEXT("MOONFALL ORIGINAL CHARACTER ART")
                   : TEXT("Character preview is configured by the game")));
}

void UMoonfallCharacterCreationWidget::SetPreviewRenderTarget(
 UTextureRenderTarget2D* ExistingTarget)
{
 PreviewTarget = ExistingTarget;
 PreviewTexture = nullptr;
 if (!PreviewImage)
  return;
 if (!PreviewTarget)
 {
  PreviewImage->SetBrush(FSlateBrush());
  PreviewImage->SetVisibility(ESlateVisibility::HitTestInvisible);
  if (PreviewCaption)
   PreviewCaption->SetText(FText::FromString(TEXT("Character preview is configured by the game")));
  return;
 }
 FSlateBrush Brush;
 Brush.SetResourceObject(PreviewTarget);
 Brush.ImageSize = FVector2D(PreviewTarget->SizeX, PreviewTarget->SizeY);
 PreviewImage->SetBrush(Brush);
 PreviewImage->SetVisibility(ESlateVisibility::Visible);
 if (PreviewCaption)
  PreviewCaption->SetText(FText::FromString(TEXT("LIVE MOONFALL CHARACTER PREVIEW")));
}

void UMoonfallCharacterCreationWidget::BindPreviewActor(
 AMoonfallCharacterPreviewActor* PreviewActor)
{
 // Detach all controls first so an invalid actor cannot leave dangling
 // movement references or a stale image behind.
 bDraggingPreview = false;
 BoundPreviewActor.Reset();
 BindPreviewRig(nullptr, nullptr);
 SetPreviewRenderTarget(nullptr);
 if (!IsValid(PreviewActor))
  return;

 BindPreviewRig(PreviewActor->GetPreviewPivot(), PreviewActor->GetPreviewArm());
 BoundPreviewActor = PreviewActor;
 PreviewActor->SetPreviewRenderTarget(PreviewActor->GetPreviewRenderTarget());
 SetPreviewRenderTarget(PreviewActor->GetPreviewRenderTarget());
}

void UMoonfallCharacterCreationWidget::BindPreviewRig(
 USceneComponent* PreviewPivot, USpringArmComponent* PreviewArm)
{
 // A custom rig overrides any previous actor binding; it cannot refresh
 // the old actor's camera or keep its render target alive by accident.
 BoundPreviewActor.Reset();
 PreviewRotationPivot = PreviewPivot;
 PreviewZoomArm = PreviewArm;
 if (PreviewRotationPivot.IsValid())
  InitialPreviewRotation = PreviewRotationPivot->GetRelativeRotation();
 if (PreviewZoomArm.IsValid())
  InitialPreviewArmLength = PreviewZoomArm->TargetArmLength;
}

void UMoonfallCharacterCreationWidget::RotatePreview(float DeltaYaw)
{
 if (!FMath::IsFinite(DeltaYaw) || FMath::IsNearlyZero(DeltaYaw))
  return;
 DeltaYaw = FMath::Clamp(DeltaYaw, -20.f, 20.f);
 if (PreviewRotationPivot.IsValid())
 {
  PreviewRotationPivot->AddLocalRotation(FRotator(0.f, DeltaYaw, 0.f));
  if (BoundPreviewActor.IsValid())
   BoundPreviewActor->RefreshPreview();
  return;
 }
 OnPreviewRotateRequested(DeltaYaw);
}

void UMoonfallCharacterCreationWidget::ZoomPreview(float ZoomSteps)
{
 if (!FMath::IsFinite(ZoomSteps) || FMath::IsNearlyZero(ZoomSteps))
  return;
 ZoomSteps = FMath::Clamp(ZoomSteps, -1.f, 1.f);
 if (PreviewZoomArm.IsValid())
 {
  const float Distance = PreviewZoomArm->TargetArmLength;
  if (!FMath::IsFinite(Distance))
   return;
  PreviewZoomArm->TargetArmLength =
    FMath::Clamp(Distance - ZoomSteps * 40.f, 125.f, 650.f);
  if (BoundPreviewActor.IsValid())
   BoundPreviewActor->RefreshPreview();
  return;
 }
 OnPreviewZoomRequested(ZoomSteps);
}

void UMoonfallCharacterCreationWidget::ResetPreviewView()
{
 bDraggingPreview = false;
 if (PreviewRotationPivot.IsValid())
  PreviewRotationPivot->SetRelativeRotation(InitialPreviewRotation);
 if (PreviewZoomArm.IsValid())
  PreviewZoomArm->TargetArmLength = InitialPreviewArmLength;
 if (BoundPreviewActor.IsValid())
  BoundPreviewActor->RefreshPreview();
}

void UMoonfallCharacterCreationWidget::RefreshRaceList()
{
 if (!RaceList || !WidgetTree)
  return;
 const bool bSameFamily = bRaceListInitialized && LastRenderedFamily == CurrentFamily;
 const TArray<FMoonfallRaceProfile> Profiles = CurrentFamily.IsNone()
   ? UMoonfallRaceLibrary::GetPlayableRaceProfiles()
   : UMoonfallRaceLibrary::GetPlayableRaceProfilesByFamily(CurrentFamily);
 if (!bSameFamily || RaceChoiceLabels.Num() != Profiles.Num())
 {
  // Only recreate controls when the catalog/family changes. The selected
  // button retains keyboard/controller focus during normal navigation.
  const float PreviousScrollOffset = bSameFamily ? RaceList->GetScrollOffset() : 0.f;
  RaceChoiceLabels.Reset();
  RaceList->ClearChildren();
  for (const FMoonfallRaceProfile& Profile : Profiles)
  {
   UMoonfallRaceChoiceButton* Choice =
     WidgetTree->ConstructWidget<UMoonfallRaceChoiceButton>();
   Choice->InitializeChoice(Profile.RaceId);
   Choice->OnRaceActivated.AddDynamic(this, &UMoonfallCharacterCreationWidget::HandleRaceChoice);
   UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
   Choice->AddChild(Label);
   RaceList->AddChild(Choice);
   RaceChoiceLabels.Add(Profile.RaceId, Label);
  }
  RaceList->SetScrollOffset(PreviousScrollOffset);
 }
 // Update the marker and accessible text without destroying clickable widgets.
 for (const FMoonfallRaceProfile& Profile : Profiles)
 {
  if (const TObjectPtr<UTextBlock>* Label = RaceChoiceLabels.Find(Profile.RaceId))
  {
   const FString Prefix = Profile.RaceId == Selection.RaceId ? TEXT("> ") : TEXT("  ");
   (*Label)->SetText(FText::FromString(Prefix + Profile.DisplayName.ToString()));
  }
 }
 LastRenderedFamily = CurrentFamily;
 bRaceListInitialized = true;
}

void UMoonfallCharacterCreationWidget::RefreshSelection()
{
 FMoonfallRaceSelectionState Next;
 if (!UMoonfallRaceLibrary::BuildPlayableRaceSelectionState(
      CurrentRaceId, CurrentFamily, BaseVitality, BasePower, BaseSpeed, Next))
  return;
 const bool bRaceChanged = Selection.RaceId != Next.RaceId;
 Selection = Next;
 CurrentRaceId = Next.RaceId;
 RefreshRaceList();
 if (bRaceChanged)
  OnRacePreviewRequested(Next.RaceId);
 if (RaceTitle)
  RaceTitle->SetText(Next.Profile.DisplayName);
 if (FamilyTitle)
  FamilyTitle->SetText(FText::FromString(
   FString::Printf(TEXT("%s | Race %d of %d"), *Next.Profile.Family.ToString(),
                    Next.RacePosition, Next.RaceTotal)));
 if (StatDetails)
  StatDetails->SetText(FText::FromString(FString::Printf(
   TEXT("Vitality: %.1f\nPower: %.1f\nSpeed: %.2f\n\nUse arrows / D-pad to navigate."),
   Next.Stats.Vitality, Next.Stats.Power, Next.Stats.Speed)));
}

void UMoonfallCharacterCreationWidget::SelectRace(FName RaceId)
{
 FMoonfallRaceSelectionState Next;
 if (!UMoonfallRaceLibrary::BuildPlayableRaceSelectionState(
      RaceId, CurrentFamily, BaseVitality, BasePower, BaseSpeed, Next)
     || Next.RaceId != RaceId)
  return;
 CurrentRaceId = Next.RaceId;
 RefreshSelection();
}

void UMoonfallCharacterCreationWidget::AdvanceRace(bool bForward)
{
 FMoonfallRaceSelectionState Next;
 if (!UMoonfallRaceLibrary::StepPlayableRaceSelectionState(
      CurrentRaceId, CurrentFamily, bForward,
      BaseVitality, BasePower, BaseSpeed, Next))
  return;
 CurrentRaceId = Next.RaceId;
 RefreshSelection();
}

void UMoonfallCharacterCreationWidget::AdvanceFamily(bool bForward)
{
 FMoonfallRaceSelectionState Next;
 if (!UMoonfallRaceLibrary::StepPlayableRaceFamilySelectionState(
      CurrentRaceId, CurrentFamily, bForward,
      BaseVitality, BasePower, BaseSpeed, Next))
  return;
 CurrentFamily = Next.Family;
 CurrentRaceId = Next.RaceId;
 RefreshSelection();
}

bool UMoonfallCharacterCreationWidget::IsOverPreview(
 const FVector2D& ScreenPosition) const
{
 return PreviewImage && PreviewImage->GetCachedGeometry().IsUnderLocation(ScreenPosition);
}

FReply UMoonfallCharacterCreationWidget::NativeOnMouseButtonDown(
 const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
 if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
     && IsOverPreview(InMouseEvent.GetScreenSpacePosition()))
 {
  bDraggingPreview = true;
  return FReply::Handled().CaptureMouse(TakeWidget());
 }
 return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMoonfallCharacterCreationWidget::NativeOnMouseButtonUp(
 const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
 if (bDraggingPreview && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
 {
  bDraggingPreview = false;
  return FReply::Handled().ReleaseMouseCapture();
 }
 return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMoonfallCharacterCreationWidget::NativeOnMouseMove(
 const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
 if (bDraggingPreview)
 {
  if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
  {
   bDraggingPreview = false;
   return FReply::Handled().ReleaseMouseCapture();
  }
  const float DeltaX = InMouseEvent.GetCursorDelta().X;
  if (FMath::IsFinite(DeltaX) && !FMath::IsNearlyZero(DeltaX))
   RotatePreview(DeltaX * 0.35f);
  return FReply::Handled();
 }
 return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMoonfallCharacterCreationWidget::NativeOnMouseWheel(
 const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
 if (IsOverPreview(InMouseEvent.GetScreenSpacePosition()))
 {
  const float WheelDelta = InMouseEvent.GetWheelDelta();
  if (FMath::IsFinite(WheelDelta) && !FMath::IsNearlyZero(WheelDelta))
   ZoomPreview(WheelDelta);
  return FReply::Handled();
 }
 // Outside the preview, the race list retains normal wheel scrolling.
 return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

void UMoonfallCharacterCreationWidget::NativeOnMouseCaptureLost(
 const FCaptureLostEvent& CaptureLostEvent)
{
 bDraggingPreview = false;
 Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void UMoonfallCharacterCreationWidget::HandleResetPreview() { ResetPreviewView(); }
void UMoonfallCharacterCreationWidget::HandlePreviousRace() { AdvanceRace(false); }
void UMoonfallCharacterCreationWidget::HandleNextRace() { AdvanceRace(true); }
void UMoonfallCharacterCreationWidget::HandlePreviousFamily() { AdvanceFamily(false); }
void UMoonfallCharacterCreationWidget::HandleNextFamily() { AdvanceFamily(true); }
void UMoonfallCharacterCreationWidget::HandleRaceChoice(FName RaceId) { SelectRace(RaceId); }
void UMoonfallCharacterCreationWidget::HandleConfirmRace()
{
 if (!Selection.RaceId.IsNone() && Selection.Profile.bPlayableCandidate)
  OnRaceConfirmed.Broadcast(Selection.RaceId);
}

FReply UMoonfallCharacterCreationWidget::NativeOnKeyDown(
 const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
 const FKey Key = InKeyEvent.GetKey();
 if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
 { HandleConfirmRace(); return FReply::Handled(); }
 if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left)
 { AdvanceRace(false); return FReply::Handled(); }
 if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right)
 { AdvanceRace(true); return FReply::Handled(); }
 if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up)
 { AdvanceFamily(false); return FReply::Handled(); }
 if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down)
 { AdvanceFamily(true); return FReply::Handled(); }
 return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
