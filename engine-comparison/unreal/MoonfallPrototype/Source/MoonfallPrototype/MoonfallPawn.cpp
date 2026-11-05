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
    HeroPortrait = CreateDefaultSubobject<UBillboardComponent>(TEXT("OriginalHeroArtwork"));
    HeroPortrait->SetupAttachment(HeroMesh);
    HeroPortrait->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
    HeroPortrait->SetRelativeScale3D(FVector(2.5f));
    HeroPortrait->SetHiddenInGame(true);
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

void AMoonfallPawn::BeginPlay()
{
    Super::BeginPlay();
    // Separate prototype save slot. Never touches production Godot saves.
    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(TEXT("MoonfallPrototypeBossRewards"), 0))
    {
        if (const UMoonfallProgressSave* Progress = Cast<UMoonfallProgressSave>(Loaded))
        {
            Victories = FMath::Clamp(Progress->PermanentBossVictories, 0, 100);
            bWon = Progress->bBossDefeated;
            DefeatedEnemyMask = Progress->DefeatedEnemyMask & ((1 << MoonfallWorldRules::TotalEnemies) - 1);
            ClaimedChamberMask = Progress->ClaimedChamberMask & ((1 << MoonfallWorldRules::TotalChambers) - 1);
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
    Health = 100.f + 5.f * Victories;
    BuildWorld();
    // These paths exist only after running the Unreal Editor art importer.
    // Missing imports keep the geometric placeholders visible.
    ZArtwork = LoadObject<UTexture2D>(nullptr, TEXT("/Game/MoonfallArt/heroes/Z_Original.Z_Original"));
    NyraArtwork = LoadObject<UTexture2D>(nullptr, TEXT("/Game/MoonfallArt/heroes/Nyra_Original.Nyra_Original"));
    UTexture2D* BossArtwork = LoadObject<UTexture2D>(nullptr,
        TEXT("/Game/MoonfallArt/v31_4_bestiary/boss_0.boss_0"));
    BossPortrait->SetSprite(BossArtwork);
    BossPortrait->SetHiddenInGame(BossArtwork == nullptr);
    if (BossArtwork) BossMesh->SetHiddenInGame(true);
    ApplyHeroArtwork(ZArtwork ? 0 : 1);
    if (bWon)
    {
        BossHealth = 0.f;
        BossMesh->SetVisibility(false);
        BossPortrait->SetHiddenInGame(true);
    }

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
            Reward.Id = ChamberId;
            ChamberCaches.Add(Reward);
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
            Enemies.Add(Enemy);
        }
    }
}

