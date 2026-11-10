#include "MoonfallHeroPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Interfaces/IPluginManager.h"
#include "MoonfallEnemy.h"
#include "MoonfallGroundLoot.h"
#include "MoonfallCombatHUD.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Math/RotationMatrix.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CollisionQueryParams.h"
// Equipment stats are intentionally derived from equipped codes, never the
// inventory list: carrying loot cannot accidentally grant combat bonuses.
// All combat bonuses share one strict decoder. Invalid or wrong-slot codes
// must not grant rarity, tier, affix or set bonuses.
namespace {
bool DecodeEquippedItem(int32 Code, int32 ExpectedSlot, int32& OutTrait, int32& OutRarity, int32& OutTier)
{
 if (Code <= 0 || ExpectedSlot < 0 || ExpectedSlot > 2) return false;
 const int32 Trait = Code / 100000;
 const int32 Slot = (Code / 10000) % 10;
 const int32 Rarity = (Code / 1000) % 10;
 const int32 Tier = Code % 1000;
 // Match RollEquipmentCode/EquipInventoryItem: common gear and some
 // uncommon gear legitimately have no trait (0). Common gear has rarity 0.
 if (Trait < 0 || Trait > 3 || Slot != ExpectedSlot ||
     Rarity < 0 || Rarity > 5 || Tier < 1 || Tier > 25 ||
     (Rarity == 0 && Trait != 0) ||
     Code != Trait * 100000 + Slot * 10000 + Rarity * 1000 + Tier) return false;
 OutTrait = Trait;
 OutRarity = Rarity;
 OutTier = Tier;
 return true;
}
int32 ValidEquippedTier(int32 Code, int32 Slot)
{
 int32 Trait = 0, Rarity = 0, Tier = 0;
 return DecodeEquippedItem(Code, Slot, Trait, Rarity, Tier) ? Tier : 0;
}
float ValidEquippedRarityBonus(int32 Code, int32 Slot, int32 ExpectedTrait, float PerRarity)
{
 int32 Trait = 0, Rarity = 0, Tier = 0;
 return DecodeEquippedItem(Code, Slot, Trait, Rarity, Tier) && Trait == ExpectedTrait
     ? Rarity * PerRarity : 0.f;
}
}
int32 AMoonfallHeroPawn::GetSetPieceCount(int32 Affix) const
{
 if (Affix < 1 || Affix > 3) return 0;
 const int32 Codes[3] = {EquippedWeaponCode, EquippedArmorCode, EquippedAccessoryCode};
 int32 Pieces = 0;
 for (int32 Slot = 0; Slot < 3; ++Slot) {
  int32 Trait = 0, Rarity = 0, Tier = 0;
  if (DecodeEquippedItem(Codes[Slot], Slot, Trait, Rarity, Tier) && Trait == Affix)
   ++Pieces;
 }
 return Pieces;
}

int32 AMoonfallHeroPawn::GetMight() const
{
 return FMath::Clamp(10 + FMath::Max(0, Level - 1) / 2 +
   ValidEquippedTier(EquippedWeaponCode, 0), 10, 100);
}

int32 AMoonfallHeroPawn::GetInsight() const
{
 return FMath::Clamp(10 + FMath::Max(0, Level - 1) / 2 +
   ValidEquippedTier(EquippedAccessoryCode, 2), 10, 100);
}

int32 AMoonfallHeroPawn::GetResolve() const
{
 return FMath::Clamp(10 + FMath::Max(0, Level - 1) / 2 +
   ValidEquippedTier(EquippedArmorCode, 1), 10, 100);
}

float AMoonfallHeroPawn::GetCriticalChance() const
{
 // Force: 2-piece improves critical chances. A 3-piece grants mastery.
 const float TraitCrit = ValidEquippedRarityBonus(EquippedWeaponCode, 0, 1, 0.008f);
 return FMath::Clamp(0.05f + TraitCrit + FMath::Clamp(TalentForceRank, 0, 10) * 0.005f
   + (bTalentForceKeystone ? 0.02f : 0.f) + (bTalentBattleWeave ? 0.01f : 0.f)
   + (GetSetPieceCount(1) >= 2 ? 0.03f : 0.f),
   0.05f, 0.25f);
}

float AMoonfallHeroPawn::GetHasteBonus() const
{
 // Focus: faster spell recovery at two matching items.
 const float TraitHaste = ValidEquippedRarityBonus(EquippedAccessoryCode, 2, 3, 0.006f);
 return FMath::Clamp(TraitHaste + FMath::Clamp(TalentFocusRank, 0, 10) * 0.01f
   + (bTalentFocusKeystone ? 0.02f : 0.f) + (bTalentArcaneAegis ? 0.01f : 0.f)
   + (GetSetPieceCount(3) >= 2 ? 0.04f : 0.f),
   0.f, 0.20f);
}

float AMoonfallHeroPawn::GetMasteryBonus() const
{
 // Mastery rewards overall valid equipped power; triple Force strengthens spells.
 const float GearMastery = FMath::Clamp((
   ValidEquippedTier(EquippedWeaponCode, 0) +
   ValidEquippedTier(EquippedArmorCode, 1) +
   ValidEquippedTier(EquippedAccessoryCode, 2)) * 0.0015f, 0.f, 0.12f);
 return FMath::Clamp(GearMastery + (GetSetPieceCount(1) >= 3 ? 0.04f : 0.f),
   0.f, 0.16f);
}

float AMoonfallHeroPawn::GetVersatilityBonus() const
{
 // Ward: Versatility increases damage dealt and halves its defensive effect.
 const float TraitVersatility = ValidEquippedRarityBonus(EquippedArmorCode, 1, 2, 0.005f);
 return FMath::Clamp(TraitVersatility + FMath::Clamp(TalentWardRank, 0, 10) * 0.008f
   + (bTalentWardKeystone ? 0.015f : 0.f) + (bTalentIronWill ? 0.015f : 0.f)
   + (GetSetPieceCount(2) >= 2 ? 0.03f : 0.f),
   0.f, 0.12f);
}

