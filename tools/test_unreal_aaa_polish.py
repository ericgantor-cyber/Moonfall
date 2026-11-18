"""Regression contracts for Moonfall combat, atmosphere, forge and stability.

These inspect source integration and math. They do NOT compile or play UE5.4.
"""
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"MoonfallUnreal"/"Source"/"MoonfallUnreal"


class MoonfallAAAPlaySlice(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero_h=(SRC/"MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cls.hero=(SRC/"MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.hud=(SRC/"MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        cls.dungeon=(SRC/"MoonfallDungeonLayout.cpp").read_text(encoding="utf-8")
        cls.dungeon_h=(SRC/"MoonfallDungeonLayout.h").read_text(encoding="utf-8")
        cls.forge=(SRC/"MoonfallForgeForecast.h").read_text(encoding="utf-8")

    def test_parry_feedback_only_from_successful_parry(self):
        parry=self.hero.split("void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount)",1)[1].split(
            "void AMoonfallHeroPawn::",1)[0]
        self.assertIn('if (bPerfectParry) {',parry)
        self.assertIn('TEXT("PERFECT PARRY  |  COUNTER READY")',parry)
        self.assertGreater(parry.index('TEXT("PERFECT PARRY'),parry.index("if (bPerfectParry) {"))
        self.assertIn('CombatCueUntil = Now + 1.2f;',parry)
        self.assertIn('ParryCounterUntil = Now + FMath::Clamp(',parry)

    def test_dodge_feedback_never_rewards_expired_or_repeated_windows(self):
        dodge=self.hero.split("bool AMoonfallHeroPawn::TryRewardPerfectDodge()",1)[1].split(
            "bool AMoonfallHeroPawn::ApplyMoonfallDodgeOnlyDamageFrom(",1)[0]
        self.assertIn("Now > PerfectDodgeWindowUntil",dodge)
        self.assertIn("Now - LastPerfectDodgeRewardAt < 1.f",dodge)
        self.assertIn('TEXT("PERFECT DODGE  |  RESOURCES RESTORED")',dodge)
        self.assertLess(dodge.index("LastPerfectDodgeRewardAt = Now;"),dodge.index("CombatCueMessage ="))
        self.assertIn("PerfectDodgeWindowUntil = -1.f;",dodge)

    def test_combo_feedback_requires_successful_attack_and_real_milestone(self):
        offensive=self.hero.split("const float ActualSpellDamage =",1)[1].split(
            "if (ActualSpellDamage > 0.f && Enemy->Health > 0.f",1)[0]
        self.assertIn("if (ActualSpellDamage > 0.f) {",offensive)
        self.assertIn("HitComboStage == 4 || HitComboStage == 8 || HitComboStage == 12",offensive)
        self.assertIn("if (bNewPostureMilestone) {",offensive)
        self.assertIn("CHAIN 12  |  FINAL PRESSURE",offensive)
        self.assertIn("CHAIN 8  |  BREAKER",offensive)
        self.assertIn("CHAIN 4  |  PRESSURE",offensive)
        self.assertIn("OnSuccessfulHitCombo(HitComboStage, Hand == 0);",offensive)
        self.assertIn("HitComboStage > IncomingStage",offensive)

    def test_combat_hud_accessible_non_flashing_and_cleared_on_defeat(self):
        self.assertIn("FString CombatCueMessage;",self.hero_h)
        self.assertIn("float CombatCueUntil = -1.f;",self.hero_h)
        self.assertIn("Hero->CombatCueMessage.IsEmpty()",self.hud)
        self.assertIn("Hero->CombatTextScale",self.hud)
        self.assertIn("Canvas->SizeX >= 540.f && Canvas->SizeY >= 350.f",self.hud)
        self.assertIn("CombatCueMessage.Empty();",self.hero)
        self.assertIn("CombatCueUntil = -1.f;",self.hero)
        cue=self.hud.split("// Text-only feedback on verified combat success.",1)[1].split(
            "// Reuse a single world scan",1)[0]
        self.assertIn("DrawText(Hero->CombatCueMessage",cue)
        self.assertNotIn("FRand",cue)
        self.assertNotIn("SpawnActor",cue)

    def test_dungeon_landmarks_have_bounded_zero_collision_cost(self):
        self.assertIn("UInstancedStaticMeshComponent* RoomLandmarks;",self.dungeon_h)
        self.assertIn('TEXT("DungeonRoomLandmarks")',self.dungeon)
        self.assertIn("RoomLandmarks->SetCollisionEnabled(ECollisionEnabled::NoCollision);",self.dungeon)
        self.assertIn("RoomLandmarks->SetGenerateOverlapEvents(false);",self.dungeon)
        self.assertIn("RoomLandmarks->SetCastShadow(false);",self.dungeon)
        self.assertIn("RoomLandmarks->SetCullDistances(9000, 16000);",self.dungeon)
        self.assertIn("RoomLandmarks->ClearInstances();",self.dungeon)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;",self.dungeon)
        props=self.dungeon.split("// Non-colliding rune columns",1)[1].split(
            "for (const float Side : {-1.f, 1.f}) {\n   Walls->AddInstance",1)[0]
        self.assertIn("for (const float Side : {-1.f, 1.f}) {",props)
        self.assertIn("RoomLandmarks->AddInstance(",props)
        self.assertIn("static_cast<uint32>(GetEffectiveLayoutSeed())",props)
        self.assertIn("Chamber + 1",props)
        self.assertNotIn("RandRange(",props)
        self.assertNotIn("FRand",props)
        self.assertEqual(14*2,28,"Two non-blocking visual instances per chamber")
        self.assertLess(14*2*30,1000,"Entire 30-dungeon world stays under 1000 new mesh instances")
        # Cube assets are 100 cm across. Side placement at +-385 extends
        # approximately 37 cm and never intersects central +-180 cm path.
        self.assertGreater(385-0.74*50,180)
        self.assertLess(385+0.74*50,450)

    def test_optional_sound_is_one_shot_after_successful_room_spawn(self):
        self.assertIn("TObjectPtr<USoundBase> ChamberEntrySound = nullptr;",self.dungeon_h)
        room=self.dungeon.split("void AMoonfallDungeonLayout::HandleChamberOverlap(",1)[1].split(
            "void AMoonfallDungeonLayout::HandleHazardOverlap(",1)[0]
        self.assertLess(room.index("EnteredChambersMask |= Bit;"),room.index(
            "UGameplayStatics::PlaySoundAtLocation("))
        self.assertLess(room.index("if (ChamberType == 1 && !StartCombatChamber"),room.index(
            "EnteredChambersMask |= Bit;"))
        self.assertIn("if (IsValid(ChamberEntrySound) && GetWorld())",room)
        self.assertIn("if ((EnteredChambersMask & Bit) != 0u)",room)
        self.assertNotIn("NewObject<USoundBase>",room)

    def test_forge_plan_is_read_only_and_bounds_five_tiers(self):
        self.assertIn("constexpr bool ValidCode(int Code)",self.forge)
        self.assertIn("constexpr FPlan Preview(int Code, int Salvage, int MaxLevels = 5)",self.forge)
        self.assertIn("MaxLevels > 5 ? 5",self.forge)
        self.assertIn("if (Price == 0 || Balance < Price) break;",self.forge)
        self.assertIn("Cost(FinalTier, Rarity)",self.forge)
        forecast=self.forge.split("constexpr FPlan Preview(",1)[1]
        for dangerous in ("Rand(", "SaveWorldProgress", "InventoryItemCodes", "SpawnActor"):
            self.assertNotIn(dangerous,forecast)
        self.assertIn("MoonfallForgeForecast::Preview(",self.hero)
        self.assertIn("MoonfallForgeForecast::Preview(SelectedCode, Hero->Salvage, 5)",self.hud)
        self.assertIn("FORGE 5: +%d tiers -> T%d  |  %d salvage total",self.hud)
        self.assertIn("if (!UpgradeInventoryItem(InventoryIndex)) break;",self.hero)
        self.assertIn("InventoryItemCodes[InventoryIndex] = Code;",self.hero)


if __name__=="__main__":
    unittest.main()
