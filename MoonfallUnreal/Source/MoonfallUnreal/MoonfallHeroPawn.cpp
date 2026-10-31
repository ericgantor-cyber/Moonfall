#include "MoonfallHeroPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
AMoonfallHeroPawn::AMoonfallHeroPawn()
{
 PrimaryActorTick.bCanEverTick = false;
 CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
 CameraBoom->SetupAttachment(GetRootComponent());
 CameraBoom->TargetArmLength = 450.f;
 CameraBoom->bUsePawnControlRotation = true;
 FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
 FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
 FollowCamera->bUsePawnControlRotation = false;
 bUseControllerRotationYaw = false;
 GetCharacterMovement()->bOrientRotationToMovement = true;
 GetCharacterMovement()->JumpZVelocity = 600.f;
}
void AMoonfallHeroPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
 Super::SetupPlayerInputComponent(Input);
 Input->BindAxis(TEXT("MoveForward"), this, &AMoonfallHeroPawn::MoveForward);
 Input->BindAxis(TEXT("MoveRight"), this, &AMoonfallHeroPawn::MoveRight);
 Input->BindAxis(TEXT("Turn"), this, &AMoonfallHeroPawn::Turn);
 Input->BindAxis(TEXT("LookUp"), this, &AMoonfallHeroPawn::LookUp);
 Input->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
 Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
 Input->BindAction(TEXT("CastLeft"), IE_Pressed, this, &AMoonfallHeroPawn::CastLeft);
 Input->BindAction(TEXT("CastRight"), IE_Pressed, this, &AMoonfallHeroPawn::CastRight);
 Input->BindAction(TEXT("DualCast"), IE_Pressed, this, &AMoonfallHeroPawn::CastBoth);
 Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AMoonfallHeroPawn::Dodge);
}
void AMoonfallHeroPawn::MoveForward(float Value)
{
 if (Controller && !FMath::IsNearlyZero(Value))
 {
  const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
 }
}
void AMoonfallHeroPawn::MoveRight(float Value)
{
 if (Controller && !FMath::IsNearlyZero(Value))
 {
  const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value);
 }
}
void AMoonfallHeroPawn::Turn(float Value) { AddControllerYawInput(Value); }
void AMoonfallHeroPawn::LookUp(float Value) { AddControllerPitchInput(Value); }
bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier)
{
 static const float Costs[6] = {16.f,13.f,22.f,25.f,18.f,14.f};
 static const float Cooldowns[6] = {0.8f,0.65f,1.2f,2.f,1.f,0.9f};
 if (!GetWorld() || School < 0 || School >= 6 || Hand < 0 || Hand > 1) return false;
 const float Time = GetWorld()->GetTimeSeconds();
 if (Mana < Costs[School] * Multiplier || Time - LastCastTime[Hand] < Cooldowns[School]) return false;
 if (School == 3 && Health >= 100.f) return false;
 Mana -= Costs[School] * Multiplier;
 LastCastTime[Hand] = Time;
 if (School == 3) Health = FMath::Min(100.f, Health + 24.f * Multiplier);
 OnSpellCast(School, Hand == 0);
 return true;
}
void AMoonfallHeroPawn::CastLeft() { TryCast(LeftSpell, 0, 1.f); }
void AMoonfallHeroPawn::CastRight() { TryCast(RightSpell, 1, 1.f); }
void AMoonfallHeroPawn::CastBoth()
{
 if (LeftSpell != RightSpell || !GetWorld()) return;
 const float CostBefore = Mana;
 if (TryCast(LeftSpell, 0, 2.f))
 {
  LastCastTime[1] = LastCastTime[0];
  OnSpellCast(LeftSpell, false);
 }
}
void AMoonfallHeroPawn::Dodge()
{
 LaunchCharacter(GetActorForwardVector() * 600.f, true, false);
}
