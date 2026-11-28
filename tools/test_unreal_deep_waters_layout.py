import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "MoonfallUnreal" / "Tools"))
from deep_waters_layout import grotto_layout, grotto_geometry, MIN_HALF_WIDTH, MIN_HALF_LENGTH, grotto_art, GROTTO_ART, shoreline_geometry, ruin_geometry, wreck_geometry, WRECK_CENTERS, cave_geometry, island_geometry, EXPANDED_HALF_WIDTH, EXPANDED_HALF_LENGTH, ocean_resources


class DeepWatersLayoutTests(unittest.TestCase):
    def test_emergency_surface_contract(self):
        root = Path(__file__).resolve().parents[1]
        source = (root / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (root / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        bindings = (root / "MoonfallUnreal/Config/DefaultInput.ini").read_text(encoding="utf-8")
        capture = source.split("// Remember the last grounded shoreline;", 1)[1].split("float WaterEyeDepth", 1)[0]
        for token in ("IsMovingOnGround()", "!GetPhysicsVolume()->bWaterVolume",
                      "EComponentMobility::Static", "Now-WaterRecoveryShoreAt<=3.0",
                      "FMath::Square(1200.f)", "WaterRecoveryVolume=Volume"):
            self.assertIn(token, capture)
        gate = source.split("bool AMoonfallHeroPawn::CanRequestEmergencySurface() const", 1)[1].split(
            "bool AMoonfallHeroPawn::TryEmergencySurface()", 1)[0]
        for token in ("bPersistentSaveWritesBlocked", "HasAuthority()", "IsLocallyControlled()",
                      "DeepWatersPreview", "Health>0.f", "bWaterSubmerged",
                      "WaterRecoveryVolume.Get()==GetPhysicsVolume()",
                      "!bWorldProgressLoadInProgress", "!bTalentWebOpen", "!bSpellBookOpen",
                      "!bQuestLogOpen", "!IsRecentlyInCombatForAmbientAI()"):
            self.assertIn(token, gate)
        rescue = source.split("bool AMoonfallHeroPawn::TryEmergencySurface()", 1)[1].split(
            "void AMoonfallHeroPawn::TryJump()", 1)[0]
        teleport = rescue.index("TeleportTo(Candidate,GetActorRotation(),false,false)")
        for token in ("TActorIterator<AMoonfallEnemy>", "FVector::DistSquared(P,Origin)",
                      "FVector::DistSquared(P,WaterRecoveryShore)", "LineTraceSingleByChannel",
                      "Floor.ImpactNormal.Z<0.7f", "ECR_Block", "OverlapBlockingTestByProfile",
                      "TActorIterator<APhysicsVolume>", "Bounds.Intersect(CandidateBounds)"):
            self.assertLess(rescue.index(token), teleport)
        self.assertGreater(rescue.index("WaterRecoveryVolume.Reset()"), teleport)
        self.assertNotIn("Oxygen =", rescue)
        self.assertIn('ActionName="EmergencySurface",Key=F1', bindings)
        self.assertIn('if (bWaterSwimming) TryEmergencySurface();', source)
        self.assertIn("shore rescue if safe", hud)
        self.assertIn("LOW AIR: rise or find an air pocket", hud)

    def test_mouse_fishing_interaction(self):
        root = Path(__file__).resolve().parents[1]
        module = root / "MoonfallUnreal/Source/MoonfallUnreal"
        source = (module / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        hud = (module / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
        inputs = (root / "MoonfallUnreal/Config/DefaultInput.ini").read_text(encoding="utf-8")
        self.assertIn('ActionName="PickupLoot",Key=F', inputs)
        self.assertIn('BindAction(TEXT("PickupLoot"), IE_Pressed, this, &AMoonfallHeroPawn::PickupNearestGroundLoot)', source)
        self.assertNotIn("ManualLootOrFish", source)
        cast = source.split("bool AMoonfallHeroPawn::TryFishingAction(", 1)[1].split("bool AMoonfallHeroPawn::TryMouseFishingAction()", 1)[0]
        self.assertIn('ActionName="Fishing",Key=N', inputs)
        self.assertNotIn("RequestedWater", cast)
        self.assertIn("FishingWaterVolume=Volume;", cast)
        self.assertIn("FishingWaterVolume=BestVolume;", cast)
        self.assertIn("Target.X<Bounds.Min.X+30.f", cast)
        self.assertIn("Target.Y<Bounds.Min.Y+30.f", cast)
        self.assertIn("!MoonfallFishing::InCastRange(Distance,MaxReach)", cast)
        rules = (module / "MoonfallFishing.h").read_text(encoding="utf-8")
        self.assertIn("DistanceSquared>=100.0*100.0", rules)
        self.assertIn("DistanceSquared<=double(Reach)*Reach", rules)
        self.assertIn("FVector::DotProduct(Forward", cast)
        self.assertIn("IsFishingWaterValid(true)", cast)
        mouse = source.split("bool AMoonfallHeroPawn::TryMouseFishingAction()", 1)[1].split("void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[0]
        for token in ("CanContinueFishing()", "DeprojectMousePositionToWorld", "FishingBobber",
                      "FMath::Square(85.f)", "LineTraceSingleByChannel", "TryFishingAction()"):
            self.assertIn(token, mouse)
        self.assertIn("Fishing.Stage==MoonfallFishing::EStage::Idle", mouse)
        self.assertLess(mouse.index("Fishing.Stage==MoonfallFishing::EStage::Idle"), mouse.index("TryFishingAction()"))
        self.assertLess(mouse.index("LineTraceSingleByChannel"), mouse.index("!CanContinueFishing()"))
        self.assertIn("CancelFishingWithFeedback();return true;", mouse)
        self.assertNotIn("TActorIterator", mouse)
        self.assertNotIn("TryFishingAction(&", mouse)
        pointer = source.split("void AMoonfallHeroPawn::FishingAction()", 1)[1].split("void AMoonfallHeroPawn::SyncFishingPresentation", 1)[0]
        self.assertIn("FInputModeGameAndUI", pointer)
        self.assertIn("SetHideCursorDuringCapture(false)", pointer)
        self.assertIn("PC->bShowMouseCursor=true", pointer)
        self.assertIn("FishingPointerOwner.Reset()", pointer)
        self.assertIn("FInputModeGameOnly()", pointer)
        self.assertIn("bOptionsOpen||bInventoryOpen||bAtlasOpen||bTalentWebOpen", pointer)
        self.assertIn("EStage::Reeling) ReleaseFishingPointer()", source)
        self.assertIn("EndPlayReason) {\n ReleaseFishingPointer();", source)
        # Modal entry must release fishing input before the menu owns the cursor.
        for method, marker in (
            ("ToggleAtlas", "bAtlasOpen = !bAtlasOpen"),
            ("ToggleInventory", "bInventoryOpen = !bInventoryOpen"),
            ("OpenOptions", "bOptionsOpen = !bOptionsOpen"),
            ("ToggleTalentWeb", "bTalentWebOpen=true"),
            ("OpenFocusedResidentContext", "bResidentContextOpen = true"),
            ("CastRightMouse", "bResidentContextOpen = true"),
        ):
            body = source.split("void AMoonfallHeroPawn::"+method+"()",1)[1].split("\n}",1)[0]
            self.assertLess(body.index("CancelFishingWithFeedback();"),body.index(marker))
        right = source.split("void AMoonfallHeroPawn::CastRightMouse()", 1)[1].split("int32 AMoonfallHeroPawn::GetRingPointerDragCode()", 1)[0]
        self.assertLess(right.index("Cast<AMoonfallWorldNPC>"), right.index("TryMouseFishingAction()"))
        self.assertLess(right.index("TryMouseFishingAction()"), right.index("CastRight();"))
        pickup = source.split("void AMoonfallHeroPawn::PickupNearestGroundLoot()", 1)[1].split("void AMoonfallHeroPawn::SetAutoLootEnabled", 1)[0]
        self.assertNotIn("TryFishingAction", pickup)
        self.assertIn("N / X: Cast", hud)
        self.assertIn("RMB / N / X: Reel", hud)
        self.assertNotIn("RMB / X: Cast", hud)
        self.assertNotIn("F / X: Cast", hud)
        self.assertIn("auto-loot", hud)
        self.assertIn("Preview->CatchOceanFish(FishingSchoolId,FishingProvince)", source)
        self.assertNotIn("CatchOceanFish", pickup)

    def test_regional_water_layout(self):
        from deep_waters_layout import regional_water_plan
        ids=set()
        for province in range(9):
            shore,nodes=regional_water_plan(province,600,600,500)
            self.assertEqual(len(shore),4)
            self.assertEqual(len(nodes),4)
            for i,_,_,(x,y,z),scale in nodes:
                self.assertNotIn(i,ids)
                ids.add(i)
                self.assertLess(abs(x)+scale[0]*50,600)
                self.assertLess(abs(y)+scale[1]*50,600)
                self.assertLess(z+scale[2]*50,0)
                self.assertGreater(z-scale[2]*50,-500)
        self.assertEqual(ids,set(range(12,48)))
        with self.assertRaises(ValueError):
            regional_water_plan(9,600,600,500)
        with self.assertRaises(ValueError):
            regional_water_plan(0,500,600,500)

    def test_regional_environment_art_and_wildlife(self):
        from deep_waters_layout import regional_environment_plan, REGIONAL_ART
        self.assertEqual(len({p[2] for p in REGIONAL_ART}),9)
        for province in range(9):
            ornaments,light,color,wildlife=regional_environment_plan(province,600,600,500)
            self.assertEqual(len(ornaments),4)
            self.assertIsNone(wildlife)
            self.assertTrue(all(0<=v<=1 for v in color))
            for _,shape,(x,y,z),scale in ornaments:
                self.assertIn(shape,("Cone","Sphere","Cylinder","Cube"))
                self.assertLess(abs(x)+50*scale[0],600)
                self.assertLess(abs(y)+50*scale[1],600)
                self.assertLess(z+50*scale[2],0)
                self.assertGreater(z-50*scale[2],-500)
            _,_,_,large=regional_environment_plan(province,1400,1400,500)
            self.assertIn(large[0],range(5))
            self.assertEqual(large[1],(0,0,-240))

    def test_six_unique_rooms(self):
        rooms = grotto_layout()
        self.assertEqual(len(rooms), 6)
        self.assertEqual(len({name for name, _ in rooms}), 6)
        self.assertEqual(len({position for _, position in rooms}), 6)

    def test_geometry_within_pool_and_no_air_pocket_obstruction(self):
        for name, (x, y, z) in grotto_layout():
            for label, (px, py, pz), scale in grotto_geometry(x, y):
                hx, hy, hz = (s * 50 for s in scale)
                self.assertLess(abs(px) + hx, MIN_HALF_WIDTH, (name, label))
                self.assertLess(abs(py) + hy, MIN_HALF_LENGTH, (name, label))
                self.assertLess(pz + hz, 0, (name, label))
                self.assertGreater(pz - hz, -500, (name, label))
                # Sphere-to-box clearance: at least 160 cm radius, floor may be tangent.
                d2 = sum(max(0, abs(a-b)-h)**2 for a,b,h in
                         zip((x,y,z),(px,py,pz),(hx,hy,hz)))
                self.assertGreaterEqual(d2, 150**2, (name, label))

    def test_art_landmarks_and_lights(self):
        self.assertEqual(len(GROTTO_ART), 6)
        self.assertEqual(len({entry[2] for entry in GROTTO_ART}), 6)
        for index, (_, (x, y, z)) in enumerate(grotto_layout()):
            ornaments, light, color = grotto_art(index, x, y)
            self.assertEqual(len(ornaments), 4)
            self.assertEqual(len({item[0] for item in ornaments}), 4)
            self.assertTrue(all(0 <= value <= 1 for value in color))
            self.assertLess(light[2], 0)
            for _, mesh, (px, py, pz), scale in ornaments:
                self.assertIn(mesh, ("Cone", "Sphere", "Cylinder"))
                self.assertLess(abs(px) + scale[0] * 50, MIN_HALF_WIDTH)
                self.assertLess(abs(py) + scale[1] * 50, MIN_HALF_LENGTH)
                self.assertLess(pz + scale[2] * 50, 0)
                self.assertGreater(pz - scale[2] * 50, -500)

    def test_shoreline_is_continuous_and_step_height_is_bounded(self):
        for width in (MIN_HALF_WIDTH, 4000.0):
            steps = shoreline_geometry(width)
            self.assertEqual(len(steps), 4)
            for (_, (x, _, z), scale), (_, (nx, _, nz), next_scale) in zip(steps, steps[1:]):
                self.assertLessEqual(nx - next_scale[0]*50, x + scale[0]*50)
                self.assertEqual(z - nz, 25.0)
            self.assertEqual(steps[0][1][0] + steps[0][2][0]*50, -width)

    def test_ruin_landmarks_clear_of_grottoes_and_current_lanes(self):
        ruins = ruin_geometry()
        self.assertEqual(len(ruins), 4)
        for _, (x, y, z), scale in ruins:
            self.assertGreater(abs(x)-scale[0]*50, 1100)
            self.assertLess(abs(x)+scale[0]*50, MIN_HALF_WIDTH)
            self.assertLess(abs(y)+scale[1]*50, MIN_HALF_LENGTH)
            self.assertGreaterEqual(z-scale[2]*50, -430)
            for _, (gx, gy, _) in grotto_layout():
                self.assertTrue(abs(x-gx)>350+scale[0]*50 or abs(y-gy)>305+scale[1]*50)

    def test_wrecks_are_within_pool_and_leave_open_ascent(self):
        import math
        self.assertEqual(len(WRECK_CENTERS), 2)
        pieces = tuple(wreck_geometry())
        self.assertEqual(len(pieces), 12)
        for name, (x,y,z), scale, yaw in pieces:
            angle=math.radians(yaw)
            hx=50*(abs(math.cos(angle))*scale[0]+abs(math.sin(angle))*scale[1])
            hy=50*(abs(math.sin(angle))*scale[0]+abs(math.cos(angle))*scale[1])
            hz=50*scale[2]
            self.assertLess(abs(x)+hx, MIN_HALF_WIDTH, name)
            self.assertLess(abs(y)+hy, MIN_HALF_LENGTH, name)
            self.assertGreaterEqual(z-hz,-430.0,name)
            self.assertLessEqual(z+hz,-190.0,name)
            for _, (gx,gy,_) in grotto_layout():
                self.assertTrue(abs(x-gx)>350+hx or abs(y-gy)>305+hy,name)
        for x,y,z in WRECK_CENTERS:
            self.assertGreater(abs(x), 1100.0)
            self.assertGreater(abs(y), 340.0)
            self.assertLess(z+90.0,0.0)

    def test_expanded_caves_and_island(self):
        for name,(x,y,z),scale in tuple(cave_geometry())+tuple(island_geometry()):
            hx,hy,hz=(s*50 for s in scale)
            self.assertLess(abs(x)+hx,EXPANDED_HALF_WIDTH,name)
            self.assertLess(abs(y)+hy,EXPANDED_HALF_LENGTH,name)
            self.assertGreaterEqual(z-hz,-430.0,name)
            for _,(gx,gy,_) in grotto_layout():
                self.assertTrue(abs(x-gx)>350+hx or abs(y-gy)>305+hy,name)
        caves=tuple(cave_geometry())
        self.assertEqual(len(caves),8)
        for i in (0,4):
            floor=caves[i]
            roof=caves[i+3]
            self.assertGreaterEqual(roof[1][2]-roof[2][2]*50-floor[1][2]-floor[2][2]*50, 250.0)
        island=tuple(island_geometry())
        self.assertEqual(len(island),8)
        for (_,(_,y,z),scale),(_,(_,ny,nz),next_scale) in zip(island[:6],island[1:7]):
            self.assertLessEqual(y-scale[1]*50,ny+next_scale[1]*50)
            self.assertEqual(z-nz,25.0)

    def test_scattered_gathering_nodes_are_submerged_and_clear(self):
        nodes=ocean_resources(MIN_HALF_WIDTH)
        self.assertEqual(len(nodes),12)
        self.assertEqual({node[0] for node in nodes},set(range(12)))
        self.assertEqual(len({node[3] for node in nodes}),12)
        for index,name,shape,(x,y,z),scale in nodes:
            hx,hy,hz=(s*50 for s in scale)
            self.assertLess(abs(x)+hx,MIN_HALF_WIDTH,name)
            self.assertLess(abs(y)+hy,MIN_HALF_LENGTH,name)
            self.assertLess(z+hz,0,name)
            self.assertGreater(z-hz,-410,name)
            for _,(gx,gy,_) in grotto_layout():
                self.assertTrue(abs(x-gx)>350+hx or abs(y-gy)>305+hy,name)

    def test_independent_swim_pose_contract(self):
        root = Path(__file__).resolve().parents[1]
        source = (root / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        rules = (root / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallWaterRules.h").read_text(encoding="utf-8")
        pose = source.split("void AMoonfallHeroPawn::RefreshSwimPose()", 1)[1].split("void AMoonfallHeroPawn::Tick", 1)[0]
        self.assertIn("SwimPoseAnchor->SetupAttachment(GetRootComponent())", source)
        self.assertIn("SwimPoseAnchor->SetRelativeRotation(Pose)", pose)
        self.assertIn("bWaterSwimming,bWaterSubmerged,bReduceMotion,WaterVerticalSpeed", pose)
        self.assertNotIn("GetMesh()", pose)
        self.assertNotIn("SetActorRotation", pose)
        self.assertIn("!bSwimming||!bSubmerged||bReducedMotion", rules)
        # The depth reset is part of respawn, before the neutral pose refresh.
        self.assertIn("bWaterSubmerged = false;\n WaterBreathingDepth=0.f;\n RefreshSwimPose();", source)

    def test_rooms_do_not_overlap(self):
        rooms = grotto_layout()
        for index, (_, (x,y,_)) in enumerate(rooms):
            for _, (ox,oy,_) in rooms[index+1:]:
                self.assertTrue(abs(x-ox) > 700 or abs(y-oy) > 610)


if __name__ == "__main__":
    unittest.main()