AMoonfallHeroPawn::AMoonfallHeroPawn()
{
 PrimaryActorTick.bCanEverTick = true;
 CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
 CameraBoom->SetupAttachment(GetRootComponent());
 CameraBoom->TargetArmLength = CameraDistance;
 CameraBoom->bUsePawnControlRotation = true;
 FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
 FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
 FollowCamera->bUsePawnControlRotation = false;
 bUseControllerRotationYaw = false;
 GetCharacterMovement()->bOrientRotationToMovement = true;
 GetCharacterMovement()->JumpZVelocity = 600.f;
 NormalWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
 // Native geometry makes five distinct finishing sequences available even to
 // characters without a skeletal mesh or an authored animation asset.
 FinisherSlashA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FinisherSlashA"));
 FinisherSlashA->SetupAttachment(GetRootComponent());
 FinisherSlashB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FinisherSlashB"));
 FinisherSlashB->SetupAttachment(GetRootComponent());
 static ConstructorHelpers::FObjectFinder<UStaticMesh> SlashMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
 for (UStaticMeshComponent* Slash : {FinisherSlashA, FinisherSlashB}) {
  if (SlashMesh.Succeeded()) Slash->SetStaticMesh(SlashMesh.Object);
  Slash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Slash->SetCastShadow(false);
  Slash->SetHiddenInGame(true);
 }
 FinisherFlash = CreateDefaultSubobject<UPointLightComponent>(TEXT("FinisherFlash"));
 FinisherFlash->SetupAttachment(GetRootComponent());
 FinisherFlash->SetIntensity(0.f);
 FinisherFlash->SetAttenuationRadius(420.f);
 FinisherFlash->SetVisibility(false);
}
void AMoonfallHeroPawn::BeginPlay() {
 Super::BeginPlay();
 // Blueprint defaults are applied after the native constructor. Capture the
 // actual authored walk speed before sprint/guard begin changing MaxWalkSpeed.
 if (GetCharacterMovement()) {
  const float AuthoredWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
  NormalWalkSpeed = FMath::IsFinite(AuthoredWalkSpeed) && AuthoredWalkSpeed > 0.f
      ? AuthoredWalkSpeed : 600.f;
  GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
 }
 int32 SavedPreference = 0;
 if (GConfig) GConfig->GetInt(TEXT("Moonfall.Graphics"), TEXT("FrameGenerationPreference"), SavedPreference, GGameUserSettingsIni);
 FrameGenerationPreference = FMath::Clamp(SavedPreference, 0, 2);
 if (GConfig) GConfig->GetBool(TEXT("Moonfall.Accessibility"), TEXT("ReduceMotion"), bReduceMotion, GGameUserSettingsIni);
 if (GConfig) GConfig->GetFloat(TEXT("Moonfall.Accessibility"), TEXT("CombatTextScale"), CombatTextScale, GGameUserSettingsIni);
 CombatTextScale = FMath::Clamp(FMath::RoundToFloat(CombatTextScale * 4.f) / 4.f, 0.75f, 2.f);
 if (GConfig) GConfig->GetFloat(TEXT("Moonfall.Accessibility"), TEXT("BossWarningScale"), BossWarningScale, GGameUserSettingsIni);
 BossWarningScale = FMath::Clamp(FMath::RoundToFloat(BossWarningScale * 4.f) / 4.f, 1.f, 2.f);
 // No FG vendor plugin or runtime capability integration has been added.
 bFrameGenerationAvailable = false;
 const TSharedPtr<IPlugin> FSR = IPluginManager::Get().FindPlugin(TEXT("FSR"));
 const TSharedPtr<IPlugin> DLSS = IPluginManager::Get().FindPlugin(TEXT("DLSS"));
 bFrameGenerationPluginDetected = (FSR.IsValid() && FSR->IsEnabled()) || (DLSS.IsValid() && DLSS->IsEnabled());
 // Detection is only a hint, never proof of a supported frame-generation runtime.
}
void AMoonfallHeroPawn::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 UpdateNativeFinisher(DeltaSeconds);
 // Expire the visible combo stage even if the player stops attacking.
 // Also discard defeated or destroyed targets to avoid a stale combo HUD.
 if (HitComboStage > 0 && GetWorld()) {
  const float ComboNow = GetWorld()->GetTimeSeconds();
  const float ComboWindow = FMath::IsFinite(HitComboWindowSeconds)
      ? FMath::Clamp(HitComboWindowSeconds, 0.5f, 4.f) : 2.f;
  const AMoonfallEnemy* ComboTarget = LastHitComboTarget.Get();
  if (Health <= 0.f || bOptionsOpen || bInventoryOpen || !IsValid(ComboTarget)
       || !FMath::IsFinite(ComboTarget->Health) || ComboTarget->Health <= 0.f
       || !FMath::IsFinite(ComboNow) || !FMath::IsFinite(LastSuccessfulHitAt)
      || ComboNow < LastSuccessfulHitAt || ComboNow - LastSuccessfulHitAt > ComboWindow) {
   HitComboStage = 0;
   LastHitComboTarget.Reset();
   LastSuccessfulHitAt = -100.f;
  }
 }
 // Bounded camera impulse, no camera shake object or spawned effects.
 // Never allow invalid frame time or camera impulse to contaminate the boom.
 if (!FMath::IsFinite(ImpactCameraOffset)) ImpactCameraOffset = 0.f;
 const float SafeCameraDelta = FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.1f) : 0.f;
 ImpactCameraOffset = FMath::Clamp(FMath::FInterpTo(FMath::Clamp(ImpactCameraOffset, 0.f, 24.f), 0.f, SafeCameraDelta, 12.f), 0.f, 24.f);
 if (CameraBoom) {
  const float SafeDistance = FMath::IsFinite(CameraDistance) ? FMath::Clamp(CameraDistance, 250.f, 850.f) : 450.f;
  const float TargetCameraDistance = FMath::Clamp(SafeDistance + (bReduceMotion ? 0.f : ImpactCameraOffset), 250.f, 874.f);
  const float CurrentCameraDistance = FMath::IsFinite(CameraBoom->TargetArmLength)
      ? FMath::Clamp(CameraBoom->TargetArmLength, 250.f, 874.f) : SafeDistance;
  CameraBoom->TargetArmLength = FMath::FInterpTo(CurrentCameraDistance, TargetCameraDistance, SafeCameraDelta, 10.f);
 }
 // Mana regenerates during gameplay and while a menu is open.
 const float SafeRecoveryDelta = FMath::IsFinite(DeltaSeconds) ? FMath::Clamp(DeltaSeconds, 0.f, 0.1f) : 0.f;
 const float MenuRecoveryBoost = (bOptionsOpen || bInventoryOpen) && FMath::IsFinite(MenuRecoveryMultiplier)
     ? FMath::Clamp(MenuRecoveryMultiplier, 1.f, 2.f) : 1.f;
 // WoW-style pacing: combat mana regenerates more slowly; resting restores full pace.
 const bool bInCombatRecoveryWindow = GetWorld() && GetWorld()->GetTimeSeconds() < CombatRecoveryEndsAt;
 const float CombatManaFactor = bInCombatRecoveryWindow
     ? (FMath::IsFinite(CombatManaRegenMultiplier)
         ? FMath::Clamp(CombatManaRegenMultiplier, 0.f, 1.f) : 0.5f) : 1.f;
 if (Health > 0.f && FMath::IsFinite(Mana) && FMath::IsFinite(MaxMana) && MaxMana > 0.f
     && FMath::IsFinite(ManaRegenPerSecond) && ManaRegenPerSecond > 0.f && Mana >= 0.f && Mana < MaxMana)
  Mana = FMath::Min(MaxMana, Mana + FMath::Clamp(ManaRegenPerSecond, 0.f, 250.f) * MenuRecoveryBoost * CombatManaFactor * (bTalentArcaneAegis ? 1.15f : 1.f)
      * (1.f + FMath::Clamp(AccessoryTier * 0.012f, 0.f, 0.25f) + (AccessoryAffix == 3 ? 0.08f : 0.f)
        + FMath::Clamp(((EquippedAccessoryCode / 1000) % 10) * 0.02f, 0.f, 0.10f)
        + (GetSetPieceCount(3) >= 3 ? 0.06f : 0.f)
        + FMath::Clamp(GetInsight() * 0.002f, 0.f, 0.20f))
      * SafeRecoveryDelta);
 // Recovery in either menu is bounded and never revives a defeated hero.
 if ((bOptionsOpen || bInventoryOpen) && Health > 0.f && FMath::IsFinite(Health)
     && FMath::IsFinite(MaxHealth) && MaxHealth > 0.f && Health < MaxHealth
     && FMath::IsFinite(MenuHealthRegenPerSecond) && MenuHealthRegenPerSecond > 0.f
     && GetWorld() && GetWorld()->GetTimeSeconds() >= MenuHealthRegenStartsAt
     && !bInCombatRecoveryWindow) {
  const float BeforeRegen = Health;
  Health = FMath::Min(MaxHealth, Health + FMath::Clamp(MenuHealthRegenPerSecond, 0.f, 20.f)
      * SafeRecoveryDelta);
  if (Health > BeforeRegen) OnHealthChanged(Health, MaxHealth);
 }
 // Sprint drains only while moving, and never drains while paused in Options.
 const float StaminaDelta = SafeRecoveryDelta;
 // Exhaustion requires releasing sprint before another burst; guard and dodge
 // immediately cancel sprint speed so defense cannot inherit a free boost.
 if (!FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina) || MaxStamina <= 0.f) {
  // Invalid runtime/Blueprint values should not spread through movement or HUD.
  bSprintExhausted = true;
 } else {
  Stamina = FMath::Clamp(Stamina, 0.f, MaxStamina);
  if (bSprintHeld && Stamina <= 0.f) bSprintExhausted = true;
 }
 const bool bCanSprint = bSprintHeld && !bSprintExhausted && !bGuarding && !bOptionsOpen && !bInventoryOpen && Health > 0.f
     && FMath::IsFinite(Stamina) && FMath::IsFinite(MaxStamina) && MaxStamina > 0.f
     && FMath::IsFinite(SprintStaminaDrainPerSecond) && SprintStaminaDrainPerSecond > 0.f
     && FMath::IsFinite(SprintSpeedMultiplier) && SprintSpeedMultiplier > 1.f
     && GetVelocity().SizeSquared2D() > FMath::Square(10.f)
     && GetLastMovementInputVector().SizeSquared2D() > 0.01f
     && GetWorld() && GetWorld()->GetTimeSeconds() >= DamageImmunityUntil;
 if (bCanSprint && Stamina > 0.f) {
  // Cap runtime tuning so a large Blueprint value cannot overflow stamina drain.
  Stamina = FMath::Max(0.f, Stamina - FMath::Clamp(SprintStaminaDrainPerSecond, 0.f, 250.f) * StaminaDelta);
  // Sprint must not shorten a longer dodge/guard cooldown.
  if (GetWorld()) {
   const float SprintRecoveryAt = GetWorld()->GetTimeSeconds() + 0.8f;
   StaminaRegenStartsAt = FMath::Max(FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : SprintRecoveryAt, SprintRecoveryAt);
  }
  if (Stamina <= 0.f) bSprintExhausted = true;
 }
 const bool bSprinting = bCanSprint && !bSprintExhausted;
 // Invalid Blueprint resource tuning cannot leave the hero guarding forever.
 // BreakGuard also restores movement speed and clears the counterattack window.
 if (bGuarding && !bOptionsOpen && Health > 0.f && GetWorld()
     && (!FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina)
         || MaxStamina <= 0.f || !FMath::IsFinite(GuardStaminaCost)
         || GuardStaminaCost < 0.f)) {
  BreakGuard();
 }
 // Sustained guarding is a deliberate defensive commitment. Drain only on
 // playable frames and never allow invalid Blueprint values to corrupt stamina.
 if (bGuarding && !bOptionsOpen && Health > 0.f && GetWorld()
     && FMath::IsFinite(Stamina) && FMath::IsFinite(MaxStamina) && MaxStamina > 0.f
     && FMath::IsFinite(GuardStaminaCost) && GuardStaminaCost >= 0.f) {
  const float GuardDrain = FMath::IsFinite(GuardStaminaDrainPerSecond)
    ? FMath::Clamp(GuardStaminaDrainPerSecond, 0.f, 30.f) : 0.f;
  Stamina = FMath::Clamp(Stamina - GuardDrain * StaminaDelta, 0.f, MaxStamina);
  if (GuardDrain > 0.f) StaminaRegenStartsAt = FMath::Max(
      FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : GetWorld()->GetTimeSeconds(),
      GetWorld()->GetTimeSeconds() + 0.8f);
  // Guard cannot continue in a state where even a base block is impossible.
  // Positive-cost guards break at the threshold; zero-cost guards break at zero.
  if (Stamina <= 0.f || (GuardStaminaCost > 0.f && Stamina < GuardStaminaCost))
   BreakGuard();
 }
 if (GetCharacterMovement()) {
  // Ramp into sprint without allowing boosted speed during guard, dodges,
  // menus, death, exhaustion or a released sprint key.
  if (bSprinting) {
   const float TargetSpeed = NormalWalkSpeed * FMath::Clamp(SprintSpeedMultiplier, 1.f, 2.5f);
   GetCharacterMovement()->MaxWalkSpeed = FMath::FInterpTo(
       FMath::Clamp(GetCharacterMovement()->MaxWalkSpeed, NormalWalkSpeed, TargetSpeed),
       TargetSpeed, StaminaDelta, FMath::Clamp(SprintAccelerationRate, 1.f, 20.f));
  } else if (bGuarding && !bOptionsOpen && Health > 0.f) {
   // Guarding is deliberate, not a full-speed retreat. Invalid tuning uses
   // normal speed instead of passing nonfinite values to CharacterMovement.
   const float GuardMultiplier = FMath::IsFinite(GuardMoveSpeedMultiplier)
       ? FMath::Clamp(GuardMoveSpeedMultiplier, 0.2f, 1.f) : 1.f;
   GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed * GuardMultiplier;
  } else {
   GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
  }
 }
 // Stamina powers dodges independently of spell mana and regenerates after exertion.
 if (Health > 0.f && !bSprinting && !bGuarding && GetWorld() && GetWorld()->GetTimeSeconds() >= StaminaRegenStartsAt
     && FMath::IsFinite(Stamina) && FMath::IsFinite(MaxStamina) && MaxStamina > 0.f
     && FMath::IsFinite(StaminaRegenPerSecond) && StaminaRegenPerSecond > 0.f) {
  Stamina = FMath::Clamp(Stamina + FMath::Clamp(StaminaRegenPerSecond, 0.f, 250.f) * StaminaDelta * MenuRecoveryBoost * (bTalentIronWill ? 1.20f : 1.f)
    * (1.f + FMath::Clamp(GetResolve() * 0.003f, 0.f, 0.30f)), 0.f, MaxStamina);
 }
}
void AMoonfallHeroPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
 Super::SetupPlayerInputComponent(Input);
 Input->BindAxis(TEXT("MoveForward"), this, &AMoonfallHeroPawn::MoveForward);
 Input->BindAxis(TEXT("MoveRight"), this, &AMoonfallHeroPawn::MoveRight);
 Input->BindAxis(TEXT("Turn"), this, &AMoonfallHeroPawn::Turn);
 Input->BindAxis(TEXT("LookUp"), this, &AMoonfallHeroPawn::LookUp);
 Input->BindAction(TEXT("ZoomIn"), IE_Pressed, this, &AMoonfallHeroPawn::ZoomCameraIn);
 Input->BindAction(TEXT("ZoomOut"), IE_Pressed, this, &AMoonfallHeroPawn::ZoomCameraOut);
 Input->BindAction(TEXT("Jump"), IE_Pressed, this, &AMoonfallHeroPawn::TryJump);
 Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
 Input->BindAction(TEXT("CastLeft"), IE_Pressed, this, &AMoonfallHeroPawn::CastLeft);
 Input->BindAction(TEXT("LeftClickContext"), IE_Pressed, this, &AMoonfallHeroPawn::LeftClickContextAction);
 Input->BindAction(TEXT("LeftClickContext"), IE_Released, this, &AMoonfallHeroPawn::ReleaseLeftClickContext);
 Input->BindAction(TEXT("CastRight"), IE_Pressed, this, &AMoonfallHeroPawn::CastRight);
 Input->BindAction(TEXT("RightClickContext"), IE_Pressed, this, &AMoonfallHeroPawn::RightClickContextAction);
 Input->BindAction(TEXT("RightClickContext"), IE_Released, this, &AMoonfallHeroPawn::ReleaseRightClickContext);
 Input->BindAction(TEXT("DualCast"), IE_Pressed, this, &AMoonfallHeroPawn::CastBoth);
 Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AMoonfallHeroPawn::Dodge);
 Input->BindAction(TEXT("Sprint"), IE_Pressed, this, &AMoonfallHeroPawn::BeginSprint);
 Input->BindAction(TEXT("Sprint"), IE_Released, this, &AMoonfallHeroPawn::EndSprint);
 Input->BindAction(TEXT("Guard"), IE_Pressed, this, &AMoonfallHeroPawn::BeginGuard);
 Input->BindAction(TEXT("Guard"), IE_Released, this, &AMoonfallHeroPawn::EndGuard);
 Input->BindAction(TEXT("GraphicsQuality"), IE_Pressed, this, &AMoonfallHeroPawn::CycleGraphicsQuality);
 Input->BindAction(TEXT("DisplayMode"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleDisplayMode);
 Input->BindAction(TEXT("Options"), IE_Pressed, this, &AMoonfallHeroPawn::OpenOptions);
 Input->BindAction(TEXT("Inventory"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleInventory);
 Input->BindAction(TEXT("CharacterStats"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleCharacterStats);
 Input->BindAction(TEXT("InventoryPage"), IE_Pressed, this, &AMoonfallHeroPawn::SwitchInventoryPage);
 Input->BindAction(TEXT("InventorySort"), IE_Pressed, this, &AMoonfallHeroPawn::CycleInventorySort);
 Input->BindAction(TEXT("InventorySalvage"), IE_Pressed, this, &AMoonfallHeroPawn::InventorySalvage);
 Input->BindAction(TEXT("UpgradeGear"), IE_Pressed, this, &AMoonfallHeroPawn::UpgradeSelectedInventoryItem);
 Input->BindAction(TEXT("ForgeFive"), IE_Pressed, this, &AMoonfallHeroPawn::ForgeFiveSelectedInventoryItem);
 Input->BindAction(TEXT("AscendGear"), IE_Pressed, this, &AMoonfallHeroPawn::AscendSelectedInventoryItem);
 Input->BindAction(TEXT("ReforgeGear"), IE_Pressed, this, &AMoonfallHeroPawn::ReforgeSelectedInventoryItem);
 Input->BindAction(TEXT("EquipBest"), IE_Pressed, this, &AMoonfallHeroPawn::EquipBestInventoryAction);
 Input->BindAction(TEXT("Atlas"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleAtlas);
 Input->BindAction(TEXT("Talents"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleTalents);
 Input->BindAction(TEXT("TalentForce"), IE_Pressed, this, &AMoonfallHeroPawn::SpendForceTalent);
 Input->BindAction(TEXT("TalentFocus"), IE_Pressed, this, &AMoonfallHeroPawn::SpendFocusTalent);
 Input->BindAction(TEXT("TalentWard"), IE_Pressed, this, &AMoonfallHeroPawn::SpendWardTalent);
 Input->BindAction(TEXT("TalentForceKeystone"), IE_Pressed, this, &AMoonfallHeroPawn::UnlockForceKeystone);
 Input->BindAction(TEXT("TalentFocusKeystone"), IE_Pressed, this, &AMoonfallHeroPawn::UnlockFocusKeystone);
 Input->BindAction(TEXT("TalentWardKeystone"), IE_Pressed, this, &AMoonfallHeroPawn::UnlockWardKeystone);
 Input->BindAction(TEXT("TalentBattleWeave"), IE_Pressed, this, &AMoonfallHeroPawn::UnlockBattleWeave);
 Input->BindAction(TEXT("TalentArcaneAegis"), IE_Pressed, this, &AMoonfallHeroPawn::UnlockArcaneAegis);
 Input->BindAction(TEXT("TalentIronWill"), IE_Pressed, this, &AMoonfallHeroPawn::UnlockIronWill);
 Input->BindAction(TEXT("TalentRespec"), IE_Pressed, this, &AMoonfallHeroPawn::RespecTalentAction);
 Input->BindAction(TEXT("TalentNext"), IE_Pressed, this, &AMoonfallHeroPawn::SelectNextTalentNode);
 Input->BindAction(TEXT("TalentPrevious"), IE_Pressed, this, &AMoonfallHeroPawn::SelectPreviousTalentNode);
 Input->BindAction(TEXT("TalentActivate"), IE_Pressed, this, &AMoonfallHeroPawn::ActivateSelectedTalentNode);
 Input->BindAction(TEXT("PickupLoot"), IE_Pressed, this, &AMoonfallHeroPawn::PickupNearestGroundLoot);
 Input->BindAction(TEXT("OptionsUp"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsUp);
 Input->BindAction(TEXT("OptionsDown"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsDown);
 Input->BindAction(TEXT("OptionsLeft"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsLeft);
 Input->BindAction(TEXT("OptionsRight"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsRight);
}
void AMoonfallHeroPawn::ZoomCameraIn() {
 if (bTalentsOpen && bAtlasOpen && !bOptionsOpen && Health > 0.f) {
  TalentZoom = FMath::Clamp((FMath::IsFinite(TalentZoom) ? TalentZoom : 1.f) + 0.1f,0.75f,1.35f);
  return;
 }
 if (!FMath::IsFinite(Health) || Health <= 0.f || bOptionsOpen || bAtlasOpen) return;
 if (bInventoryOpen) {
  // WoW-style bag viewport scroll: one row per notch, no selection changes.
  // Clamp to the last *full visible viewport*, with no dead wheel ticks.
  int32 VisibleRows = 2;
  if (const APlayerController* PC = Cast<APlayerController>(GetController()))
   if (const AMoonfallCombatHUD* HUD = Cast<AMoonfallCombatHUD>(PC->GetHUD()))
    VisibleRows = HUD->GetInventoryVisibleRowCount();
  InventoryViewStart = FMath::Clamp(InventoryViewStart - 1, 0,
    FMath::Max(0, InventoryItemCodes.Num() - VisibleRows));
  return;
 }
 CameraDistance = FMath::Clamp((FMath::IsFinite(CameraDistance) ? CameraDistance : 450.f) - 65.f, 250.f, 850.f);
}
void AMoonfallHeroPawn::ZoomCameraOut() {
 if (bTalentsOpen && bAtlasOpen && !bOptionsOpen && Health > 0.f) {
  TalentZoom = FMath::Clamp((FMath::IsFinite(TalentZoom) ? TalentZoom : 1.f) - 0.1f,0.75f,1.35f);
  return;
 }
 if (!FMath::IsFinite(Health) || Health <= 0.f || bOptionsOpen || bAtlasOpen) return;
 if (bInventoryOpen) {
  // Scroll the visible bag list independently of highlighted equipment.
  // Clamp to the last *full visible viewport*, with no dead wheel ticks.
  int32 VisibleRows = 2;
  if (const APlayerController* PC = Cast<APlayerController>(GetController()))
   if (const AMoonfallCombatHUD* HUD = Cast<AMoonfallCombatHUD>(PC->GetHUD()))
    VisibleRows = HUD->GetInventoryVisibleRowCount();
  InventoryViewStart = FMath::Clamp(InventoryViewStart + 1, 0,
    FMath::Max(0, InventoryItemCodes.Num() - VisibleRows));
  return;
 }
 CameraDistance = FMath::Clamp((FMath::IsFinite(CameraDistance) ? CameraDistance : 450.f) + 65.f, 250.f, 850.f);
}
void AMoonfallHeroPawn::TryJump()
{
 // Jump must obey the same defeated/menu input locks as Dodge and Cast.
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen) return;
 Jump();
}
void AMoonfallHeroPawn::MoveForward(float Value)
{
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen || !FMath::IsFinite(Value)) return;
 if (Controller && !FMath::IsNearlyZero(Value))
 {
  const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
 }
}
void AMoonfallHeroPawn::MoveRight(float Value)
{
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen || !FMath::IsFinite(Value)) return;
 if (Controller && !FMath::IsNearlyZero(Value))
 {
  const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value);
 }
}
void AMoonfallHeroPawn::Turn(float Value) { if (Health > 0.f && !bOptionsOpen && !bInventoryOpen && !bAtlasOpen && FMath::IsFinite(Value)) AddControllerYawInput(Value); }
void AMoonfallHeroPawn::LookUp(float Value) { if (Health > 0.f && !bOptionsOpen && !bInventoryOpen && !bAtlasOpen && FMath::IsFinite(Value)) AddControllerPitchInput(Value); }
bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier, bool bOvercharged)
{
 if (bAtlasOpen) return false;
 static const float Costs[6] = {16.f,13.f,22.f,25.f,18.f,14.f};
 static const float Cooldowns[6] = {0.8f,0.65f,1.2f,2.f,1.f,0.9f};
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || !GetWorld() || School < 0 || School >= 6 || Hand < 0 || Hand > 1 || !FMath::IsFinite(Multiplier) || Multiplier <= 0.f || Multiplier > 2.f) return false;
 // Skyrim-inspired two-hand overcharge: the same spell in both hands costs
 // 2.8x mana for 2.2x effect. Single-hand casts retain their previous values.
 // Mind Flow lowers actual casting costs, including dual-hand overcharge.
 // The reduction is applied before checking and spending mana.
 const float ManaCostMultiplier = (bOvercharged ? 2.8f : Multiplier)
   * (bTalentFocusKeystone ? 0.90f : 1.f);
 const float EffectMultiplier = bOvercharged ? 2.2f : Multiplier;
 const float Time = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth) || MaxHealth <= 0.f || Health > MaxHealth || !FMath::IsFinite(MaxMana) || MaxMana <= 0.f || !FMath::IsFinite(Mana) || !FMath::IsFinite(SpellPowerMultiplier) || SpellPowerMultiplier <= 0.f || Mana < Costs[School] * ManaCostMultiplier || Time - LastCastTime[Hand] < Cooldowns[School] / (1.f + GetHasteBonus())) return false;
 if (School == 3 && Health >= MaxHealth) return false;
 if (School != 3) {
  if (!IsValid(FollowCamera)) return false;
  FHitResult Hit;
  const FVector Origin = FollowCamera->GetComponentLocation();
  const FVector End = Origin + FollowCamera->GetForwardVector() * 1800.f;
  FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallSpell), false, this);
  if (!GetWorld()->LineTraceSingleByChannel(Hit, Origin, End, ECC_Visibility, Params)) return false;
  AMoonfallEnemy* Enemy = Cast<AMoonfallEnemy>(Hit.GetActor());
  if (!IsValid(Enemy) || Enemy->Health <= 0.f) return false;
  // Reject corrupted target resource state before damage, mana cost or combo rewards.
  if (!FMath::IsFinite(Enemy->Health) || !FMath::IsFinite(Enemy->MaxHealth)
      || Enemy->MaxHealth <= 0.f) return false;
  // The first successful hostile spell shortly after a perfect parry is
  // empowered. Misses, invalid targets and healing cannot spend the opening.
  const bool bCounterReady = Time <= ParryCounterUntil
      && FMath::IsFinite(ParryCounterDamageMultiplier) && ParryCounterDamageMultiplier >= 1.f;
  const float CounterMultiplier = bCounterReady
      ? FMath::Clamp(ParryCounterDamageMultiplier, 1.f, 2.f) : 1.f;
  // Distinct spell schools exert different stance pressure. All posture gains
  // depend on real health damage and stop when the target dies.
  static const float SchoolPostureWeights[6] = {0.8f, 0.65f, 1.35f, 0.f, 1.15f, 0.9f};
  // Force weapon affix adds a modest spell damage bonus, independent of tiers.
  const float WeaponDamageBonus = 1.f + FMath::Clamp(0.018f * WeaponTier, 0.f, 0.35f)
      + (WeaponAffix == 1 ? 0.06f : 0.f)
      + FMath::Clamp(((EquippedWeaponCode / 1000) % 10) * 0.015f, 0.f, 0.075f);
  const float MomentumBonus = Time <= FinisherMomentumUntil
      && FMath::IsFinite(FinisherMomentumDamageMultiplier)
    ? FMath::Clamp(FinisherMomentumDamageMultiplier, 1.f, 1.5f) : 1.f;
  // Every connected same-target attack earns its own stage bonus.
  // Twelve connected attacks can chain, but damage caps at the stage-four
  // bonus. Longer sequences unlock posture milestones, not runaway damage.
  // at defaults. The current hit is not allowed to invent a previous hit.
  const float ComboWindow = FMath::IsFinite(HitComboWindowSeconds)
      ? FMath::Clamp(HitComboWindowSeconds, 0.5f, 4.f) : 2.f;
  // Combo power must be earned on the SAME living target. Moving to a
  // different enemy starts its own chain rather than inheriting free damage.
  const bool bSameTarget = LastHitComboTarget.IsValid() && LastHitComboTarget.Get() == Enemy;
  const bool bComboAlive = bSameTarget && Time >= LastSuccessfulHitAt && Time - LastSuccessfulHitAt <= ComboWindow;
  const int32 IncomingStage = bComboAlive ? FMath::Clamp(HitComboStage, 0, 12) : 0;
  const float ComboStepBonus = FMath::IsFinite(HitComboDamagePerStage)
      ? FMath::Clamp(HitComboDamagePerStage, 0.f, 0.12f) : 0.f;
  // After stage 12, the next connected hit begins a new chain rather than
  // looping the last animation and its damage bonus forever.
  const int32 EffectiveIncomingStage = IncomingStage == 12 ? 0 : IncomingStage;
  const float HitComboMultiplier = 1.f + ComboStepBonus * FMath::Clamp(EffectiveIncomingStage, 0, 3);
  // Secondary stats: Mastery adds reliable power, Versatility scales offense
  // and Critical Strike provides an occasional 1.5x hostile spell hit.
  // Crit is rolled only after a valid enemy was acquired.
  const bool bCriticalSpell = FMath::FRand() < GetCriticalChance();
  const float MightMultiplier = 1.f + FMath::Clamp(GetMight() * 0.001f, 0.f, 0.10f);
  const float SpellDamage = (18.f + School * 5.f) * EffectMultiplier * SpellPowerMultiplier
    * WeaponDamageBonus * CounterMultiplier * MomentumBonus * HitComboMultiplier
    * (1.f + GetMasteryBonus()) * (1.f + GetVersatilityBonus())
    * ((bCriticalSpell ? 1.5f : 1.f)
      + (bCriticalSpell ? (bTalentForceKeystone ? 0.20f : 0.f)
        + (bTalentBattleWeave ? 0.15f : 0.f) : 0.f)) * MightMultiplier;
  // Reject invalid damage before spending mana or consuming a hard-earned counter.
  if (!FMath::IsFinite(SpellDamage) || SpellDamage <= 0.f) return false;
  const float HealthBeforeSpell = Enemy->Health;
  Enemy->TakeSpellDamage(SpellDamage);
  // A valid hostile cast is combat even if enemy defenses absorb all HP damage.
  // Combo stages, posture, and counter rewards still require real damage.
  CombatRecoveryEndsAt = FMath::Max(
       FMath::IsFinite(CombatRecoveryEndsAt) ? CombatRecoveryEndsAt : 0.f,
       Time + (FMath::IsFinite(CombatRecoverySeconds)
       ? FMath::Clamp(CombatRecoverySeconds, 0.f, 12.f) : 5.f));
  const float ActualSpellDamage = FMath::Clamp(HealthBeforeSpell - Enemy->Health, 0.f, HealthBeforeSpell);
  if (ActualSpellDamage > 0.f) {
   HitComboStage = bComboAlive ? (IncomingStage >= 12 ? 1 : IncomingStage + 1) : 1;
   LastHitComboTarget = Enemy;
   LastSuccessfulHitAt = Time;
   // Milestones are earned only once per uninterrupted same-target chain.
   // Each fourth hit (4, 8, 12) adds bounded posture pressure, but no
   // area hit, free health, or repeated reward when the chain is capped.
   const bool bNewPostureMilestone = HitComboStage > IncomingStage
       && (HitComboStage == 4 || HitComboStage == 8 || HitComboStage == 12);
   if (bNewPostureMilestone && Enemy->Health > 0.f
       && FMath::IsFinite(HitComboFourthHitPosture) && HitComboFourthHitPosture > 0.f)
    Enemy->ApplyPostureDamage(FMath::Clamp(HitComboFourthHitPosture, 0.f, 30.f));
   if (bNewPostureMilestone && Enemy->Health > 0.f)
    Enemy->RegisterComboMilestone(HitComboStage);
   // A lethal hit may have already started finisher choreography in
   // TakeSpellDamage. Never interrupt that finisher with a basic combo montage.
   // Nonlethal hits use their authored slot, without requiring every slot.
   if (Enemy->Health > 0.f && !bReduceMotion && HitComboStage >= 1 && HitComboStage <= 12
       && ComboMontages.IsValidIndex(HitComboStage - 1) && GetMesh()) {
    if (UAnimInstance* Anim = GetMesh()->GetAnimInstance()) {
     if (UAnimMontage* Montage = ComboMontages[HitComboStage - 1].Get())
      Anim->Montage_Play(Montage, 1.f);
    }
   }
   OnSuccessfulHitCombo(HitComboStage, Hand == 0);
  }
  if (ActualSpellDamage > 0.f && Enemy->Health > 0.f
      && FMath::IsFinite(OffensiveSpellPostureMultiplier) && OffensiveSpellPostureMultiplier > 0.f) {
   const float ExtraPosture = ActualSpellDamage * 0.25f * SchoolPostureWeights[School]
       * FMath::Clamp(OffensiveSpellPostureMultiplier, 0.f, 3.f);
   if (FMath::IsFinite(ExtraPosture) && ExtraPosture > 0.f)
    Enemy->ApplyPostureDamage(FMath::Min(ExtraPosture, 100.f));
  }
  if (bCounterReady && ActualSpellDamage > 0.f) {
   // A perfect-parry counter also pressures posture, making the next stance
   // break attainable without handing out free pressure on a miss or no-op.
   if (Enemy->Health > 0.f && FMath::IsFinite(ParryCounterPostureDamage)
       && ParryCounterPostureDamage > 0.f)
    Enemy->ApplyPostureDamage(FMath::Clamp(ParryCounterPostureDamage, 0.f, 100.f));
   ParryCounterUntil = -1.f;
  }
  const FColor SpellColors[6] = {FColor(245, 60, 39), FColor(65, 204, 245), FColor(168, 72, 236), FColor(72, 230, 130), FColor(245, 224, 64), FColor(76, 178, 68)};
  const FColor ImpactColor = SpellColors[School];
  DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 24.f * EffectMultiplier, 12, ImpactColor, false, 0.22f, 0, 2.f);
  DrawDebugLine(GetWorld(), Origin, Hit.ImpactPoint, ImpactColor, false, 0.12f, 0, 1.5f);
 }
 Mana -= Costs[School] * ManaCostMultiplier;
 LastCastTime[Hand] = Time;
 if (School == 3) {
  Health = FMath::Min(MaxHealth, Health + 24.f * EffectMultiplier * SpellPowerMultiplier);
  OnHealthChanged(Health, MaxHealth);
 }
 OnSpellCast(School, Hand == 0);
 return true;
}
void AMoonfallHeroPawn::CastLeft() { if (bOptionsOpen || bInventoryOpen) return; TryCast(LeftSpell, 0, 1.f); }
void AMoonfallHeroPawn::LeftClickContextAction()
{
 if (bAtlasOpen && bTalentsOpen) {
  if (APlayerController* PC = Cast<APlayerController>(GetController())) {
   if (AMoonfallCombatHUD* HUD = Cast<AMoonfallCombatHUD>(PC->GetHUD())) {
    float X = 0.f, Y = 0.f;
    if (PC->GetMousePosition(X,Y)) {
     const int32 Node = HUD->GetTalentNodeAtScreenPosition(this,X,Y);
     if (Node >= 0) {
      TalentSelectedNode = Node;
      ActivateTalentNode(Node);
     }
    }
   }
  }
  return;
 }
 if (bAtlasOpen) return;
 // Mouse left selects a visible bag row, but never equips or salvages it.
 // Outside menus the same button performs the usual left-hand spell.
 if (bOptionsOpen) return;
 if (bInventoryOpen) {
  if (bCharacterStatsOpen) return;
  if (APlayerController* Controller = Cast<APlayerController>(GetController())) {
   if (AMoonfallCombatHUD* HUD = Cast<AMoonfallCombatHUD>(Controller->GetHUD())) {
    float MouseX = 0.f, MouseY = 0.f;
    if (Controller->GetMousePosition(MouseX, MouseY)) {
     if (HUD->IsInventorySortButtonAtScreenPosition(this, MouseX, MouseY)) {
      CycleInventorySort();
      return;
     }
     const int32 ClickedIndex = HUD->GetInventoryItemAtScreenPosition(this, MouseX, MouseY);
     if (InventoryItemCodes.IsValidIndex(ClickedIndex))
      InventorySelection = ClickedIndex;
    }
   }
  }
  return;
 }
 // Defer the individual spell until release so the second button can
 // form a dual-cast chord without the first cast consuming its cooldown.
 bLeftMouseHeld = true;
 if (bRightMouseHeld && !bMouseChordConsumed) {
  bMouseChordConsumed = true;
  CastBoth();
 }
}
void AMoonfallHeroPawn::CastRight()
{
 if (bAtlasOpen) return;
 // Keyboard and controller right-hand cast input must never equip gear.
 // Gamepad RB is also bound to forging while inventory is open.
 if (bOptionsOpen || bInventoryOpen) return;
 TryCast(RightSpell, 1, 1.f);
}
void AMoonfallHeroPawn::RightClickContextAction()
{
 if (bAtlasOpen) return;
 // Only the physical right mouse button equips from inventory.
 // Controller shoulder buttons keep combat/forge behavior separate.
 if (bOptionsOpen) return;
 if (bInventoryOpen) {
  if (bCharacterStatsOpen) return;
  if (APlayerController* Controller = Cast<APlayerController>(GetController())) {
   if (AMoonfallCombatHUD* HUD = Cast<AMoonfallCombatHUD>(Controller->GetHUD())) {
    float MouseX = 0.f, MouseY = 0.f;
    if (Controller->GetMousePosition(MouseX, MouseY)) {
     const int32 ClickedIndex = HUD->GetInventoryItemAtScreenPosition(this, MouseX, MouseY);
     // A right-click on empty inventory space must never equip the
     // previously selected item. Only the clicked visible row can equip.
     if (InventoryItemCodes.IsValidIndex(ClickedIndex)) {
      InventorySelection = ClickedIndex;
      EquipSelectedInventoryItem();
     }
    }
   }
  }
  return;
 }
 bRightMouseHeld = true;
 if (bLeftMouseHeld && !bMouseChordConsumed) {
  bMouseChordConsumed = true;
  CastBoth();
 }
}
void AMoonfallHeroPawn::ResetMouseCastChord()
{
 bLeftMouseHeld = false;
 bRightMouseHeld = false;
 bMouseChordConsumed = false;
}
void AMoonfallHeroPawn::ReleaseLeftClickContext()
{
 const bool bSingleCast = bLeftMouseHeld && !bMouseChordConsumed;
 bLeftMouseHeld = false;
 if (bSingleCast && !bOptionsOpen && !bInventoryOpen)
  TryCast(LeftSpell, 0, 1.f);
 if (!bLeftMouseHeld && !bRightMouseHeld) bMouseChordConsumed = false;
}
void AMoonfallHeroPawn::ReleaseRightClickContext()
{
 const bool bSingleCast = bRightMouseHeld && !bMouseChordConsumed;
 bRightMouseHeld = false;
 if (bSingleCast && !bOptionsOpen && !bInventoryOpen)
  TryCast(RightSpell, 1, 1.f);
 if (!bLeftMouseHeld && !bRightMouseHeld) bMouseChordConsumed = false;
}
void AMoonfallHeroPawn::CastBoth()
{
 if (bAtlasOpen) return;
 // LT can mean Equip Best in inventory: reject combat before doing spell work.
 if (bOptionsOpen || bInventoryOpen || bAtlasOpen || !FMath::IsFinite(Health) || Health <= 0.f) return;
 if (LeftSpell != RightSpell || !GetWorld()) return;
 // Validate both hands before spending mana or applying a single overcharge.
 if (LeftSpell < 0 || LeftSpell >= 6 || RightSpell < 0 || RightSpell >= 6) return;
 const float Time = GetWorld()->GetTimeSeconds();
 const float Haste = GetHasteBonus();
 static const float Cooldowns[6] = {0.8f,0.65f,1.2f,2.f,1.f,0.9f};
 if (!FMath::IsFinite(Time) || !FMath::IsFinite(Haste) || Haste <= -1.f ||
     !FMath::IsFinite(LastCastTime[0]) || !FMath::IsFinite(LastCastTime[1])) return;
 const float SharedCooldown = Cooldowns[LeftSpell] / (1.f + Haste);
 if (!FMath::IsFinite(SharedCooldown) || SharedCooldown <= 0.f ||
     Time - LastCastTime[0] < SharedCooldown ||
     Time - LastCastTime[1] < SharedCooldown) return;
 if (TryCast(LeftSpell, 0, 2.f, true))
 {
  // One payment and one spell effect, but both hands enter cooldown.
  LastCastTime[1] = LastCastTime[0];
  OnSpellCast(LeftSpell, false);
 }
}
void AMoonfallHeroPawn::BreakGuard()
{
 if (!bGuarding || !GetWorld()) return;
 EndGuard();
 // Losing guard also forfeits any outstanding counterattack opportunity.
 ParryCounterUntil = -1.f;
 const float RecoverySeconds = FMath::IsFinite(GuardBreakRecoverySeconds)
    ? FMath::Clamp(GuardBreakRecoverySeconds, 0.f, 3.f) : 1.f;
 GuardBrokenUntil = GetWorld()->GetTimeSeconds() + RecoverySeconds;
 OnGuardBroken();
}

