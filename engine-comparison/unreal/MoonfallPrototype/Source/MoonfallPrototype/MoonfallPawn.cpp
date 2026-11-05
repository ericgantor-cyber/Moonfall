#include "MoonfallPawn.h"
#include "MoonfallProgressSave.h"
#include "MoonfallLootRules.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
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
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cube.Succeeded()) FloorMesh->SetStaticMesh(Cube.Object);
    if (Sphere.Succeeded())
    {
        HeroMesh->SetStaticMesh(Sphere.Object);
        BossMesh->SetStaticMesh(Sphere.Object);
    }
    if (Cylinder.Succeeded()) WarningMesh->SetStaticMesh(Cylinder.Object);
    HeroMesh->SetRelativeLocation(FVector(0.f, -500.f, 100.f));
    BossMesh->SetRelativeLocation(FVector(0.f, 600.f, 100.f));
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
    if (Health > 0.f && DodgeCooldown <= 0.f && Stamina >= 45.f)
    {
        DodgeTimer = 0.32f;
        DodgeCooldown = 1.2f;
        Stamina -= 45.f;
    }
}
void AMoonfallPawn::Attack()
{
    if (Health <= 0.f || bWon || AttackCooldown > 0.f) return;
    AttackCooldown = 0.35f;
    if (FVector::Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation()) <= 400.f)
    {
        BossHealth = FMath::Max(0.f, BossHealth - 15.f);
        if (BossHealth <= 0.f) Victory();
    }
}
void AMoonfallPawn::CastBolt()
{
    if (Health <= 0.f || bWon || BoltCooldown > 0.f || Mana < 30.f) return;
    if (FVector::Dist2D(HeroMesh->GetRelativeLocation(), BossMesh->GetRelativeLocation()) > 1000.f) return;
    Mana -= 30.f;
    BoltCooldown = 1.1f;
    BossHealth = FMath::Max(0.f, BossHealth - (25.f + 2.f * Progression.Level));
    if (BossHealth <= 0.f) Victory();
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
    const int32 Previous = Victories;
    Victories = FMath::Min(100, Victories + 1);
    Health += 5.f * (Victories - Previous);
    const int32 LevelsGained = Progression.AwardDefeat(0, true, true, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
    Health = FMath::Min(100.f + 5.f * Victories, Health + 20.f * LevelsGained);
    // Prototype drop uses matching rarity thresholds, not a Godot-compatible RNG seed.
    const int32 Rank = MoonfallLootRules::RarityIndex(FMath::RandRange(0, 999), 0, 0);
    BossLoot = FString(MoonfallLootRules::Rarities[Rank]) + TEXT(" boss relic (prototype)");
    if (BossInventory.Num() < 100) BossInventory.Add(BossLoot);
    UMoonfallProgressSave* Progress = Cast<UMoonfallProgressSave>(
        UGameplayStatics::CreateSaveGameObject(UMoonfallProgressSave::StaticClass()));
    bool bSaved = false;
    if (Progress)
    {
        Progress->PermanentBossVictories = Victories;
        Progress->BossRelics = BossInventory;
        Progress->HeroLevel = Progression.Level;
        Progress->HeroXP = Progression.Experience;
        Progress->HeroPoints = Progression.HeroPoints;
        Progress->TamerPoints = Progression.TamerPoints;
        bSaved = UGameplayStatics::SaveGameToSlot(Progress, TEXT("MoonfallPrototypeBossRewards"), 0);
    }
    if (!bSaved) UE_LOG(LogTemp, Warning, TEXT("Moonfall prototype: reward save failed."));
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
    AttackCooldown = FMath::Max(0.f, AttackCooldown - Dt);
    DodgeTimer = FMath::Max(0.f, DodgeTimer - Dt);
    DodgeCooldown = FMath::Max(0.f, DodgeCooldown - Dt);
    Stamina = FMath::Min(150.f, Stamina + 30.f * Dt);
    Mana = FMath::Min(180.f, Mana + 8.f * Dt);
    BoltCooldown = FMath::Max(0.f, BoltCooldown - Dt);
    HealCooldown = FMath::Max(0.f, HealCooldown - Dt);
    if (Health > 0.f)
    {
        FVector Direction(Forward, Right, 0.f);
        Direction = Direction.GetClampedToMaxSize(1.f);
        FVector Position = HeroMesh->GetRelativeLocation() + Direction * (DodgeTimer > 0.f ? 1200.f : 600.f) * Dt;
        Position.X = FMath::Clamp(Position.X, -1600.f, 1600.f);
        Position.Y = FMath::Clamp(Position.Y, -1600.f, 1600.f);
        HeroMesh->SetRelativeLocation(Position);
        CameraBoom->SetRelativeLocation(FVector(Position.X, Position.Y, 0.f));
    }
    if (bWon || Health <= 0.f) return;
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
