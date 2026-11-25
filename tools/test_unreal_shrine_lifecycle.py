from pathlib import Path
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "MoonfallUnreal/Source/MoonfallUnreal/MoonfallCheckpointShrine.cpp"

class ShrineLifecycleTests(unittest.TestCase):
    def test_invalid_actor_and_health_rejected_before_discovery(self):
        body = SOURCE.read_text().split("void AMoonfallCheckpointShrine::TryDiscoverOrQueueRetry(", 1)[1].split("void AMoonfallCheckpointShrine::RetryPendingDiscovery()", 1)[0]
        for guard in ("!GetWorld()", "IsActorBeingDestroyed()", "Hero->IsActorBeingDestroyed()", "!FMath::IsFinite(Hero->Health)", "GetActorLocation().ContainsNaN()"):
            self.assertLess(body.index(guard), body.index("Hero->DiscoverShrine(this)"))
        after = body.split("if (Hero->DiscoverShrine(this))", 1)[1]
        self.assertLess(after.index("!IsValid(this)"), after.index("RefreshShrineDisplay(Hero)"))
        self.assertLess(after.index("!IsValid(this)"), after.index("GetWorldTimerManager().ClearTimer"))
    def test_retry_is_bounded_and_rejects_invalid_state(self):
        body = SOURCE.read_text().split("void AMoonfallCheckpointShrine::RetryPendingDiscovery()", 1)[1].split("void AMoonfallCheckpointShrine::BeginPlay()", 1)[0]
        for guard in ("!GetWorld()", "Hero->IsActorBeingDestroyed()", "!FMath::IsFinite(Hero->Health)", "GetActorLocation().ContainsNaN()", "++DiscoveryRetryAttempts > 8"):
            self.assertLess(body.index(guard), body.index("TryDiscoverOrQueueRetry(Hero)"))
        self.assertIn("PendingDiscoveryHero.Reset()", body)

if __name__ == "__main__":
    unittest.main()