void AMoonfallHeroPawn::BeginGuard()
{
 if (bAtlasOpen) return;
 if (bOptionsOpen || bInventoryOpen || Health <= 0.f || !FMath::IsFinite(Health) || !GetWorld() || bGuarding
     // Do not stack a fresh parry window on top of dodge invulnerability.
     || GetWorld()->GetTimeSeconds() < DamageImmunityUntil
     || GetWorld()->GetTimeSeconds() < GuardBrokenUntil
     || !FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina) || MaxStamina <= 0.f
     || !FMath::IsFinite(GuardStaminaCost) || GuardStaminaCost < 0.f
     || Stamina < GuardStaminaCost) return;
 // Guard and parry cooldown timestamps must be finite before committing stamina or stance.
 const float GuardNow = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(GuardNow) || !FMath::IsFinite(DamageImmunityUntil)
     || !FMath::IsFinite(GuardBrokenUntil) || !FMath::IsFinite(NextParryAt)) return;
 // A new guard stance cancels held sprint instead of resuming it on release.
 EndSprint();
 bGuarding = true;
 if (GetCharacterMovement()) {
  const float GuardMultiplier = FMath::IsFinite(GuardMoveSpeedMultiplier)
      ? FMath::Clamp(GuardMoveSpeedMultiplier, 0.2f, 1.f) : 1.f;
  GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed * GuardMultiplier;
 }
 const float Now = GetWorld()->GetTimeSeconds();
 // Holding guard cannot repeatedly open parry windows.
 if (Now >= NextParryAt && FMath::IsFinite(ParryWindowSeconds)
     && FMath::IsFinite(ParryCooldownSeconds)) {
  ParryWindowEndsAt = Now + FMath::Clamp(ParryWindowSeconds, 0.05f, 0.3f);
  NextParryAt = Now + FMath::Clamp(ParryCooldownSeconds, 0.3f, 2.f);
 } else {
  ParryWindowEndsAt = -1.f;
 }
}
void AMoonfallHeroPawn::EndGuard()
{
 const bool bWasGuarding = bGuarding;
 bGuarding = false;
 ParryWindowEndsAt = -1.f;
 if (bWasGuarding && GetCharacterMovement())
  GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
 // Guard stance cannot refill stamina for free; preserve longer dodge/impact delays.
 if (bWasGuarding && GetWorld())
  StaminaRegenStartsAt = FMath::Max(FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : GetWorld()->GetTimeSeconds(), GetWorld()->GetTimeSeconds() + 0.4f);
}
void AMoonfallHeroPawn::BeginSprint() {
 // Menu and defeat inputs must not latch a sprint for later gameplay.
 if (bOptionsOpen || bInventoryOpen || Health <= 0.f) return;
 // Holding or repeating the press must not bypass exhaustion.
 if (!bSprintHeld) {
  // An empty pool cannot be bypassed by tapping sprint repeatedly.
  const bool bValidStamina = FMath::IsFinite(Stamina) && FMath::IsFinite(MaxStamina)
      && FMath::IsFinite(SprintRestartStamina) && MaxStamina > 0.f;
  const float Required = bValidStamina ? FMath::Clamp(SprintRestartStamina, 0.f, MaxStamina) : 0.f;
  bSprintExhausted = !bValidStamina || Stamina < Required || Stamina <= 0.f;
 }
 bSprintHeld = true;
}
void AMoonfallHeroPawn::EndSprint() {
 bSprintHeld = false;
 bSprintExhausted = false;
 if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
}

