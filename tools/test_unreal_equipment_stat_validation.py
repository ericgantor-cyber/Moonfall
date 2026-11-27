"""Source-level Unreal equipment validation checks; native compilation still required."""
from pathlib import Path
import unittest
SOURCE = (Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Source" / "MoonfallUnreal" / "MoonfallHeroPawn.cpp")
HEADER = SOURCE.with_name("MoonfallHeroPawn.h")
class EquipmentStatSafety(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text(encoding="utf-8")
    def test_decoder_guards_corrupt_codes(self):
        body = self.source.split("bool DecodeEquippedItem(", 1)[1].split("int32 ValidEquippedTier(", 1)[0]
        self.assertIn("ExpectedSlot >= MoonfallEquipmentLayout::ActiveSlotCount", body)
        self.assertIn("MoonfallEquipmentLayout::ValidFullItem(Code, ExpectedSlot)", body)
        policy = SOURCE.with_name("MoonfallGearAdvisorPolicy.h").read_text(encoding="utf-8")
        for token in ("Code <= 0", "EncodedSlot == Slot", "Affix >= 0 && Affix <= 3",
                      "Rarity >= 0", "Rarity <= 5", "Tier >= 1", "Tier <= 25",
                      "(Rarity != 0 || Affix == 0)",
                      "Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier",
                      "MoonfallGearAdvisor::Valid(Code-Slot*10000,0)"):
            self.assertIn(token, policy)
    def test_stat_bonuses_validate_ownership(self):
        getter = self.source.split("int32 AMoonfallHeroPawn::GetEquipmentSlotCode(", 1)[1].split("int32 AMoonfallHeroPawn::GetValidatedEquippedTier(", 1)[0]
        tier = self.source.split("int32 AMoonfallHeroPawn::GetValidatedEquippedTier(", 1)[1].split("int32 AMoonfallHeroPawn::GetItemPower(", 1)[0]
        self.assertIn("GetValidatedEquippedTier(Slot) > 0 ? Code : 0", getter)
        self.assertIn("InventoryItemCodes.Contains(Code) ? ValidEquippedTier(Code, Slot) : 0", tier)
        self.assertIn("Slot < 0 || Slot >= MoonfallEquipmentLayout::ActiveSlotCount", tier)
        self.assertIn("GetRawEquipmentSlotCode(Slot)", getter)
        self.assertIn("GetRawEquipmentSlotCode(Slot)", tier)
        raw = self.source.split("int32 AMoonfallHeroPawn::GetRawEquipmentSlotCode(", 1)[1].split("int32 AMoonfallHeroPawn::GetEquipmentSlotCode(", 1)[0]
        for slot, field in enumerate(("EquippedWeaponCode", "EquippedArmorCode", "EquippedAccessoryCode")):
            self.assertIn(f"case {slot}: return {field};", raw)
        self.assertIn("Slot >= MoonfallEquipmentLayout::FullSlotCount) return 0;", raw)
        self.assertIn("Slot < MoonfallEquipmentLayout::LegacySlotCount", raw)
        self.assertIn("return ExtraEquippedCodes[Slot-MoonfallEquipmentLayout::LegacySlotCount];", raw)
        for name, slot, trait in (("Weapon", 0, 1), ("Armor", 1, 2), ("Accessory", 2, 3)):
            self.assertIn(f"Owned{name}Code = GetEquipmentSlotCode({slot})", self.source)
            self.assertIn(f"ValidEquippedTraitRarity(Owned{name}Code, {slot}, {trait})", self.source)
            self.assertIn(f"ValidEquippedTier(Owned{name}Code, {slot})", self.source)
        self.assertIn("const float Tier = static_cast<float>(GetValidatedEquippedTier(0));", self.source)
    def test_expanded_previews_use_all_slots_and_require_a_spare_ring_copy(self):
        source = self.source
        policy = SOURCE.with_name("MoonfallGearAdvisorPolicy.h").read_text(encoding="utf-8")
        self.assertIn("FullCandidateImprovement", policy)
        self.assertIn("FullWornCopies(Candidate,Worn,ActiveSlots)+1", policy)
        score = source.split("int32 AMoonfallHeroPawn::GetEquipmentUpgradeScore(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetRecommendedEquipmentIndex()", 1)[0]
        self.assertIn("FFullLoadout EquippedCodes", score)
        self.assertIn("GetValidatedEquipmentCodes(ValidatedCodes)", score)
        self.assertIn("GetRawEquipmentSlotCode(Slot)!=ValidatedCodes[Slot]", score)
        self.assertIn("FullCandidateImprovement(Candidate,EquippedCodes", score)
        self.assertIn("FullOwnedCopies(Candidate,InventoryItemCodes,Required)", score)
        self.assertNotIn("for(int32 Code:InventoryItemCodes)", score)
        recommended = source.split("int32 AMoonfallHeroPawn::GetRecommendedEquipmentIndex()", 1)[1].split(
            "int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades()", 1)[0]
        self.assertIn("TMap<int32,int32> OwnedCounts", recommended)
        self.assertIn("FullCandidateImprovement(Code", recommended)
        self.assertLess(recommended.index("if(!Copies||*Copies<=0)continue;"), recommended.index("const int32 PhysicalCopies=*Copies;"))
        self.assertLess(recommended.index("const int32 PhysicalCopies=*Copies;"), recommended.index("*Copies=0;"))
        self.assertLess(recommended.index("*Copies=0;"), recommended.index("FullCandidateImprovement(Code"))
        self.assertIn("ActiveSlotCount,PhysicalCopies)", recommended)
        self.assertIn("if(Improvement>FullBestImprovement)", recommended)
        delta = source.split("int32 AMoonfallHeroPawn::GetItemPowerDelta(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetProjectedSetPieceCount(", 1)[0]
        self.assertIn("FullWornCopies(ItemCode,Worn", delta)
        self.assertIn("FullOwnedCopies(ItemCode,InventoryItemCodes,Required)", delta)
        self.assertNotIn("int32 Copies=0;for(int32 Code:InventoryItemCodes)", delta)
        self.assertIn("FullRingQuickEquipTarget(ItemCode,Worn", delta)
        self.assertIn("return Target<0?0:CandidatePower-GetItemPower(Worn[Target]);", delta)
        projected = source.split("int32 AMoonfallHeroPawn::GetProjectedSetPieceCount(", 1)[1].split(
            "bool AMoonfallHeroPawn::HasLegendaryPerk(", 1)[0]
        self.assertIn("FullWornCopies(CandidateCode,Worn", projected)
        self.assertIn("FullOwnedCopies(CandidateCode,InventoryItemCodes,Required)", projected)
        self.assertNotIn("int32 Copies=0;for(int32 Code:InventoryItemCodes)", projected)
        self.assertIn("FullRingQuickEquipTarget(CandidateCode,Worn", projected)
        self.assertIn("if(Target<0)return CurrentPieces;", projected)
        self.assertLess(projected.index("++CurrentPieces"), projected.index("FullOwnedCopies(CandidateCode,InventoryItemCodes,Required)"))
        self.assertIn("DecodeEquippedItem(Worn[Target],Target", projected)
        policy = SOURCE.with_name("MoonfallGearAdvisorPolicy.h").read_text(encoding="utf-8")
        self.assertIn("return FullRingQuickEquipTarget(Code,Worn,ActiveSlots,FullOwnedCopies(Code,Inventory));", policy)
    def test_validated_equipment_snapshot_counts_duplicates_in_one_pass(self):
        snapshot = self.source.split("void AMoonfallHeroPawn::GetValidatedEquipmentCodes(", 1)[1].split(
            "uint64 AMoonfallHeroPawn::GetEquipmentLoadoutFingerprint()", 1)[0]
        for token in ("OutCodes.Init(0,MoonfallEquipmentLayout::FullSlotCount)",
                      "TMap<int32,int32> OwnedCounts,RequiredCounts",
                      "for(int32 Code:InventoryItemCodes)", "RequiredCounts.FindOrAdd(Code)",
                      "OwnedCounts.Find(Code)", "*Owned>=*Required"):
            self.assertIn(token, snapshot)
        fingerprint = self.source.split("uint64 AMoonfallHeroPawn::GetEquipmentLoadoutFingerprint()", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetEquipmentSlotCode(", 1)[0]
        self.assertIn("GetRawEquipmentSlotCode(Slot)", fingerprint)
        self.assertNotIn("InventoryItemCodes", fingerprint)
        power = self.source.split("int32 AMoonfallHeroPawn::GetEquipmentPower() const", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetItemPowerDelta(", 1)[0]
        self.assertIn("GetValidatedEquipmentCodes(Codes)", power)
    def test_equipment_snapshot_counts_only_worn_item_codes(self):
        snapshot = self.source.split("void AMoonfallHeroPawn::GetValidatedEquipmentCodes(", 1)[1].split(
            "uint64 AMoonfallHeroPawn::GetEquipmentLoadoutFingerprint()", 1)[0]
        self.assertIn("for(const auto& Required:RequiredCounts)OwnedCounts.Add(Required.Key,0);", snapshot)
        self.assertIn("if(!RequiredCounts.IsEmpty())", snapshot)
        self.assertIn("if(int32* Owned=OwnedCounts.Find(Code))++*Owned;", snapshot)
        self.assertNotIn("OwnedCounts.FindOrAdd(Code)", snapshot)
        self.assertLess(snapshot.index("RequiredCounts.FindOrAdd(Code)"), snapshot.index("for(int32 Code:InventoryItemCodes)"))

    def test_expanded_tier_refresh_uses_one_validated_loadout(self):
        body = self.source.split("void AMoonfallHeroPawn::RefreshEquipmentTier()", 1)[1].split("\n}\n", 1)[0]
        expanded = body.split("if(MoonfallEquipmentLayout::ActiveSlotCount>MoonfallEquipmentLayout::SlotCount)", 1)[1].split("return;", 1)[0]
        self.assertEqual(expanded.count("GetValidatedEquipmentCodes(Codes)"), 1)
        self.assertLess(expanded.index("GetValidatedEquipmentCodes(Codes)"), expanded.index("for(int32 Slot=0;"))
        self.assertIn("ValidEquippedTier(Codes[Slot],Slot)", expanded)
        self.assertNotIn("GetValidatedEquippedTier(Slot)", expanded)
        self.assertIn("FMath::Max(EquipmentTier, GetValidatedEquippedTier(Slot))", body)

    def test_expanded_tier_queries_reuse_validated_snapshot(self):
        tier = self.source.split("int32 AMoonfallHeroPawn::GetValidatedEquippedTier(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetCraftEquipmentSlot(", 1)[0]
        self.assertIn("GetValidatedEquipmentCodes(Codes);", tier)
        self.assertIn("Codes.IsValidIndex(Slot)?ValidEquippedTier(Codes[Slot],Slot):0", tier)
        self.assertNotIn("FullOwnedCopies(", tier)
        self.assertIn("InventoryItemCodes.Contains(Code) ? ValidEquippedTier(Code, Slot) : 0", tier)

    def test_equipment_snapshot_cache_invalidates_on_inventory_mutation(self):
        header = HEADER.read_text(encoding="utf-8")
        self.assertIn("uint32 EquipmentInventoryRevision=1;", header)
        self.assertIn("mutable uint32 CachedValidatedInventoryRevision=0;", header)
        self.assertIn("mutable uint64 CachedValidatedLoadoutFingerprint=0;", header)
        snapshot = self.source.split("void AMoonfallHeroPawn::GetValidatedEquipmentCodes(", 1)[1].split(
            "uint64 AMoonfallHeroPawn::GetEquipmentLoadoutFingerprint()", 1)[0]
        self.assertIn("CachedValidatedInventoryRevision==EquipmentInventoryRevision", snapshot)
        self.assertIn("CachedValidatedLoadoutFingerprint==Fingerprint", snapshot)
        self.assertIn("OutCodes=CachedValidatedEquipmentCodes", snapshot)
        dirty = self.source.split("void AMoonfallHeroPawn::MarkEquipmentInventoryDirty()", 1)[1].split(
            "void AMoonfallHeroPawn::GetValidatedEquipmentCodes(", 1)[0]
        self.assertIn("++EquipmentInventoryRevision", dirty)
        for signature in ("bool AMoonfallHeroPawn::SalvageInventoryItem(",
                          "bool AMoonfallHeroPawn::UpgradeInventoryItem(",
                          "bool AMoonfallHeroPawn::ReforgeInventoryItemAffix(",
                          "bool AMoonfallHeroPawn::AscendInventoryItemRarity(",
                          "void AMoonfallHeroPawn::AwardEquipmentDrop(",
                          "bool AMoonfallHeroPawn::CollectGroundEquipmentCodeImpl(",
                          "bool AMoonfallHeroPawn::LoadWorldProgress(",
                          "bool AMoonfallHeroPawn::TradeCompanionGear("):
            body = self.source.split(signature, 1)[1].split("\n}\n", 1)[0]
            self.assertIn("MarkEquipmentInventoryDirty()", body, signature)

    def test_companion_trade_preflights_transaction_state_and_old_gear(self):
        body = self.source.split("bool AMoonfallHeroPawn::TradeCompanionGear(", 1)[1].split(
            "bool AMoonfallHeroPawn::CommitCompanionParty(", 1)[0]
        for guard in ("!GetWorld()", "IsActorBeingDestroyed()", "!FMath::IsFinite(Health)",
                      "Health<=0.f", "!bWorldProgressInitialized", "bWorldProgressLoadInProgress",
                      "bPersistentSaveWritesBlocked", "bEquipmentCraftInProgress",
                      "bEquipBestInProgress", "bTalentAllocationInProgress",
                      "Old!=0&&!MoonfallGearAdvisor::Valid(Old,Slot)"):
            self.assertLess(body.index(guard), body.index("const TArray<int32> PreviousBag"))
        self.assertIn("InventoryItemCodes=PreviousBag;", body)

    def test_ring_strip_hit_testing_matches_hud_and_consumes_clicks(self):
        hud = SOURCE.with_name("MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        hit = self.source.split("int32 AMoonfallHeroPawn::HitTestInventoryRingStrip()", 1)[1].split("\n}", 1)[0]
        for token in ("RingX=Left+18.f,RingY=Top+PanelHeight-HelpOffset+20.f",
                      "RingWidth=(PanelWidth-36.f)/MoonfallEquipmentLayout::RingSlotCount",
                      "RingWidth-2.f"):
            self.assertIn(token, hit)
            self.assertIn(token, hud)
        self.assertIn("if(PanelHeight<420.f)return INDEX_NONE;", hit)
        self.assertIn("ActiveSlotCount<=MoonfallEquipmentLayout::SlotCount", hit)
        self.assertIn("if(RingIndex!=INDEX_NONE)return;", self.source)
        self.assertIn("BeginRingPointerDrag(INDEX_NONE,RingIndex);", self.source)
        self.assertIn("BeginRingPointerDrag(HitIndex,INDEX_NONE);", self.source)
        release = self.source.split("void AMoonfallHeroPawn::ReleaseInventoryPointer()", 1)[1].split("\n}", 1)[0]
        self.assertIn("EquipInventoryRing(BagIndex,Target);", release)
        self.assertIn("MoveEquippedRing(FingerIndex,Target);", release)
        self.assertIn("UnequipRing(FingerIndex);", release)
        self.assertIn("else if(HitTestInventoryBagDropSurface())", release)
        drop = self.source.split("bool AMoonfallHeroPawn::HitTestInventoryBagDropSurface()", 1)[1].split("\n}", 1)[0]
        self.assertIn("MouseY<FirstY+Rows*Spacing", drop)
        self.assertIn("MouseX<Left+PanelWidth-16.f", drop)
        self.assertIn("!CanEditEquippedRings()", drop)
        self.assertIn("!FMath::IsFinite(MouseX)||!FMath::IsFinite(MouseY)", drop)
        self.assertNotIn("GetInventoryDisplayOrder()", drop)
        self.assertNotIn("InventoryItemCodes.Num()", drop)
        row = self.source.split("int32 AMoonfallHeroPawn::HitTestInventoryRow()", 1)[1].split("\n}", 1)[0]
        self.assertLess(row.index("!FMath::IsFinite(MouseY)"), row.index("FMath::FloorToInt("))
        self.assertIn("InventoryItemCodes[BagIndex]==Code", release)
        self.assertIn("Revision!=EquipmentInventoryRevision", release)
        self.assertIn("Fingerprint!=GetEquipmentLoadoutFingerprint()", release)
        self.assertLess(release.index("ClearRingPointerDrag();"), release.index("EquipInventoryRing("))
        self.assertIn("RINGS: drag to equip / swap | drag to bag to remove", hud)

    def test_ring_navigation_focus_consumes_existing_menu_actions(self):
        hud = SOURCE.with_name("MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("CurrentRow<=0&&CanFocusRingNavigation()", self.source)
        self.assertIn("RingNavigationIndex=0;", self.source)
        self.assertIn("if(HasRingNavigationFocus()){if(RingNavigationSwapSource==INDEX_NONE)UnequipRing(RingNavigationIndex);return;}", self.source)
        self.assertIn("if(HasRingNavigationFocus()){ClearRingNavigationFocus();return;}", self.source)
        self.assertIn("if(HasRingNavigationFocus()){ArmOrCancelRingNavigationSwap();return;}", self.source)
        self.assertIn("if(HasRingNavigationFocus()){ConfirmRingNavigationAction();return;}", self.source)
        swap = self.source.split("void AMoonfallHeroPawn::ConfirmRingNavigationAction()", 1)[1].split("\n}", 1)[0]
        self.assertIn("RingNavigationSwapRevision==EquipmentInventoryRevision", swap)
        self.assertIn("!bRingNavigationSwapInterrupted&&CanEditEquippedRings()", swap)
        self.assertIn("RingNavigationSwapSource!=INDEX_NONE&&!bRingNavigationSwapInterrupted", self.source)
        self.assertIn("bRingNavigationSwapInterrupted=true;", self.source)
        clear_swap = self.source.split("void AMoonfallHeroPawn::ClearRingNavigationSwap()", 1)[1].split("\n}", 1)[0]
        self.assertIn("bRingNavigationSwapInterrupted=false;", clear_swap)
        self.assertIn("SWAP CANCELED | A/ENTER or UP dismiss", hud)
        self.assertIn("RingNavigationSwapFingerprint==GetEquipmentLoadoutFingerprint()", swap)
        self.assertLess(swap.index("ClearRingNavigationSwap();"), swap.index("MoveEquippedRing(Source,RingNavigationIndex);"))
        self.assertIn("if(bValid&&Source!=RingNavigationIndex)MoveEquippedRing(Source,RingNavigationIndex);", swap)
        self.assertIn("return; // Never fall through", swap)
        self.assertIn("Index==DragSource||Index==SwapSource", hud)
        self.assertIn("if(CanEditEquippedRings())EquipInventoryRing(InventorySelection,RingNavigationIndex);", self.source)
        self.assertIn("if(Index==FocusedRing)DrawSelectionRail", hud)
        self.assertIn("A/ENTER equip | UP pick up | X/DEL remove", hud)
        self.assertIn("A/ENTER confirm | UP cancel", hud)

    def test_ring_drag_feedback_is_read_only_and_validates_hovered_target(self):
        hud = SOURCE.with_name("MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        preview = self.source.split("bool AMoonfallHeroPawn::CanDropDraggedRingOnFinger(", 1)[1].split("\n}", 1)[0]
        self.assertIn("TryAssignFullRing(Code,Target,Worn", preview)
        self.assertIn("TryMoveFullRing(From,Target,Worn", preview)
        self.assertIn("InventoryItemCodes[RingDragBagIndex]!=Code", preview)
        self.assertNotIn("ExtraEquippedCodes[", preview)
        self.assertNotIn("RefreshEquipmentTier()", preview)
        self.assertIn("RingDragTargetCache[FingerIndex]", preview)
        self.assertIn("if(Cached!=0)return Cached>0;", preview)
        self.assertLess(preview.index("GetRingPointerDragCode()"), preview.index("RingDragTargetCache[FingerIndex]"))
        clear = self.source.split("void AMoonfallHeroPawn::ClearRingPointerDrag()", 1)[1].split("\n}", 1)[0]
        self.assertIn("RingDragTargetCache.fill(0);", clear)
        self.assertIn("else if(bWorn)DrawRect", hud)
        self.assertIn("DragHover!=INDEX_NONE&&Hero->CanDropDraggedRingOnFinger(DragHover)", hud)
        self.assertNotIn('bDropAllowed?TEXT("+"):TEXT("X")', hud)
        self.assertIn("Index==DragHover&&bDropAllowed?SlotGold:QualityInk", hud)
        self.assertIn("const bool bBlockedHover=Index==DragHover&&!bDropAllowed;", hud)
        self.assertIn("const bool bDim=bRingLocked||bBlockedHover;", hud)
        self.assertIn("RingQualityColors[WornItem.Rarity]", hud)
        self.assertIn("RingQualityColors[DragItem.Rarity]", hud)
        self.assertIn("DrawGearGlyph(GhostX+12.f", hud)
        self.assertIn('TEXT("RING T%d")', hud)
        self.assertIn("FMath::IsFinite(MX)&&FMath::IsFinite(MY)", hud)

    def test_inventory_pointer_coordinates_are_finite(self):
        for name in ("ScrollInventoryToMouseTrack", "HitTestInventoryCloseButton",
                     "HitTestInventorySortButton", "HitTestInventoryEquipButton", "HitTestInventoryRow"):
            with self.subTest(name=name):
                body = self.source.split("AMoonfallHeroPawn::" + name + "(", 1)[1].split("\n}", 1)[0]
                self.assertIn("!FMath::IsFinite(MouseX)", body)
                self.assertIn("!FMath::IsFinite(MouseY)", body)
        scroll = self.source.split("AMoonfallHeroPawn::ScrollInventoryToMouseTrack(", 1)[1].split("\n}", 1)[0]
        self.assertLess(scroll.index("!FMath::IsFinite(MouseY)"), scroll.index("FMath::RoundToInt("))
        row = self.source.split("AMoonfallHeroPawn::HitTestInventoryRow(", 1)[1].split("\n}", 1)[0]
        self.assertIn("!HasVisibleInventoryPointerSurface(Controller)", row)
        self.assertIn("bSpellBookOpen || bTalentWebOpen || bAtlasOpen", row)
        self.assertIn("MouseX >= Left + PanelWidth - 16.f", row)
        self.assertLess(row.index("if (DisplayPosition < Start || DisplayPosition >= End)"), row.index("GetInventoryDisplayOrder()"))

    def test_bag_drop_surface_has_no_gold_highlight(self):
        hud = SOURCE.with_name("MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertNotIn("CanDropDraggedRingInBag()", hud)
        self.assertNotIn("DrawRect(SlotGold,BagX", hud)
        self.assertIn("else if(HitTestInventoryBagDropSurface())", self.source)

    def test_expanded_ring_equip_entry_points_share_transaction_guard(self):
        explicit = self.source.split("bool AMoonfallHeroPawn::EquipInventoryRing(", 1)[1].split("\n}", 1)[0]
        self.assertLess(explicit.index("if(!CanEditEquippedRings())return false;"), explicit.index("EquipInventoryItemWithPreviousTier("))
        self.assertIn("Target<0||Target>=MoonfallEquipmentLayout::ActiveSlotCount", explicit)
        equip = self.source.split("bool AMoonfallHeroPawn::EquipInventoryItemWithPreviousTier(", 1)[1].split("\n}", 1)[0]
        self.assertLess(equip.index("if(!bEquipmentCraftInProgress&&!CanEditEquippedRings())return false;"), equip.index("ExtraEquippedCodes[Target-"))
        self.assertIn("if(bEquipmentCraftInProgress){", equip)

    def test_native_ring_move_and_remove_are_guarded_and_bag_read_only(self):
        guard = self.source.split("bool AMoonfallHeroPawn::CanEditEquippedRings()", 1)[1].split(
            "bool AMoonfallHeroPawn::MoveEquippedRing(", 1)[0]
        for token in ("Health>0.f", "!IsRecentlyInCombatForAmbientAI()",
                      "!bEquipmentCraftInProgress", "!bEquipBestInProgress",
                      "!bPersistentSaveWritesBlocked", "!bWorldProgressLoadInProgress",
                      "ActiveSlotCount>MoonfallEquipmentLayout::SlotCount"):
            self.assertIn(token, guard)
        for name, policy in (("MoveEquippedRing", "TryMoveFullRing"),
                             ("UnequipRing", "TryRemoveFullRing")):
            body = self.source.split("bool AMoonfallHeroPawn::" + name + "(", 1)[1].split("\n}", 1)[0]
            self.assertIn("if(!CanEditEquippedRings())return false;", body)
            self.assertLess(body.index(policy), body.index("ExtraEquippedCodes["))
            self.assertLess(body.index("ExtraEquippedCodes["), body.index("RefreshEquipmentTier();"))
            for forbidden in ("InventoryItemCodes.Remove", "InventoryItemCodes.Add", "SaveWorldProgress("):
                self.assertNotIn(forbidden, body)

    def test_expanded_crafting_preflights_physical_copies(self):
        source = self.source
        helper = source.split("bool AMoonfallHeroPawn::CanCraftEquipmentCopy(", 1)[1].split(
            "int32 AMoonfallHeroPawn::GetLootComparisonTier(", 1)[0]
        self.assertIn("GetRawEquipmentSlotCode(I)", helper)
        self.assertIn("MoonfallEquipmentLayout::CanCraftFullCopy(", helper)
        self.assertIn("ActiveSlotCount<=MoonfallEquipmentLayout::SlotCount", helper)
        for name in ("UpgradeInventoryItem", "ReforgeInventoryItemAffix", "AscendInventoryItemRarity"):
            body = source.split("bool AMoonfallHeroPawn::" + name + "(", 1)[1].split("\n}", 1)[0]
            self.assertLess(body.index("if(!CanCraftEquipmentCopy(Code,Slot))return false;"),
                            body.index("InventoryItemCodes[InventoryIndex] = NewCode;"))

    def test_crafting_blocks_combat_at_entry_and_between_batch_steps(self):
        for name in ("UpgradeInventoryItem", "ReforgeInventoryItemAffix", "AscendInventoryItemRarity", "ForgeInventoryItemLevels"):
            body = self.source.split("AMoonfallHeroPawn::" + name + "(", 1)[1].split("\n}\n", 1)[0]
            self.assertLess(body.index("IsRecentlyInCombatForAmbientAI()"), body.index("InventoryItemCodes[InventoryIndex]"))
        batch = self.source.split("AMoonfallHeroPawn::ForgeInventoryItemLevels(", 1)[1].split("\n}\n", 1)[0]
        loop = batch.split("for (int32 Step = 0; Step < Limit; ++Step)", 1)[1]
        self.assertLess(loop.index("IsRecentlyInCombatForAmbientAI()"), loop.index("if (!UpgradeInventoryItem(InventoryIndex))"))
        hud = SOURCE.with_name("MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("bCraftCombatLocked=Hero->IsRecentlyInCombatForAmbientAI()", hud)
        self.assertIn("Crafting: unavailable in combat", hud)
        self.assertIn("Reforge: unavailable in combat", hud)
        self.assertIn("Ascend: unavailable in combat", hud)
        self.assertIn("FORGE 5: UNAVAILABLE IN COMBAT", hud)
        self.assertIn("FPlan Plan = bCraftHealthLocked || bCraftCombatLocked", hud)
        self.assertIn("bCraftHealthLocked=!FMath::IsFinite(Hero->Health)||Hero->Health<=0.f", hud)
        for operation in ("Plan.Valid", "bReforgeable", "bAscendable", "bForgeable"):
            self.assertIn("!bCraftHealthLocked && !bCraftCombatLocked && " + operation, hud)
        self.assertIn("!bCraftCombatLocked && Plan.Valid && Plan.Upgrades > 0", hud)

    def test_crafting_rejects_invalid_health_capacity_and_equip_best_reentry(self):
        for name in ("UpgradeInventoryItem", "ReforgeInventoryItemAffix", "AscendInventoryItemRarity", "ForgeInventoryItemLevels"):
            body = self.source.split("AMoonfallHeroPawn::" + name + "(", 1)[1].split("\n}\n", 1)[0]
            for guard in ("bEquipBestInProgress", "!FMath::IsFinite(MaxHealth)", "MaxHealth < 1.f"):
                self.assertLess(body.index(guard), body.index("InventoryItemCodes[InventoryIndex]"))
        batch = self.source.split("AMoonfallHeroPawn::ForgeInventoryItemLevels(", 1)[1].split("\n}\n", 1)[0]
        loop = batch.split("for (int32 Step = 0; Step < Limit; ++Step)", 1)[1]
        for guard in ("bEquipBestInProgress", "!FMath::IsFinite(MaxHealth)", "MaxHealth < 1.f"):
            self.assertLess(loop.index(guard), loop.index("if (!UpgradeInventoryItem(InventoryIndex))"))
        hud = SOURCE.with_name("MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        self.assertIn("||!FMath::IsFinite(Hero->MaxHealth)||Hero->MaxHealth<1.f", hud)

    def test_save_restore_batches_equipment_cache_invalidation(self):
        load = self.source.split("bool AMoonfallHeroPawn::LoadWorldProgress(", 1)[1]
        rebuild = load.split("InventoryItemCodes.Reset();", 1)[1].split("const auto RestoreGear", 1)[0]
        self.assertEqual(rebuild.count("MarkEquipmentInventoryDirty();"), 1)
        self.assertIn("InventoryItemCodes.Reserve(LoadScanCount);", rebuild)
        self.assertIn("if (ValidMoonfallItemCode(Code)) InventoryItemCodes.Add(Code);", rebuild)
        self.assertLess(rebuild.index("InventoryItemCodes.Add(Code);"), rebuild.index("MarkEquipmentInventoryDirty();"))
        self.assertIn("}\n // Invalidate once", rebuild)

    def test_ring_bonus_cache_tracks_inventory_revision_and_loadout_fingerprint(self):
        header = HEADER.read_text(encoding="utf-8")
        self.assertIn("mutable uint32 CachedRingBonusInventoryRevision=0;", header)
        self.assertIn("mutable uint64 CachedRingBonusLoadoutFingerprint=0;", header)
        self.assertIn("mutable MoonfallEquipmentLayout::FRingBonuses CachedNativeRingBonuses{};", header)
        self.assertLess(header.index('#include "MoonfallGearAdvisorPolicy.h"'), header.index('#include "MoonfallHeroPawn.generated.h"'))
        dirty = self.source.split("void AMoonfallHeroPawn::MarkEquipmentInventoryDirty()", 1)[1].split(
            "void AMoonfallHeroPawn::GetValidatedEquipmentCodes(", 1)[0]
        self.assertIn("CachedRingBonusInventoryRevision=0;", dirty)
        rings = self.source.split("FRingBonuses AMoonfallHeroPawn::GetNativeRingBonuses()", 1)[1].split(
            "float AMoonfallHeroPawn::GetRingLuckBonus()", 1)[0]
        self.assertIn("CachedRingBonusInventoryRevision==EquipmentInventoryRevision", rings)
        self.assertIn("CachedRingBonusLoadoutFingerprint==Fingerprint", rings)
        self.assertIn("return CachedNativeRingBonuses;", rings)
        self.assertIn("CachedNativeRingBonuses=MoonfallEquipmentLayout::BuildRingBonusesValidated(", rings)
        self.assertIn("GetEquipmentLoadoutFingerprint()", rings)

    def test_ring_stats_respect_activation_ownership_and_global_caps(self):
        source = self.source
        rings = source.split("FRingBonuses AMoonfallHeroPawn::GetNativeRingBonuses()", 1)[1].split(
            "float AMoonfallHeroPawn::GetRingLuckBonus()", 1)[0]
        self.assertIn("ActiveSlotCount<=MoonfallEquipmentLayout::SlotCount", rings)
        self.assertIn("GetRawEquipmentSlotCode(Slot)", rings)
        self.assertIn("GetValidatedEquipmentCodes(ValidatedCodes)", rings)
        self.assertIn("BuildRingBonusesValidated(", rings)
        self.assertIn("Worn,MoonfallEquipmentLayout::ActiveSlotCount)", rings)
        for method, field, cap in (("GetCriticalChance", "CritBps", "0.25f"),
                                   ("GetHasteBonus", "HasteBps", "0.20f"),
                                   ("GetMasteryBonus", "MasteryBps", "0.16f"),
                                   ("GetVersatilityBonus", "VersatilityBps", "0.12f")):
            body = source.split(f"float AMoonfallHeroPawn::{method}() const", 1)[1].split("\n}", 1)[0]
            self.assertIn(f"GetNativeRingBonuses().{field} * 0.0001f", body)
            self.assertIn(cap, body)
        mastery = source.split("float AMoonfallHeroPawn::GetMasteryBonus() const", 1)[1].split("\n}", 1)[0]
        self.assertIn("MoonfallEquipmentLayout::IsRingSlot(Slot)", mastery)
        self.assertIn("GetNativeRingBonuses().LuckBps / 100", source)
        self.assertIn("GetNativeRingBonuses().LuckBps * 0.0001f", source)
    def test_expanded_equip_best_preserves_callback_ring_changes_and_health_guard(self):
        body = self.source.split("int32 AMoonfallHeroPawn::EquipBestExpandedLoadout()", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        self.assertIn("!IsActorBeingDestroyed()&&GetWorld()", body)
        self.assertIn("FMath::IsFinite(MaxHealth)&&MaxHealth>=1.f", body)
        self.assertIn("BeforeRings[Slot]!=Initial[Slot]", body)
        self.assertIn("bRingSelectionChanged?MoonfallEquipmentLayout::FFullPlan{}", body)
        self.assertLess(body.index("if(!bRingSelectionChanged&&RingPlan.Changes>0"), body.index("=RingPlan.Codes[Slot]"))

    def test_expanded_equip_best_does_not_re_equip_retained_duplicate_copies(self):
        body = self.source.split("int32 AMoonfallHeroPawn::EquipBestExpandedLoadout()", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        self.assertIn("const int32 PlannedIndex=FixedPlan.Indices[Slot];", body)
        self.assertIn("if(PlannedIndex<0||FixedPlan.Codes[Slot]<=0)continue;", body)
        self.assertLess(body.index("if(PlannedIndex<0"), body.index("IndexOfByKey(FixedPlan.Codes[Slot])"))

    def test_legacy_equip_best_revalidates_world_and_health_capacity(self):
        body = self.source.split("int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades()", 1)[1].split(
            "int32 AMoonfallHeroPawn::EquipBestExpandedLoadout()", 1)[0]
        entry, loop = body.split("for (int32 Slot = 0; Slot < MoonfallEquipmentLayout::ActiveSlotCount; ++Slot)", 1)
        for guard in ("!GetWorld()", "!FMath::IsFinite(MaxHealth)", "MaxHealth < 1.f"):
            self.assertLess(entry.index(guard), entry.index("Planner.Finish()"))
            self.assertLess(loop.index(guard), loop.index("IsRecentlyInCombatForAmbientAI()"))

    def test_legacy_equip_best_recovers_only_planned_inventory_items(self):
        body = self.source.split("int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades()", 1)[1].split(
            "int32 AMoonfallHeroPawn::EquipBestExpandedLoadout()", 1)[0]
        self.assertIn("const int32 PlannedIndex = Plan.Indices[Slot];", body)
        self.assertLess(body.index("if (PlannedIndex < 0 || Plan.Codes[Slot] <= 0) continue;"),
                        body.index("IndexOfByKey(Plan.Codes[Slot])"))
        self.assertLess(body.index("IndexOfByKey(Plan.Codes[Slot])"),
                        body.rindex("InventoryItemCodes[Index] != Plan.Codes[Slot]"))
        self.assertLess(body.rindex("InventoryItemCodes[Index] != Plan.Codes[Slot]"),
                        body.index("if (EquipInventoryItem(Index))"))

    def test_expanded_equip_best_recovers_shifted_inventory_indices(self):
        body = self.source.split("int32 AMoonfallHeroPawn::EquipBestExpandedLoadout()", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        self.assertIn("const int32 PlannedIndex=FixedPlan.Indices[Slot];", body)
        self.assertIn("int32 Index=PlannedIndex;", body)
        self.assertIn("Index=InventoryItemCodes.IndexOfByKey(FixedPlan.Codes[Slot]);", body)
        self.assertIn("InventoryItemCodes[Index]!=FixedPlan.Codes[Slot]", body)
        self.assertLess(body.index("if(PlannedIndex<0||FixedPlan.Codes[Slot]<=0)continue;"),
                        body.index("IndexOfByKey(FixedPlan.Codes[Slot])"))

    def test_expanded_equip_best_revalidates_and_commits_rings_atomically(self):
        source = self.source
        body = source.split("int32 AMoonfallHeroPawn::EquipBestExpandedLoadout()", 1)[1].split(
            "bool AMoonfallHeroPawn::EquipInventoryItem(", 1)[0]
        for token in ("TGuardValue<bool> BatchGuard", "IsFullLoadoutOwned(Initial",
                      "FFullPlanner Fixed", "ActiveSlotCount,true)",
                      "if(!CanContinue())break", "InventoryItemCodes[Index]!=FixedPlan.Codes[Slot]",
                      "GetRawEquipmentSlotCode(Slot)!=Initial[Slot]", "PlanFlexibleRings(BeforeRings",
                      "IsFullLoadoutOwned(RingPlan.Codes", "if(CanContinue()&&Health>0.f",
                      "ExtraEquippedCodes[Slot-MoonfallEquipmentLayout::LegacySlotCount]=RingPlan.Codes[Slot]"):
            self.assertIn(token, body)
        self.assertLess(body.index("IsFullLoadoutOwned(RingPlan.Codes"), body.index("=RingPlan.Codes[Slot]"))
        self.assertLess(body.index("=RingPlan.Codes[Slot]"), body.index("OnBestLoadoutEquipped(Applied)"))
if __name__ == "__main__":
    unittest.main()
