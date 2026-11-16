#include "MoonfallPawn.h"
#include "MoonfallProgressSave.h"
#include "MoonfallLootRules.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AMoonfallPawn::AMoonfallPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Player0;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    HeroMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hero"));
    HeroMesh->SetupAttachment(Root);
    HeroMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BossMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Boss"));
    BossMesh->SetupAttachment(Root);
    BossMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WarningMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Telegraph"));
    WarningMesh->SetupAttachment(Root);
    WarningMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
    FloorMesh->SetupAttachment(Root);
    FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BossPortrait = CreateDefaultSubobject<UBillboardComponent>(TEXT("OriginalBossArtwork"));
    BossPortrait->SetupAttachment(BossMesh);
    BossPortrait->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
    BossPortrait->SetRelativeScale3D(FVector(2.5f));
    BossPortrait->SetHiddenInGame(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cube.Succeeded()) { FloorMesh->SetStaticMesh(Cube.Object); RegionShape = Cube.Object; }
    if (Sphere.Succeeded())
    {
        EnemyShape = Sphere.Object;
        HeroMesh->SetStaticMesh(Sphere.Object);
        BossMesh->SetStaticMesh(Sphere.Object);
    }
    if (Cylinder.Succeeded()) WarningMesh->SetStaticMesh(Cylinder.Object);
    HeroMesh->SetRelativeLocation(FVector(0.f, -500.f, 100.f));
    BossMesh->SetRelativeLocation(FVector(MoonfallWorldRules::BossX, 600.f, 100.f));
    BossMesh->SetRelativeScale3D(FVector(2.f));
    WarningMesh->SetRelativeScale3D(FVector(5.f, 5.f, 0.01f));
    WarningMesh->SetVisibility(false);
    FloorMesh->SetRelativeLocation(FVector(0.f, 0.f, -30.f));
    FloorMesh->SetRelativeScale3D(FVector(34.f, 34.f, 0.5f));
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(Root);
    CameraBoom->TargetArmLength = ZoomDistance;
    CameraBoom->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
    CameraBoom->bDoCollisionTest = false;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraBoom);
}

void AMoonfallPawn::EndPlay(const EEndPlayReason::Type Reason)
{
    // Save on real game exit/level travel, not arbitrary editor/actor destruction.
    // If the hero died, SaveProgress retains the last living checkpoint.
    if (Reason == EEndPlayReason::Quit || Reason == EEndPlayReason::LevelTransition)
    {
        if (HeroMesh && GetWorld()) SaveProgress();
    }
    Super::EndPlay(Reason);
}

void AMoonfallPawn::BeginPlay()
{
    Super::BeginPlay();
    bool bLoadedCheckpoint = false;
    // Separate prototype save slot. Never touches production Godot saves.
    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(TEXT("MoonfallPrototypeBossRewards"), 0))
    {
        if (const UMoonfallProgressSave* Progress = Cast<UMoonfallProgressSave>(Loaded))
        {
            Victories = FMath::Clamp(Progress->PermanentBossVictories, 0, 100);
            bWon = Progress->bBossDefeated;
            // Checkpoint is restored before the world is constructed.
            if (Progress->bHasCheckpoint && FMath::IsFinite(Progress->SavedHealth) &&
                Progress->SavedHealth > 0.f &&
                !Progress->SavedHeroPosition.ContainsNaN())
            {
                const FVector Saved = Progress->SavedHeroPosition;
                if (FMath::IsFinite(Saved.X) && FMath::IsFinite(Saved.Y))
                {
                    HeroMesh->SetRelativeLocation(FVector(
                        FMath::Clamp(Saved.X, -1600.f, 17600.f),
                        FMath::Clamp(Saved.Y, -3200.f, 3200.f), 100.f));
                    Health = FMath::Clamp(Progress->SavedHealth, 1.f,
                        100.f + 5.f * FMath::Clamp(Progress->PermanentBossVictories, 0, 100) +
                        2.f * (FMath::Clamp(Progress->HeroLevel, 1, 100) - 1));
                    if (Progress->bHasResourceCheckpoint)
                    {
                        if (FMath::IsFinite(Progress->SavedMana))
                            Mana = FMath::Clamp(Progress->SavedMana, 0.f, 180.f);
                        if (FMath::IsFinite(Progress->SavedStamina))
                            Stamina = FMath::Clamp(Progress->SavedStamina, 0.f, 150.f);
                    }
                    bLoadedCheckpoint = true;
                }
            }
            // Keep world achievements, inventory, and discovery flags from a
            // legacy or damaged save, but repair an unusable respawn snapshot.
            // Otherwise bHasCheckpoint may stay true forever and prevent
            // safe checkpoint bootstrap when enemies occupy the start.
            bCheckpointNeedsRepair = Progress->bHasCheckpoint && !bLoadedCheckpoint;
            // Legacy SelectedHeroIndex is retained in old save objects but ignored:
            // Z and Nyra are encounter bosses, not player avatar choices.
            DefeatedEnemyMask = Progress->DefeatedEnemyMask & ((1 << MoonfallWorldRules::TotalEnemies) - 1);
            DiscoveredRegionMask = Progress->DiscoveredRegionMask & ((1 << MoonfallWorldRules::RegionCount) - 1);
            if (Progress->DiscoveredMapCells.Num() == 48 * 16)
                DiscoveredMapCells = Progress->DiscoveredMapCells;
            const FVector CandidateWaypoint = Progress->SavedWaypoint;
            if (Progress->bHasWaypoint && !CandidateWaypoint.ContainsNaN() &&
                FMath::IsFinite(CandidateWaypoint.X) && FMath::IsFinite(CandidateWaypoint.Y) &&
                CandidateWaypoint.X >= -1600.f && CandidateWaypoint.X <= 17600.f &&
                CandidateWaypoint.Y >= -3200.f && CandidateWaypoint.Y <= 3200.f)
            {
                Waypoint = FVector(CandidateWaypoint.X, CandidateWaypoint.Y, 0.f);
                bHasWaypoint = true;
            }
            ClaimedChamberMask = Progress->ClaimedChamberMask & ((1 << MoonfallWorldRules::TotalChambers) - 1);
            DefeatedGuardianMask = Progress->DefeatedGuardianMask & ((1 << MoonfallWorldRules::TotalChambers) - 1);
            // Never allow saved chamber claims without a defeated guardian.
            // Restores Nyra's final-chamber encounter if flags are inconsistent.
            ClaimedChamberMask &= DefeatedGuardianMask;
            Progression.Level = FMath::Clamp(Progress->HeroLevel, 1, 100);
            Progression.Experience = FMath::Clamp(Progress->HeroXP, 0, Progression.ExperienceToNext() - 1);
            Progression.HeroPoints = FMath::Clamp(Progress->HeroPoints, 0, 100);
            Progression.TamerPoints = FMath::Clamp(Progress->TamerPoints, 0, 100);
            for (const FString& Relic : Progress->BossRelics)
            {
                if (BossInventory.Num() >= 100) break;
                if (!Relic.IsEmpty() && Relic.Len() <= 100) BossInventory.Add(Relic);
            }
        }
    }
    if (!bLoadedCheckpoint)
        Health = GetHudMaxHealth(); // Full health at a new start, never from kills.
    // Loaded living checkpoints may be within enemy aggro range. Grant a short
    // damage-only grace period so respawn cannot instantly chain into death.
    RespawnGraceTimer = bLoadedCheckpoint ? 2.5f : 0.f;
    // Load original boss art before world generation so Nyra's guardian can use it.
    // Missing imports retain the regular prototype mesh.
    ZArtwork = LoadObject<UTexture2D>(nullptr, TEXT("/Game/MoonfallArt/heroes/Z_Original.Z_Original"));
    NyraArtwork = LoadObject<UTexture2D>(nullptr, TEXT("/Game/MoonfallArt/heroes/Nyra_Original.Nyra_Original"));
    // Reveal the starting / restored region, but do not expose future regions.
    DiscoveredRegionMask |= 1 << MoonfallWorldRules::RegionAt(HeroMesh->GetRelativeLocation().X);
    RevealNearbyMapCells(HeroMesh->GetRelativeLocation());
    // Start the isometric camera at the loaded checkpoint instead of
    // flying from the origin whenever a save restores another region.
    if (CameraBoom && HeroMesh)
    {
        const FVector HeroPosition = HeroMesh->GetRelativeLocation();
        CameraBoom->SetRelativeLocation(FVector(HeroPosition.X, HeroPosition.Y, 0.f));
    }
    BuildWorld();
    if (bCheckpointNeedsRepair)
    {
        // Replace the damaged checkpoint at the new-game starting position.
        // This is an explicit repair, not a reset of earned loot or discoveries.
        SaveProgress();
    }
    // Runtime boot marker for the trusted Windows smoke test. A map name in the
    // engine log alone does not prove the pawn or its continuous world started.
    if (GetWorld() && HeroMesh && FloorMesh && Camera && Health > 0.f)
    {
        UE_LOG(LogTemp, Display,
            TEXT("MOONFALL_BOOT_READY pawn=MoonfallPawn regions=%d chambers=%d"),
            MoonfallWorldRules::RegionCount, MoonfallWorldRules::TotalChambers);
    }
    // Z is the main god/boss; never assign original character art to HeroMesh.
    BossPortrait->SetSprite(ZArtwork);
    BossPortrait->SetHiddenInGame(ZArtwork == nullptr);
    if (ZArtwork) BossMesh->SetHiddenInGame(true);
    if (bWon)
    {
        BossHealth = 0.f;
        BossMesh->SetVisibility(false);
        BossPortrait->SetHiddenInGame(true);
    }

}

UBillboardComponent* AMoonfallPawn::AddWorldArtwork(
    USceneComponent* Parent, const FString& AssetPath,
    const FVector& LocalPosition, float Scale)
{
    // Missing Unreal Editor imports leave safe geometric placeholders in place.
    UTexture2D* Art = LoadObject<UTexture2D>(nullptr, *AssetPath);
    if (!Art || !Parent) return nullptr;
    UBillboardComponent* Billboard = NewObject<UBillboardComponent>(this);
    Billboard->SetupAttachment(Parent);
    Billboard->SetSprite(Art);
    Billboard->SetRelativeLocation(LocalPosition);
    Billboard->SetRelativeScale3D(FVector(Scale));
    Billboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AddInstanceComponent(Billboard);
    Billboard->RegisterComponent();
    return Billboard;
}

