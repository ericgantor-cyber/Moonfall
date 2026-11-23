"""Contract tests: WASD and keyboard arrows must only drive movement.

Source-level checks run without Godot or Unreal; engine playtesting is separate.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
RESERVED = {"W", "A", "S", "D", "Up", "Down", "Left", "Right"}
AXES = {
    "W": ("forward", 1),
    "S": ("forward", -1),
    "Up": ("forward", 1),
    "Down": ("forward", -1),
    "A": ("right", -1),
    "D": ("right", 1),
    "Left": ("right", -1),
    "Right": ("right", 1),
}


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


class MovementKeyContracts(unittest.TestCase):
    def check_unreal(self, path, forward, right):
        config = read(path)
        actions = [line for line in config.splitlines()
                   if line.startswith("+ActionMappings=")]
        for action in actions:
            key = re.search(r"Key=([^,)]+)", action)
            self.assertIsNotNone(key, action)
            self.assertNotIn(key.group(1), RESERVED, action)

        axes = {line for line in config.splitlines()
                if line.startswith("+AxisMappings=")}
        for key, (direction, scale) in AXES.items():
            axis = forward if direction == "forward" else right
            self.assertTrue(any(
                f'AxisName="{axis}"' in line
                and f"Scale={scale:.6f}" in line
                and f"Key={key})" in line
                for line in axes
            ), f"{path}: missing movement binding for {key}")

    def test_production_unreal_axes_are_movement_only(self):
        self.check_unreal("MoonfallUnreal/Config/DefaultInput.ini",
                          "MoveForward", "MoveRight")

    def test_unreal_prototype_axes_are_movement_only(self):
        self.check_unreal("engine-comparison/unreal/MoonfallPrototype/Config/DefaultInput.ini",
                          "Forward", "Right")
        prototype = read("engine-comparison/unreal/MoonfallPrototype/Config/DefaultInput.ini")
        self.assertIn('AxisName="MapPanHorizontal",Scale=1.000000,Key=Gamepad_RightX', prototype)

    def test_godot_sword_angles_not_triggered_by_movement(self):
        script = read("godot-v38/scripts/native_v38_0.gd")
        self.assertIn("event.alt_pressed", script)
        self.assertIn("[KEY_1,KEY_2,KEY_3,KEY_4].find(event.keycode)", script)
        self.assertNotRegex(script, r"KEY_(UP|DOWN|LEFT|RIGHT)\b")

    def test_godot_wait_and_talent_controls_not_arrows(self):
        wait = read("godot-v38/scripts/native_v11.gd")
        wait_controls = wait.split("func _unhandled_key_input(", 1)[1].split("if wait_transition_active:", 1)[0]
        for direction in ("LEFT", "RIGHT", "UP", "DOWN"):
            self.assertNotIn(f"KEY_{direction}:", wait_controls)
        self.assertIn("KEY_PAGEUP:wait_hours=", wait_controls)
        self.assertIn("KEY_PAGEDOWN:wait_hours=", wait_controls)
        web = read("godot-v38/scripts/systems/moonfall_talent_web.gd")
        for direction in ("LEFT", "RIGHT", "UP", "DOWN"):
            self.assertNotIn(f"key.keycode == KEY_{direction}", web)
        self.assertIn("KEY_KP_4", web)
        self.assertIn("KEY_KP_6", web)

    def test_unreal_creator_does_not_repurpose_movement(self):
        creator = read("MoonfallUnreal/Source/MoonfallUnreal/MoonfallCharacterCreationWidget.cpp")
        key_handler = creator.split("NativeOnKeyDown(", 1)[1]
        self.assertNotRegex(key_handler, r"EKeys::(Up|Down|Left|Right)\b")
        self.assertIn("EKeys::Gamepad_DPad_Left", key_handler)
        self.assertIn("EKeys::PageUp", key_handler)

    def test_godot3d_rebinds_cannot_use_movement_for_abilities(self):
        game = read("Moonfall3D/scripts/game.gd")
        self.assertIn("func _is_movement_key(key: int)", game)
        self.assertIn("_is_movement_key(key) and except_action not in", game)
        self.assertIn("and _is_movement_key(int(parsed[action]))", game)
        for key in ("W", "A", "S", "D", "UP", "DOWN", "LEFT", "RIGHT"):
            self.assertIn(f"Input.is_key_pressed(KEY_{key})", game)


if __name__ == "__main__":
    unittest.main()
