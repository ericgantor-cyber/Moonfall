"""Audit UE5 controller and keyboard input mappings.

Conflicts are *reported* rather than silently removed because some mappings
are deliberately context dependent. Actual gameplay conflicts need UE5 tests.
"""
from collections import defaultdict
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
PATH=ROOT/"MoonfallUnreal"/"Config"/"DefaultInput.ini"
PATTERN=re.compile(r'^\+ActionMappings=\(ActionName="([^"]+)",Key=([^,)]+)')
AXIS=re.compile(r'^\+AxisMappings=\(AxisName="([^"]+)",Scale=([^,]+),Key=([^,)]+)')

def scan(text):
    actions=defaultdict(set)
    axes=defaultdict(set)
    for line in text.splitlines():
        action=PATTERN.match(line.strip())
        if action:
            actions[action.group(2)].add(action.group(1))
        axis=AXIS.match(line.strip())
        if axis:
            axes[axis.group(3)].add(axis.group(1))
    conflicts={key: sorted(names) for key,names in sorted(actions.items())
               if len(names)>1}
    return actions,axes,conflicts

def main():
    actions,axes,conflicts=scan(PATH.read_text(encoding="utf-8"))
    required_actions={
        "Gamepad_FaceButton_Bottom":{"Jump"},
        "Gamepad_Special_Right":{"Options"},
        "LeftMouseButton":{"ResidentMouseClick"},
        "Escape":{"Options"},
        "J":{"Atlas"},
        "B":{"Inventory"},
        "C":{"CharacterStats"},
    }
    for key,required in required_actions.items():
        if not required.issubset(actions.get(key,set())):
            raise SystemExit("Missing essential mapping for "+key+": "+str(sorted(required)))
    required_axes={
        "Gamepad_LeftX":{"MoveRight"},"Gamepad_LeftY":{"MoveForward"},
        "Gamepad_RightX":{"Turn"},"Gamepad_RightY":{"LookUp"},
    }
    for key,required in required_axes.items():
        if not required.issubset(axes.get(key,set())):
            raise SystemExit("Missing controller axis for "+key)
    # These five shared buttons remain context-sensitive. Their gameplay
    # actions and inventory actions must be verified with UE5 runtime input.
    expected_contextual={
        "Gamepad_FaceButton_Left": {"InventorySalvage","PickupLoot"},
        "Gamepad_FaceButton_Right": {"AscendGear","Dodge"},
        "Gamepad_LeftShoulder": {"CastLeft","ReforgeGear"},
        "Gamepad_LeftTrigger": {"DualCast","EquipBest"},
        "Gamepad_RightShoulder": {"CastRight","ForgeFive"},
    }
    # Current Atlas uses RightThumbstick for Chronicle focus, while combat
    # uses the same button for Guard. Only this exact pair is permitted.
    # The branch can also run before Chronicle lands from main.
    thumb_actions = actions.get("Gamepad_RightThumbstick", set())
    if thumb_actions == {"ChronicleFocus", "Guard"}:
        expected_contextual["Gamepad_RightThumbstick"] = {"ChronicleFocus", "Guard"}
    elif thumb_actions != {"Guard"}:
        raise SystemExit("Unexpected RightThumbstick action mapping: " +
                         str(sorted(thumb_actions)))
    for key, actions_expected in expected_contextual.items():
        actual = actions.get(key,set())
        if not actions_expected.issubset(actual):
            raise SystemExit("Lost expected contextual bind on "+key+": "+str(sorted(actual)))
    if len(actions.get("Gamepad_FaceButton_Bottom",set()))!=1:
        raise SystemExit("Gamepad A must only activate Jump, not a character menu")
    if set(conflicts)!=set(expected_contextual):
        raise SystemExit("Unexpected shared controller or keyboard actions: "+
                         str({k:v for k,v in conflicts.items()
                              if k not in expected_contextual}))
    pawn=(ROOT/"MoonfallUnreal"/"Source"/"MoonfallUnreal"/"MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
    if "ChronicleFocus" in thumb_actions:
        # Both mapped callbacks execute on the same press; the Atlas focus
        # callback must be disabled in gameplay, Guard disabled in Atlas.
        chronicle=pawn.split("void AMoonfallHeroPawn::ToggleAtlasChronicleFocus()",1)[1].split(
            "void AMoonfallHeroPawn::StepAtlasChronicle(",1)[0]
        guard=pawn.split("void AMoonfallHeroPawn::BeginGuard()",1)[1].split(
            "void AMoonfallHeroPawn::EndGuard()",1)[0]
        if "if (!bAtlasOpen" not in chronicle.split("return;",1)[0]:
            raise SystemExit("Chronicle focus must be disabled outside the Atlas")
        if "bAtlasOpen" not in guard.split("return;",1)[0]:
            raise SystemExit("Guard must be disabled inside the Atlas")
    jump_section=pawn.split("void AMoonfallHeroPawn::TryJump()",1)[1].split(
        "void AMoonfallHeroPawn::MoveForward(",1)[0]
    assert "PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom)" in jump_section
    assert "ToggleCharacterStats();" in jump_section
    assert "if (bInventoryOpen && !bOptionsOpen && !bAtlasOpen)" in jump_section
    # A stale/streamed-out NPC must not allow A to fall through to Jump.
    assert "if (bResidentContextOpen) {" in jump_section
    resident_block=jump_section.split("if (bResidentContextOpen) {",1)[1].split(
        "// A has one input binding:",1)[0]
    assert "FocusedResident.IsValid()" in resident_block
    assert "ChooseResidentContextAction(ResidentContextSelection);" in resident_block
    assert "return;" in resident_block
    assert "Jump();" not in resident_block
    # Invalid NPC pointers must close the modal on the same input event,
    # never falling through to combat Jump or silently changing quest state.
    assert "if (!FocusedResident.IsValid()) {" in resident_block
    assert resident_block.index("CloseResidentContext();") < resident_block.index(
        "ChooseResidentContextAction(ResidentContextSelection);")
    assert "Jump();" in jump_section
    # Up/Down share the gameplay and menu bindings. A stale dialogue must be
    # closed before selecting either a different NPC or an unrelated menu.
    dpad=pawn.split("void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[1].split(
        "void AMoonfallHeroPawn::CastRightMouse()",1)[0]
    assert "if (!FocusedResident.IsValid() || !FocusedResident->CanHeroInteract(this))" in dpad
    assert dpad.index("CloseResidentContext();") < dpad.index(
        "AMoonfallWorldNPC* Resident = GetNearestResidentNPC();")
    for nav_name,next_name in (("OptionsUp","OptionsDown"),("OptionsDown","OptionsLeft")):
        nav=pawn.split(f"void AMoonfallHeroPawn::{nav_name}()",1)[1].split(
            f"void AMoonfallHeroPawn::{next_name}()",1)[0]
        assert "if (bResidentContextOpen) {" in nav
        assert "if (!FocusedResident.IsValid()) { CloseResidentContext(); return; }" in nav
        assert nav.index("CloseResidentContext(); return;") < nav.index(
            "ResidentContextSelection =")
    print("Stale NPC dialogue closes on A and D-pad without leaking input: PASS (source verified)")
    # Dialogue is modal even if its NPC weak pointer becomes invalid after
    # streaming: neither keyboard nor gamepad movement may move the hero.
    for movement_name in ("MoveForward", "MoveRight"):
        movement=pawn.split(
            f"void AMoonfallHeroPawn::{movement_name}(float Value)",1)[1].split(
            "return;",1)[0]
        for blocker in (
            "Health <= 0.f", "bOptionsOpen", "bInventoryOpen",
            "bAtlasOpen", "bResidentContextOpen", "FocusedResident.IsValid()",
            "!FMath::IsFinite(Value)"
        ):
            assert blocker in movement, (movement_name, blocker)
    print("Resident dialogue and streamed-out NPC block movement: PASS (source verified)")
    assert '+ActionMappings=(ActionName="CharacterStats",Key=Gamepad_FaceButton_Bottom)' not in PATH.read_text(encoding="utf-8")
    # The Atlas is a modal screen, not a combat resource-farming context.
    dodge_reward=pawn.split("bool AMoonfallHeroPawn::TryRewardPerfectDodge()",1)[1].split(
        "bool AMoonfallHeroPawn::Apply",1)[0]
    racial_power=pawn.split("bool AMoonfallHeroPawn::ActivateRacePower()",1)[1].split(
        "const float Now = GetWorld()->GetTimeSeconds();",1)[0]
    assert "bOptionsOpen || bInventoryOpen || bAtlasOpen) return false;" in dodge_reward
    assert "bInventoryOpen || bOptionsOpen || bAtlasOpen || bResidentContextOpen) return false;" in racial_power
    # A resident conversation is a modal interaction: combat stance, sprint,
    # and consumable racial power cannot activate while choosing dialogue.
    for signature in (
        "void AMoonfallHeroPawn::BeginGuard()",
        "void AMoonfallHeroPawn::BeginSprint()",
    ):
        section=pawn.split(signature,1)[1].split("\n}",1)[0]
        assert "bResidentContextOpen" in section.split("return;",1)[0], signature
    assert "bResidentContextOpen" in racial_power
    # Opening an NPC conversation must release existing held inputs, not just
    # prevent new ones. All cleanup must precede modal activation/callback.
    resident_open=pawn.split("void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[1].split(
        "void AMoonfallHeroPawn::CastRightMouse()",1)[0]
    acquired=resident_open.split("AMoonfallWorldNPC* Resident = GetNearestResidentNPC();",1)[1]
    before_modal=acquired.split("bResidentContextOpen = true;",1)[0]
    for cleanup in ("StopJumping();", "EndSprint();", "EndGuard();",
                    "GetCharacterMovement()->StopMovementImmediately();"):
        assert cleanup in before_modal, cleanup
    assert acquired.index("EndSprint();") < acquired.index("FocusedResident = Resident;")
    assert acquired.index("EndGuard();") < acquired.index("FocusedResident = Resident;")
    # An NPC may stream out between input events. Opening B/J must clear the
    # modal flag even when its weak actor pointer is already invalid.
    for signature, next_signature in (
        ("void AMoonfallHeroPawn::ToggleInventory()", "void AMoonfallHeroPawn::EquipSelectedInventoryItem()"),
        ("void AMoonfallHeroPawn::ToggleAtlas()", "void AMoonfallHeroPawn::Atlas"),
    ):
        section=pawn.split(signature,1)[1].split(next_signature,1)[0]
        assert "if (bResidentContextOpen || FocusedResident.IsValid()) CloseResidentContext();" in section, signature
    print("Inventory/Atlas clear streamed-out NPC dialogue state: PASS (source verified)")
    # Neither timed auto-loot nor manual pickup should mutate world loot
    # while a dialogue choice is active. The timer must reset in that state.
    auto_loot=pawn.split("// Throttled automatic pickup",1)[1].split(
        "// Only track pointer motion",1)[0]
    assert "!bResidentContextOpen" in auto_loot.split("AutoLootElapsed +=",1)[0]
    assert "else AutoLootElapsed = 0.f;" in auto_loot
    manual_loot=pawn.split("void AMoonfallHeroPawn::PickupNearestGroundLoot()",1)[1].split(
        "// No arbitrary six-drop",1)[0]
    assert "bResidentContextOpen" in manual_loot
    print("NPC dialogue blocks auto/manual loot and resets pickup timer: PASS (source verified)")
    print("Resident dialogue releases held sprint, guard, jump and movement: PASS (source verified)")
    print("Resident dialogue blocks sprint, guard and racial powers: PASS (source verified)")
    # After a resident streams out, the dialogue flag can outlive the pointer.
    # Dodge must not grant immunity, launch the hero or spend stamina then.
    dodge=pawn.split("void AMoonfallHeroPawn::Dodge()",1)[1].split(
        "void AMoonfallHeroPawn::RewardPerfectDodge()",1)[0]
    dodge_guard=dodge.split("return;",1)[0]
    assert "bResidentContextOpen" in dodge_guard
    assert "FocusedResident.IsValid()" in dodge_guard
    assert dodge.index("bResidentContextOpen") < dodge.index(
        "Stamina = FMath::Clamp(Stamina - DodgeStaminaCost")
    print("Resident dialogue blocks dodge even after NPC streaming: PASS (source verified)")
    # Guard all five shared gamepad buttons against cross-context effects.
    # Source checks supplement, but never replace, a native controller playtest.
    def start(signature):
        return pawn.split(signature,1)[1][:600]
    for signature in (
        "void AMoonfallHeroPawn::PickupNearestGroundLoot()",
        "void AMoonfallHeroPawn::Dodge()",
        "bool AMoonfallHeroPawn::TryCast(",
    ):
        section=start(signature)
        assert "bInventoryOpen" in section and "bAtlasOpen" in section, signature
    assert "bInventoryOpen && !bCharacterStatsOpen" in start(
        "void AMoonfallHeroPawn::SalvageSelectedInventoryItem()")
    for signature in (
        "void AMoonfallHeroPawn::AscendSelectedInventoryItem()",
        "void AMoonfallHeroPawn::ReforgeSelectedInventoryItem()",
        "void AMoonfallHeroPawn::ForgeFiveSelectedInventoryItem()",
        "void AMoonfallHeroPawn::EquipBestInventoryAction()",
    ):
        assert "!bCharacterStatsOpen" in start(signature), signature
    for signature in (
        "bool AMoonfallHeroPawn::AscendInventoryItemRarity(",
        "int32 AMoonfallHeroPawn::ForgeInventoryItemLevels(",
        "bool AMoonfallHeroPawn::ReforgeInventoryItemAffix(",
        "int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades(",
    ):
        section=start(signature)
        assert "!bInventoryOpen" in section and "bOptionsOpen" in section, signature
    print("Five shared gamepad buttons: source context guards PASS; UE5 playtest pending")
    print("Atlas modal combat resource guards: PASS (source verified)")
    print("Contextual Gamepad A: Jump in world, Character Stats in inventory (source verified)")
    print("Safe gameplay Jump button: PASS (no menu binding)")
    print("Mapping presence: PASS")
    print("Shared action keys needing contextual UE5 testing: %d" % len(conflicts))
    for key,names in conflicts.items():
        print("  %s: %s" % (key, ", ".join(names)))
    print("Conflicts are NOT proven safe; test each context before merging.")

if __name__=="__main__":
    main()
