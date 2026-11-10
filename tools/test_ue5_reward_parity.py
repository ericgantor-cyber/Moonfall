#!/usr/bin/env python3
"""Static UE5 reward parity contract. Does not compile Unreal or play the game."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"

class UE5RewardParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.hero_h = (ROOT / "MoonfallHeroPawn.h").read_text()
        cls.hero = (ROOT / "MoonfallHeroPawn.cpp").read_text()
        cls.enemy = (ROOT / "MoonfallEnemy.cpp").read_text()
        cls.save = (ROOT / "MoonfallRewardSaveGame.h").read_text()
        cls.hud = (ROOT / "MoonfallCombatHUD.cpp").read_text()

    def test_persistent_reward_schema_and_engine_limit(self):
        self.assertIn("class MOONFALLUNREAL_API UMoonfallRewardSaveGame : public USaveGame", self.save)
        for field in ("int64 Gold = 0", "int64 ConfirmedKills = 0", "TArray<FName> CollectedKeys"):
            self.assertIn(field, self.save)
        self.assertIn("SaveGameToSlot(Save, RewardSaveSlot, RewardSaveUser)", self.hero)
        self.assertIn("LoadGameFromSlot(RewardSaveSlot, RewardSaveUser)", self.hero)
        self.assertIn("LoadRewardProgress();", self.hero)
        self.assertIn("MAX_int64 - Gold", self.hero)
        self.assertNotIn("99999999", self.hero[self.hero.index("int64 AMoonfallHeroPawn::AddGold"):self.hero.index("bool AMoonfallHeroPawn::HasQuestKey")])

    def test_key_whitelist_duplicate_guard_and_milestone(self):
        for key in ("wardens_crest", "outlaw_seal", "fallen_gate_key"):
            self.assertIn(key, self.hero)
        self.assertIn("!IsSupportedQuestKey(KeyId) || HasQuestKey(KeyId)", self.hero)
        self.assertIn("CollectedKeys.Num() < 128", self.hero)
        self.assertIn("if (ConfirmedRewardKills == 5)", self.hero)
        self.assertIn("SpawnMilestoneKey(DefeatedLocation);", self.hero)
        self.assertNotIn('CollectQuestKey(FName(TEXT("fallen_gate_key")))', self.hero)
        self.assertIn("SaveRewardProgress();", self.hero.split("bool AMoonfallHeroPawn::CollectQuestKey", 1)[1])

    def test_gold_is_from_confirmed_death_not_hud_or_damage(self):
        death = self.enemy.split("OnEnemyDefeated(bLastHitWasShatter);", 1)[1]
        self.assertIn("Hero->AwardEnemyCurrency(bElite, bBoss, GetActorLocation());", death)
        self.assertIn("Hero->SpawnGroundEquipmentDrop(bElite, bBoss, GetActorLocation());", death)
        self.assertEqual(self.enemy.count("Hero->AwardEnemyCurrency(bElite, bBoss, GetActorLocation());"), 1)
        self.assertIn("const int64 BaseGold = 3 + ConfirmedRewardKills / 4;", self.hero)
        self.assertIn("AddGold(BaseGold);", self.hero)

    def test_quest_reward_is_one_time_and_saved(self):
        self.assertIn("TArray<FName> CompletedFactionQuests", self.save)
        self.assertIn("Save->CompletedFactionQuests = CompletedFactionQuests;", self.hero)
        self.assertIn("for (const FName QuestId : Save->CompletedFactionQuests)", self.hero)
        quest = self.hero.split("bool AMoonfallHeroPawn::CompleteFactionQuest(FName QuestId)", 1)[1].split(
            "void AMoonfallHeroPawn::ToggleKeyRing()", 1)[0]
        self.assertIn("CompletedFactionQuests.Contains(QuestId)", quest)
        self.assertIn('FName(TEXT("wardens_crest"))', quest)
        self.assertIn('FName(TEXT("outlaw_seal"))', quest)
        self.assertIn("CompletedFactionQuests.Add(QuestId);", quest)
        self.assertIn("bKeyRingExpanded = true;", quest)

    def test_key_ring_input_and_visible_names(self):
        inputs = (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text()
        self.assertNotIn('ActionName="KeyRing",Key=K', inputs)
        self.assertNotIn('BindAction(TEXT("KeyRing")', self.hero)
        self.assertIn("PC->bEnableClickEvents = bInventoryOpen;", self.hero)
        self.assertIn("PC->bShowMouseCursor = bInventoryOpen;", self.hero)
        self.assertIn("NotifyHitBoxClick(FName BoxName)", self.hud)
        self.assertIn('AddHitBox(FVector2D(Left + 12.f, KeyRowY)', self.hud)
        self.assertIn('FName(TEXT("MoonfallKeyRing"))', self.hud)
        self.assertIn("Hero->bInventoryOpen && !Hero->bOptionsOpen", self.hud)
        self.assertIn("if (!Hero->bCharacterStatsOpen)", self.hud)
        self.assertIn('FString::Printf(TEXT("KEYS (%d)")', self.hud)
        self.assertIn("Hero->bKeyRingInventoryOpen", self.hud)
        self.assertIn("const float PopWidth = FMath::Min(300.f, PanelWidth - 24.f);", self.hud)
        self.assertIn("bKeyRingInventoryOpen = !bKeyRingInventoryOpen;", self.hero)
        self.assertIn("bKeyRingInventoryOpen = false;", self.hero)
        self.assertIn("Hero->ToggleKeyRing();", self.hud)
        self.assertIn("Hero->bKeyRingExpanded", self.hud)
        for label in ("Wardens' Crest", "Outlaw Seal", "Fallen Gate Key"):
            self.assertIn(label, self.hud)

    def test_unreal_dungeon_gate_requires_owned_key_and_preserves_it(self):
        gate_h = (ROOT / "MoonfallKeyGate.h").read_text()
        gate_cpp = (ROOT / "MoonfallKeyGate.cpp").read_text()
        self.assertIn('FName RequiredKeyId = FName(TEXT("fallen_gate_key"))', gate_h)
        self.assertIn("bool TryUnlock(AActor* Candidate);", gate_h)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", gate_cpp)
        self.assertIn("SetRootComponent(GateRoot);", gate_cpp)
        self.assertIn("Cast<AMoonfallHeroPawn>(Candidate)", gate_cpp)
        self.assertIn("!Hero->HasQuestKey(RequiredKeyId)", gate_cpp)
        self.assertIn("OnGateLocked(RequiredKeyId);", gate_cpp)
        self.assertIn("GateLabel->SetText(FText::FromString(", gate_cpp)
        self.assertIn("REQUIRES %s  [F / X]", gate_cpp)
        self.assertIn('FName(TEXT("wardens_crest"))', gate_cpp)
        self.assertIn('FName(TEXT("outlaw_seal"))', gate_cpp)
        self.assertIn("GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);", gate_cpp)
        self.assertIn("OnGateUnlocked();", gate_cpp)
        self.assertNotIn("OnKeyProximityBegin", gate_cpp)
        self.assertNotIn("OnComponentBeginOverlap", gate_cpp)
        self.assertNotIn("TryUnlock(OtherActor);", gate_cpp)
        self.assertIn("explicit interaction", gate_h)
        self.assertNotIn("CollectedKeys.Remove", gate_cpp)
        self.assertNotIn("CollectQuestKey(", gate_cpp)

    def test_ac_style_manual_world_keys_and_locked_gate_interaction(self):
        key_h = (ROOT / "MoonfallKeyPickup.h").read_text()
        key_cpp = (ROOT / "MoonfallKeyPickup.cpp").read_text()
        gate_cpp = (ROOT / "MoonfallKeyGate.cpp").read_text()
        self.assertIn("class MOONFALLUNREAL_API AMoonfallKeyPickup", key_h)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", key_cpp)
        self.assertIn("Hero->CollectQuestKey(KeyId)", key_cpp)
        self.assertIn("Destroy();", key_cpp)
        self.assertIn("if (TryInteractWithNearbyKey()) return;", self.hero)
        self.assertIn("TActorIterator<AMoonfallKeyPickup>", self.hero)
        self.assertIn("TActorIterator<AMoonfallKeyGate>", self.hero)
        self.assertIn("FMath::Square(230.f)", self.hero)
        self.assertIn("MoonfallKeyInteractSight", self.hero)
        self.assertIn("Gate->TryUnlock(this);", self.hero)
        self.assertIn("Key->TryCollect(this);", self.hero)
        self.assertIn("SpawnMilestoneKey(DefeatedLocation);", self.hero)
        self.assertIn("SpawnMilestoneKey(GetActorLocation()", self.hero)
        self.assertNotIn("OnComponentBeginOverlap", gate_cpp)
        self.assertNotIn('ActionName="KeyRing",Key=K', (ROOT.parents[1] / "Config" / "DefaultInput.ini").read_text())

    def test_opt_in_key_trial_is_placed_and_never_auto_unlocks(self):
        arena_h = (ROOT / "MoonfallArena.h").read_text()
        arena_cpp = (ROOT / "MoonfallArena.cpp").read_text()
        gate_cpp = (ROOT / "MoonfallKeyGate.cpp").read_text()
        self.assertIn("bool bEnableKeyTrial = false;", arena_h)
        self.assertIn("if (bEnableKeyTrial)", arena_cpp)
        self.assertIn("!TrialHero->HasQuestKey(TrialKey)", arena_cpp)
        self.assertIn("UGameplayStatics::GetPlayerPawn(this, 0)", arena_cpp)
        self.assertIn("AMoonfallKeyPickup::StaticClass()", arena_cpp)
        self.assertIn("AMoonfallKeyGate::StaticClass()", arena_cpp)
        self.assertIn("KeyPickup->KeyId = TrialKey;", arena_cpp)
        self.assertIn("Gate->RequiredKeyId = TrialKey;", arena_cpp)
        self.assertIn("BeginDeferredActorSpawnFromClass", arena_cpp)
        self.assertIn("FinishSpawningActor(KeyPickup, KeyTransform);", arena_cpp)
        self.assertIn("FinishSpawningActor(Gate, GateTransform);", arena_cpp)
        self.assertNotIn("Gate->TryUnlock(", arena_cpp)
        self.assertNotIn("OnComponentBeginOverlap", gate_cpp)

    def test_v2_saves_retain_v1_compatibility_and_named_gates(self):
        self.assertIn("int32 SchemaVersion = 2;", self.save)
        self.assertIn("TArray<FName> UnlockedGateIds;", self.save)
        self.assertIn("Save->SchemaVersion != 1 && Save->SchemaVersion != 2", self.hero)
        self.assertIn("if (Save->SchemaVersion >= 2)", self.hero)
        self.assertIn("Save->UnlockedGateIds = UnlockedGateIds;", self.hero)
        self.assertIn("UnlockedGateIds.Num() < 256", self.hero)
        self.assertIn("bool AMoonfallHeroPawn::HasUnlockedGate(FName GateId) const", self.hero)
        unlock = self.hero.split("bool AMoonfallHeroPawn::CommitGateUnlock(", 1)[1].split(
            "void AMoonfallHeroPawn::ToggleKeyRing()", 1)[0]
        self.assertIn("!HasQuestKey(RequiredKeyId) && !HasUnlockedGate(GateId)", unlock)
        self.assertIn("UnlockedGateIds.Add(GateId);", unlock)
        self.assertIn("if (!SaveRewardProgress())", unlock)
        self.assertIn("UnlockedGateIds.Remove(GateId);", unlock)
        gate = (ROOT / "MoonfallKeyGate.cpp").read_text()
        gate_h = (ROOT / "MoonfallKeyGate.h").read_text()
        self.assertIn("FName GateId = NAME_None;", gate_h)
        self.assertIn("Hero->CommitGateUnlock(GateId, RequiredKeyId)", gate)
        self.assertIn("Hero->HasUnlockedGate(GateId)", gate)
        self.assertLess(gate.index("Hero->CommitGateUnlock("), gate.index("bUnlocked = true;"))
        arena = (ROOT / "MoonfallArena.cpp").read_text()
        self.assertIn('Gate->GateId = FName(TEXT("wardens_trial_gate"));', arena)

    def test_no_key_or_quest_is_consumed_on_save_failure(self):
        pickup = (ROOT / "MoonfallKeyPickup.cpp").read_text()
        collect = self.hero.split("bool AMoonfallHeroPawn::CollectQuestKey(", 1)[1].split(
            "bool AMoonfallHeroPawn::CompleteFactionQuest(", 1)[0]
        self.assertIn("if (!SaveRewardProgress())", collect)
        self.assertIn("CollectedKeys.Remove(KeyId);", collect)
        self.assertLess(collect.index("if (!SaveRewardProgress())"), collect.index("return true;"))
        self.assertIn("if (!Hero->CollectQuestKey(KeyId)) return false;", pickup)
        self.assertLess(pickup.index("Hero->CollectQuestKey(KeyId)"), pickup.index("Destroy();"))
        quest = self.hero.split("bool AMoonfallHeroPawn::CompleteFactionQuest(", 1)[1].split(
            "bool AMoonfallHeroPawn::HasUnlockedGate(", 1)[0]
        self.assertIn("if (bNeedsKey && CollectedKeys.Num() >= 128) return false;", quest)
        self.assertIn("CompletedFactionQuests.Remove(QuestId);", quest)
        self.assertIn("if (bNeedsKey) CollectedKeys.Remove(RewardKey);", quest)
        gold = self.hero.split("int64 AMoonfallHeroPawn::AddGold(", 1)[1].split(
            "bool AMoonfallHeroPawn::HasQuestKey(", 1)[0]
        self.assertIn("Gold = PreviousGold;", gold)
        self.assertIn("return 0;", gold)
        kill = self.hero.split("void AMoonfallHeroPawn::AwardEnemyCurrency(", 1)[1].split(
            "void AMoonfallHeroPawn::BeginPlay()", 1)[0]
        self.assertIn("ConfirmedRewardKills = PreviousKills;", kill)
        self.assertIn("if (Gold >= MAX_int64)", kill)
        self.assertIn("const int64 GrantedGold = AddGold(BaseGold);", kill)

    def test_equipment_remains_manual_and_hud_shows_rewards(self):
        self.assertIn("void PickupNearestGroundLoot();", self.hero_h)
        self.assertIn("Loot->TryCollect()", self.hero)
        self.assertIn("Hero->Gold", self.hud)
        self.assertIn("Hero->CollectedKeys.Num()", self.hud)
        self.assertIn("void AMoonfallHeroPawn::EquipSelectedInventoryItem()", self.hero)

if __name__ == "__main__":
    unittest.main()
