"""UE5 Spell Book: executable engine-free layout checks and gameplay contracts."""
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
HERO = SRC / "MoonfallHeroPawn.cpp"
HUD = SRC / "MoonfallCombatHUD.cpp"
HEADER = SRC / "MoonfallHeroPawn.h"


class UnrealSpellBookTests(unittest.TestCase):
    def test_catalog_and_pointer_geometry_compile_and_run(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if compiler is None:
            self.skipTest("C++17 compiler unavailable")
        code = r"""
#include "MoonfallSpellBook.h"
#include <cmath>
#include <cstring>
int main() {
 using namespace MoonfallSpellBook;
 static_assert(SpellCount==6, "six existing spell schools");
 static_assert(Spells[3].bHealing, "school 3 is restoration");
 static_assert(!Spells[0].bHealing && !Spells[5].bHealing, "no free offensive heals");
 const float expectedCost[] = {16.f,13.f,22.f,25.f,18.f,14.f};
 const float expectedCooldown[] = {0.8f,0.65f,1.2f,2.f,1.f,0.9f};
 for (int i=0; i<6; ++i) {
  if (!IsValid(i) || Spells[i].ManaCost != expectedCost[i] ||
      Spells[i].Cooldown != expectedCooldown[i] ||
      !Spells[i].Name || !Spells[i].Name[0] ||
      !Spells[i].Description || !Spells[i].Description[0]) return 1;
 }
 if (IsValid(-1) || IsValid(6)) return 2;
 for (const auto pair : {320,480,760,1280,1920}) {
  const auto g = Layout(static_cast<float>(pair), 720.f);
  if (Hit(g,g.TabX+3.f,g.TabY+3.f,false)!=Tab) return 3;
  if (Hit(g,g.RowX+4.f,g.RowY+4.f,false)!=None) return 4;
  for (int i=0; i<SpellCount; ++i) {
   const float y=g.RowY+i*g.RowGap+g.RowHeight*0.5f;
   if (Hit(g,g.RowX+3.f,y,true)!=i) return 5;
  }
  if (Hit(g,g.EquipLeftX+3.f,g.EquipY+3.f,true)!=EquipLeft) return 6;
  if (Hit(g,g.EquipRightX+3.f,g.EquipY+3.f,true)!=EquipRight) return 7;
  if (Hit(g,g.Left-100.f,g.Top-100.f,true)!=None) return 8;
  if (g.EquipLeftX+g.EquipWidth>=g.EquipRightX) return 9;
 }
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as temp:
            cpp = pathlib.Path(temp) / "spellbook.cpp"
            exe = pathlib.Path(temp) / "spellbook"
            cpp.write_text(code)
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                            "-Werror", "-I", str(SRC), str(cpp), "-o", str(exe)],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(exe)], check=True, capture_output=True, text=True)

    def test_catalog_agrees_with_live_spell_costs_and_cooldowns(self):
        book = (SRC / "MoonfallSpellBook.h").read_text()
        hero = HERO.read_text()
        cast = hero.split("bool AMoonfallHeroPawn::TryCast(", 1)[1].split(
            "void AMoonfallHeroPawn::CastLeft()", 1)[0]
        costs = re.search(r"static const float Costs\[6\] = \{([^}]+)\};", cast)
        cooldowns = re.search(r"static const float Cooldowns\[6\] = \{([^}]+)\};", cast)
        self.assertIsNotNone(costs)
        self.assertIsNotNone(cooldowns)
        for numeric_list, expected in (
            (costs, [16,13,22,25,18,14]),
            (cooldowns, [0.8,0.65,1.2,2,1,0.9]),
        ):
            actual = [float(part.strip().removesuffix("f"))
                      for part in numeric_list.group(1).split(",")]
            self.assertEqual(actual, expected)
        self.assertIn('{"Mending Light", "Restoration",', book)
        self.assertIn("static_assert(sizeof(Spells)", book)

    def test_inventory_only_book_and_safe_two_hand_equip(self):
        hero = HERO.read_text()
        header = HEADER.read_text()
        toggle = hero.split("void AMoonfallHeroPawn::ToggleSpellBook()", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipSpellBookSelection", 1)[0]
        equip = hero.split("bool AMoonfallHeroPawn::EquipSpellBookSelection(bool bLeftHand)", 1)[1].split(
            "bool AMoonfallHeroPawn::HandleSpellBookPointer", 1)[0]
        for guard in ("!bInventoryOpen", "bOptionsOpen", "bAtlasOpen",
                      "bResidentContextOpen", "Health <= 0.f"):
            self.assertIn(guard, toggle)
            self.assertIn(guard, equip)
        self.assertIn("bSpellBookOpen = !bSpellBookOpen;", toggle)
        self.assertIn("bCharacterStatsOpen = false;", toggle)
        self.assertIn("LeftSpell = SpellBookSelection;", equip)
        self.assertIn("return ReadyRightHandSpell(SpellBookSelection);", equip)
        self.assertIn("MoonfallSpellBook::IsValid(SpellBookSelection)", equip)
        self.assertIn("bool bSpellBookOpen = false;", header)
        self.assertIn("int32 SpellBookSelection = 0;", header)
        self.assertIn("if (bInventoryOpen) ToggleSpellBook(); else ActivateRacePower();", header)
        self.assertIn("bSpellBookOpen = false;", hero)
        self.assertNotIn('ActionName="SpellBook"', (
            ROOT / "MoonfallUnreal" / "Config" / "DefaultInput.ini").read_text())

    def test_pointer_priority_and_modal_input_safety(self):
        hero = HERO.read_text()
        pointer = hero.split("bool AMoonfallHeroPawn::HandleSpellBookPointer()", 1)[1].split(
            "// Equip from the existing inventory favorites strip", 1)[0]
        self.assertIn("MoonfallSpellBook::Layout(", pointer)
        self.assertIn("MoonfallSpellBook::Hit(", pointer)
        self.assertIn("if (Hit == MoonfallSpellBook::Tab) ToggleSpellBook();", pointer)
        self.assertIn("SpellBookSelection = Hit;", pointer)
        self.assertIn("EquipSpellBookSelection(true)", pointer)
        self.assertIn("EquipSpellBookSelection(false)", pointer)
        left_mouse = hero.split("void AMoonfallHeroPawn::ResidentContextMouseClick()", 1)[1].split(
            "\n}", 1)[0]
        right_mouse = hero.split("void AMoonfallHeroPawn::CastRightMouse()", 1)[1].split(
            "\n}", 1)[0]
        self.assertLess(left_mouse.index("HandleSpellBookPointer() || bSpellBookOpen"),
                        left_mouse.index("HitTestInventorySortButton()"))
        self.assertLess(right_mouse.index("HandleSpellBookPointer() || bSpellBookOpen"),
                        right_mouse.index("HitTestInventorySortButton()"))
        for direction, expected in (("Up", "SpellBookSelection - 1"),
                                    ("Down", "SpellBookSelection + 1")):
            fn = hero.split(f"void AMoonfallHeroPawn::Options{direction}()", 1)[1].split("\n}",1)[0]
            self.assertIn("if (bSpellBookOpen)", fn)
            self.assertIn(expected, fn)
        for direction, expected in (("Left", "EquipSpellBookSelection(true)"),
                                    ("Right", "EquipSpellBookSelection(false)")):
            fn = hero.split(f"void AMoonfallHeroPawn::Options{direction}()", 1)[1].split("\n}",1)[0]
            self.assertIn("if (bInventoryOpen && bSpellBookOpen)", fn)
            self.assertIn(expected, fn)
        self.assertIn("if (bSpellBookOpen) { bSpellBookOpen = false; return; }", hero)

    def test_book_is_drawn_using_shared_geometry_with_actual_spell_details(self):
        book = (SRC / "MoonfallSpellBook.h").read_text()
        hud = HUD.read_text()
        self.assertIn("MoonfallSpellBook::Layout(Canvas->SizeX, Canvas->SizeY)", hud)
        self.assertIn("MoonfallSpellBook::Spells[School]", hud)
        self.assertIn("MoonfallSpellBook::Spells[SelectedSpell]", hud)
        self.assertIn("Book.RowY + School*Book.RowGap", hud)
        self.assertIn("Book.EquipLeftX, Book.EquipY", hud)
        self.assertIn("Book.EquipRightX, Book.EquipY", hud)
        self.assertIn("Book.TabX, Book.TabY", hud)
        self.assertIn('TEXT("SPELL BOOK")', hud)
        self.assertIn('TEXT("EQUIP LEFT")', hud)
        self.assertIn('TEXT("EQUIP RIGHT")', hud)
        self.assertIn("Entry.ManaCost", hud)
        self.assertIn("Entry.Cooldown", hud)
        self.assertIn("bLeft ? TEXT(\"[L]\")", hud)
        self.assertIn("bRight ? TEXT(\"[R]\")", hud)
        self.assertIn("inline int Hit(", book)


if __name__ == "__main__":
    unittest.main()
