"""One-slot sword input buffering with executable C++17 timing and source integration checks."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"
HERO = SOURCE / "MoonfallHeroPawn.cpp"
ENEMY = SOURCE / "MoonfallEnemy.cpp"


class CombatInputBufferTests(unittest.TestCase):
    def test_cxx_policy_compiles_and_exercises_timing_and_cancellation(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if not compiler:
            self.skipTest("C++17 compiler unavailable")
        cpp = r"""
#include "MoonfallCombatInputBuffer.h"
#include <limits>
int main() {
 using MoonfallCombatInputBuffer::FSingleSwing;
 FSingleSwing queue;
 if (queue.Consume(1.f,1.f,true)) return 1;
 if (queue.Queue(0.70f,1.f)) return 2; // Too early for intentional buffer.
 if (!queue.Queue(0.85f,1.f)) return 3;
 if (queue.Consume(0.99f,1.f,true)) return 4; // Not recovered yet.
 if (!queue.Consume(1.01f,1.f,true)) return 5;
 if (queue.Consume(1.02f,1.f,true)) return 6; // Exactly one swing.
 if (!queue.Queue(1.85f,2.f)) return 7;
 if (!queue.Queue(1.90f,2.f)) return 8; // Still one slot, no stacking.
 if (!queue.Consume(2.01f,2.f,true)) return 9;
 if (queue.Consume(2.02f,2.f,true)) return 10;
 if (!queue.Queue(2.85f,3.f)) return 11;
 if (queue.Consume(3.13f,3.f,true)) return 12; // Expired grace.
 if (!queue.Queue(3.85f,4.f)) return 13;
 if (queue.Consume(3.90f,4.f,false)) return 14; // Opening a modal cancels.
 if (queue.Consume(4.01f,4.f,true)) return 15;
 if (queue.Queue(5.f,5.f)) return 16; // No attack was awaiting recovery.
 if (!queue.Queue(5.85f,6.f)) return 17;
 queue.Clear(); // Dodge, parry, hand switch, death or map opening.
 if (queue.Consume(6.01f,6.f,true)) return 18;
 const float nan=std::numeric_limits<float>::quiet_NaN();
 const float inf=std::numeric_limits<float>::infinity();
 if (queue.Queue(nan,7.f) || queue.Queue(6.85f,inf)) return 19;
 queue.ExpiresAt=nan;
 if (queue.Consume(7.1f,7.f,true)) return 20;
 if (queue.ExpiresAt!=-1.f) return 21;
 if (queue.Queue(7.f,7.2f)) return 22; // Only the final 180ms.
 if (!queue.Queue(8.85f,9.f)) return 23;
 if (queue.Consume(8.80f,9.f,true)) return 24; // Clock rolled behind press.
 if (queue.ExpiresAt!=-1.f || queue.QueuedAt!=-1.f) return 25;
 if (queue.Consume(9.01f,9.f,true)) return 26; // No stale replay.
 if (!queue.Queue(9.85f,10.f)) return 27;
 queue.ExpiresAt=-inf;
 if (queue.Consume(10.f,10.f,true)) return 28;
 if (queue.ExpiresAt!=-1.f || queue.QueuedAt!=-1.f) return 29;
 if (queue.Queue(-0.1f,0.f)) return 30;
 if (!queue.Queue(10.85f,11.f)) return 31;
 if (!queue.Consume(11.f,11.f,true)) return 32;
 if (queue.ExpiresAt!=-1.f || queue.QueuedAt!=-1.f) return 33;
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "input_buffer.cpp"
            binary = Path(tmp) / "input_buffer"
            source.write_text(cpp, encoding="utf-8")
            subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra",
                            "-Werror", "-I", str(SOURCE), str(source),
                            "-o", str(binary)], check=True,
                           capture_output=True, text=True)
            subprocess.run([str(binary)], check=True,
                           capture_output=True, text=True)

    def test_input_routes_only_sword_presses_into_bounded_buffer(self):
        hero = HERO.read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        action = hero.split("void AMoonfallHeroPawn::RightHandAction()",1)[1].split(
            "bool AMoonfallHeroPawn::ReadyRightHandSpell",1)[0]
        self.assertIn("MoonfallCombatInputBuffer::FSingleSwing SwordBuffer;", header)
        self.assertIn("SwordBuffer.Queue(CombatNow, NextMeleeAttackAt)", action)
        self.assertIn("!bRightHandSpellReadied && !bGuarding", action)
        self.assertLess(action.index("SwordBuffer.Queue("),
                        action.index("if (bRightHandSpellReadied) CastRight();"))
        self.assertIn("else MeleeAttack();", action)
        self.assertIn("SwordBuffer.Clear();", action)
        tick = hero.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)",1)[1].split(
            " // Keep one save slot forever;",1)[0]
        self.assertIn("SwordBuffer.Consume(CombatNow, NextMeleeAttackAt, bCanDispatchBufferedMelee)",tick)
        self.assertIn("MeleeAttack();",tick)
        for guard in ("!bOptionsOpen", "!bInventoryOpen", "!bAtlasOpen",
                      "!bResidentContextOpen", "!FocusedResident.IsValid()",
                      "!bGuarding", "!bRightHandSpellReadied", "Health > 0.f",
                      "FMath::IsFinite(Health)"):
            self.assertIn(guard,tick)
        melee = hero.split("void AMoonfallHeroPawn::MeleeAttack()",1)[1].split(
            "void AMoonfallHeroPawn::BreakGuard()",1)[0]
        self.assertLess(melee.index("SwordBuffer.Clear();"),
                        melee.index("NextMeleeAttackAt = Now +"))
        self.assertEqual(tick.count("SwordBuffer.Consume("),1)

    def test_explicit_spells_supersede_buffered_melee(self):
        hero = HERO.read_text(encoding="utf-8")
        for name, following, cast in (
            ("CastLeft", "CastRight", "TryCast(LeftSpell, 0, 1.f)"),
            ("CastRight", "CastBoth", "TryCast(RightSpell, 1, 1.f)"),
            ("CastBoth", "RightHandAction", "TryCast(LeftSpell, 0, 2.f)"),
        ):
            with self.subTest(action=name):
                body = hero.split(
                    f"void AMoonfallHeroPawn::{name}()", 1)[1].split(
                    f"void AMoonfallHeroPawn::{following}()", 1)[0]
                self.assertIn("SwordBuffer.Clear();", body)
                self.assertLess(body.index("SwordBuffer.Clear();"),
                                body.index(cast))
                self.assertLess(body.index("if (bOptionsOpen || bAtlasOpen) return;"),
                                body.index("SwordBuffer.Clear();"))
        dual = hero.split("void AMoonfallHeroPawn::CastBoth()", 1)[1].split(
            "void AMoonfallHeroPawn::RightHandAction()", 1)[0]
        self.assertLess(dual.index("SwordBuffer.Clear();"),
                        dual.index("if (LeftSpell != RightSpell || !GetWorld()) return;"))
        action = hero.split("void AMoonfallHeroPawn::RightHandAction()",1)[1].split(
            "bool AMoonfallHeroPawn::ReadyRightHandSpell",1)[0]
        self.assertIn("if (bRightHandSpellReadied) CastRight();", action)

    def test_defense_menus_respawn_and_hand_change_cancel_queue(self):
        hero = HERO.read_text(encoding="utf-8")
        methods = (
            ("void AMoonfallHeroPawn::BeginGuard()", "void AMoonfallHeroPawn::EndGuard()"),
            ("void AMoonfallHeroPawn::Dodge()", "void AMoonfallHeroPawn::RewardPerfectDodge()"),
            ("void AMoonfallHeroPawn::ToggleInventory()", "void AMoonfallHeroPawn::EquipSelectedInventoryItem()"),
            ("void AMoonfallHeroPawn::OpenOptions()", "void AMoonfallHeroPawn::OptionsUp()"),
            ("void AMoonfallHeroPawn::ToggleAtlas()", "void AMoonfallHeroPawn::"),
            ("bool AMoonfallHeroPawn::ReadyRightHandSpell(", "void AMoonfallHeroPawn::ReadyRightHandWeapon()"),
            ("void AMoonfallHeroPawn::OpenFocusedResidentContext()", "void AMoonfallHeroPawn::CastRightMouse()"),
            ("void AMoonfallHeroPawn::CloseResidentContext()", "void AMoonfallHeroPawn::OpenFocusedResidentContext()"),
        )
        for start,end in methods:
            with self.subTest(start=start):
                self.assertIn(start,hero)
                block=hero.split(start,1)[1].split(end,1)[0]
                self.assertIn("SwordBuffer.Clear();",block)
        respawn=hero.split("void AMoonfallHeroPawn::RespawnAtCheckpoint()",1)[1]
        self.assertIn("SwordBuffer.Clear();\n MoonfallOmniattack::Reset(SwordFlow);",respawn)

    def test_mouse_npc_dialogue_cancels_melee_and_stale_focus_recovers(self):
        hero = HERO.read_text(encoding="utf-8")
        # Both mouse and controller paths must cancel a previously queued
        # sword press at the moment a real NPC conversation begins.
        mouse = hero.split("void AMoonfallHeroPawn::CastRightMouse()",1)[1].split(
            "void AMoonfallHeroPawn::ReleaseInventoryPointer()",1)[0]
        open_dialogue = mouse.split("if (Resident->CanHeroInteract(this)) {",1)[1].split(
            "OnResidentContextOpened(",1)[0]
        self.assertIn("SwordBuffer.Clear();", open_dialogue)
        self.assertLess(open_dialogue.index("SwordBuffer.Clear();"),
                        open_dialogue.index("FocusedResident = Resident;"))
        controller = hero.split(
            "void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[1].split(
            "void AMoonfallHeroPawn::CastRightMouse()",1)[0]
        self.assertIn("SwordBuffer.Clear();", controller)
        # A failed or incomplete modal transition must not soft-lock input
        # or leave an enemy permanently paused against a stale focus.
        tick = hero.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)",1)[1].split(
            " // A second click shortly before recovery",1)[0]
        self.assertIn("if (!bResidentContextOpen && FocusedResident.IsValid())",tick)
        self.assertIn("CloseResidentContext();",tick)
        self.assertLess(tick.index("if (!bResidentContextOpen && FocusedResident.IsValid())"),
                        tick.index("if (bResidentContextOpen) {"))
        close = hero.split("void AMoonfallHeroPawn::CloseResidentContext()",1)[1].split(
            "void AMoonfallHeroPawn::OpenFocusedResidentContext()",1)[0]
        self.assertIn("FocusedResident.Reset();", close)
        self.assertIn("SwordBuffer.Clear();", close)
        self.assertIn("PC->bShowMouseCursor = bOptionsOpen || bInventoryOpen || bAtlasOpen;",close)

    def test_enemy_attacks_continue_during_dialogue_but_stop_on_defeat(self):
        enemy=ENEMY.read_text(encoding="utf-8")
        tick=enemy.split("void AMoonfallEnemy::Tick(float DeltaSeconds)",1)[1]
        guard=tick.split("// The hostile world stays live while any player menu is open:",1)[1].split(
            "// A corrupt nonfinite windup",1)[0]
        for forbidden in ("Hero->bOptionsOpen", "Hero->bInventoryOpen",
                          "Hero->bAtlasOpen", "Hero->bCharacterStatsOpen",
                          "Hero->bResidentContextOpen",
                          "Hero->FocusedResident.IsValid()",
                          "Hero->bTalentWebOpen"):
            self.assertNotIn(forbidden,guard)
        self.assertIn("!Hero->IsPlayerControlled()",guard)
        self.assertIn("Hero->Health <= 0.f",guard)
        for reset in ("CancelDivineSignature();", "WindupEndsAt = -1.f;",
                      "BossWarningEndTime = -1.f;",
                      "bComboSecondHitPending = false;",
                      "bComboSecondHitActive = false;",
                      "LockedAttackDirection = FVector::ZeroVector;"):
            self.assertIn(reset,guard)
        self.assertLess(guard.index("Hero->Health <= 0.f"),
                        guard.index("CancelDivineSignature();"))


if __name__ == "__main__":
    unittest.main()