void AMoonfallHeroPawn::Dodge()
{
 if (bAtlasOpen) return;
 if (bOptionsOpen || bInventoryOpen || Health <= 0.f || !GetWorld()) return;
 const float Now = GetWorld()->GetTimeSeconds();
 // Nonfinite cooldown timestamps must not allow repeated dodges or immunity.
 if (!FMath::IsFinite(Now) || !FMath::IsFinite(NextDodgeAt)
     || !FMath::IsFinite(DamageImmunityUntil)
     || Now < NextDodgeAt || Now < DamageImmunityUntil
     || !FMath::IsFinite(DodgeCooldown) || DodgeCooldown <= 0.f
     || !FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina)
     || !FMath::IsFinite(DodgeStaminaCost) || MaxStamina <= 0.f
     || DodgeStaminaCost < 0.f || Stamina < DodgeStaminaCost
     || !FMath::IsFinite(DodgeLaunchSpeed) || DodgeLaunchSpeed <= 0.f) return;
 Stamina = FMath::Clamp(Stamina - DodgeStaminaCost, 0.f, MaxStamina);
 StaminaRegenStartsAt = FMath::Max(FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : Now, Now + 0.8f);
 if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = NormalWalkSpeed;
 // Dodge takes priority over sprint. Releasing Sprint permits a fresh attempt.
 bSprintExhausted = true;
 EndGuard();
 // Even if a Blueprint changes cooldown at runtime, dodges cannot restart
 // inside the 0.24s immunity window or schedule a nonfinite cooldown.
 NextDodgeAt = Now + FMath::Clamp(DodgeCooldown, 0.24f, 5.f);
 DamageImmunityUntil = Now + 0.24f;
 PerfectDodgeWindowUntil = Now + 0.14f;
 // Prefer live input for responsive controller/keyboard dodges. On frames
 // without new input, use the most recently consumed movement direction.
 const FVector PendingDirection = GetPendingMovementInputVector().GetSafeNormal2D();
 const FVector LastDirection = GetLastMovementInputVector().GetSafeNormal2D();
 const FVector ForwardDirection = GetActorForwardVector().GetSafeNormal2D();
 const FVector Direction = !PendingDirection.IsNearlyZero() ? PendingDirection
     : (!LastDirection.IsNearlyZero() ? LastDirection : ForwardDirection);
 LaunchCharacter(Direction * FMath::Clamp(DodgeLaunchSpeed, 250.f, 1200.f), true, false);
}

void AMoonfallHeroPawn::RewardPerfectDodge() {
 TryRewardPerfectDodge();
}

bool AMoonfallHeroPawn::TryRewardPerfectDodge() {
 // Rewards are earned only during live gameplay, not behind paused overlays.
 if (!GetWorld() || Health <= 0.f || bOptionsOpen || bInventoryOpen) return false;
 const float Now = GetWorld()->GetTimeSeconds();
 // Invalid or expired windows must never grant an unearned dodge reward.
 if (!FMath::IsFinite(Now) || !FMath::IsFinite(PerfectDodgeWindowUntil)
     || !FMath::IsFinite(LastPerfectDodgeRewardAt)
     || Now > PerfectDodgeWindowUntil || Now - LastPerfectDodgeRewardAt < 1.f) return false;
 LastPerfectDodgeRewardAt = Now;
 PerfectDodgeWindowUntil = -1.f;
 // Invalid Blueprint resource values must never contaminate the combat HUD.
 if (FMath::IsFinite(MaxMana) && MaxMana > 0.f && FMath::IsFinite(Mana))
  Mana = FMath::Clamp(Mana + 8.f, 0.f, MaxMana);
 // Restore stamina only from a finite, capped reward on a true perfect dodge.
 // The one-second reward gate above prevents repeated hits from farming stamina.
 if (FMath::IsFinite(MaxStamina) && MaxStamina > 0.f && FMath::IsFinite(Stamina)
     && FMath::IsFinite(PerfectDodgeStaminaReward) && PerfectDodgeStaminaReward > 0.f) {
  Stamina = FMath::Clamp(Stamina + FMath::Clamp(PerfectDodgeStaminaReward, 0.f, 25.f), 0.f, MaxStamina);
 }
 return true;
}

bool AMoonfallHeroPawn::ApplyMoonfallDamageFrom(float Amount, const FVector& SourceLocation) {
 bLastIncomingHitParried = false;
 IncomingDamageSource = SourceLocation;
 bHasIncomingDamageSource = true;
 ApplyMoonfallDamage(Amount);
 bHasIncomingDamageSource = false;
 return bLastIncomingHitParried;
}

void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {
 // Reject invalid hero vitals and damage before guard math or HUD updates.
 if (Health <= 0.f || !FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth)
     || MaxHealth <= 0.f || !FMath::IsFinite(Amount) || Amount <= 0.f || !GetWorld()) return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now) || !FMath::IsFinite(DamageImmunityUntil)
     || GetWorld()->GetTimeSeconds() < DamageImmunityUntil) return;
 const float PreviousHealth = Health;
 // Ward armor affix provides a small, capped flat mitigation bonus.
 const float ArmorMitigation = FMath::Min(0.34f, FMath::Min(0.28f, ArmorTier * 0.01f)
   + (ArmorAffix == 2 ? 0.06f : 0.f)
   + FMath::Clamp(((EquippedArmorCode / 1000) % 10) * 0.005f, 0.f, 0.025f)
   + GetVersatilityBonus() * 0.5f
   + (GetSetPieceCount(2) >= 3 ? 0.025f : 0.f));
 const float ArmoredDamage = Amount * (1.f - ArmorMitigation);
 float DamageAfterGuard = ArmoredDamage;
 // Enemy attacks must come from the front 150-degree guard cone.
 // Older environment damage with no provided origin retains its guard behavior.
 // Guard follows the hero mesh/actor facing, not the orbiting camera.
 // Movement currently orients the character independently of view yaw.
 const FVector GuardForward = GetActorForwardVector().GetSafeNormal2D();
 const FVector ToSource = (IncomingDamageSource - GetActorLocation()).GetSafeNormal2D();
 const bool bFrontAttack = !bHasIncomingDamageSource || ToSource.IsNearlyZero()
  || FVector::DotProduct(GuardForward, ToSource) >= 0.258819f;
 // Heavy attacks exert greater guard pressure. Invalid tuning values fail closed.
 const bool bValidGuardCost = FMath::IsFinite(GuardStaminaCost) && GuardStaminaCost >= 0.f
     && FMath::IsFinite(GuardImpactStaminaScale) && GuardImpactStaminaScale >= 0.f;
 const float HitGuardCost = bValidGuardCost && FMath::IsFinite(MaxStamina) && MaxStamina > 0.f
     ? GuardStaminaCost + FMath::Clamp(ArmoredDamage * FMath::Min(GuardImpactStaminaScale, 1.f), 0.f, MaxStamina)
     : 0.f;
 // Perfect timing negates impact stamina altogether. The guard stance
 // still costs stamina while held, and normal blocks retain heavy-hit pressure.
 // Perfect parries are deliberate responses to a known, front-facing
 // attacker. Environmental damage without a source may still be blocked.
 const bool bPerfectParry = bGuarding && bHasIncomingDamageSource && bFrontAttack
     && FMath::IsFinite(ParryWindowEndsAt) && FMath::IsFinite(NextParryAt)
     && Now <= ParryWindowEndsAt && Now <= NextParryAt;
 const float RequiredGuardCost = bPerfectParry ? 0.f : HitGuardCost;
 if (bGuarding && bFrontAttack && !bOptionsOpen
     && FMath::IsFinite(Stamina) && FMath::IsFinite(MaxStamina) && MaxStamina > 0.f
     && bValidGuardCost && FMath::IsFinite(RequiredGuardCost)
     && Stamina >= RequiredGuardCost) {
  // Successful blocks and parries both count as active combat.
  CombatRecoveryEndsAt = FMath::Max(
       FMath::IsFinite(CombatRecoveryEndsAt) ? CombatRecoveryEndsAt : 0.f,
       Now + (FMath::IsFinite(CombatRecoverySeconds)
      ? FMath::Clamp(CombatRecoverySeconds, 0.f, 12.f) : 5.f));
  // Physical defense uses the same stamina pool as sprinting and dodging.
  Stamina = FMath::Clamp(Stamina - RequiredGuardCost, 0.f, MaxStamina);
  // Guard impacts cannot shorten an existing dodge/sprint recovery delay.
  StaminaRegenStartsAt = FMath::Max(FMath::IsFinite(StaminaRegenStartsAt) ? StaminaRegenStartsAt : Now, Now + 0.8f);
  if (bPerfectParry) {
   // Perfect timing returns a small amount of mana as well as stamina.
   if (FMath::IsFinite(Mana) && FMath::IsFinite(MaxMana) && MaxMana > 0.f
       && FMath::IsFinite(PerfectParryManaReward) && PerfectParryManaReward > 0.f)
    Mana = FMath::Clamp(Mana + FMath::Clamp(PerfectParryManaReward, 0.f, 15.f), 0.f, MaxMana);
   bLastIncomingHitParried = bHasIncomingDamageSource;
   DamageAfterGuard = 0.f;
   ParryWindowEndsAt = -1.f;
   // Malformed tuning must not preserve a previous counterattack window.
   ParryCounterUntil = -1.f;
   if (FMath::IsFinite(ParryCounterWindowSeconds) && ParryCounterWindowSeconds > 0.f)
    ParryCounterUntil = Now + FMath::Clamp(ParryCounterWindowSeconds, 0.f, 5.f);
   if (FMath::IsFinite(PerfectParryStaminaReward) && PerfectParryStaminaReward > 0.f)
    Stamina = FMath::Clamp(Stamina + PerfectParryStaminaReward, 0.f, MaxStamina);
  } else {
   // Bulwark improves sustained frontal blocking, not perfect parry.
   DamageAfterGuard *= bTalentWardKeystone ? 0.32f : 0.4f;
  }
  OnGuardImpact(ArmoredDamage - DamageAfterGuard, bPerfectParry);
  if (Stamina < GuardStaminaCost) BreakGuard();
 } else if (bGuarding && bFrontAttack) {
  // Insufficient stamina breaks guard without granting free damage reduction.
  BreakGuard();
 }
 Health = FMath::Max(0.f, Health - DamageAfterGuard);
 // Taking actual health damage interrupts the perfect-parry offensive opening.
 // Fully blocked/parried hits leave it intact.
 const float DamageTaken = PreviousHealth - Health;
 if (DamageTaken > 0.f) {
  CombatRecoveryEndsAt = FMath::Max(
       FMath::IsFinite(CombatRecoveryEndsAt) ? CombatRecoveryEndsAt : 0.f,
       Now + (FMath::IsFinite(CombatRecoverySeconds)
       ? FMath::Clamp(CombatRecoverySeconds, 0.f, 12.f) : 5.f));
  // Prevent immediate menu healing after taking combat damage.
  MenuHealthRegenStartsAt = Now + (FMath::IsFinite(MenuHealthRegenDelaySeconds)
       ? FMath::Clamp(MenuHealthRegenDelaySeconds, 0.f, 10.f) : 3.f);
  ParryCounterUntil = -1.f;
  // A real injury breaks the aggressive momentum chain. Parries and blocks
  // still preserve it, so defensive skill matters.
  FinisherMomentumUntil = -1.f;
  HitComboStage = 0;
  LastHitComboTarget.Reset();
  LastSuccessfulHitAt = -100.f;
 }
 if (Health <= 0.f) {
  // Prevent lingering movement/jump input from carrying through defeat.
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  // Clear any held sprint so a later respawn cannot inherit it.
  EndSprint();
  EndGuard();
  // Defeat clears every gameplay menu and releases the inventory cursor.
  bInventoryOpen = false;
  bCharacterStatsOpen = false;
  bAtlasOpen = false;
  bTalentsOpen = false;
  bOptionsOpen = false;
  ResetMouseCastChord();
  UpdateInventoryMouseMode();
 }
 if (!bReduceMotion) ImpactCameraOffset = FMath::Clamp(ImpactCameraOffset + FMath::Min(DamageTaken * 0.5f, 18.f), 0.f, 24.f);
 OnDamageImpact(DamageTaken);
 OnHealthChanged(Health, MaxHealth);
}