void AMoonfallPawn::BuildWorld()
{
    // One contiguous strip of six 34 m cells, overlapping 2 m; no waves.
    static const TCHAR* BeastNames[MoonfallWorldRules::RegionCount] =
        { TEXT("wolf"), TEXT("fox"), TEXT("boar"), TEXT("elk"), TEXT("panther"), TEXT("serpent") };
    for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
    {
        if (Region > 0 && RegionShape)
        {
            UStaticMeshComponent* Ground = NewObject<UStaticMeshComponent>(this);
            Ground->SetupAttachment(Root);
            Ground->SetStaticMesh(RegionShape);
            Ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Ground->SetRelativeLocation(FVector(MoonfallWorldRules::RegionCenter(Region), 0.f, -30.f));
            Ground->SetRelativeScale3D(FVector(34.f, 34.f, .5f));
            AddInstanceComponent(Ground);
            Ground->RegisterComponent();
        }
        // Original imported region illustration as a non-colliding world landmark.
        const FString RegionArtPath = FString::Printf(
            TEXT("/Game/MoonfallArt/v31_4_bestiary/region_%d.region_%d"), Region, Region);
        AddWorldArtwork(Root, RegionArtPath,
            FVector(MoonfallWorldRules::RegionCenter(Region), 1300.f, 300.f), 7.f);
        // New AI-generated environment crops, optional after Unreal Editor import.
        if (Region == 0)
            AddWorldArtwork(Root,
                TEXT("/Game/MoonfallGeneratedArt/moonfall_ai_waterfall_ruins.moonfall_ai_waterfall_ruins"),
                FVector(MoonfallWorldRules::RegionCenter(Region) - 1200.f, 1300.f, 400.f), 8.f);
        if (Region == 4)
            AddWorldArtwork(Root,
                TEXT("/Game/MoonfallGeneratedArt/moonfall_ai_moonlit_gate.moonfall_ai_moonlit_gate"),
                FVector(MoonfallWorldRules::RegionCenter(Region) + 1000.f, 1300.f, 400.f), 9.f);
        // Five side chambers directly overlap each region's edge, with no portals.
        for (int32 Chamber = 0; Chamber < MoonfallWorldRules::ChambersPerRegion; ++Chamber)
        {
            const FVector Center = MoonfallWorldRules::ChamberCenter(Region, Chamber);
            UStaticMeshComponent* Floor = NewObject<UStaticMeshComponent>(this);
            Floor->SetupAttachment(Root);
            Floor->SetStaticMesh(RegionShape);
            Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Floor->SetRelativeLocation(FVector(Center.X, Center.Y, -30.f));
            Floor->SetRelativeScale3D(FVector(16.f, 16.f, .5f));
            AddInstanceComponent(Floor);
            Floor->RegisterComponent();
            const int32 ChamberId = Region * MoonfallWorldRules::ChambersPerRegion + Chamber;
            if ((ClaimedChamberMask & (1 << ChamberId)) != 0) continue;
            UStaticMeshComponent* Cache = NewObject<UStaticMeshComponent>(this);
            Cache->SetupAttachment(Root);
            Cache->SetStaticMesh(EnemyShape);
            Cache->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Cache->SetRelativeLocation(MoonfallWorldRules::ChamberReward(Region, Chamber));
            Cache->SetRelativeScale3D(FVector(.55f));
            AddInstanceComponent(Cache);
            Cache->RegisterComponent();
            FChamberCache Reward;
            Reward.Mesh = Cache;
            Reward.Artwork = AddWorldArtwork(Cache,
                TEXT("/Game/MoonfallArt/v31_3_art/treasure_cache.treasure_cache"),
                FVector::ZeroVector, 2.f);
            if (Reward.Artwork) Cache->SetHiddenInGame(true);
            Reward.Id = ChamberId;
            ChamberCaches.Add(Reward);
            if ((DefeatedGuardianMask & (1 << ChamberId)) == 0)
            {
                UStaticMeshComponent* GuardianBody = NewObject<UStaticMeshComponent>(this);
                GuardianBody->SetupAttachment(Root);
                GuardianBody->SetStaticMesh(EnemyShape);
                GuardianBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                GuardianBody->SetRelativeLocation(MoonfallWorldRules::ChamberReward(Region, Chamber) + FVector(0.f, 400.f, 0.f));
                GuardianBody->SetRelativeScale3D(FVector(1.3f));
                AddInstanceComponent(GuardianBody);
                GuardianBody->RegisterComponent();
                FWorldEnemy Guardian;
                Guardian.Mesh = GuardianBody;
                Guardian.Health = MoonfallWorldRules::EnemyMaxHealth(Region) * 1.8f;
                Guardian.Region = Region;
                Guardian.Id = ChamberId;
                Guardian.Home = GuardianBody->GetRelativeLocation();
                Guardian.bGuardian = true;
                // Nyra guards the last connected chamber as a real, defeatable
                // encounter using the existing guardian HP, attacks, XP and save mask.
                if (ChamberId == MoonfallWorldRules::TotalChambers - 1 && NyraArtwork)
                {
                    // NyraArtwork loads in BeginPlay before BuildWorld (see below).
                    UBillboardComponent* NyraBillboard = NewObject<UBillboardComponent>(this);
                    NyraBillboard->SetupAttachment(GuardianBody);
                    NyraBillboard->SetSprite(NyraArtwork);
                    NyraBillboard->SetRelativeScale3D(FVector(2.5f));
                    AddInstanceComponent(NyraBillboard);
                    NyraBillboard->RegisterComponent();
                    GuardianBody->SetHiddenInGame(true);
                    Guardian.Portrait = NyraBillboard;
                }
                Enemies.Add(Guardian);
            }
        }
        const FString Name = BeastNames[Region];
        UTexture2D* OriginalArt = LoadObject<UTexture2D>(nullptr,
            *FString::Printf(TEXT("/Game/MoonfallArt/v31_4_bestiary/%s.%s"), *Name, *Name));
        for (int32 Slot = 0; Slot < MoonfallWorldRules::EnemiesPerRegion; ++Slot)
        {
            const int32 Id = Region * MoonfallWorldRules::EnemiesPerRegion + Slot;
            if ((DefeatedEnemyMask & (1 << Id)) != 0) continue;
            UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(this);
            Body->SetupAttachment(Root);
            Body->SetStaticMesh(EnemyShape);
            Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Body->SetRelativeLocation(MoonfallWorldRules::EnemyPosition(Region, Slot));
            Body->SetRelativeScale3D(FVector(1.1f));
            AddInstanceComponent(Body);
            Body->RegisterComponent();

            UBillboardComponent* Portrait = nullptr;
            if (OriginalArt)
            {
                Portrait = NewObject<UBillboardComponent>(this);
                Portrait->SetupAttachment(Body);
                Portrait->SetSprite(OriginalArt);
                Portrait->SetRelativeScale3D(FVector(2.f));
                AddInstanceComponent(Portrait);
                Portrait->RegisterComponent();
                Body->SetHiddenInGame(true);
            }

            FWorldEnemy Enemy;
            Enemy.Mesh = Body;
            Enemy.Portrait = Portrait;
            Enemy.Health = MoonfallWorldRules::EnemyMaxHealth(Region);
            Enemy.Region = Region;
            Enemy.Id = Id;
            Enemy.Home = Body->GetRelativeLocation();
            Enemies.Add(Enemy);
        }
    }
}

void AMoonfallPawn::UpdateWorldEnemies(float Dt)
{
    // Bounded, allocation-free local AI. Inactive enemies return home instead of
    // freezing on their leash boundary. Squared distances avoid per-enemy sqrt calls.
    const FVector HeroPosition = HeroMesh->GetRelativeLocation();
    const float MoveDt = FMath::Clamp(Dt, 0.f, 0.05f);
    for (FWorldEnemy& Enemy : Enemies)
    {
        if (!Enemy.Mesh) continue;
        // An attack is a one-frame event. Holding contact cannot deal damage
        // without a fresh, telegraphed windup and cooldown.
        Enemy.bAttackReadyThisFrame = false;
        Enemy.AttackCooldown = FMath::Max(0.f, Enemy.AttackCooldown - MoveDt);
        // Snapshot stagger before advancing timers so the final active frame
        // blocks both movement and contact damage.
        Enemy.bStaggeredThisFrame = Enemy.StaggerTimer > 0.f;
        if (Enemy.bStaggeredThisFrame)
        {
            Enemy.StaggerTimer = FMath::Max(0.f, Enemy.StaggerTimer - MoveDt);
            Enemy.AttackWindup = 0.f; // A real stagger interrupts an attack.
            continue;
        }
        // Guardians enter their final phase once below 35% of their starting HP.
        // Derive this each update from existing health: no extra save state.
        Enemy.bEnraged = Enemy.bGuardian &&
            Enemy.Health <= 0.35f * 1.8f * MoonfallWorldRules::EnemyMaxHealth(Enemy.Region);
        const FVector From = Enemy.Mesh->GetRelativeLocation();
        FVector ToHero = HeroPosition - From;
        ToHero.Z = 0.f;
        FVector FromHome = From - Enemy.Home;
        FromHome.Z = 0.f;
        const float Leash = Enemy.bGuardian ? 600.f : 900.f;
        const float HeroDistSq = ToHero.SizeSquared();
        // Keep combat contact while close. Previously crossing 160 cm
        // incorrectly switched to return-home, making melee enemies retreat.
        // Once leash is reached, return far enough inward before reacquiring.
        // This avoids a one-frame chase/retreat loop along the leash boundary.
        if (FromHome.SizeSquared() >= FMath::Square(Leash))
            Enemy.bReturningHome = true;
        if (Enemy.bReturningHome &&
            FromHome.SizeSquared() <= FMath::Square(Leash - 140.f))
            Enemy.bReturningHome = false;
        const bool bAggro = HeroDistSq < FMath::Square(800.f) &&
            !Enemy.bReturningHome && FromHome.SizeSquared() < FMath::Square(Leash);
        // Contact strikes require a readable windup. Moving away, breaking
        // leash, or stunning the attacker cancels the pending strike.
        // Full frame hitches advance no more than 50ms of enemy combat time.
        if (bAggro && HeroDistSq <= FMath::Square(240.f))
        {
            if (Enemy.AttackWindup > 0.f)
            {
                Enemy.AttackWindup = FMath::Max(0.f, Enemy.AttackWindup - MoveDt);
                if (Enemy.AttackWindup <= 0.f)
                {
                    Enemy.bAttackReadyThisFrame = true;
                    Enemy.AttackCooldown = Enemy.bGuardian ? 2.f : 1.5f;
                }
            }
            else if (Enemy.AttackCooldown <= 0.f)
            {
                const int32 Tactic = Enemy.bGuardian ? 0 : Enemy.Id % 3;
                Enemy.AttackWindup = Enemy.bGuardian ? (Enemy.bEnraged ? 0.6f : 0.85f) :
                    (Tactic == 2 ? 0.75f : (Tactic == 1 ? 0.55f : 0.65f));
            }
        }
        else
        {
            // Expire partial windups when a target escapes. Prevent the next
            // re-engagement from instantly firing an old half-finished strike.
            if (Enemy.AttackWindup > 0.f)
                Enemy.AttackCooldown = FMath::Max(Enemy.AttackCooldown, 0.25f);
            Enemy.AttackWindup = 0.f;
        }
        if (bAggro)
        {
            // Three lightweight, deterministic normal-enemy tactics without
            // spawning additional actors or allocating per-frame path data:
            // 0: direct hunter; 1: circling skirmisher; 2: charging bruiser.
            // Guardians retain their more aggressive direct pursuit.
            const int32 Tactic = Enemy.bGuardian ? 0 : Enemy.Id % 3;
            const float HeroDistance = FMath::Sqrt(HeroDistSq);
            FVector MoveDirection = ToHero.GetSafeNormal();
            float Speed = Enemy.bEnraged ? 360.f : (Enemy.bGuardian ? 280.f : 200.f);
            if (Tactic == 1)
            {
                // Circle at medium range and backpedal only at close range.
                // Keep a radial component so skirmishers eventually engage.
                const FVector Tangent(-MoveDirection.Y, MoveDirection.X, 0.f);
                if (HeroDistance < 240.f)
                    MoveDirection = (-MoveDirection + Tangent * 0.35f).GetSafeNormal();
                else if (HeroDistance < 520.f)
                    MoveDirection = (MoveDirection * 0.35f + Tangent).GetSafeNormal();
                Speed = 225.f;
            }
            else if (Tactic == 2 && HeroDistance < 620.f)
            {
                // Bruisers close distance quickly but still obey the leash.
                Speed = 310.f;
            }
            const bool bNeedsMove = HeroDistance > 160.f ||
                (Tactic == 1 && HeroDistance < 240.f);
            if (bNeedsMove)
            {
                const float Step = Speed * MoveDt;
                const FVector Candidate = From + MoveDirection * Step;
                const FVector ToCandidateHome = Candidate - Enemy.Home;
                // Clamp all tactics inside the same chamber/home boundary.
                const FVector Clamped = Enemy.Home +
                    ToCandidateHome.GetClampedToMaxSize2D(Leash);
                Enemy.Mesh->SetRelativeLocation(Clamped);
            }
        }
        else if (FromHome.SizeSquared() > FMath::Square(30.f))
        {
            const float ReturnStep = FMath::Min(180.f * MoveDt, FromHome.Size());
            Enemy.Mesh->SetRelativeLocation(From - FromHome.GetSafeNormal() * ReturnStep);
        }
    }
}

