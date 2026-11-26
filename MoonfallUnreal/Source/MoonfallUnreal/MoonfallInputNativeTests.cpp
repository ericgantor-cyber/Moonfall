// Native loaded input-settings contract, not a physical input-device test.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallCurrentInputContract,
    "Moonfall.Controls.CurrentInputContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallCurrentInputContract::RunTest(const FString& Parameters)
{
 const UInputSettings* Settings = GetDefault<UInputSettings>();
 if (!TestNotNull(TEXT("Loaded Unreal InputSettings"), Settings)) return false;
 const auto& Actions = Settings->GetActionMappings();
 const auto& Axes = Settings->GetAxisMappings();
 auto RequireExclusiveAction = [&](const FKey& Key, const FName Action) {
  int32 Count = 0;
  for (const FInputActionKeyMapping& Mapping : Actions) {
   if (Mapping.Key != Key) continue;
   ++Count;
   TestTrue(FString::Printf(TEXT("%s routes only to %s"), *Key.ToString(), *Action.ToString()),
            Mapping.ActionName == Action);
  }
  TestEqual(FString::Printf(TEXT("%s has exactly one action"), *Key.ToString()), Count, 1);
 };
 RequireExclusiveAction(EKeys::LeftMouseButton, TEXT("ResidentMouseClick"));
 RequireExclusiveAction(EKeys::RightMouseButton, TEXT("CastRightMouse"));
 RequireExclusiveAction(EKeys::Gamepad_RightTrigger, TEXT("RightHandAction"));
 RequireExclusiveAction(EKeys::Gamepad_LeftTrigger, TEXT("DualCast"));
 RequireExclusiveAction(EKeys::Gamepad_FaceButton_Bottom, TEXT("ControllerConfirm"));
 RequireExclusiveAction(EKeys::Gamepad_FaceButton_Right, TEXT("ControllerBack"));
 RequireExclusiveAction(EKeys::Gamepad_RightThumbstick, TEXT("ControllerRightStick"));
 RequireExclusiveAction(EKeys::Gamepad_LeftShoulder, TEXT("ControllerLeftShoulder"));
 RequireExclusiveAction(EKeys::Gamepad_RightShoulder, TEXT("ControllerRightShoulder"));
 RequireExclusiveAction(EKeys::C, TEXT("CharacterStats"));
 RequireExclusiveAction(EKeys::B, TEXT("Inventory"));
 RequireExclusiveAction(EKeys::J, TEXT("Atlas"));
 RequireExclusiveAction(EKeys::P, TEXT("SpellBook"));
 const FKey MovementKeys[] = {EKeys::W,EKeys::S,EKeys::A,EKeys::D,EKeys::Up,EKeys::Down,EKeys::Left,EKeys::Right};
 const FName MovementAxes[] = {TEXT("MoveForward"),TEXT("MoveForward"),TEXT("MoveRight"),TEXT("MoveRight"),
                               TEXT("MoveForward"),TEXT("MoveForward"),TEXT("MoveRight"),TEXT("MoveRight")};
 const float MovementScales[] = {1.f,-1.f,-1.f,1.f,1.f,-1.f,-1.f,1.f};
 for (int32 Index=0; Index<UE_ARRAY_COUNT(MovementKeys); ++Index) {
  int32 Count=0;
  for (const FInputActionKeyMapping& Mapping : Actions)
   TestFalse(TEXT("Movement key must never own an action"), Mapping.Key == MovementKeys[Index]);
  for (const FInputAxisKeyMapping& Mapping : Axes) {
   if (Mapping.Key != MovementKeys[Index]) continue;
   ++Count;
   TestTrue(TEXT("Movement axis and scale match"),
       Mapping.AxisName == MovementAxes[Index] && Mapping.Scale == MovementScales[Index]);
  }
  TestEqual(FString::Printf(TEXT("%s has exactly one movement axis"), *MovementKeys[Index].ToString()), Count, 1);
 }
 return !HasAnyErrors();
}
#endif
