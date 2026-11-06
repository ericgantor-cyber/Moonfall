#include "MoonfallHeroPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Interfaces/IPluginManager.h"
#include "MoonfallEnemy.h"
#include "CollisionQueryParams.h"
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
}
void AMoonfallHeroPawn::BeginPlay() {
 Super::BeginPlay();
 int32 SavedPreference = 0;
 if (GConfig) GConfig->GetInt(TEXT("Moonfall.Graphics"), TEXT("FrameGenerationPreference"), SavedPreference, GGameUserSettingsIni);
 FrameGenerationPreference = FMath::Clamp(SavedPreference, 0, 2);
 if (GConfig) GConfig->GetBool(TEXT("Moonfall.Accessibility"), TEXT("ReduceMotion"), bReduceMotion, GGameUserSettingsIni);
 if (GConfig) GConfig->GetFloat(TEXT("Moonfall.Accessibility"), TEXT("CombatTextScale"), CombatTextScale, GGameUserSettingsIni);
 CombatTextScale = FMath::Clamp(FMath::RoundToFloat(CombatTextScale * 4.f) / 4.f, 0.75f, 2.f);
 if (GConfig) GConfig->GetFloat(TEXT("Moonfall.Accessibility"), TEXT("BossWarningScale"), BossWarningScale, GGameUserSettingsIni);
 BossWarningScale = FMath::Clamp(FMath::RoundToFloat(BossWarningScale * 4.f) / 4.f, 1.f, 2.f);
 // Restore only a valid, explicitly saved personal map pin; never restore quest locations.
 if (GConfig) {
  bool bSavedWaypointActive = false;
  GConfig->GetBool(TEXT("Moonfall.Map"), TEXT("PersonalWaypointActive"), bSavedWaypointActive, GGameUserSettingsIni);
  if (bSavedWaypointActive) {
   float X = 0.f, Y = 0.f, Z = 0.f;
   const bool bValid = GConfig->GetFloat(TEXT("Moonfall.Map"), TEXT("WaypointX"), X, GGameUserSettingsIni)
    && GConfig->GetFloat(TEXT("Moonfall.Map"), TEXT("WaypointY"), Y, GGameUserSettingsIni)
    && GConfig->GetFloat(TEXT("Moonfall.Map"), TEXT("WaypointZ"), Z, GGameUserSettingsIni)
    && FMath::IsFinite(X) && FMath::IsFinite(Y) && FMath::IsFinite(Z)
    && FMath::Abs(X) <= 10000000.f && FMath::Abs(Y) <= 10000000.f && FMath::Abs(Z) <= 10000000.f;
   if (bValid) {
    PersonalWaypoint = FVector(X,Y,Z);
    bPersonalWaypointActive = true;
   }
  }
 }
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
 // Bounded camera impulse, no camera shake object or spawned effects.
 ImpactCameraOffset = FMath::FInterpTo(ImpactCameraOffset, 0.f, FMath::Max(0.f, DeltaSeconds), 12.f);
 if (CameraBoom)
 {
  const float TargetDistance = FMath::Clamp(CameraDistance + (bReduceMotion ? 0.f : ImpactCameraOffset), 250.f, 850.f);
  CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetDistance, FMath::Max(0.f, DeltaSeconds), 10.f);
 }
 if (Health > 0.f && Mana < MaxMana) Mana = FMath::Min(MaxMana, Mana + ManaRegenPerSecond * (1.f + FMath::Min(0.25f, AccessoryTier * 0.012f)) * FMath::Max(0.f, DeltaSeconds));
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
 Input->BindAction(TEXT("MinimapZoomIn"), IE_Pressed, this, &AMoonfallHeroPawn::ZoomMinimapIn);
 Input->BindAction(TEXT("MinimapZoomOut"), IE_Pressed, this, &AMoonfallHeroPawn::ZoomMinimapOut);
 Input->BindAction(TEXT("WorldMap"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleWorldMap);
 Input->BindAction(TEXT("WorldMapRecenter"), IE_Pressed, this, &AMoonfallHeroPawn::RecenterWorldMap);
 Input->BindAction(TEXT("PlaceWaypoint"), IE_Pressed, this, &AMoonfallHeroPawn::PlacePersonalWaypoint);
 Input->BindAction(TEXT("MapClickWaypoint"), IE_Pressed, this, &AMoonfallHeroPawn::PlacePersonalWaypointAtCursor);
 Input->BindAction(TEXT("ClearWaypoint"), IE_Pressed, this, &AMoonfallHeroPawn::ClearPersonalWaypoint);
 Input->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
 Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
 Input->BindAction(TEXT("CastLeft"), IE_Pressed, this, &AMoonfallHeroPawn::CastLeft);
 Input->BindAction(TEXT("CastRight"), IE_Pressed, this, &AMoonfallHeroPawn::CastRight);
 Input->BindAction(TEXT("DualCast"), IE_Pressed, this, &AMoonfallHeroPawn::CastBoth);
 Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AMoonfallHeroPawn::Dodge);
 Input->BindAction(TEXT("GraphicsQuality"), IE_Pressed, this, &AMoonfallHeroPawn::CycleGraphicsQuality);
 Input->BindAction(TEXT("DisplayMode"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleDisplayMode);
 Input->BindAction(TEXT("Options"), IE_Pressed, this, &AMoonfallHeroPawn::OpenOptions);
 Input->BindAction(TEXT("OptionsUp"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsUp);
 Input->BindAction(TEXT("OptionsDown"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsDown);
 Input->BindAction(TEXT("OptionsLeft"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsLeft);
 Input->BindAction(TEXT("OptionsRight"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsRight);
}
void AMoonfallHeroPawn::ToggleWorldMap()
{
 if (bOptionsOpen) return;
 bWorldMapOpen = !bWorldMapOpen;
 if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->bShowMouseCursor = bWorldMapOpen;
}
void AMoonfallHeroPawn::RecenterWorldMap() { if (bWorldMapOpen && !bOptionsOpen) WorldMapPan = FVector2D::ZeroVector; }
void AMoonfallHeroPawn::PlacePersonalWaypoint()
{
 if (!bWorldMapOpen || bOptionsOpen) return;
 const FVector Origin = GetActorLocation();
 // The atlas is player-relative and panned in world X/Y, so its centre is the placement target.
 PersonalWaypoint = FVector(Origin.X + WorldMapPan.X, Origin.Y + WorldMapPan.Y, Origin.Z);
 bPersonalWaypointActive = true;
 SavePersonalWaypoint();
}
void AMoonfallHeroPawn::PlacePersonalWaypointAtCursor()
{
 if (!bWorldMapOpen || bOptionsOpen) return;
 APlayerController* PC = Cast<APlayerController>(GetController());
 if (!PC) return;
 int32 ViewX = 0, ViewY = 0;
 PC->GetViewportSize(ViewX,ViewY);
 float MouseX = 0.f, MouseY = 0.f;
 if (ViewX <= 0 || ViewY <= 0 || !PC->GetMousePosition(MouseX,MouseY)) return;
 const float Width = FMath::Max(200.f, static_cast<float>(ViewX) - 60.f);
 const float Height = FMath::Max(180.f, static_cast<float>(ViewY) - 90.f);
 const float Left = (ViewX - Width) * 0.5f;
 const float Top = (ViewY - Height) * 0.5f;
 // Match DrawExplorationAtlas bounds and scale. Ignore title, footer and outside clicks.
 if (MouseX < Left+12.f || MouseX > Left+Width-12.f ||
     MouseY < Top+70.f || MouseY > Top+Height-16.f) return;
 const float Range = FMath::Clamp(WorldMapRange, 5000.f, 50000.f);
 const float Scale = FMath::Min(Width-40.f,Height-75.f)*0.5f/Range;
 if (Scale <= KINDA_SMALL_NUMBER) return;
 const FVector Origin = GetActorLocation();
 PersonalWaypoint = FVector(
  Origin.X+WorldMapPan.X+(MouseX-ViewX*0.5f)/Scale,
  Origin.Y+WorldMapPan.Y-(MouseY-ViewY*0.5f)/Scale,Origin.Z);
 bPersonalWaypointActive = true;
 SavePersonalWaypoint();
}
void AMoonfallHeroPawn::ClearPersonalWaypoint()
{
 if (bWorldMapOpen && !bOptionsOpen && bPersonalWaypointActive) {
  bPersonalWaypointActive = false;
  SavePersonalWaypoint();
 }
}
void AMoonfallHeroPawn::SavePersonalWaypoint()
{
 if (!GConfig) return;
 GConfig->SetBool(TEXT("Moonfall.Map"), TEXT("PersonalWaypointActive"), bPersonalWaypointActive, GGameUserSettingsIni);
 if (bPersonalWaypointActive) {
  GConfig->SetFloat(TEXT("Moonfall.Map"), TEXT("WaypointX"), PersonalWaypoint.X, GGameUserSettingsIni);
  GConfig->SetFloat(TEXT("Moonfall.Map"), TEXT("WaypointY"), PersonalWaypoint.Y, GGameUserSettingsIni);
  GConfig->SetFloat(TEXT("Moonfall.Map"), TEXT("WaypointZ"), PersonalWaypoint.Z, GGameUserSettingsIni);
 }
 GConfig->Flush(false, GGameUserSettingsIni);
}
void AMoonfallHeroPawn::SetQuestTarget(FVector WorldLocation)
{
 if (WorldLocation.ContainsNaN() || WorldLocation.GetAbsMax() > 100000000.f) return;
 QuestTarget = WorldLocation;
 bQuestTargetActive = true;
}
void AMoonfallHeroPawn::ClearQuestTarget()
{
 bQuestTargetActive = false;
}
void AMoonfallHeroPawn::BeginArenaBossQuest(FVector BossLocation)
{
 if (bArenaBossQuestComplete || bArenaBossQuestActive || bQuestTargetActive ||
     BossLocation.ContainsNaN() || BossLocation.GetAbsMax() > 100000000.f) return;
 SetQuestTarget(BossLocation);
 bArenaBossQuestActive = true;
}
void AMoonfallHeroPawn::CompleteArenaBossQuest()
{
 if (!bArenaBossQuestActive || bArenaBossQuestComplete) return;
 bArenaBossQuestActive = false;
 bArenaBossQuestComplete = true;
 ArenaBossQuestCompletedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
 ClearQuestTarget();
}
void AMoonfallHeroPawn::MoveForward(float Value)
{
 if (bOptionsOpen || bWorldMapOpen) return;
 if (Controller && !FMath::IsNearlyZero(Value))
 {
  const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
 }
}
void AMoonfallHeroPawn::MoveRight(float Value)
{
 if (bOptionsOpen || bWorldMapOpen) return;
 if (Controller && !FMath::IsNearlyZero(Value))
 {
  const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value);
 }
}
void AMoonfallHeroPawn::Turn(float Value) { if (!bOptionsOpen && !bWorldMapOpen) AddControllerYawInput(Value); }
void AMoonfallHeroPawn::LookUp(float Value) { if (!bOptionsOpen && !bWorldMapOpen) AddControllerPitchInput(Value); }
void AMoonfallHeroPawn::ZoomCameraIn() { if (!bOptionsOpen && !bWorldMapOpen) CameraDistance = FMath::Clamp(CameraDistance - 50.f, 250.f, 850.f); }
void AMoonfallHeroPawn::ZoomCameraOut() { if (!bOptionsOpen && !bWorldMapOpen) CameraDistance = FMath::Clamp(CameraDistance + 50.f, 250.f, 850.f); }
void AMoonfallHeroPawn::ZoomMinimapIn()
{
 if (bOptionsOpen) return;
 if (bWorldMapOpen) WorldMapRange = FMath::Clamp(WorldMapRange - 2500.f, 5000.f, 50000.f);
 else MinimapRange = FMath::Clamp(MinimapRange - 400.f, 1200.f, 4800.f);
}
void AMoonfallHeroPawn::ZoomMinimapOut()
{
 if (bOptionsOpen) return;
 if (bWorldMapOpen) WorldMapRange = FMath::Clamp(WorldMapRange + 2500.f, 5000.f, 50000.f);
 else MinimapRange = FMath::Clamp(MinimapRange + 400.f, 1200.f, 4800.f);
}
bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier)
{
 static const float Costs[6] = {16.f,13.f,22.f,25.f,18.f,14.f};
 static const float Cooldowns[6] = {0.8f,0.65f,1.2f,2.f,1.f,0.9f};
 if (Health <= 0.f || bOptionsOpen || bWorldMapOpen || !GetWorld() || School < 0 || School >= 6 || Hand < 0 || Hand > 1 || !FMath::IsFinite(Multiplier) || Multiplier <= 0.f || Multiplier > 2.f) return false;
 const float Time = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth) || MaxHealth <= 0.f || Health > MaxHealth || !FMath::IsFinite(MaxMana) || MaxMana <= 0.f || !FMath::IsFinite(Mana) || !FMath::IsFinite(SpellPowerMultiplier) || SpellPowerMultiplier <= 0.f || Mana < Costs[School] * Multiplier || Time - LastCastTime[Hand] < Cooldowns[School]) return false;
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
  Enemy->TakeSpellDamage((18.f + School * 5.f) * Multiplier * SpellPowerMultiplier * (1.f + FMath::Min(0.35f, 0.018f * WeaponTier)));
  const FColor SpellColors[6] = {FColor(245, 60, 39), FColor(65, 204, 245), FColor(168, 72, 236), FColor(72, 230, 130), FColor(245, 224, 64), FColor(76, 178, 68)};
  const FColor ImpactColor = SpellColors[School];
  DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 24.f * Multiplier, 12, ImpactColor, false, 0.22f, 0, 2.f);
  DrawDebugLine(GetWorld(), Origin, Hit.ImpactPoint, ImpactColor, false, 0.12f, 0, 1.5f);
 }
 Mana -= Costs[School] * Multiplier;
 LastCastTime[Hand] = Time;
 if (School == 3) {
  Health = FMath::Min(MaxHealth, Health + 24.f * Multiplier * SpellPowerMultiplier);
  OnHealthChanged(Health, MaxHealth);
 }
 OnSpellCast(School, Hand == 0);
 return true;
}
void AMoonfallHeroPawn::CastLeft() { if (bOptionsOpen || bWorldMapOpen) return; TryCast(LeftSpell, 0, 1.f); }
void AMoonfallHeroPawn::CastRight() { if (bOptionsOpen || bWorldMapOpen) return; TryCast(RightSpell, 1, 1.f); }
void AMoonfallHeroPawn::CastBoth()
{
 if (bOptionsOpen || bWorldMapOpen) return;
 if (LeftSpell != RightSpell || !GetWorld()) return;
 const float Time = GetWorld()->GetTimeSeconds();
 static const float Cooldowns[6] = {0.8f,0.65f,1.2f,2.f,1.f,0.9f};
 if (LeftSpell < 0 || LeftSpell >= 6 || RightSpell < 0 || RightSpell >= 6 || Time - LastCastTime[1] < Cooldowns[LeftSpell]) return;
 if (TryCast(LeftSpell, 0, 2.f))
 {
  LastCastTime[1] = LastCastTime[0];
  OnSpellCast(LeftSpell, false);
 }
}
void AMoonfallHeroPawn::Dodge()
{
 if (bOptionsOpen || bWorldMapOpen || Health <= 0.f || !GetWorld()) return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (Now < NextDodgeAt) return;
 NextDodgeAt = Now + DodgeCooldown;
 DamageImmunityUntil = Now + 0.24f;
 PerfectDodgeWindowUntil = Now + 0.14f;
 const FVector MoveDirection = GetLastMovementInputVector().GetSafeNormal2D();
 const FVector Direction = MoveDirection.IsNearlyZero() ? GetActorForwardVector() : MoveDirection;
 LaunchCharacter(Direction * 650.f, true, false);
}

