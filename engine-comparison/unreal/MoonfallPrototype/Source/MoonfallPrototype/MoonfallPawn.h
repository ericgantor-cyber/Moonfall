#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "MoonfallProgression.h"
#include "MoonfallWorldRules.h"
#include "MoonfallPawn.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UBillboardComponent;
class UTexture2D;

UCLASS()
class MOONFALLPROTOTYPE_API AMoonfallPawn : public APawn
{
    GENERATED_BODY()
public:
    AMoonfallPawn();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    bool IsDefeated() const { return Health <= 0.f; }
    bool IsGuarding() const { return bGuardHeld && Health > 0.f && DodgeTimer <= 0.f && Stamina > 0.f; }
    bool IsParryWindowActive() const { return IsGuarding() && GuardParryTimer > 0.f; }
    FVector GetGuardDirection() const { return GuardDirection; }
    float GetHudHealth() const { return Health; }
    float GetHudMaxHealth() const { return 100.f + 5.f * Victories + 2.f * (Progression.Level - 1); }
    float GetHudMana() const { return Mana; }
    float GetHudMaxMana() const { return 180.f; } // Spell mana spending not yet ported.
    float GetHudManaRatio() const { return Mana / GetHudMaxMana(); }
    float GetHudStamina() const { return Stamina; }
    float GetHudMaxStamina() const { return 150.f; }
    int32 GetHudLevel() const { return Progression.Level; }
    int32 GetHudExperience() const { return Progression.Experience; }
    int32 GetHudExperienceToNext() const { return Progression.ExperienceToNext(); }
    float GetHudXpRatio() const { return static_cast<float>(Progression.Experience) / FMath::Max(1, Progression.ExperienceToNext()); }
    FVector GetHeroMapPosition() const { return HeroMesh ? HeroMesh->GetRelativeLocation() : FVector::ZeroVector; }
    FVector GetBossMapPosition() const { return BossMesh ? BossMesh->GetRelativeLocation() : FVector::ZeroVector; }
    bool IsBossDefeated() const { return bWon; }
    // T cycles ALL -> CHAMBERS -> BOSS -> OFF, no enemy tracking.
    int32 GetMinimapTrackingMode() const { return MinimapTrackingMode; }
    int32 GetMinimapZoomLevel() const { return MinimapZoomLevel; }
    float GetMinimapWorldRadius() const { return MinimapZoomLevel == 2 ? 850.f : (MinimapZoomLevel == 1 ? 1200.f : 1700.f); }
    float GetBossPostureRatio() const { return FMath::Clamp(BossPosture / BossPostureThreshold, 0.f, 1.f); }
    bool IsBossPostureActive() const { return !bWon && BossPosture > 0.f; }
    bool IsBossStaggered() const { return !bWon && BossStaggerTimer > 0.f; }
    TArray<FVector> GetChamberMapPositions() const;
    struct FDamageNumber
    {
        FVector LocalPosition = FVector::ZeroVector;
        float Amount = 0.f;
        float Age = 0.f;
        bool bHeavy = false;
        bool bMagic = false;
        bool bIncoming = false;
        bool bDodged = false;
        bool bBlocked = false;
        bool bParried = false;
        bool bGuardBroken = false;
    };
    const TArray<FDamageNumber>& GetDamageNumbers() const { return DamageNumbers; }
    static constexpr float DamageNumberLifetime = 1.25f;
    struct FAttackWarning
    {
        FVector LocalPosition = FVector::ZeroVector;
        FVector DirectionToHero = FVector::ForwardVector;
        float Urgency = 0.f;
        bool bGuardian = false;
        bool bBoss = false;
        bool bUnblockable = false;
    };
    TArray<FAttackWarning> GetAttackWarnings() const;
    struct FEnemyPosture
    {
        FVector LocalPosition = FVector::ZeroVector;
        float Ratio = 0.f;
        bool bGuardian = false;
    };
    TArray<FEnemyPosture> GetEnemyPostures() const;
private:
    UPROPERTY() USceneComponent* Root;
    UPROPERTY() UStaticMeshComponent* HeroMesh;
    UPROPERTY() UStaticMeshComponent* BossMesh;
    UPROPERTY() UStaticMeshComponent* WarningMesh;
    UPROPERTY() UStaticMeshComponent* FloorMesh;
    UPROPERTY() UStaticMesh* RegionShape = nullptr;
    UPROPERTY() UStaticMesh* EnemyShape = nullptr;
    struct FWorldEnemy
    {
        UStaticMeshComponent* Mesh = nullptr;
        UBillboardComponent* Portrait = nullptr;
        float Health = 0.f;
        float StaggerTimer = 0.f; // Surviving guardians are stunned by dodge ripostes.
        float AttackWindup = 0.f; // Telegraph before an ordinary enemy can strike.
        float AttackRecovery = 0.f; // No instant repeated contact hits.
        float Posture = 0.f; // Temporary defense pressure, not persistent character stats.
        float PostureIdleTimer = 0.f;
        bool bAttackReadyThisFrame = false;
        bool bStaggeredThisFrame = false; // Keep contact disabled on the final stagger frame.
        int32 Region = 0;
        int32 Id = 0;
        bool bGuardian = false;
        bool bEnraged = false; // Final 35% guardian combat phase.
        bool bReturningHome = false; // Leash hysteresis prevents edge-of-range jitter.
        FVector Home = FVector::ZeroVector;
    };
    TArray<FWorldEnemy> Enemies;
    TArray<FDamageNumber> DamageNumbers; // Bounded, temporary combat feedback only.
    void RecordDamageNumber(const FVector& LocalPosition, float Amount, bool bHeavy, bool bMagic);
    void RecordHeroHit(float Amount, bool bDodged);
    void RecordGuardEvent(bool bParried, bool bBroken);
    float ResolveGuardHit(const FVector& AttackOrigin, float Damage, bool bUnblockable, bool& bParried);
    void BeginGuard();
    void EndGuard();
    void CycleMinimapTracking();
    int32 MinimapTrackingMode = 0; // 0 all, 1 chambers, 2 boss, 3 off.
    // UI clicks are handled before combat, so zoom buttons never trigger attacks.
    bool TryHandleMinimapZoomClick();
    int32 MinimapZoomLevel = 0; // 0 wide, 1 medium, 2 close.
    struct FChamberCache
    {
        UStaticMeshComponent* Mesh = nullptr;
        UBillboardComponent* Artwork = nullptr;
        int32 Id = 0;
    };
    TArray<FChamberCache> ChamberCaches;
    int32 ClaimedChamberMask = 0;
    int32 DefeatedGuardianMask = 0;
    int32 DefeatedEnemyMask = 0;
    float RespawnGraceTimer = 0.f; // Short safety window on restored living checkpoints.
    static constexpr float RespawnGraceDuration = 2.f;
    float EnemyDamageCooldown = 0.f;
    bool bGuardHeld = false;
    bool bBossRadialAttack = false; // Radial boss attacks are dodge-only.
    FVector GuardDirection = FVector::ForwardVector;
    float GuardParryTimer = 0.f;
    float GuardParryRecovery = 0.f; // Stops repeated taps from refreshing the parry.
    float GuardBreakTimer = 0.f;
    bool bParryActiveThisFrame = false; // Preserve the last valid parry frame.
    static constexpr float ParryWindow = 0.22f;
    static constexpr float ParryRecovery = 0.45f;
    static constexpr float GuardBreakDuration = 1.1f;
    // A precision parry empowers one landed melee counter; never persists in saves.
    bool bPrecisionRiposteReady = false;
    float CounterWindow = 0.f;
    int32 BossAttackCount = 0;
    int32 BossAttackPhase = 1;
    float BossWarningRadius = 250.f;
    float BossPosture = 0.f;
    float BossPostureIdleTimer = 0.f;
    float BossStaggerTimer = 0.f;
    static constexpr float BossPostureThreshold = 30.f;
    void ApplyBossPosture(float Damage, bool bRiposte, bool bFinisher, bool bMagic);
    static constexpr float CounterDuration = 1.5f;
    static constexpr float CounterMultiplier = 1.75f;
    int32 LastCheckpointRegion = 0; // Commit living progress on entering a new region.
    float CheckpointTimer = 0.f;
    static constexpr float CheckpointInterval = 16.f;
    UPROPERTY() UBillboardComponent* BossPortrait;
    UPROPERTY() UTexture2D* ZArtwork;
    UPROPERTY() UTexture2D* NyraArtwork;
    UPROPERTY() USpringArmComponent* CameraBoom;
    UPROPERTY() UCameraComponent* Camera;
    float Forward = 0.f, Right = 0.f, Health = 100.f, BossHealth = 100.f;
    float Mana = 180.f, Stamina = 150.f;
    float StaminaRegenDelay = 0.f; // Prevent instant recovery after committing to a dodge.
    int32 MeleeComboCount = 0;
    float MeleeComboTimer = 0.f; // Chain three landed clicks within a short window.
    float AttackCooldown = 0.f, DodgeTimer = 0.f, DodgeCooldown = 0.f, CastCooldown = 3.f, CastTimer = 0.f;
    float BoltCooldown = 0.f, HealCooldown = 0.f;
    float ZoomDistance = 1600.f;
    int32 Victories = 0;
    FString BossLoot = TEXT("No boss loot yet");
    TArray<FString> BossInventory;
    FMoonfallProgression Progression;
    bool bCasting = false, bWon = false;
    bool bRespawnRequested = false; // Ignore repeated clicks while the level reloads.
    FVector TargetWorld = FVector::ZeroVector;
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Zoom(float Value);
    void Attack();
    void Dodge();
    void CastBolt();
    void CastHeal();
    void Victory();
    void BuildWorld();
    UBillboardComponent* AddWorldArtwork(USceneComponent* Parent, const FString& AssetPath,
        const FVector& LocalPosition, float Scale);
    int32 NearestEnemyIndex(float Radius) const;
    void DamageWorldEnemy(int32 Index, float Damage, bool bRiposte = false, bool bComboFinisher = false, bool bMagic = false);
    void ApplyPostureDamage(FWorldEnemy& Enemy, float Amount);
    static float PostureThreshold(const FWorldEnemy& Enemy) { return Enemy.bGuardian ? 95.f : 60.f; }
    int32 GuardianBlessingCount() const;
    float BasicAttackDamage() const;
    float ArcaneBoltDamage() const;
    float HealingSpellAmount() const;
    void SaveProgress();
    void CollectNearbyChambers();
    void UpdateWorldEnemies(float Dt);
};
