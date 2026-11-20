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
    jump_section=pawn.split("void AMoonfallHeroPawn::TryJump()",1)[1].split(
        "void AMoonfallHeroPawn::MoveForward(",1)[0]
    assert "PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom)" in jump_section
    assert "ToggleCharacterStats();" in jump_section
    assert "if (bInventoryOpen && !bOptionsOpen && !bAtlasOpen)" in jump_section
    assert "Jump();" in jump_section
    assert '+ActionMappings=(ActionName="CharacterStats",Key=Gamepad_FaceButton_Bottom)' not in PATH.read_text(encoding="utf-8")
    # The Atlas is a modal screen, not a combat resource-farming context.
    dodge_reward=pawn.split("bool AMoonfallHeroPawn::TryRewardPerfectDodge()",1)[1].split(
        "bool AMoonfallHeroPawn::Apply",1)[0]
    racial_power=pawn.split("bool AMoonfallHeroPawn::ActivateRacePower()",1)[1].split(
        "const float Now = GetWorld()->GetTimeSeconds();",1)[0]
    assert "bOptionsOpen || bInventoryOpen || bAtlasOpen) return false;" in dodge_reward
    assert "bInventoryOpen || bOptionsOpen || bAtlasOpen) return false;" in racial_power
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
