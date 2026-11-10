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
        self.assertIn('if (ConfirmedRewardKills >= 5 && !HasQuestKey(FName(TEXT("fallen_gate_key"))))', self.hero)
        self.assertIn("SpawnMilestoneKey(DefeatedLocation);", self.hero)
        self.assertNotIn('CollectQuestKey(FName(TEXT("fallen_gate_key")))', self.hero)
        self.assertIn("if (!SaveRewardProgress())", self.hero.split("bool AMoonfallHeroPawn::CollectQuestKey", 1)[1])

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
        interaction = self.hero.split("bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        pickup_branch = interaction.split("if (AMoonfallKeyPickup* Key = Cast<AMoonfallKeyPickup>(Nearest))", 1)[1].split(
            "if (AMoonfallKeyGate* Gate", 1)[0]
        self.assertIn("Key->TryCollect(this);", pickup_branch)
        self.assertIn("return true;", pickup_branch)
        self.assertNotIn("return Key->TryCollect(this);", pickup_branch)
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

    def test_saved_door_opens_on_load_without_proximity_or_replaying_first_unlock(self):
        gate = (ROOT / "MoonfallKeyGate.cpp").read_text()
        header = (ROOT / "MoonfallKeyGate.h").read_text()
        self.assertIn("virtual void BeginPlay() override;", header)
        self.assertIn("void OnSavedGateRestored();", header)
        self.assertIn("void AMoonfallKeyGate::BeginPlay()", gate)
        load = gate.split("void AMoonfallKeyGate::BeginPlay()", 1)[1].split(
            "bool AMoonfallKeyGate::TryUnlock(", 1)[0]
        self.assertIn("if (GateId.IsNone()) return;", load)
        self.assertIn("UGameplayStatics::LoadGameFromSlot(SlotName, 0)", load)
        self.assertIn("Saved->SchemaVersion != 2", load)
        self.assertIn("Saved->UnlockedGateIds.Contains(GateId)", load)
        self.assertIn("ApplyOpenState();", load)
        self.assertIn("OnSavedGateRestored();", load)
        self.assertNotIn("OnGateUnlocked();", load)
        self.assertNotIn("OnComponentBeginOverlap", gate)
        unlock = gate.split("bool AMoonfallKeyGate::TryUnlock(", 1)[1].split(
            "void AMoonfallKeyGate::ApplyOpenState()", 1)[0]
        self.assertLess(unlock.index("Hero->CommitGateUnlock(GateId, RequiredKeyId)"),
                        unlock.index("ApplyOpenState();"))
        self.assertIn("OnGateUnlocked();", unlock)
        apply = gate.split("void AMoonfallKeyGate::ApplyOpenState()", 1)[1]
        self.assertIn("bUnlocked = true;", apply)
        self.assertIn("GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);", apply)

    def test_no_key_or_quest_is_consumed_on_save_failure(self):
        pickup = (ROOT / "MoonfallKeyPickup.cpp").read_text()
        collect = self.hero.split("bool AMoonfallHeroPawn::CollectQuestKey(", 1)[1].split(
            "bool AMoonfallHeroPawn::CompleteFactionQuest(", 1)[0]
        self.assertIn("if (!SaveRewardProgress())", collect)
        self.assertIn("CollectedKeys.Remove(KeyId);", collect)
        self.assertLess(collect.index("if (!SaveRewardProgress())"), collect.index("return true;"))
        self.assertIn("if (!Hero->CollectQuestKey(KeyId)) return false;", pickup)
        collection_branch = pickup.split("if (!Hero->CollectQuestKey(KeyId)) return false;", 1)[1]
        self.assertIn("Destroy();", collection_branch)
        self.assertLess(collection_branch.index("Destroy();"), collection_branch.index("return true;"))
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

    def test_key_drop_recovery_and_stale_world_cleanup(self):
        key_cpp = (ROOT / "MoonfallKeyPickup.cpp").read_text()
        key_h = (ROOT / "MoonfallKeyPickup.h").read_text()
        self.assertIn("virtual void BeginPlay() override;", key_h)
        self.assertIn("void AMoonfallKeyPickup::BeginPlay()", key_cpp)
        self.assertIn("Hero->HasQuestKey(KeyId)", key_cpp)
        self.assertIn("if (!Hero->CollectQuestKey(KeyId)) return false;", key_cpp)
        self.assertIn("Destroy();", key_cpp)
        self.assertIn("if (HasQuestKey(Pickup->KeyId))", self.hero)
        self.assertIn("StaleOwnedKeys.Add(Pickup);", self.hero)
        self.assertIn("for (AMoonfallKeyPickup* Pickup : StaleOwnedKeys)", self.hero)
        self.assertIn("if (IsValid(Pickup)) Pickup->Destroy();", self.hero)
        self.assertIn("Consider(Pickup);", self.hero)
        self.assertNotIn("if (IsValid(*It) && !HasQuestKey(It->KeyId)) Consider(*It);", self.hero)
        self.assertIn('if (ConfirmedRewardKills >= 5 && !HasQuestKey(FName(TEXT("fallen_gate_key"))))', self.hero)
        self.assertIn("SpawnMilestoneKey(DefeatedLocation);", self.hero)

    def test_world_keys_show_names_and_milestone_spawn_is_deferred(self):
        pickup_cpp = (ROOT / "MoonfallKeyPickup.cpp").read_text()
        pickup_h = (ROOT / "MoonfallKeyPickup.h").read_text()
        self.assertIn("void RefreshKeyLabel();", pickup_h)
        self.assertIn("RefreshKeyLabel();", pickup_cpp)
        for name in ("WARDENS' CREST", "OUTLAW SEAL", "FALLEN GATE KEY"):
            self.assertIn(name, pickup_cpp)
        self.assertIn("Name + TEXT(\"  [F / X]\")", pickup_cpp)
        self.assertIn("UGameplayStatics::BeginDeferredActorSpawnFromClass(", self.hero)
        self.assertIn("UGameplayStatics::FinishSpawningActor(Pickup, SpawnTransform);", self.hero)
        milestone = self.hero.split("void AMoonfallHeroPawn::SpawnMilestoneKey(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardEnemyCurrency(", 1)[0]
        self.assertLess(milestone.index('Pickup->KeyId = FName(TEXT("fallen_gate_key"));'),
                        milestone.index("FinishSpawningActor(Pickup, SpawnTransform);"))
        self.assertNotIn("SpawnActor<AMoonfallKeyPickup>", milestone)
        self.assertIn("RefreshKeyLabel();", pickup_cpp.split("void AMoonfallKeyPickup::BeginPlay()", 1)[1])

    def test_accessible_key_and_gate_interaction_feedback(self):
        self.assertIn("FString KeyInteractionNotice;", self.hero_h)
        self.assertIn("float KeyInteractionNoticeUntil = -1.f;", self.hero_h)
        self.assertIn("bool bKeyInteractionNoticeError = false;", self.hero_h)
        interaction = self.hero.split("bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        for phrase in ("QUEST KEY COLLECTED", "KEY ALREADY COLLECTED",
                       "KEY NOT COLLECTED - CHECK SAVE OR KEY ID",
                       "DOOR UNLOCKED", "DOOR NOT OPENED - CHECK SAVE",
                       "LOCKED - FIND THE MATCHING KEY"):
            self.assertIn(phrase, interaction)
        self.assertIn("KeyInteractionNoticeUntil = GetWorld()->GetTimeSeconds() + 3.5f;", interaction)
        self.assertIn("Hero->KeyInteractionNoticeUntil > NowSeconds", self.hud)
        self.assertIn("Hero->bKeyInteractionNoticeError", self.hud)
        self.assertIn("FMath::Clamp(Hero->CombatTextScale, 0.75f, 2.f)", self.hud)
        self.assertNotIn("Tick(", interaction)

    def test_facing_priority_keeps_accessible_nearest_fallback(self):
        interaction = self.hero.split("bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        self.assertIn("AActor* NearestFacing = nullptr;", interaction)
        self.assertIn("const FVector Facing = GetActorForwardVector().GetSafeNormal2D();", interaction)
        self.assertIn("FVector::DotProduct(Facing, Direction) >= 0.25f", interaction)
        self.assertIn("DistanceSq <= BestFacingDistanceSq", interaction)
        self.assertIn("if (NearestFacing) Nearest = NearestFacing;", interaction)
        self.assertIn("DistanceSq > RangeSquared", interaction)
        self.assertNotIn("DistanceSq > BestDistanceSq) return;", interaction)
        self.assertIn("AActor* Nearest = nullptr;", interaction)
        self.assertIn("if (AMoonfallKeyPickup* Key = Cast<AMoonfallKeyPickup>(Nearest))", interaction)
        self.assertIn("if (AMoonfallKeyGate* Gate = Cast<AMoonfallKeyGate>(Nearest))", interaction)

    def test_stale_owned_keys_do_not_preempt_new_door_interactions(self):
        interaction = self.hero.split("bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        self.assertIn("TArray<AMoonfallKeyPickup*> StaleOwnedKeys;", interaction)
        self.assertIn("if (HasQuestKey(Pickup->KeyId))", interaction)
        self.assertIn("StaleOwnedKeys.Add(Pickup);", interaction)
        self.assertLess(interaction.index("StaleOwnedKeys.Add(Pickup);"),
                        interaction.index("Consider(Pickup);"))
        self.assertIn("for (AMoonfallKeyPickup* Pickup : StaleOwnedKeys)", interaction)
        self.assertIn("if (IsValid(Pickup)) Pickup->Destroy();", interaction)
        self.assertLess(interaction.index("for (AMoonfallKeyPickup* Pickup : StaleOwnedKeys)"),
                        interaction.index("for (TActorIterator<AMoonfallKeyGate>"))
        self.assertIn("if (NearestFacing) Nearest = NearestFacing;", interaction)

    def test_unreadable_reward_save_is_not_reset_or_overwritten(self):
        self.assertIn("bool bRewardSaveLoadFailed = false;", self.hero_h)
        loader = self.hero.split("bool AMoonfallHeroPawn::LoadRewardProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveRewardProgress()", 1)[0]
        self.assertIn("bRewardSaveLoadFailed = true;", loader)
        self.assertIn("bRewardSaveLoadFailed = false;", loader)
        self.assertIn("Save->SchemaVersion != 1 && Save->SchemaVersion != 2", loader)
        validated_load = loader.split("if (!Save ||", 1)[1]
        self.assertIn("CollectedKeys.Reset();", validated_load)
        self.assertIn("Gold = 0;", validated_load)
        self.assertIn("bRewardSaveLoadFailed = true;", loader)
        self.assertIn("if (!UGameplayStatics::DoesSaveGameExist(", loader)
        save_method = self.hero.split("bool AMoonfallHeroPawn::SaveRewardProgress()", 1)[1].split(
            "int64 AMoonfallHeroPawn::AddGold(", 1)[0]
        self.assertIn("if (bRewardSaveLoadFailed) return false;", save_method)
        self.assertLess(save_method.index("if (bRewardSaveLoadFailed) return false;"),
                        save_method.index("CreateSaveGameObject("))
        self.assertIn("SaveGameToSlot(Save, RewardSaveSlot, RewardSaveUser)", save_method)

    def test_equipment_remains_manual_and_hud_shows_rewards(self):
        self.assertIn("void PickupNearestGroundLoot();", self.hero_h)
        self.assertIn("Loot->TryCollect()", self.hero)
        self.assertIn("Hero->Gold", self.hud)
        self.assertIn("Hero->CollectedKeys.Num()", self.hud)
        self.assertIn("void AMoonfallHeroPawn::EquipSelectedInventoryItem()", self.hero)

if __name__ == "__main__":
    unittest.main()