int32 AMoonfallPawn::NearestEnemyIndex(float Radius) const
{
    int32 Best = INDEX_NONE;
    float Distance = Radius;
    const FVector HeroPosition = HeroMesh->GetRelativeLocation();
    for (int32 I = 0; I < Enemies.Num(); ++I)
    {
        const FWorldEnemy& Enemy = Enemies[I];
        const float Current = FVector::Dist2D(Enemy.Mesh->GetRelativeLocation(), HeroPosition);
        if (Current < Distance) { Distance = Current; Best = I; }
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

void AMoonfallPawn::DamageWorldEnemy(int32 Index, float Damage)
{
    if (!Enemies.IsValidIndex(Index)) return;
    FWorldEnemy& Enemy = Enemies[Index];
    Enemy.Health = FMath::Max(0.f, Enemy.Health - Damage);
    if (Enemy.Health > 0.f) return;
    const int32 Region = Enemy.Region;
    DefeatedEnemyMask |= 1 << Enemy.Id;
    if (Enemy.Portrait) Enemy.Portrait->DestroyComponent();
    Enemy.Mesh->DestroyComponent();
    Enemies.RemoveAt(Index);
    const int32 LevelsGained = Progression.AwardDefeat(Region, false, false, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
    Health = FMath::Min(100.f + 5.f * Victories, Health + 20.f * LevelsGained);
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
        ClaimedChamberMask |= 1 << Id;
        Cache.Mesh->DestroyComponent();
        ChamberCaches.RemoveAt(I);
        const int32 Region = Id / MoonfallWorldRules::ChambersPerRegion;
        const int32 LevelsGained = Progression.AwardDefeat(
            Region, false, false, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
        Health = FMath::Min(100.f + 5.f * Victories, Health + 20.f * LevelsGained);
        Mana = FMath::Min(180.f, Mana + 25.f);
        SaveProgress();
    }
}

void AMoonfallPawn::SaveProgress()
{
    UMoonfallProgressSave* Progress = Cast<UMoonfallProgressSave>(
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
    Progress->bBossDefeated = bWon;
    if (!UGameplayStatics::SaveGameToSlot(Progress, TEXT("MoonfallPrototypeBossRewards"), 0))
        UE_LOG(LogTemp, Warning, TEXT("Moonfall prototype: reward save failed."));
}

void AMoonfallPawn::ApplyHeroArtwork(int32 Index)
{
    UTexture2D* Chosen = Index == 1 ? NyraArtwork : ZArtwork;
    if (!Chosen) return; // Never replace artwork with a fabricated portrait.
    HeroArtIndex = Index;
    HeroPortrait->SetSprite(Chosen);
    HeroPortrait->SetHiddenInGame(false);
    HeroMesh->SetHiddenInGame(true);
}
void AMoonfallPawn::ChooseZ() { ApplyHeroArtwork(0); }
void AMoonfallPawn::ChooseNyra() { ApplyHeroArtwork(1); }

void AMoonfallPawn::MoveForward(float Value) { Forward = Value; }
void AMoonfallPawn::MoveRight(float Value) { Right = Value; }
void AMoonfallPawn::Zoom(float Value)
{
    ZoomDistance = FMath::Clamp(ZoomDistance - Value * 125.f, 900.f, 2500.f);
    CameraBoom->TargetArmLength = ZoomDistance;
}
void AMoonfallPawn::Dodge()
{
    if (Health > 0.f && DodgeCooldown <= 0.f && Stamina >= 45.f)
    {
        DodgeTimer = 0.32f;
        DodgeCooldown = 1.2f;
        Stamina -= 45.f;
    }
}
void AMoonfallPawn::Attack()
{
    if (Health <= 0.f || AttackCooldown > 0.f) return;
    AttackCooldown = 0.35f;
    const int32 EnemyIndex = NearestEnemyIndex(400.f);
    if (EnemyIndex != INDEX_NONE) DamageWorldEnemy(EnemyIndex, 15.f);
    else if (!bWon && FVector::Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation()) <= 400.f)
    {
        BossHealth = FMath::Max(0.f, BossHealth - 15.f);
        if (BossHealth <= 0.f) Victory();
    }
}
void AMoonfallPawn::CastBolt()
{
    if (Health <= 0.f || BoltCooldown > 0.f || Mana < 30.f) return;
    const int32 EnemyIndex = NearestEnemyIndex(1000.f);
    if (EnemyIndex == INDEX_NONE && (bWon ||
        FVector::Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation()) > 1000.f)) return;
    Mana -= 30.f;
    BoltCooldown = 1.1f;
    if (EnemyIndex != INDEX_NONE) DamageWorldEnemy(EnemyIndex, 25.f + 2.f * Progression.Level);
    else
    {
        BossHealth = FMath::Max(0.f, BossHealth - (25.f + 2.f * Progression.Level));
        if (BossHealth <= 0.f) Victory();
    }
}

void AMoonfallPawn::CastHeal()
{
    const float MaxHealth = 100.f + 5.f * Victories;
    if (Health <= 0.f || HealCooldown > 0.f || Mana < 40.f || Health >= MaxHealth) return;
    Mana -= 40.f;
    HealCooldown = 5.f;
    Health = FMath::Min(MaxHealth, Health + 30.f + Progression.Level);
}

void AMoonfallPawn::Victory()
{
    bWon = true;
    bCasting = false;
    WarningMesh->SetVisibility(false);
    BossMesh->SetVisibility(false);
    BossPortrait->SetHiddenInGame(true);
    const int32 Previous = Victories;
    Victories = FMath::Min(100, Victories + 1);
    Health += 5.f * (Victories - Previous);
    const int32 LevelsGained = Progression.AwardDefeat(0, true, true, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
    Health = FMath::Min(100.f + 5.f * Victories, Health + 20.f * LevelsGained);
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
    Input->BindAction(TEXT("SelectZArt"), IE_Pressed, this, &AMoonfallPawn::ChooseZ);
    Input->BindAction(TEXT("SelectNyraArt"), IE_Pressed, this, &AMoonfallPawn::ChooseNyra);
}
void AMoonfallPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Dt = FMath::Max(0.f, DeltaSeconds);
    AttackCooldown = FMath::Max(0.f, AttackCooldown - Dt);
    DodgeTimer = FMath::Max(0.f, DodgeTimer - Dt);
    DodgeCooldown = FMath::Max(0.f, DodgeCooldown - Dt);
    Stamina = FMath::Min(150.f, Stamina + 30.f * Dt);
    Mana = FMath::Min(180.f, Mana + 8.f * Dt);
    BoltCooldown = FMath::Max(0.f, BoltCooldown - Dt);
    HealCooldown = FMath::Max(0.f, HealCooldown - Dt);
    EnemyDamageCooldown = FMath::Max(0.f, EnemyDamageCooldown - Dt);
    if (Health > 0.f)
    {
        FVector Direction(Forward, Right, 0.f);
        Direction = Direction.GetClampedToMaxSize(1.f);
        FVector Position = HeroMesh->GetRelativeLocation() + Direction * (DodgeTimer > 0.f ? 1200.f : 600.f) * Dt;
        Position.X = FMath::Clamp(Position.X, -1600.f, 17600.f);
        Position.Y = FMath::Clamp(Position.Y, -3200.f, 3200.f);
        HeroMesh->SetRelativeLocation(Position);
        CollectNearbyChambers();
        CameraBoom->SetRelativeLocation(FVector(Position.X, Position.Y, 0.f));
        if (Health > 0.f && DodgeTimer <= 0.f && EnemyDamageCooldown <= 0.f &&
            NearestEnemyIndex(175.f) != INDEX_NONE)
        {
            const FWorldEnemy& Nearby = Enemies[NearestEnemyIndex(175.f)];
            Health = FMath::Max(0.f, Health - (5.f + 2.f * Nearby.Region));
            EnemyDamageCooldown = 1.2f;
        }
    }
    if (bWon || Health <= 0.f) return;
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
            TargetWorld = HeroMesh->GetRelativeLocation();
            WarningMesh->SetRelativeLocation(FVector(TargetWorld.X, TargetWorld.Y, 0.f));
            WarningMesh->SetVisibility(true);
            CastTimer = 1.15f;
            bCasting = true;
        }
    }
    else
    {
        CastTimer -= Dt;
        if (CastTimer <= 0.f)
        {
            bCasting = false;
            WarningMesh->SetVisibility(false);
            CastCooldown = 3.5f;
            if (DodgeTimer <= 0.f && FVector::Dist2D(HeroMesh->GetRelativeLocation(), TargetWorld) < 250.f)
                Health = FMath::Max(0.f, Health - 20.f);
        }
    }

}