void AMoonfallHeroPawn::RewardPerfectDodge() {
 if (!GetWorld() || Health <= 0.f) return;
 const float Now = GetWorld()->GetTimeSeconds();
 if (Now > PerfectDodgeWindowUntil || Now - LastPerfectDodgeRewardAt < 1.f) return;
 LastPerfectDodgeRewardAt = Now;
 PerfectDodgeWindowUntil = -1.f;
 Mana = FMath::Min(MaxMana, Mana + 8.f);
}

void AMoonfallHeroPawn::ApplyMoonfallDamage(float Amount) {
 if (Health <= 0.f || !FMath::IsFinite(Amount) || Amount <= 0.f || !GetWorld() || GetWorld()->GetTimeSeconds() < DamageImmunityUntil) return;
 const float PreviousHealth = Health;
 Health = FMath::Max(0.f, Health - Amount * (1.f - FMath::Min(0.28f, ArmorTier * 0.01f)));
 const float DamageTaken = PreviousHealth - Health;
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

void AMoonfallHeroPawn::OpenOptions()
{
 bOptionsOpen = !bOptionsOpen;
 if (bOptionsOpen) {
  bWorldMapOpen = false;
  if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->bShowMouseCursor = false;
 }
}
void AMoonfallHeroPawn::OptionsUp() { if (bWorldMapOpen) WorldMapPan.Y = FMath::Clamp(WorldMapPan.Y + 2000.f, -50000.f, 50000.f); else if (bOptionsOpen) OptionsSelection = (OptionsSelection + 6) % 7; }
void AMoonfallHeroPawn::OptionsDown() { if (bWorldMapOpen) WorldMapPan.Y = FMath::Clamp(WorldMapPan.Y - 2000.f, -50000.f, 50000.f); else if (bOptionsOpen) OptionsSelection = (OptionsSelection + 1) % 7; }
void AMoonfallHeroPawn::OptionsLeft() { if (bWorldMapOpen) WorldMapPan.X = FMath::Clamp(WorldMapPan.X - 2000.f, -50000.f, 50000.f); else AdjustOption(-1); }
void AMoonfallHeroPawn::OptionsRight() { if (bWorldMapOpen) WorldMapPan.X = FMath::Clamp(WorldMapPan.X + 2000.f, -50000.f, 50000.f); else AdjustOption(1); }
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

void AMoonfallHeroPawn::AwardEquipmentDrop(bool bEliteEnemy, bool bBossEnemy)
{
 if (Health <= 0.f) return;
 // Bounded deterministic loot, without spawned pickup actors or per-frame work.
 const int32 NewTier = FMath::Clamp(Level / 5 + (bBossEnemy ? 2 : (bEliteEnemy ? 1 : 0)), 1, 25);
 // Rarity: 0 common, 1 uncommon, 2 rare, 3 epic. Boss loot is more valuable.
 LastLootRarity = bBossEnemy ? 3 : (bEliteEnemy ? 2 : (Level >= 10 ? 1 : 0));
 LastLootTier = NewTier;
 LastLootSlot = bBossEnemy ? 2 : (bEliteEnemy ? 1 : 0);
 const int32 PreviousTier = LastLootSlot == 2 ? AccessoryTier : (LastLootSlot == 1 ? ArmorTier : WeaponTier);
 LastLootUpgrade = NewTier - PreviousTier;
 // Compact stable item code: slot, rarity, tier. Maximum 30 entries.
 const int32 ItemCode = LastLootSlot * 10000 + LastLootRarity * 1000 + NewTier;
 if (InventoryItemCodes.Num() >= 30) InventoryItemCodes.RemoveAt(0);
 InventoryItemCodes.Add(ItemCode);
 // Rotate awards across equipment categories; higher-tier items replace lower tiers.
 int32* Slot = bBossEnemy ? &AccessoryTier : (bEliteEnemy ? &ArmorTier : &WeaponTier);
 if (NewTier > *Slot) {
  const int32 Previous = *Slot;
  *Slot = NewTier;
  EquipmentTier = FMath::Max(WeaponTier, FMath::Max(ArmorTier, AccessoryTier));
  if (Slot == &ArmorTier) {
   MaxHealth += 2.f * (NewTier - Previous);
   Health = FMath::Min(MaxHealth, Health + 2.f * (NewTier - Previous));
   OnHealthChanged(Health, MaxHealth);
  }
 } else {
  Salvage = FMath::Min(9999, Salvage + (bBossEnemy ? 5 : (bEliteEnemy ? 2 : 1)));
 }
}
