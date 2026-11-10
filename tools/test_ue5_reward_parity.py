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
        self.assertIn("if (!LoadRewardProgress())", self.hero)
        self.assertIn("GetRewardSlotName()", self.save)
        self.assertIn("UMoonfallRewardSaveGame::GetRewardSlotName()", self.hero)
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
        self.assertIn("void AMoonfallHeroPawn::InteractWithNearbyWorld()", self.hero)
        self.assertIn("TryInteractWithNearbyKey();", self.hero.split(
            "void AMoonfallHeroPawn::InteractWithNearbyWorld()", 1)[1].split(
            "void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0])
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
        self.assertIn("int32 SchemaVersion = 5;", self.save)
        self.assertIn("TArray<FName> UnlockedGateIds;", self.save)
        self.assertIn("Save->SchemaVersion != 1 && Save->SchemaVersion != 2 && Save->SchemaVersion != 3 && Save->SchemaVersion != 4 && Save->SchemaVersion != 5", self.hero)
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
        self.assertIn("Saved->SchemaVersion != 2 && Saved->SchemaVersion != 3 && Saved->SchemaVersion != 4 && Saved->SchemaVersion != 5", load)
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
        self.assertIn('const FName MilestoneKey(TEXT("fallen_gate_key"));', milestone)
        self.assertLess(milestone.index("Pickup->KeyId = MilestoneKey;"),
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
        self.assertIn("Save->SchemaVersion != 1 && Save->SchemaVersion != 2 && Save->SchemaVersion != 3 && Save->SchemaVersion != 4 && Save->SchemaVersion != 5", loader)
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

    def test_failed_reward_load_warns_player_and_prevents_key_spawn(self):
        startup = self.hero.split("void AMoonfallHeroPawn::BeginPlay()", 1)[1].split(
            "void AMoonfallHeroPawn::Tick(", 1)[0]
        self.assertIn("if (!LoadRewardProgress())", startup)
        self.assertIn("REWARD SAVE ERROR - ORIGINAL FILE PROTECTED", startup)
        self.assertIn("KeyInteractionNoticeUntil = GetWorld()", startup)
        self.assertIn("bKeyInteractionNoticeError = true;", startup)
        self.assertIn("if (!bRewardSaveLoadFailed && ConfirmedRewardKills >= 5", startup)
        self.assertLess(startup.index("if (!LoadRewardProgress())"),
                        startup.index("SpawnMilestoneKey(GetActorLocation()"))
        self.assertIn("Hero->KeyInteractionNoticeUntil > NowSeconds", self.hud)

    def test_gate_restoration_reuses_ready_hero_progress_and_falls_back_safely(self):
        gate = (ROOT / "MoonfallKeyGate.cpp").read_text()
        self.assertIn("bool bRewardSaveInitialized = false;", self.hero_h)
        loader = self.hero.split("bool AMoonfallHeroPawn::LoadRewardProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveRewardProgress()", 1)[0]
        self.assertEqual(loader.count("bRewardSaveInitialized = true;"), 3)
        restore = gate.split("void AMoonfallKeyGate::BeginPlay()", 1)[1].split(
            "bool AMoonfallKeyGate::TryUnlock(", 1)[0]
        self.assertIn("Hero->bRewardSaveInitialized", restore)
        self.assertIn("Hero->bRewardSaveLoadFailed", restore)
        self.assertIn("Hero->HasUnlockedGate(GateId)", restore)
        self.assertIn("const FString& SlotName = UMoonfallRewardSaveGame::GetRewardSlotName();", restore)
        self.assertIn("UGameplayStatics::LoadGameFromSlot(SlotName, 0)", restore)
        self.assertIn("Saved->UnlockedGateIds.Contains(GateId)", restore)
        self.assertIn("ApplyOpenState();", restore)
        self.assertIn("OnSavedGateRestored();", restore)

    def test_milestone_pickup_is_cached_weakly_without_losing_recovery(self):
        self.assertIn("TWeakObjectPtr<AMoonfallKeyPickup> PendingMilestoneKey;", self.hero_h)
        spawn = self.hero.split("void AMoonfallHeroPawn::SpawnMilestoneKey(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardEnemyCurrency(", 1)[0]
        self.assertIn("PendingMilestoneKey.IsValid()", spawn)
        self.assertIn("PendingMilestoneKey->GetWorld() == GetWorld()", spawn)
        self.assertIn("PendingMilestoneKey.Reset();", spawn)
        self.assertIn("TActorIterator<AMoonfallKeyPickup>", spawn)
        self.assertIn("PendingMilestoneKey = *It;", spawn)
        self.assertIn("if (IsValid(Pickup)) PendingMilestoneKey = Pickup;", spawn)
        self.assertLess(spawn.index("PendingMilestoneKey.IsValid()"),
                        spawn.index("TActorIterator<AMoonfallKeyPickup>"))
        self.assertLess(spawn.index("Pickup->KeyId = MilestoneKey;"),
                        spawn.index("FinishSpawningActor(Pickup, SpawnTransform);"))
        self.assertIn("if (ConfirmedRewardKills >= 5", self.hero)

    def test_failed_kill_reward_save_warns_without_claiming_currency(self):
        kill = self.hero.split("void AMoonfallHeroPawn::AwardEnemyCurrency(", 1)[1].split(
            "void AMoonfallHeroPawn::BeginPlay()", 1)[0]
        self.assertEqual(kill.count("ConfirmedRewardKills = PreviousKills;"), 2)
        self.assertEqual(kill.count("REWARD SAVE FAILED - GOLD AND KILL NOT SAVED"), 2)
        self.assertEqual(kill.count("REWARD SAVE ERROR - ORIGINAL FILE PROTECTED"), 2)
        self.assertEqual(kill.count("bKeyInteractionNoticeError = true;"), 2)
        self.assertEqual(kill.count("KeyInteractionNoticeUntil = GetWorld()"), 2)
        self.assertIn("if (!SaveRewardProgress())", kill)
        self.assertIn("if (GrantedGold <= 0)", kill)
        self.assertIn("SpawnMilestoneKey(DefeatedLocation);", kill)
        self.assertIn("Hero->KeyInteractionNoticeUntil > NowSeconds", self.hud)

    def test_v3_ground_loot_pickup_is_durable_and_transactional(self):
        ground = (ROOT / "MoonfallGroundLoot.cpp").read_text()
        self.assertIn("int32 SchemaVersion = 5;", self.save)
        self.assertIn("TArray<int32> InventoryItemCodes;", self.save)
        self.assertIn("Save->InventoryItemCodes = InventoryItemCodes;", self.hero)
        loader = self.hero.split("bool AMoonfallHeroPawn::LoadRewardProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveRewardProgress()", 1)[0]
        self.assertIn("if (Save->SchemaVersion >= 3)", loader)
        self.assertIn("InventoryItemCodes.Reset();", loader)
        self.assertIn("InventoryItemCodes.Num() >= 4096", loader)
        self.assertIn("InventoryItemCodes.Add(Code);", loader)
        method = self.hero.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertIn("InventoryItemCodes.Add(ItemCode);", method)
        self.assertIn("if (!SaveRewardProgress())", method)
        self.assertIn("InventoryItemCodes.Pop();", method)
        self.assertIn("LOOT SAVE FAILED - ITEM LEFT ON GROUND", method)
        self.assertLess(method.index("InventoryItemCodes.Add(ItemCode);"),
                        method.index("if (!SaveRewardProgress())"))
        self.assertLess(method.index("if (!SaveRewardProgress())"),
                        method.index("LastLootAffix = Affix;"))
        self.assertIn("!Hero->CollectPersistedGroundEquipmentCode(ItemCode, PersistentDropId)", ground)
        self.assertLess(ground.index("!Hero->CollectPersistedGroundEquipmentCode(ItemCode, PersistentDropId)"),
                        ground.index("bCollected = true;"))
        self.assertLess(ground.index("bCollected = true;"), ground.index("Destroy();"))
        gate = (ROOT / "MoonfallKeyGate.cpp").read_text()
        self.assertIn("Saved->SchemaVersion != 2 && Saved->SchemaVersion != 3 && Saved->SchemaVersion != 4 && Saved->SchemaVersion != 5", gate)

    def test_salvage_and_loot_save_as_one_rollback_safe_transaction(self):
        self.assertIn("UPROPERTY(SaveGame) int32 Salvage = 0;", self.save)
        self.assertIn("Save->Salvage = FMath::Clamp(Salvage, 0, 9999);", self.hero)
        self.assertIn("Salvage = FMath::Clamp(Save->Salvage, 0, 9999);", self.hero)
        salvage = self.hero.split("bool AMoonfallHeroPawn::SalvageInventoryItem(", 1)[1].split(
            "\n}\n", 1)[0]
        self.assertIn("const int32 PreviousSalvage = Salvage;", salvage)
        self.assertIn("InventoryItemCodes.RemoveAt(InventoryIndex);", salvage)
        self.assertIn("if (!SaveRewardProgress())", salvage)
        self.assertIn("InventoryItemCodes.Insert(Code, InventoryIndex);", salvage)
        self.assertIn("Salvage = PreviousSalvage;", salvage)
        self.assertIn("SALVAGE SAVE FAILED - ITEM RESTORED", salvage)
        self.assertLess(salvage.index("InventoryItemCodes.RemoveAt(InventoryIndex);"),
                        salvage.index("if (!SaveRewardProgress())"))
        self.assertLess(salvage.index("if (!SaveRewardProgress())"),
                        salvage.index("InventorySelection = FMath::Clamp("))

    def test_equipment_loadout_and_crafting_commit_atomically(self):
        for field in ("EquippedWeaponCode", "EquippedArmorCode", "EquippedAccessoryCode"):
            self.assertIn(f"UPROPERTY(SaveGame) int32 {field} = 0;", self.save)
            self.assertIn(f"Save->{field} = {field};", self.hero)
            self.assertIn(f"Save->{field}", self.hero)
        loader = self.hero.split("bool AMoonfallHeroPawn::LoadRewardProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveRewardProgress()", 1)[0]
        self.assertIn("InventoryItemCodes.Contains(Code)", loader)
        self.assertIn("PreviousArmorTierOnLoad", loader)
        self.assertIn("ArmorHealthDelta", loader)
        self.assertIn("EquipmentTier = FMath::Max(", loader)
        equip = self.hero.split("bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[1].split(
            "\n}\n", 1)[0]
        self.assertIn("if (!bDeferEquippedSave && !SaveRewardProgress())", equip)
        self.assertIn("*EquippedCode = PreviousCode;", equip)
        self.assertIn("MaxHealth = PreviousMaxHealth;", equip)
        self.assertIn("EQUIP SAVE FAILED - LOADOUT RESTORED", equip)
        self.assertIn("bool bDeferEquippedSave = false;", self.hero_h)
        for name in ("UpgradeInventoryItem", "AscendInventoryItemRarity", "ReforgeInventoryItemAffix"):
            craft = self.hero.split(f"bool AMoonfallHeroPawn::{name}(", 1)[1].split(
                "\n}\n", 1)[0]
            self.assertIn("bDeferEquippedSave = true;", craft)
            self.assertIn("bDeferEquippedSave = false;", craft)
            self.assertIn("const int32 OldSalvage = Salvage;", craft)
            self.assertIn("if (!SaveRewardProgress())", craft)
            self.assertIn("InventoryItemCodes[InventoryIndex] = Code;", craft)
            self.assertIn("Salvage = OldSalvage;", craft)
            self.assertIn("*LiveCode = OldEquippedCode;", craft)
            self.assertIn("MaxHealth = OldMaxHealth;", craft)
            self.assertIn("CRAFT SAVE FAILED - ITEM AND MATERIALS RESTORED", craft)
            self.assertLess(craft.index("if (!SaveRewardProgress())"),
                            craft.index("OnEquipment", craft.index("if (!SaveRewardProgress())")))

    def test_scripted_equipment_reward_saves_or_leaves_same_rolled_item(self):
        grant = self.hero.split("void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[1].split(
            "bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[0]
        self.assertIn("const int32 ItemCode = RollEquipmentCode(", grant)
        self.assertIn("if (CollectGroundEquipmentCode(ItemCode))", grant)
        self.assertIn("AMoonfallGroundLoot::StaticClass()", grant)
        self.assertIn("Loot->InitializeLoot(ItemCode);", grant)
        self.assertIn("UGameplayStatics::FinishSpawningActor(Loot, SpawnTransform);", grant)
        self.assertIn("REWARD UNSAVED - ITEM LEFT ON GROUND", grant)
        self.assertIn("REWARD UNSAVED - COULD NOT SPAWN RECOVERY DROP", grant)
        self.assertLess(grant.index("if (CollectGroundEquipmentCode(ItemCode))"),
                        grant.index("BeginDeferredActorSpawnFromClass("))
        self.assertLess(grant.index("Loot->InitializeLoot(ItemCode);"),
                        grant.index("FinishSpawningActor(Loot, SpawnTransform);"))
        self.assertNotIn("InventoryItemCodes.Add(ItemCode);", grant)
        self.assertNotIn("*EquippedCode = ItemCode;", grant)
        self.assertNotIn("MaxHealth +=", grant)
        pickup = self.hero.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertIn("if (!SaveRewardProgress())", pickup)
        self.assertIn("InventoryItemCodes.Pop();", pickup)

    def test_pickup_auto_equips_only_an_empty_matching_slot(self):
        collect = self.hero.split("bool AMoonfallHeroPawn::CollectGroundEquipmentCode(", 1)[1].split(
            "bool AMoonfallHeroPawn::CollectPersistedGroundEquipmentCode(", 1)[0]
        self.assertIn("const bool bAutoEquipEmptySlot = *EquippedCode == 0;", collect)
        self.assertIn("if (bAutoEquipEmptySlot)", collect)
        self.assertIn("*EquippedCode = ItemCode;", collect)
        self.assertIn("*EquippedTier = Tier;", collect)
        self.assertIn("*EquippedAffix = Affix;", collect)
        self.assertIn("if (SlotIndex == 1)", collect)
        self.assertIn("const float HealthBonus = 2.f * Tier;", collect)
        self.assertIn("InventoryItemCodes.Add(ItemCode);", collect)
        self.assertIn("if (!SaveRewardProgress())", collect)
        self.assertIn("InventoryItemCodes.Pop();", collect)
        self.assertIn("*EquippedCode = 0;", collect)
        self.assertIn("EquipmentTier = PreviousEquipmentTier;", collect)
        self.assertIn("MaxHealth = PreviousMaxHealth;", collect)
        self.assertIn("Health = PreviousHealth;", collect)
        self.assertLess(collect.index("InventoryItemCodes.Add(ItemCode);"),
                        collect.index("if (!SaveRewardProgress())"))
        self.assertLess(collect.index("if (!SaveRewardProgress())"),
                        collect.index("OnHealthChanged(Health, MaxHealth);"))
        self.assertEqual(collect.count("SaveRewardProgress()"), 1)
        self.assertNotIn("EquipInventoryItem(", collect)
        self.assertNotIn("SalvageInventoryItem(", collect)

    def test_v4_ground_drops_persist_until_atomic_pickup(self):
        self.assertIn("int32 SchemaVersion = 5;", self.save)
        self.assertIn("struct FMoonfallWorldLootRecord", self.save)
        self.assertIn("UPROPERTY(SaveGame) FGuid DropId;", self.save)
        self.assertIn("TArray<FMoonfallWorldLootRecord> PendingWorldLoot;", self.save)
        self.assertIn("Save->PendingWorldLoot = PendingWorldLoot;", self.hero)
        self.assertIn("if (Save->SchemaVersion >= 4)", self.hero)
        self.assertIn("if (!bRewardSaveLoadFailed) RestorePendingWorldEquipmentDrops();", self.hero)
        batch = self.hero.split("bool AMoonfallHeroPawn::RegisterWorldEquipmentDrops(", 1)[1].split(
            "void AMoonfallHeroPawn::RestorePendingWorldEquipmentDrops()", 1)[0]
        self.assertIn("PendingWorldLoot.Num() + NewDrops.Num() > 512", batch)
        self.assertIn("NewDrops.Num() > 4", batch)
        self.assertIn("PendingWorldLoot.Append(NewDrops);", batch)
        self.assertIn("TSet<FGuid> BatchIds;", batch)
        self.assertIn("if (BatchIds.Contains(Record.DropId)) return false;", batch)
        self.assertIn("PendingWorldLoot.SetNum(OriginalCount);", batch)
        self.assertEqual(batch.count("SaveRewardProgress()"), 1)
        self.assertLess(batch.index("PendingWorldLoot.Append(NewDrops);"),
                        batch.index("if (!SaveRewardProgress())"))
        self.assertLess(batch.index("if (!SaveRewardProgress())"),
                        batch.index("BeginDeferredActorSpawnFromClass("))
        self.assertIn("Loot->SetPersistentDropId(Record.DropId);", batch)
        spawn = self.hero.split("void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[1].split(
            "bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[0]
        self.assertIn("RegisterWorldEquipmentDrops(Drops)", spawn)
        self.assertIn("Drops.Add(Record);", spawn)
        self.assertIn("const int32 BaseDropCount = bBossEnemy ? 3 : (bEliteEnemy ? 2 : 1);", spawn)
        self.assertIn("const int32 BonusDropChance = bBossEnemy ? 50 : (bEliteEnemy ? 35 : 20);", spawn)
        self.assertIn("const int32 DropCount = BaseDropCount + (FMath::RandRange(0, 99) < BonusDropChance ? 1 : 0);", spawn)
        self.assertLess(spawn.index("Drops.Add(Record);"), spawn.index("RegisterWorldEquipmentDrops(Drops)"))
        collect = self.hero.split("bool AMoonfallHeroPawn::CollectPersistedGroundEquipmentCode(", 1)[1].split(
            "bool AMoonfallHeroPawn::RegisterWorldEquipmentDrops(", 1)[0]
        self.assertIn("PendingWorldLoot.RemoveAt(Index);", collect)
        self.assertIn("if (CollectGroundEquipmentCode(ItemCode)) return true;", collect)
        self.assertIn("PendingWorldLoot.Insert(Record, Index);", collect)
        ground_cpp = (ROOT / "MoonfallGroundLoot.cpp").read_text()
        self.assertIn("CollectPersistedGroundEquipmentCode(ItemCode, PersistentDropId)", ground_cpp)

    def test_drop_rng_is_bounded_and_rolls_per_item(self):
        loot_roll = self.hero.split("int32 AMoonfallHeroPawn::RollEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[0]
        self.assertIn("const int32 BaseTier = Level / 5", loot_roll)
        self.assertIn("BaseTier + FMath::RandRange(-1, 1)", loot_roll)
        self.assertIn("FMath::Clamp(BaseTier + FMath::RandRange(-1, 1), 1, 25)", loot_roll)
        self.assertIn("LastLootSlot = PreferredSlot >= 0 && PreferredSlot <= 2", loot_roll)
        self.assertIn("PreferredSlot : FMath::RandRange(0, 2);", loot_roll)
        self.assertIn("const int32 LootRoll = FMath::RandRange(0, 99);", loot_roll)
        spawn = self.hero.split("void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[1].split(
            "bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[0]
        self.assertIn("BonusDropChance = bBossEnemy ? 50 : (bEliteEnemy ? 35 : 20)", spawn)
        self.assertIn("Drops.Reserve(DropCount);", spawn)
        self.assertIn("for (int32 Drop = 0; Drop < DropCount; ++Drop)", spawn)
        batch = self.hero.split("bool AMoonfallHeroPawn::RegisterWorldEquipmentDrops(", 1)[1].split(
            "void AMoonfallHeroPawn::RestorePendingWorldEquipmentDrops()", 1)[0]
        self.assertIn("NewDrops.Num() > 4", batch)
        self.assertEqual(batch.count("SaveRewardProgress()"), 1)

    def test_encounter_loot_slots_are_diverse_without_replacing_item_rng(self):
        roll = self.hero.split("int32 AMoonfallHeroPawn::RollEquipmentCode(", 1)[1].split(
            "void AMoonfallHeroPawn::AwardEquipmentDrop(", 1)[0]
        spawn = self.hero.split("void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[1].split(
            "bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[0]
        self.assertIn("int32 PreferredSlot = INDEX_NONE", self.hero_h)
        self.assertIn("TArray<int32> UnusedSlots = {0, 1, 2};", spawn)
        self.assertIn("PreferredSlot = UnusedSlots[Choice];", spawn)
        self.assertIn("UnusedSlots.RemoveAtSwap(Choice);", spawn)
        self.assertIn("if (!UnusedSlots.IsEmpty())", spawn)
        self.assertIn("RollEquipmentCode(bEliteEnemy, bBossEnemy, PreferredSlot)", spawn)
        self.assertIn("PreferredSlot : FMath::RandRange(0, 2);", roll)
        self.assertIn("const int32 LootRoll = FMath::RandRange(0, 99);", roll)
        self.assertIn("BaseTier + FMath::RandRange(-1, 1)", roll)
        self.assertLess(spawn.index("UnusedSlots.RemoveAtSwap(Choice);"),
                        spawn.index("RollEquipmentCode(bEliteEnemy, bBossEnemy, PreferredSlot)"))

    def test_world_loot_overflow_falls_back_to_visible_temporary_pickup(self):
        restore = self.hero.split("void AMoonfallHeroPawn::RestorePendingWorldEquipmentDrops()", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertIn("TMap<FGuid, AMoonfallGroundLoot*> SpawnedActors;", restore)
        self.assertLess(restore.index("for (TActorIterator<AMoonfallGroundLoot>"),
                        restore.index("for (const FMoonfallWorldLootRecord& Record", restore.index("TMap<FGuid")))
        self.assertEqual(restore.count("TActorIterator<AMoonfallGroundLoot>"), 1)
        spawn = self.hero.split("void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[1].split(
            "bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[0]
        self.assertIn("if (Drops.IsEmpty() || RegisterWorldEquipmentDrops(Drops)) return;", spawn)
        self.assertIn("for (const FMoonfallWorldLootRecord& Record : Drops)", spawn)
        self.assertIn("Fallback->InitializeLoot(Record.ItemCode);", spawn)
        self.assertIn("LOOT NOT SAVED - TEMPORARY DROPS, COLLECT BEFORE EXIT", spawn)
        self.assertIn("LOOT NOT SAVED - SOME WORLD DROPS FAILED", spawn)
        self.assertLess(spawn.index("RegisterWorldEquipmentDrops(Drops)"),
                        spawn.index("Fallback->InitializeLoot(Record.ItemCode);"))

    def test_v5_world_loot_is_level_scoped_with_legacy_migration(self):
        self.assertIn("int32 SchemaVersion = 5;", self.save)
        self.assertIn("UPROPERTY(SaveGame) FName LevelId = NAME_None;", self.save)
        load = self.hero.split("bool AMoonfallHeroPawn::LoadRewardProgress()", 1)[1].split(
            "bool AMoonfallHeroPawn::SaveRewardProgress()", 1)[0]
        self.assertIn("if (Save->SchemaVersion >= 4)", load)
        self.assertIn("if (Save->SchemaVersion == 4) Accepted.LevelId = CurrentLevelId;", load)
        self.assertIn("if (Accepted.LevelId.IsNone()) continue;", load)
        self.assertIn("PendingWorldLoot.Add(Accepted);", load)
        register = self.hero.split("bool AMoonfallHeroPawn::RegisterWorldEquipmentDrops(", 1)[1].split(
            "void AMoonfallHeroPawn::RestorePendingWorldEquipmentDrops()", 1)[0]
        self.assertIn("Record.LevelId != CurrentLevelId", register)
        restore = self.hero.split("void AMoonfallHeroPawn::RestorePendingWorldEquipmentDrops()", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertIn("Record.LevelId != CurrentLevelId", restore)
        spawn = self.hero.split("void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[1].split(
            "bool AMoonfallHeroPawn::TryInteractWithNearbyKey()", 1)[0]
        self.assertIn("Record.LevelId = CurrentLevelId;", spawn)
        self.assertIn("UGameplayStatics::GetCurrentLevelName(this, true)", self.hero)
        gate = (ROOT / "MoonfallKeyGate.cpp").read_text()
        self.assertIn("Saved->SchemaVersion != 5", gate)

    def test_persisted_loot_pickup_requires_correct_level_and_identity(self):
        collect = self.hero.split("bool AMoonfallHeroPawn::CollectPersistedGroundEquipmentCode(", 1)[1].split(
            "bool AMoonfallHeroPawn::RegisterWorldEquipmentDrops(", 1)[0]
        self.assertIn("if (!GetWorld() || bRewardSaveLoadFailed) return false;", collect)
        self.assertIn("if (CurrentLevelId.IsNone()) return false;", collect)
        self.assertIn("Record.DropId == DropId && Record.ItemCode == ItemCode", collect)
        self.assertIn("Record.LevelId == CurrentLevelId", collect)
        self.assertLess(collect.index("Record.LevelId == CurrentLevelId"),
                        collect.index("PendingWorldLoot.RemoveAt(Index);"))
        self.assertIn("PendingWorldLoot.Insert(Record, Index);", collect)
        restore = self.hero.split("void AMoonfallHeroPawn::RestorePendingWorldEquipmentDrops()", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertIn("TMap<FGuid, AMoonfallGroundLoot*> SpawnedActors;", restore)
        self.assertIn("SpawnedActors.Find(Record.DropId)", restore)
        self.assertIn("if ((*ExistingActor)->GetItemCode() == Record.ItemCode) continue;", restore)
        self.assertIn("restoring saved item", restore)
        self.assertIn("(*ExistingActor)->Destroy();", restore)
        self.assertIn("SpawnedActors.Remove(Record.DropId);", restore)
        self.assertLess(restore.index("(*ExistingActor)->Destroy();"),
                        restore.index("BeginDeferredActorSpawnFromClass("))
        self.assertIn("SpawnedActors.Add(Record.DropId, Loot);", restore)
        self.assertEqual(restore.count("TActorIterator<AMoonfallGroundLoot>"), 1)

    def test_restore_removes_duplicate_and_orphaned_persistent_pickups(self):
        restore = self.hero.split("void AMoonfallHeroPawn::RestorePendingWorldEquipmentDrops()", 1)[1].split(
            "void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(", 1)[0]
        self.assertIn("TSet<FGuid> ExpectedIds;", restore)
        self.assertIn("if (Record.LevelId == CurrentLevelId) ExpectedIds.Add(Record.DropId);", restore)
        self.assertIn("if (!IsValid(Existing) || !Existing->GetPersistentDropId().IsValid()) continue;", restore)
        self.assertIn("!ExpectedIds.Contains(Id) || SpawnedActors.Contains(Id)", restore)
        self.assertIn("Existing->Destroy();", restore)
        self.assertLess(restore.index("Existing->Destroy();"),
                        restore.index("for (const FMoonfallWorldLootRecord& Record : PendingWorldLoot)", restore.index("TMap<FGuid")))
        self.assertIn("SpawnedActors.Add(Id, Existing);", restore)
        self.assertIn("(*ExistingActor)->Destroy();", restore)
        self.assertIn("SpawnedActors.Add(Record.DropId, Loot);", restore)
        self.assertEqual(restore.count("TActorIterator<AMoonfallGroundLoot>"), 1)

    def test_auto_loot_collects_all_reachable_items_without_a_six_item_limit(self):
        tick = self.hero.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::", 1)[0]
        pickup = self.hero.split("void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[1].split(
            "void AMoonfallHeroPawn::EquipBestInventoryAction()", 1)[0]
        self.assertIn("NextAutoLootAt = Now + 0.35f;", tick)
        self.assertIn("PickupNearestGroundLoot();", tick)
        self.assertIn("!bOptionsOpen && !bInventoryOpen", tick)
        self.assertIn("float NextAutoLootAt = 0.f;", self.hero_h)
        self.assertIn("for (TActorIterator<AMoonfallGroundLoot> It(GetWorld()); It; ++It)", pickup)
        self.assertIn("NearbyLoot.Add(Candidate);", pickup)
        self.assertNotIn("MaxLootPerPress", pickup)
        self.assertNotIn("MaxLootScanned", pickup)
        self.assertIn("InventoryItemCodes.Num() >= 4096", pickup)
        self.assertIn("FMath::Square(250.f)", pickup)
        self.assertIn("ECC_Visibility", pickup)
        self.assertIn("if (IsValid(Loot) && !Loot->TryCollect())", pickup)
        self.assertNotIn("TryInteractWithNearbyKey()", pickup)

    def test_auto_loot_recovers_missing_saved_drops_and_respects_save_failures(self):
        pickup = self.hero.split("void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[1].split(
            "void AMoonfallHeroPawn::EquipBestInventoryAction()", 1)[0]
        self.assertIn("for (int32 Attempt = 0; Attempt < 2; ++Attempt)", pickup)
        self.assertIn("NearbyLoot.Reset();", pickup)
        self.assertIn("if (!NearbyLoot.IsEmpty() || Attempt > 0) break;", pickup)
        self.assertIn("if (Record.LevelId != CurrentLevelId) continue;", pickup)
        self.assertIn("if (!bNearbySavedLoot) break;", pickup)
        self.assertIn("RestorePendingWorldEquipmentDrops();", pickup)
        self.assertEqual(pickup.count("RestorePendingWorldEquipmentDrops();"), 1)
        self.assertIn("NextAutoLootAt = GetWorld()->GetTimeSeconds() + 2.f;", pickup)
        self.assertIn("if (InventoryItemCodes.Num() >= 4096) break;", pickup)
        self.assertIn("GetWorld()->LineTraceSingleByChannel(", pickup)
        self.assertNotIn("EquipInventoryItem(", pickup)
        self.assertNotIn("SalvageInventoryItem(", pickup)

    def test_equipment_remains_manual_and_hud_shows_rewards(self):
        self.assertIn("void PickupNearestGroundLoot();", self.hero_h)
        self.assertIn("Loot->TryCollect()", self.hero)
        self.assertIn("Hero->Gold", self.hud)
        self.assertIn("Hero->CollectedKeys.Num()", self.hud)
        self.assertIn("void AMoonfallHeroPawn::EquipSelectedInventoryItem()", self.hero)

if __name__ == "__main__":
    unittest.main()