void AMoonfallHeroPawn::CycleGraphicsQuality() {
 if (!GEngine) return;
 UGameUserSettings* Settings = GEngine->GetGameUserSettings();
 if (!Settings) return;
 const int32 Next = (Settings->GetOverallScalabilityLevel() + 1) % 4;
 Settings->SetOverallScalabilityLevel(Next);
 Settings->ApplySettings(false);
 Settings->SaveSettings();
}
void AMoonfallHeroPawn::ToggleDisplayMode() {
 if (!GEngine) return;
 UGameUserSettings* Settings = GEngine->GetGameUserSettings();
 if (!Settings) return;
 const EWindowMode::Type Next = Settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? EWindowMode::Windowed : EWindowMode::WindowedFullscreen;
 Settings->SetFullscreenMode(Next);
 Settings->ApplySettings(false);
 Settings->SaveSettings();
}

void AMoonfallHeroPawn::SwitchInventoryPage()
{
 // Controller A is contextual: gameplay still jumps, inventory switches pages.
 // It never equips, salvages, closes the inventory or enters the Atlas.
 if (!bInventoryOpen || bOptionsOpen || bAtlasOpen ||
     !FMath::IsFinite(Health) || Health <= 0.f) return;
 ResetMouseCastChord();
 bCharacterStatsOpen = !bCharacterStatsOpen;
}

void AMoonfallHeroPawn::ToggleCharacterStats()
{
 // C opens character info from gameplay; controller A still switches
 // between the character sheet and inventory without equipping anything.
 if (bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f) return;
 if (bAtlasOpen) { bAtlasOpen = false; bTalentsOpen = false; }
 UpdateInventoryMouseMode();
 if (!bInventoryOpen) {
  ResetMouseCastChord();
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  EndSprint();
  EndGuard();
  bInventoryOpen = true;
  bCharacterStatsOpen = true;
  UpdateInventoryMouseMode();
 } else if (bCharacterStatsOpen) {
  // C closes character info without leaving the inventory behind.
  bCharacterStatsOpen = false;
  bInventoryOpen = false;
  UpdateInventoryMouseMode();
 } else {
  bCharacterStatsOpen = true;
 }
}

void AMoonfallHeroPawn::UpdateInventoryMouseMode()
{
 if (APlayerController* Controller = Cast<APlayerController>(GetController())) {
  if (!Controller->IsLocalController()) return;
  if ((bInventoryOpen || (bAtlasOpen && bTalentsOpen)) && FMath::IsFinite(Health) && Health > 0.f) {
   // GameAndUI preserves the bound right-click action without hiding the cursor.
   FInputModeGameAndUI InputMode;
   InputMode.SetHideCursorDuringCapture(false);
   InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
   Controller->SetInputMode(InputMode);
   Controller->bShowMouseCursor = true;
  } else {
   Controller->SetInputMode(FInputModeGameOnly());
   Controller->bShowMouseCursor = false;
  }
 }
}

void AMoonfallHeroPawn::ToggleAtlas()
{
 if (!FMath::IsFinite(Health) || Health <= 0.f || bOptionsOpen) return;
 ResetMouseCastChord();
 // M always opens the map, or closes it when the map is already visible.
 const bool bWasMapOpen = bAtlasOpen && !bTalentsOpen;
 if (!bAtlasOpen) {
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  EndSprint();
  EndGuard();
  if (bInventoryOpen) {
   bInventoryOpen = false;
   bCharacterStatsOpen = false;
   UpdateInventoryMouseMode();
  }
 }
 bAtlasOpen = !bWasMapOpen;
 bTalentsOpen = false;
 UpdateInventoryMouseMode();
}

int32 AMoonfallHeroPawn::GetUnspentTalentPoints() const
{
 // Level 1 begins with no points. Clamp overspending after external level edits.
 return FMath::Max(0, FMath::Clamp(Level, 1, 100) - 1
   - TalentForceRank - TalentFocusRank - TalentWardRank
   - int32(bTalentForceKeystone) - int32(bTalentFocusKeystone)
   - int32(bTalentWardKeystone) - int32(bTalentBattleWeave)
   - int32(bTalentArcaneAegis) - int32(bTalentIronWill));
}

bool AMoonfallHeroPawn::SpendTalentPoint(int32 Branch)
{
 // Talents belong to the playable hero, not the Z/Nyra bosses.
 // Never spend points from combat, inventory, map, or the death screen.
 if (!bAtlasOpen || !bTalentsOpen || bInventoryOpen || bOptionsOpen ||
     !FMath::IsFinite(Health) || Health <= 0.f ||
     GetUnspentTalentPoints() <= 0) return false;
 int32* Rank = Branch == 0 ? &TalentForceRank
   : (Branch == 1 ? &TalentFocusRank : (Branch == 2 ? &TalentWardRank : nullptr));
 if (!Rank || *Rank < 0 || *Rank >= 10) return false;
 ++(*Rank);
 return true;
}

void AMoonfallHeroPawn::SpendForceTalent() { SpendTalentPoint(0); }
void AMoonfallHeroPawn::SpendFocusTalent() { SpendTalentPoint(1); }
void AMoonfallHeroPawn::SpendWardTalent() { SpendTalentPoint(2); }

bool AMoonfallHeroPawn::ActivateTalentNode(int32 Node)
{
 if (Node < 0 || Node >= 9 || !bAtlasOpen || !bTalentsOpen ||
     bOptionsOpen || bInventoryOpen || Health <= 0.f) return false;
 TalentSelectedNode = Node;
 return Node < 3 ? SpendTalentPoint(Node) : UnlockTalentNode(Node);
}

void AMoonfallHeroPawn::ActivateSelectedTalentNode()
{
 ActivateTalentNode(FMath::Clamp(TalentSelectedNode,0,8));
}

void AMoonfallHeroPawn::SelectNextTalentNode()
{
 if (bAtlasOpen && bTalentsOpen && !bOptionsOpen && Health > 0.f)
  TalentSelectedNode = (FMath::Clamp(TalentSelectedNode,0,8) + 1) % 9;
}

void AMoonfallHeroPawn::SelectPreviousTalentNode()
{
 if (bAtlasOpen && bTalentsOpen && !bOptionsOpen && Health > 0.f)
  TalentSelectedNode = (FMath::Clamp(TalentSelectedNode,0,8) + 8) % 9;
}

bool AMoonfallHeroPawn::CanUnlockTalentNode(int32 Node) const
{
 if (!bAtlasOpen || !bTalentsOpen || bInventoryOpen || bOptionsOpen ||
     !FMath::IsFinite(Health) || Health <= 0.f ||
     GetUnspentTalentPoints() <= 0) return false;
 // Tier II: invest three ranks in a root branch.
 if (Node == 3) return TalentForceRank >= 3 && !bTalentForceKeystone;
 if (Node == 4) return TalentFocusRank >= 3 && !bTalentFocusKeystone;
 if (Node == 5) return TalentWardRank >= 3 && !bTalentWardKeystone;
 // Tier III: linked keystones in both connected branches.
 if (Node == 6) return bTalentForceKeystone && bTalentFocusKeystone && !bTalentBattleWeave;
 if (Node == 7) return bTalentFocusKeystone && bTalentWardKeystone && !bTalentArcaneAegis;
 if (Node == 8) return bTalentWardKeystone && bTalentForceKeystone && !bTalentIronWill;
 return false;
}

bool AMoonfallHeroPawn::UnlockTalentNode(int32 Node)
{
 if (!CanUnlockTalentNode(Node)) return false;
 bool* Flag = Node == 3 ? &bTalentForceKeystone
   : Node == 4 ? &bTalentFocusKeystone
   : Node == 5 ? &bTalentWardKeystone
   : Node == 6 ? &bTalentBattleWeave
   : Node == 7 ? &bTalentArcaneAegis
   : Node == 8 ? &bTalentIronWill : nullptr;
 if (!Flag || *Flag) return false;
 *Flag = true;
 return true;
}

void AMoonfallHeroPawn::UnlockForceKeystone() { UnlockTalentNode(3); }
void AMoonfallHeroPawn::UnlockFocusKeystone() { UnlockTalentNode(4); }
void AMoonfallHeroPawn::UnlockWardKeystone() { UnlockTalentNode(5); }
void AMoonfallHeroPawn::UnlockBattleWeave() { UnlockTalentNode(6); }
void AMoonfallHeroPawn::UnlockArcaneAegis() { UnlockTalentNode(7); }
void AMoonfallHeroPawn::UnlockIronWill() { UnlockTalentNode(8); }

void AMoonfallHeroPawn::RespecTalentAction() { RespecTalents(); }

bool AMoonfallHeroPawn::RespecTalents()
{
 // Explicit Backspace only inside K; no implicit resets or costs.
 if (!bAtlasOpen || !bTalentsOpen || bInventoryOpen || bOptionsOpen ||
     !FMath::IsFinite(Health) || Health <= 0.f) return false;
 TalentForceRank = 0;
 TalentFocusRank = 0;
 TalentWardRank = 0;
 bTalentForceKeystone = false;
 bTalentFocusKeystone = false;
 bTalentWardKeystone = false;
 bTalentBattleWeave = false;
 bTalentArcaneAegis = false;
 bTalentIronWill = false;
 return true;
}

void AMoonfallHeroPawn::ToggleTalents()
{
 if (!FMath::IsFinite(Health) || Health <= 0.f || bOptionsOpen) return;
 ResetMouseCastChord();
 // Reuse the gameplay menu lock of the map, but show a separate K screen.
 if (bAtlasOpen && bTalentsOpen) {
  bTalentsOpen = false;
  bAtlasOpen = false;
  UpdateInventoryMouseMode();
  return;
 }
 if (!bAtlasOpen) {
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  EndSprint();
  EndGuard();
  if (bInventoryOpen) {
   bInventoryOpen = false;
   bCharacterStatsOpen = false;
   UpdateInventoryMouseMode();
  }
 }
 bAtlasOpen = true;
 bTalentsOpen = true;
 UpdateInventoryMouseMode();
}