float AMoonfallPawn::GetNearestEnemyWindupSeconds() const
{
    if (!HeroMesh || Health <= 0.f || bWorldMapOpen) return 0.f;
    const FVector HeroPosition = HeroMesh->GetRelativeLocation();
    float NearestThreat = TNumericLimits<float>::Max();
    for (const FWorldEnemy& Enemy : Enemies)
    {
        if (!Enemy.Mesh || Enemy.bReturningHome || Enemy.bStaggeredThisFrame ||
            Enemy.AttackWindup <= 0.f) continue;
        if (FVector::DistSquared2D(HeroPosition, Enemy.Mesh->GetRelativeLocation()) >
            FMath::Square(320.f)) continue;
        NearestThreat = FMath::Min(NearestThreat, Enemy.AttackWindup);
    }
    return NearestThreat == TNumericLimits<float>::Max() ? 0.f : NearestThreat;
}

int32 AMoonfallPawn::NearestEnemyIndex(float Radius) const
{
    if (!HeroMesh || Radius <= 0.f) return INDEX_NONE;
    int32 Best = INDEX_NONE;
    float BestDistanceSq = FMath::Square(Radius);
    const FVector HeroPosition = HeroMesh->GetRelativeLocation();
    for (int32 I = 0; I < Enemies.Num(); ++I)
    {
        const FWorldEnemy& Enemy = Enemies[I];
        if (!Enemy.Mesh) continue; // An unspawned or destroyed mesh is not targetable.
        const FVector Delta = Enemy.Mesh->GetRelativeLocation() - HeroPosition;
        const float DistanceSq = Delta.SizeSquared2D();
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            Best = I;
        }
    }
    return Best;
}

TArray<FVector> AMoonfallPawn::GetEnemyMapPositions() const
{
    TArray<FVector> Result;
    Result.Reserve(Enemies.Num());
    for (const FWorldEnemy& Enemy : Enemies)
        if (Enemy.Mesh) Result.Add(Enemy.Mesh->GetRelativeLocation());
    return Result;
}

void AMoonfallPawn::ShowDamageNumber(float Damage, float RemainingHealth)
{
    // One bounded HUD popup instead of allocating text widgets per hit.
    LastDamageAmount = FMath::Clamp(Damage, 0.f, FMath::Max(0.f, RemainingHealth));
    if (LastDamageAmount > 0.f) DamageNumberTimer = 0.85f;
}

void AMoonfallPawn::DamageWorldEnemy(int32 Index, float Damage, bool bRiposte, bool bComboFinisher)
{
    if (!Enemies.IsValidIndex(Index)) return;
    FWorldEnemy& Enemy = Enemies[Index];
    ShowDamageNumber(Damage, Enemy.Health);
    Enemy.Health = FMath::Max(0.f, Enemy.Health - Damage);
    if (Enemy.Health > 0.f)
    {
        // The reward requires a real hit on a living guardian; defeated foes
        // need no stun state and ordinary hits cannot lock down bosses.
        if ((bRiposte && Enemy.bGuardian) || (bComboFinisher && !Enemy.bGuardian))
        {
            // Third-hit finishers briefly stagger normal enemies; earned
            // ripostes retain the longer guardian-specific stagger.
            Enemy.StaggerTimer = Enemy.bGuardian ? 0.8f :
                FMath::Min(0.8f, 0.4f + 0.1f * ForgedRegionTier(2));
            // Graveglass ranks lengthen only landed normal-enemy finishers.
            // Guardian riposte timing stays intact.
            Enemy.AttackWindup = 0.f; // Riposte/finisher interrupts prepared strikes.
            Enemy.AttackCooldown = FMath::Max(Enemy.AttackCooldown, 0.5f);
            // Give a landed riposte physical space without pushing guardians
            // outside their chamber leash or introducing a physics actor.
            if (Enemy.Mesh && HeroMesh)
            {
                const FVector From = Enemy.Mesh->GetRelativeLocation();
                FVector Away = From - HeroMesh->GetRelativeLocation();
                Away.Z = 0.f;
                if (!Away.IsNearlyZero())
                {
                    const FVector Candidate = From + Away.GetSafeNormal() *
                        (Enemy.bGuardian ? 120.f : 90.f);
                    const FVector LeashDelta = Candidate - Enemy.Home;
                    const FVector Bounded = Enemy.Home +
                        LeashDelta.GetClampedToMaxSize2D(Enemy.bGuardian ? 600.f : 900.f);
                    Enemy.Mesh->SetRelativeLocation(Bounded);
                }
            }
        }
        return;
    }
    const int32 Region = Enemy.Region;
    if (Enemy.bGuardian) DefeatedGuardianMask |= 1 << Enemy.Id;
    else DefeatedEnemyMask |= 1 << Enemy.Id;
    const bool bGuardian = Enemy.bGuardian;
    if (Enemy.Portrait) Enemy.Portrait->DestroyComponent();
    Enemy.Mesh->DestroyComponent();
    Enemies.RemoveAt(Index);
    // Combat victories grant XP, not an automatic health refill.
    Progression.AwardDefeat(Region, bGuardian, false, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
    SaveProgress();
}

TArray<FVector> AMoonfallPawn::GetChamberMapPositions() const
{
    TArray<FVector> Positions;
    Positions.Reserve(ChamberCaches.Num());
    for (const FChamberCache& Cache : ChamberCaches)
        if (Cache.Mesh) Positions.Add(Cache.Mesh->GetRelativeLocation());
    return Positions;
}

void AMoonfallPawn::CollectNearbyChambers()
{
    const FVector HeroPosition = HeroMesh->GetRelativeLocation();
    bool bClaimedAny = false;
    for (int32 I = ChamberCaches.Num() - 1; I >= 0; --I)
    {
        const FChamberCache& Cache = ChamberCaches[I];
        if (!Cache.Mesh || FVector::Dist2D(HeroPosition, Cache.Mesh->GetRelativeLocation()) > 250.f) continue;
        const int32 Id = Cache.Id;
        if ((DefeatedGuardianMask & (1 << Id)) == 0) continue; // No loot before guardian defeat.
        ClaimedChamberMask |= 1 << Id;
        bClaimedAny = true;
        if (Cache.Artwork) Cache.Artwork->DestroyComponent();
        Cache.Mesh->DestroyComponent();
        ChamberCaches.RemoveAt(I);
        const int32 Region = Id / MoonfallWorldRules::ChambersPerRegion;
        Progression.AwardDefeat(
            Region, false, false, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
        Mana = FMath::Min(180.f, Mana + 25.f);
        // Each cleared chamber grants one named, unique equipment material.
        // Use the existing persistent inventory rather than another save slot.
        if (BossInventory.Num() < 100)
        {
            static const TCHAR* Materials[6] = {
                TEXT("Embersteel"), TEXT("Moonweave"), TEXT("Graveglass"),
                TEXT("Stormhide"), TEXT("Stariron"), TEXT("Voidcrystal")
            };
            const FString Reward = FString::Printf(
                TEXT("%s chamber cache #%d"),
                Materials[FMath::Clamp(Region, 0, 5)], Id + 1);
            BossInventory.Add(Reward);
        }
    }
    // Persist once for all nearby claims, not one disk operation per chest.
    if (bClaimedAny) SaveProgress();
}

bool AMoonfallPawn::IsMapPositionDiscovered(const FVector& Position) const
{
    // The clamped movement bounds include their upper edges, while 48x16 cell
    // indices end at 47/15. Match the reveal grid at those exact boundaries
    // without ever treating an out-of-world location as discovered.
    if (!FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y) ||
        Position.X < -1600.f || Position.X > 17600.f ||
        Position.Y < -3200.f || Position.Y > 3200.f)
        return false;
    const int32 X = FMath::Clamp(FMath::FloorToInt((Position.X + 1600.f) / 400.f), 0, 47);
    const int32 Y = FMath::Clamp(FMath::FloorToInt((Position.Y + 3200.f) / 400.f), 0, 15);
    return DiscoveredMapCells.Num() == 48 * 16 &&
        DiscoveredMapCells[Y * 48 + X] != 0;
}

bool AMoonfallPawn::RevealNearbyMapCells(const FVector& Position)
{
    if (DiscoveredMapCells.Num() != 48 * 16)
        DiscoveredMapCells.Init(0, 48 * 16);
    const int32 X = FMath::Clamp(FMath::FloorToInt((Position.X + 1600.f) / 400.f), 0, 47);
    const int32 Y = FMath::Clamp(FMath::FloorToInt((Position.Y + 3200.f) / 400.f), 0, 15);
    bool bChanged = false;
    for (int32 DY = -1; DY <= 1; ++DY)
        for (int32 DX = -1; DX <= 1; ++DX)
        {
            const int32 CX = X + DX, CY = Y + DY;
            if (CX < 0 || CX >= 48 || CY < 0 || CY >= 16) continue;
            uint8& Cell = DiscoveredMapCells[CY * 48 + CX];
            if (Cell == 0) { Cell = 1; bChanged = true; }
        }
    return bChanged;
}

int32 AMoonfallPawn::GetDiscoveredRegionCount() const
{
    int32 Count = 0;
    for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
        if ((DiscoveredRegionMask & (1 << Region)) != 0) ++Count;
    return Count;
}

int32 AMoonfallPawn::GetQuestDefeatedBeasts() const
{
    // Read the same persistent flags used to stop defeated enemies respawning.
    int32 Count = 0;
    for (int32 Id = 0; Id < MoonfallWorldRules::TotalEnemies; ++Id)
        if ((DefeatedEnemyMask & (1 << Id)) != 0) ++Count;
    return Count;
}

int32 AMoonfallPawn::GetQuestClearedChambers() const
{
    // A chamber counts only when its defeated guardian's reward is claimed.
    int32 Count = 0;
    for (int32 Id = 0; Id < MoonfallWorldRules::TotalChambers; ++Id)
        if ((ClaimedChamberMask & (1 << Id)) != 0 &&
            (DefeatedGuardianMask & (1 << Id)) != 0) ++Count;
    return Count;
}

bool AMoonfallPawn::IsSafeCheckpointLocation() const
{
    // Respawn grace and recent combat do not establish a new safe location.
    // Progress is still saved, but the previous living checkpoint is retained.
    if (!HeroMesh || Health <= 0.f || bRespawnInProgress || bCasting ||
        RespawnGraceTimer > 0.f || EnemyDamageCooldown > 0.f)
        return false;
    const FVector Position = HeroMesh->GetRelativeLocation();
    if (!bWon && BossMesh &&
        FVector::DistSquared2D(Position, BossMesh->GetRelativeLocation()) <
            FMath::Square(1500.f))
        return false;
    for (const FWorldEnemy& Enemy : Enemies)
    {
        if (Enemy.Mesh && FVector::DistSquared2D(Position,
                Enemy.Mesh->GetRelativeLocation()) < FMath::Square(750.f))
            return false;
    }
    return true;
}

void AMoonfallPawn::SaveProgress()
{
    // Reuse the prior save so a death-time write cannot erase the last living checkpoint.
    UMoonfallProgressSave* Progress = Cast<UMoonfallProgressSave>(
        UGameplayStatics::LoadGameFromSlot(TEXT("MoonfallPrototypeBossRewards"), 0));
    if (!Progress)
        Progress = Cast<UMoonfallProgressSave>(
            UGameplayStatics::CreateSaveGameObject(UMoonfallProgressSave::StaticClass()));
    if (!Progress) { UE_LOG(LogTemp, Warning, TEXT("Moonfall: could not create SaveGame.")); return; }
    Progress->PermanentBossVictories = Victories;
    Progress->BossRelics = BossInventory;
    Progress->HeroLevel = Progression.Level;
    Progress->HeroXP = Progression.Experience;
    Progress->HeroPoints = Progression.HeroPoints;
    Progress->TamerPoints = Progression.TamerPoints;
    Progress->DefeatedEnemyMask = DefeatedEnemyMask;
    Progress->DiscoveredRegionMask = DiscoveredRegionMask;
    Progress->DiscoveredMapCells = DiscoveredMapCells;
    Progress->bHasWaypoint = bHasWaypoint;
    Progress->SavedWaypoint = bHasWaypoint ? Waypoint : FVector::ZeroVector;
    Progress->ClaimedChamberMask = ClaimedChamberMask;
    Progress->DefeatedGuardianMask = DefeatedGuardianMask;
    Progress->bBossDefeated = bWon;
    // Preserve legacy SelectedHeroIndex from the loaded save; never reassign it.
    bool bWroteCheckpoint = false;
    // Always persist quest, loot and enemy victories immediately, but do not
    // overwrite a living checkpoint while surrounded by enemies or near a boss.
    // Bootstrap a first checkpoint even if the starting area is contested.
    if (HeroMesh && Health > 0.f &&
        (!Progress->bHasCheckpoint || bCheckpointNeedsRepair || IsSafeCheckpointLocation()))
    {
        Progress->bHasCheckpoint = true;
        Progress->SavedHeroPosition = HeroMesh->GetRelativeLocation();
        Progress->SavedHealth = Health;
        Progress->bHasResourceCheckpoint = true;
        Progress->SavedMana = Mana;
        Progress->SavedStamina = Stamina;
        bWroteCheckpoint = true;
    }
    if (!UGameplayStatics::SaveGameToSlot(Progress, TEXT("MoonfallPrototypeBossRewards"), 0))
        UE_LOG(LogTemp, Warning, TEXT("Moonfall prototype: reward save failed."));
    else if (bWroteCheckpoint)
        bCheckpointNeedsRepair = false;
}

bool AMoonfallPawn::GetWaypoint(FVector& OutWaypoint) const
{
    if (!bHasWaypoint) return false;
    OutWaypoint = Waypoint;
    return true;
}

bool AMoonfallPawn::GetQuestDestination(FVector& OutDestination) const
{
    // Choose only an already discovered incomplete destination. Never disclose fogged locations.
    const FVector HeroPosition = GetHeroMapPosition();
    float BestDistanceSq = TNumericLimits<float>::Max();
    bool bFound = false;
    for (const FVector& Candidate : GetChamberMapPositions())
    {
        if (!IsMapPositionDiscovered(Candidate)) continue;
        const float DistanceSq = FVector::DistSquared2D(HeroPosition, Candidate);
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            OutDestination = Candidate;
            bFound = true;
        }
    }
    if (!bFound && !bWon && IsMapPositionDiscovered(GetBossMapPosition()))
    {
        OutDestination = GetBossMapPosition();
        bFound = true;
    }
    return bFound;
}

