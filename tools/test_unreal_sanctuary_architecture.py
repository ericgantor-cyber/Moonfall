"""UE5 Shattered Sanctuary environment: bounded layout, route and source contracts."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class SanctuaryArchitectureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.arena = (SRC / "MoonfallArena.cpp").read_text(encoding="utf-8")
        cls.header = (SRC / "MoonfallArena.h").read_text(encoding="utf-8")

    def test_engine_free_gothic_layout_compiles_and_preserves_path(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if not compiler:
            self.skipTest("C++17 compiler not installed")
        cpp = r'''
#include "MoonfallSanctuaryArchitecture.h"
#include <cmath>
using namespace MoonfallSanctuaryArchitecture;
int main() {
 if (GothicCount < 30 || RelicCount < 18) return 1;
 if (GothicCount + RelicCount > MaxTotalPieces) return 2;
 for (const FPiece& p : GothicStone) {
  if (!ClearsMainPath(p) || !std::isfinite(p.X) || !std::isfinite(p.Y)
      || !std::isfinite(p.Z) || p.Y < 2400.f || p.Y > 4600.f) return 3;
 }
 for (const FPiece& p : MoonstoneRelics) {
  if (!ClearsMainPath(p) || !std::isfinite(p.YawDegrees)
      || p.Y < 2400.f || p.Y > 4600.f) return 4;
 }
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "sanctuary.cpp"
            executable = Path(directory) / "sanctuary"
            source.write_text(cpp, encoding="utf-8")
            subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-Werror",
                "-I",str(SRC),str(source),"-o",str(executable)],
                check=True,capture_output=True,text=True)
            subprocess.run([str(executable)],check=True,capture_output=True,text=True)

    def test_instanced_geometry_has_no_collision_ticking_or_excessive_lights(self):
        arena = self.arena
        method = arena.split("void AMoonfallArena::CreateSanctuaryAtmosphere()",1)[1].split(
            "void AMoonfallArena::BeginPlay()",1)[0]
        for required in (
            "NewObject<UInstancedStaticMeshComponent>", "Details->AddInstance(FTransform(",
            "Details->SetCollisionEnabled(ECollisionEnabled::NoCollision);",
            "Details->SetCollisionResponseToAllChannels(ECR_Ignore);",
            "Details->SetGenerateOverlapEvents(false);",
            "Details->SetCastShadow(false);", "Details->SetCullDistances(14000, 22000);",
            "AddInstanceComponent(Details);","Details->RegisterComponent();",
            "Details->SetStaticMesh(StoneMesh);",
            "SanctuaryMoonstoneDetails = BuildDetails(","SanctuaryGothicDetails = BuildDetails(",
            "MoonLight->SetCastShadows(false);", "SanctuaryMoonLights.Add(MoonLight);"
        ):
            self.assertIn(required,method)
        self.assertLess(method.index("Details->AddInstance(FTransform("),
                        method.index("Details->RegisterComponent();"))
        self.assertIn("if (!bEnableStartingSanctuary || !bEnableSanctuaryAtmosphere",method)
        self.assertIn("FVector(-760.f, 4160.f, 310.f)",method)
        lanterns = method.split("const FVector Lanterns[] = {",1)[1].split("};",1)[0]
        plaques = method.split("const FStoryPlaque Plaques[] = {",1)[1].split("};",1)[0]
        self.assertEqual(lanterns.count("FVector("),4)
        self.assertEqual(plaques.count("FVector("),3)
        self.assertNotIn("TActorIterator",method)
        self.assertNotIn("LineTrace",method)
        self.assertNotIn("SetTimer",method)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", arena)

    def test_lore_signs_are_nonblocking_and_readable(self):
        method = self.arena.split("void AMoonfallArena::CreateSanctuaryAtmosphere()",1)[1].split(
            "void AMoonfallArena::BeginPlay()",1)[0]
        for phrase in ("FirstlightOath","FirstlightWarden","FirstlightMoon",
                       "THE SIX ROADS HOLD DIFFERENT TRUTHS",
                       "FOLLOW THE SOUTHERN PASS","LET THE WITNESSES SPEAK"):
            self.assertIn(phrase,method)
        self.assertIn("LoreMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);",method)
        self.assertIn("SanctuaryGuideSigns.Add(LoreMarker);",method)
        self.assertLess(method.index("LoreMarker->SetRelativeLocation(Plaque.At);"),
                        method.index("LoreMarker->RegisterComponent();"))

    def test_existing_shrine_gates_encounter_and_safe_sentries_unchanged(self):
        arena,header = self.arena,self.header
        self.assertIn("bool bEnableSanctuaryAtmosphere = true;",header)
        self.assertIn("bool bEnableStartingSanctuary = true;",header)
        enabled = arena.split("if (bEnableStartingSanctuary) {",1)[1].split("} else {",1)[0]
        self.assertIn("CreateSanctuaryAtmosphere();",enabled)
        for required in ("CreateBlock(FVector(0,1850,-20),FVector(5,10,0.4f));",
                         "CreateBlock(FVector(-850,1500,200),FVector(13,1,5));",
                         "CreateBlock(FVector(850,1500,200),FVector(13,1,5));",
                         'TEXT("sanctuary_firstlight")', 'TEXT("CrossroadsGuide")',
                         'FVector(-790.f, 4380.f, 60.f)', 'FVector(790.f, 4380.f, 60.f)'):
            self.assertIn(required,arena)
        self.assertIn("PracticeEnemy->ChaseSpeed = 0.f;",arena)
        self.assertIn("PracticeEnemy->AttackDamage = 2.f;",arena)
        self.assertIn("GetActorTransform().TransformPosition(Placement.Offset)",arena)
        self.assertIn("FeaturedDivineBossId",arena)

    def test_firstlight_dialogue_keeps_lore_and_existing_contract(self):
        dialogue = (SRC / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        speak = dialogue.split("bool AMoonfallWorldNPC::TryTalk(AMoonfallHeroPawn* Hero)",1)[1].split(
            "bool AMoonfallWorldNPC::OfferResidentQuest",1)[0]
        for text in ('SettlementId == FName(TEXT("Firstlight"))',
                     "Spoken.ToString() + TEXT(\" \") + Offer",
                     "I have a task for you. Select Accept in our conversation.",
                     'Hero->GetResidentQuestState(ResidentId) == 1',
                     "GetContractSalvageCost()",
                     'Hero->GetResidentQuestState(ResidentId) == 2',
                     'Disposition != TEXT("Hostile")'):
            self.assertIn(text,speak)
        self.assertIn(": FText::FromString(Offer);",speak)
        self.assertIn("OnDialogueStarted(Hero, Reaction);",speak)
        # No new quest currency, save object or ambient actor timer.
        self.assertNotIn("SaveGameToSlot",speak)

    def test_two_persistent_id_residents_without_new_quest_schema(self):
        scene=self.arena.split("// Two stable, no-Tick sanctuary residents",1)[1].split(
            "// Twelve fixed representatives",1)[0]
        for needle in ("FirstlightArchivist", "FirstlightWatchkeeper",
                       "Archivist Veyra", "Warden Cael", "if (bEnableStartingSanctuary)",
                       "GetActorTransform().TransformPosition(Entry.Position)",
                       "Person->ResidentId = FName(Entry.Id);",
                       "Person->SocialRole = FName(Entry.Role);",
                       "Person->Greeting = FText::FromString(Entry.Greeting);"):
            self.assertIn(needle,scene)
        self.assertIn("GetWorld()->SpawnActor<AMoonfallWorldNPC>",scene)
        self.assertNotIn("SaveGameToSlot",scene)
        self.assertNotIn("CreateBlock(",scene)

if __name__=="__main__":
    unittest.main()
