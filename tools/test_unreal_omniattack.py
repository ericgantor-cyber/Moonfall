"""UE5-only, engine-free policy and source contracts for native sword combat."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
GAME = ROOT / "MoonfallUnreal"
SRC = GAME / "Source" / "MoonfallUnreal"


class UnrealOmniattackTests(unittest.TestCase):
    def test_four_angle_policy_compiles_and_cycles_without_repeats(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if compiler is None:
            self.skipTest("C++17 compiler not installed in this environment")
        cpp = r"""
#include "MoonfallOmniattack.h"
#include <array>
int main() {
 for (unsigned seed=0; seed<16; ++seed) {
  MoonfallOmniattack::FFlow flow;
  int previous=-1;
  for (int cycle=0; cycle<32; ++cycle) {
   std::array<int, 4> seen = {0,0,0,0};
   for (int step=0; step<4; ++step) {
    const int angle = MoonfallOmniattack::Next(flow, seed*7u+cycle*3u+step);
    if (angle<0 || angle>=4 || angle==previous) return 1;
    seen[angle]++;
    previous=angle;
    if (flow.Step!=step+1) return 2;
   }
   for (int count:seen) if (count!=1) return 3;
  }
  MoonfallOmniattack::Reset(flow);
  if (flow.Last!=-1 || flow.Remaining!=0x0f || flow.Step!=0) return 4;
 }
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as tmp:
            source = pathlib.Path(tmp) / "omni.cpp"
            binary = pathlib.Path(tmp) / "omni"
            source.write_text(cpp, encoding="utf-8")
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                            "-Werror", "-I", str(SRC), str(source), "-o",
                            str(binary)], check=True, capture_output=True, text=True)
            subprocess.run([str(binary)], check=True, capture_output=True, text=True)

    def test_native_melee_has_real_collision_and_no_spell_cast(self):
        hero = (SRC / "MoonfallHeroPawn.cpp").read_text()
        code = hero.split("void AMoonfallHeroPawn::MeleeAttack()", 1)[1].split(
            "void AMoonfallHeroPawn::BreakGuard()", 1)[0]
        for required in ("SweepSingleByChannel(", "ECC_Visibility",
                         "FCollisionShape::MakeSphere(", "NextMeleeAttackAt",
                         "MeleeStaminaCost", "SwordFlowStep",
                         "MoonfallOmniattack::Next(", "TakeMeleeDamage(",
                         "ApplyPostureDamage(", "OnMeleeSwing(", "ComboMontages",
                         "LastHitComboTarget"):
            self.assertIn(required, code)
        self.assertNotIn("TryCast(", code)
        self.assertIn("if (!bResidentContextOpen) { RightHandAction(); return; }", hero)
        self.assertIn('Input->BindAction(TEXT("RightHandAction"), IE_Pressed', hero)
        self.assertIn("void AMoonfallHeroPawn::CastLeft()", hero)
        self.assertIn("void AMoonfallHeroPawn::CastRight()", hero)
        enemy = (SRC / "MoonfallEnemy.cpp").read_text()
        self.assertIn("void AMoonfallEnemy::TakeMeleeDamage(float Amount)", enemy)
        self.assertIn("TakeSpellDamage(Amount);", enemy)

    def test_skyrim_style_primary_uses_equipped_right_hand(self):
        hero = (SRC / "MoonfallHeroPawn.cpp").read_text()
        header = (SRC / "MoonfallHeroPawn.h").read_text()
        action = hero.split("void AMoonfallHeroPawn::RightHandAction()", 1)[1].split(
            "bool AMoonfallHeroPawn::ReadyRightHandSpell", 1)[0]
        self.assertIn("if (bRightHandSpellReadied) CastRight();", action)
        self.assertIn("else MeleeAttack();", action)
        self.assertIn("bool AMoonfallHeroPawn::ReadyRightHandSpell(int32 School)", hero)
        ready_spell = hero.split("bool AMoonfallHeroPawn::ReadyRightHandSpell(int32 School)", 1)[1].split(
            "void AMoonfallHeroPawn::ReadyRightHandWeapon()", 1)[0]
        self.assertIn("School < 0 || School >= 6", ready_spell)
        self.assertIn("RightSpell = School;", ready_spell)
        self.assertIn("bRightHandSpellReadied = true;", ready_spell)
        equip_weapon = hero.split("bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[1].split(
            "bool AMoonfallHeroPawn::", 1)[0]
        self.assertIn("if (SlotIndex == 0) ReadyRightHandWeapon();", equip_weapon)
        self.assertIn("bool bRightHandSpellReadied = false;", header)
        self.assertIn("OnRightHandLoadoutChanged", header)
        self.assertNotIn("TryCast(", action)

    def test_favorites_only_equip_inside_inventory(self):
        hero = (SRC / "MoonfallHeroPawn.cpp").read_text()
        header = (SRC / "MoonfallHeroPawn.h").read_text()
        equip = hero.split("bool AMoonfallHeroPawn::EquipRightHandFavorite(int32 FavoriteIndex)", 1)[1].split(
            "int32 AMoonfallHeroPawn::HitTestRightHandFavorite()", 1)[0]
        for lock in ("!bInventoryOpen", "bCharacterStatsOpen", "bOptionsOpen",
                     "bAtlasOpen", "bResidentContextOpen", "Health <= 0.f",
                     "FavoriteIndex < 0 || FavoriteIndex > 6"):
            self.assertIn(lock, equip)
        self.assertIn("if (FavoriteIndex == 0)", equip)
        self.assertIn("ReadyRightHandWeapon();", equip)
        self.assertIn("return ReadyRightHandSpell(FavoriteIndex - 1);", equip)
        self.assertIn("EquipRightHandFavorite(int32 FavoriteIndex);", header)
        self.assertNotIn("ToggleRightHandLoadout", hero + header)
        self.assertIn("void AMoonfallHeroPawn::CastLeft()", hero)
        self.assertIn("void AMoonfallHeroPawn::CastRight()", hero)
        config = (GAME / "Config" / "DefaultInput.ini").read_text()
        self.assertIn('ActionName="CastLeft",Key=Z', config)
        self.assertIn('ActionName="CastRight",Key=X', config)
        self.assertNotIn("Key=V)", config)
        self.assertNotIn("ToggleRightHandLoadout", config)

    def test_inventory_favorites_mouse_and_controller_areas_match_hud(self):
        hero = (SRC / "MoonfallHeroPawn.cpp").read_text()
        hud = (SRC / "MoonfallCombatHUD.cpp").read_text()
        clicks = hero.split("void AMoonfallHeroPawn::ResidentContextMouseClick()", 1)[1].split(
            "\n}", 1)[0]
        self.assertIn("const int32 Favorite = HitTestRightHandFavorite();", clicks)
        self.assertIn("EquipRightHandFavorite(Favorite);", clicks)
        hit = hero.split("int32 AMoonfallHeroPawn::HitTestRightHandFavorite() const", 1)[1].split(
            "// Native UE5 melee", 1)[0]
        for geometry in ("Left + 18.f", "Top + 119.f", "(PanelWidth - 36.f) / 7.f",
                         "StripY + 22.f", "MouseX - StripX"):
            self.assertIn(geometry, hit)
            self.assertIn(geometry, hud)
        self.assertIn("for (int32 Slot = 0; Slot < 7; ++Slot)", hud)
        self.assertIn("ActiveFavorite == Slot", hud)
        for direction, offset in (("Left", "+ 6"), ("Right", "+ 1")):
            block = hero.split(f"void AMoonfallHeroPawn::Options{direction}()", 1)[1].split(
                "\n}", 1)[0]
            self.assertIn("if (bInventoryOpen) {", block)
            self.assertIn("!bCharacterStatsOpen && !bOptionsOpen", block)
            self.assertIn("EquipRightHandFavorite(", block)
            self.assertIn(offset, block)
        self.assertIn("RIGHT HAND: %s  |  B: choose", hud)
        self.assertNotIn("V / D-PAD LEFT", hud)

    def test_sword_flow_clears_only_after_safe_respawn(self):
        hero = (SRC / "MoonfallHeroPawn.cpp").read_text()
        respawn = hero.split("void AMoonfallHeroPawn::RespawnAtCheckpoint()", 1)[1].split(
            "// One unified roster.", 1)[0]
        self.assertLess(respawn.index("if (!bFoundSafeRespawn) {"),
                        respawn.index("MoonfallOmniattack::Reset(SwordFlow);"))
        for item in ("MoonfallOmniattack::Reset(SwordFlow);",
                     "CurrentSwordAngle = -1;", "SwordFlowStep = 0;",
                     "NextMeleeAttackAt = 0.f;", "HitComboStage = 0;",
                     "LastHitComboTarget.Reset();"):
            self.assertIn(item, respawn)

    def test_character_creator_leaves_movement_arrows_unbound(self):
        creator = (SRC / "MoonfallCharacterCreationWidget.cpp").read_text()
        for key in ("Left", "Right", "Up", "Down"):
            self.assertNotIn("Key == EKeys::" + key, creator)
        self.assertIn("Key == EKeys::PageUp", creator)
        self.assertIn("Key == EKeys::PageDown", creator)
        self.assertIn("EKeys::Gamepad_DPad_Left", creator)
        self.assertIn("EKeys::Gamepad_DPad_Right", creator)

    def test_movement_keys_are_exclusively_movement(self):
        lines = (GAME / "Config" / "DefaultInput.ini").read_text().splitlines()
        reserved = {"W", "A", "S", "D", "Up", "Down", "Left", "Right"}
        for line in lines:
            if not line.startswith("+ActionMappings="):
                continue
            action, _, key = line.partition("Key=")
            self.assertNotIn(key.rstrip(")"), reserved, action)
        for key in reserved:
            self.assertTrue(any(line.startswith("+AxisMappings=")
                                and "Key=" + key + ")" in line for line in lines), key)
        self.assertTrue(any('ActionName="RightHandAction",Key=Gamepad_RightTrigger'
                            in line for line in lines))
        self.assertFalse(any('ToggleRightHandLoadout' in line for line in lines))
        self.assertFalse(any('Key=V)' in line for line in lines))
        self.assertFalse(any("\\n" in line for line in lines))


if __name__ == "__main__":
    unittest.main()
