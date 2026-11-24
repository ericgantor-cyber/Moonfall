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
#include <initializer_list>
#include <limits>
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
  const auto bag = Layout(static_cast<float>(pair),720.f,false);
  const auto g = Layout(static_cast<float>(pair),720.f,true);
  if (Hit(bag,bag.TabX+3.f,bag.TabY+3.f,false)!=Tab) return 3;
  if (Hit(bag,bag.RowX+4.f,bag.RowY+4.f,false)!=None) return 4;
  if (g.Width<bag.Width) return 5; // Opening expands the reading surface.
  if (Hit(g,g.TabX+3.f,g.TabY+3.f,true)!=Tab) return 6;
  for (int i=0; i<SpellCount; ++i) {
   const float y=g.RowY+i*g.RowGap+g.RowHeight*0.5f;
   if (Hit(g,g.RowX+3.f,y,true)!=i) return 7;
  }
  if (Hit(g,g.EquipLeftX+3.f,g.EquipY+3.f,true)!=EquipLeft) return 8;
  if (Hit(g,g.EquipRightX+3.f,g.EquipY+3.f,true)!=EquipRight) return 9;
  if (Hit(g,g.Left-100.f,g.Top-100.f,true)!=None) return 10;
  if (g.EquipLeftX+g.EquipWidth>=g.EquipRightX) return 11;
 }
 const auto ready = Evaluate(0,100.f,100.f,70.f,100.f,1.f,5.f,-100.f,0.f);
 if (ready.State!=EReadiness::Ready || ready.CooldownRemaining!=0.f) return 12;
 const auto cooldown = Evaluate(0,100.f,100.f,70.f,100.f,1.f,0.2f,0.f,0.f);
 if (cooldown.State!=EReadiness::Cooldown ||
     std::fabs(cooldown.CooldownRemaining-0.6f)>0.001f) return 13;
 const auto withHaste = Evaluate(0,100.f,100.f,70.f,100.f,1.f,0.6f,0.f,1.f);
 if (withHaste.State!=EReadiness::Ready) return 14;
 const auto noMana = Evaluate(0,5.f,100.f,70.f,100.f,1.f,5.f,-100.f,0.f);
 if (noMana.State!=EReadiness::LowMana || noMana.ManaShortfall!=11.f) return 15;
 const auto full = Evaluate(3,100.f,100.f,100.f,100.f,1.f,5.f,-100.f,0.f);
 if (full.State!=EReadiness::FullHealth) return 16;
 const auto canHeal = Evaluate(3,100.f,100.f,50.f,100.f,1.f,5.f,-100.f,0.f);
 if (canHeal.State!=EReadiness::Ready) return 17;
 const auto invalidSchool = Evaluate(6,100.f,100.f,70.f,100.f,1.f,5.f,-100.f,0.f);
 if (invalidSchool.State!=EReadiness::Invalid) return 18;
 const auto invalidMana = Evaluate(1,std::numeric_limits<float>::quiet_NaN(),
   100.f,70.f,100.f,1.f,5.f,-100.f,0.f);
 if (invalidMana.State!=EReadiness::Invalid) return 19;
 const auto invalidHaste = Evaluate(1,100.f,100.f,70.f,100.f,1.f,5.f,-100.f,-1.f);
 if (invalidHaste.State!=EReadiness::Invalid) return 20;
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

    def test_cast_state_policy_is_read_only_and_uses_real_per_hand_timers(self):
        book = (SRC / "MoonfallSpellBook.h").read_text()
        hero = HERO.read_text()
        header = HEADER.read_text()
        method = hero.split("MoonfallSpellBook::FCastState AMoonfallHeroPawn::GetSpellBookCastState(",1)[1].split(
            "bool AMoonfallHeroPawn::HandleSpellBookPointer()",1)[0]
        self.assertIn("GetSpellBookCastState(int32 School, bool bLeftHand) const;",header)
        self.assertIn("LastCastTime[bLeftHand ? 0 : 1]",method)
        self.assertIn("GetHasteBonus()",method)
        self.assertIn("SpellPowerMultiplier",method)
        self.assertIn("GetWorld()->GetTimeSeconds()",method)
        self.assertIn("EReadiness::Cooldown",book)
        self.assertIn("EReadiness::FullHealth",book)
        self.assertIn("EReadiness::LowMana",book)
        self.assertIn("EReadiness::Ready",book)
        self.assertNotIn("LineTrace",method)
        self.assertNotIn("TryCast(",method)
        self.assertNotIn("Mana -=",method)

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
        self.assertIn('ActionName="SpellBook",Key=P', (
            ROOT / "MoonfallUnreal" / "Config" / "DefaultInput.ini").read_text())

    def test_p_opens_spellbook_directly_and_closes_it_without_rebinding_movement(self):
        config = (ROOT / "MoonfallUnreal" / "Config" / "DefaultInput.ini").read_text()
        hero = HERO.read_text()
        header = HEADER.read_text()
        hud = HUD.read_text()
        self.assertEqual(config.count('ActionName="SpellBook",Key=P'), 1)
        self.assertNotIn('ActionName="CharacterStats",Key=P', config)
        self.assertIn('ActionName="CharacterStats",Key=C', config)
        self.assertIn('BindAction(TEXT("SpellBook"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleSpellBookShortcut)', hero)
        self.assertIn("void ToggleSpellBookShortcut();", header)
        self.assertIn("void AMoonfallHeroPawn::ToggleSpellBookShortcut()", hero)
        shortcut = hero.split("void AMoonfallHeroPawn::ToggleSpellBookShortcut()", 1)[1].split(
            "void AMoonfallHeroPawn::ToggleSpellBook()", 1)[0]
        for guard in ("bOptionsOpen", "bAtlasOpen", "bResidentContextOpen",
                      "FocusedResident.IsValid()", "Health <= 0.f",
                      "!FMath::IsFinite(Health)"):
            self.assertIn(guard, shortcut)
        # P must never dismiss the atlas via ToggleInventory's atlas-close path.
        self.assertLess(shortcut.index("bAtlasOpen"),
                        shortcut.index("ToggleInventory();"))
        self.assertIn("if (!bInventoryOpen) {", shortcut)
        self.assertIn("ToggleInventory();", shortcut)
        self.assertIn("if (bInventoryOpen) ToggleSpellBook();", shortcut)
        self.assertIn("if (bSpellBookOpen) {", shortcut)
        self.assertIn("ToggleInventory(); // Also clears the spell-book state", shortcut)
        self.assertIn("ToggleSpellBook();", shortcut)
        self.assertIn("P: spell book", hud)
        self.assertIn("P: CLOSE", hud)
        self.assertNotIn("C / P / A:", hud)
        for movement in ("W", "A", "S", "D", "Up", "Down", "Left", "Right"):
            self.assertTrue(any(line.startswith("+AxisMappings=") and "Key=" + movement + ")" in line
                                for line in config.splitlines()), movement)
            self.assertFalse(any(line.startswith("+ActionMappings=") and "Key=" + movement + ")" in line
                                 for line in config.splitlines()), movement)

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
                        left_mouse.index("HitTestInventoryCloseButton()"))
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
        self.assertIn("if (HandleSpellBookPointer() || bSpellBookOpen) return;", left_mouse)
        self.assertIn("if (HandleSpellBookPointer() || bSpellBookOpen) return;", right_mouse)

    def test_book_is_drawn_using_shared_geometry_with_actual_spell_details(self):
        book = (SRC / "MoonfallSpellBook.h").read_text()
        hud = HUD.read_text()
        self.assertIn("MoonfallSpellBook::Layout(Canvas->SizeX, Canvas->SizeY, Hero->bSpellBookOpen)", hud)
        self.assertIn("0.f,0.f,Canvas->SizeX,Canvas->SizeY", hud)
        self.assertIn("THE GRIMOIRE", hud)
        self.assertIn("GetSpellBookCastState(SelectedSpell,true)", hud)
        self.assertIn("GetSpellBookCastState(SelectedSpell,false)", hud)
        self.assertIn('TEXT("READY TO HEAL")', hud)
        self.assertIn('TEXT("READY TO AIM")', hud)
        self.assertIn('TEXT("FULL HEALTH")', hud)
        self.assertIn('TEXT("COOLDOWN %.1fs")', hud)
        self.assertIn('TEXT("NEED %.0f MANA")', hud)
        self.assertIn("MoonfallSpellBook::Spells[School]", hud)
        self.assertIn("MoonfallSpellBook::Spells[SelectedSpell]", hud)
        self.assertIn("Book.RowY + School*Book.RowGap", hud)
        self.assertIn("Book.EquipLeftX,Book.EquipY", hud)
        self.assertIn("Book.EquipRightX,Book.EquipY", hud)
        self.assertIn("Book.TabX, Book.TabY", hud)
        self.assertIn('TEXT("SPELL BOOK")', hud)
        self.assertIn('TEXT("EQUIP LEFT")', hud)
        self.assertIn('TEXT("EQUIP RIGHT")', hud)
        self.assertIn("Entry.ManaCost", hud)
        self.assertIn("Entry.Cooldown", hud)
        self.assertIn("bLeft ? TEXT(\"[L]\")", hud)
        self.assertIn("bRight ? TEXT(\"[R]\")", hud)
        self.assertIn("inline int Hit(", book)


    def test_inventory_mutations_reject_nonfinite_health(self):
        # A corrupted Blueprint health value (NaN or infinity) must not
        # bypass the alive-player guard and mutate inventory or salvage.
        hero = HERO.read_text()
        signatures = (
            ("bool", "SalvageInventoryItem"),
            ("bool", "UpgradeInventoryItem"),
            ("bool", "ReforgeInventoryItemAffix"),
            ("bool", "AscendInventoryItemRarity"),
            ("int32", "ForgeInventoryItemLevels"),
            ("bool", "EquipInventoryItem"),
        )
        for return_type, method in signatures:
            with self.subTest(method=method):
                start = f"{return_type} AMoonfallHeroPawn::{method}("
                body = hero.split(start, 1)[1].split("\n}", 1)[0]
                before_item_access = body.split("InventoryItemCodes[InventoryIndex]", 1)[0]
                self.assertIn("Health <= 0.f", before_item_access)
                self.assertIn("!FMath::IsFinite(Health)", before_item_access)

    def test_direct_equip_and_batch_forge_refuse_non_bag_pages(self):
        hero = HERO.read_text()
        for signature, end in (
            ("bool AMoonfallHeroPawn::EquipInventoryItem(", "namespace {"),
            ("int32 AMoonfallHeroPawn::ForgeInventoryItemLevels(",
             "void AMoonfallHeroPawn::ForgeFiveSelectedInventoryItem()"),
        ):
            with self.subTest(signature=signature):
                body = hero.split(signature, 1)[1].split(end, 1)[0]
                self.assertIn("!bInventoryOpen || bCharacterStatsOpen || bSpellBookOpen", body)
                self.assertIn("|| bOptionsOpen", body)
                self.assertLess(body.index("bSpellBookOpen"),
                                body.index("InventoryItemCodes.IsValidIndex"))

    def test_direct_salvage_enforces_modal_ownership(self):
        hero = HERO.read_text()
        salvage = hero.split("bool AMoonfallHeroPawn::SalvageInventoryItem(", 1)[1].split(
            "bool AMoonfallHeroPawn::UpgradeInventoryItem(", 1)[0]
        self.assertIn("!bInventoryOpen || bCharacterStatsOpen || bSpellBookOpen", salvage)
        self.assertIn("|| bAtlasOpen || bResidentContextOpen || Health <= 0.f", salvage)
        self.assertLess(salvage.index("bSpellBookOpen"), salvage.index("InventoryItemCodes.RemoveAt"))
        self.assertLess(salvage.index("bSpellBookOpen"), salvage.index("Salvage ="))

    def test_crafting_mutations_reject_other_inventory_pages(self):
        # Direct C++/Blueprint callers must obey the same modal safety rules
        # as input wrappers, without spending salvage while another page owns UI.
        hero = HERO.read_text()
        for method in ("UpgradeInventoryItem", "ReforgeInventoryItemAffix",
                       "AscendInventoryItemRarity"):
            with self.subTest(method=method):
                body = hero.split("bool AMoonfallHeroPawn::" + method + "(", 1)[1].split("\n}", 1)[0]
                self.assertIn("!bInventoryOpen || bCharacterStatsOpen || bSpellBookOpen", body)
                self.assertIn("|| bOptionsOpen || Health <= 0.f", body)
                self.assertLess(body.index("bSpellBookOpen"), body.index("InventoryItemCodes[InventoryIndex]"))

if __name__ == "__main__":
    unittest.main()