void AMoonfallHeroPawn::ToggleInventory() {
 if (bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f) return;
 ResetMouseCastChord();
 if (bAtlasOpen) { bAtlasOpen = false; bTalentsOpen = false; }
 UpdateInventoryMouseMode();
 if (bCharacterStatsOpen) {
  // B returns from character info directly to the editable inventory.
  bCharacterStatsOpen = false;
  return;
 }
 if (!bInventoryOpen) {
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  EndSprint();
  EndGuard();
  InventorySelection = FMath::Clamp(InventorySelection, 0, FMath::Max(0, InventoryItemCodes.Num() - 1));
  InventoryViewStart = InventorySelection;
 }
 bInventoryOpen = !bInventoryOpen;
 if (!bInventoryOpen) bCharacterStatsOpen = false;
 UpdateInventoryMouseMode();
}
void AMoonfallHeroPawn::CycleInventorySort()
{
 // Reorder only while the editable backpack is open. No gear or currency mutations.
 if (!bInventoryOpen || bCharacterStatsOpen || bOptionsOpen ||
     !FMath::IsFinite(Health) || Health <= 0.f) return;
 InventorySortMode = InventorySortMode == 1 ? 2 : 1;
 if (InventoryItemCodes.Num() < 2) return;
 const int32 OldSelection = FMath::Clamp(InventorySelection, 0, InventoryItemCodes.Num() - 1);
 const int32 SelectedCode = InventoryItemCodes[OldSelection];
 // Retain the selected duplicate by its stable occurrence number.
 int32 SelectedOccurrence = 0;
 for (int32 Index = 0; Index < OldSelection; ++Index)
  if (InventoryItemCodes[Index] == SelectedCode) ++SelectedOccurrence;
 const int32 Mode = InventorySortMode;
 InventoryItemCodes.StableSort([Mode](const int32 A, const int32 B)
 {
  const auto Valid = [](int32 Code) -> bool {
   if (Code <= 0) return false;
   const int32 Affix = Code / 100000;
   const int32 Slot = (Code / 10000) % 10;
   const int32 Rarity = (Code / 1000) % 10;
   const int32 Tier = Code % 1000;
   return Affix >= 0 && Affix <= 3 && Slot >= 0 && Slot <= 2 &&
     Rarity >= 0 && Rarity <= 5 && Tier >= 1 && Tier <= 25 &&
     (Rarity > 0 || Affix == 0) &&
     Code == Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
  };
  const bool AValid = Valid(A), BValid = Valid(B);
  if (AValid != BValid) return AValid;
  if (!AValid) return false;
  const int32 ASlot = (A / 10000) % 10, BSlot = (B / 10000) % 10;
  const int32 ARarity = (A / 1000) % 10, BRarity = (B / 1000) % 10;
  const int32 ATier = A % 1000, BTier = B % 1000;
  if (Mode == 2 && ASlot != BSlot) return ASlot < BSlot;
  if (ARarity != BRarity) return ARarity > BRarity;
  if (ATier != BTier) return ATier > BTier;
  if (Mode == 1 && ASlot != BSlot) return ASlot < BSlot;
  return A < B;
 });
 for (int32 Index = 0; Index < InventoryItemCodes.Num(); ++Index)
  if (InventoryItemCodes[Index] == SelectedCode) {
   if (SelectedOccurrence == 0) {
    InventorySelection = Index;
    InventoryViewStart = Index;
    return;
   }
   --SelectedOccurrence;
  }
 // Defensive fallback: selection remains a valid visible entry.
 InventorySelection = FMath::Clamp(OldSelection, 0, InventoryItemCodes.Num() - 1);
 InventoryViewStart = InventorySelection;
}
void AMoonfallHeroPawn::EquipSelectedInventoryItem() {
 if (!bInventoryOpen || bCharacterStatsOpen || !InventoryItemCodes.IsValidIndex(InventorySelection)) return;
 EquipInventoryItem(InventorySelection);
}
void AMoonfallHeroPawn::SalvageSelectedInventoryItem() {
 if (bInventoryOpen && !bCharacterStatsOpen) SalvageInventoryItem(InventorySelection);
}
void AMoonfallHeroPawn::InventorySalvage() { SalvageSelectedInventoryItem(); }
bool AMoonfallHeroPawn::SalvageInventoryItem(int32 InventoryIndex)
{
 // Enforce the inventory menu boundary for Blueprint and direct callers too.
 // A background call must never destroy a bag item or award free salvage.
 if (!bInventoryOpen || bCharacterStatsOpen || bOptionsOpen ||
     !FMath::IsFinite(Health) || Health <= 0.f ||
     !InventoryItemCodes.IsValidIndex(InventoryIndex)) return false;
 const int32 Code = InventoryItemCodes[InventoryIndex];
 if (Code <= 0) return false;
 // The final inventory copy of equipped gear is protected. Extra duplicates
 // may be salvaged because the worn code remains represented in the bag.
 const bool bEquippedCode = Code == EquippedWeaponCode || Code == EquippedArmorCode
     || Code == EquippedAccessoryCode;
 if (bEquippedCode) {
  int32 Copies = 0;
  for (const int32 Candidate : InventoryItemCodes)
   if (Candidate == Code) ++Copies;
  if (Copies <= 1) return false;
 }
 const int32 Affix = Code / 100000;
 const int32 SlotIndex = (Code / 10000) % 10;
 const int32 Rarity = (Code / 1000) % 10;
 const int32 Tier = Code % 1000;
 if (Affix < 0 || Affix > 3 || SlotIndex < 0 || SlotIndex > 2
     || Rarity < 0 || Rarity > 5 || Tier < 1 || Tier > 25
     || (Rarity == 0 && Affix != 0)
     || Code != Affix * 100000 + SlotIndex * 10000 + Rarity * 1000 + Tier)
  return false;
 // Higher rarity/tier gear produces more salvage, never exceeding the cap.
 const int32 SalvageReward = 1 + Rarity * 2 + Tier / 5;
 Salvage = FMath::Clamp(Salvage + SalvageReward, 0, 9999);
 InventoryItemCodes.RemoveAt(InventoryIndex);
 InventorySelection = FMath::Clamp(InventorySelection, 0, FMath::Max(0, InventoryItemCodes.Num() - 1));
 // A shrinking bag never leaves the scroll viewport stranded past its end.
 InventoryViewStart = FMath::Clamp(InventoryViewStart, 0, FMath::Max(0, InventoryItemCodes.Num() - 1));
 return true;
}
bool AMoonfallHeroPawn::UpgradeInventoryItem(int32 InventoryIndex)
{
 // Craft the selected inventory item on explicit input. If it is worn,
 // upgrade that equipped slot too, without auto-equipping other bag items.
 if (!bInventoryOpen || bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f
     || !InventoryItemCodes.IsValidIndex(InventoryIndex)) return false;
 const int32 Code = InventoryItemCodes[InventoryIndex];
 if (Code <= 0) return false;
 const int32 Affix = Code / 100000;
 const int32 Slot = (Code / 10000) % 10;
 const int32 Rarity = (Code / 1000) % 10;
 const int32 Tier = Code % 1000;
 if (Affix < 0 || Affix > 3 || Slot < 0 || Slot > 2
     || Rarity < 0 || Rarity > 5 || Tier < 1 || Tier >= 25
     || (Rarity == 0 && Affix != 0)
     || Code != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier) return false;
 // A selected item matching the worn slot upgrades both representations.
 // Other items remain unchanged even when they share the same item code.
 const int32 EquippedCode = Slot == 0 ? EquippedWeaponCode
     : (Slot == 1 ? EquippedArmorCode : EquippedAccessoryCode);
 const bool bUpgradingEquipped = Code == EquippedCode;
 // Snapshot the currently worn tier before the equip sync updates it.
 const int32 PreviousEquippedTier = Slot == 0 ? WeaponTier
     : (Slot == 1 ? ArmorTier : AccessoryTier);
 // Higher tiers and rarities consume more of the salvage recovered from drops.
 const int32 Cost = 4 + Tier * 2 + Rarity * 3;
 if (Salvage < Cost) return false;
 const int32 NewCode = Affix * 100000 + Slot * 10000 + Rarity * 1000 + (Tier + 1);
 InventoryItemCodes[InventoryIndex] = NewCode;
 // Reuse the existing equip path so all derived stats, notably armor HP,
 // update together. Roll back the inventory mutation on unexpected failure.
 if (bUpgradingEquipped && !EquipInventoryItem(InventoryIndex)) {
  InventoryItemCodes[InventoryIndex] = Code;
  return false;
 }
 Salvage -= Cost;
 LastLootUpgrade = (Tier + 1) - PreviousEquippedTier;
 OnEquipmentCrafted(NewCode, Cost);
 return true;
}

bool AMoonfallHeroPawn::ReforgeInventoryItemAffix(int32 InventoryIndex)
{
 // Explicit crafting inside inventory only. Common/uncommon traits cannot be
 // reforged; rare and higher retain their tier and quality unchanged.
 if (!bInventoryOpen || bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f
     || !InventoryItemCodes.IsValidIndex(InventoryIndex)) return false;
 const int32 Code = InventoryItemCodes[InventoryIndex];
 if (Code <= 0) return false;
 const int32 Affix = Code / 100000;
 const int32 Slot = (Code / 10000) % 10;
 const int32 Rarity = (Code / 1000) % 10;
 const int32 Tier = Code % 1000;
 if (Affix < 0 || Affix > 3 || Slot < 0 || Slot > 2
     || Rarity < 2 || Rarity > 5 || Tier < 1 || Tier > 25
     || Code != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier) return false;
 // Unlike normal drops, reforge always yields a nonzero affix different
 // from the old one, including for an originally traitless Rare item.
 const int32 Cost = 12 + Tier * 2 + Rarity * 8;
 if (Salvage < Cost) return false;
 const int32 NewAffix = Affix == 0 ? FMath::RandRange(1, 3)
     : 1 + ((Affix - 1 + FMath::RandRange(1, 2)) % 3);
 const int32 NewCode = NewAffix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
 const int32 EquippedCode = Slot == 0 ? EquippedWeaponCode
     : (Slot == 1 ? EquippedArmorCode : EquippedAccessoryCode);
 const bool bReforgingEquipped = Code == EquippedCode;
 InventoryItemCodes[InventoryIndex] = NewCode;
 // Keep the bag and the worn item in sync, including direct combat bonuses.
 // Roll back and leave salvage untouched if that sync unexpectedly fails.
 if (bReforgingEquipped && !EquipInventoryItem(InventoryIndex)) {
  InventoryItemCodes[InventoryIndex] = Code;
  return false;
 }
 Salvage -= Cost;
 LastLootAffix = NewAffix;
 OnEquipmentReforged(NewCode, Cost);
 return true;
}

void AMoonfallHeroPawn::ReforgeSelectedInventoryItem()
{
 if (!bCharacterStatsOpen) ReforgeInventoryItemAffix(InventorySelection);
}

bool AMoonfallHeroPawn::AscendInventoryItemRarity(int32 InventoryIndex)
{
 // Explicit inventory-only crafting. Ascension does not change tier or affix.
 if (!bInventoryOpen || bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f
     || !InventoryItemCodes.IsValidIndex(InventoryIndex)) return false;
 const int32 Code = InventoryItemCodes[InventoryIndex];
 if (Code <= 0) return false;
 const int32 Affix = Code / 100000;
 const int32 Slot = (Code / 10000) % 10;
 const int32 Rarity = (Code / 1000) % 10;
 const int32 Tier = Code % 1000;
 if (Affix < 0 || Affix > 3 || Slot < 0 || Slot > 2
     || Rarity < 0 || Rarity >= 5 || Tier < 1 || Tier > 25
     || (Rarity == 0 && Affix != 0)
     || Code != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier) return false;
 // Rising rank requires progressively more materials, but costs remain
 // bounded. Validate the entire transaction before editing inventory.
 const int32 Cost = 20 + Tier * 3 + Rarity * 15;
 if (Salvage < Cost) return false;
 const int32 EquippedCode = Slot == 0 ? EquippedWeaponCode
     : (Slot == 1 ? EquippedArmorCode : EquippedAccessoryCode);
 const bool bAscendingEquipped = Code == EquippedCode;
 const int32 NewCode = Affix * 100000 + Slot * 10000 + (Rarity + 1) * 1000 + Tier;
 InventoryItemCodes[InventoryIndex] = NewCode;
 if (bAscendingEquipped && !EquipInventoryItem(InventoryIndex)) {
  InventoryItemCodes[InventoryIndex] = Code;
  return false;
 }
 Salvage -= Cost;
 LastLootRarity = Rarity + 1;
 OnEquipmentAscended(NewCode, Cost);
 return true;
}

void AMoonfallHeroPawn::AscendSelectedInventoryItem()
{
 if (!bCharacterStatsOpen) AscendInventoryItemRarity(InventorySelection);
}

int32 AMoonfallHeroPawn::ForgeInventoryItemLevels(int32 InventoryIndex, int32 MaxLevels)
{
 if (!bInventoryOpen || bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f
     || !InventoryItemCodes.IsValidIndex(InventoryIndex)) return 0;
 // Reuse validated, transactional one-tier crafting for every step.
 // No special currency discount, overflow, or forced equipment changes.
 const int32 Limit = FMath::Clamp(MaxLevels, 0, 5);
 int32 Upgraded = 0;
 for (int32 Step = 0; Step < Limit; ++Step) {
  if (!UpgradeInventoryItem(InventoryIndex)) break;
  ++Upgraded;
 }
 return Upgraded;
}

void AMoonfallHeroPawn::ForgeFiveSelectedInventoryItem()
{
 if (!bCharacterStatsOpen) ForgeInventoryItemLevels(InventorySelection, 5);
}

void AMoonfallHeroPawn::UpgradeSelectedInventoryItem()
{
 if (!bCharacterStatsOpen) UpgradeInventoryItem(InventorySelection);
}

void AMoonfallHeroPawn::OpenOptions() {
 // The defeat screen owns input until respawn; do not reopen gameplay menus.
 if (Health <= 0.f || !FMath::IsFinite(Health)) return;
 ResetMouseCastChord();
 if (bAtlasOpen) {
  bAtlasOpen = false;
  bTalentsOpen = false;
  UpdateInventoryMouseMode();
  return;
 }
 if (bInventoryOpen) {
  bInventoryOpen = false;
  bCharacterStatsOpen = false;
  UpdateInventoryMouseMode();
  return;
 }
 if (!bOptionsOpen) {
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  EndSprint();
  EndGuard();
 }
 bOptionsOpen = !bOptionsOpen;
}
void AMoonfallHeroPawn::OptionsUp() {
 if (bInventoryOpen) {
  InventorySelection = FMath::Max(0, InventorySelection - 1);
  InventoryViewStart = InventorySelection;
  return;
 }
 if (bOptionsOpen) OptionsSelection = (OptionsSelection + 6) % 7;
}
void AMoonfallHeroPawn::OptionsDown() {
 if (bInventoryOpen) {
  InventorySelection = FMath::Min(FMath::Max(0, InventoryItemCodes.Num() - 1), InventorySelection + 1);
  InventoryViewStart = InventorySelection;
  return;
 }
 if (bOptionsOpen) OptionsSelection = (OptionsSelection + 1) % 7;
}
void AMoonfallHeroPawn::OptionsLeft() {
 if (bInventoryOpen) {
  // Directional input only browses gear. Equipping is row right-click only.
  InventorySelection = FMath::Clamp(InventorySelection - 5, 0,
    FMath::Max(0, InventoryItemCodes.Num() - 1));
  InventoryViewStart = InventorySelection;
  return;
 }
 AdjustOption(-1);
}
void AMoonfallHeroPawn::OptionsRight() {
 if (bInventoryOpen) {
  InventorySelection = FMath::Clamp(InventorySelection + 5, 0,
    FMath::Max(0, InventoryItemCodes.Num() - 1));
  InventoryViewStart = InventorySelection;
  return;
 }
 AdjustOption(1);
}
void AMoonfallHeroPawn::AdjustOption(int32 Direction) {
 if (!bOptionsOpen || !GEngine) return;
 UGameUserSettings* Settings = GEngine->GetGameUserSettings();
 if (!Settings) return;
 if (OptionsSelection == 0) {
  const FIntPoint Resolutions[] = {FIntPoint(1280,720), FIntPoint(1920,1080), FIntPoint(2560,1440), FIntPoint(3840,2160)};
  int32 Current = 1;
  for (int32 i = 0; i < 4; ++i) if (Settings->GetScreenResolution() == Resolutions[i]) Current = i;
  Settings->SetScreenResolution(Resolutions[FMath::Clamp(Current + Direction, 0, 3)]);
 } else if (OptionsSelection == 1) {
  float CurrentScale = 100.f;
  float NormalizedScale = 0.f, MinScale = 50.f, MaxScale = 100.f;
  Settings->GetResolutionScaleInformationEx(CurrentScale, NormalizedScale, MinScale, MaxScale);
  Settings->SetResolutionScaleValueEx(FMath::Clamp(CurrentScale + Direction * 10.f, FMath::Max(50.f, MinScale), FMath::Min(100.f, MaxScale)));
 } else if (OptionsSelection == 2) {
  Settings->SetOverallScalabilityLevel(FMath::Clamp(Settings->GetOverallScalabilityLevel() + Direction, 0, 3));
 } else if (OptionsSelection == 3) {
  FrameGenerationPreference = FMath::Clamp(FrameGenerationPreference + Direction, 0, 2);
  // Store user preference only. The feature remains inactive without vendor integration.
  if (GConfig) {
   GConfig->SetInt(TEXT("Moonfall.Graphics"), TEXT("FrameGenerationPreference"), FrameGenerationPreference, GGameUserSettingsIni);
   GConfig->Flush(false, GGameUserSettingsIni);
  }
  return;
 } else if (OptionsSelection == 4) {
  bReduceMotion = Direction > 0;
  if (bReduceMotion) ImpactCameraOffset = 0.f;
  if (GConfig) {
   GConfig->SetBool(TEXT("Moonfall.Accessibility"), TEXT("ReduceMotion"), bReduceMotion, GGameUserSettingsIni);
   GConfig->Flush(false, GGameUserSettingsIni);
  }
  return;
 } else if (OptionsSelection == 5) {
  CombatTextScale = FMath::Clamp(CombatTextScale + Direction * 0.25f, 0.75f, 2.f);
  if (GConfig) {
   GConfig->SetFloat(TEXT("Moonfall.Accessibility"), TEXT("CombatTextScale"), CombatTextScale, GGameUserSettingsIni);
   GConfig->Flush(false, GGameUserSettingsIni);
  }
  return;
 } else if (OptionsSelection == 6) {
  BossWarningScale = FMath::Clamp(BossWarningScale + Direction * 0.25f, 1.f, 2.f);
  if (GConfig) {
   GConfig->SetFloat(TEXT("Moonfall.Accessibility"), TEXT("BossWarningScale"), BossWarningScale, GGameUserSettingsIni);
   GConfig->Flush(false, GGameUserSettingsIni);
  }
  return;
 }
 Settings->ApplySettings(false);
 Settings->SaveSettings();
}

