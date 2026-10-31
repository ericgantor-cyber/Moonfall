#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MoonfallHeroPawn.generated.h"
class USpringArmComponent;
class UCameraComponent;
UCLASS()
class MOONFALLUNREAL_API AMoonfallHeroPawn : public ACharacter
{
 GENERATED_BODY()
public:
 AMoonfallHeroPawn();
 virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
 virtual void BeginPlay() override;
 UFUNCTION(BlueprintCallable, Category="Moonfall|Combat") void ApplyMoonfallDamage(float Amount);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnHealthChanged(float NewHealth, float MaxHealth);
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") bool bOptionsOpen = false;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") int32 OptionsSelection = 0;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") int32 FrameGenerationPreference = 0;
 UPROPERTY(BlueprintReadOnly, Category="Moonfall|Options") bool bFrameGenerationAvailable = false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") float Mana = 100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") float MaxMana = 100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") float Health = 100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") int32 LeftSpell = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Combat") int32 RightSpell = 1;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Combat") void OnSpellCast(int32 School, bool bLeftHand);
private:
 UPROPERTY(VisibleAnywhere) USpringArmComponent* CameraBoom;
 UPROPERTY(VisibleAnywhere) UCameraComponent* FollowCamera;
 float LastCastTime[2] = {-100.f,-100.f};
 void MoveForward(float Amount);
 void MoveRight(float Amount);
 void Turn(float Amount);
 void LookUp(float Amount);
 void CastLeft();
 void CastRight();
 void CastBoth();
 void Dodge();
 void OpenOptions();
 void OptionsUp();
 void OptionsDown();
 void OptionsLeft();
 void OptionsRight();
 void AdjustOption(int32 Direction);
 void CycleGraphicsQuality();
 void ToggleDisplayMode();
 bool TryCast(int32 School, int32 Hand, float Multiplier);
};