void AMoonfallPawn::ToggleWorldMap()
{
    // The Respawn screen owns mouse focus after defeat.
    if (Health <= 0.f || bRespawnInProgress) return;
    bWorldMapOpen = !bWorldMapOpen;
    if (bWorldMapOpen && HeroMesh)
    {
        // Start controller navigation over the hero's revealed map cell.
        // Unlike zoom-based map pan, this selection works at 1x zoom.
        const FVector HeroPosition = HeroMesh->GetRelativeLocation();
        WorldMapCursorCells = FVector2D(
            FMath::Clamp(FMath::FloorToFloat((HeroPosition.X + 1600.f) / 400.f) + .5f, .5f, 47.5f),
            FMath::Clamp(FMath::FloorToFloat((HeroPosition.Y + 3200.f) / 400.f) + .5f, .5f, 15.5f));
        WorldMapPanCells.X = FMath::Clamp(WorldMapCursorCells.X - 24.f,
            -24.f * (1.f - 1.f / WorldMapZoom), 24.f * (1.f - 1.f / WorldMapZoom));
        WorldMapPanCells.Y = FMath::Clamp(WorldMapCursorCells.Y - 8.f,
            -8.f * (1.f - 1.f / WorldMapZoom), 8.f * (1.f - 1.f / WorldMapZoom));
    }
    DodgeBufferTimer = 0.f; // Map UI cannot queue a combat dodge.
    Forward = 0.f;
    Right = 0.f;
    MapPanHorizontalInput = 0.f;
    MapPanVerticalInput = 0.f;
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (bWorldMapOpen)
        {
            // GameAndUI keeps mapped game actions active while allowing a real
            // mouse cursor to reach discovered atlas cells without viewport capture.
            FInputModeGameAndUI MapInput;
            MapInput.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            MapInput.SetHideCursorDuringCapture(false);
            PC->SetInputMode(MapInput);
            PC->bShowMouseCursor = true;
        }
        else
        {
            // Restore click-to-attack and movement after closing the atlas.
            PC->SetInputMode(FInputModeGameOnly());
            PC->bShowMouseCursor = false;
        }
    }
}

void AMoonfallPawn::CloseWorldMap()
{
    // Escape closes the atlas without consuming a combat action or affecting
    // Escape behavior elsewhere once an in-game settings menu is added.
    if (bWorldMapOpen) ToggleWorldMap();
}

void AMoonfallPawn::GetWorldMapLayout(float Width, float Height, float& OutCell,
    FVector2D& OutOrigin, FVector2D& OutClipMin, FVector2D& OutClipMax) const
{
    const float BaseCell = FMath::Max(1.f, FMath::Min(12.f,
        FMath::Min((Width - 100.f) / 48.f, (Height - 200.f) / 16.f)));
    OutCell = BaseCell * WorldMapZoom;
    const FVector2D Center(Width * 0.5f, Height * 0.5f);
    OutOrigin = Center - FVector2D(24.f + WorldMapPanCells.X,
        8.f + WorldMapPanCells.Y) * OutCell;
    OutClipMin = Center - FVector2D(24.f, 8.f) * BaseCell;
    OutClipMax = Center + FVector2D(24.f, 8.f) * BaseCell;
}

void AMoonfallPawn::PanMapHorizontal(float Value) { MapPanHorizontalInput = Value; }
void AMoonfallPawn::PanMapVertical(float Value) { MapPanVerticalInput = Value; }

void AMoonfallPawn::PlaceMapWaypoint()
{
    if (!bWorldMapOpen || Health <= 0.f) return;
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;
    float MouseX = 0.f, MouseY = 0.f;
    if (!PC->GetMousePosition(MouseX, MouseY)) return;
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width < 200 || Height < 200) return;
    float CellSize = 0.f;
    FVector2D Origin, ClipMin, ClipMax;
    GetWorldMapLayout(static_cast<float>(Width), static_cast<float>(Height),
        CellSize, Origin, ClipMin, ClipMax);
    if (MouseX < ClipMin.X || MouseX >= ClipMax.X ||
        MouseY < ClipMin.Y || MouseY >= ClipMax.Y) return;
    const int32 X = FMath::FloorToInt((MouseX - Origin.X) / CellSize);
    const int32 Y = FMath::FloorToInt((MouseY - Origin.Y) / CellSize);
    if (X < 0 || X >= 48 || Y < 0 || Y >= 16) return;
    const FVector Candidate(-1600.f + (X + .5f) * 400.f,
        -3200.f + (Y + .5f) * 400.f, 0.f);
    if (!IsMapPositionDiscovered(Candidate)) return;
    Waypoint = Candidate;
    bHasWaypoint = true;
    SaveProgress();
}

void AMoonfallPawn::SetMapCenterWaypoint()
{
    // Controller focus is an independent movable tile, not the mouse.
    // At 1x zoom the whole atlas is visible, so gamepad A can still pick
    // any explored tile instead of always the fixed center cell.
    if (!bWorldMapOpen || Health <= 0.f) return;
    const int32 X = FMath::FloorToInt(WorldMapCursorCells.X);
    const int32 Y = FMath::FloorToInt(WorldMapCursorCells.Y);
    if (X < 0 || X >= 48 || Y < 0 || Y >= 16) return;
    const FVector Candidate(-1600.f + (X + .5f) * 400.f,
        -3200.f + (Y + .5f) * 400.f, 0.f);
    if (!IsMapPositionDiscovered(Candidate)) return;
    Waypoint = Candidate;
    bHasWaypoint = true;
    SaveProgress();
}