int32 AMoonfallHeroPawn::RegisterFinisherKill(bool bShatterKill, bool bBossKill, FVector DefeatedLocation, AMoonfallEnemy* DefeatedEnemy)
{
 if (Health <= 0.f || !GetWorld()) return 0;
 const float Now = GetWorld()->GetTimeSeconds();
 // A confirmed finisher is combat, not a safe rest window.
 CombatRecoveryEndsAt = FMath::Max(
       FMath::IsFinite(CombatRecoveryEndsAt) ? CombatRecoveryEndsAt : 0.f,
       Now + (FMath::IsFinite(CombatRecoverySeconds)
     ? FMath::Clamp(CombatRecoverySeconds, 0.f, 12.f) : 5.f));
 const float Window = FMath::IsFinite(FinisherChainWindowSeconds)
   ? FMath::Clamp(FinisherChainWindowSeconds, 0.5f, 10.f) : 4.f;
 const bool bContinueChain = Now >= LastFinisherKillAt && Now - LastFinisherKillAt <= Window;
 FinisherSequenceCount = bContinueChain
   ? FMath::Min(FinisherSequenceCount + 1, 1000000) : 1;
 FinisherChainCount = FMath::Min(FinisherSequenceCount, 5);
 LastFinisherKillAt = Now;
 const int32 ChainStep = (FinisherSequenceCount - 1) % 5 + 1;
 // Every confirmed kill contributes capped momentum, independent of queued
 // visual playback. Boss and shatter kills earn stronger resource rewards.
 const float KillRewardScale = bBossKill ? 2.f : (bShatterKill ? 1.5f : 1.f);
 if (FMath::IsFinite(MaxStamina) && MaxStamina > 0.f && FMath::IsFinite(Stamina)
     && FMath::IsFinite(FinisherStaminaRestore) && FinisherStaminaRestore > 0.f)
  Stamina = FMath::Clamp(Stamina + FMath::Clamp(FinisherStaminaRestore, 0.f, 25.f)
      * KillRewardScale, 0.f, MaxStamina);
 if (FMath::IsFinite(MaxMana) && MaxMana > 0.f && FMath::IsFinite(Mana)
     && FMath::IsFinite(FinisherManaRestore) && FinisherManaRestore > 0.f)
  Mana = FMath::Clamp(Mana + FMath::Clamp(FinisherManaRestore, 0.f, 25.f)
      * KillRewardScale, 0.f, MaxMana);
 if (FinisherChainCount >= 3 && FMath::IsFinite(FinisherMomentumDurationSeconds)
     && FinisherMomentumDurationSeconds >= 1.f)
  FinisherMomentumUntil = Now + FMath::Clamp(FinisherMomentumDurationSeconds, 1.f, 12.f);
 // Kills during menus must still resolve the enemy death animation, but must
 // not start or queue hidden finisher choreography.
 if (bOptionsOpen || bInventoryOpen || !FinisherSlashA || !FinisherSlashB || !FinisherFlash) {
  if (IsValid(DefeatedEnemy)) DefeatedEnemy->BeginFinisherDefeat(ChainStep);
  return ChainStep;
 }
 // Queue follow-up executions instead of cutting off the current choreography.
 // Five visual styles cycle by combo count; the bounded queue prevents bloat.
 const float NativeDuration = bReduceMotion ? 0.22f : 0.78f;
 if (PendingFinishers.Num() > 0 || (Now >= NativeFinisherStartedAt && Now - NativeFinisherStartedAt < NativeDuration)) {
  FPendingFinisher Pending;
  Pending.Target = DefeatedLocation.ContainsNaN()
    ? GetActorLocation() + GetActorForwardVector() * 140.f : DefeatedLocation;
  Pending.Style = ChainStep;
  Pending.Enemy = DefeatedEnemy;
  Pending.bShatter = bShatterKill;
  Pending.bBoss = bBossKill;
  if (PendingFinishers.Num() < MaxPendingFinishers) PendingFinishers.Add(Pending);
  else {
   // Preserve all earlier queued choreography and prioritize the most recent
   // finishing move if more than eight kills arrive before playback catches up.
   if (AMoonfallEnemy* SupersededEnemy = PendingFinishers.Last().Enemy.Get())
    SupersededEnemy->BeginFinisherDefeat(PendingFinishers.Last().Style);
   PendingFinishers[PendingFinishers.Num() - 1] = Pending;
  }
  return ChainStep;
 }
 StartFinisherPresentation(ChainStep, bShatterKill, bBossKill, DefeatedLocation, DefeatedEnemy);
 return ChainStep;
}

void AMoonfallHeroPawn::StartFinisherPresentation(int32 ChainStep, bool bShatterKill, bool bBossKill, const FVector& DefeatedLocation, AMoonfallEnemy* DefeatedEnemy)
{
 if (!GetWorld()) return;
 const float Now = GetWorld()->GetTimeSeconds();
 // Built-in impact choreography is mandatory. No mesh/AnimBP/asset assignment
 // is needed for the two slash shapes and flash attached in the constructor.
 // The enemy's death reaction begins alongside its actual queued execution,
 // not when it first entered the queue. Weak references expire safely.
 if (IsValid(DefeatedEnemy)) DefeatedEnemy->BeginFinisherDefeat(ChainStep);
 NativeFinisherStartedAt = Now;
 NativeFinisherOrigin = GetActorLocation();
 NativeFinisherTarget = DefeatedLocation.ContainsNaN() ? NativeFinisherOrigin + GetActorForwardVector() * 140.f : DefeatedLocation;
 NativeFinisherForward = (NativeFinisherTarget - NativeFinisherOrigin).GetSafeNormal2D();
 if (NativeFinisherForward.IsNearlyZero()) NativeFinisherForward = GetActorForwardVector().GetSafeNormal2D();
 NativeFinisherStyle = ChainStep;
 bNativeFinisherShatter = bShatterKill;
 bNativeFinisherBoss = bBossKill;
 if (!bReduceMotion && GetMesh() && FinisherMontages.Num() > 0) {
  if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance()) {
   const int32 MontageIndex = (ChainStep - 1) % FinisherMontages.Num();
   if (UAnimMontage* Montage = FinisherMontages[MontageIndex].Get())
    AnimInstance->Montage_Play(Montage, 1.f);
  }
 }
 OnFinisherChain(ChainStep, bShatterKill, bBossKill);
}

void AMoonfallHeroPawn::UpdateNativeFinisher(float DeltaSeconds)
{
 if (!GetWorld()) return;
 float Age = GetWorld()->GetTimeSeconds() - NativeFinisherStartedAt;
 const float Duration = bReduceMotion ? 0.22f : 0.78f;
 const bool bMissingFinisherVisuals = !FinisherSlashA || !FinisherSlashB || !FinisherFlash;
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bMissingFinisherVisuals) {
  // Resolve skipped presentations so defeated enemies never remain frozen
  // when the hero is defeated or gameplay menus interrupt choreography.
  for (const FPendingFinisher& Pending : PendingFinishers) {
   if (AMoonfallEnemy* WaitingEnemy = Pending.Enemy.Get())
    WaitingEnemy->BeginFinisherDefeat(Pending.Style);
  }
  PendingFinishers.Reset();
 }
 // Never access a missing effect component; queued enemy death reactions above
 // still resolve, so stripped/failed VFX do not strand corpses.
 if (bMissingFinisherVisuals) return;
 // Start the oldest queued execution after the previous effect completes.
 // Presentation (including montage and Blueprint notification) occurs on start.
 if (Health > 0.f && !bOptionsOpen && !bInventoryOpen && Age >= Duration && PendingFinishers.Num() > 0) {
  const FPendingFinisher Pending = PendingFinishers[0];
  PendingFinishers.RemoveAt(0);
  StartFinisherPresentation(Pending.Style, Pending.bShatter, Pending.bBoss, Pending.Target, Pending.Enemy.Get());
  Age = 0.f;
 }
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || Age < 0.f || Age >= Duration) {
  FinisherSlashA->SetHiddenInGame(true);
  FinisherSlashB->SetHiddenInGame(true);
  FinisherFlash->SetVisibility(false);
  return;
 }
 const float T = FMath::Clamp(Age / Duration, 0.f, 1.f);
 const float Envelope = FMath::Sin(T * PI);
 // Snapshot facing and style on kill so moving or chaining the next attack
 // does not bend this finisher's trajectory toward an unrelated target.
 const FVector Origin = NativeFinisherOrigin;
 const FVector Forward = NativeFinisherForward;
 const FVector Side = FVector::CrossProduct(FVector::UpVector, Forward);
 const int32 Style = FMath::Clamp(NativeFinisherStyle, 1, 5);
 // Five distinct readable movements: crossing cleave, rising strike,
 // double sweep, spinning slash, and final descending execution.
 const float Spin = (Style == 4 ? 2.f * PI : PI) * T;
 const float Sweep = FMath::Sin((T - 0.5f) * PI);
 const float Lateral = Style == 1 ? 110.f * Sweep
   : Style == 2 ? -65.f + 130.f * T
   : Style == 3 ? 130.f * Sweep
   : Style == 4 ? 115.f * FMath::Cos(Spin)
   : 70.f * FMath::Sin(Spin);
 const float Height = Style == 2 ? 55.f + T * 150.f
   : Style == 5 ? 245.f - T * 165.f
   : 105.f + 60.f * FMath::Sin(Spin);
 const FVector Center = Origin + Forward * (90.f + 45.f * Envelope)
    + Side * Lateral + FVector::UpVector * Height;
 const float BladeLength = bNativeFinisherBoss ? 3.2f : (bNativeFinisherShatter ? 2.8f : 2.25f);
 const float Tilt = Style == 2 ? 70.f - 140.f * T
   : Style == 5 ? -70.f + 140.f * T : 110.f * Sweep;
 const FRotator SlashRotation = FRotationMatrix::MakeFromX(Forward).Rotator()
   + FRotator(Tilt, 0.f, Style == 4 ? FMath::RadiansToDegrees(Spin) : 35.f * Sweep);
 FinisherSlashA->SetWorldLocationAndRotation(Center, SlashRotation);
 FinisherSlashA->SetWorldScale3D(FVector(BladeLength * Envelope, 0.055f, 0.025f));
 FinisherSlashA->SetHiddenInGame(bReduceMotion);
 FinisherSlashB->SetWorldLocationAndRotation(Center + Side * 35.f - FVector::UpVector * 30.f,
   SlashRotation + FRotator(Style == 3 ? -55.f : 48.f, 0.f, 0.f));
 FinisherSlashB->SetWorldScale3D(FVector(BladeLength * 0.65f * Envelope, 0.045f, 0.023f));
 FinisherSlashB->SetHiddenInGame(bReduceMotion || Style == 2);
 FinisherFlash->SetWorldLocation(Center);
 FinisherFlash->SetLightColor(bNativeFinisherShatter ? FLinearColor(1.f, 0.25f, 0.8f)
   : FLinearColor(0.2f, 0.78f, 1.f));
 FinisherFlash->SetIntensity((bReduceMotion ? 900.f : (bNativeFinisherBoss ? 4800.f : 3500.f)) * Envelope);
 FinisherFlash->SetVisibility(true);
}

void AMoonfallHeroPawn::AwardExperience(int32 Amount)
{
 if (Amount <= 0 || Health <= 0.f || Level >= 100) return;
 Experience = FMath::Min(1000000, Experience + FMath::Clamp(Amount, 0, 100000));
 while (Experience >= ExperienceToNextLevel && Level < 100) {
  Experience -= ExperienceToNextLevel;
  ++Level;
  ExperienceToNextLevel = 100 + (Level - 1) * 55;
  MaxHealth += 4.f;
  SpellPowerMultiplier = 1.f + 0.025f * FMath::Min(Level - 1, 30) + 0.012f * FMath::Max(0, Level - 31);
  Health = FMath::Min(MaxHealth, Health + 12.f);
  Mana = FMath::Min(MaxMana, Mana + 10.f);
  OnHealthChanged(Health, MaxHealth);
 }
 if (Level >= 100) Experience = 0;
}

int32 AMoonfallHeroPawn::RollEquipmentCode(bool bEliteEnemy, bool bBossEnemy)
{
 if (!FMath::IsFinite(Health) || Health <= 0.f) return 0;
 // Roll randomized item stats once at death; pickup is handled separately.
 // Keep boss/elite rewards meaningful: guaranteed epic for bosses, at least rare
 // for elites. Regular enemies use weighted chances with a modest level bias.
 const int32 NewTier = FMath::Clamp(Level / 5 + (bBossEnemy ? 2 : (bEliteEnemy ? 1 : 0)), 1, 25);
 const int32 LootRoll = FMath::RandRange(0, 99);
 const int32 RarityBias = FMath::Clamp(Level / 5, 0, 12);
 // Existing rarity codes 0-3 are stable for compatibility.
 // 4 Emerald and 5 Legendary add special high-end reward targets.
 // Bosses always roll at least Epic; max-level bosses can yield Legendary.
 const int32 RolledRarity = bBossEnemy
   ? (Level >= 60 && LootRoll < 12 ? 5 : (Level >= 30 && LootRoll < 40 ? 4 : 3))
   : (bEliteEnemy
     ? (Level >= 50 && LootRoll < 3 ? 5 : (Level >= 20 && LootRoll < 12 ? 4
       : (LootRoll < 27 ? 3 : 2)))
     : (Level >= 60 && LootRoll < 1 ? 5 : (Level >= 40 && LootRoll < 3 ? 4
       : (LootRoll < 3 + RarityBias / 3 ? 3
       : (LootRoll < 18 + RarityBias ? 2
       : (LootRoll < 53 + RarityBias ? 1 : 0)))))));
 // Enemy rank influences quality, not gear slot: any enemy can drop
 // weapons, armor, or accessories.
 const int32 RolledSlot = FMath::RandRange(0, 2);
 // Rare and better drops carry one randomized trait. Lower rarities may
 // roll a trait, but common items never do. These are encoded in item IDs.
 const int32 RolledAffix = RolledRarity == 0 ? 0
   : (RolledRarity == 1 && FMath::RandRange(0, 99) < 60
     ? 0 : FMath::RandRange(1, 3));
 // Compact item code: affix (100000s), slot (10000s), rarity (1000s),
 // tier (units). Distinct drops persist until the player salvages them.
 const int32 ItemCode = RolledAffix * 100000 + RolledSlot * 10000
   + RolledRarity * 1000 + NewTier;
 return ItemCode;
}

