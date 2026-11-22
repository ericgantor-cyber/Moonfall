// Validated by the real Unreal Editor on a UE5.4-equipped Windows runner.
// This verifies native-loaded InputSettings, not physical mouse switch hardware.
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
 const auto CountBinding = [&Mappings](const FName Action, const FKey Key) -> int32 {
  int32 Count = 0;
  for (const FInputActionKeyMapping& Mapping : Mappings) {
   if (Mapping.ActionName == Action && Mapping.Key == Key
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
 TestEqual(TEXT("T locks target"), CountBinding(TEXT("LockTarget"), EKeys::T), 1);
 TestEqual(TEXT("Middle click locks target"),
   CountBinding(TEXT("LockTarget"), EKeys::MiddleMouseButton), 1);
 TestEqual(TEXT("N activates racial ability"), CountBinding(TEXT("RacePower"), EKeys::N), 1);
 return !HasAnyErrors();
}
#endif