void AMoonfallPawn::ControllerAttack()
{
    // RT is combat-only. Mouse click has a separate map cursor action;
    // controller A owns map waypoints and death-screen respawn.
    if (Health <= 0.f || bWorldMapOpen || bRespawnInProgress) return;
    Attack();
}

void AMoonfallPawn::ControllerConfirm()
{
    if (Health <= 0.f) { RespawnFromCheckpoint(); return; }
    if (bWorldMapOpen) { SetMapCenterWaypoint(); return; }
    Attack();
}

void AMoonfallPawn::ControllerBack()
{
    if (Health <= 0.f) return;
    if (bWorldMapOpen) { CloseWorldMap(); return; }
    Dodge();
}

void AMoonfallPawn::ControllerZoomIn()
{
    if (Health > 0.f) Zoom(1.f);
}

void AMoonfallPawn::ControllerZoomOut()
{
    if (Health > 0.f) Zoom(-1.f);
}

void AMoonfallPawn::ControllerSetWaypoint()
{
    if (Health <= 0.f || bRespawnInProgress) return;
    if (bWorldMapOpen) SetMapCenterWaypoint();
    else SetWaypoint();
}

void AMoonfallPawn::SetWaypoint()
{
    if (bWorldMapOpen) return; // V never replaces a clicked map waypoint.
    if (Health <= 0.f || !HeroMesh) return;
    Waypoint = HeroMesh->GetRelativeLocation();
    Waypoint.Z = 0.f;
    bHasWaypoint = true;
    SaveProgress();
}

void AMoonfallPawn::ClearWaypoint()
{
    // Defeat and level travel must not mutate persistent navigation data.
    if (Health <= 0.f || bRespawnInProgress || !bHasWaypoint) return;
    bHasWaypoint = false;
    Waypoint = FVector::ZeroVector;
    SaveProgress();
}

void AMoonfallPawn::MoveForward(float Value) { Forward = Value; }
void AMoonfallPawn::MoveRight(float Value) { Right = Value; }
void AMoonfallPawn::Zoom(float Value)
{
    // Prevent mouse-wheel camera changes behind the death/respawn overlay.
    if (Health <= 0.f || bRespawnInProgress) return;
    if (bWorldMapOpen)
    {
        if (FMath::IsNearlyZero(Value)) return;
        WorldMapZoom = FMath::Clamp(WorldMapZoom + Value * 0.2f, 1.f, 3.f);
        const float MaxPanX = 24.f * (1.f - 1.f / WorldMapZoom);
        const float MaxPanY = 8.f * (1.f - 1.f / WorldMapZoom);
        WorldMapPanCells.X = FMath::Clamp(WorldMapPanCells.X, -MaxPanX, MaxPanX);
        WorldMapPanCells.Y = FMath::Clamp(WorldMapPanCells.Y, -MaxPanY, MaxPanY);
        return;
    }
    ZoomDistance = FMath::Clamp(ZoomDistance - Value * 125.f, 900.f, 2500.f);
    CameraBoom->TargetArmLength = ZoomDistance;
}
void AMoonfallPawn::GetRespawnButtonBounds(float Width, float Height,
    FVector2D& OutMin, FVector2D& OutMax) const
{
    // Large, responsive single-click target. Use exactly this geometry for
    // HUD rendering, hover feedback and mouse hit testing.
    const float ButtonWidth = FMath::Max(160.f, FMath::Min(360.f, Width - 32.f));
    const float ButtonHeight = FMath::Max(56.f, FMath::Min(84.f, Height * 0.16f));
    const FVector2D Center(Width * 0.5f, Height * 0.5f);
    OutMin = Center - FVector2D(ButtonWidth * 0.5f, ButtonHeight * 0.5f);
    OutMax = Center + FVector2D(ButtonWidth * 0.5f, ButtonHeight * 0.5f);
}

void AMoonfallPawn::ClickRespawnButton()
{
    if (Health > 0.f || bWorldMapOpen) return;
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width < 240 || Height < 160) return;
    float MouseX = 0.f, MouseY = 0.f;
    if (!PC->GetMousePosition(MouseX, MouseY)) return;
    FVector2D Min, Max;
    GetRespawnButtonBounds(static_cast<float>(Width), static_cast<float>(Height), Min, Max);
    if (MouseX >= Min.X && MouseX <= Max.X && MouseY >= Min.Y && MouseY <= Max.Y)
        RespawnFromCheckpoint();
}

