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
    print("Safe gameplay Jump button: PASS (no menu binding)")
    print("Mapping presence: PASS")
    print("Shared action keys needing contextual UE5 testing: %d" % len(conflicts))
    for key,names in conflicts.items():
        print("  %s: %s" % (key, ", ".join(names)))
    print("Conflicts are NOT proven safe; test each context before merging.")

if __name__=="__main__":
    main()
