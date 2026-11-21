"""Source contracts for living-world NPC save wiring (not native UE5 tests)."""
from pathlib import Path
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class LivingWorldNPCWiringTests(unittest.TestCase):
    def test_saved_bonds_drive_readonly_relationship_rank(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn('#include "MoonfallLivingWorldRules.h"', hero)
        self.assertIn("FString GetResidentRelationshipRank(FName ResidentId) const;", header)
        rank = hero.split("FString AMoonfallHeroPawn::GetResidentRelationshipRank(", 1)[1].split(
            "bool AMoonfallHeroPawn::ChangeResidentBond(", 1
        )[0]
        self.assertIn("MoonfallLivingWorld::RelationshipRank(GetResidentBond(ResidentId))", rank)
        for label in ("Hostile", "Wary", "Neutral", "Trusted", "Devoted"):
            self.assertIn('TEXT("' + label + '")', rank)
        self.assertNotIn("SaveWorldProgress(", rank)
        self.assertIn("Save->ResidentBonds = ResidentBonds;", hero)

    def test_community_outlook_uses_persisted_quest_and_shrine_ledgers(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("FString GetWorldCommunityOutlook() const;", header)
        outlook = hero.split("FString AMoonfallHeroPawn::GetWorldCommunityOutlook() const", 1)[1].split(
            "bool AMoonfallHeroPawn::IsSanctuaryRoomUnlocked(", 1)[0]
        for expected in (
            "for (const auto& Entry : ResidentQuestStates)",
            "if (Entry.Value == 2) ++CompletedContracts;",
            "Progress.completedContracts = CompletedContracts;",
            "Progress.shrineDiscovered = !DiscoveredShrineIds.IsEmpty();",
            "MoonfallLivingWorld::SettlementStatus(Progress)",
            "Endangered", "Recovering", "Flourishing",
        ):
            self.assertIn(expected, outlook)
        for forbidden in ("SaveWorldProgress(", "GetAllActorsOfClass(", "SpawnActor(",
                          "ResidentQuestStates.Add(", "DiscoveredShrineIds.Add("):
            self.assertNotIn(forbidden, outlook)
        self.assertIn("Save->ResidentQuestStates = ResidentQuestStates;", hero)
        self.assertIn("Save->DiscoveredShrineIds = DiscoveredShrineIds;", hero)
    def test_111_named_gear_and_tinker_level_requirements(self):
        catalog = (SOURCE / "MoonfallResidentCatalog.h").read_text(encoding="utf-8")
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        for expected in ("constexpr const char* Epithets[37]",
                         "constexpr const char* SlotTitles[3]",
                         "constexpr const char* NamePrefix(int index)",
                         "constexpr const char* NameSuffix(int index)",
                         "constexpr int CraftLevelRequired(int index)",
                         "1 + (item.tier - 1) / 5"):
            self.assertIn(expected, catalog)
        trade = hero.split("bool AMoonfallHeroPawn::PurchaseResidentWorkItem()", 1)[1].split(
            "// Bounded, persistent faction relationship API.", 1
        )[0]
        self.assertIn("MoonfallResidentCatalog::CraftLevelRequired(ResidentCatalogSelection)", trade)
        self.assertIn("if (bTinker && Level < RequiredLevel)", trade)
        self.assertLess(trade.index("if (bTinker && Level < RequiredLevel)"),
                        trade.index("const int32 NewIndex = InventoryItemCodes.Add(ItemCode)"))
        self.assertIn("MoonfallResidentCatalog::NamePrefix(Hero->ResidentCatalogSelection)", hud)
        self.assertIn("MoonfallResidentCatalog::NameSuffix(Hero->ResidentCatalogSelection)", hud)
        self.assertIn("Hero->Level < RequiredLevel", hud)

    def test_111_catalog_selection_mouse_controller_and_confirmation_integrity(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        catalog = (SOURCE / "MoonfallResidentCatalog.h").read_text(encoding="utf-8")
        self.assertIn("constexpr int Count = 111;", catalog)
        self.assertIn("int32 ResidentCatalogSelection = 0;", header)
        self.assertIn("bool StepResidentCatalog(int32 Direction);", header)
        for source in (hero, hud):
            self.assertIn('#include "MoonfallResidentCatalog.h"', source)
        browse = hero.split("bool AMoonfallHeroPawn::StepResidentCatalog(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetResidentCatalogItemCode()", 1
        )[0]
        for expected in ("ResidentContextSelection != 3",
                         "MoonfallResidentCatalog::Next(ResidentCatalogSelection, Direction)",
                         "bResidentTradePending = false;",
                         "ResidentTradeConfirmUntil = 0.f"):
            self.assertIn(expected, browse)
        self.assertIn("StepResidentCatalog(-1)", hero)
        self.assertIn("StepResidentCatalog(1)", hero)
        self.assertIn("Hero->ResidentCatalogSelection + 1", hud)
        self.assertIn("Catalog.tier, Catalog.rarity, Catalog.code", hud)
        self.assertIn("GetResidentCatalogPrice(bMerchant)", hud)
        self.assertIn("MouseX < Left + 61.f", hero)
        self.assertIn("MouseX > Left + PanelW - 61.f", hero)
        self.assertIn("if (Row == 3", hero)
        self.assertIn("if (Choice == 3) return PurchaseResidentWorkItem();", hero)

    def test_moral_consequences_use_read_only_character_meter_and_saved_npc_markers(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        sheet = hud.split("if (Hero->bCharacterStatsOpen)", 1)[1].split(
            "// Identical button rectangle to HitTestInventorySortButton", 1
        )[0]
        for expected in ("Good / Evil: %s (%+d)", "FMath::Clamp(Hero->MoralAlignment, -100, 100)",
                         "MoralScore < 0", "FMath::Abs(MoralScore) / 200.f",
                         "MoralMeterWidth"):
            self.assertIn(expected, sheet)
        for forbidden in ("SetActorLocation(", "SaveWorldProgress(", "SetMaterial(", "SpawnActor("):
            self.assertNotIn(forbidden, sheet)
        sign = npc.split("void AMoonfallWorldNPC::RefreshCouncilAssignmentSign(", 1)[1].split(
            "void AMoonfallWorldNPC::RefreshCouncilStationActivity(", 1
        )[0]
        for required in ("ResidentMoralChoices.Find(ResidentId)", "[HELPED]",
                         "[THREATENED]", "FColor(125, 231, 164)",
                         "FColor(232, 123, 133)"):
            self.assertIn(required, sign)
        choice = hero.split("bool AMoonfallHeroPawn::CommitResidentMoralChoice()", 1)[1].split(
            "bool AMoonfallHeroPawn::PurchaseResidentWorkItem()", 1
        )[0]
        self.assertLess(choice.index("if (!SaveWorldProgress())"),
                        choice.index("Resident->RefreshCouncilAssignmentSign(this);"))
        self.assertIn("I remember the help you gave me.", npc)
        self.assertIn("I remember how you threatened me.", npc)

    def test_morality_changes_crossroads_visuals_and_actual_trade_quotes(self):
        arena = (SOURCE / "MoonfallArena.cpp").read_text(encoding="utf-8")
        arena_header = (SOURCE / "MoonfallArena.h").read_text(encoding="utf-8")
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        rules = (SOURCE / "MoonfallMoralityRules.h").read_text(encoding="utf-8")
        for symbol in ("HopeSettlementDecor", "ShadowSettlementDecor",
                       "LastDisplayedMoralityTier"):
            self.assertIn(symbol, arena_header)
            self.assertIn(symbol, arena)
        refresh = arena.split("void AMoonfallArena::RefreshCouncilSettlementVisuals()", 1)[1].split(
            "void AMoonfallArena::CreateBlock(", 1
        )[0]
        for required in ("MoonfallMorality::WorldTier(Hero->MoralAlignment)",
                         "MoralTier == LastDisplayedMoralityTier",
                         "HopeSettlementDecor->SetVisibility(MoralTier > 0)",
                         "ShadowSettlementDecor->SetVisibility(MoralTier < 0)",
                         "if (bPolicyChanged)", "MoonfallMorality::WorldState("):
            self.assertIn(required, refresh)
        for forbidden in ("SaveWorldProgress(", "SpawnActor(", "SetActorLocation(",
                          "SetCollisionEnabled(", "SetTimer("):
            self.assertNotIn(forbidden, refresh)
        self.assertIn("constexpr int AdjustShopPrice(", rules)
        self.assertIn("MoonfallMorality::AdjustShopPrice(", hero)
        trade = hero.split("bool AMoonfallHeroPawn::PurchaseResidentWorkItem()", 1)[1].split(
            "// Bounded, persistent faction relationship API.", 1
        )[0]
        self.assertIn("const int32 Price = GetResidentCatalogPrice(bMerchant)", trade)
        self.assertIn("Salvage -= Price;", trade)
        self.assertIn("CatalogPrice < BasePrice", hud)
        self.assertIn("CatalogPrice > BasePrice", hud)

    def test_hero_morality_motifs_do_not_modify_character_artwork(self):
        cpp = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        for name in ("MoralOrnamentLeft", "MoralOrnamentRight", "MoralPresenceLight"):
            self.assertIn(name, header)
            self.assertIn(name, cpp)
        constructor = cpp.split("AMoonfallHeroPawn::AMoonfallHeroPawn()", 1)[1].split(
            "void AMoonfallHeroPawn::RefreshMoralPresenceVisuals()", 1
        )[0]
        for required in ('TEXT("MoralOrnamentLeft")', 'TEXT("MoralOrnamentRight")',
                         'TEXT("MoralPresenceLight")', "SetupAttachment(GetRootComponent())",
                         "ECollisionEnabled::NoCollision", "SetGenerateOverlapEvents(false)",
                         "SetCastShadow(false)", "SetCastShadows(false)",
                         "SetHiddenInGame(true)", "SetVisibility(false)"):
            self.assertIn(required, constructor)
        refresh = cpp.split("void AMoonfallHeroPawn::RefreshMoralPresenceVisuals()", 1)[1].split(
            "void AMoonfallHeroPawn::CacheStartingSanctuaryAnchor()", 1
        )[0]
        for required in ("MoonfallMorality::WorldTier(MoralAlignment)",
                         "Health > 0.f && bWorldProgressInitialized",
                         "FMath::Abs(Tier) >= 2", "bReduceMotion",
                         "bLastMoralVisualReduceMotion", "LastMoralVisualTier",
                         "SetRelativeRotation(", "SetLightColor(", "SetIntensity("):
            self.assertIn(required, refresh)
        for forbidden in ("SaveWorldProgress(", "SetSkeletalMesh(", "SetMaterial(",
                          "SpawnActor(", "SetActorLocation(", "GetAllActorsOfClass("):
            self.assertNotIn(forbidden, refresh)
        tick = cpp.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::", 1
        )[0]
        self.assertIn("MoralVisualRefreshElapsed >= 0.5f", tick)
        self.assertIn("RefreshMoralPresenceVisuals();", tick)
        self.assertIn("MoralVisualRefreshElapsed += SafeTimerDelta;", tick)

    def test_good_evil_choices_persist_and_rollback_atomically(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        save = (SOURCE / "MoonfallPersistentSave.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        self.assertIn("UPROPERTY() int32 Version = 14;", save)
        self.assertIn("UPROPERTY() int32 MoralAlignment = 0;", save)
        self.assertIn("UPROPERTY() TMap<FName, int32> ResidentMoralChoices;", save)
        self.assertIn("Save->MoralAlignment = MoralAlignment;", hero)
        self.assertIn("Save->ResidentMoralChoices = ResidentMoralChoices;", hero)
        self.assertIn("Save->Version >= 14", hero)
        self.assertIn("Save->Version > 14", hero)
        self.assertIn("MoonfallMorality::IsValidAction(Entry.Value)", hero)
        self.assertIn("ResidentMoralChoices.Num() > 13", hero)
        self.assertIn("bool CommitResidentMoralChoice();", header)
        self.assertIn("if (Choice == 4) return CommitResidentMoralChoice();", hero)
        self.assertIn("const FString Names[5]", hud)
        self.assertIn("for (int32 Row = 0; Row < 5; ++Row)", hud)
        self.assertIn("for (int32 Row = 0; Row < 5; ++Row)", hero)
        self.assertIn("Hero->MoralAlignment", hud)
        self.assertIn("People have heard of your kindness.", npc)
        self.assertIn("People fear what you might do next.", npc)
        choice = hero.split("bool AMoonfallHeroPawn::CommitResidentMoralChoice()", 1)[1].split(
            "bool AMoonfallHeroPawn::PurchaseResidentWorkItem()", 1
        )[0]
        for required in ("Resident->CanHeroInteract(this)",
                         "ResidentMoralChoices.Contains(Resident->ResidentId)",
                         "ResidentMoralConfirmUntil = Now + 6.f;",
                         "MoonfallMorality::ResidentActionDelta(bAid)",
                         "MoonfallMorality::ResidentSalvageDelta(bAid)",
                         "MoonfallMorality::ResidentBondDelta(bAid)",
                         "MoonfallMorality::ResidentReputationDelta(bAid)",
                         "ResidentMoralChoices.Add(Id, bAid ? 1 : -1)",
                         "if (!SaveWorldProgress())",
                         "MoralAlignment = PreviousMorality;",
                         "Salvage = PreviousSalvage;",
                         "ResidentMoralChoices.Remove(Id);"):
            self.assertIn(required, choice)
        self.assertLess(choice.index("ResidentMoralChoices.Add(Id, bAid ? 1 : -1)"),
                        choice.index("if (!SaveWorldProgress())"))
        self.assertIn("bResidentMoralActionPending = false;", hero)
        self.assertIn("ToggleResidentMoralChoice(-1)", hero)
        self.assertIn("ToggleResidentMoralChoice(1)", hero)

    def test_merchant_and_tinker_transactions_are_confirmed_and_atomic(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("bool PurchaseResidentWorkItem();", header)
        self.assertIn("bool bResidentTradePending = false;", header)
        self.assertIn("if (Choice == 3) return PurchaseResidentWorkItem();", hero)
        method = hero.split("bool AMoonfallHeroPawn::PurchaseResidentWorkItem()", 1)[1].split(
            "// Bounded, persistent faction relationship API.", 1
        )[0]
        for expected in ("Resident->CanHeroInteract(this)",
                         "bPersistentSaveWritesBlocked", "CouncilDecisionId < 0",
                         "InventoryItemCodes.Num() >= 10000", "const auto Stock = MoonfallResidentCatalog::Get(ResidentCatalogSelection)",
                         "const int32 Price = GetResidentCatalogPrice(bMerchant)",
                         "const int32 ItemCode = Stock.code",
                         "bResidentTradePending = true;", "ResidentTradeConfirmUntil = Now + 6.f",
                         "Salvage -= Price;", "InventoryItemCodes.Add(ItemCode)",
                         "if (!SaveWorldProgress())", "InventoryItemCodes.RemoveAt(NewIndex)",
                         "Salvage = PreviousSalvage;", "InventorySelection = OldSelection;"):
            self.assertIn(expected, method)
        self.assertLess(method.index("InventoryItemCodes.Add(ItemCode)"),
                        method.index("if (!SaveWorldProgress())"))
        for forbidden in ("ChangeFactionReputation(", "ResidentBonds.Add(", "EquipInventoryItem("):
            self.assertNotIn(forbidden, method)
        self.assertIn("const FString Names[5]", hud)
        self.assertIn("for (int32 Row = 0; Row < 5; ++Row)", hud)
        self.assertIn("for (int32 Row = 0; Row < 5; ++Row)", hero)
        self.assertIn("ResidentContextSelection = (ResidentContextSelection + 4) % 5", hero)
        self.assertIn("ResidentContextSelection = (ResidentContextSelection + 1) % 5", hero)
        self.assertIn("FMath::Min(520.f, FMath::Max(390.f, Canvas->SizeY - 24.f))", hud)
        self.assertIn("FMath::Min(520.f, FMath::Max(390.f, Height - 24.f))", hero)

    def test_stationary_professions_use_existing_refresh_without_save_effects(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallWorldNPC.h").read_text(encoding="utf-8")
        arena = (SOURCE / "MoonfallArena.cpp").read_text(encoding="utf-8")
        arena_h = (SOURCE / "MoonfallArena.h").read_text(encoding="utf-8")
        self.assertIn("void RefreshCouncilStationActivity(const AMoonfallHeroPawn* Hero);", header)
        self.assertIn("TArray<TWeakObjectPtr<AMoonfallWorldNPC>> CouncilStationResidents;", arena_h)
        station = npc.split("void AMoonfallWorldNPC::RefreshCouncilStationActivity(", 1)[1].split(
            "bool AMoonfallWorldNPC::ConfigureCouncilWorkRoute(", 1
        )[0]
        for expected in ('SocialRole == FName(TEXT("Merchant"))',
                         'SocialRole == FName(TEXT("Tinker"))',
                         'SettlementId == FName(TEXT("Crossroads"))',
                         "Hero->CouncilDecisionId >= 0",
                         "CouncilWorkMarker->SetVisibility(false);",
                         "CouncilWorkMarker->SetVisibility(true);",
                         "CouncilStationVisualStep = (CouncilStationVisualStep + 1) % 4;",
                         "CouncilWorkMarker->SetRelativeScale3D("):
            self.assertIn(expected, station)
        for forbidden in ("SaveWorldProgress(", "SpawnActor(", "SetActorLocation(",
                          "SetTimer(", "ChangeFactionReputation("):
            self.assertNotIn(forbidden, station)
        refresh = arena.split("void AMoonfallArena::RefreshCouncilSettlementVisuals()", 1)[1].split(
            "void AMoonfallArena::CreateBlock(", 1
        )[0]
        self.assertIn("Resident->RefreshCouncilStationActivity(Hero);", refresh)
        self.assertLess(refresh.index("Resident->RefreshCouncilStationActivity(Hero);"),
                        refresh.index("if (Policy == LastDisplayedCouncilDecision"))
        self.assertIn("CouncilStationResidents.Add(Person);", arena)
        self.assertIn("RefreshCouncilSettlementVisuals, 2.0f, true", arena)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", npc)

    def test_council_work_effects_are_role_specific_and_hide_on_interruption(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        movement = npc.split("void AMoonfallWorldNPC::AdvanceCouncilWorkStep(", 1)[1].split(
            "FString AMoonfallWorldNPC::GetCouncilWorkPhase()", 1
        )[0]
        for required in ("Hero->FocusedResident.Get() == this",
                         "CouncilWorkMarker->SetVisibility(false);",
                         "PlayerForPause.ContainsNaN()",
                         "FMath::Square(300.f)",
                         'SocialRole == FName(TEXT("Scout"))',
                         "FVector(0.18f, 0.18f, 0.06f)",
                         "FVector(0.10f, 0.10f, 0.22f)",
                         "10.f * FMath::Sin(CouncilWorkVisualPhase)",
                         "45.f * FMath::Sin(CouncilWorkVisualPhase)"):
            self.assertIn(required, movement)
        self.assertLess(movement.index("PlayerForPause.ContainsNaN()"),
                        movement.index("CouncilWorkHoldSeconds - StepSeconds"))
        self.assertLess(movement.index("CouncilWorkMarker->SetVisibility(false);"),
                        movement.index("CouncilWorkMarker->SetVisibility(bWorking)"))
        for forbidden in ("SaveWorldProgress(", "SpawnActor(", "ChangeFactionReputation("):
            self.assertNotIn(forbidden, movement)

    def test_council_work_marker_animates_only_during_work_stops(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallWorldNPC.h").read_text(encoding="utf-8")
        self.assertIn("UStaticMeshComponent* CouncilWorkMarker;", header)
        self.assertIn("float CouncilWorkVisualPhase = 0.f;", header)
        constructor = npc.split("AMoonfallWorldNPC::AMoonfallWorldNPC()", 1)[1].split(
            "int32 AMoonfallWorldNPC::GetBondWithHero(", 1
        )[0]
        for required in ('TEXT("CouncilWorkMarker")', "CouncilWorkMarker->SetupAttachment(Body)",
                         "ECollisionEnabled::NoCollision", "SetGenerateOverlapEvents(false)",
                         "CouncilWorkMarker->SetVisibility(false)"):
            self.assertIn(required, constructor)
        movement = npc.split("void AMoonfallWorldNPC::AdvanceCouncilWorkStep(", 1)[1].split(
            "FString AMoonfallWorldNPC::GetCouncilWorkPhase()", 1
        )[0]
        for required in ("bCouncilWorkAtPost && CouncilWorkHoldSeconds > 0.f",
                         "CouncilWorkMarker->SetVisibility(bWorking)",
                         "CouncilWorkVisualPhase + StepSeconds * 2.f",
                         "CouncilWorkMarker->SetRelativeLocation(bScout",
                         "CouncilWorkMarker->SetRelativeRotation(",
                         "CouncilWorkMarker->SetVisibility(false)"):
            self.assertIn(required, movement)
        self.assertNotIn("SaveWorldProgress(", movement)
        self.assertNotIn("SpawnActor(", movement)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", npc)

    def test_council_work_at_post_has_bounded_rest_and_ui_feedback(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallWorldNPC.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("FString GetCouncilWorkPhase() const;", header)
        move = npc.split("void AMoonfallWorldNPC::AdvanceCouncilWorkStep(", 1)[1].split(
            "FString AMoonfallWorldNPC::GetCouncilWorkPhase()", 1
        )[0]
        for required in ("CouncilWorkHoldSeconds > 0.f",
                         "CouncilWorkHoldSeconds - StepSeconds",
                         "bCouncilWorkReturning = !bCouncilWorkReturning",
                         "CouncilWorkHoldSeconds = bCouncilWorkAtPost ? 5.f : 2.f",
                         "Hero->FocusedResident.Get() == this"):
            self.assertIn(required, move)
        self.assertLess(move.index("CouncilWorkHoldSeconds > 0.f"),
                        move.index("SetActorLocation(Next, true, &Blocked"))
        phase = npc.split("FString AMoonfallWorldNPC::GetCouncilWorkPhase()", 1)[1].split(
            "FString AMoonfallWorldNPC::GetCouncilAssignment(", 1
        )[0]
        for expected in ("At assigned station", "Working at post",
                         "Resting at home", "Returning to station",
                         "Walking to work"):
            self.assertIn(expected, phase)
        self.assertIn("Resident->GetCouncilWorkPhase()", hud)
        self.assertNotIn("SaveWorldProgress(", move + phase)
        self.assertNotIn("SpawnActor(", move + phase)

    def test_council_work_routes_are_navmesh_validated_and_rate_limited(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallWorldNPC.h").read_text(encoding="utf-8")
        arena = (SOURCE / "MoonfallArena.cpp").read_text(encoding="utf-8")
        arena_h = (SOURCE / "MoonfallArena.h").read_text(encoding="utf-8")
        self.assertIn("bool ConfigureCouncilWorkRoute(const FVector& EndOffset);", header)
        self.assertIn("void AdvanceCouncilWorkStep(const AMoonfallHeroPawn* Hero, float StepSeconds);", header)
        configure = npc.split("bool AMoonfallWorldNPC::ConfigureCouncilWorkRoute(", 1)[1].split(
            "void AMoonfallWorldNPC::AdvanceCouncilWorkStep(", 1
        )[0]
        for required in ("UNavigationSystemV1", "ProjectPointToNavigation(",
                         "FindPathToLocationSynchronously(", "Path->IsPartial()",
                         "Path->GetPathLength() > 180.f", "bCouncilWorkRouteValid = true;"):
            self.assertIn(required, configure)
        move = npc.split("void AMoonfallWorldNPC::AdvanceCouncilWorkStep(", 1)[1].split(
            "FString AMoonfallWorldNPC::GetCouncilAssignment(", 1
        )[0]
        for required in ("!bCouncilWorkRouteValid", "Hero->CouncilDecisionId < 0",
                         "Hero->FocusedResident.Get() == this", "FMath::Square(300.f)",
                         "55.f * StepSeconds", "SetActorLocation(Next, true, &Blocked",
                         "bCouncilWorkRouteValid = false"):
            self.assertIn(required, move)
        for forbidden in ("SaveWorldProgress(", "SpawnActor(", "OnDialogueStarted(",
                          "ResidentBonds.Add(", "ChangeFactionReputation("):
            self.assertNotIn(forbidden, configure + move)
        self.assertIn("TArray<TWeakObjectPtr<AMoonfallWorldNPC>> CouncilMobileResidents;", arena_h)
        self.assertIn("CouncilMobileResidents.Num() < 4", arena)
        self.assertIn("FVector(100.f, 0.f, 0.f) : FVector(-100.f, 0.f, 0.f)", arena)
        self.assertIn("StepCouncilWorkRoutes, 0.2f, true", arena)
        self.assertIn("if (!CouncilMobileResidents.IsEmpty())", arena)
        self.assertIn("Path->PathPoints.Num() != 2", configure)
        self.assertIn("MoonfallCouncilRoute", configure)
        self.assertIn("Home + EyeOffset, Destination + EyeOffset, ECC_Visibility", configure)
        self.assertIn("MoonfallCouncilStep", move)
        self.assertIn("Current + EyeOffset, Next + EyeOffset, ECC_Visibility", move)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", npc)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", arena)

    def test_council_world_labels_only_refresh_on_policy_changes(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallWorldNPC.h").read_text(encoding="utf-8")
        arena = (SOURCE / "MoonfallArena.cpp").read_text(encoding="utf-8")
        self.assertIn("UTextRenderComponent* CouncilAssignmentSign;", header)
        self.assertIn("void RefreshCouncilAssignmentSign(const AMoonfallHeroPawn* Hero);", header)
        self.assertIn('CreateDefaultSubobject<UTextRenderComponent>(TEXT("CouncilAssignmentSign"))', npc)
        method = npc.split("void AMoonfallWorldNPC::RefreshCouncilAssignmentSign(", 1)[1].split(
            "bool AMoonfallWorldNPC::ConfigureCouncilWorkRoute(", 1
        )[0]
        for expected in ("ECollisionEnabled::NoCollision", "SetGenerateOverlapEvents(false)",
                         "CouncilAssignmentSign->SetVisibility(false)", "CouncilAssignmentSign->SetVisibility(true)",
                         'SettlementId == FName(TEXT("Crossroads"))', "Hero->CouncilDecisionId >= 0",
                         "GetCouncilAssignment(Hero).Left(24)"):
            self.assertIn(expected, npc)
        for forbidden in ("SaveWorldProgress(", "SpawnActor(", "SetActorLocation(", "GetTimerManager()"):
            self.assertNotIn(forbidden, method)
        refresh = arena.split("void AMoonfallArena::RefreshCouncilSettlementVisuals()", 1)[1].split(
            "void AMoonfallArena::CreateBlock(", 1
        )[0]
        self.assertLess(refresh.index("if (Policy == LastDisplayedCouncilDecision"),
                        refresh.index("TActorIterator<AMoonfallWorldNPC>"))
        self.assertIn("It->RefreshCouncilAssignmentSign(Hero);", refresh)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", npc)

    def test_council_assignments_are_readonly_role_based_and_visible_in_npc_menu(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallWorldNPC.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("FString GetCouncilAssignment(const AMoonfallHeroPawn* Hero) const;", header)
        method = npc.split("FString AMoonfallWorldNPC::GetCouncilAssignment(", 1)[1].split(
            "bool AMoonfallWorldNPC::SharesRaceWith(", 1
        )[0]
        for required in ('SettlementId != FName(TEXT("Crossroads"))',
                         "Hero->CouncilDecisionId < 0", "Hero->CouncilDecisionId > 2",
                         "return GetCurrentActivity();",
                         "Protecting Residents", "Escorting Travelers", "Supplying Families",
                         "Repairing Machinery", "Stocking Building Supplies",
                         "Monitoring the Borders", "Maintaining Defenses"):
            self.assertIn(required, method)
        for forbidden in ("SaveWorldProgress(", "ChangeFactionReputation(", "SpawnActor(",
                          "SetActorLocation(", "SetTimer(", "ResidentBonds.Add("):
            self.assertNotIn(forbidden, method)
        self.assertIn("Resident->GetCouncilAssignment(Hero)", hud)
        self.assertIn("bShowResidentFeedback", hud)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", npc)

    def test_completed_contract_npcs_acknowledge_durable_council_policy(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        method = npc.split("bool AMoonfallWorldNPC::TryTalk(", 1)[1].split(
            "bool AMoonfallWorldNPC::OfferResidentQuest(", 1
        )[0]
        for phrase in ("Hero->GetResidentQuestState(ResidentId) == 2",
                       "Hero->CouncilDecisionId >= 0",
                       "switch (Hero->CouncilDecisionId)",
                       "The council chose to protect our people.",
                       "The council funded rebuilding.",
                       "The council ordered stronger borders."):
            self.assertIn(phrase, method)
        for forbidden in ("ChangeFactionReputation(", "ChangeResidentBond(",
                          "SaveWorldProgress(", "SpawnActor("):
            self.assertNotIn(forbidden, method)

    def test_council_world_props_follow_saved_policy_without_collision_or_tick(self):
        arena = (SOURCE / "MoonfallArena.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallArena.h").read_text(encoding="utf-8")
        for component in ("CouncilProtectionDecor", "CouncilReconstructionDecor",
                          "CouncilFortificationDecor"):
            self.assertIn(component, header)
            self.assertIn('CreateDefaultSubobject<UInstancedStaticMeshComponent>', arena)
        for required in ("CouncilDecisionId", "RefreshCouncilSettlementVisuals()",
                         "CouncilProtectionDecor->SetVisibility(Policy == 0);",
                         "CouncilReconstructionDecor->SetVisibility(Policy == 1);",
                         "CouncilFortificationDecor->SetVisibility(Policy == 2);",
                         "CouncilWorldSign->SetText(", "SetTimer(CouncilVisualRefreshTimer",
                         "ECollisionEnabled::NoCollision", "SetGenerateOverlapEvents(false)"):
            self.assertIn(required, arena)
        method = arena.split("void AMoonfallArena::RefreshCouncilSettlementVisuals()", 1)[1].split(
            "void AMoonfallArena::CreateBlock(", 1
        )[0]
        for forbidden in ("SaveWorldProgress(", "SpawnActor(", "FactionReputation.Add(",
                          "TakeSpellDamage(", "SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)"):
            self.assertNotIn(forbidden, method)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", arena)

    def test_council_v13_commit_is_single_shot_and_save_rollback_safe(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        save = (SOURCE / "MoonfallPersistentSave.h").read_text(encoding="utf-8")
        inputs = (SOURCE.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn("UPROPERTY() int32 Version = 14;", save)
        self.assertIn("UPROPERTY() int32 CouncilDecisionId = -1;", save)
        self.assertIn("Save->CouncilDecisionId = CouncilDecisionId;", hero)
        self.assertIn("Save->Version >= 13 ? Save->CouncilDecisionId : -1", hero)
        self.assertIn("Save->Version > 14", hero)
        self.assertIn("Save->CouncilDecisionId < -1", hero)
        self.assertIn("Save->CouncilDecisionId > 2", hero)
        self.assertIn("int32 CouncilDecisionId = -1;", header)
        self.assertIn('ActionName="CouncilConfirm",Key=F9', inputs)
        self.assertIn("ConfirmCouncilDecisionInput", header)
        self.assertIn("void AMoonfallHeroPawn::ConfirmCouncilDecisionInput()", hero)
        confirm = hero.split("bool AMoonfallHeroPawn::ConfirmCouncilDecision()", 1)[1].split(
            "void AMoonfallHeroPawn::CloseCouncilPointer()", 1
        )[0]
        self.assertIn("CouncilDecisionId != -1", confirm)
        self.assertIn("!bCouncilDecisionConfirmArmed", confirm)
        self.assertIn("bCouncilDecisionConfirmArmed = true;", confirm)
        self.assertIn("CouncilDecisionId = Selected;", confirm)
        self.assertIn("if (!SaveWorldProgress())", confirm)
        self.assertIn("CouncilDecisionId = OldDecision;", confirm)
        self.assertLess(confirm.index("CouncilDecisionId = Selected;"), confirm.index("if (!SaveWorldProgress())"))
        self.assertLess(confirm.index("if (!SaveWorldProgress())"), confirm.index("CouncilDecisionId = OldDecision;"))
        self.assertNotIn("FactionReputation.Add(", confirm)

    def test_council_pointer_click_preview_is_modal_and_safe(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        inputs = (SOURCE.parents[1] / "Config" / "DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="CouncilPointer",Key=F8', inputs)
        self.assertIn('BindAction(TEXT("CouncilPointer")', hero)
        self.assertIn("bool bCouncilPointerOpen = false;", header)
        toggle = hero.split("void AMoonfallHeroPawn::ToggleCouncilPointer()", 1)[1].split(
            "bool AMoonfallHeroPawn::SelectCouncilPreview(", 1
        )[0]
        self.assertIn("!IsSanctuaryRoomUnlocked(4)", toggle)
        self.assertIn("!IsNearStartingSanctuary()", toggle)
        self.assertIn("PC->bShowMouseCursor = true;", toggle)
        self.assertIn("bCouncilPointerOpen = false;", hero)
        self.assertIn("PC->bShowMouseCursor = bOptionsOpen || bInventoryOpen || bAtlasOpen", hero)
        click = hero.split("void AMoonfallHeroPawn::ResidentContextMouseClick()", 1)[1].split(
            "void AMoonfallHeroPawn::TalkToNearestNPC()", 1
        )[0]
        mode = click.split("if (bCouncilPointerOpen) {", 1)[1].split(
            "if (!bResidentContextOpen) { CastLeft(); return; }", 1
        )[0]
        for expected in ("PC->bShowMouseCursor", "ViewW < 1280 || ViewH < 700",
                         "PointerY >= RowY && PointerY <= RowY + 25.f",
                         "SelectCouncilPreview(Choice);"):
            self.assertIn(expected, mode)
        self.assertNotIn("CastLeft();", mode)
        self.assertNotIn("SaveWorldProgress(", mode)
        self.assertIn("bCouncilPointerOpen && (Health <= 0.f || !IsNearStartingSanctuary()", hero)
        self.assertIn("bResidentContextOpen || bCouncilPointerOpen || !GetWorld()", hero)
        self.assertIn('if (bCouncilPointerOpen) { CloseCouncilPointer(); return; }', hero)
        self.assertIn('const TCHAR* ChoiceLabels[3]', hud)
        self.assertIn('LedgerY + 274.f, 98.f, 25.f', hud)

    def test_council_selection_is_controller_accessible_but_never_commits(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("bool SelectCouncilPreview(int32 DecisionId);", header)
        self.assertIn("int32 CouncilPreviewSelection = 0;", header)
        selection = hero.split("bool AMoonfallHeroPawn::SelectCouncilPreview(int32 DecisionId)", 1)[1].split(
            "bool AMoonfallHeroPawn::AcceptResidentQuest(", 1
        )[0]
        for required in ("DecisionId < 0 || DecisionId > 2", "IsNearStartingSanctuary()",
                         "IsSanctuaryRoomUnlocked(4)", "CouncilPreviewSelection = DecisionId;"):
            self.assertIn(required, selection)
        for forbidden in ("SaveWorldProgress(", "ChangeFactionReputation(",
                          "FactionReputation.Add(", "UGameplayStatics::SaveGameToSlot("):
            self.assertNotIn(forbidden, selection)
        left = hero.split("void AMoonfallHeroPawn::OptionsLeft()", 1)[1].split(
            "void AMoonfallHeroPawn::OptionsRight()", 1
        )[0]
        right = hero.split("void AMoonfallHeroPawn::OptionsRight()", 1)[1].split(
            "void AMoonfallHeroPawn::AdjustOption(", 1
        )[0]
        self.assertIn("SelectCouncilPreview((CouncilPreviewSelection + 2) % 3)", left)
        self.assertIn("SelectCouncilPreview((CouncilPreviewSelection + 1) % 3)", right)
        self.assertIn("Hero->PreviewCouncilDecision(Selected)", hud)
        self.assertIn("D-pad: preview | F8: mouse | F9 twice: save", hud)

    def test_sanctuary_hud_uses_actual_progress_and_preserves_small_screens(self):
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        block = hud.split("// The sanctuary ledger is read-only:", 1)[1].split(
            "// Compact circular resource HUD at bottom left", 1
        )[0]
        for expected in ("bSanctuaryObjective", "Canvas->SizeX >= 1280.f",
                         "Canvas->SizeY >= 700.f", "!Hero->bAtlasOpen",
                         "!Hero->bOptionsOpen", "Hero->IsSanctuaryRoomUnlocked(Room)",
                         "Hero->GetWorldCommunityOutlook()",
                         "Hero->IsSanctuaryRoomUnlocked(4)",
                         "COUNCIL CHOICES (PREVIEW / CONFIRM)"):
            self.assertIn(expected, block)
        for forbidden in ("SaveWorldProgress(", "SpawnActor(", "ChangeFactionReputation(",
                          "ResidentQuestStates.Add("):
            self.assertNotIn(forbidden, block)

    def test_sanctuary_unlock_uses_saved_firstlight_and_proven_victories(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("bool IsSanctuaryRoomUnlocked(int32 RoomId) const;", header)
        method = hero.split("bool AMoonfallHeroPawn::IsSanctuaryRoomUnlocked(int32 RoomId) const", 1)[1].split(
            "FString AMoonfallHeroPawn::PreviewCouncilDecision(", 1
        )[0]
        for expected in ('HasDiscoveredShrine(FName(TEXT("sanctuary_firstlight")))',
                         "GetTotalDefeatedDungeonBosses()", "Entry.Value == 2",
                         "MoonfallLivingWorld::SanctuaryRoomUnlocked(",
                         "RoomId > static_cast<int32>(MoonfallLivingWorld::SanctuaryRoom::Portal)"):
            self.assertIn(expected, method)
        self.assertNotIn("SaveWorldProgress(", method)
        self.assertNotIn("SpawnActor(", method)

    def test_council_choice_is_readonly_preview(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        self.assertIn("FString PreviewCouncilDecision(int32 DecisionId) const;", header)
        method = hero.split("FString AMoonfallHeroPawn::PreviewCouncilDecision(int32 DecisionId) const", 1)[1].split(
            "FString AMoonfallHeroPawn::GetConfirmedCouncilPolicy() const", 1
        )[0]
        for required in ("MoonfallLivingWorld::Decide(Decision)", "Protect Residents",
                         "Fund Reconstruction", "Fortify Borders", "preview only",
                         "Invalid decision"):
            self.assertIn(required, method)
        for forbidden in ("SaveWorldProgress(", "ChangeFactionReputation(",
                          "FactionReputation.Add(", "ResidentQuestStates.Add("):
            self.assertNotIn(forbidden, method)

    def test_mixed_school_cast_is_single_transactional_hit(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        cast = hero.split("void AMoonfallHeroPawn::CastBoth()", 1)[1].split(
            "void AMoonfallHeroPawn::BreakGuard()", 1)[0]
        self.assertIn("void OnSpellWeaved(int32 LeftSchool, int32 RightSchool, int32 HybridType);", header)
        for required in ("LeftSpell != RightSpell", "MoonfallLivingWorld::Weave(LeftSpell, RightSpell)",
                         "Mana < RequiredMana", "TryCast(LeftSpell, 0, 1.25f)",
                         "Mana -= Costs[RightSpell] * 0.5f;",
                         "LastCastTime[1] = LastCastTime[0];",
                         "OnSpellWeaved(LeftSpell, RightSpell, static_cast<int32>(Pair))",
                         "if (TryCast(LeftSpell, 0, 2.f))"):
            self.assertIn(required, cast)
        self.assertLess(cast.index("Mana < RequiredMana"), cast.index("TryCast(LeftSpell, 0, 1.25f)"))
        self.assertLess(cast.index("TryCast(LeftSpell, 0, 1.25f)"), cast.index("Mana -= Costs[RightSpell] * 0.5f;"))
        self.assertLess(cast.index("Mana -= Costs[RightSpell] * 0.5f;"), cast.index("OnSpellWeaved("))
        self.assertNotIn("TryCast(RightSpell", cast)
        self.assertNotIn("SaveWorldProgress(", cast)
    def test_weapon_resonance_reads_valid_equipped_gear_and_saved_boss_mask(self):
        hero = (SOURCE / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
        hud = (SOURCE / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("FString GetEquippedWeaponResonance() const;", header)
        method = hero.split("FString AMoonfallHeroPawn::GetEquippedWeaponResonance() const", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetItemPower(", 1)[0]
        for required in ("InventoryItemCodes.Contains(EquippedWeaponCode)",
                         "DecodeEquippedItem(EquippedWeaponCode, 0, Trait, Rarity, Tier)",
                         "DefeatedDungeonBossMask",
                         "MoonfallLivingWorld::BossResonance(DefeatedBosses, Tier, Rarity)",
                         "Victories &= Victories - 1u;",
                         "Awakened", "Ascendant", "Mythic"):
            self.assertIn(required, method)
        for forbidden in ("SaveWorldProgress(", "InventoryItemCodes.Add(", "EquippedWeaponCode ="):
            self.assertNotIn(forbidden, method)
        self.assertIn("Hero->GetEquippedWeaponResonance()", hud)
        self.assertIn("Save->DefeatedDungeonBossMask = DefeatedDungeonBossMask;", hero)
    def test_dialogue_reflects_completed_contract_and_never_grants_bond(self):
        npc = (SOURCE / "MoonfallWorldNPC.cpp").read_text(encoding="utf-8")
        section = npc.split("bool AMoonfallWorldNPC::TryTalk(", 1)[1].split(
            "bool AMoonfallWorldNPC::OfferResidentQuest(", 1
        )[0]
        self.assertIn("GetResidentQuestState(ResidentId) == 2", section)
        self.assertIn("GetResidentRelationshipRank(ResidentId)", section)
        self.assertIn('Rank == TEXT("Trusted")', section)
        self.assertIn('Rank == TEXT("Devoted")', section)
        self.assertNotIn("ChangeResidentBond(", section)
        self.assertNotIn("SaveWorldProgress(", section)


if __name__ == "__main__":
    unittest.main()