void AMoonfallPawn::RespawnFromCheckpoint()
{
    // Restart only after death; never overwrite the last living checkpoint
    // with a defeated pawn before the saved world is reconstructed.
    if (Health > 0.f || bWorldMapOpen || bRespawnInProgress || !GetWorld()) return;
    const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
    if (LevelName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Moonfall: cannot respawn without a valid current level."));
        return;
    }
    bRespawnInProgress = true; // Ignore double-clicks during level travel.
    UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

void AMoonfallPawn::Parry()
{
    // A deliberate, short right-click timing window; never a held guard.
    if (bWorldMapOpen || Health <= 0.f || DodgeTimer > 0.f ||
        ParryCooldown > 0.f || AttackCooldown > 0.f || Stamina < 25.f) return;
    Stamina -= 25.f;
    StaminaRegenDelay = 0.6f;
    ParryTimer = 0.22f;
    ParryCooldown = 1.25f;
}

void AMoonfallPawn::Dodge()
{
    if (bWorldMapOpen) return; // Map navigation must not spend stamina.
    if (ParryTimer > 0.f) return; // Cannot dodge during a committed parry.
    if (Health <= 0.f || bRespawnInProgress || Stamina < 45.f ||
        FVector2D(Forward, Right).SizeSquared() <= 0.01f) return;
    if (DodgeCooldown <= 0.f)
    {
        DodgeBufferTimer = 0.f; // A new dodge supersedes any queued input.
        // Commit to the initial direction for the complete dodge. Releasing
        // a stick or movement key must not waste stamina or shorten travel.
        // A partially tilted analog stick still triggers a complete dodge:
        // magnitude controls walking speed, not stamina-paid dodge distance.
        DodgeDirection = FVector(Forward, Right, 0.f).GetSafeNormal();
        DodgeTimer = 0.32f;
        DodgeCooldown = 1.2f;
        Stamina -= 45.f;
        StaminaRegenDelay = 0.6f;
    }
    else if (DodgeCooldown <= 0.18f)
    {
        // A slightly early button press queues a single dodge. Nothing is
        // spent until it actually begins, and its direction stays committed.
        BufferedDodgeDirection = FVector(Forward, Right, 0.f).GetSafeNormal();
        DodgeBufferTimer = 0.2f;
    }
}
int32 AMoonfallPawn::GuardianBlessingCount() const
{
    // The existing 30-bit saved guardian mask is the only authority.
    // Each unique guardian victory grants one permanent stack, no farming.
    int32 Count = 0;
    for (int32 Id = 0; Id < MoonfallWorldRules::TotalChambers; ++Id)
        if ((DefeatedGuardianMask & (1 << Id)) != 0) ++Count;
    return Count;
}

int32 AMoonfallPawn::ChamberMasteryCount() const
{
    // Claimed guardian caches are permanent. Derive mastery from the existing
    // save mask so there is no second progress counter to get out of sync.
    int32 Count = 0;
    for (int32 Id = 0; Id < MoonfallWorldRules::TotalChambers; ++Id)
        if ((ClaimedChamberMask & (1 << Id)) != 0 &&
            (DefeatedGuardianMask & (1 << Id)) != 0) ++Count;
    return Count;
}

// Inventory-affix format: "Forged <name> #R|DMG=D|WARD=W".
// Bare "#R" items from earlier releases remain valid with zero RNG bonuses.
// Validated, bounded parsing rejects tampered names and absurd stats on load.
static bool ParseForgedEquipment(const FString& Item, const FString& Prefix,
    int32& OutRank, int32& OutDamage, int32& OutWard)
{
    if (!Item.StartsWith(Prefix)) return false;
    const FString Suffix = Item.RightChop(Prefix.Len());
    FString RankText, AffixText;
    const bool bHasRoll = Suffix.Split(TEXT("|DMG="), &RankText, &AffixText);
    if (!bHasRoll) RankText = Suffix;
    if (RankText.Len() != 1 || !RankText.IsNumeric()) return false;
    const int32 Rank = FCString::Atoi(*RankText);
    if (Rank < 1 || Rank > 4) return false;
    int32 Damage = 0, Ward = 0;
    if (bHasRoll)
    {
        FString DamageText, WardText;
        if (!AffixText.Split(TEXT("|WARD="), &DamageText, &WardText) ||
            DamageText.IsEmpty() || WardText.IsEmpty() ||
            DamageText.Len() > 2 || WardText.Len() > 2 ||
            !DamageText.IsNumeric() || !WardText.IsNumeric()) return false;
        Damage = FCString::Atoi(*DamageText);
        Ward = FCString::Atoi(*WardText);
        if (Damage < 1 || Damage > Rank * 2 || Ward < 0 || Ward > Rank) return false;
    }
    OutRank = Rank;
    OutDamage = Damage;
    OutWard = Ward;
    return true;
}

static const TCHAR* ForgedGearNames[6] = {
    TEXT("Forged Embersteel Blade"), TEXT("Forged Moonweave Focus"),
    TEXT("Forged Graveglass Blade"), TEXT("Forged Stormhide Focus"),
    TEXT("Forged Stariron Blade"), TEXT("Forged Voidcrystal Focus")
};

int32 AMoonfallPawn::ForgedRegionTier(int32 Region) const
{
    if (Region < 0 || Region >= MoonfallWorldRules::RegionCount) return 0;
    const FString Prefix = FString(ForgedGearNames[Region]) + TEXT(" #");
    int32 BestTier = 0;
    for (const FString& Item : BossInventory)
    {
        int32 Tier = 0, Damage = 0, Ward = 0;
        if (ParseForgedEquipment(Item, Prefix, Tier, Damage, Ward))
            BestTier = FMath::Max(BestTier, Tier);
    }
    return BestTier;
}

void AMoonfallPawn::GetForgedRollStats(int32 Region, int32& OutDamage, int32& OutWard) const
{
    OutDamage = 0;
    OutWard = 0;
    if (Region < 0 || Region >= MoonfallWorldRules::RegionCount) return;
    const FString Prefix = FString(ForgedGearNames[Region]) + TEXT(" #");
    int32 HighestTier = 0;
    int32 HighestScore = -1;
    for (const FString& Item : BossInventory)
    {
        int32 Tier = 0, Damage = 0, Ward = 0;
        if (!ParseForgedEquipment(Item, Prefix, Tier, Damage, Ward)) continue;
        // At most one valid item counts per region; duplicate strings cannot
        // stack stats. For legacy duplicates choose highest rank then quality.
        const int32 Score = Damage + Ward;
        if (Tier > HighestTier || (Tier == HighestTier && Score > HighestScore))
        {
            HighestTier = Tier;
            HighestScore = Score;
            OutDamage = Damage;
            OutWard = Ward;
        }
    }
}

int32 AMoonfallPawn::ForgedRandomDamage(bool bArcane) const
{
    int32 Sum = 0;
    for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
    {
        if ((Region % 2 == 1) != bArcane) continue;
        int32 Damage = 0, Ward = 0;
        GetForgedRollStats(Region, Damage, Ward);
        Sum += Damage;
    }
    return FMath::Min(24, Sum); // 3 pieces, at most 8 damage each.
}

int32 AMoonfallPawn::ForgedRandomWard() const
{
    int32 Sum = 0;
    for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
    {
        int32 Damage = 0, Ward = 0;
        GetForgedRollStats(Region, Damage, Ward);
        Sum += Ward;
    }
    return FMath::Min(24, Sum); // 6 pieces, at most 4 ward each.
}

int32 AMoonfallPawn::ForgedEquipmentPower(bool bArcane) const
{
    // Fixed mastery remains, independently of randomized affixes.
    int32 Power = 0;
    for (int32 Region = 0; Region < MoonfallWorldRules::RegionCount; ++Region)
        if ((Region % 2 == 1) == bArcane)
            Power += ForgedRegionTier(Region);
    return FMath::Min(12, Power);
}

void AMoonfallPawn::ForgeEquipment()
{
    ForgeEquipmentForType(-1); // Existing C / right shoulder behavior.
}

void AMoonfallPawn::ForgeMeleeEquipment()
{
    ForgeEquipmentForType(0);
}

void AMoonfallPawn::ForgeArcaneEquipment()
{
    ForgeEquipmentForType(1);
}

void AMoonfallPawn::ForgeEquipmentForType(int32 DesiredType)
{
    // Forge rank I with two distinct guardian caches, then ranks II, III
    // and IV each consume one more cache. A region's five guardian caches
    // therefore fully fund one Masterwork rank IV piece, with no leftovers.
    // Never forge during combat, while defeated, or behind an open map.
    if (bWorldMapOpen || Health <= 0.f || bRespawnInProgress ||
        !IsSafeCheckpointLocation()) return;
    static const TCHAR* Materials[6] = {
        TEXT("Embersteel"), TEXT("Moonweave"), TEXT("Graveglass"),
        TEXT("Stormhide"), TEXT("Stariron"), TEXT("Voidcrystal")
    };
    static const TCHAR* Gear[6] = {
        TEXT("Forged Embersteel Blade"), TEXT("Forged Moonweave Focus"),
        TEXT("Forged Graveglass Blade"), TEXT("Forged Stormhide Focus"),
        TEXT("Forged Stariron Blade"), TEXT("Forged Voidcrystal Focus")
    };
    // Forge the region the player is physically exploring first. This lets
    // players choose a recipe naturally by visiting its region, without
    // introducing inventory screens, new input buttons, or a cluttered HUD.
    const int32 LocalRegion = HeroMesh ?
        MoonfallWorldRules::RegionAt(HeroMesh->GetRelativeLocation().X) : 0;
    for (int32 Offset = 0; Offset < MoonfallWorldRules::RegionCount; ++Offset)
    {
        const int32 Region = (LocalRegion + Offset) % MoonfallWorldRules::RegionCount;
        // Optional focused controls never silently forge the wrong gear type.
        // -1 remains the original auto-forge, prioritizing the local region.
        if (DesiredType >= 0 && (Region % 2) != DesiredType) continue;
        const int32 CurrentTier = ForgedRegionTier(Region);
        if (CurrentTier >= 4) continue; // Masterwork cap: no further material cost.
        const int32 MaterialCost = CurrentTier == 0 ? 2 : 1;
        TArray<int32, TInlineAllocator<2>> MaterialsToSpend;
        TArray<FString, TInlineAllocator<2>> ConsumedMaterialNames;
        // Only authentic, uniquely identified caches whose guardians were
        // defeated and claimed may enter a recipe. A duplicate inventory
        // string cannot be spent twice in the same forge action.
        for (int32 Chamber = 0; Chamber < MoonfallWorldRules::ChambersPerRegion; ++Chamber)
        {
            const int32 Id = Region * MoonfallWorldRules::ChambersPerRegion + Chamber;
            if ((ClaimedChamberMask & (1 << Id)) == 0 ||
                (DefeatedGuardianMask & (1 << Id)) == 0) continue;
            const FString Material = FString::Printf(
                TEXT("%s chamber cache #%d"), Materials[Region], Id + 1);
            for (int32 I = 0; I < BossInventory.Num(); ++I)
            {
                if (BossInventory[I] != Material) continue;
                MaterialsToSpend.Add(I);
                ConsumedMaterialNames.Add(Material);
                break; // One material for one real cache ID.
            }
            if (MaterialsToSpend.Num() == MaterialCost) break;
        }
        if (MaterialsToSpend.Num() != MaterialCost) continue;
        // Transactional inventory update. Remove all legacy copies from the
        // chosen region; replace with exactly one bounded upgraded item.
        const FString GearPrefix = FString(Gear[Region]) + TEXT(" #");
        int32 PreviousDamage = 0, PreviousWard = 0;
        GetForgedRollStats(Region, PreviousDamage, PreviousWard);
        const int32 NewTier = CurrentTier + 1;
        // One genuine random roll per successful forge. Upgrade rolls never
        // reduce existing affixes; no roll occurs without paid materials.
        const int32 RolledDamage = FMath::Max(PreviousDamage,
            FMath::RandRange(1, NewTier * 2));
        const int32 RolledWard = FMath::Max(PreviousWard,
            FMath::RandRange(0, NewTier));
        TArray<FString> UpdatedInventory;
        UpdatedInventory.Reserve(BossInventory.Num());
        for (int32 I = 0; I < BossInventory.Num(); ++I)
        {
            // Remove all copies of consumed cache IDs, including malformed
            // legacy duplicate entries, so they cannot fuel later recipes.
            if (MaterialsToSpend.Contains(I) ||
                ConsumedMaterialNames.Contains(BossInventory[I])) continue;
            int32 OldTier = 0, OldDamage = 0, OldWard = 0;
            if (ParseForgedEquipment(BossInventory[I], GearPrefix,
                OldTier, OldDamage, OldWard)) continue;
            UpdatedInventory.Add(BossInventory[I]);
        }
        // Two-or-one authentic materials removed, so adding one never expands storage.
        UpdatedInventory.Add(FString::Printf(TEXT("%s #%d|DMG=%d|WARD=%d"),
            Gear[Region], NewTier, RolledDamage, RolledWard));
        BossInventory = MoveTemp(UpdatedInventory);
        SaveProgress();
        return; // Exactly one deliberate forge operation per button press.
    }
}

float AMoonfallPawn::BasicAttackDamage() const
{
    return 18.f + Progression.Level + 0.5f * GuardianBlessingCount() +
        0.75f * ChamberMasteryCount() + 2.f * ForgedEquipmentPower(false) + ForgedRandomDamage(false);
}

float AMoonfallPawn::ArcaneBoltDamage() const
{
    return 28.f + 2.f * Progression.Level + 1.f * GuardianBlessingCount() +
        1.25f * ChamberMasteryCount() + 3.f * ForgedEquipmentPower(true) + ForgedRandomDamage(true);
}

float AMoonfallPawn::HealingSpellAmount() const
{
    // Stormhide focuses specialize in restorative magic; no automatic
    // healing from kills, only the existing manually cast E ability.
    return 30.f + Progression.Level + 4.f * ForgedRegionTier(3);
}

void AMoonfallPawn::Attack()
{
    if (Health <= 0.f) { ClickRespawnButton(); return; }
    if (bWorldMapOpen) { PlaceMapWaypoint(); return; }
    if (Health <= 0.f || ParryTimer > 0.f || AttackCooldown > 0.f) return;
    const int32 EnemyIndex = NearestEnemyIndex(400.f);
    const float BossDistance = bWon ? TNumericLimits<float>::Max() :
        FVector::Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation());
    const bool bBossInRange = BossDistance <= 400.f;
    // A miss has brief recovery, while a landed attack uses its full cadence.
    // Neither a miss nor an invalid click consumes a saved perfect-dodge riposte.
    AttackCooldown = (EnemyIndex != INDEX_NONE || bBossInRange) ? 0.35f : 0.12f;
    if (EnemyIndex == INDEX_NONE && !bBossInRange)
    {
        MeleeComboCount = 0;
        MeleeComboTimer = 0.f;
    }
    if (EnemyIndex != INDEX_NONE || bBossInRange)
    {
        // Resolve the nearest valid target, not always the first world enemy.
        const float EnemyDistance = EnemyIndex == INDEX_NONE ? TNumericLimits<float>::Max() :
            FVector::Dist2D(HeroMesh->GetRelativeLocation(),
                            Enemies[EnemyIndex].Mesh->GetRelativeLocation());
        const bool bHitBoss = bBossInRange && BossDistance < EnemyDistance;
        const bool bRiposte = CounterWindow > 0.f;
        // Every third landed click becomes a finisher. Misses or pauses reset it.
        MeleeComboCount = MeleeComboTimer > 0.f ? MeleeComboCount + 1 : 1;
        const bool bComboFinisher = MeleeComboCount >= 3;
        if (bComboFinisher) MeleeComboCount = 0;
        MeleeComboTimer = 1.1f;
        const float ComboMultiplier = bComboFinisher ? 1.5f : 1.f;
        const float Damage = BasicAttackDamage() * ComboMultiplier *
            (bRiposte ? CounterMultiplier : 1.f);
        // Aggressive play helps sustain dodges. Only a landed third strike
        // restores stamina; whiffs and arcane bolts never trigger the reward.
        if (bComboFinisher)
            Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 8.f);
        // Embersteel improves the return from a landed combo finisher only.
        if (bComboFinisher && ForgedRegionTier(0) > 0)
            Stamina = FMath::Min(GetHudMaxStamina(),
                Stamina + 2.f * ForgedRegionTier(0));
        CounterWindow = 0.f; // The riposte is spent only on a successful in-range attack.
        if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte, bComboFinisher);
        else
        {
            ShowDamageNumber(Damage, BossHealth);
            BossHealth = FMath::Max(0.f, BossHealth - Damage);
            // Perfect-dodge ripostes can interrupt a live telegraphed boss cast.
            if (bRiposte && bCasting && BossHealth > 0.f)
            {
                bCasting = false;
                WarningMesh->SetVisibility(false);
                CastCooldown = 2.f; // Start a fresh, exact interrupt recovery window.
            }
            else if (bComboFinisher && bCasting && BossHealth > 0.f)
            {
                // A landed third strike can break a boss windup, but the boss
                // recovers sooner than from a Perfect Dodge riposte.
                bCasting = false;
                WarningMesh->SetVisibility(false);
                CastCooldown = 1.5f;
            }
            if (BossHealth <= 0.f) Victory();
        }
    }
}
void AMoonfallPawn::CastBolt()
{
    if (bWorldMapOpen) return; // No combat casts through the map.
    // Moonweave rank lowers the cost of a successful arcane bolt only.
    const float BoltManaCost = FMath::Max(22.f, 30.f - 2.f * ForgedRegionTier(1));
    if (Health <= 0.f || ParryTimer > 0.f || BoltCooldown > 0.f ||
        Mana < BoltManaCost) return;
    const int32 EnemyIndex = NearestEnemyIndex(1000.f);
    const float BossDistance = bWon ? TNumericLimits<float>::Max() :
        FVector::Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation());
    const bool bBossInRange = BossDistance <= 1000.f;
    if (EnemyIndex == INDEX_NONE && !bBossInRange) return;
    const float EnemyDistance = EnemyIndex == INDEX_NONE ? TNumericLimits<float>::Max() :
        FVector::Dist2D(HeroMesh->GetRelativeLocation(),
                        Enemies[EnemyIndex].Mesh->GetRelativeLocation());
    const bool bHitBoss = bBossInRange && BossDistance < EnemyDistance;
    Mana -= BoltManaCost;
    BoltCooldown = 1.1f;
    // Voidcrystal focuses reward spell builds with faster follow-up casts.
    BoltCooldown = FMath::Max(0.7f,
        BoltCooldown - 0.08f * ForgedRegionTier(5));
    // A warned perfect dodge also empowers the next successful bolt, matching
    // the melee riposte. Invalid/out-of-range casts never consume the window.
    const bool bRiposte = CounterWindow > 0.f;
    const float Damage = ArcaneBoltDamage() *
        (bRiposte ? CounterMultiplier : 1.f);
    CounterWindow = 0.f;
    if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte);
    else
    {
        ShowDamageNumber(Damage, BossHealth);
        BossHealth = FMath::Max(0.f, BossHealth - Damage);
        if (bRiposte && bCasting && BossHealth > 0.f)
        {
            bCasting = false;
            WarningMesh->SetVisibility(false);
            CastCooldown = 2.f; // Start a fresh, exact interrupt recovery window.
        }
        if (BossHealth <= 0.f) Victory();
    }
}

