"""Source regression checks for nonboss in-world attack silhouette fallback.

No Unreal native rendering or packaged build is performed here.
"""
import unittest
from pathlib import Path

SRC=Path(__file__).resolve().parents[1]/"MoonfallUnreal"/"Source"/"MoonfallUnreal"


class AttackTellVisualTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.enemy=(SRC/"MoonfallEnemy.cpp").read_text(encoding="utf-8")
        cls.header=(SRC/"MoonfallEnemy.h").read_text(encoding="utf-8")
        cls.visual=(SRC/"MoonfallAttackTellVisual.h").read_text(encoding="utf-8")
        cls.hero=(SRC/"MoonfallHeroPawn.h").read_text(encoding="utf-8")

    def test_cue_has_no_collision_damage_or_new_actor(self):
        c=self.enemy.split('AttackTellMarker = CreateDefaultSubobject<UStaticMeshComponent>',1
            )[1].split('PrimaryActorTick.bCanEverTick = true;',1)[0]
        self.assertIn('AttackTellMarker->SetupAttachment(Body);',c)
        self.assertIn('/Engine/BasicShapes/Cube.Cube',c)
        self.assertIn('AttackTellMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);',c)
        self.assertIn('AttackTellMarker->SetCollisionResponseToAllChannels(ECR_Ignore);',c)
        self.assertIn('AttackTellMarker->SetGenerateOverlapEvents(false);',c)
        self.assertIn('AttackTellMarker->SetCastShadow(false);',c)
        self.assertIn('AttackTellMarker->SetVisibility(false);',c)
        self.assertIn('bool bEnableProceduralAttackTell = true;',self.header)
        self.assertIn('TObjectPtr<UStaticMeshComponent> AttackTellMarker = nullptr;',self.header)
        self.assertNotIn('SpawnActor',c)
        self.assertNotIn('SetActorLocation',c)

    def test_updates_only_existing_actor_tick_and_live_nonboss_windup(self):
        tick=self.enemy.split('// Reuse this existing 0.1-second AI update;',1)[1].split(
            'if (bBoss && Now < BossPhaseMessageUntil)',1)[0]
        self.assertIn('UpdateAttackTellMarker(Now, IsValid(Hero) && Hero->bReduceMotion);',tick)
        updater=self.enemy.split(
            'void AMoonfallEnemy::UpdateAttackTellMarker(float Now, bool bReducedMotion) {',1
        )[1].split('float AMoonfallEnemy::GetAttackWarningRemaining',1)[0]
        for required in (
            'bEnableProceduralAttackTell && !bBoss',
            '!ActiveNonBossTelegraphMove.IsNone()',
            'Health > 0.f', 'FMath::IsFinite(Now)',
            'FMath::IsFinite(WindupEndsAt) && WindupEndsAt > Now',
            'FMath::IsFinite(CurrentWindup) && CurrentWindup > 0.f',
            'FMath::IsFinite(CurrentAttackMultiplier)',
            'FMath::IsFinite(CurrentAttackRangeMultiplier)',
            'MoonfallAttackTellVisual::Select(',
            'MoonfallAttackTellVisual::Profile(Kind)',
            'MoonfallAttackTellVisual::Progress(',
            'MoonfallAttackTellVisual::Growth(Progress)',
        ):
            self.assertIn(required,updater)
        self.assertNotIn('SpawnActor',updater)
        self.assertNotIn('TActorIterator',updater)
        self.assertNotIn('LineTrace',updater)
        self.assertNotIn('SetActorLocation',updater)
        self.assertNotIn('GetWorldTimerManager',updater)

    def test_never_rotates_or_moves_authoritative_collision_root(self):
        update=self.enemy.split(
            'void AMoonfallEnemy::UpdateAttackTellMarker(float Now, bool bReducedMotion) {',1
        )[1].split('float AMoonfallEnemy::GetAttackWarningRemaining',1)[0]
        self.assertIn('AttackTellMarker->SetRelativeScale3D(Scale);',update)
        self.assertIn('AttackTellMarker->SetRelativeLocation(Location);',update)
        self.assertIn('AttackTellMarker->SetRelativeRotation(Rotation);',update)
        self.assertNotIn('Body->SetRelativeRotation(',update)
        self.assertNotIn('Body->SetRelativeLocation(',update)
        self.assertNotIn('AddActorWorldOffset(',update)
        self.assertNotIn('CurrentWindup =',update)
        self.assertNotIn('CurrentAttackMultiplier =',update)
        self.assertNotIn('NextAttackAt =',update)
        self.assertNotIn('AttackDamage =',update)

    def test_reduced_motion_uses_static_full_size_cues_without_flashing(self):
        update=self.enemy.split(
            'void AMoonfallEnemy::UpdateAttackTellMarker(float Now, bool bReducedMotion) {',1
        )[1].split('float AMoonfallEnemy::GetAttackWarningRemaining',1)[0]
        self.assertIn('Hero->bReduceMotion',self.enemy)
        self.assertIn('bool bReduceMotion = false;',self.hero)
        self.assertIn('bReducedMotion ? 0.f : 14.f * Progress',update)
        self.assertIn('bReducedMotion ? 0.f : Profile.YawDegrees * Progress',update)
        self.assertIn('if (!AttackTellMarker->IsVisible()) AttackTellMarker->SetVisibility(true);',update)
        self.assertNotIn('Sin(',update)
        self.assertNotIn('Blink',update)
        self.assertNotIn('SetWorldLocation',update)

    def test_marker_disappears_on_interruption_and_resolution(self):
        end=self.enemy.split(
            'void AMoonfallEnemy::EndNonBossTelegraph(bool bResolved) {',1
        )[1].split('void AMoonfallEnemy::UpdateAttackTellMarker',1)[0]
        self.assertIn('ActiveNonBossTelegraphMove = NAME_None;',end)
        self.assertIn('AttackTellMarker->SetVisibility(false);',end)
        self.assertLess(end.index('AttackTellMarker->SetVisibility(false);'),
                        end.index('OnAttackTelegraphResolved('))
        self.assertLess(end.index('AttackTellMarker->SetVisibility(false);'),
                        end.index('OnAttackTelegraphInterrupted('))
        self.assertEqual(self.enemy.count('EndNonBossTelegraph(true);'),1)
        self.assertGreaterEqual(self.enemy.count('EndNonBossTelegraph(false);'),8)
        self.assertIn('UpdateAttackTellMarker(Now, IsValid(Hero) && Hero->bReduceMotion);',self.enemy)

    def test_named_shapes_are_finite_sized_and_source_testable(self):
        for word in ('Quick','Heavy','Sweep','Crush','CounterSweep'):
            self.assertIn(word,self.visual)
        self.assertIn('constexpr FProfile Profile(',self.visual)
        self.assertIn('constexpr EMove Select(',self.visual)
        self.assertIn('constexpr float Progress(',self.visual)
        self.assertIn('constexpr float Growth(',self.visual)
        self.assertNotIn('FMath::Rand',self.visual)
        self.assertNotIn('new ',self.visual)
        self.assertNotIn('UPROPERTY',self.visual)


if __name__=="__main__":
    unittest.main()
