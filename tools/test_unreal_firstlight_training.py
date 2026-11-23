"""Executable C++17 tutorial/practice policy and UE5 action-integration regressions."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

SRC = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class FirstlightTrainingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.hero_h = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.enemy = (SRC / "MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")

    def test_cplusplus17_policy_behaviour(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if not compiler:
            self.skipTest("C++17 compiler unavailable")
        code = r'''
#include "MoonfallFirstlightTutorial.h"
#include "MoonfallPracticeAI.h"
#include <cassert>
#include <limits>
using namespace MoonfallFirstlightTutorial;
int main() {
 FProgress p;
 assert(p.Count()==0 && p.Next()==EAction::DiscoverShrine && !p.Finished());
 assert(p.Mark(EAction::SwingSword) && !p.Mark(EAction::SwingSword));
 assert(p.Mark(EAction::PerformDodge) && p.Count()==2);
 assert(!p.Mark(EAction::Complete) && !p.Has(EAction::Complete));
 assert(p.Next()==EAction::DiscoverShrine);
 assert(p.Mark(EAction::DiscoverShrine) && p.Mark(EAction::SpeakToGuide));
 assert(p.Mark(EAction::OpenSpellbook));
 assert(p.Finished() && p.Count()==5 && !p.Mark(EAction::OpenSpellbook));
 assert(MoonfallPracticeAI::PatternFor(1)==0);
 assert(MoonfallPracticeAI::PatternFor(2)==2);
 assert(MoonfallPracticeAI::ReadableWindup(.38f,1)>=1.10f);
 assert(MoonfallPracticeAI::ReadableWindup(.65f,2)>=1.35f);
 assert(MoonfallPracticeAI::NonLethalDamage(200.f,5.f)<=1.5f);
 assert(MoonfallPracticeAI::NonLethalDamage(200.f,1.2f)<=.20001f);
 assert(MoonfallPracticeAI::NonLethalDamage(200.f,1.f)==0.f);
 assert(MoonfallPracticeAI::NonLethalDamage(200.f,-1.f)==0.f);
 assert(MoonfallPracticeAI::NonLethalDamage(
     std::numeric_limits<float>::infinity(),2.f)==0.f);
 assert(MoonfallPracticeAI::NonLethalDamage(
     2.f,std::numeric_limits<float>::quiet_NaN())==0.f);
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/"training.cpp"
            exe=Path(d)/"training"
            source.write_text(code,encoding="utf-8")
            subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-Werror",
                "-I",str(SRC),str(source),"-o",str(exe)],
                check=True,text=True,capture_output=True)
            subprocess.run([str(exe)],check=True,text=True,capture_output=True)

    def test_tutorial_only_new_players_near_sanctuary(self):
        self.assertIn("MoonfallFirstlightTutorial::FProgress FirstlightTutorial;",self.hero_h)
        mark=self.hero.split("void AMoonfallHeroPawn::RecordFirstlightAction(",1)[1].split(
            "bool AMoonfallHeroPawn::IsNearStartingSanctuary()",1)[0]
        self.assertIn("bStartingSanctuaryPlaced && IsNearStartingSanctuary()",mark)
        self.assertIn("FirstlightTutorial.Mark(Action);",mark)
        self.assertNotIn("SaveWorldProgress",mark)

    def test_shrine_and_guide_only_accepted_events(self):
        discover=self.hero.split("bool AMoonfallHeroPawn::DiscoverShrine(",1)[1].split(
            "int32 AMoonfallHeroPawn::GetDungeonTreasureState(",1)[0]
        self.assertIn('Shrine->ShrineId == FName(TEXT("sanctuary_firstlight"))',discover)
        self.assertLess(discover.index("if (!SaveWorldProgress()) {"),
                        discover.index("RecordFirstlightAction(MoonfallFirstlightTutorial::EAction::DiscoverShrine);"))
        self.assertEqual(self.hero.count(
            "RecordFirstlightAction(MoonfallFirstlightTutorial::EAction::SpeakToGuide);"),2)
        for opening,closing in (
            ("void AMoonfallHeroPawn::OpenFocusedResidentContext()",
             "void AMoonfallHeroPawn::CastRightMouse()"),
            ("void AMoonfallHeroPawn::CastRightMouse()",
             "void AMoonfallHeroPawn::ReleaseInventoryPointer()")):
            context=self.hero.split(opening,1)[1].split(closing,1)[0]
            self.assertIn('Resident->ResidentId == FName(TEXT("CrossroadsGuide"))',context)
            self.assertLess(context.index("OnResidentContextOpened("),
                            context.index("RecordFirstlightAction("))

    def test_swing_dodge_and_grimoire_are_actual_actions(self):
        melee=self.hero.split("void AMoonfallHeroPawn::MeleeAttack()",1)[1].split(
            "void AMoonfallHeroPawn::BreakGuard()",1)[0]
        self.assertLess(melee.index("Stamina = FMath::Clamp(Stamina - Cost"),
                        melee.index("RecordFirstlightAction(MoonfallFirstlightTutorial::EAction::SwingSword);"))
        dodge=self.hero.split("void AMoonfallHeroPawn::Dodge()",1)[1].split(
            "void AMoonfallHeroPawn::RewardPerfectDodge()",1)[0]
        self.assertLess(dodge.index("LaunchCharacter("),
                        dodge.index("RecordFirstlightAction(MoonfallFirstlightTutorial::EAction::PerformDodge);"))
        book=self.hero.split("void AMoonfallHeroPawn::ToggleSpellBook()",1)[1].split(
            "bool AMoonfallHeroPawn::EquipSpellBookSelection(",1)[0]
        self.assertIn("if (bSpellBookOpen) {",book)
        self.assertIn("RecordFirstlightAction(MoonfallFirstlightTutorial::EAction::OpenSpellbook);",book)

    def test_hud_never_blocks_quests_or_requires_tutorial(self):
        tracker=self.hud.split("// World-wide quest tracker,",1)[1].split(
            "// Compact circular resource HUD",1)[0]
        for phrase in ("Hero->FirstlightTutorial.Next()","Hero->FirstlightTutorial.Count()",
                       "Hero->bStartingSanctuaryPlaced","OPTIONAL:",
                       "FIRST STEPS %u/5", "right-click / D-pad Up",
                       "left-click / RT","Open your Spell Book (P)",
                       "FOUND %d/30  |  BOSSES %d/30  |  REGIONS %d/6"):
            self.assertIn(phrase,tracker)
        for costly in ("TActorIterator","OpenLevel","GetAllActorsOfClass"):
            self.assertNotIn(costly,tracker)

    def test_training_ai_limits_and_no_extra_world_scans(self):
        e=self.enemy
        self.assertIn('#include "MoonfallPracticeAI.h"',e)
        self.assertIn('const bool bSanctuaryPractice = ActorHasTag(FName(TEXT("MoonfallSanctuaryPractice")));',e)
        far=e.split("if (bSanctuaryPractice && Distance > 450.f) {",1)[1].split(
            "// Far-away enemies use a slower think rate",1)[0]
        for required in ("LockedAttackDirection = FVector::ZeroVector;",
                         "bCanSeeHero = false;","AttackTellMarker->SetVisibility(false);",
                         "MoonfallPracticeAI::ReapproachDelay",
                         "MoonfallPracticeAI::IdleTickSeconds","return;"):
            self.assertIn(required,far)
        self.assertNotIn("TActorIterator",far)
        selected=e.split("const int32 PatternCount =",1)[1].split(
            "WindupEndsAt = Now + CurrentWindup;",1)[0]
        for value in ("MoonfallPracticeAI::PatternFor(AttackSequence)",
                      "MoonfallPracticeAI::ReadableWindup(CurrentWindup, AttackSequence)",
                      "CurrentAttackMultiplier = FMath::Min(CurrentAttackMultiplier, 1.f)"):
            self.assertIn(value,selected)

    def test_nonlethal_training_keeps_existing_damage_and_parry_path(self):
        strike=self.enemy
        part=strike.split("const float IncomingAttackDamage = AttackDamage *",1)[1].split(
            "const bool bPerfectDodge =",1)[0]
        self.assertIn("const float EffectiveAttackDamage = bSanctuaryPractice",part)
        self.assertIn("MoonfallPracticeAI::NonLethalDamage(IncomingAttackDamage, Hero->Health)",part)
        self.assertIn(": IncomingAttackDamage;",part)
        self.assertIn("Hero->TryRewardPerfectDodge();",strike)
        self.assertIn("Hero->ApplyMoonfallDamageFrom(EffectiveAttackDamage, GetActorLocation())",strike)
        self.assertIn("Hero->ApplyMoonfallDodgeOnlyDamageFrom(EffectiveAttackDamage, GetActorLocation())",strike)


if __name__=="__main__":
    unittest.main()
