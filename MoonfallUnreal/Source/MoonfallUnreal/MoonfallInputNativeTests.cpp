// Unreal Editor automation test for the loaded controller and mouse bindings.
// Runs only with UE5 editor automation; CI static checks do NOT execute it.
// Tests InputSettings mappings, not physical MB5 or controller hardware.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallMB5InputBindingsTest,
  "Moonfall.Controls.MB5DualCastBindings",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallMB5InputBindingsTest::RunTest(const FString& Parameters)
{
 const UInputSettings* Settings = GetDefault<UInputSettings>();
 if (!TestNotNull(TEXT("UE5 input settings available"), Settings)) return false;
 const TArray<FInputActionKeyMapping>& Mappings = Settings->GetActionMappings();

 // Only unmodified presses count; an unexpected Shift/Ctrl modifier must not
 // satisfy the expected accessible one-press MB5 binding.
 const auto CountBinding = [&Mappings](const TCHAR* Action, const FKey Key) -> int32 {
  const FName ActionName(Action);
  int32 Count = 0;
  for (const FInputActionKeyMapping& Mapping : Mappings) {
   if (Mapping.ActionName == ActionName && Mapping.Key == Key
       && !Mapping.bShift && !Mapping.bCtrl && !Mapping.bAlt && !Mapping.bCmd)
    ++Count;
  }
  return Count;
 };
 int32 AllMB5Bindings = 0;
 for (const FInputActionKeyMapping& Mapping : Mappings)
  if (Mapping.Key == EKeys::ThumbMouseButton2) ++AllMB5Bindings;

 TestEqual(TEXT("MB5 has exactly one mapping across all actions"), AllMB5Bindings, 1);
 TestEqual(TEXT("MB5 exclusively dual-casts"), 
   CountBinding(TEXT("DualCast"), EKeys::ThumbMouseButton2), 1);
 TestEqual(TEXT("Left mouse retains its contextual spell and interaction action"),
   CountBinding(TEXT("ResidentMouseClick"), EKeys::LeftMouseButton), 1);
 TestEqual(TEXT("Right mouse retains contextual casting"),
   CountBinding(TEXT("CastRightMouse"), EKeys::RightMouseButton), 1);
 TestEqual(TEXT("Controller A jumps or confirms dialogue, not both Jump and Stats"),
   CountBinding(TEXT("Jump"), EKeys::Gamepad_FaceButton_Bottom), 1);
 TestEqual(TEXT("Controller A does not also open Character Stats"),
   CountBinding(TEXT("CharacterStats"), EKeys::Gamepad_FaceButton_Bottom), 0);
 TestEqual(TEXT("R3 opens Character Stats only in inventory through ChronicleFocus"),
   CountBinding(TEXT("ChronicleFocus"), EKeys::Gamepad_RightThumbstick), 1);
 TestEqual(TEXT("R3 still guards in gameplay"),
   CountBinding(TEXT("Guard"), EKeys::Gamepad_RightThumbstick), 1);
 TestEqual(TEXT("Controller left trigger retains dual-cast"),
   CountBinding(TEXT("DualCast"), EKeys::Gamepad_LeftTrigger), 1);
 TestEqual(TEXT("Controller left shoulder retains single-hand cast"),
   CountBinding(TEXT("CastLeft"), EKeys::Gamepad_LeftShoulder), 1);
 TestEqual(TEXT("Controller right shoulder retains single-hand cast"),
   CountBinding(TEXT("CastRight"), EKeys::Gamepad_RightShoulder), 1);

 TestEqual(TEXT("Z does not cast left"), CountBinding(TEXT("CastLeft"), EKeys::Z), 0);
 TestEqual(TEXT("X does not cast right"), CountBinding(TEXT("CastRight"), EKeys::X), 0);
 TestEqual(TEXT("Z does not dual cast"), CountBinding(TEXT("DualCast"), EKeys::Z), 0);
 TestEqual(TEXT("X does not dual cast"), CountBinding(TEXT("DualCast"), EKeys::X), 0);
 TestEqual(TEXT("T locks target"), CountBinding(TEXT("LockOn"), EKeys::T), 1);
 TestEqual(TEXT("Middle click locks target"),
   CountBinding(TEXT("LockOn"), EKeys::MiddleMouseButton), 1);
 TestEqual(TEXT("N activates racial ability"), CountBinding(TEXT("RacePower"), EKeys::N), 1);
 TestEqual(TEXT("Y is not racial ability"), CountBinding(TEXT("RacePower"), EKeys::Y), 0);
 TestEqual(TEXT("R3 cannot also dual-cast in gameplay"),
   CountBinding(TEXT("DualCast"), EKeys::Gamepad_RightThumbstick), 0);
 TestEqual(TEXT("Right shoulder is not dodge"),
   CountBinding(TEXT("Dodge"), EKeys::Gamepad_RightShoulder), 0);
 TestEqual(TEXT("Left trigger no longer casts a single left spell"),
   CountBinding(TEXT("CastLeft"), EKeys::Gamepad_LeftTrigger), 0);
 TestEqual(TEXT("Right trigger no longer casts a single right spell"),
   CountBinding(TEXT("CastRight"), EKeys::Gamepad_RightTrigger), 0);
 TestEqual(TEXT("Controller B is contextual back/dodge"),
   CountBinding(TEXT("GamepadBack"), EKeys::Gamepad_FaceButton_Right), 1);
 TestEqual(TEXT("D-pad up reaches NPC dialogue/contextual lock-on"),
   CountBinding(TEXT("ResidentContext"), EKeys::Gamepad_DPad_Up), 1);
 TestEqual(TEXT("D-pad down retains direct lock-on"),
   CountBinding(TEXT("LockOn"), EKeys::Gamepad_DPad_Down), 1);
 return !HasAnyErrors();
}
#endif