// Legacy direct grant remains available for scripted rewards. Enemy deaths
// instead spawn a ground actor, so equipment is not auto-collected on kills.
void AMoonfallHeroPawn::AwardEquipmentDrop(bool bEliteEnemy, bool bBossEnemy)
{
 // Legacy scripted rewards are direct inventory grants, never automatic equips.
 // Enemy kills still create world loot through SpawnGroundEquipmentDrop.
 if (!FMath::IsFinite(Health) || Health <= 0.f) return;
 const int32 ItemCode = RollEquipmentCode(bEliteEnemy, bBossEnemy);
 if (ItemCode <= 0) return;
 // Mirror Godot's bounded item inventory: failed awards must not announce loot.
 // UE stores individual item codes, rather than Godot's stack counts.
 if (InventoryItemCodes.Num() >= MaxInventoryItems) return;
 // A scripted grant has been received. Only now update the loot notification.
 // Keep the player's existing inventory selection and worn gear unchanged.
 const int32 RolledAffix = ItemCode / 100000;
 const int32 RolledSlot = (ItemCode / 10000) % 10;
 const int32 RolledRarity = (ItemCode / 1000) % 10;
 const int32 RolledTier = ItemCode % 1000;
 InventoryItemCodes.Add(ItemCode);
 LastLootAffix = RolledAffix;
 LastLootSlot = RolledSlot;
 LastLootRarity = RolledRarity;
 LastLootTier = RolledTier;
 const int32 EquippedTier = RolledSlot == 0 ? WeaponTier
     : (RolledSlot == 1 ? ArmorTier : AccessoryTier);
 LastLootUpgrade = RolledTier - EquippedTier;
}

bool AMoonfallHeroPawn::CollectGroundEquipmentCode(int32 ItemCode)
{
 if (bAtlasOpen) return false;
 // Enforce the same gameplay state at the collection boundary as the
 // manual pickup input. Other future callers must not bypass open menus.
 if (!FMath::IsFinite(Health) || Health <= 0.f || bOptionsOpen || bInventoryOpen || ItemCode <= 0) return false;
 const int32 Affix = ItemCode / 100000;
 const int32 SlotIndex = (ItemCode / 10000) % 10;
 const int32 Rarity = (ItemCode / 1000) % 10;
 const int32 Tier = ItemCode % 1000;
 if (Affix < 0 || Affix > 3 || SlotIndex < 0 || SlotIndex > 2
     || Rarity < 0 || Rarity > 5 || Tier < 1 || Tier > 25
     || (Rarity == 0 && Affix != 0)
     || ItemCode != Affix * 100000 + SlotIndex * 10000 + Rarity * 1000 + Tier)
  return false;
 // Preserve the physical ground drop when the bag is full. TryCollect must
 // only destroy the actor after CollectGroundEquipmentCode returns true.
 if (InventoryItemCodes.Num() >= MaxInventoryItems) return false;
 // Picking up an item NEVER equips, automatically salvages, or deletes gear.
 InventoryItemCodes.Add(ItemCode);
 LastLootAffix = Affix;
 LastLootSlot = SlotIndex;
 LastLootRarity = Rarity;
 LastLootTier = Tier;
 const int32 EquippedTier = SlotIndex == 0 ? WeaponTier
     : (SlotIndex == 1 ? ArmorTier : AccessoryTier);
 LastLootUpgrade = Tier - EquippedTier;
 return true;
}

void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(bool bEliteEnemy, bool bBossEnemy, FVector DefeatedLocation)
{
 if (!GetWorld() || !FMath::IsFinite(Health) || Health <= 0.f) return;
 // Bosses scatter three rewards, elites two, regular enemies one.
 // All remain on the ground until individually collected; never auto-salvage.
 const int32 DropCount = bBossEnemy ? 3 : (bEliteEnemy ? 2 : 1);
 // A corrupt enemy location must never generate loot at infinite world coordinates.
 // Fall back to the hero only if the hero location is also fully finite.
 const auto FiniteLocation = [](const FVector& Location) {
  return FMath::IsFinite(Location.X) && FMath::IsFinite(Location.Y) &&
         FMath::IsFinite(Location.Z);
 };
 const FVector HeroPosition = GetActorLocation();
 const FVector Origin = FiniteLocation(DefeatedLocation) ? DefeatedLocation : HeroPosition;
 if (!FiniteLocation(Origin)) return;
 for (int32 Drop = 0; Drop < DropCount; ++Drop) {
  const int32 ItemCode = RollEquipmentCode(bEliteEnemy, bBossEnemy);
  if (ItemCode <= 0) continue;
  const float Angle = 2.f * PI * static_cast<float>(Drop) / static_cast<float>(DropCount);
  const float Spread = DropCount == 1 ? 0.f : 105.f;
  const FVector SpawnAt = Origin + FVector(Spread * FMath::Cos(Angle),
    Spread * FMath::Sin(Angle), 45.f);
  if (!FiniteLocation(SpawnAt)) continue;
  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  if (AMoonfallGroundLoot* Loot = GetWorld()->SpawnActor<AMoonfallGroundLoot>(
      AMoonfallGroundLoot::StaticClass(), SpawnAt, FRotator::ZeroRotator, Params))
   Loot->InitializeLoot(ItemCode);
 }
}

void AMoonfallHeroPawn::PickupNearestGroundLoot()
{
 if (bAtlasOpen) return;
 if (!FMath::IsFinite(Health) || Health <= 0.f || bOptionsOpen || bInventoryOpen || !GetWorld()) return;
 // One actor pass prevents distant drops from starving nearby ones. Keep the
 // closest 96 so blocked loot does not hide reachable drops behind it.
 // Expensive visibility traces happen only within TryCollect, never per actor.
 constexpr int32 MaxLootCandidates = 96;
 constexpr int32 MaxLootPerPress = 6;
 TArray<AMoonfallGroundLoot*> NearbyLoot;
 NearbyLoot.Reserve(MaxLootCandidates);
 TArray<float> NearbyDistanceSq;
 NearbyDistanceSq.Reserve(MaxLootCandidates);
 const FVector HeroPosition = GetActorLocation();
 const float PickupRangeSq = FMath::Square(250.f);
 for (TActorIterator<AMoonfallGroundLoot> It(GetWorld()); It; ++It) {
  AMoonfallGroundLoot* Candidate = *It;
  if (!IsValid(Candidate) || Candidate->GetItemCode() <= 0) continue;
  const FVector LootPosition = Candidate->GetActorLocation();
  if (FMath::Abs(LootPosition.Z - HeroPosition.Z) > 150.f) continue;
  const float DistanceSq = FVector::DistSquared2D(LootPosition, HeroPosition);
  if (!FMath::IsFinite(DistanceSq) || DistanceSq > PickupRangeSq) continue;
  int32 InsertAt = 0;
  while (InsertAt < NearbyDistanceSq.Num() && NearbyDistanceSq[InsertAt] <= DistanceSq)
   ++InsertAt;
  if (InsertAt >= MaxLootCandidates) continue;
  NearbyLoot.Insert(Candidate, InsertAt);
  NearbyDistanceSq.Insert(DistanceSq, InsertAt);
  if (NearbyLoot.Num() > MaxLootCandidates) {
   NearbyLoot.RemoveAt(MaxLootCandidates);
   NearbyDistanceSq.RemoveAt(MaxLootCandidates);
  }
 }
 int32 Collected = 0;
 for (AMoonfallGroundLoot* Loot : NearbyLoot) {
  if (!IsValid(Loot)) continue;
  // Failed or obstructed pickups do not count toward six successes.
  // TryCollect is the sole authority for reach, cover and item validity.
  if (Loot->TryCollect() && ++Collected >= MaxLootPerPress) break;
 }
}

// A deterministic, no-network gear advisor. Tier has the largest weight;
// rarity and set synergy only break close ties, so an AI hint never silently
// changes equipment or overrides a deliberate player preference.
int32 AMoonfallHeroPawn::GetEquipmentUpgradeScore(int32 InventoryIndex) const
{
 // Suppress advice outside a valid inventory state, including direct
 // Blueprint queries, while preserving a read-only score for valid entries.
 if (!bInventoryOpen || bCharacterStatsOpen || bOptionsOpen ||
     !FMath::IsFinite(Health) || Health <= 0.f ||
     !InventoryItemCodes.IsValidIndex(InventoryIndex)) return 0;
 const int32 Candidate = InventoryItemCodes[InventoryIndex];
 if (Candidate <= 0) return 0;
 const int32 Slot = (Candidate / 10000) % 10;
 int32 Trait = 0, Rarity = 0, Tier = 0;
 if (!DecodeEquippedItem(Candidate, Slot, Trait, Rarity, Tier)) return 0;
 const int32 EquippedCodes[3] = {EquippedWeaponCode, EquippedArmorCode, EquippedAccessoryCode};
 if (Candidate == EquippedCodes[Slot]) return 0;
 const auto GearScore = [&](int32 Code) {
  int32 ScoredTrait = 0, ScoredRarity = 0, ScoredTier = 0;
  if (!DecodeEquippedItem(Code, Slot, ScoredTrait, ScoredRarity, ScoredTier)) return 0;
  // Existing valid gear gains a small bonus for set synergy with the OTHER
  // slots only. No double-counting its own set piece.
  int32 MatchingPieces = 0;
  if (ScoredTrait > 0) {
   for (int32 OtherSlot = 0; OtherSlot < 3; ++OtherSlot) {
    if (OtherSlot == Slot) continue;
    int32 OtherTrait = 0, OtherRarity = 0, OtherTier = 0;
    if (DecodeEquippedItem(EquippedCodes[OtherSlot], OtherSlot,
                           OtherTrait, OtherRarity, OtherTier) &&
        OtherTrait == ScoredTrait) ++MatchingPieces;
   }
  }
  const int32 SynergyScore = MatchingPieces == 2 ? 80
      : (MatchingPieces == 1 ? 40 : 0);
  return ScoredTier * 1000 + ScoredRarity * 100 +
         (ScoredTrait > 0 ? 10 : 0) + SynergyScore;
 };
 return GearScore(Candidate) - GearScore(EquippedCodes[Slot]);
}

int32 AMoonfallHeroPawn::GetRecommendedEquipmentIndex() const
{
 // Only advise while the inventory is displayed. This function is read-only
 // and does not change selection, worn gear, health, currency or the save.
 if (!bInventoryOpen || bCharacterStatsOpen || bOptionsOpen ||
     !FMath::IsFinite(Health) || Health <= 0.f)
  return -1;
 int32 BestIndex = -1;
 int32 BestImprovement = 0;
 for (int32 Index = 0; Index < InventoryItemCodes.Num(); ++Index) {
  const int32 Improvement = GetEquipmentUpgradeScore(Index);
  if (Improvement > BestImprovement) {
   BestImprovement = Improvement;
   BestIndex = Index;
  }
 }
 return BestIndex;
}

void AMoonfallHeroPawn::EquipBestInventoryAction() { if (!bCharacterStatsOpen) EquipBestInventoryUpgrades(); }

int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades()
{
 // Require valid hero state before even scanning the inventory. The explicit
 // EquipInventoryItem boundary independently enforces the same restriction.
 if (!bInventoryOpen || bCharacterStatsOpen || bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f) return 0;
 const int32 EquippedCodes[3] = {EquippedWeaponCode, EquippedArmorCode, EquippedAccessoryCode};
 int32 BestScores[3] = {-1, -1, -1};
 int32 BestIndices[3] = {-1, -1, -1};
 // Keep existing equipment unless the candidate is strictly better.
 // Tier is primary, rarity secondary, an affix breaks ties.
 for (int32 Slot = 0; Slot < 3; ++Slot) {
  const int32 Code = EquippedCodes[Slot];
  if (Code <= 0) continue;
  const int32 Affix = Code / 100000;
  const int32 EncodedSlot = (Code / 10000) % 10;
  const int32 Rarity = (Code / 1000) % 10;
  const int32 Tier = Code % 1000;
  if (EncodedSlot != Slot || Affix < 0 || Affix > 3 || Rarity < 0 || Rarity > 5
      || Tier < 1 || Tier > 25 || (Rarity == 0 && Affix != 0)
      || Code != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier) continue;
  BestScores[Slot] = Tier * 100 + Rarity * 10 + (Affix > 0 ? 1 : 0);
 }
 for (int32 Index = 0; Index < InventoryItemCodes.Num(); ++Index) {
  const int32 Code = InventoryItemCodes[Index];
  if (Code <= 0) continue;
  const int32 Affix = Code / 100000;
  const int32 Slot = (Code / 10000) % 10;
  const int32 Rarity = (Code / 1000) % 10;
  const int32 Tier = Code % 1000;
  if (Slot < 0 || Slot > 2 || Affix < 0 || Affix > 3 || Rarity < 0 || Rarity > 5
      || Tier < 1 || Tier > 25 || (Rarity == 0 && Affix != 0)
      || Code != Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier) continue;
  const int32 Score = Tier * 100 + Rarity * 10 + (Affix > 0 ? 1 : 0);
  if (Score > BestScores[Slot]) {
   BestScores[Slot] = Score;
   BestIndices[Slot] = Index;
  }
 }
 int32 UpgradesEquipped = 0;
 for (int32 Slot = 0; Slot < 3; ++Slot) {
  if (BestIndices[Slot] >= 0 && EquipInventoryItem(BestIndices[Slot]))
   ++UpgradesEquipped;
 }
 OnBestLoadoutEquipped(UpgradesEquipped);
 return UpgradesEquipped;
}

bool AMoonfallHeroPawn::EquipInventoryItem(int32 InventoryIndex)
{
 // Equipping is always deliberate and inventory-scoped, even when called
 // directly from a Blueprint or by the worn-item crafting sync path.
 if (!FMath::IsFinite(Health) || Health <= 0.f || !bInventoryOpen ||
     bCharacterStatsOpen || bOptionsOpen ||
     !InventoryItemCodes.IsValidIndex(InventoryIndex)) return false;
 const int32 Code = InventoryItemCodes[InventoryIndex];
 if (Code <= 0) return false;
 // Validate every decoded field before mutating equipped items or health.
 const int32 Affix = Code / 100000;
 const int32 SlotIndex = (Code / 10000) % 10;
 const int32 Rarity = (Code / 1000) % 10;
 const int32 Tier = Code % 1000;
 if (Affix < 0 || Affix > 3 || SlotIndex < 0 || SlotIndex > 2
     || Rarity < 0 || Rarity > 5 || Tier < 1 || Tier > 25
     || (Rarity == 0 && Affix != 0)
     || Code != Affix * 100000 + SlotIndex * 10000 + Rarity * 1000 + Tier)
  return false;
 int32* EquippedTier = SlotIndex == 0 ? &WeaponTier : (SlotIndex == 1 ? &ArmorTier : &AccessoryTier);
 int32* EquippedAffix = SlotIndex == 0 ? &WeaponAffix : (SlotIndex == 1 ? &ArmorAffix : &AccessoryAffix);
 int32* EquippedCode = SlotIndex == 0 ? &EquippedWeaponCode
     : (SlotIndex == 1 ? &EquippedArmorCode : &EquippedAccessoryCode);
 if (*EquippedCode == Code && *EquippedTier == Tier && *EquippedAffix == Affix) return false;
 const int32 PreviousTier = *EquippedTier;
 *EquippedTier = Tier;
 *EquippedAffix = Affix;
 *EquippedCode = Code;
 EquipmentTier = FMath::Max(WeaponTier, FMath::Max(ArmorTier, AccessoryTier));
 if (SlotIndex == 1 && PreviousTier != Tier) {
  // Equipping lower-tier armor cannot grant free health, or reduce it below 1.
  const float HealthDelta = 2.f * (Tier - PreviousTier);
  MaxHealth = FMath::Max(1.f, MaxHealth + HealthDelta);
  Health = FMath::Clamp(Health + HealthDelta, 1.f, MaxHealth);
  OnHealthChanged(Health, MaxHealth);
 }
 return true;
}