void AMoonfallPawn::CastHeal()
{
    if (bWorldMapOpen) return;
    const float MaxHealth = GetHudMaxHealth();
    if (Health <= 0.f || ParryTimer > 0.f || HealCooldown > 0.f || Mana < 40.f || Health >= MaxHealth) return;
    Mana -= 40.f;
    HealCooldown = 5.f;
    Health = FMath::Min(MaxHealth, Health + HealingSpellAmount());
}

void AMoonfallPawn::Victory()
{
    bWon = true;
    bCasting = false;
    BossAttackCount = 0;
    WarningMesh->SetVisibility(false);
    BossMesh->SetVisibility(false);
    BossPortrait->SetHiddenInGame(true);
    Victories = FMath::Min(100, Victories + 1);
    // Permanent boss bonus raises max HP only; the kill does not heal the hero.
    Progression.AwardDefeat(0, true, true, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
    // Prototype drop uses matching rarity thresholds, not a Godot-compatible RNG seed.
    const int32 Rank = MoonfallLootRules::RarityIndex(FMath::RandRange(0, 999), 0, 0);
    BossLoot = FString(MoonfallLootRules::Rarities[Rank]) + TEXT(" boss relic (prototype)");
    if (BossInventory.Num() < 100) BossInventory.Add(BossLoot);
    SaveProgress();
}
void AMoonfallPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("Forward"), this, &AMoonfallPawn::MoveForward);
    Input->BindAxis(TEXT("Right"), this, &AMoonfallPawn::MoveRight);
    Input->BindAxis(TEXT("Zoom"), this, &AMoonfallPawn::Zoom);
    Input->BindAxis(TEXT("MapPanHorizontal"), this, &AMoonfallPawn::PanMapHorizontal);
    Input->BindAxis(TEXT("MapPanVertical"), this, &AMoonfallPawn::PanMapVertical);
    Input->BindAction(TEXT("Attack"), IE_Pressed, this, &AMoonfallPawn::Attack);
    Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AMoonfallPawn::Dodge);
    Input->BindAction(TEXT("Parry"), IE_Pressed, this, &AMoonfallPawn::Parry);
    Input->BindAction(TEXT("ForgeEquipment"), IE_Pressed, this, &AMoonfallPawn::ForgeEquipment);
    Input->BindAction(TEXT("ForgeMelee"), IE_Pressed, this, &AMoonfallPawn::ForgeMeleeEquipment);
    Input->BindAction(TEXT("ForgeArcane"), IE_Pressed, this, &AMoonfallPawn::ForgeArcaneEquipment);

    Input->BindAction(TEXT("ArcaneBolt"), IE_Pressed, this, &AMoonfallPawn::CastBolt);
    Input->BindAction(TEXT("ArcaneHeal"), IE_Pressed, this, &AMoonfallPawn::CastHeal);
    Input->BindAction(TEXT("SetWaypoint"), IE_Pressed, this, &AMoonfallPawn::SetWaypoint);
    Input->BindAction(TEXT("ClearWaypoint"), IE_Pressed, this, &AMoonfallPawn::ClearWaypoint);
    Input->BindAction(TEXT("ToggleWorldMap"), IE_Pressed, this, &AMoonfallPawn::ToggleWorldMap);
    Input->BindAction(TEXT("CloseWorldMap"), IE_Pressed, this, &AMoonfallPawn::CloseWorldMap);
    Input->BindAction(TEXT("ControllerAttack"), IE_Pressed, this, &AMoonfallPawn::ControllerAttack);
    Input->BindAction(TEXT("ControllerConfirm"), IE_Pressed, this, &AMoonfallPawn::ControllerConfirm);
    Input->BindAction(TEXT("ControllerBack"), IE_Pressed, this, &AMoonfallPawn::ControllerBack);
    Input->BindAction(TEXT("ControllerZoomIn"), IE_Pressed, this, &AMoonfallPawn::ControllerZoomIn);
    Input->BindAction(TEXT("ControllerZoomOut"), IE_Pressed, this, &AMoonfallPawn::ControllerZoomOut);
    Input->BindAction(TEXT("ControllerSetWaypoint"), IE_Pressed, this, &AMoonfallPawn::ControllerSetWaypoint);
}
void AMoonfallPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Dt = FMath::Max(0.f, DeltaSeconds);
    if (Health <= 0.f && !bDeathInputEnabled)
    {
        // If defeat and atlas state coincide, force-clear atlas mode. Calling
        // ToggleWorldMap here is blocked by its dead-hero guard and can strand
        // both controller A and mouse Respawn behind bWorldMapOpen.
        bWorldMapOpen = false;
        DodgeBufferTimer = 0.f;
        Forward = 0.f;
        Right = 0.f;
        MapPanHorizontalInput = 0.f;
        MapPanVerticalInput = 0.f;
        // Enable an accessible mouse pointer once for click-based respawn.
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            FInputModeGameAndUI DeathInput;
            DeathInput.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            DeathInput.SetHideCursorDuringCapture(false);
            PC->SetInputMode(DeathInput);
            PC->bShowMouseCursor = true;
            bDeathInputEnabled = true;
        }
    }
    // A defeated hero cannot keep regenerating resources or advancing AI.
    // Clear any active boss telegraph before the early return, otherwise
    // dying during the warning leaves an attack marker behind RESPAWN UI.
    if (Health <= 0.f)
    {
        bCasting = false;
        CastTimer = 0.f;
        if (WarningMesh) WarningMesh->SetVisibility(false);
        return;
    }
    if (bWorldMapOpen)
    {
        const float MapDt = FMath::Min(Dt, 0.05f);
        WorldMapCursorCells.X = FMath::Clamp(WorldMapCursorCells.X +
            MapPanHorizontalInput * 10.f * MapDt / WorldMapZoom, .5f, 47.5f);
        WorldMapCursorCells.Y = FMath::Clamp(WorldMapCursorCells.Y +
            MapPanVerticalInput * 10.f * MapDt / WorldMapZoom, .5f, 15.5f);
        const float MaxPanX = 24.f * (1.f - 1.f / WorldMapZoom);
        const float MaxPanY = 8.f * (1.f - 1.f / WorldMapZoom);
        // Follow the cursor only as far as the world edges allow.
        WorldMapPanCells.X = FMath::Clamp(WorldMapCursorCells.X - 24.f, -MaxPanX, MaxPanX);
        WorldMapPanCells.Y = FMath::Clamp(WorldMapCursorCells.Y - 8.f, -MaxPanY, MaxPanY);
        // The full-screen atlas disables combat actions. Freeze combat and
        // resource clocks too, so opening it never causes unavoidable damage
        // or grants free healing, mana, cooldowns, or invulnerability time.
        // Keep map panning responsive; resume from the exact prior combat state.
        return;
    }
    AttackCooldown = FMath::Max(0.f, AttackCooldown - Dt);
    // Preserve combat reaction windows across a rare long render stall.
    // Ordinary frame intervals still advance the timers in real time.
    const float ReactionDt = FMath::Min(Dt, 0.05f);
    MeleeComboTimer = FMath::Max(0.f, MeleeComboTimer - ReactionDt);
    if (MeleeComboTimer <= 0.f) MeleeComboCount = 0;
    // Advance cooldown before accepting a queued dodge, so the newly started
    // dodge protects and moves the hero on this very frame, not one frame late.
    DodgeCooldown = FMath::Max(0.f, DodgeCooldown - Dt);
    // A buffered press already happened before this frame. Resolve at most
    // one queue, and commit stamina only when the dodge actually starts.
    if (DodgeBufferTimer > 0.f)
    {
        const bool bBufferValidThisFrame = DodgeBufferTimer > 0.f;
        DodgeBufferTimer = FMath::Max(0.f, DodgeBufferTimer - Dt);
        if (bBufferValidThisFrame && DodgeCooldown <= 0.f &&
            DodgeTimer <= 0.f && ParryTimer <= 0.f && Stamina >= 45.f)
        {
            DodgeDirection = BufferedDodgeDirection;
            DodgeTimer = 0.32f;
            DodgeCooldown = 1.2f;
            Stamina -= 45.f;
            StaminaRegenDelay = 0.6f;
            DodgeBufferTimer = 0.f;
        }
    }
    // Snapshot newly activated or existing dodge protection before consuming
    // frame time. This same state controls movement and both damage paths.
    const float DodgeTimeRemainingThisFrame = DodgeTimer;
    const bool bDodgeActiveThisFrame = DodgeTimeRemainingThisFrame > 0.f;
    bool bParryActiveThisFrame = ParryTimer > 0.f;
    DodgeTimer = FMath::Max(0.f, DodgeTimer - ReactionDt);
    ParryTimer = FMath::Max(0.f, ParryTimer - ReactionDt);
    ParryCooldown = FMath::Max(0.f, ParryCooldown - Dt);
    // Dodging temporarily stops passive stamina gain. A rewarded perfect
    // dodge can still restore its explicit 10 stamina immediately.
    // Only regenerate for the portion of this frame after the delay ends.
    // Otherwise a frame that crosses the boundary grants free early stamina.
    const float StaminaRegenDt = FMath::Max(0.f, Dt - StaminaRegenDelay);
    StaminaRegenDelay = FMath::Max(0.f, StaminaRegenDelay - Dt);
    if (StaminaRegenDt > 0.f)
        Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 30.f * StaminaRegenDt);
    Mana = FMath::Min(180.f, Mana + 8.f * Dt);
    BoltCooldown = FMath::Max(0.f, BoltCooldown - Dt);
    HealCooldown = FMath::Max(0.f, HealCooldown - Dt);
    EnemyDamageCooldown = FMath::Max(0.f, EnemyDamageCooldown - Dt);
    DamageNumberTimer = FMath::Max(0.f, DamageNumberTimer - Dt);
    // Timer advances only in live gameplay. The world atlas pauses it too.
    const bool bRespawnProtectedThisFrame = RespawnGraceTimer > 0.f;
    RespawnGraceTimer = FMath::Max(0.f, RespawnGraceTimer - Dt);
    // Preserve earned ripostes during a render stall, matching dodge and telegraph timing.
    CounterWindow = FMath::Max(0.f, CounterWindow - ReactionDt);
    CheckpointTimer += Dt;
    if (Health > 0.f)
    {
        // A frame hitch must not teleport the hero through a boss warning or chamber.
        const float MovementDt = FMath::Min(Dt, 0.05f);
        // The last dodge frame must not travel a full frame after only a
        // fraction of the 0.32s dodge remains. Divide the movement between
        // committed dash and ordinary input without discarding frame time.
        const float DodgeMovementDt = bDodgeActiveThisFrame
            ? FMath::Min(MovementDt, DodgeTimeRemainingThisFrame) : 0.f;
        const float WalkMovementDt = MovementDt - DodgeMovementDt;
        const FVector WalkDirection = FVector(Forward, Right, 0.f).GetClampedToMaxSize(1.f);
        FVector Position = HeroMesh->GetRelativeLocation() +
            DodgeDirection * 1200.f * DodgeMovementDt +
            WalkDirection * 600.f * WalkMovementDt;
        Position.X = FMath::Clamp(Position.X, -1600.f, 17600.f);
        Position.Y = FMath::Clamp(Position.Y, -3200.f, 3200.f);
        HeroMesh->SetRelativeLocation(Position);
        const int32 CurrentRegion = MoonfallWorldRules::RegionAt(Position.X);
        const int32 RegionBit = 1 << CurrentRegion;
        const bool bNewRegion = (DiscoveredRegionMask & RegionBit) == 0;
        if (bNewRegion) DiscoveredRegionMask |= RegionBit;
        const bool bNewCells = RevealNearbyMapCells(Position);
        if (bNewRegion || bNewCells)
            SaveProgress(); // Persist only when discoveries change, not every frame.
        if (CheckpointTimer >= CheckpointInterval)
        {
            CheckpointTimer = 0.f;
            SaveProgress();
        }
        CollectNearbyChambers();
        UpdateWorldEnemies(Dt);
        // Smooth isometric tracking while keeping checkpoint startup exact.
        // Capped interpolation delta avoids a large hitch teleporting camera.
        const FVector CameraTarget(Position.X, Position.Y, 0.f);
        CameraBoom->SetRelativeLocation(FMath::VInterpTo(
            CameraBoom->GetRelativeLocation(), CameraTarget, FMath::Min(Dt, 0.05f), 9.f));
        if (Health > 0.f && EnemyDamageCooldown <= 0.f)
        {
            // Find the nearest *engaged* contact. A retreating enemy cannot
            // shield another active enemy just because it is slightly closer.
            int32 ContactIndex = INDEX_NONE;
            float ContactDistanceSq = FMath::Square(175.f);
            const FVector HeroPosition = HeroMesh->GetRelativeLocation();
            for (int32 I = 0; I < Enemies.Num(); ++I)
            {
                const FWorldEnemy& Candidate = Enemies[I];
                if (!Candidate.Mesh || Candidate.bReturningHome || Candidate.bStaggeredThisFrame) continue;
                if (!Candidate.bAttackReadyThisFrame) continue;
                const FVector HomeOffset = Candidate.Mesh->GetRelativeLocation() - Candidate.Home;
                const float Leash = Candidate.bGuardian ? 600.f : 900.f;
                if (HomeOffset.SizeSquared2D() >= FMath::Square(Leash)) continue;
                const FVector ToHero = Candidate.Mesh->GetRelativeLocation() - HeroPosition;
                const float DistanceSq = ToHero.SizeSquared2D();
                if (DistanceSq < ContactDistanceSq)
                {
                    ContactDistanceSq = DistanceSq;
                    ContactIndex = I;
                }
            }
            if (ContactIndex != INDEX_NONE)
            {
                const FWorldEnemy& Contact = Enemies[ContactIndex];
                if (bDodgeActiveThisFrame) CounterWindow = CounterDuration;
                else if (bParryActiveThisFrame)
                {
                    // One defensive success per parry input. Stagger the
                    // specific contact enemy instead of disabling all AI.
                    ParryTimer = 0.f;
                    bParryActiveThisFrame = false; // One successful parry per input.
                    CounterWindow = CounterDuration;
                    if (Enemies.IsValidIndex(ContactIndex))
                        Enemies[ContactIndex].StaggerTimer = 0.8f;
                }
                else if (!bRespawnProtectedThisFrame)
                {
                    const float ContactDamage = 5.f + 2.f * Contact.Region +
                        (Contact.bEnraged ? 4.f : 0.f);
                    // Stariron wards reduce only landed hits, never below 1 HP.
                    const float MitigatedDamage = FMath::Max(1.f,
                        ContactDamage - 0.75f * ForgedRegionTier(4) - 0.25f * ForgedRandomWard());
                    Health = FMath::Max(0.f, Health - MitigatedDamage);
                }
                // Protection does not consume damage recovery. The first
                // unprotected contact must still be a real threat.
                if (!bRespawnProtectedThisFrame)
                    EnemyDamageCooldown = 1.2f;
            }
        }
    }
    if (bWon || Health <= 0.f)
    {
        // No danger decals may linger over the world when the hero has died.
        bCasting = false;
        WarningMesh->SetVisibility(false);
        return;
    }
    if (FVector::Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation()) > 1600.f)
    {
        // Boss has a local aggro radius, not global map-wide attacks.
        bCasting = false;
        WarningMesh->SetVisibility(false);
        CastCooldown = FMath::Max(1.f, CastCooldown);
        return;
    }
    if (!bCasting)
    {
        CastCooldown -= Dt;
        if (CastCooldown <= 0.f)
        {
            // Phase thresholds are sampled at cast start; the warning remains stable
            // even if the hero damages the boss during the windup.
            BossAttackPhase = BossHealth <= 30.f ? 3 : (BossHealth <= 65.f ? 2 : 1);
            ++BossAttackCount;
            const bool bRadial = BossAttackPhase >= 2 && BossAttackCount % 2 == 0;
            const FVector Center = bRadial ? BossMesh->GetRelativeLocation()
                                           : HeroMesh->GetRelativeLocation();
            TargetWorld = FVector(Center.X, Center.Y, 0.f);
            BossWarningRadius = BossAttackPhase == 3 ? 350.f :
                                (BossAttackPhase == 2 ? 300.f : 250.f);
            // Cylinder mesh is 100 cm wide, so scale XY = radius / 50.
            WarningMesh->SetRelativeScale3D(FVector(
                BossWarningRadius / 50.f, BossWarningRadius / 50.f, 0.01f));
            WarningMesh->SetRelativeLocation(TargetWorld);
            WarningMesh->SetVisibility(true);
            CastTimer = BossAttackPhase == 3 ? 0.95f :
                        (BossAttackPhase == 2 ? 1.05f : 1.15f);
            bCasting = true;
        }
    }
    else
    {
        // Pulse the existing warning mesh rather than allocating a new VFX actor.
        // Pulse never understates the actual hazard radius.
        const float Pulse = 1.f + 0.06f * (0.5f + 0.5f *
            FMath::Sin((GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f) *
                (BossAttackPhase == 3 ? 17.f : 11.f)));
        WarningMesh->SetRelativeScale3D(FVector(
            BossWarningRadius / 50.f * Pulse, BossWarningRadius / 50.f * Pulse, 0.01f));
        // Preserve a readable reaction window during long render hitches.
        // A delayed frame cannot instantly consume the entire cast warning.
        const float TelegraphDt = FMath::Min(Dt, 0.05f);
        CastTimer -= TelegraphDt;
        if (CastTimer <= 0.f)
        {
            bCasting = false;
            WarningMesh->SetVisibility(false);
            CastCooldown = BossAttackPhase == 3 ? 2.25f :
                           (BossAttackPhase == 2 ? 2.9f : 3.5f);
            if (FVector::Dist2D(HeroMesh->GetRelativeLocation(), TargetWorld) < BossWarningRadius)
            {
                if (bParryActiveThisFrame)
                {
                    // Timed boss parry cancels this resolved attack and grants
                    // a riposte. It never restores HP or refunds the parry cost.
                    ParryTimer = 0.f;
                    bParryActiveThisFrame = false; // One successful parry per input.
                    CounterWindow = CounterDuration;
                    CastCooldown = FMath::Max(CastCooldown, 2.f);
                }
                else if (bDodgeActiveThisFrame)
                {
                    // A successful warned dodge rewards timing, not passive kill healing.
                    // Each strike resolves only once, limiting the stamina refund.
                    CounterWindow = CounterDuration;
                    Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 10.f);
                }
                else if (EnemyDamageCooldown <= 0.f && !bRespawnProtectedThisFrame)
                {
                    // Boss and mob strikes share a brief damage recovery window:
                    // overlapping attacks must not instantly double-hit the hero.
                    const float BossBaseDamage = BossAttackPhase == 3 ? 28.f :
                        (BossAttackPhase == 2 ? 24.f : 20.f);
                    Health = FMath::Max(0.f, Health - FMath::Max(1.f,
                        BossBaseDamage - 0.75f * ForgedRegionTier(4) - 0.25f * ForgedRandomWard()));
                    EnemyDamageCooldown = 1.2f;
                }
            }
        }
    }

}
