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
        self.assertEqual(self.boat.count("Hero->SetActorLocation("), 2)
        self.assertEqual(self.boat.count("Rider->SetActorLocation("), 2)

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
        self.assertLess(boarding.index("FMath::Square(DockReach)"),
                        boarding.index("Hero->SetActorLocation(BoardingLocation"))
        self.assertIn("GetWorld()->FindTeleportSpot(Hero, BoardingLocation, Hero->GetActorRotation())", boarding)
        self.assertLess(boarding.index("FindTeleportSpot(Hero, BoardingLocation"),
                        boarding.index("Hero->SetActorLocation(BoardingLocation"))
        self.assertIn("Hero->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform)", boarding)
        self.assertIn("Movement->StopMovementImmediately();", boarding)
        self.assertIn("Movement->DisableMovement();", boarding)
        self.assertIn("Passenger = Hero;", boarding)
        self.assertIn("const FVector OriginalLocation = Hero->GetActorLocation();", boarding)
        self.assertIn("Hero->SetActorLocation(OriginalLocation, false, nullptr, ETeleportType::TeleportPhysics);", boarding)
        self.assertLess(boarding.index("Hero->AttachToActor("),
                        boarding.index("Hero->SetActorLocation(OriginalLocation"))

    def test_hero_moves_with_ship_then_lands_safely_and_regains_movement(self):
        completion = self.boat.split("void AMoonfallGothicFerry::FinishCrossing(", 1)[1].split(
            "void AMoonfallGothicFerry::EndPlay(", 1)[0]
        self.assertIn("Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform)", completion)
        self.assertIn("const float IntoLand = bDestinationIsland ? 800.f : -800.f;", completion)
        self.assertIn("if (!GetWorld()->FindTeleportSpot(Rider, Landing, Rider->GetActorRotation()))", completion)
        self.assertIn("Rider->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);", completion)
        self.assertIn("until the blocking obstruction is cleared", completion)
        self.assertIn("!Rider->SetActorLocation(Landing, false, nullptr, ETeleportType::TeleportPhysics)", completion)
        self.assertIn("Movement->SetMovementMode(MOVE_Walking);", completion)
        self.assertIn("FMath::IsFinite(Rider->Health) && Rider->Health > 0.f", completion)
        self.assertLess(completion.index("Rider->Health > 0.f"),
                        completion.index("Movement->SetMovementMode(MOVE_Walking)"))
        self.assertLess(completion.index("GetWorld()->FindTeleportSpot("),
                        completion.index("Rider->DetachFromActor("))
        self.assertLess(completion.index("Rider->DetachFromActor("),
                        completion.index("Rider->SetActorLocation(Landing"))
        # Read the actual C++ constants rather than re-asserting literals:
        # safe landfall must stay within controller boarding reach.
        reach = re.search(r"DockReach = ([0-9]+)[.]f;", self.h)
        landing = re.search(r"IntoLand = bDestinationIsland [?] ([0-9]+)[.]f : -([0-9]+)[.]f;", completion)
        self.assertIsNotNone(reach)
        self.assertIsNotNone(landing)
        self.assertLessEqual(max(int(landing.group(1)), int(landing.group(2))),
                             int(reach.group(1)))
        self.assertIn("} else if (IsValid(Rider)) {", completion)
        self.assertIn("The respawn/death", completion)
        self.assertGreaterEqual(completion.count("Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);"), 2)
        self.assertIn("Passenger.Reset();", completion)
        self.assertIn("if (bUnderway) {", self.boat)
        self.assertIn("Passenger.Reset();", self.boat)
        self.assertIn("GetWorldTimerManager().ClearTimer(CrossingTimer);", self.boat)
        self.assertLess(completion.index("FindTeleportSpot"), completion.index("ClearTimer(CrossingTimer)"))
        self.assertIn("!IsValid(Rider)", self.boat)
        self.assertIn("FinishCrossing(false);", self.boat)

    def test_actor_teardown_releases_passenger_even_when_shore_blocked(self):
        teardown = self.boat.split("void AMoonfallGothicFerry::EndPlay(", 1)[1]
        self.assertNotIn("FinishCrossing(false)", teardown)
        self.assertIn("Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);", teardown)
        self.assertIn("GetWorld()->FindTeleportSpot(Rider, SafeShore, Rider->GetActorRotation())", teardown)
        self.assertIn("Movement->SetMovementMode(MOVE_Walking);", teardown)
        self.assertIn("Passenger.Reset();", teardown)
        self.assertIn("bUnderway = false;", teardown)
        self.assertLess(teardown.index("ClearTimer(CrossingTimer)"),
                        teardown.index("Rider->DetachFromActor("))

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
        for name in ("Hull", "Mast", "Sail"):
            self.assertIn(name + "->SetCollisionResponseToAllChannels(ECR_Ignore);", self.boat)
            self.assertIn(name + "->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);", self.boat)
        self.assertIn('TEXT("THE NIGHTFERRY")', self.boat)
        self.assertIn("PrimaryActorTick.bCanEverTick = false;", self.boat)
        self.assertNotIn("void AMoonfallGothicFerry::Tick(", self.boat)
        self.assertIn("0.05f, true);", self.boat)

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
