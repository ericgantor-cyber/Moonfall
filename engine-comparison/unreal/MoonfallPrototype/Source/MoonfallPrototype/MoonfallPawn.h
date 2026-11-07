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
    int32 GetQuestDefeatedBeasts() const;
    int32 GetQuestClearedChambers() const;
    TArray<FVector> GetEnemyMapPositions() const;
    TArray<FVector> GetChamberMapPositions() const;
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
        bool bStaggeredThisFrame = false; // Keep contact disabled on the final stagger frame.
        int32 Region = 0;
        int32 Id = 0;
        bool bGuardian = false;
        bool bEnraged = false; // Final 35% guardian combat phase.
        bool bReturningHome = false; // Leash hysteresis prevents edge-of-range jitter.
        FVector Home = FVector::ZeroVector;
    };
    TArray<FWorldEnemy> Enemies;
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
    float EnemyDamageCooldown = 0.f;
    float CounterWindow = 0.f;
    int32 BossAttackCount = 0;
    int32 BossAttackPhase = 1;
    float BossWarningRadius = 250.f;
    static constexpr float CounterDuration = 1.5f;
    static constexpr float CounterMultiplier = 1.75f;
    float CheckpointTimer = 0.f;
    static constexpr float CheckpointInterval = 8.f;
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
    void DamageWorldEnemy(int32 Index, float Damage, bool bRiposte = false, bool bComboFinisher = false);
    int32 GuardianBlessingCount() const;
    float BasicAttackDamage() const;
    float ArcaneBoltDamage() const;
    float HealingSpellAmount() const;
    void SaveProgress();
    void CollectNearbyChambers();
    void UpdateWorldEnemies(float Dt);
};
