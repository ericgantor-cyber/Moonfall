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
            // Legacy SelectedHeroIndex is retained in old save objects but ignored:
            // Z and Nyra are encounter bosses, not player avatar choices.
            DefeatedEnemyMask = Progress->DefeatedEnemyMask & ((1 << MoonfallWorldRules::TotalEnemies) - 1);
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
    // Load original boss art before world generation so Nyra's guardian can use it.
    // Missing imports retain the regular prototype mesh.
    ZArtwork = LoadObject<UTexture2D>(nullptr, TEXT("/Game/MoonfallArt/heroes/Z_Original.Z_Original"));
    NyraArtwork = LoadObject<UTexture2D>(nullptr, TEXT("/Game/MoonfallArt/heroes/Nyra_Original.Nyra_Original"));
    BuildWorld();
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
        Enemy.bAttackReadyThisFrame = false;
        Enemy.AttackRecovery = FMath::Max(0.f, Enemy.AttackRecovery - MoveDt);
        // Snapshot stagger before advancing timers so the final active frame
        // blocks both movement and contact damage.
        Enemy.bStaggeredThisFrame = Enemy.StaggerTimer > 0.f;
        if (Enemy.bStaggeredThisFrame)
        {
            Enemy.StaggerTimer = FMath::Max(0.f, Enemy.StaggerTimer - MoveDt);
            Enemy.AttackWindup = 0.f;
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
        // Telegraph contact attacks. Starting a windup is separate from
        // delivering damage, giving players time to move or dodge.
        if (Enemy.AttackWindup > 0.f)
        {
            Enemy.AttackWindup = FMath::Max(0.f, Enemy.AttackWindup - MoveDt);
            if (Enemy.AttackWindup <= 0.f)
            {
                Enemy.bAttackReadyThisFrame = bAggro &&
                    HeroDistSq <= FMath::Square(210.f);
                Enemy.AttackRecovery = Enemy.bGuardian ? 1.7f : 1.3f;
            }
        }
        else if (bAggro && HeroDistSq <= FMath::Square(210.f) &&
                 Enemy.AttackRecovery <= 0.f)
        {
            Enemy.AttackWindup = Enemy.bGuardian ? 0.85f : 0.6f;
        }
        // Pulse the existing placeholder/portrait during a warning.
        // No new textures or permanent scene components needed.
        const float WarningScale = Enemy.AttackWindup > 0.f ? 1.25f : 1.f;
        if (Enemy.Portrait) Enemy.Portrait->SetRelativeScale3D(FVector(2.f * WarningScale));
        else Enemy.Mesh->SetRelativeScale3D(FVector((Enemy.bGuardian ? 1.3f : 1.1f) * WarningScale));
        if (bAggro && Enemy.AttackWindup <= 0.f)
        {
            if (HeroDistSq > FMath::Square(160.f))
            {
                const float Remaining = FMath::Max(0.f, Leash - FromHome.Size());
                const float Step = FMath::Min((Enemy.bEnraged ? 360.f : (Enemy.bGuardian ? 280.f : 200.f)) * MoveDt, Remaining);
                const FVector Candidate = From + ToHero.GetSafeNormal() * Step;
                const FVector ToCandidateHome = Candidate - Enemy.Home;
                // Prevent oblique pursuit from leaking outside the home radius.
                const FVector Clamped = Enemy.Home + ToCandidateHome.GetClampedToMaxSize2D(Leash);
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

void AMoonfallPawn::RecordDamageNumber(const FVector& LocalPosition,
    float Amount, bool bHeavy, bool bMagic)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0.f) return;
    // A bounded list prevents growth in long battles.
    if (DamageNumbers.Num() >= 24) DamageNumbers.RemoveAt(0);
    FDamageNumber Number;
    Number.LocalPosition = LocalPosition + FVector(0.f, 0.f, 130.f);
    Number.Amount = Amount;
    Number.bHeavy = bHeavy;
    Number.bMagic = bMagic;
    DamageNumbers.Add(Number);
}

void AMoonfallPawn::DamageWorldEnemy(int32 Index, float Damage, bool bRiposte, bool bComboFinisher, bool bMagic)
{
    if (!Enemies.IsValidIndex(Index)) return;
    FWorldEnemy& Enemy = Enemies[Index];
    const float ActualDamage = FMath::Min(Enemy.Health, FMath::Max(0.f, Damage));
    if (Enemy.Mesh)
        RecordDamageNumber(Enemy.Mesh->GetRelativeLocation(), ActualDamage,
            bComboFinisher || bRiposte, bMagic);
    Enemy.Health = FMath::Max(0.f, Enemy.Health - Damage);
    if (Enemy.Health > 0.f)
    {
        // The reward requires a real hit on a living guardian; defeated foes
        // need no stun state and ordinary hits cannot lock down bosses.
        if ((bRiposte && Enemy.bGuardian) || (bComboFinisher && !Enemy.bGuardian))
        {
            // Third-hit finishers briefly stagger normal enemies; earned
            // ripostes retain the longer guardian-specific stagger.
            Enemy.StaggerTimer = Enemy.bGuardian ? 0.8f : 0.4f;
            Enemy.AttackWindup = 0.f; // A successful interrupt cancels the warning.
            Enemy.AttackRecovery = FMath::Max(Enemy.AttackRecovery, 0.5f);
            if (Enemy.Portrait) Enemy.Portrait->SetRelativeScale3D(FVector(2.f));
            else Enemy.Mesh->SetRelativeScale3D(FVector(Enemy.bGuardian ? 1.3f : 1.1f));
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
    for (int32 I = ChamberCaches.Num() - 1; I >= 0; --I)
    {
        const FChamberCache& Cache = ChamberCaches[I];
        if (!Cache.Mesh || FVector::Dist2D(HeroPosition, Cache.Mesh->GetRelativeLocation()) > 250.f) continue;
        const int32 Id = Cache.Id;
        if ((DefeatedGuardianMask & (1 << Id)) == 0) continue; // No loot before guardian defeat.
        ClaimedChamberMask |= 1 << Id;
        if (Cache.Artwork) Cache.Artwork->DestroyComponent();
        Cache.Mesh->DestroyComponent();
        ChamberCaches.RemoveAt(I);
        const int32 Region = Id / MoonfallWorldRules::ChambersPerRegion;
        Progression.AwardDefeat(
            Region, false, false, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
        Mana = FMath::Min(180.f, Mana + 25.f);
        SaveProgress();
    }
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
    Progress->ClaimedChamberMask = ClaimedChamberMask;
    Progress->DefeatedGuardianMask = DefeatedGuardianMask;
    Progress->bBossDefeated = bWon;
    // Preserve legacy SelectedHeroIndex from the loaded save; never reassign it.
    if (HeroMesh && Health > 0.f)
    {
        Progress->bHasCheckpoint = true;
        Progress->SavedHeroPosition = HeroMesh->GetRelativeLocation();
        Progress->SavedHealth = Health;
        Progress->bHasResourceCheckpoint = true;
        Progress->SavedMana = Mana;
        Progress->SavedStamina = Stamina;
    }
    if (!UGameplayStatics::SaveGameToSlot(Progress, TEXT("MoonfallPrototypeBossRewards"), 0))
        UE_LOG(LogTemp, Warning, TEXT("Moonfall prototype: reward save failed."));
}

void AMoonfallPawn::MoveForward(float Value) { Forward = Value; }
void AMoonfallPawn::MoveRight(float Value) { Right = Value; }
void AMoonfallPawn::Zoom(float Value)
{
    ZoomDistance = FMath::Clamp(ZoomDistance - Value * 125.f, 900.f, 2500.f);
    CameraBoom->TargetArmLength = ZoomDistance;
}
void AMoonfallPawn::Dodge()
{
    if (Health > 0.f && DodgeCooldown <= 0.f && Stamina >= 45.f &&
        FVector2D(Forward, Right).SizeSquared() > 0.01f)
    {
        DodgeTimer = 0.32f;
        DodgeCooldown = 1.2f;
        Stamina -= 45.f;
        StaminaRegenDelay = 0.6f;
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

float AMoonfallPawn::BasicAttackDamage() const
{
    return 18.f + Progression.Level + 0.5f * GuardianBlessingCount();
}

float AMoonfallPawn::ArcaneBoltDamage() const
{
    return 28.f + 2.f * Progression.Level + 1.f * GuardianBlessingCount();
}

float AMoonfallPawn::HealingSpellAmount() const
{
    return 30.f + Progression.Level;
}

void AMoonfallPawn::Attack()
{
    if (Health <= 0.f || AttackCooldown > 0.f) return;
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
        CounterWindow = 0.f; // The riposte is spent only on a successful in-range attack.
        if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte, bComboFinisher);
        else
        {
            RecordDamageNumber(BossMesh->GetRelativeLocation(),
                FMath::Min(BossHealth, Damage), bComboFinisher || bRiposte, false);
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
    if (Health <= 0.f || BoltCooldown > 0.f || Mana < 30.f) return;
    const int32 EnemyIndex = NearestEnemyIndex(1000.f);
    const float BossDistance = bWon ? TNumericLimits<float>::Max() :
        FVector::Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation());
    const bool bBossInRange = BossDistance <= 1000.f;
    if (EnemyIndex == INDEX_NONE && !bBossInRange) return;
    const float EnemyDistance = EnemyIndex == INDEX_NONE ? TNumericLimits<float>::Max() :
        FVector::Dist2D(HeroMesh->GetRelativeLocation(),
                        Enemies[EnemyIndex].Mesh->GetRelativeLocation());
    const bool bHitBoss = bBossInRange && BossDistance < EnemyDistance;
    Mana -= 30.f;
    BoltCooldown = 1.1f;
    // A warned perfect dodge also empowers the next successful bolt, matching
    // the melee riposte. Invalid/out-of-range casts never consume the window.
    const bool bRiposte = CounterWindow > 0.f;
    const float Damage = ArcaneBoltDamage() *
        (bRiposte ? CounterMultiplier : 1.f);
    CounterWindow = 0.f;
    if (!bHitBoss) DamageWorldEnemy(EnemyIndex, Damage, bRiposte, false, true);
    else
    {
        RecordDamageNumber(BossMesh->GetRelativeLocation(),
            FMath::Min(BossHealth, Damage), bRiposte, true);
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
    const float MaxHealth = GetHudMaxHealth();
    if (Health <= 0.f || HealCooldown > 0.f || Mana < 40.f || Health >= MaxHealth) return;
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
    Input->BindAction(TEXT("Attack"), IE_Pressed, this, &AMoonfallPawn::Attack);
    Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AMoonfallPawn::Dodge);
    Input->BindAction(TEXT("ArcaneBolt"), IE_Pressed, this, &AMoonfallPawn::CastBolt);
    Input->BindAction(TEXT("ArcaneHeal"), IE_Pressed, this, &AMoonfallPawn::CastHeal);
}
void AMoonfallPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Dt = FMath::Max(0.f, DeltaSeconds);
    for (int32 I = DamageNumbers.Num() - 1; I >= 0; --I)
    {
        DamageNumbers[I].Age += Dt;
        if (DamageNumbers[I].Age >= DamageNumberLifetime)
            DamageNumbers.RemoveAtSwap(I, 1, EAllowShrinking::No);
    }
    AttackCooldown = FMath::Max(0.f, AttackCooldown - Dt);
    // Preserve combat reaction windows across a rare long render stall.
    // Ordinary frame intervals still advance the timers in real time.
    const float ReactionDt = FMath::Min(Dt, 0.05f);
    MeleeComboTimer = FMath::Max(0.f, MeleeComboTimer - ReactionDt);
    if (MeleeComboTimer <= 0.f) MeleeComboCount = 0;
    // Snapshot dodge protection before advancing timers. The same active
    // frame controls travel, enemy contact and boss impact.
    const bool bDodgeActiveThisFrame = DodgeTimer > 0.f;
    DodgeTimer = FMath::Max(0.f, DodgeTimer - ReactionDt);
    DodgeCooldown = FMath::Max(0.f, DodgeCooldown - Dt);
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
    // Preserve earned ripostes during a render stall, matching dodge and telegraph timing.
    CounterWindow = FMath::Max(0.f, CounterWindow - ReactionDt);
    CheckpointTimer += Dt;
    if (Health > 0.f)
    {
        FVector Direction(Forward, Right, 0.f);
        Direction = Direction.GetClampedToMaxSize(1.f);
        // A frame hitch must not teleport the hero through a boss warning or chamber.
        const float MovementDt = FMath::Min(Dt, 0.05f);
        FVector Position = HeroMesh->GetRelativeLocation() +
            Direction * (bDodgeActiveThisFrame ? 1200.f : 600.f) * MovementDt;
        Position.X = FMath::Clamp(Position.X, -1600.f, 17600.f);
        Position.Y = FMath::Clamp(Position.Y, -3200.f, 3200.f);
        HeroMesh->SetRelativeLocation(Position);
        if (CheckpointTimer >= CheckpointInterval)
        {
            CheckpointTimer = 0.f;
            SaveProgress();
        }
        CollectNearbyChambers();
        UpdateWorldEnemies(Dt);
        CameraBoom->SetRelativeLocation(FVector(Position.X, Position.Y, 0.f));
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
                if (!Candidate.Mesh || Candidate.bReturningHome ||
                    Candidate.bStaggeredThisFrame || !Candidate.bAttackReadyThisFrame) continue;
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
                else
                {
                    const float ContactDamage = 5.f + 2.f * Contact.Region +
                        (Contact.bEnraged ? 4.f : 0.f);
                    Health = FMath::Max(0.f, Health - ContactDamage);
                }
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
                if (bDodgeActiveThisFrame)
                {
                    // A successful warned dodge rewards timing, not passive kill healing.
                    // Each strike resolves only once, limiting the stamina refund.
                    CounterWindow = CounterDuration;
                    Stamina = FMath::Min(GetHudMaxStamina(), Stamina + 10.f);
                }
                else if (EnemyDamageCooldown <= 0.f)
                {
                    // Boss and mob strikes share a brief damage recovery window:
                    // overlapping attacks must not instantly double-hit the hero.
                    Health = FMath::Max(0.f, Health -
                        (BossAttackPhase == 3 ? 28.f : (BossAttackPhase == 2 ? 24.f : 20.f)));
                    EnemyDamageCooldown = 1.2f;
                }
            }
        }
    }

}
