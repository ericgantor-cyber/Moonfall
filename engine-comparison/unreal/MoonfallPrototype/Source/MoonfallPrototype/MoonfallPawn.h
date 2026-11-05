#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MoonfallPawn.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class MOONFALLPROTOTYPE_API AMoonfallPawn : public APawn
{
    GENERATED_BODY()
public:
    AMoonfallPawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
private:
    UPROPERTY() USceneComponent* Root;
    UPROPERTY() UStaticMeshComponent* HeroMesh;
    UPROPERTY() UStaticMeshComponent* BossMesh;
    UPROPERTY() UStaticMeshComponent* WarningMesh;
    UPROPERTY() UStaticMeshComponent* FloorMesh;
    UPROPERTY() USpringArmComponent* CameraBoom;
    UPROPERTY() UCameraComponent* Camera;
    float Forward = 0.f, Right = 0.f, Health = 100.f, BossHealth = 100.f;
    float AttackCooldown = 0.f, DodgeTimer = 0.f, DodgeCooldown = 0.f, CastCooldown = 3.f, CastTimer = 0.f;
    float ZoomDistance = 1600.f;
    int32 Victories = 0;
    FString BossLoot = TEXT("No boss loot yet");
    TArray<FString> BossInventory;
    bool bCasting = false, bWon = false;
    FVector TargetWorld = FVector::ZeroVector;
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Zoom(float Value);
    void Attack();
    void Dodge();
    void Victory();
};
