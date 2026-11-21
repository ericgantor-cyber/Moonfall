"""Source-level safety tests for Moonfall's mouse/controller gothic boat crossing.

These validate source contracts only, not UE5.4 native compilation or a
successful runtime boat trip. Run with python -m unittest -v.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal" / "Source" / "MoonfallUnreal"


class GothicFerryContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.h = (SRC / "MoonfallGothicFerry.h").read_text(encoding="utf-8")
        cls.boat = (SRC / "MoonfallGothicFerry.cpp").read_text(encoding="utf-8")
        cls.hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
        cls.world = (SRC / "MoonfallExpandedWorld.cpp").read_text(encoding="utf-8")
        cls.inputs = (ROOT / "MoonfallUnreal/Config/DefaultInput.ini").read_text(encoding="utf-8")

    def test_reversible_80m_crossing_uses_moving_boat_not_teleport(self):
        self.assertIn("CrossingDuration = 11.f;", self.h)
        self.assertIn("DockReach = 950.f;", self.h)
        self.assertIn("bIsland ? 246400.f : 237600.f", self.boat)
        self.assertIn("const FVector From = DockLocation(bAtIsland);", self.boat)
        self.assertIn("const FVector To = DockLocation(!bAtIsland);", self.boat)
        self.assertIn("SetActorLocation(FMath::Lerp(From, To, Eased));", self.boat)
        self.assertIn("const float Alpha = GetCrossingProgress();", self.boat)
        self.assertIn("FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f)", self.boat)
        self.assertIn("if (Alpha >= 1.f) FinishCrossing(true);", self.boat)
        self.assertIn("if (bArrived) bAtIsland = !bAtIsland;", self.boat)
        self.assertNotIn("UGameplayStatics::OpenLevel(", self.boat)
        self.assertNotIn("ServerTravel(", self.boat)
        # Only short boarding and safe onshore disembark change pawn position.
        self.assertEqual(self.boat.count("Hero->SetActorLocation("), 0)
        # Only a rider still on the ship can be placed on shore.
        self.assertEqual(self.boat.count("Rider->SetActorLocation("), 1)

    def test_destination_and_arrival_countdown_for_accessible_ui(self):
        self.assertIn("FText GetDestinationName() const;", self.h)
        self.assertIn("float GetSecondsUntilArrival() const;", self.h)
        destination = self.boat.split("FText AMoonfallGothicFerry::GetDestinationName() const", 1)[1].split(
            "float AMoonfallGothicFerry::GetSecondsUntilArrival() const", 1)[0]
        self.assertIn("bAtIsland ?", destination)
        self.assertIn('TEXT("Mainland Harbor")', destination)
        self.assertIn('TEXT("Gothic Isle")', destination)
        countdown = self.boat.split("float AMoonfallGothicFerry::GetSecondsUntilArrival() const", 1)[1].split(
            "bool AMoonfallGothicFerry::TryBoard(", 1)[0]
        self.assertIn("if (!bUnderway || !GetWorld()) return 0.f;", countdown)
        self.assertIn("CrossingDuration - Elapsed", countdown)
        self.assertIn("FMath::Clamp(", countdown)
        self.assertNotIn("TActorIterator", countdown)

    def test_no_ferry_status_text_visible_at_dock_or_during_voyage(self):
        # Keep data-only Blueprint accessors, remove the automatically rendered text.
        self.assertNotIn("VoyageStatus", self.boat)
        self.assertNotIn("VoyageStatus", self.h)
        self.assertNotIn("UpdateVoyageStatus", self.boat)
        self.assertNotIn("LastDisplayedSeconds", self.h)
        self.assertNotIn("RIGHT-CLICK / D-PAD UP", self.boat)
        self.assertNotIn("WAITING FOR CLEAR DOCK", self.boat)
        self.assertIn('TEXT("THE NIGHTFERRY")', self.boat)

    def test_boarding_requires_real_nearby_live_hero_and_open_ui(self):
        boarding = self.boat.split("bool AMoonfallGothicFerry::TryBoard(", 1)[1].split(
            "void AMoonfallGothicFerry::AdvanceCrossing()", 1)[0]
        for requirement in (
            "IsValid(Hero)", "bUnderway", "Passenger.IsValid()",
            "Hero->Health <= 0.f", "Hero->bOptionsOpen", "Hero->bInventoryOpen",
            "Hero->bAtlasOpen", "Hero->bResidentContextOpen",
            "Hero->GetWorld() != GetWorld()", "DockLocation(bAtIsland)",
            "FMath::Square(DockReach)", "FMath::Abs(Hero->GetActorLocation().Z - Dock.Z) > 350.f",
        ):
            self.assertIn(requirement, boarding, requirement)
        # A character must physically walk onto the deck first.
        self.assertIn("Hero->GetMovementBase() != Deck", boarding)
        self.assertIn("Movement->IsMovingOnGround()", boarding)
        self.assertNotIn("FindTeleportSpot", boarding)
        self.assertNotIn("Hero->SetActorLocation(", boarding)
        self.assertNotIn("Hero->SetBase(Deck)", boarding)
        self.assertNotIn("Movement->DisableMovement();", boarding)
        self.assertNotIn("Hero->AttachToActor(", boarding)
        self.assertIn("Passenger = Hero;", boarding)
        self.assertLess(boarding.index("Hero->GetMovementBase() != Deck"),
                        boarding.index("Passenger = Hero;"))

    def test_no_automatic_overboard_recovery_or_timer_pause(self):
        crossing = self.boat.split("void AMoonfallGothicFerry::AdvanceCrossing()", 1)[1].split(
            "void AMoonfallGothicFerry::FinishCrossing(", 1)[0]
        self.assertNotIn("bOverboard", crossing)
        self.assertNotIn("SafeDeck", crossing)
        self.assertNotIn("FindTeleportSpot", crossing)
        self.assertNotIn("Rider->SetActorLocation(", crossing)
        self.assertNotIn("Rider->SetBase(Deck)", crossing)
        self.assertNotIn("CrossingStartedAt +=", crossing)
        self.assertNotIn("LastCrossingUpdateAt", self.h)
        self.assertNotIn("LastCrossingUpdateAt", self.boat)
        self.assertIn("SetActorLocation(FMath::Lerp(From, To, Eased));", crossing)

    def test_hero_moves_with_ship_then_lands_safely_and_regains_movement(self):
        completion = self.boat.split("void AMoonfallGothicFerry::FinishCrossing(", 1)[1].split(
            "void AMoonfallGothicFerry::EndPlay(", 1)[0]
        self.assertIn("Rider->SetBase(nullptr);", completion)
        self.assertIn("const float IntoLand = bDestinationIsland ? 800.f : -800.f;", completion)
        self.assertIn("if (!GetWorld()->FindTeleportSpot(Rider, Landing, Rider->GetActorRotation()))", completion)
        self.assertNotIn("Rider->AttachToActor(", completion)
        self.assertIn("until the blocking obstruction is cleared", completion)
        self.assertIn("!Rider->SetActorLocation(Landing, false, nullptr, ETeleportType::TeleportPhysics)", completion)
        self.assertIn("Movement->SetMovementMode(MOVE_Walking);", completion)
        self.assertIn("bRiderOnDeck && FMath::IsFinite(Rider->Health) && Rider->Health > 0.f", completion)
        self.assertLess(completion.index("Rider->Health > 0.f"),
                        completion.index("Movement->SetMovementMode(MOVE_Walking)"))
        self.assertLess(completion.index("GetWorld()->FindTeleportSpot("),
                        completion.index("Rider->SetBase(nullptr)"))
        self.assertLess(completion.index("Rider->SetActorLocation(Landing"),
                        completion.index("Rider->SetBase(nullptr)"))
        # Read the actual C++ constants rather than re-asserting literals:
        # safe landfall must stay within controller boarding reach.
        reach = re.search(r"DockReach = ([0-9]+)[.]f;", self.h)
        landing = re.search(r"IntoLand = bDestinationIsland [?] ([0-9]+)[.]f : -([0-9]+)[.]f;", completion)
        self.assertIsNotNone(reach)
        self.assertIsNotNone(landing)
        self.assertLessEqual(max(int(landing.group(1)), int(landing.group(2))),
                             int(reach.group(1)))
        self.assertIn("} else if (IsValid(Rider) && bRiderOnDeck) {", completion)
        self.assertIn("} else if (IsValid(Rider) && bRiderOnDeck) {", completion)
        self.assertIn("Rider->GetMovementBase() == Deck", completion)
        self.assertGreaterEqual(completion.count("Rider->SetBase(nullptr);"), 2)
        self.assertIn("Passenger.Reset();", completion)
        self.assertIn("if (bUnderway) {", self.boat)
        self.assertIn("Passenger.Reset();", self.boat)
        self.assertIn("GetWorldTimerManager().ClearTimer(CrossingTimer);", self.boat)
        self.assertLess(completion.index("FindTeleportSpot"), completion.index("ClearTimer(CrossingTimer)"))
        self.assertIn("!IsValid(Rider)", self.boat)
        self.assertIn("FinishCrossing(false);", self.boat)

    def test_arrival_only_lands_rider_still_standing_on_deck(self):
        completion = self.boat.split("void AMoonfallGothicFerry::FinishCrossing(", 1)[1].split(
            "void AMoonfallGothicFerry::EndPlay(", 1)[0]
        self.assertIn("const bool bRiderOnDeck = IsValid(Rider) && Rider->GetMovementBase() == Deck;", completion)
        self.assertIn("if (bRiderOnDeck && FMath::IsFinite(Rider->Health)", completion)
        self.assertIn("} else if (IsValid(Rider) && bRiderOnDeck) {", completion)
        self.assertLess(completion.index("if (bRiderOnDeck && FMath::IsFinite(Rider->Health)"),
                        completion.index("Rider->SetActorLocation(Landing"))
        self.assertNotIn("SafeDeck", completion)

    def test_actor_teardown_releases_passenger_even_when_shore_blocked(self):
        teardown = self.boat.split("void AMoonfallGothicFerry::EndPlay(", 1)[1]
        executable = "\n".join(line for line in teardown.splitlines() if not line.strip().startswith("//"))
        self.assertNotIn("FinishCrossing(false)", executable)
        self.assertIn("Rider->SetBase(nullptr);", teardown)
        self.assertNotIn("SafeShore", teardown)
        self.assertNotIn("Rider->SetActorLocation(", teardown)
        self.assertIn("if (Rider->GetMovementBase() == Deck)", teardown)
        self.assertIn("Movement->SetMovementMode(MOVE_Falling);", teardown)
        self.assertNotIn("Movement->SetMovementMode(MOVE_Walking);", teardown)
        self.assertIn("Passenger.Reset();", teardown)
        self.assertIn("bUnderway = false;", teardown)
        self.assertLess(teardown.index("ClearTimer(CrossingTimer)"),
                        teardown.index("Rider->SetBase(nullptr)"))

    def test_ship_interaction_keeps_mouse_and_controller_combat_actions(self):
        mouse = self.hero.split("void AMoonfallHeroPawn::CastRightMouse() {", 1)[1].split(
            "void AMoonfallHeroPawn::ReleaseInventoryPointer()", 1)[0]
        npc = mouse.index("Cast<AMoonfallWorldNPC>(Hit.GetActor())")
        ferry = mouse.index("Cast<AMoonfallGothicFerry>(Hit.GetActor())")
        self.assertLess(ferry, npc)
        self.assertIn("if (Ferry->TryBoard(this))", mouse)
        self.assertIn("CastRight();", mouse)
        self.assertLess(mouse.index("if (bAtlasOpen) return;"), ferry)
        self.assertLess(mouse.index("if (bInventoryOpen)"), ferry)
        controller = self.hero.split("void AMoonfallHeroPawn::OpenFocusedResidentContext() {", 1)[1].split(
            "void AMoonfallHeroPawn::CastRightMouse()", 1)[0]
        self.assertIn("if (!IsValid(Resident) || !Resident->CanHeroInteract(this)) {", controller)
        self.assertIn("TActorIterator<AMoonfallGothicFerry>", controller)
        self.assertIn("Ferry->TryBoard(this)", controller)
        self.assertLess(controller.index("if (bOptionsOpen || bInventoryOpen || bAtlasOpen)"),
                        controller.index("TActorIterator<AMoonfallGothicFerry>"))
        self.assertIn('ActionName="CastRightMouse",Key=RightMouseButton', self.inputs)
        self.assertIn('ActionName="ResidentContext",Key=Gamepad_DPad_Up', self.inputs)
        self.assertIn('ActionName="CastRight",Key=X', self.inputs)

    def test_interaction_mesh_uses_visibility_only_not_blocking_character(self):
        self.assertIn("Hull->SetCollisionEnabled(ECollisionEnabled::QueryOnly);", self.boat)
        self.assertIn("Deck->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);", self.boat)
        self.assertIn("Deck->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);", self.boat)
        self.assertIn("TObjectPtr<UStaticMeshComponent> Deck;", self.h)
        self.assertIn("Hero->GetMovementBase() != Deck", self.boat)
        self.assertNotIn("Hero->SetBase(Deck);", self.boat)
        self.assertNotIn("Movement->DisableMovement();", self.boat)
        for name in ("Hull", "Mast", "Sail"):
            self.assertIn(name + "->SetCollisionResponseToAllChannels(ECR_Ignore);", self.boat)
            self.assertIn(name + "->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);", self.boat)
        self.assertIn('TEXT("THE NIGHTFERRY")', self.boat)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", self.boat)
        self.assertNotIn("void AMoonfallGothicFerry::Tick(", self.boat)
        self.assertIn("0.05f, true);", self.boat)

    def test_gangplank_deployed_at_dock_and_stowed_during_voyage(self):
        self.assertIn("TObjectPtr<UStaticMeshComponent> Gangplank;", self.h)
        self.assertIn("void SetGangplankDeployed(bool bDeployed);", self.h)
        self.assertIn("Gangplank->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);", self.boat)
        self.assertIn("Gangplank->SetCanEverAffectNavigation(false);", self.boat)
        ramp = self.boat.split("void AMoonfallGothicFerry::SetGangplankDeployed(", 1)[1].split(
            "float AMoonfallGothicFerry::GetCrossingProgress()", 1)[0]
        self.assertIn("bAtIsland ? -1.f : 1.f", ramp)
        self.assertIn("SetRelativeRotation(FRotator(", ramp)
        self.assertIn("SetHiddenInGame(!bDeployed)", ramp)
        self.assertIn("ECollisionEnabled::NoCollision", ramp)
        start = self.boat.split("void AMoonfallGothicFerry::BeginPlay()", 1)[1].split(
            "void AMoonfallGothicFerry::SetGangplankDeployed(", 1)[0]
        self.assertIn("SetGangplankDeployed(true)", start)
        boarding = self.boat.split("bool AMoonfallGothicFerry::TryBoard(", 1)[1].split(
            "void AMoonfallGothicFerry::AdvanceCrossing()", 1)[0]
        self.assertIn("SetGangplankDeployed(false)", boarding)
        completion = self.boat.split("void AMoonfallGothicFerry::FinishCrossing(", 1)[1].split(
            "void AMoonfallGothicFerry::EndPlay(", 1)[0]
        self.assertIn("SetGangplankDeployed(true)", completion)

    def test_swimming_controls_reuse_camera_and_jump_without_new_combat_bindings(self):
        jump = self.hero.split("void AMoonfallHeroPawn::TryJump()", 1)[1].split(
            "void AMoonfallHeroPawn::MoveForward(", 1)[0]
        forward = self.hero.split("void AMoonfallHeroPawn::MoveForward(", 1)[1].split(
            "void AMoonfallHeroPawn::MoveRight(", 1)[0]
        self.assertIn("GetCharacterMovement()->IsSwimming()", jump)
        self.assertIn("bSwimAscendHeld = true;", jump)
        self.assertLess(jump.index("bSwimAscendHeld = true;"), jump.index("GetCharacterMovement()->IsSwimming()"))
        self.assertLess(jump.index("bSwimAscendHeld = true;"), jump.index("Jump();"))
        self.assertLess(jump.index("if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen) return;"),
                        jump.index("bSwimAscendHeld = true;"))
        self.assertIn("if (GetCharacterMovement() && GetCharacterMovement()->IsSwimming())\n  return;", jump)
        self.assertIn("bool bSwimAscendHeld = false;", (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8"))
        self.assertIn("void AMoonfallHeroPawn::ReleaseJump()", self.hero)
        self.assertIn("bSwimAscendHeld = false;", self.hero)
        self.assertIn("StopJumping();", self.hero)
        self.assertIn('Input->BindAction(TEXT("Jump"), IE_Released, this, &AMoonfallHeroPawn::ReleaseJump);', self.hero)
        tick = self.hero.split("void AMoonfallHeroPawn::Tick(float DeltaSeconds)", 1)[1].split(
            "void AMoonfallHeroPawn::SetupPlayerInputComponent(", 1)[0]
        self.assertIn("if (bSwimAscendHeld && Health > 0.f", tick)
        self.assertIn("GetCharacterMovement()->IsSwimming()", tick)
        self.assertIn("AddMovementInput(FVector::UpVector, 1.f);", tick)
        self.assertLess(tick.index("AddMovementInput(FVector::UpVector, 1.f);"), tick.index("Super::Tick(DeltaSeconds);"))
        self.assertIn("GetCharacterMovement()->IsSwimming()", forward)
        self.assertIn("Controller->GetControlRotation()", forward)
        self.assertIn("FRotationMatrix(Rotation).GetUnitAxis(EAxis::X)", forward)
        self.assertIn('ActionName="Jump",Key=SpaceBar', self.inputs)
        self.assertIn('ActionName="Jump",Key=Gamepad_FaceButton_Bottom', self.inputs)
        self.assertIn('AxisName="MoveForward",Scale=1.000000,Key=Gamepad_LeftY', self.inputs)

    def test_wow_forever_style_water_entry_uses_swimming_not_rescue(self):
        world_header = (SRC / "MoonfallExpandedWorld.h").read_text(encoding="utf-8")
        swimming = self.world.split("void AMoonfallExpandedWorld::UpdateSeaSwimming()", 1)[1].split(
            "void AMoonfallExpandedWorld::RefreshNearbyTiles()", 1)[0]
        self.assertIn("TWeakObjectPtr<class AMoonfallHeroPawn> SeaSwimmer;", world_header)
        # The swim area must extend beyond the narrow piers but remain bounded.
        self.assertIn("FMath::Abs(Position.X) < 5000.f", swimming)
        self.assertNotIn("FMath::Abs(Position.X) < 1300.f", swimming)
        self.assertIn("Position.Y > 238000.f && Position.Y < 246000.f", swimming)
        self.assertIn("Position.Z < -40.f", swimming)
        self.assertIn("const bool bLeaveWater = !bSeaGap || Position.Z > 60.f;", swimming)
        self.assertIn("if (Movement->MovementMode != MOVE_Swimming)", swimming)
        self.assertIn("if (!FMath::IsFinite(Hero->Health) || Hero->Health <= 0.f)", swimming)
        self.assertIn("if (Position.ContainsNaN())", swimming)
        self.assertLess(swimming.index("Hero->Health <= 0.f"), swimming.index("Movement->SetMovementMode(MOVE_Swimming);"))
        self.assertLess(swimming.index("const bool bLeaveWater"), swimming.index("Movement->SetMovementMode(MOVE_Falling);"))
        self.assertIn("Movement->MovementMode == MOVE_Falling", swimming)
        self.assertIn("Movement->SetMovementMode(MOVE_Swimming);", swimming)
        self.assertIn("Movement->SetMovementMode(MOVE_Falling);", swimming)
        self.assertIn("} else if (bOnFerry) {", swimming)
        self.assertIn("Movement->SetMovementMode(MOVE_Walking);", swimming)
        self.assertIn("} else if (bLeaveWater) {", swimming)
        self.assertLess(swimming.index("} else if (bOnFerry) {"),
                        swimming.index("} else if (bLeaveWater) {"))
        deck_exit = swimming.split("} else if (bOnFerry) {", 1)[1].split(
            "} else if (bLeaveWater) {", 1)[0]
        self.assertNotIn("SetActorLocation(", deck_exit)
        self.assertNotIn("SetBase(", deck_exit)
        self.assertIn("Hero->GetMovementBase()->GetOwner()", swimming)
        self.assertNotIn("SetActorLocation(", swimming)
        self.assertNotIn("FindTeleportSpot", swimming)
        self.assertNotIn("Rider->SetBase(Deck)", swimming)
        self.assertIn("FTimerHandle SeaSwimmingTimer;", world_header)
        self.assertIn("&AMoonfallExpandedWorld::UpdateSeaSwimming, 0.1f, true);", self.world)
        self.assertIn("GetWorldTimerManager().ClearTimer(SeaSwimmingTimer);", self.world)
        self.assertIn("SeaSwimmer.Reset();", self.world)
        streaming = self.world.split("void AMoonfallExpandedWorld::RefreshNearbyTiles()", 1)[1]
        self.assertNotIn("UpdateSeaSwimming();", streaming)
        self.assertIn("&AMoonfallExpandedWorld::RefreshNearbyTiles, 0.35f, true, 0.1f);", self.world)

    def test_single_instance_in_connected_overworld(self):
        start = self.world.split("void AMoonfallExpandedWorld::BeginPlay()", 1)[1].split(
            "void AMoonfallExpandedWorld::EndPlay(", 1
        )[0]
        self.assertIn("TActorIterator<AMoonfallGothicFerry>", start)
        self.assertIn("if (!bFerryAlreadyExists && GetWorld())", start)
        self.assertIn("SpawnActor<AMoonfallGothicFerry>(", start)
        self.assertIn("AMoonfallGothicFerry::DockLocation(false)", start)
        # Shore docks are kept separated by open water without a land bridge.
        self.assertIn("FVector(0.f, 238000.f, 0.f)", self.world)
        self.assertIn("FVector(0.f, 246000.f, 0.f)", self.world)
        self.assertNotRegex(
            self.world,
            re.compile(r"AddRoad\(FVector\(0\.f, 238000\.f, 0\.f\),\s*FVector\(0\.f, 246000\.f, 0\.f\)"))


if __name__ == "__main__":
    unittest.main()
