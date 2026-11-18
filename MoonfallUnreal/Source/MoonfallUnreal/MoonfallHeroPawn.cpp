#include "MoonfallHeroPawn.h"
#include "MoonfallPersistentSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationInvokerComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ConfigCacheIni.h"
#include "Interfaces/IPluginManager.h"
#include "MoonfallEnemy.h"
#include "MoonfallDungeonLayout.h"
#include "MoonfallArena.h"
#include "MoonfallWorldNPC.h"
#include "MoonfallCheckpointShrine.h"
#include "MoonfallGroundLoot.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Math/RotationMatrix.h"
#include "Math/RandomStream.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CollisionQueryParams.h"
// Equipment stats are intentionally derived from equipped codes, never the
// inventory list: carrying loot cannot accidentally grant combat bonuses.
// Ignore malformed codes so externally edited Blueprint state fails closed.
namespace {
// Decode only the expected slot. Equipped fields can contain stale or
// externally edited data; they must not grant combat bonuses.
bool DecodeEquippedItem(int32 Code, int32 ExpectedSlot,
                        int32& OutTrait, int32& OutRarity, int32& OutTier)
{
 if (Code <= 0 || ExpectedSlot < 0 || ExpectedSlot > 2) return false;
 const int32 Trait = Code / 100000;
 const int32 Slot = (Code / 10000) % 10;
 const int32 Rarity = (Code / 1000) % 10;
 const int32 Tier = Code % 1000;
 if (Trait < 0 || Trait > 3 || Slot != ExpectedSlot
     || Rarity < 0 || Rarity > 5 || Tier < 1 || Tier > 25
     || (Rarity == 0 && Trait != 0)
     || Code != Trait * 100000 + Slot * 10000 + Rarity * 1000 + Tier)
  return false;
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
int32 ValidEquippedRarity(int32 Code, int32 Slot)
{
 int32 Trait = 0, Rarity = 0, Tier = 0;
 return DecodeEquippedItem(Code, Slot, Trait, Rarity, Tier) ? Rarity : 0;
}
float ValidEquippedTraitRarity(int32 Code, int32 Slot, int32 ExpectedTrait)
{
 int32 Trait = 0, Rarity = 0, Tier = 0;
 return DecodeEquippedItem(Code, Slot, Trait, Rarity, Tier) && Trait == ExpectedTrait
   ? static_cast<float>(Rarity) : 0.f;
}
}

int32 AMoonfallHeroPawn::GetValidatedEquippedTier(int32 Slot) const
{
 if (Slot < 0 || Slot > 2) return 0;
 const int32 Code = Slot == 0 ? EquippedWeaponCode
     : (Slot == 1 ? EquippedArmorCode : EquippedAccessoryCode);
 return InventoryItemCodes.Contains(Code) ? ValidEquippedTier(Code, Slot) : 0;
}

int32 AMoonfallHeroPawn::GetItemPower(int32 ItemCode) const
{
 // Use the same strict decoder as equipped combat stats. No save mutations.
 if (ItemCode <= 0) return 0;
 const int32 Slot = (ItemCode / 10000) % 10;
 if (Slot < 0 || Slot > 2) return 0;
 int32 Trait = 0, Rarity = 0, Tier = 0;
 if (!DecodeEquippedItem(ItemCode, Slot, Trait, Rarity, Tier)) return 0;
 return Tier * 100 + Rarity * 10 + (Trait > 0 ? 1 : 0);
}

int32 AMoonfallHeroPawn::GetEquipmentPower() const
{
 const int32 Codes[3] = {EquippedWeaponCode, EquippedArmorCode, EquippedAccessoryCode};
 int32 Total = 0;
 for (int32 Slot = 0; Slot < 3; ++Slot) {
  int32 Trait = 0, Rarity = 0, Tier = 0;
  if (InventoryItemCodes.Contains(Codes[Slot])
      && DecodeEquippedItem(Codes[Slot], Slot, Trait, Rarity, Tier))
   Total += GetItemPower(Codes[Slot]);
 }
 return Total;
}

int32 AMoonfallHeroPawn::GetItemPowerDelta(int32 ItemCode) const
{
 if (!InventoryItemCodes.Contains(ItemCode)) return 0;
 const int32 CandidatePower = GetItemPower(ItemCode);
 if (CandidatePower <= 0) return 0;
 const int32 Slot = (ItemCode / 10000) % 10;
 if (Slot < 0 || Slot > 2) return 0;
 const int32 CurrentCode = Slot == 0 ? EquippedWeaponCode
     : (Slot == 1 ? EquippedArmorCode : EquippedAccessoryCode);
 int32 Trait = 0, Rarity = 0, Tier = 0;
 const bool bCurrentOwnedAndValid = InventoryItemCodes.Contains(CurrentCode)
     && DecodeEquippedItem(CurrentCode, Slot, Trait, Rarity, Tier);
 return CandidatePower - (bCurrentOwnedAndValid ? GetItemPower(CurrentCode) : 0);
}

int32 AMoonfallHeroPawn::GetProjectedSetPieceCount(int32 Affix, int32 CandidateCode) const
{
 if (Affix < 1 || Affix > 3 || !InventoryItemCodes.Contains(CandidateCode)
     || GetItemPower(CandidateCode) <= 0) return 0;
 const int32 Slot = (CandidateCode / 10000) % 10;
 if (Slot < 0 || Slot > 2) return 0;
 int32 CandidateTrait = 0, CandidateRarity = 0, CandidateTier = 0;
 if (!DecodeEquippedItem(CandidateCode, Slot, CandidateTrait, CandidateRarity, CandidateTier))
  return 0;
 const int32 CurrentCode = Slot == 0 ? EquippedWeaponCode
     : (Slot == 1 ? EquippedArmorCode : EquippedAccessoryCode);
 int32 PreviousTrait = 0, PreviousRarity = 0, PreviousTier = 0;
 const bool bCurrentValid = InventoryItemCodes.Contains(CurrentCode)
     && DecodeEquippedItem(CurrentCode, Slot, PreviousTrait, PreviousRarity, PreviousTier);
 return FMath::Clamp(GetSetPieceCount(Affix)
     - (bCurrentValid && PreviousRarity >= 1 && PreviousTrait == Affix ? 1 : 0)
     + (CandidateRarity >= 1 && CandidateTrait == Affix ? 1 : 0), 0, 3);
}

bool AMoonfallHeroPawn::HasLegendaryPerk(int32 Slot) const
{
 if (Slot < 0 || Slot > 2) return false;
 const int32 Code = Slot == 0 ? EquippedWeaponCode
     : (Slot == 1 ? EquippedArmorCode : EquippedAccessoryCode);
 if (!InventoryItemCodes.Contains(Code)) return false;
 int32 Trait = 0, Rarity = 0, Tier = 0;
 return DecodeEquippedItem(Code, Slot, Trait, Rarity, Tier) && Rarity == 5;
}

int32 AMoonfallHeroPawn::GetSetPieceCount(int32 Affix) const
{
 if (Affix < 1 || Affix > 3) return 0;
 const int32 Codes[3] = {EquippedWeaponCode, EquippedArmorCode, EquippedAccessoryCode};
 int32 Pieces = 0;
 for (int32 Slot = 0; Slot < 3; ++Slot) {
  int32 Trait = 0, Rarity = 0, Tier = 0;
  if (InventoryItemCodes.Contains(Codes[Slot])
      && DecodeEquippedItem(Codes[Slot], Slot, Trait, Rarity, Tier)
      && Trait == Affix && Rarity >= 1)
   ++Pieces;
 }
 return Pieces;
}

int32 AMoonfallHeroPawn::GetMight() const
{
 return FMath::Clamp(10 + FMath::Max(0, Level - 1) / 2
   + (InventoryItemCodes.Contains(EquippedWeaponCode)
     ? ValidEquippedTier(EquippedWeaponCode, 0) : 0) + GetRaceMightBonus(), 10, 100);
}

int32 AMoonfallHeroPawn::GetInsight() const
{
 return FMath::Clamp(10 + FMath::Max(0, Level - 1) / 2
   + (InventoryItemCodes.Contains(EquippedAccessoryCode)
     ? ValidEquippedTier(EquippedAccessoryCode, 2) : 0) + GetRaceInsightBonus(), 10, 100);
}

int32 AMoonfallHeroPawn::GetResolve() const
{
 return FMath::Clamp(10 + FMath::Max(0, Level - 1) / 2
   + (InventoryItemCodes.Contains(EquippedArmorCode)
     ? ValidEquippedTier(EquippedArmorCode, 1) : 0) + GetRaceResolveBonus(), 10, 100);
}

float AMoonfallHeroPawn::GetCriticalChance() const
{
 const float TraitCrit = InventoryItemCodes.Contains(EquippedWeaponCode)
   ? ValidEquippedTraitRarity(EquippedWeaponCode, 0, 1) * 0.008f : 0.f;
 return FMath::Clamp(0.05f + TraitCrit + (GetSetPieceCount(1) >= 2 ? 0.03f : 0.f),
   0.05f, 0.25f);
}

float AMoonfallHeroPawn::GetHasteBonus() const
{
 const float TraitHaste = InventoryItemCodes.Contains(EquippedAccessoryCode)
   ? ValidEquippedTraitRarity(EquippedAccessoryCode, 2, 3) * 0.006f : 0.f;
 return FMath::Clamp(TraitHaste + (GetSetPieceCount(3) >= 2 ? 0.04f : 0.f)
   + (HasLegendaryPerk(2) ? 0.025f : 0.f),
   0.f, 0.20f);
}

float AMoonfallHeroPawn::GetMasteryBonus() const
{
 const float GearMastery = FMath::Clamp(
   ((InventoryItemCodes.Contains(EquippedWeaponCode) ? ValidEquippedTier(EquippedWeaponCode, 0) : 0)
    + (InventoryItemCodes.Contains(EquippedArmorCode) ? ValidEquippedTier(EquippedArmorCode, 1) : 0)
    + (InventoryItemCodes.Contains(EquippedAccessoryCode) ? ValidEquippedTier(EquippedAccessoryCode, 2) : 0)) * 0.0015f, 0.f, 0.12f);
 return FMath::Clamp(GearMastery + (GetSetPieceCount(1) >= 3 ? 0.04f : 0.f),
   0.f, 0.16f);
}

float AMoonfallHeroPawn::GetVersatilityBonus() const
{
 const float TraitVersatility = InventoryItemCodes.Contains(EquippedArmorCode)
   ? ValidEquippedTraitRarity(EquippedArmorCode, 1, 2) * 0.005f : 0.f;
 return FMath::Clamp(TraitVersatility + (GetSetPieceCount(2) >= 2 ? 0.03f : 0.f),
   0.f, 0.12f);
}

AMoonfallHeroPawn::AMoonfallHeroPawn()
{
 PrimaryActorTick.bCanEverTick = true;
 DungeonNavigationInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(
   TEXT("DungeonNavigationInvoker"));
 // Small active navmesh radius helps keep 30 long dungeon lanes affordable.
 DungeonNavigationInvoker->SetGenerationRadii(3000.f, 5000.f);
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
void AMoonfallHeroPawn::CacheStartingSanctuaryAnchor() {
 if (bHasStartingSanctuaryAnchor || !GetWorld()) return;
 for (TActorIterator<AMoonfallArena> It(GetWorld()); It; ++It) {
  const AMoonfallArena* Arena = *It;
  if (!IsValid(Arena) || !Arena->bEnableStartingSanctuary) continue;
  const FVector Anchor = Arena->GetStartingSanctuaryLocation();
  if (!Anchor.ContainsNaN() && Anchor.SizeSquared() < 1.0e14f) {
   StartingSanctuaryAnchor = Anchor;
   bHasStartingSanctuaryAnchor = true;
  }
  break; // Never follow a different authored arena on later HUD frames.
 }
}

bool AMoonfallHeroPawn::IsNearStartingSanctuary() const {
 if (!bHasStartingSanctuaryAnchor || GetActorLocation().ContainsNaN()) return false;
 return FVector::DistSquared2D(GetActorLocation(), StartingSanctuaryAnchor)
   <= FMath::Square(1450.f);
}

void AMoonfallHeroPawn::TryPlaceNewCharacterAtSanctuary() {
 if (bStartingSanctuaryPlaced || !GetWorld() || bPersistentSaveWritesBlocked
     || UGameplayStatics::DoesSaveGameExist(MoonfallWorldSlot, 0)) return;
 for (TActorIterator<AMoonfallArena> It(GetWorld()); It; ++It) {
  const AMoonfallArena* Arena = *It;
  if (!IsValid(Arena) || !Arena->bEnableStartingSanctuary) continue;
  const FVector Start = Arena->GetStartingSanctuaryLocation();
  if (!Start.ContainsNaN()
      && TeleportTo(Start, GetActorRotation(), false, false)) {
   InitialSpawnTransform = GetActorTransform();
   bHasInitialSpawnTransform = true;
   RespawnCheckpointLocation = GetActorLocation();
   bHasRespawnCheckpointLocation = true;
   bStartingSanctuaryPlaced = true;
   StartingSanctuaryAnchor = Start;
   bHasStartingSanctuaryAnchor = true;
   // Shrine discovery remains in the shrine's own overlap/spawn checks.
   for (TActorIterator<AMoonfallCheckpointShrine> ShrineIt(GetWorld()); ShrineIt; ++ShrineIt)
    if (IsValid(*ShrineIt)) ShrineIt->CheckHeroAtSpawn(this);
  }
  break; // Never move between multiple authored arenas.
 }
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
 if (GConfig) GConfig->GetBool(TEXT("Moonfall.Gameplay"), TEXT("AutoLootEnabled"), bAutoLootEnabled, GGameUserSettingsIni);
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
 // Remember the level-authored spawn before a save load can relocate us.
 // A map with no PlayerStart may still have a valid placed hero pawn.
 const FVector SpawnLocation = GetActorLocation();
 bHasInitialSpawnTransform = !SpawnLocation.ContainsNaN()
     && SpawnLocation.SizeSquared() < 1.0e14f;
 if (bHasInitialSpawnTransform) {
  InitialSpawnTransform = GetActorTransform();
  RespawnCheckpointLocation = SpawnLocation;
  bHasRespawnCheckpointLocation = true;
 }
 const bool bLoadedWorldProgress = LoadWorldProgress();
 // Recheck once on the next game tick: GameMode may create the arena after
 // this hero's BeginPlay. Neither pass can override a returning player's save.
 if (!bLoadedWorldProgress
     && !UGameplayStatics::DoesSaveGameExist(MoonfallWorldSlot, 0)
     && !bPersistentSaveWritesBlocked && GetWorld()) {
  TryPlaceNewCharacterAtSanctuary();
  GetWorldTimerManager().SetTimerForNextTick(
    this, &AMoonfallHeroPawn::TryPlaceNewCharacterAtSanctuary);
 }
 bWorldProgressInitialized = true;
 // Returning saves also need the Sanctuary location for contextual guidance.
 // Find it once now and retry once next tick if the GameMode spawns the arena later.
 CacheStartingSanctuaryAnchor();
 if (!bHasStartingSanctuaryAnchor && GetWorld()) {
  GetWorldTimerManager().SetTimerForNextTick(
    this, &AMoonfallHeroPawn::CacheStartingSanctuaryAnchor);
 }
 if (!bPersistentSaveWritesBlocked) RestorePendingWorldEquipmentDrops();
 // When a shrine spawned before this pawn, BeginPlay overlap notifications
 // may have happened before save load was complete. A one-time scan after
 // the saved position and shrine ledger are restored closes that gap.
 if (GetWorld()) {
  for (TActorIterator<AMoonfallCheckpointShrine> It(GetWorld()); It; ++It)
   if (IsValid(*It)) It->CheckHeroAtSpawn(this);
 }
}
void AMoonfallHeroPawn::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 // A nonfinite frame delta must not poison the persistent autosave or
 // defeat/respawn timers, leaving the hero unable to save or respawn.
 const float SafeTimerDelta = FMath::IsFinite(DeltaSeconds)
     ? FMath::Clamp(DeltaSeconds, 0.f, 0.25f) : 0.f;
 // Throttled automatic pickup uses the same authoritative world-loot collection API as manual pickup.
 if (bAutoLootEnabled && FMath::IsFinite(DeltaSeconds) && DeltaSeconds > 0.f && Health > 0.f
     && !bOptionsOpen && !bInventoryOpen && !bAtlasOpen) {
  AutoLootElapsed += FMath::Min(DeltaSeconds, 1.f);
  if (AutoLootElapsed >= 0.35f) {
   AutoLootElapsed = FMath::Fmod(AutoLootElapsed, 0.35f);
   PickupNearestGroundLoot();
  }
 } else AutoLootElapsed = 0.f;
 // Only track pointer motion while dragging the inventory scrollbar.
 if (bInventoryScrollbarDragging) {
  if (!bInventoryOpen || bCharacterStatsOpen || bOptionsOpen || Health <= 0.f)
   bInventoryScrollbarDragging = false;
  else
   ScrollInventoryToMouseTrack();
 }
 // A streaming/despawned resident must not leave player cursor or input locked.
 if (bResidentContextOpen) {
  const AMoonfallWorldNPC* Resident = FocusedResident.Get();
  // Check cheap safety/range every tick. Full LOS is revalidated only
  // on actual choices, avoiding a world trace on every player frame.
  const float Range = IsValid(Resident) && FMath::IsFinite(Resident->InteractRange)
      ? FMath::Clamp(Resident->InteractRange, 50.f, 400.f) : 0.f;
  if (!IsValid(Resident) || Health <= 0.f || bOptionsOpen || bInventoryOpen ||
      Range <= 0.f || GetActorLocation().ContainsNaN() ||
      Resident->GetActorLocation().ContainsNaN() ||
      FVector::DistSquared(GetActorLocation(), Resident->GetActorLocation()) > FMath::Square(Range))
   CloseResidentContext();
 }
 // Keep one save slot forever; only alive characters overwrite the checkpoint.
 if (Health > 0.f) {
  RevealAtlasAtWorldLocation();
  DefeatElapsedSeconds = 0.f;
  SecondsSinceLastWorldSave += SafeTimerDelta;
  if (SecondsSinceLastWorldSave >= 2.f && GetWorld()
      && GetWorld()->GetTimeSeconds() >= NextSafeCheckpointAt) {
   if (SaveWorldProgress()) SecondsSinceLastWorldSave = 0.f;
  }
 } else {
  DefeatElapsedSeconds += SafeTimerDelta;
  if (DefeatElapsedSeconds >= 3.f) RespawnAtCheckpoint();
 }
 UpdateNativeFinisher(DeltaSeconds);
 // Expire the visible combo stage even if the player stops attacking.
 // Also discard defeated or destroyed targets to avoid a stale combo HUD.
 if (HitComboStage > 0 && GetWorld()) {
  const float ComboNow = GetWorld()->GetTimeSeconds();
  const float ComboWindow = FMath::IsFinite(HitComboWindowSeconds)
      ? FMath::Clamp(HitComboWindowSeconds, 0.5f, 4.f) : 2.f;
  const AMoonfallEnemy* ComboTarget = LastHitComboTarget.Get();
  if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen || !IsValid(ComboTarget)
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
 const float MenuRecoveryBoost = (bOptionsOpen || bInventoryOpen || bAtlasOpen) && FMath::IsFinite(MenuRecoveryMultiplier)
     ? FMath::Clamp(MenuRecoveryMultiplier, 1.f, 2.f) : 1.f;
 // WoW-style pacing: combat mana regenerates more slowly; resting restores full pace.
 const bool bInCombatRecoveryWindow = GetWorld() && GetWorld()->GetTimeSeconds() < CombatRecoveryEndsAt;
 const float CombatManaFactor = bInCombatRecoveryWindow
     ? (FMath::IsFinite(CombatManaRegenMultiplier)
         ? FMath::Clamp(CombatManaRegenMultiplier, 0.f, 1.f) : 0.5f) : 1.f;
 if (Health > 0.f && FMath::IsFinite(Mana) && FMath::IsFinite(MaxMana) && MaxMana > 0.f
     && FMath::IsFinite(ManaRegenPerSecond) && ManaRegenPerSecond > 0.f && Mana >= 0.f && Mana < MaxMana)
  Mana = FMath::Min(MaxMana, Mana + FMath::Clamp(ManaRegenPerSecond, 0.f, 250.f) * (1.f + GetRaceManaRegenBonus()) * MenuRecoveryBoost * CombatManaFactor
      * (1.f + (InventoryItemCodes.Contains(EquippedAccessoryCode)
          ? FMath::Clamp(ValidEquippedTier(EquippedAccessoryCode, 2) * 0.012f, 0.f, 0.25f) : 0.f)
        + (InventoryItemCodes.Contains(EquippedAccessoryCode) && ValidEquippedTraitRarity(EquippedAccessoryCode, 2, 3) > 0.f ? 0.08f : 0.f)
        + (InventoryItemCodes.Contains(EquippedAccessoryCode)
          ? FMath::Clamp(ValidEquippedRarity(EquippedAccessoryCode, 2) * 0.02f, 0.f, 0.10f) : 0.f)
        + (GetSetPieceCount(3) >= 3 ? 0.06f : 0.f)
        + FMath::Clamp(GetInsight() * 0.002f, 0.f, 0.20f))
      * SafeRecoveryDelta);
 // Recovery in any menu, including the Atlas, is bounded and never revives a defeated hero.
 if ((bOptionsOpen || bInventoryOpen || bAtlasOpen) && Health > 0.f && FMath::IsFinite(Health)
     && FMath::IsFinite(MaxHealth) && MaxHealth > 0.f && Health < MaxHealth
     && FMath::IsFinite(MenuHealthRegenPerSecond) && MenuHealthRegenPerSecond > 0.f
     && GetWorld() && GetWorld()->GetTimeSeconds() >= MenuHealthRegenStartsAt
     && !bInCombatRecoveryWindow) {
  const float BeforeRegen = Health;
  Health = FMath::Min(MaxHealth, Health + FMath::Clamp(MenuHealthRegenPerSecond, 0.f, 20.f)
      * SafeRecoveryDelta);
  if (Health > BeforeRegen) OnHealthChanged(Health, MaxHealth);
 }
 // Sprint drains only while moving, and never drains while a menu is open.
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
 const bool bCanSprint = bSprintHeld && !bSprintExhausted && !bGuarding && !bOptionsOpen && !bInventoryOpen && !bAtlasOpen && Health > 0.f
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
  Stamina = FMath::Clamp(Stamina + FMath::Clamp(StaminaRegenPerSecond, 0.f, 250.f) * (1.f + GetRaceStaminaRegenBonus()) * StaminaDelta * MenuRecoveryBoost
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
 Input->BindAction(TEXT("ResidentMouseClick"), IE_Pressed, this, &AMoonfallHeroPawn::ResidentContextMouseClick);
 Input->BindAction(TEXT("ResidentMouseClick"), IE_Released, this, &AMoonfallHeroPawn::ReleaseInventoryPointer);
 Input->BindAction(TEXT("CastRight"), IE_Pressed, this, &AMoonfallHeroPawn::CastRight);
 Input->BindAction(TEXT("CastRightMouse"), IE_Pressed, this, &AMoonfallHeroPawn::CastRightMouse);
 Input->BindAction(TEXT("ResidentContext"), IE_Pressed, this, &AMoonfallHeroPawn::OpenFocusedResidentContext);
 Input->BindAction(TEXT("DualCast"), IE_Pressed, this, &AMoonfallHeroPawn::CastBoth);
 Input->BindAction(TEXT("Dodge"), IE_Pressed, this, &AMoonfallHeroPawn::Dodge);
 Input->BindAction(TEXT("Sprint"), IE_Pressed, this, &AMoonfallHeroPawn::BeginSprint);
 Input->BindAction(TEXT("Sprint"), IE_Released, this, &AMoonfallHeroPawn::EndSprint);
 Input->BindAction(TEXT("Guard"), IE_Pressed, this, &AMoonfallHeroPawn::BeginGuard);
 Input->BindAction(TEXT("Guard"), IE_Released, this, &AMoonfallHeroPawn::EndGuard);
 Input->BindAction(TEXT("GraphicsQuality"), IE_Pressed, this, &AMoonfallHeroPawn::CycleGraphicsQuality);
 Input->BindAction(TEXT("DisplayMode"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleDisplayMode);
 Input->BindAction(TEXT("Options"), IE_Pressed, this, &AMoonfallHeroPawn::OpenOptions);
 Input->BindAction(TEXT("Atlas"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleAtlas);
 Input->BindAction(TEXT("Inventory"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleInventory);
 Input->BindAction(TEXT("CharacterStats"), IE_Pressed, this, &AMoonfallHeroPawn::ToggleCharacterStats);
 Input->BindAction(TEXT("InventorySalvage"), IE_Pressed, this, &AMoonfallHeroPawn::InventorySalvage);
 Input->BindAction(TEXT("UpgradeGear"), IE_Pressed, this, &AMoonfallHeroPawn::UpgradeSelectedInventoryItem);
 Input->BindAction(TEXT("ForgeFive"), IE_Pressed, this, &AMoonfallHeroPawn::ForgeFiveSelectedInventoryItem);
 Input->BindAction(TEXT("AscendGear"), IE_Pressed, this, &AMoonfallHeroPawn::AscendSelectedInventoryItem);
 Input->BindAction(TEXT("ReforgeGear"), IE_Pressed, this, &AMoonfallHeroPawn::ReforgeSelectedInventoryItem);
 Input->BindAction(TEXT("EquipBest"), IE_Pressed, this, &AMoonfallHeroPawn::EquipBestInventoryAction);
 Input->BindAction(TEXT("PickupLoot"), IE_Pressed, this, &AMoonfallHeroPawn::PickupNearestGroundLoot);
 Input->BindAction(TEXT("RacePower"), IE_Pressed, this, &AMoonfallHeroPawn::TriggerRacePower);
 Input->BindAction(TEXT("OptionsUp"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsUp);
 Input->BindAction(TEXT("OptionsDown"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsDown);
 Input->BindAction(TEXT("OptionsLeft"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsLeft);
 Input->BindAction(TEXT("OptionsRight"), IE_Pressed, this, &AMoonfallHeroPawn::OptionsRight);
}
TArray<int32> AMoonfallHeroPawn::GetInventoryDisplayOrder() const
{
 TArray<int32> Order;
 const int32 Count = InventoryItemCodes.Num();
 Order.Reserve(Count);
 for (int32 Index = 0; Index < Count; ++Index) Order.Add(Index);
 if (InventorySortMode == 0) return Order;
 // Sort only a temporary index array. Persistent inventory and equipped slots
 // never change. Original position is the stable tie-breaker for duplicates.
 Order.Sort([this](int32 A, int32 B) {
  const int32 CodeA = InventoryItemCodes[A], CodeB = InventoryItemCodes[B];
  const int32 SlotA = CodeA > 0 ? (CodeA / 10000) % 10 : 99;
  const int32 SlotB = CodeB > 0 ? (CodeB / 10000) % 10 : 99;
  const int32 RarityA = CodeA > 0 ? (CodeA / 1000) % 10 : -1;
  const int32 RarityB = CodeB > 0 ? (CodeB / 1000) % 10 : -1;
  const int32 TierA = CodeA > 0 ? CodeA % 1000 : -1;
  const int32 TierB = CodeB > 0 ? CodeB % 1000 : -1;
  if (InventorySortMode == 1) {
   if (RarityA != RarityB) return RarityA > RarityB;
   if (TierA != TierB) return TierA > TierB;
   if (SlotA != SlotB) return SlotA < SlotB;
  } else {
   if (SlotA != SlotB) return SlotA < SlotB;
   if (RarityA != RarityB) return RarityA > RarityB;
   if (TierA != TierB) return TierA > TierB;
  }
  return A < B;
 });
 return Order;
}

bool AMoonfallHeroPawn::ScrollInventoryToMouseTrack()
{
 if (Health <= 0.f || !bInventoryOpen || bCharacterStatsOpen || bOptionsOpen)
  return false;
 const APlayerController* PC = Cast<APlayerController>(Controller);
 if (!PC || !PC->bShowMouseCursor) return false;
 int32 Width = 0, Height = 0;
 float MouseX = 0.f, MouseY = 0.f;
 PC->GetViewportSize(Width, Height);
 if (Width <= 0 || Height <= 0 || !PC->GetMousePosition(MouseX, MouseY))
  return false;
 const int32 Rows = GetInventoryVisibleRows();
 const int32 Count = InventoryItemCodes.Num();
 // A bag may shrink while dragging after a salvage or item transfer.
 // Cancel the drag rather than keeping a stale scrollbar gesture alive.
 if (Count <= Rows) {
  bInventoryScrollbarDragging = false;
  InventoryViewStart = 0;
  return false;
 }
 const float PanelWidth = FMath::Min(720.f, FMath::Max(280.f,
   FMath::Min(float(Width) - 24.f, float(Width) * 0.48f)));
 const float PanelHeight = FMath::Min(860.f, FMath::Max(280.f, float(Height) - 24.f));
 const float Left = float(Width) - PanelWidth - 12.f;
 const float Top = (float(Height) - PanelHeight) * 0.5f;
 const float TrackX = Left + PanelWidth - 11.f;
 const float TrackY = Top + 145.f;
 const float RowSpacing = FMath::Max(21.f, (PanelHeight - 390.f) / Rows);
 const float TrackHeight = RowSpacing * Rows;
 // Initial click requires the track. Once dragging, clamped vertical motion
 // continues even when the cursor moves horizontally beyond its narrow track.
 if (!bInventoryScrollbarDragging &&
     (MouseX < TrackX - 8.f || MouseX > TrackX + 8.f ||
      MouseY < TrackY || MouseY > TrackY + TrackHeight))
  return false;
 bInventoryScrollbarDragging = true;
 const float ThumbHeight = FMath::Max(14.f, TrackHeight * Rows / Count);
 const float Travel = FMath::Max(1.f, TrackHeight - ThumbHeight);
 const float Fraction = FMath::Clamp((MouseY - TrackY - ThumbHeight * 0.5f) / Travel, 0.f, 1.f);
 InventoryViewStart = FMath::Clamp(FMath::RoundToInt(Fraction * (Count - Rows)), 0, Count - Rows);
 return true;
}

bool AMoonfallHeroPawn::HitTestInventoryCloseButton() const
{
 if (Health <= 0.f || !bInventoryOpen || bOptionsOpen)
  return false;
 const APlayerController* PC = Cast<APlayerController>(Controller);
 if (!PC || !PC->bShowMouseCursor) return false;
 int32 Width = 0, Height = 0;
 float MouseX = 0.f, MouseY = 0.f;
 PC->GetViewportSize(Width, Height);
 if (Width <= 0 || Height <= 0 || !PC->GetMousePosition(MouseX, MouseY))
  return false;
 const float PanelWidth = FMath::Min(720.f, FMath::Max(280.f,
   FMath::Min(float(Width) - 24.f, float(Width) * 0.48f)));
 const float PanelHeight = FMath::Min(860.f, FMath::Max(280.f, float(Height) - 24.f));
 const float Left = float(Width) - PanelWidth - 12.f;
 const float Top = (float(Height) - PanelHeight) * 0.5f;
 const float SortWidth = PanelWidth < 550.f ? 105.f : 172.f;
 const float SortX = Left + PanelWidth - SortWidth - 14.f;
 const float CloseX = SortX - 36.f;
 return MouseX >= CloseX && MouseX <= CloseX + 28.f &&
        MouseY >= Top + 14.f && MouseY <= Top + 42.f;
}

bool AMoonfallHeroPawn::HitTestInventorySortButton() const
{
 if (Health <= 0.f || !bInventoryOpen || bCharacterStatsOpen || bOptionsOpen)
  return false;
 const APlayerController* PC = Cast<APlayerController>(Controller);
 if (!PC || !PC->bShowMouseCursor) return false;
 int32 Width = 0, Height = 0;
 float MouseX = 0.f, MouseY = 0.f;
 PC->GetViewportSize(Width, Height);
 if (Width <= 0 || Height <= 0 || !PC->GetMousePosition(MouseX, MouseY)) return false;
 const float PanelWidth = FMath::Min(720.f, FMath::Max(280.f, FMath::Min(float(Width) - 24.f, float(Width) * 0.48f)));
 const float PanelHeight = FMath::Min(860.f, FMath::Max(280.f, float(Height) - 24.f));
 // Right-docked inventory. Match MoonfallCombatHUD Canvas geometry exactly.
 const float Left = float(Width) - PanelWidth - 12.f;
 const float Top = (float(Height) - PanelHeight) * 0.5f;
 const float SortWidth = PanelWidth < 550.f ? 105.f : 172.f;
 const float SortX = Left + PanelWidth - SortWidth - 14.f;
 const float SortY = Top + 14.f;
 return MouseX >= SortX && MouseX <= SortX + SortWidth &&
        MouseY >= SortY && MouseY <= SortY + 28.f;
}

bool AMoonfallHeroPawn::HitTestInventoryEquipButton() const
{
 if (Health <= 0.f || !bInventoryOpen || bCharacterStatsOpen || bOptionsOpen
     || !InventoryItemCodes.IsValidIndex(InventorySelection)) return false;
 const APlayerController* PC = Cast<APlayerController>(Controller);
 if (!PC || !PC->bShowMouseCursor) return false;
 int32 Width = 0, Height = 0;
 float MouseX = 0.f, MouseY = 0.f;
 PC->GetViewportSize(Width, Height);
 if (Width <= 0 || Height <= 0 || !PC->GetMousePosition(MouseX, MouseY)) return false;
 const float PanelWidth = FMath::Min(720.f, FMath::Max(280.f,
     FMath::Min(float(Width) - 24.f, float(Width) * 0.48f)));
 if (PanelWidth < 550.f) return false; // compact layout leaves this button hidden
 const float PanelHeight = FMath::Min(860.f, FMath::Max(280.f, float(Height) - 24.f));
 const float Left = float(Width) - PanelWidth - 12.f;
 const float Top = (float(Height) - PanelHeight) * 0.5f;
 const float EquipX = Left + PanelWidth - 186.f;
 const float EquipY = Top + 87.f;
 return MouseX >= EquipX && MouseX <= EquipX + 172.f &&
        MouseY >= EquipY && MouseY <= EquipY + 30.f;
}

int32 AMoonfallHeroPawn::GetInventoryVisibleRows() const
{
 int32 ViewWidth = 0;
 int32 ViewHeight = 720;
 if (APlayerController* Controller = Cast<APlayerController>(GetController()))
  Controller->GetViewportSize(ViewWidth, ViewHeight);
 // Match the Canvas panel height and reserve footer/stat space.
 const float PanelHeight = FMath::Min(860.f, FMath::Max(280.f, float(ViewHeight) - 24.f));
 return FMath::Clamp(FMath::FloorToInt((PanelHeight - 390.f) / 25.f), 2, 20);
}

void AMoonfallHeroPawn::KeepInventorySelectionVisible()
{
 const int32 Count = InventoryItemCodes.Num();
 InventorySelection = FMath::Clamp(InventorySelection, 0, FMath::Max(0, Count - 1));
 const int32 Rows = GetInventoryVisibleRows();
 const int32 MaxStart = FMath::Max(0, Count - Rows);
 const int32 DisplaySelection = GetInventoryDisplayOrder().Find(InventorySelection);
 const int32 SelectedRow = DisplaySelection == INDEX_NONE ? 0 : DisplaySelection;
 InventoryViewStart = FMath::Clamp(InventoryViewStart, 0, MaxStart);
 if (SelectedRow < InventoryViewStart)
  InventoryViewStart = SelectedRow;
 else if (SelectedRow >= InventoryViewStart + Rows)
  InventoryViewStart = SelectedRow - Rows + 1;
 InventoryViewStart = FMath::Clamp(InventoryViewStart, 0, MaxStart);
}

int32 AMoonfallHeroPawn::HitTestInventoryRow() const
{
 if (Health <= 0.f || !bInventoryOpen || bCharacterStatsOpen || bOptionsOpen)
  return INDEX_NONE;
 const APlayerController* PC = Cast<APlayerController>(Controller);
 if (!PC || !PC->bShowMouseCursor) return INDEX_NONE;
 int32 Width = 0, Height = 0;
 float MouseX = 0.f, MouseY = 0.f;
 PC->GetViewportSize(Width, Height);
 if (Width <= 0 || Height <= 0 || !PC->GetMousePosition(MouseX, MouseY))
  return INDEX_NONE;
 const float PanelWidth = FMath::Min(720.f, FMath::Max(280.f, FMath::Min(float(Width) - 24.f, float(Width) * 0.48f)));
 const float PanelHeight = FMath::Min(860.f, FMath::Max(280.f, float(Height) - 24.f));
 // Right-docked inventory. Match MoonfallCombatHUD Canvas geometry exactly.
 const float Left = float(Width) - PanelWidth - 12.f;
 const float Top = (float(Height) - PanelHeight) * 0.5f;
 const int32 Rows = GetInventoryVisibleRows();
 const int32 Count = InventoryItemCodes.Num();
 const int32 Start = FMath::Clamp(InventoryViewStart, 0, FMath::Max(0, Count - Rows));
 const int32 End = FMath::Min(Count, Start + Rows);
 const float Spacing = FMath::Max(21.f, (PanelHeight - 390.f) / Rows);
 const float FirstY = Top + 145.f;
 if (MouseX < Left + 12.f || MouseX > Left + PanelWidth - 16.f ||
     MouseY < FirstY - 3.f || MouseY >= FirstY + (End - Start) * Spacing)
  return INDEX_NONE;
 const int32 RelativeRow = FMath::FloorToInt((MouseY - FirstY) / Spacing);
 const int32 DisplayPosition = Start + RelativeRow;
 const TArray<int32> Order = GetInventoryDisplayOrder();
 return DisplayPosition >= Start && DisplayPosition < End && Order.IsValidIndex(DisplayPosition)
   && InventoryItemCodes.IsValidIndex(Order[DisplayPosition])
   ? Order[DisplayPosition] : INDEX_NONE;
}

void AMoonfallHeroPawn::ZoomCameraIn() {
 if (bAtlasOpen) { AtlasZoom = FMath::Clamp((FMath::IsFinite(AtlasZoom) ? AtlasZoom : 1.f) + 0.2f, 0.8f, 1.8f); return; }
 if (Health > 0.f && !bOptionsOpen && bInventoryOpen && !bCharacterStatsOpen) {
  InventoryViewStart = FMath::Max(0, FMath::Min(InventoryViewStart, FMath::Max(0, InventoryItemCodes.Num() - GetInventoryVisibleRows())) - 1);
  return;
 }
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen) return;
 CameraDistance = FMath::Clamp((FMath::IsFinite(CameraDistance) ? CameraDistance : 450.f) - 65.f, 250.f, 850.f);
}
void AMoonfallHeroPawn::ZoomCameraOut() {
 if (bAtlasOpen) { AtlasZoom = FMath::Clamp((FMath::IsFinite(AtlasZoom) ? AtlasZoom : 1.f) - 0.2f, 0.8f, 1.8f); return; }
 if (Health > 0.f && !bOptionsOpen && bInventoryOpen && !bCharacterStatsOpen) {
  InventoryViewStart = FMath::Min(FMath::Max(0, InventoryItemCodes.Num() - GetInventoryVisibleRows()), InventoryViewStart + 1);
  return;
 }
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen) return;
 CameraDistance = FMath::Clamp((FMath::IsFinite(CameraDistance) ? CameraDistance : 450.f) + 65.f, 250.f, 850.f);
}
void AMoonfallHeroPawn::TryJump()
{
 if (FocusedResident.IsValid() && !bOptionsOpen && !bInventoryOpen && !bAtlasOpen) {
  ChooseResidentContextAction(ResidentContextSelection);
  return;
 }
 // Jump must obey the same defeated/menu input locks as Dodge and Cast.
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen) return;
 Jump();
}
void AMoonfallHeroPawn::MoveForward(float Value)
{
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen || FocusedResident.IsValid() || !FMath::IsFinite(Value)) return;
 if (Controller && !FMath::IsNearlyZero(Value))
 {
  const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
 }
}
void AMoonfallHeroPawn::MoveRight(float Value)
{
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen || FocusedResident.IsValid() || !FMath::IsFinite(Value)) return;
 if (Controller && !FMath::IsNearlyZero(Value))
 {
  const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
  AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value);
 }
}
void AMoonfallHeroPawn::Turn(float Value) { if (Health > 0.f && !bOptionsOpen && !bInventoryOpen && !bAtlasOpen && !bResidentContextOpen && FMath::IsFinite(Value)) AddControllerYawInput(Value); }
void AMoonfallHeroPawn::LookUp(float Value) { if (Health > 0.f && !bOptionsOpen && !bInventoryOpen && !bAtlasOpen && !bResidentContextOpen && FMath::IsFinite(Value)) AddControllerPitchInput(Value); }
bool AMoonfallHeroPawn::TryCast(int32 School, int32 Hand, float Multiplier)
{
 static const float Costs[6] = {16.f,13.f,22.f,25.f,18.f,14.f};
 static const float Cooldowns[6] = {0.8f,0.65f,1.2f,2.f,1.f,0.9f};
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen || bResidentContextOpen || !GetWorld() || School < 0 || School >= 6 || Hand < 0 || Hand > 1 || !FMath::IsFinite(Multiplier) || Multiplier <= 0.f || Multiplier > 2.f) return false;
 const float Time = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth) || MaxHealth <= 0.f || Health > MaxHealth || !FMath::IsFinite(MaxMana) || MaxMana <= 0.f || !FMath::IsFinite(Mana) || !FMath::IsFinite(SpellPowerMultiplier) || SpellPowerMultiplier <= 0.f || Mana < Costs[School] * Multiplier || Time - LastCastTime[Hand] < Cooldowns[School] / (1.f + GetHasteBonus())) return false;
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
  const bool bOwnedWeapon = InventoryItemCodes.Contains(EquippedWeaponCode);
  const float WeaponDamageBonus = 1.f
      + (bOwnedWeapon ? FMath::Clamp(0.018f * ValidEquippedTier(EquippedWeaponCode, 0), 0.f, 0.35f) : 0.f)
      + (bOwnedWeapon && ValidEquippedTraitRarity(EquippedWeaponCode, 0, 1) > 0.f ? 0.06f : 0.f)
      + (bOwnedWeapon ? FMath::Clamp(ValidEquippedRarity(EquippedWeaponCode, 0) * 0.015f, 0.f, 0.075f) : 0.f);
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
  const float SpellDamage = (18.f + School * 5.f) * Multiplier * SpellPowerMultiplier
    * WeaponDamageBonus * CounterMultiplier * MomentumBonus * HitComboMultiplier
    * (1.f + GetMasteryBonus()) * (1.f + GetVersatilityBonus()) * ((GetWorld() && GetWorld()->GetTimeSeconds() < RacePowerActiveUntil && (RaceId == 1 || RaceId == 9 || RaceId == 10)) ? 1.15f : 1.f)
    * (bCriticalSpell ? (HasLegendaryPerk(0) ? 1.65f : 1.5f) : 1.f) * MightMultiplier;
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
void AMoonfallHeroPawn::CastLeft() { if (bOptionsOpen || bAtlasOpen) return; TryCast(LeftSpell, 0, 1.f); }
void AMoonfallHeroPawn::CastRight() { if (bOptionsOpen || bAtlasOpen) return; TryCast(RightSpell, 1, 1.f); }
void AMoonfallHeroPawn::CastBoth()
{
 if (bOptionsOpen || bAtlasOpen) return;
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
 if (bOptionsOpen || bInventoryOpen || bAtlasOpen || Health <= 0.f || !FMath::IsFinite(Health) || !GetWorld() || bGuarding
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
 if (bOptionsOpen || bInventoryOpen || bAtlasOpen || Health <= 0.f) return;
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
 if (bOptionsOpen || bInventoryOpen || bAtlasOpen || FocusedResident.IsValid() || Health <= 0.f || !GetWorld()) return;
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

bool AMoonfallHeroPawn::ApplyMoonfallDodgeOnlyDamageFrom(float Amount, const FVector& SourceLocation) {
 // Invulnerability from timed dodges still wins; only blocking and parrying
 // are bypassed. Restore the scoped state before returning to the caller.
 bIncomingDamageDodgeOnly = true;
 const bool bParried = ApplyMoonfallDamageFrom(Amount, SourceLocation);
 bIncomingDamageDodgeOnly = false;
 return bParried;
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
 const bool bOwnedArmor = InventoryItemCodes.Contains(EquippedArmorCode);
 const float ArmorMitigation = FMath::Min(0.34f,
   (bOwnedArmor ? FMath::Min(0.28f, ValidEquippedTier(EquippedArmorCode, 1) * 0.01f) : 0.f)
   + (bOwnedArmor && ValidEquippedTraitRarity(EquippedArmorCode, 1, 2) > 0.f ? 0.06f : 0.f)
   + (bOwnedArmor ? FMath::Clamp(ValidEquippedRarity(EquippedArmorCode, 1) * 0.005f, 0.f, 0.025f) : 0.f)
   + GetVersatilityBonus() * 0.5f
   + (GetSetPieceCount(2) >= 3 ? 0.025f : 0.f)
   + (HasLegendaryPerk(1) ? 0.015f : 0.f));
 const bool bRacialWard = bRaceChosen && (RaceId == 3 || RaceId == 7 || RaceId == 8)
   && Now < RacePowerActiveUntil;
 const float ArmoredDamage = Amount * (1.f - ArmorMitigation) * (bRacialWard ? 0.85f : 1.f);
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
 const bool bPerfectParry = !bIncomingDamageDodgeOnly && bGuarding && bHasIncomingDamageSource && bFrontAttack
     && FMath::IsFinite(ParryWindowEndsAt) && FMath::IsFinite(NextParryAt)
     && Now <= ParryWindowEndsAt && Now <= NextParryAt;
 const float RequiredGuardCost = bPerfectParry ? 0.f : HitGuardCost;
 if (!bIncomingDamageDodgeOnly && bGuarding && bFrontAttack && !bOptionsOpen
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
   DamageAfterGuard *= 0.4f;
  }
  OnGuardImpact(ArmoredDamage - DamageAfterGuard, bPerfectParry);
  if (Stamina < GuardStaminaCost) BreakGuard();
 } else if (!bIncomingDamageDodgeOnly && bGuarding && bFrontAttack) {
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
  CloseResidentContext();
  // Prevent lingering movement/jump input from carrying through defeat.
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  // Clear any held sprint so a later respawn cannot inherit it.
  EndSprint();
  EndGuard();
  // Defeat clears every gameplay menu so the death/respawn screen stays visible.
  bInventoryOpen = false;
  bCharacterStatsOpen = false;
  bOptionsOpen = false;
  bAtlasOpen = false;
  bInventoryScrollbarDragging = false;
  AtlasDiscoveredShrineLocations.Reset();
  AtlasDiscoveredShrineNames.Reset();
  bAtlasHasActiveRespawnShrineLocation = false;
  if (APlayerController* PC = Cast<APlayerController>(Controller))
   PC->bShowMouseCursor = false;
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

void AMoonfallHeroPawn::ToggleCharacterStats()
{
 if (bOptionsOpen || Health <= 0.f || !FMath::IsFinite(Health)) return;
 // The C character sheet must open directly, without first pressing B.
 // Reuse inventory opening to apply its movement, mouse and modal cleanup.
 if (!bInventoryOpen) {
  ToggleInventory();
  if (!bInventoryOpen) return;
  bCharacterStatsOpen = true;
  return;
 }
 bCharacterStatsOpen = !bCharacterStatsOpen;
 bInventoryScrollbarDragging = false;
}

void AMoonfallHeroPawn::ToggleAtlas() {
 if (Health <= 0.f || !FMath::IsFinite(Health) || bOptionsOpen) return;
 // Blueprint and corrupt transient values must never poison projected map pixels.
 AtlasZoom = FMath::Clamp(FMath::IsFinite(AtlasZoom) ? AtlasZoom : 1.f, 0.8f, 1.8f);
 AtlasPanY = FMath::Clamp(FMath::IsFinite(AtlasPanY) ? AtlasPanY : -2500.f, -12000.f, 12000.f);
 if (FocusedResident.IsValid()) CloseResidentContext();
 if (!bAtlasOpen) {
  if (bInventoryOpen) ToggleInventory();
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  EndSprint();
  EndGuard();
  // Discover active, already-visited shrines from their real actors once
  // per Atlas opening. No per-frame world actor scan or new save fields.
  AtlasDiscoveredShrineLocations.Reset();
  AtlasDiscoveredShrineNames.Reset();
  bAtlasHasActiveRespawnShrineLocation = false;
  AtlasActiveRespawnShrineLocation = FVector::ZeroVector;
  if (GetWorld()) {
   TSet<FName> SeenShrineIds;
   for (TActorIterator<AMoonfallCheckpointShrine> It(GetWorld()); It; ++It) {
    const AMoonfallCheckpointShrine* Shrine = *It;
    if (!IsValid(Shrine) || Shrine->ShrineId.IsNone()
        || !HasDiscoveredShrine(Shrine->ShrineId)
        || SeenShrineIds.Contains(Shrine->ShrineId)) continue;
    const FVector ShrinePos = Shrine->GetActorLocation();
    if (!FMath::IsFinite(ShrinePos.X) || !FMath::IsFinite(ShrinePos.Y)) continue;
    if (AtlasDiscoveredShrineLocations.Num() >= 64) break;
    SeenShrineIds.Add(Shrine->ShrineId);
    // Keep Atlas and world labels identical, while caching only discovered shrines.
    const FString ShrineTitle = Shrine->GetMapDisplayName();
    AtlasDiscoveredShrineLocations.Add(ShrinePos);
    AtlasDiscoveredShrineNames.Add(ShrineTitle.Left(36));
    if (ActiveShrineId == Shrine->ShrineId
        && IsCurrentRespawnLocation(Shrine->GetRespawnLocation())) {
     AtlasActiveRespawnShrineLocation = ShrinePos;
     bAtlasHasActiveRespawnShrineLocation = true;
    }
   }
  }
 }
 bAtlasOpen = !bAtlasOpen;
 if (APlayerController* PC = Cast<APlayerController>(Controller)) PC->bShowMouseCursor = bAtlasOpen;
}
void AMoonfallHeroPawn::ToggleInventory() {
 if (bAtlasOpen) { bAtlasOpen = false; if (APlayerController* PC = Cast<APlayerController>(Controller)) PC->bShowMouseCursor = false; }
 if (FocusedResident.IsValid()) CloseResidentContext();
 if (bOptionsOpen || Health <= 0.f) return;
 if (!bInventoryOpen) {
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  EndSprint();
  EndGuard();
  InventorySelection = FMath::Clamp(InventorySelection, 0, FMath::Max(0, InventoryItemCodes.Num() - 1));
  KeepInventorySelectionVisible();
 }
 bInventoryOpen = !bInventoryOpen;
 bInventoryScrollbarDragging = false;
 if (APlayerController* PC = Cast<APlayerController>(Controller))
  PC->bShowMouseCursor = bInventoryOpen;
 if (!bInventoryOpen) bCharacterStatsOpen = false;
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
 if (Health <= 0.f || !InventoryItemCodes.IsValidIndex(InventoryIndex)) return false;
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
 // Keep the same currency limit as SaveWorldProgress. Clamp before
 // adding to avoid overflow and never erase a legitimate saved balance.
 constexpr int32 MaxSalvage = 100000000;
 const int32 SalvageReward = 1 + Rarity * 2 + Tier / 5;
 const int32 SafeBalance = FMath::Clamp(Salvage, 0, MaxSalvage);
 Salvage = SafeBalance + FMath::Min(SalvageReward, MaxSalvage - SafeBalance);
 InventoryItemCodes.RemoveAt(InventoryIndex);
 InventorySelection = FMath::Clamp(InventorySelection, 0, FMath::Max(0, InventoryItemCodes.Num() - 1));
 return true;
}
bool AMoonfallHeroPawn::UpgradeInventoryItem(int32 InventoryIndex)
{
 // Craft the selected inventory item on explicit input. If it is worn,
 // upgrade that equipped slot too, without auto-equipping other bag items.
 if (!bInventoryOpen || bOptionsOpen || Health <= 0.f
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
 // Preserve the actual worn tier BEFORE equipping the crafted item.
 // Reading after EquipInventoryItem reports zero gain for worn upgrades.
 const int32 PreviousEquippedTier = GetValidatedEquippedTier(Slot);
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
 if (!bInventoryOpen || bOptionsOpen || Health <= 0.f
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
 if (!bInventoryOpen || bOptionsOpen || Health <= 0.f
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
 if (!bInventoryOpen || bOptionsOpen || Health <= 0.f
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
 if (bResidentContextOpen || FocusedResident.IsValid()) { CloseResidentContext(); return; }
 // The defeat screen owns input until respawn; do not reopen gameplay menus.
 if (Health <= 0.f || !FMath::IsFinite(Health)) return;
 if (bAtlasOpen) { ToggleAtlas(); return; }
 if (bInventoryOpen) {
  if (bCharacterStatsOpen) {
   // Esc backs out of character details to the bag without closing the inventory.
   bCharacterStatsOpen = false;
   bInventoryScrollbarDragging = false;
   return;
  }
  ToggleInventory(); // Same cleanup path as the mouse close button.
  return;
 }
 if (!bOptionsOpen) {
  StopJumping();
  if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
  EndSprint();
  EndGuard();
 }
 bOptionsOpen = !bOptionsOpen;
 // Match the inventory close workflow and allow the Options X to be clicked.
 if (APlayerController* PC = Cast<APlayerController>(Controller))
  PC->bShowMouseCursor = bOptionsOpen;
}
void AMoonfallHeroPawn::OptionsUp() {
 if (bAtlasOpen) { AtlasSelectedRegion = FMath::Clamp(AtlasSelectedRegion - 1, 0, 5); return; }
 if (FocusedResident.IsValid() && !bOptionsOpen && !bInventoryOpen) {
  ResidentContextSelection = (ResidentContextSelection + 2) % 3;
  bResidentTurnInPending = false;
  return;
 }
 if (bInventoryOpen) {
  const TArray<int32> DisplayOrder = GetInventoryDisplayOrder();
  const int32 CurrentRow = DisplayOrder.Find(InventorySelection);
  if (DisplayOrder.Num() > 0)
   InventorySelection = DisplayOrder[FMath::Clamp(CurrentRow - 1, 0, DisplayOrder.Num() - 1)];
  KeepInventorySelectionVisible();
  return;
 }
 if (bOptionsOpen) OptionsSelection = (OptionsSelection + 7) % 8;
}
void AMoonfallHeroPawn::OptionsDown() {
 if (bAtlasOpen) { AtlasSelectedRegion = FMath::Clamp(AtlasSelectedRegion + 1, 0, 5); return; }
 if (FocusedResident.IsValid() && !bOptionsOpen && !bInventoryOpen) {
  ResidentContextSelection = (ResidentContextSelection + 1) % 3;
  bResidentTurnInPending = false;
  return;
 }
 if (bInventoryOpen) {
  const TArray<int32> DisplayOrder = GetInventoryDisplayOrder();
  const int32 CurrentRow = DisplayOrder.Find(InventorySelection);
  if (DisplayOrder.Num() > 0)
   InventorySelection = DisplayOrder[FMath::Clamp(CurrentRow + 1, 0, DisplayOrder.Num() - 1)];
  KeepInventorySelectionVisible();
  return;
 }
 if (bOptionsOpen) OptionsSelection = (OptionsSelection + 1) % 8;
}
void AMoonfallHeroPawn::OptionsLeft() {
 if (bAtlasOpen) { AtlasPanY = FMath::Clamp((FMath::IsFinite(AtlasPanY) ? AtlasPanY : -2500.f) - 750.f, -12000.f, 12000.f); return; }
 if (bInventoryOpen) return;
 AdjustOption(-1);
}
void AMoonfallHeroPawn::OptionsRight() {
 if (bAtlasOpen) { AtlasPanY = FMath::Clamp((FMath::IsFinite(AtlasPanY) ? AtlasPanY : -2500.f) + 750.f, -12000.f, 12000.f); return; }
 if (bInventoryOpen) return;
 AdjustOption(1);
}
void AMoonfallHeroPawn::AdjustOption(int32 Direction) {
 if (!bOptionsOpen || !GEngine) return;
 UGameUserSettings* Settings = GEngine->GetGameUserSettings();
 if (!Settings) return;
 if (OptionsSelection == 0) {
  const FIntPoint Resolutions[] = {FIntPoint(1280,720), FIntPoint(1920,1080), FIntPoint(2560,1440), FIntPoint(3840,2160)};
  const FIntPoint CurrentResolution = Settings->GetScreenResolution();
  const int64 CurrentPixels = int64(CurrentResolution.X) * int64(CurrentResolution.Y);
  int32 Target = Direction < 0 ? 0 : 3;
  // Non-preset screen sizes must step relative to the current resolution,
  // rather than incorrectly assuming 1080p.
  if (Direction < 0) {
   for (int32 i = 3; i >= 0; --i) {
    if (int64(Resolutions[i].X) * int64(Resolutions[i].Y) < CurrentPixels) {
     Target = i;
     break;
    }
   }
  } else {
   for (int32 i = 0; i < 4; ++i) {
    if (int64(Resolutions[i].X) * int64(Resolutions[i].Y) > CurrentPixels) {
     Target = i;
     break;
    }
   }
  }
  if (CurrentResolution != Resolutions[Target])
   Settings->SetScreenResolution(Resolutions[Target]);
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
 } else if (OptionsSelection == 7) {
  // Minus and plus must navigate the three display modes in opposite directions.
  const EWindowMode::Type CurrentMode = Settings->GetFullscreenMode();
  const int32 CurrentIndex = CurrentMode == EWindowMode::Windowed ? 0
      : CurrentMode == EWindowMode::WindowedFullscreen ? 1 : 2;
  const int32 NextIndex = (CurrentIndex + (Direction < 0 ? 2 : 1)) % 3;
  const EWindowMode::Type NextMode = NextIndex == 0 ? EWindowMode::Windowed
      : NextIndex == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Fullscreen;
  Settings->SetFullscreenMode(NextMode);
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
 if (Health <= 0.f) return 0;
 // Roll randomized item stats once at death; pickup is handled separately.
 // Keep boss/elite rewards meaningful: guaranteed epic for bosses, at least rare
 // for elites. Regular enemies use weighted chances with a modest level bias.
 const int32 NewTier = FMath::Clamp(Level / 5 + (bBossEnemy ? 2 : (bEliteEnemy ? 1 : 0)), 1, 25);
 const int32 LootRoll = FMath::RandRange(0, 99);
 const int32 RarityBias = FMath::Clamp(Level / 5, 0, 12);
 // Existing rarity codes 0-3 are stable for compatibility.
 // 4 Emerald and 5 Legendary add special high-end reward targets.
 // Bosses always roll at least Epic; max-level bosses can yield Legendary.
 LastLootRarity = bBossEnemy
   ? (Level >= 60 && LootRoll < 12 ? 5 : (Level >= 30 && LootRoll < 40 ? 4 : 3))
   : (bEliteEnemy
     ? (Level >= 50 && LootRoll < 3 ? 5 : (Level >= 20 && LootRoll < 12 ? 4
       : (LootRoll < 27 ? 3 : 2)))
     : (Level >= 60 && LootRoll < 1 ? 5 : (Level >= 40 && LootRoll < 3 ? 4
       : (LootRoll < 3 + RarityBias / 3 ? 3
       : (LootRoll < 18 + RarityBias ? 2
       : (LootRoll < 53 + RarityBias ? 1 : 0)))))));
 LastLootTier = NewTier;
 // Enemy rank influences quality, not gear slot: any enemy can drop
 // weapons, armor, or accessories.
 LastLootSlot = FMath::RandRange(0, 2);
 // Rare and better drops carry one randomized trait. Lower rarities may
 // roll a trait, but common items never do. These are encoded in item IDs.
 LastLootAffix = LastLootRarity == 0 ? 0
   : (LastLootRarity == 1 && FMath::RandRange(0, 99) < 60
     ? 0 : FMath::RandRange(1, 3));
 const int32 PreviousTier = GetValidatedEquippedTier(LastLootSlot);
 LastLootUpgrade = NewTier - PreviousTier;
 // Compact item code: affix (100000s), slot (10000s), rarity (1000s),
 // tier (units). Distinct drops persist until the player salvages them.
 const int32 ItemCode = LastLootAffix * 100000 + LastLootSlot * 10000
   + LastLootRarity * 1000 + NewTier;
 return ItemCode;
}

// Legacy direct grant remains available for scripted rewards. Enemy deaths
// instead spawn a ground actor, so equipment is not auto-collected on kills.
void AMoonfallHeroPawn::AwardEquipmentDrop(bool bEliteEnemy, bool bBossEnemy)
{
 // Scripted rewards must honor the same persistent inventory capacity as
 // ground pickups. Do not roll or mutate last-loot UI when the bag is full.
 if (InventoryItemCodes.Num() >= 10000) return;
 const int32 ItemCode = RollEquipmentCode(bEliteEnemy, bBossEnemy);
 if (!ValidMoonfallItemCode(ItemCode)) return;
 // Keep every valid scripted reward in the bag without equipping it.
 // The paginated inventory supports growth beyond the former 30-slot limit.
 InventoryItemCodes.Add(ItemCode);
 InventorySelection = FMath::Clamp(InventorySelection, 0, InventoryItemCodes.Num() - 1);
 // Rewards are inventory choices only. Never auto-equip scripted drops;
 // the player chooses when to change weapon, armor, or accessory.
}
bool AMoonfallHeroPawn::CollectGroundEquipmentCode(int32 ItemCode)
{
 return CollectGroundEquipmentCodeImpl(ItemCode, INDEX_NONE);
}

bool AMoonfallHeroPawn::CollectDungeonTreasureCode(
 int32 ItemCode, int32 Zone, int32 Dungeon, int32 Chamber)
{
 if (Zone < 0 || Zone >= 6 || Dungeon < 0 || Dungeon >= 5
     || Chamber <= 0 || Chamber >= 13) return false;
 const int32 Key = (Zone * 5 + Dungeon) * 14 + Chamber;
 if (!DungeonTreasureStates.IsValidIndex(Key)
     || DungeonTreasureStates[Key] != ItemCode || ItemCode <= 0) return false;
 return CollectGroundEquipmentCodeImpl(ItemCode, Key);
}

bool AMoonfallHeroPawn::CollectPersistedGroundEquipmentCode(int32 ItemCode, const FGuid& DropId)
{
 if (!DropId.IsValid() || bPersistentSaveWritesBlocked) return false;
 const FName LevelId(*UGameplayStatics::GetCurrentLevelName(this, true));
 const int32 Index = PendingWorldLoot.IndexOfByPredicate(
  [&](const FMoonfallWorldLootRecord& Record) {
   return Record.DropId == DropId && Record.ItemCode == ItemCode && Record.LevelId == LevelId;
  });
 if (Index == INDEX_NONE) return false;
 const FMoonfallWorldLootRecord Previous = PendingWorldLoot[Index];
 PendingWorldLoot.RemoveAt(Index);
 if (CollectGroundEquipmentCodeImpl(ItemCode, INDEX_NONE)) return true;
 PendingWorldLoot.Insert(Previous, Index);
 return false;
}

bool AMoonfallHeroPawn::CollectGroundEquipmentCodeImpl(int32 ItemCode, int32 DungeonTreasureKey)
{
 if (Health <= 0.f || ItemCode <= 0) return false;
 const int32 Affix = ItemCode / 100000;
 const int32 SlotIndex = (ItemCode / 10000) % 10;
 const int32 Rarity = (ItemCode / 1000) % 10;
 const int32 Tier = ItemCode % 1000;
 if (Affix < 0 || Affix > 3 || SlotIndex < 0 || SlotIndex > 2
     || Rarity < 0 || Rarity > 5 || Tier < 1 || Tier > 25
     || (Rarity == 0 && Affix != 0)
     || ItemCode != Affix * 100000 + SlotIndex * 10000 + Rarity * 1000 + Tier)
  return false;
 // Ground loot remains in the world until it is safely recorded.
 // SaveGameToSlot is synchronous here; a failed save must leave pickup and
 // all last-loot UI values untouched, even when this code already exists.
 if (InventoryItemCodes.Num() >= 10000) return false;
 const int32 OldLootAffix = LastLootAffix;
 const int32 OldLootSlot = LastLootSlot;
 const int32 OldLootRarity = LastLootRarity;
 const int32 OldLootTier = LastLootTier;
 const int32 OldLootUpgrade = LastLootUpgrade;
 const int32 AddedIndex = InventoryItemCodes.Add(ItemCode);
 // Dungeon treasure is claimed in the SAME synchronous save as inventory.
 // If writing fails, restore both fields before another pickup attempt.
 const int32 PreviousTreasureState = DungeonTreasureKey != INDEX_NONE
   && DungeonTreasureStates.IsValidIndex(DungeonTreasureKey)
   ? DungeonTreasureStates[DungeonTreasureKey] : 0;
 if (DungeonTreasureKey != INDEX_NONE) {
  if (PreviousTreasureState != ItemCode) {
   InventoryItemCodes.RemoveAt(AddedIndex);
   return false;
  }
  DungeonTreasureStates[DungeonTreasureKey] = -1;
 }
 LastLootAffix = Affix;
 LastLootSlot = SlotIndex;
 LastLootRarity = Rarity;
 LastLootTier = Tier;
 const int32 EquippedTier = GetValidatedEquippedTier(SlotIndex);
 LastLootUpgrade = Tier - EquippedTier;
 if (!SaveWorldProgress()) {
  InventoryItemCodes.RemoveAt(AddedIndex);
  if (DungeonTreasureKey != INDEX_NONE)
   DungeonTreasureStates[DungeonTreasureKey] = PreviousTreasureState;
  LastLootAffix = OldLootAffix;
  LastLootSlot = OldLootSlot;
  LastLootRarity = OldLootRarity;
  LastLootTier = OldLootTier;
  LastLootUpgrade = OldLootUpgrade;
  return false;
 }
 // This pickup was saved synchronously; avoid a duplicate autosave next Tick.
 SecondsSinceLastWorldSave = 0.f;
 return true;
}

bool AMoonfallHeroPawn::RegisterPersistentEquipmentDrops(const TArray<FMoonfallWorldLootRecord>& Drops)
{
 if (!GetWorld() || !bWorldProgressInitialized || bPersistentSaveWritesBlocked
     || Drops.IsEmpty() || Drops.Num() > 3 || PendingWorldLoot.Num() + Drops.Num() > 512) return false;
 const FName LevelId(*UGameplayStatics::GetCurrentLevelName(this, true));
 if (LevelId.IsNone()) return false;
 TSet<FGuid> Batch;
 for (const FMoonfallWorldLootRecord& Record : Drops) {
  if (!Record.DropId.IsValid() || Batch.Contains(Record.DropId)
      || Record.LevelId != LevelId || !ValidMoonfallItemCode(Record.ItemCode)
      || Record.Location.ContainsNaN() || Record.Location.SizeSquared() >= 1.0e14f
      || PendingWorldLoot.ContainsByPredicate([&](const FMoonfallWorldLootRecord& Existing)
         { return Existing.DropId == Record.DropId; })) return false;
  Batch.Add(Record.DropId);
 }
 const int32 Before = PendingWorldLoot.Num();
 PendingWorldLoot.Append(Drops);
 if (!SaveWorldProgress()) { PendingWorldLoot.SetNum(Before); return false; }
 for (const FMoonfallWorldLootRecord& Record : Drops) {
  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  if (AMoonfallGroundLoot* Loot = GetWorld()->SpawnActor<AMoonfallGroundLoot>(
      AMoonfallGroundLoot::StaticClass(), Record.Location, FRotator::ZeroRotator, Params)) {
   Loot->SetPersistentDropId(Record.DropId);
   // Reject partially initialized pickups instead of allowing an untracked
   // collect or recording a missing restored drop as present.
   if (Loot->GetPersistentDropId() != Record.DropId) {
    Loot->Destroy();
    continue;
   }
   Loot->InitializeLoot(Record.ItemCode);
   if (!IsValid(Loot) || Loot->IsActorBeingDestroyed()
       || Loot->GetItemCode() != Record.ItemCode) {
    if (IsValid(Loot)) Loot->Destroy();
    continue;
   }
  }
 }
 return true;
}

void AMoonfallHeroPawn::RestorePendingWorldEquipmentDrops()
{
 if (!GetWorld() || bPersistentSaveWritesBlocked) return;
 const FName LevelId(*UGameplayStatics::GetCurrentLevelName(this, true));
 TSet<FGuid> Expected, Present;
 for (const FMoonfallWorldLootRecord& Record : PendingWorldLoot)
  if (Record.LevelId == LevelId) Expected.Add(Record.DropId);
 for (TActorIterator<AMoonfallGroundLoot> It(GetWorld()); It; ++It) {
  AMoonfallGroundLoot* Loot = *It;
  if (!IsValid(Loot) || !Loot->GetPersistentDropId().IsValid()) continue;
  const FGuid Id = Loot->GetPersistentDropId();
  const int32 MatchingRecord = PendingWorldLoot.IndexOfByPredicate(
   [&](const FMoonfallWorldLootRecord& Record) {
    return Record.LevelId == LevelId && Record.DropId == Id
      && Record.ItemCode == Loot->GetItemCode();
   });
  if (!Expected.Contains(Id) || Present.Contains(Id) || MatchingRecord == INDEX_NONE
      || Loot->IsActorBeingDestroyed()) { Loot->Destroy(); continue; }
  Present.Add(Id);
 }
 for (const FMoonfallWorldLootRecord& Record : PendingWorldLoot) {
  if (Record.LevelId != LevelId || Present.Contains(Record.DropId)) continue;
  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  if (AMoonfallGroundLoot* Loot = GetWorld()->SpawnActor<AMoonfallGroundLoot>(
      AMoonfallGroundLoot::StaticClass(), Record.Location, FRotator::ZeroRotator, Params)) {
   Loot->SetPersistentDropId(Record.DropId);
   // Reject partially initialized pickups instead of allowing an untracked
   // collect or recording a missing restored drop as present.
   if (Loot->GetPersistentDropId() != Record.DropId) {
    Loot->Destroy();
    continue;
   }
   Loot->InitializeLoot(Record.ItemCode);
   if (!IsValid(Loot) || Loot->IsActorBeingDestroyed()
       || Loot->GetItemCode() != Record.ItemCode) {
    if (IsValid(Loot)) Loot->Destroy();
    continue;
   }
   Present.Add(Record.DropId);
  }
 }
}

void AMoonfallHeroPawn::SpawnGroundEquipmentDrop(bool bEliteEnemy, bool bBossEnemy, FVector DefeatedLocation)
{
 if (!GetWorld() || Health <= 0.f) return;
 const int32 DropCount = bBossEnemy ? 3 : (bEliteEnemy ? 2 : 1);
 const FVector Origin = DefeatedLocation.ContainsNaN() ? GetActorLocation() : DefeatedLocation;
 const FName LevelId(*UGameplayStatics::GetCurrentLevelName(this, true));
 TArray<FMoonfallWorldLootRecord> Drops;
 for (int32 Drop = 0; Drop < DropCount; ++Drop) {
  const int32 ItemCode = RollEquipmentCode(bEliteEnemy, bBossEnemy);
  if (ItemCode <= 0) continue;
  const float Angle = 2.f * PI * static_cast<float>(Drop) / static_cast<float>(DropCount);
  const float Spread = DropCount == 1 ? 0.f : 105.f;
  FMoonfallWorldLootRecord Record;
  Record.DropId = FGuid::NewGuid();
  Record.ItemCode = ItemCode;
  Record.LevelId = LevelId;
  Record.Location = Origin + FVector(Spread * FMath::Cos(Angle),
   Spread * FMath::Sin(Angle), 45.f);
  Drops.Add(Record);
 }
 if (Drops.IsEmpty() || RegisterPersistentEquipmentDrops(Drops)) return;
 // Failed disk writes retain temporary drops in the current session.
 UE_LOG(LogTemp, Warning, TEXT("Moonfall durable loot save failed; spawning temporary loot."));
 for (const FMoonfallWorldLootRecord& Record : Drops) {
  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  if (AMoonfallGroundLoot* Loot = GetWorld()->SpawnActor<AMoonfallGroundLoot>(
      AMoonfallGroundLoot::StaticClass(), Record.Location, FRotator::ZeroRotator, Params))
   Loot->InitializeLoot(Record.ItemCode);
 }
}

void AMoonfallHeroPawn::PickupNearestGroundLoot()
{
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen || !GetWorld()) return;
 // No arbitrary six-drop or candidate cap: nearby loot remains collectible.
 // World scans happen at a throttled interval for automatic collection.
 const FVector HeroPosition = GetActorLocation();
 if (HeroPosition.ContainsNaN()) return;
 const float PickupRangeSq = FMath::Square(250.f);
 TArray<AMoonfallGroundLoot*> NearbyLoot;
 for (TActorIterator<AMoonfallGroundLoot> It(GetWorld()); It; ++It) {
  AMoonfallGroundLoot* Candidate = *It;
  if (!IsValid(Candidate) || Candidate->GetItemCode() <= 0) continue;
  const FVector LootPosition = Candidate->GetActorLocation();
  if (LootPosition.ContainsNaN() || FMath::Abs(LootPosition.Z - HeroPosition.Z) > 150.f) continue;
  const float DistanceSq = FVector::DistSquared2D(LootPosition, HeroPosition);
  if (!FMath::IsFinite(DistanceSq) || DistanceSq > PickupRangeSq) continue;
  NearbyLoot.Add(Candidate);
 }
 // The loot actor performs the authoritative distance, cover and inventory checks.
 // Rejected or blocked loot stays in the world; auto-equip is never triggered.
 for (AMoonfallGroundLoot* Loot : NearbyLoot) {
  if (IsValid(Loot)) Loot->TryCollect();
 }
}

void AMoonfallHeroPawn::SetAutoLootEnabled(bool bEnabled)
{
 bAutoLootEnabled = bEnabled;
 AutoLootElapsed = 0.f;
 if (GConfig) {
  GConfig->SetBool(TEXT("Moonfall.Gameplay"), TEXT("AutoLootEnabled"), bAutoLootEnabled, GGameUserSettingsIni);
  GConfig->Flush(false, GGameUserSettingsIni);
 }
}

void AMoonfallHeroPawn::EquipBestInventoryAction() { if (!bCharacterStatsOpen) EquipBestInventoryUpgrades(); }

int32 AMoonfallHeroPawn::GetEquipmentUpgradeScore(int32 InventoryIndex) const
{
 // A candidate is only suggested while the player is safely browsing the bag.
 if (!bInventoryOpen || bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f
     || !InventoryItemCodes.IsValidIndex(InventoryIndex)) return 0;
 const int32 EquippedCodes[] = {EquippedWeaponCode, EquippedArmorCode, EquippedAccessoryCode};
 const int32 Candidate = InventoryItemCodes[InventoryIndex];
 const int32 Slot = (Candidate / 10000) % 10;
 if (Slot < 0 || Slot > 2 || Candidate <= 0 || Candidate == EquippedCodes[Slot]) return 0;
 const auto ValidCode = [this](int32 Code, int32 ExpectedSlot) {
  if (Code <= 0 || !InventoryItemCodes.Contains(Code)) return false;
  const int32 Affix = Code / 100000;
  const int32 EncodedSlot = (Code / 10000) % 10;
  const int32 Rarity = (Code / 1000) % 10;
  const int32 Tier = Code % 1000;
  return EncodedSlot == ExpectedSlot && Affix >= 0 && Affix <= 3
    && Rarity >= 0 && Rarity <= 5 && Tier >= 1 && Tier <= 25
    && (Rarity != 0 || Affix == 0)
    && Code == Affix * 100000 + ExpectedSlot * 10000 + Rarity * 1000 + Tier;
 };
 if (!ValidCode(Candidate, Slot)) return 0;
 const auto GearScore = [&](int32 Code) {
  if (!ValidCode(Code, Slot)) return 0;
  const int32 Affix = Code / 100000;
  const int32 Rarity = (Code / 1000) % 10;
  const int32 Tier = Code % 1000;
  int32 MatchingPieces = 0;
  if (Affix > 0) {
   for (int32 OtherSlot = 0; OtherSlot < 3; ++OtherSlot) {
    if (OtherSlot == Slot) continue;
    const int32 OtherCode = EquippedCodes[OtherSlot];
    if (ValidCode(OtherCode, OtherSlot) && OtherCode / 100000 == Affix)
     ++MatchingPieces;
   }
  }
  const int32 Synergy = MatchingPieces == 2 ? 80 : MatchingPieces == 1 ? 40 : 0;
  return Tier * 1000 + Rarity * 100 + (Affix > 0 ? 10 : 0) + Synergy;
 };
 return GearScore(Candidate) - GearScore(EquippedCodes[Slot]);
}

int32 AMoonfallHeroPawn::GetRecommendedEquipmentIndex() const
{
 // An explicit on-demand query, not a per-frame scan of large bags.
 if (!bInventoryOpen || bOptionsOpen || !FMath::IsFinite(Health) || Health <= 0.f)
  return INDEX_NONE;
 int32 BestIndex = INDEX_NONE;
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

int32 AMoonfallHeroPawn::EquipBestInventoryUpgrades()
{
 // Only a deliberate input while viewing inventory can change worn equipment.
 if (!bInventoryOpen || bOptionsOpen || Health <= 0.f) return 0;
 const int32 EquippedCodes[3] = {EquippedWeaponCode, EquippedArmorCode, EquippedAccessoryCode};
 int32 BestScores[3] = {-1, -1, -1};
 int32 BestIndices[3] = {-1, -1, -1};
 // Keep existing equipment unless the candidate is strictly better.
 // Tier is primary, rarity secondary, an affix breaks ties.
 for (int32 Slot = 0; Slot < 3; ++Slot) {
  const int32 Code = EquippedCodes[Slot];
  // A stale or externally edited equipped code cannot outrank owned loot.
  if (Code <= 0 || !InventoryItemCodes.Contains(Code)) continue;
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
 if (Health <= 0.f || !InventoryItemCodes.IsValidIndex(InventoryIndex)) return false;
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
 // The cached Blueprint tier may be stale; only owned, correctly decoded
 // equipment may influence the max-health delta when changing armor.
 const int32 PreviousTier = GetValidatedEquippedTier(SlotIndex);
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


namespace {
const FString MoonfallWorldSlot(TEXT("MoonfallPersistentWorldV1"));
// Defined with the faction/relationship helpers below; shared by save migration.
bool IsNamedMoonfallResident(FName Id);
bool ValidMoonfallItemCode(int32 Code) {
 if (Code <= 0) return false;
 const int32 Trait = Code / 100000;
 const int32 Slot = (Code / 10000) % 10;
 const int32 Rarity = (Code / 1000) % 10;
 const int32 Tier = Code % 1000;
 // Match RollEquipmentCode, ground pickup and manual equip. Common gear
 // has no trait; uncommon gear may be traitless. Reject only malformed codes,
 // not legitimately collected rewards that must persist through respawning.
 return Trait >= 0 && Trait <= 3 && Slot >= 0 && Slot <= 2
     && Rarity >= 0 && Rarity <= 5 && Tier >= 1 && Tier <= 25
     && (Rarity != 0 || Trait == 0)
     && Code == Trait * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
}
// A collision-aware recovery location is safer than reviving at the death site.
// The level generator supplies a PlayerStart; authored worlds can supply more.
bool RestoreMoonfallPlayerStart(AMoonfallHeroPawn* Hero) {
 if (!IsValid(Hero) || !Hero->GetWorld()) return false;
 for (TActorIterator<APlayerStart> It(Hero->GetWorld()); It; ++It) {
  const APlayerStart* Start = *It;
  if (IsValid(Start) && Hero->TeleportTo(Start->GetActorLocation(),
      Start->GetActorRotation(), false, false)) return true;
 }
 return false;
}
}
bool AMoonfallHeroPawn::RestoreSafeSpawnLocation() {
 // Favor authored PlayerStart actors; use the original placed pawn transform
 // only when a map has no usable PlayerStart. Collision checks stay enabled.
 if (RestoreMoonfallPlayerStart(this)) return true;
 if (!bHasInitialSpawnTransform) return false;
 const FVector Location = InitialSpawnTransform.GetLocation();
 if (Location.ContainsNaN() || Location.SizeSquared() >= 1.0e14f) return false;
 return TeleportTo(Location, InitialSpawnTransform.GetRotation().Rotator(), false, false);
}
// Touching a new shrine sets the next death checkpoint, not every autosave.
bool AMoonfallHeroPawn::HasDiscoveredShrine(FName ShrineId) const {
 return !ShrineId.IsNone() && DiscoveredShrineIds.Contains(ShrineId);
}
bool AMoonfallHeroPawn::IsCurrentRespawnLocation(const FVector& Location) const {
 // When a blocked saved shrine recovers at PlayerStart, its ID remains
 // discovered but must not be labeled as the current respawn destination.
 return bHasRespawnCheckpointLocation && !Location.ContainsNaN()
     && !RespawnCheckpointLocation.ContainsNaN()
     && FVector::DistSquared(RespawnCheckpointLocation, Location) <= FMath::Square(10.f);
}
bool AMoonfallHeroPawn::DiscoverShrine(AMoonfallCheckpointShrine* Shrine) {
 if (!IsValid(Shrine) || Shrine->GetWorld() != GetWorld()
     || Shrine->ShrineId.IsNone() || Health <= 0.f
     || !bWorldProgressInitialized || bWorldProgressLoadInProgress
     || bPersistentSaveWritesBlocked || HasDiscoveredShrine(Shrine->ShrineId)
     || DiscoveredShrineIds.Num() >= 64) return false;
 const FVector ShrineLocation = Shrine->GetActorLocation();
 const FVector HeroLocation = GetActorLocation();
 const FVector NewRespawn = Shrine->GetRespawnLocation();
 if (ShrineLocation.ContainsNaN() || HeroLocation.ContainsNaN()
     || NewRespawn.ContainsNaN() || NewRespawn.SizeSquared() >= 1.0e14f
     || FVector::DistSquared2D(ShrineLocation, HeroLocation) > FMath::Square(220.f)
     || FMath::Abs(ShrineLocation.Z - HeroLocation.Z) > 200.f) return false;
 // A proximity trigger can overlap through walls. Only an unobstructed
 // discovered shrine may become the persisted respawn checkpoint.
 FCollisionQueryParams ShrineSight(SCENE_QUERY_STAT(MoonfallShrineDiscovery), false);
 ShrineSight.AddIgnoredActor(this);
 ShrineSight.AddIgnoredActor(Shrine);
 if (GetWorld()->LineTraceTestByChannel(HeroLocation,
     ShrineLocation + FVector(0.f, 0.f, 80.f), ECC_Visibility, ShrineSight)) return false;
 // Line of sight alone does not prove the saved spawn position is safe.
 // Validate against the *actual scaled character capsule* before replacing
 // the previous working checkpoint. Ignore the hero and shrine geometry.
 const UCapsuleComponent* Capsule = GetCapsuleComponent();
 if (!Capsule) return false;
 float CapsuleRadius = 0.f;
 float CapsuleHalfHeight = 0.f;
 Capsule->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
 if (!FMath::IsFinite(CapsuleRadius) || !FMath::IsFinite(CapsuleHalfHeight)
     || CapsuleRadius <= 0.f || CapsuleHalfHeight < CapsuleRadius) return false;
 FCollisionQueryParams SpawnClearance(SCENE_QUERY_STAT(MoonfallShrineSpawnClearance), false);
 SpawnClearance.AddIgnoredActor(this);
 SpawnClearance.AddIgnoredActor(Shrine);
 if (GetWorld()->OverlapBlockingTestByChannel(NewRespawn, GetActorQuat(),
     ECC_Pawn, FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
     SpawnClearance)) return false;
 const FName PreviousShrineId = ActiveShrineId;
 const FVector PreviousCheckpoint = RespawnCheckpointLocation;
 const bool bPreviousCheckpointValid = bHasRespawnCheckpointLocation;
 DiscoveredShrineIds.Add(Shrine->ShrineId);
 ActiveShrineId = Shrine->ShrineId;
 RespawnCheckpointLocation = NewRespawn;
 bHasRespawnCheckpointLocation = true;
 // Unlock + checkpoint persist together. Failed saves cannot activate a
 // shrine in memory and silently lose it after quitting.
 if (!SaveWorldProgress()) {
  DiscoveredShrineIds.Remove(Shrine->ShrineId);
  ActiveShrineId = PreviousShrineId;
  RespawnCheckpointLocation = PreviousCheckpoint;
  bHasRespawnCheckpointLocation = bPreviousCheckpointValid;
  return false;
 }
 // Shrine discovery saved the world already; restart the autosave interval.
 SecondsSinceLastWorldSave = 0.f;
 // Discovery is rare, so update visual statuses only after a saved change.
 // Old shrines stop showing ACTIVE without a per-frame world scan.
 for (TActorIterator<AMoonfallCheckpointShrine> It(GetWorld()); It; ++It)
  if (IsValid(*It)) It->RefreshShrineDisplay(this);
 OnShrineDiscovered(ActiveShrineId);
 return true;
}
int32 AMoonfallHeroPawn::GetDungeonTreasureState(int32 Zone, int32 Dungeon, int32 Chamber) const
{
 if (Zone < 0 || Zone >= 6 || Dungeon < 0 || Dungeon >= 5
     || Chamber <= 0 || Chamber >= 13) return 0;
 const int32 Key = (Zone * 5 + Dungeon) * 14 + Chamber;
 return DungeonTreasureStates.IsValidIndex(Key) ? DungeonTreasureStates[Key] : 0;
}

int32 AMoonfallHeroPawn::GetOrCreateDungeonTreasureCode(
 int32 Zone, int32 Dungeon, int32 Chamber, int32 LayoutSeed)
{
 if (Zone < 0 || Zone >= 6 || Dungeon < 0 || Dungeon >= 5
     || Chamber <= 0 || Chamber >= 13 || Health <= 0.f
     || !FMath::IsFinite(Health) || !bWorldProgressInitialized
     || bWorldProgressLoadInProgress || bPersistentSaveWritesBlocked
     || !HasDiscoveredDungeon(Zone, Dungeon)) return 0;
 const int32 Key = (Zone * 5 + Dungeon) * 14 + Chamber;
 if (DungeonTreasureStates.IsValidIndex(Key)) {
  if (DungeonTreasureStates[Key] != 0)
   return FMath::Max(0, DungeonTreasureStates[Key]);
 }
 // Use a deterministic seed, independent of framerate and visit order.
 FRandomStream Stream(LayoutSeed ^ (Key * 15427 + 0x6C31));
 const int32 Slot = Stream.RandRange(0, 2);
 const int32 Rarity = Stream.RandRange(2, 5);
 const int32 Affix = Stream.RandRange(0, 3);
 const int32 Tier = FMath::Clamp(Level, 1, 25);
 const int32 Code = Affix * 100000 + Slot * 10000 + Rarity * 1000 + Tier;
 const TArray<int32> Previous = DungeonTreasureStates;
 DungeonTreasureStates.SetNumZeroed(420);
 DungeonTreasureStates[Key] = Code;
 if (!SaveWorldProgress()) {
  DungeonTreasureStates = Previous;
  return 0;
 }
 return Code;
}

bool AMoonfallHeroPawn::HasDefeatedDungeonBoss(int32 Zone, int32 Dungeon) const
{
 if (Zone < 0 || Zone >= 6 || Dungeon < 0 || Dungeon >= 5) return false;
 return (static_cast<uint32>(DefeatedDungeonBossMask)
     & (1u << (Zone * 5 + Dungeon))) != 0u;
}

int32 AMoonfallHeroPawn::GetDefeatedDungeonBossCount(int32 Zone) const
{
 if (Zone < 0 || Zone >= 6) return 0;
 int32 Count = 0;
 for (int32 Dungeon = 0; Dungeon < 5; ++Dungeon)
  if (HasDefeatedDungeonBoss(Zone, Dungeon)) ++Count;
 return Count;
}

int32 AMoonfallHeroPawn::GetTotalDefeatedDungeonBosses() const
{
 int32 Total = 0;
 for (int32 Zone = 0; Zone < 6; ++Zone)
  Total += GetDefeatedDungeonBossCount(Zone);
 return Total;
}

int32 AMoonfallHeroPawn::GetClearedDungeonZoneCount() const
{
 int32 Cleared = 0;
 for (int32 Zone = 0; Zone < 6; ++Zone)
  if (GetDefeatedDungeonBossCount(Zone) == 5) ++Cleared;
 return Cleared;
}

bool AMoonfallHeroPawn::MarkDungeonBossDefeated(AMoonfallEnemy* DefeatedBoss)
{
 // A raw dungeon ID is not proof of victory. The actual dead, tagged boss
 // must belong to a live dungeon layout in this world.
 if (!IsValid(DefeatedBoss) || DefeatedBoss->GetWorld() != GetWorld()
     || !DefeatedBoss->bBoss || !FMath::IsFinite(DefeatedBoss->Health)
     || DefeatedBoss->Health > 0.f
     || !DefeatedBoss->Tags.Contains(FName(TEXT("MoonfallDungeonBoss"))))
  return false;
 const AMoonfallDungeonLayout* DungeonLayout =
   Cast<AMoonfallDungeonLayout>(DefeatedBoss->GetOwner());
 if (!IsValid(DungeonLayout) || DungeonLayout->GetWorld() != GetWorld())
  return false;
 const int32 Zone = DungeonLayout->ZoneIndex;
 const int32 Dungeon = DungeonLayout->DungeonIndex;
 if (Zone < 0 || Zone >= 6 || Dungeon < 0 || Dungeon >= 5
     || !bWorldProgressInitialized || bWorldProgressLoadInProgress
     || bPersistentSaveWritesBlocked || !GetWorld()
     || !HasDiscoveredDungeon(Zone, Dungeon)
     || HasDefeatedDungeonBoss(Zone, Dungeon)) return false;
 // The 30-bit permanent victory ledger doubles as the idempotency key for
 // completion salvage. No extra save schema or one-shot reward flags needed.
 const int32 PreviousMask = DefeatedDungeonBossMask;
 const int32 PreviousSalvage = Salvage;
 const bool bCompletesZone = GetDefeatedDungeonBossCount(Zone) == 4;
 const bool bCompletesWorld = GetTotalDefeatedDungeonBosses() == 29;
 // Base reward grows by region, not framerate or RNG; one final-boss
 // completion pays 20-57 salvage, fifth boss +100 and last world boss +500.
 const int32 BaseReward = 20 + Zone * 5 + Dungeon * 2;
 const int32 CompletionReward = BaseReward + (bCompletesZone ? 100 : 0)
   + (bCompletesWorld ? 500 : 0);
 constexpr int32 MaxSalvage = 100000000;
 const int32 SafeSalvage = FMath::Clamp(Salvage, 0, MaxSalvage);
 const int32 SalvageEarned = FMath::Min(CompletionReward, MaxSalvage - SafeSalvage);
 DefeatedDungeonBossMask |= static_cast<int32>(1u << (Zone * 5 + Dungeon));
 Salvage = SafeSalvage + SalvageEarned;
 // Save BOTH new ledger and salvage in a single world-save write. A failing
 // save rolls both fields back. Do not notify Blueprint before persistence.
 if (!SaveWorldProgress()) {
  DefeatedDungeonBossMask = PreviousMask;
  Salvage = PreviousSalvage;
  return false;
 }
 OnDungeonBossVictory(Zone, Dungeon, SalvageEarned);
 if (bCompletesZone) OnDungeonZoneCleared(Zone);
 if (bCompletesWorld) OnAllDungeonsCleared();
 return true;
}

bool AMoonfallHeroPawn::HasDiscoveredDungeon(int32 Zone, int32 Dungeon) const
{
 if (Zone < 0 || Zone >= 6 || Dungeon < 0 || Dungeon >= 5) return false;
 return (static_cast<uint32>(DiscoveredDungeonMask) & (1u << (Zone * 5 + Dungeon))) != 0u;
}

int32 AMoonfallHeroPawn::GetDiscoveredDungeonCount(int32 Zone) const
{
 if (Zone < 0 || Zone >= 6) return 0;
 int32 Count = 0;
 for (int32 Dungeon = 0; Dungeon < 5; ++Dungeon)
  if (HasDiscoveredDungeon(Zone, Dungeon)) ++Count;
 return Count;
}

int32 AMoonfallHeroPawn::GetNextUnchartedDungeonZone() const
{
 for (int32 Zone = 0; Zone < 6; ++Zone)
  if (GetDiscoveredDungeonCount(Zone) < 5) return Zone;
 return INDEX_NONE;
}

int32 AMoonfallHeroPawn::GetTotalDiscoveredDungeons() const
{
 int32 Count = 0;
 for (int32 Zone = 0; Zone < 6; ++Zone)
  Count += GetDiscoveredDungeonCount(Zone);
 return Count;
}

bool AMoonfallHeroPawn::DiscoverDungeonEntrance(int32 Zone, int32 Dungeon)
{
 if (Zone < 0 || Zone >= 6 || Dungeon < 0 || Dungeon >= 5
     || Health <= 0.f || !FMath::IsFinite(Health)
     || !bWorldProgressInitialized || bWorldProgressLoadInProgress
     || bPersistentSaveWritesBlocked || !GetWorld()
     || HasDiscoveredDungeon(Zone, Dungeon)) return false;
 const int32 PreviousMask = DiscoveredDungeonMask;
 const bool bCompletesZone = GetDiscoveredDungeonCount(Zone) == 4;
 DiscoveredDungeonMask |= static_cast<int32>(1u << (Zone * 5 + Dungeon));
 // Persist atomically before notifying UI. Failed writes roll back the bit,
 // while the original slot remains intact.
 if (!SaveWorldProgress()) {
  DiscoveredDungeonMask = PreviousMask;
  return false;
 }
 OnDungeonDiscovered(Zone, Dungeon);
 // The fifth distinct entrance charts the zone once, after durable saving.
 if (bCompletesZone) OnDungeonZoneCharted(Zone);
 return true;
}

bool AMoonfallHeroPawn::HasExploredAtlasCell(int32 Zone, int32 Column, int32 Row) const
{
 if (Zone < 0 || Zone >= 6 || Column < 0 || Column >= 8 || Row < 0 || Row >= 7
     || ExploredZoneFogMasks.Num() != 6) return false;
 const int32 CellBit = Row * 8 + Column;
 return (static_cast<uint64>(ExploredZoneFogMasks[Zone]) & (1ULL << CellBit)) != 0ULL;
}

void AMoonfallHeroPawn::RevealAtlasAtWorldLocation()
{
 // A fixed six-zone XY grid with at most 336 explored cells (48 saved bytes).
 // Mark once per occupied cell; persistence uses the existing two-second autosave.
 if (!bWorldProgressInitialized || bWorldProgressLoadInProgress || bPersistentSaveWritesBlocked
     || Health <= 0.f || !FMath::IsFinite(Health)) return;
 const FVector Position = GetActorLocation();
 if (!FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y)
     || Position.X < -20250.f || Position.X > 20250.f
     || Position.Y < -12000.f || Position.Y > 12000.f) return;
 if (ExploredZoneFogMasks.IsEmpty()) ExploredZoneFogMasks.Init(0, 6);
 if (ExploredZoneFogMasks.Num() != 6) return;
 const int32 Zone = FMath::Clamp(FMath::FloorToInt((Position.X + 20250.f) / 6750.f), 0, 5);
 const float ZoneMinX = -20250.f + Zone * 6750.f;
 const int32 Column = FMath::Clamp(FMath::FloorToInt((Position.X - ZoneMinX) / (6750.f / 8.f)), 0, 7);
 const int32 Row = FMath::Clamp(FMath::FloorToInt((Position.Y + 12000.f) / (24000.f / 7.f)), 0, 6);
 ExploredZoneFogMasks[Zone] |= (1LL << (Row * 8 + Column));
}

bool AMoonfallHeroPawn::SaveWorldProgress() {
 // Wait until initial save restoration finishes. During respawn TeleportTo and
 // OnHealthChanged can synchronously notify Blueprint listeners, which must
 // not save half-restored inventory, quests, equipment or shrine progress.
 // Also protect existing incompatible/unreadable save slots.
 if (!bWorldProgressInitialized || bWorldProgressLoadInProgress
     || bPersistentSaveWritesBlocked || !bHasRespawnCheckpointLocation
     || RespawnCheckpointLocation.ContainsNaN()
     || RespawnCheckpointLocation.SizeSquared() >= 1.0e14f) return false;
 if (Health <= 0.f || !GetWorld() || !FMath::IsFinite(MaxHealth)) return false;
 // Existing saves have no V12 fog grid; initialize it without erasing other data.
 if (ExploredZoneFogMasks.IsEmpty()) ExploredZoneFogMasks.Init(0, 6);
 if (ExploredZoneFogMasks.Num() != 6) return false;
 const uint64 AllowedFogBits = (1ULL << 56) - 1ULL;
 for (const int64 Mask : ExploredZoneFogMasks)
  if ((static_cast<uint64>(Mask) & ~AllowedFogBits) != 0ULL) return false;
 RevealAtlasAtWorldLocation();
 // Never create a save that this version's loader will reject on reload.
 // The source collection remains intact if its supported serialized size
 // would be exceeded; ground drops are not consumed in that case.
 if (InventoryItemCodes.Num() > 10000 || ResidentQuestStates.Num() > 64
     || DiscoveredShrineIds.Num() > 64
     || (static_cast<uint32>(DiscoveredDungeonMask) & ~((1u << 30) - 1u)) != 0u
     || PendingWorldLoot.Num() > 512
     || DungeonTreasureStates.Num() > 420
     || (static_cast<uint32>(DefeatedDungeonBossMask) & ~((1u << 30) - 1u)) != 0u
     || (static_cast<uint32>(DefeatedDungeonBossMask)
         & ~static_cast<uint32>(DiscoveredDungeonMask)) != 0u) return false;
 for (int32 Index = 0; Index < DungeonTreasureStates.Num(); ++Index) {
  const int32 Code = DungeonTreasureStates[Index];
  if (Code != 0 && Code != -1 && !ValidMoonfallItemCode(Code)) return false;
  if (Code != 0 && (Index % 14 == 0 || Index % 14 == 13)) return false;
 }
 // The loader only restores known residents with accepted or completed
 // contracts. Never save quest entries it would silently discard.
 for (const auto& Entry : ResidentQuestStates)
  if (!IsNamedMoonfallResident(Entry.Key) || (Entry.Value != 1 && Entry.Value != 2))
   return false;
 // Reputation and bond maps must round-trip without silently dropping keys
 // or clamping values on the next load.
 static const FName SaveFactionIds[] = {FName(TEXT("Wanderers")),
  FName(TEXT("VerdantCircle")), FName(TEXT("StoneHearth")),
  FName(TEXT("MoonCoven")), FName(TEXT("AshenClans"))};
 static const FName SaveResidentIds[] = {
  FName(TEXT("CrossroadsGuide")), FName(TEXT("Alden")),
  FName(TEXT("Starweaver")), FName(TEXT("Leafwhisper")),
  FName(TEXT("Anvilkeeper")), FName(TEXT("PipHearthfoot")),
  FName(TEXT("EmberScout")), FName(TEXT("Geargrin")),
  FName(TEXT("Mossfang")), FName(TEXT("Birchroot")),
  FName(TEXT("Duskveil")), FName(TEXT("Silverwatch")),
  FName(TEXT("Ashscale"))
 };
 for (const auto& Entry : FactionReputation)
  if (!SaveFactionIds[0].IsNone() &&
      !((Entry.Key == SaveFactionIds[0]) || (Entry.Key == SaveFactionIds[1])
       || (Entry.Key == SaveFactionIds[2]) || (Entry.Key == SaveFactionIds[3])
       || (Entry.Key == SaveFactionIds[4])) || Entry.Value < -100 || Entry.Value > 100)
   return false;
 for (const auto& Entry : ResidentBonds) {
  bool bKnownResident = false;
  for (const FName Id : SaveResidentIds) if (Entry.Key == Id) { bKnownResident = true; break; }
  if (!bKnownResident || Entry.Value < -100 || Entry.Value > 100) return false;
 }
 TSet<FGuid> UniqueWorldDrops;
 for (const FMoonfallWorldLootRecord& Record : PendingWorldLoot) {
  if (!Record.DropId.IsValid() || UniqueWorldDrops.Contains(Record.DropId)
      || !ValidMoonfallItemCode(Record.ItemCode) || Record.LevelId.IsNone()
      || Record.Location.ContainsNaN() || Record.Location.SizeSquared() >= 1.0e14f) return false;
  UniqueWorldDrops.Add(Record.DropId);
 }
 for (const int32 Code : InventoryItemCodes)
  if (!ValidMoonfallItemCode(Code)) return false;
 // Keep the equipped references consistent with the bag; otherwise the loader
 // would silently clear an equipped slot on the next launch.
 const int32 WornCodes[3] = {EquippedWeaponCode, EquippedArmorCode, EquippedAccessoryCode};
 for (int32 Slot = 0; Slot < 3; ++Slot) {
  const int32 Code = WornCodes[Slot];
  if (Code != 0 && (!ValidMoonfallItemCode(Code)
      || ((Code / 10000) % 10) != Slot || !InventoryItemCodes.Contains(Code)))
   return false;
 }
 // A V7 save must be readable by our own loader. Reject duplicate, empty,
 // or inconsistent shrine IDs rather than overwriting the last valid slot
 // with a ledger that LoadWorldProgress() would reject on the next launch.
 TSet<FName> UniqueShrines;
 for (const FName Id : DiscoveredShrineIds) {
  if (Id.IsNone() || UniqueShrines.Contains(Id)) return false;
  UniqueShrines.Add(Id);
 }
 if ((!ActiveShrineId.IsNone() && !UniqueShrines.Contains(ActiveShrineId))
     || (ActiveShrineId.IsNone() && !UniqueShrines.IsEmpty())) return false;
 // The checkpoint is the saved shrine, not the hero's airborne position.
 // Falling or jumping must not suppress progress saves or ground-loot pickup.
 // Still reject invalid world positions, and validate the shrine above.
 if (GetActorLocation().ContainsNaN() || GetActorLocation().SizeSquared() >= 1.0e14f) return false;
 UMoonfallPersistentSave* Save = Cast<UMoonfallPersistentSave>(
     UGameplayStatics::CreateSaveGameObject(UMoonfallPersistentSave::StaticClass()));
 if (!Save) return false;
 // Never overwrite the shrine checkpoint with the hero's current position.
 Save->CheckpointLocation = RespawnCheckpointLocation;
 Save->ActiveShrineId = ActiveShrineId;
 Save->DiscoveredShrineIds = DiscoveredShrineIds;
 Save->ExploredZoneFogMasks = ExploredZoneFogMasks;
 Save->DiscoveredDungeonMask = DiscoveredDungeonMask;
 Save->DefeatedDungeonBossMask = DefeatedDungeonBossMask;
 Save->DungeonTreasureStates = DungeonTreasureStates;
 Save->PendingWorldLoot = PendingWorldLoot;
 Save->RaceId = FMath::Clamp(RaceId, 0, 11);
 Save->FactionReputation = FactionReputation;
 Save->ResidentBonds = ResidentBonds;
 Save->ResidentQuestStates = ResidentQuestStates;
 Save->bRaceChosen = bRaceChosen;
 const float Cooldown = GetRacePowerCooldownRemaining();
 Save->RacePowerReadyUtc = Cooldown > 0.f
     ? FDateTime::UtcNow().ToUnixTimestamp() + FMath::CeilToInt64(FMath::Min(Cooldown, 45.f)) : 0;
 Save->Level = FMath::Clamp(Level, 1, 100);
 Save->Experience = FMath::Clamp(Experience, 0, 1000000);
 Save->Salvage = FMath::Clamp(Salvage, 0, 100000000);
 Save->MaxHealth = FMath::Clamp(MaxHealth, 1.f, 100000.f);
 // Retain inventory and the worn IDs so replacing gear never wipes old loot.
 Save->InventoryItemCodes = InventoryItemCodes;
 Save->EquippedWeaponCode = EquippedWeaponCode;
 Save->EquippedArmorCode = EquippedArmorCode;
 Save->EquippedAccessoryCode = EquippedAccessoryCode;
 Save->LeftSpell = LeftSpell;
 Save->RightSpell = RightSpell;
 return UGameplayStatics::SaveGameToSlot(Save, MoonfallWorldSlot, 0);
}
bool AMoonfallHeroPawn::LoadWorldProgress() {
 if (!UGameplayStatics::DoesSaveGameExist(MoonfallWorldSlot, 0)) return false;
 const UMoonfallPersistentSave* Save = Cast<UMoonfallPersistentSave>(
     UGameplayStatics::LoadGameFromSlot(MoonfallWorldSlot, 0));
 if (!Save || Save->Version < 1 || Save->Version > 12) {
  // Protect newer/corrupt-but-detected saves. Calling Tick's two-second
  // autosave must not replace progress from another Moonfall version.
  bPersistentSaveWritesBlocked = true;
  UE_LOG(LogTemp, Warning, TEXT("Moonfall save slot is unreadable or has an unsupported version; writes disabled."));
  return false;
 }
 // Fail closed on oversized legacy or V7 collections. Simply scanning the
 // first 10,000 inventory entries (or skipping a large quest-state map)
 // would silently discard the remainder on the next two-second autosave.
 // Keep the original slot intact rather than saving truncated progress.
 if (Save->InventoryItemCodes.Num() > 10000
     || (Save->Version >= 6 && Save->ResidentQuestStates.Num() > 64)) {
  bPersistentSaveWritesBlocked = true;
  UE_LOG(LogTemp, Warning, TEXT("Moonfall saved inventory or resident quests exceed supported bounds; original save protected."));
  return false;
 }
 // Reputation and bond records must be valid before restoration; otherwise
 // the allowlist/clamping below would silently alter the next saved version.
 if (Save->Version >= 4) {
  static const FName AllowedFactionIds[] = {FName(TEXT("Wanderers")),
   FName(TEXT("VerdantCircle")), FName(TEXT("StoneHearth")),
   FName(TEXT("MoonCoven")), FName(TEXT("AshenClans"))};
  for (const auto& Entry : Save->FactionReputation) {
   bool bKnown = false;
   for (const FName Id : AllowedFactionIds) if (Entry.Key == Id) { bKnown = true; break; }
   if (!bKnown || Entry.Value < -100 || Entry.Value > 100) {
    bPersistentSaveWritesBlocked = true;
    UE_LOG(LogTemp, Warning, TEXT("Moonfall save contains invalid faction reputation; original save protected."));
    return false;
   }
  }
 }
 if (Save->Version >= 5) {
  static const FName AllowedResidentIds[] = {
   FName(TEXT("CrossroadsGuide")), FName(TEXT("Alden")),
   FName(TEXT("Starweaver")), FName(TEXT("Leafwhisper")),
   FName(TEXT("Anvilkeeper")), FName(TEXT("PipHearthfoot")),
   FName(TEXT("EmberScout")), FName(TEXT("Geargrin")),
   FName(TEXT("Mossfang")), FName(TEXT("Birchroot")),
   FName(TEXT("Duskveil")), FName(TEXT("Silverwatch")),
   FName(TEXT("Ashscale"))
  };
  for (const auto& Entry : Save->ResidentBonds) {
   bool bKnown = false;
   for (const FName Id : AllowedResidentIds) if (Entry.Key == Id) { bKnown = true; break; }
   if (!bKnown || Entry.Value < -100 || Entry.Value > 100) {
    bPersistentSaveWritesBlocked = true;
    UE_LOG(LogTemp, Warning, TEXT("Moonfall save contains invalid resident bond; original save protected."));
    return false;
   }
  }
 }
 // Protect all saved quest entries rather than filtering unsupported
 // residents or states and overwriting their original progress on autosave.
 if (Save->Version >= 6) {
  for (const auto& Entry : Save->ResidentQuestStates) {
   if (!IsNamedMoonfallResident(Entry.Key) || (Entry.Value != 1 && Entry.Value != 2)) {
    bPersistentSaveWritesBlocked = true;
    UE_LOG(LogTemp, Warning, TEXT("Moonfall save contains invalid resident quest state; original save protected."));
    return false;
   }
  }
 }
 // Do not silently discard malformed saved gear then overwrite the original
 // slot during autosave. Leave the slot untouched for backup or repair.
 for (const int32 Code : Save->InventoryItemCodes) {
  if (!ValidMoonfallItemCode(Code)) {
   bPersistentSaveWritesBlocked = true;
   UE_LOG(LogTemp, Warning, TEXT("Moonfall save contains invalid equipment; original inventory protected."));
   return false;
  }
 }
 // Validate worn references before restoring anything. Otherwise RestoreGear
 // would clear an unowned or wrong-slot item and a later autosave would
 // permanently overwrite the original save with the missing equipment.
 const int32 SavedWornCodes[3] = {
  Save->EquippedWeaponCode, Save->EquippedArmorCode, Save->EquippedAccessoryCode
 };
 for (int32 Slot = 0; Slot < 3; ++Slot) {
  const int32 Code = SavedWornCodes[Slot];
  if (Code != 0 && (!ValidMoonfallItemCode(Code)
      || ((Code / 10000) % 10) != Slot || !Save->InventoryItemCodes.Contains(Code))) {
   bPersistentSaveWritesBlocked = true;
   UE_LOG(LogTemp, Warning, TEXT("Moonfall save has inconsistent equipped gear; original save protected."));
   return false;
  }
 }
 // Validate the V7 discovered-shrine ledger before touching live progress.
 // Old V1-V6 saves retain their legacy checkpoint until a shrine is found.
 if (Save->Version >= 7) {
  TSet<FName> UniqueShrines;
  if (Save->DiscoveredShrineIds.Num() > 64) {
   bPersistentSaveWritesBlocked = true;
   return false;
  }
  for (const FName Id : Save->DiscoveredShrineIds) {
   if (Id.IsNone() || UniqueShrines.Contains(Id)) {
    bPersistentSaveWritesBlocked = true;
    return false;
   }
   UniqueShrines.Add(Id);
  }
  if ((!Save->ActiveShrineId.IsNone() && !UniqueShrines.Contains(Save->ActiveShrineId))
      || (Save->ActiveShrineId.IsNone() && !UniqueShrines.IsEmpty())) {
   bPersistentSaveWritesBlocked = true;
   return false;
  }
 }
 // Reject unsupported dungeon bits without modifying live state or overwriting the slot.
 if (Save->Version >= 8 &&
     (static_cast<uint32>(Save->DiscoveredDungeonMask) & ~((1u << 30) - 1u)) != 0u) {
  bPersistentSaveWritesBlocked = true;
  return false;
 }
 // V10: Reject invalid/unexplored boss bits rather than silently
 // overwriting the original save through the two-second autosave.
 if (Save->Version >= 10 &&
     ((static_cast<uint32>(Save->DefeatedDungeonBossMask) & ~((1u << 30) - 1u)) != 0u
      || (static_cast<uint32>(Save->DefeatedDungeonBossMask)
          & ~static_cast<uint32>(Save->DiscoveredDungeonMask)) != 0u)) {
  bPersistentSaveWritesBlocked = true;
  return false;
 }
 // Validate pending/claimed rewards before applying loaded progress.
 if (Save->Version >= 9) {
  if (Save->DungeonTreasureStates.Num() > 420) {
   bPersistentSaveWritesBlocked = true;
   return false;
  }
  for (int32 Index = 0; Index < Save->DungeonTreasureStates.Num(); ++Index) {
   const int32 Code = Save->DungeonTreasureStates[Index];
   if ((Code != 0 && Code != -1 && !ValidMoonfallItemCode(Code))
       || (Code != 0 && (Index % 14 == 0 || Index % 14 == 13))) {
    bPersistentSaveWritesBlocked = true;
    return false;
   }
  }
 }
 // V11 rejects corrupted or duplicated drops before mutating live progress.
 if (Save->Version >= 11) {
  if (Save->PendingWorldLoot.Num() > 512) { bPersistentSaveWritesBlocked = true; return false; }
  TSet<FGuid> Seen;
  for (const FMoonfallWorldLootRecord& Record : Save->PendingWorldLoot) {
   if (!Record.DropId.IsValid() || Seen.Contains(Record.DropId)
       || !ValidMoonfallItemCode(Record.ItemCode) || Record.LevelId.IsNone()
       || Record.Location.ContainsNaN() || Record.Location.SizeSquared() >= 1.0e14f) {
    bPersistentSaveWritesBlocked = true;
    return false;
   }
   Seen.Add(Record.DropId);
  }
 }
 // V12: fail closed on missing/oversized or invalid geographic exploration masks.
 // V1-V11 saves legitimately have no fog history and start with unrevealed cells.
 if (Save->Version >= 12) {
  if (Save->ExploredZoneFogMasks.Num() != 6) {
   bPersistentSaveWritesBlocked = true;
   return false;
  }
  const uint64 AllowedFogBits = (1ULL << 56) - 1ULL;
  for (const int64 Mask : Save->ExploredZoneFogMasks) {
   if ((static_cast<uint64>(Mask) & ~AllowedFogBits) != 0ULL) {
    bPersistentSaveWritesBlocked = true;
    return false;
   }
  }
 }
 // Validate and resolve the checkpoint before touching saved progression.
 // An unchecked SetActorLocation can embed the pawn inside solid geometry.
 const FVector SavedCheckpoint = Save->CheckpointLocation;
 if (SavedCheckpoint.ContainsNaN() || SavedCheckpoint.SizeSquared() >= 1.0e14f) {
  bPersistentSaveWritesBlocked = true;
  UE_LOG(LogTemp, Warning, TEXT("Moonfall save checkpoint is invalid; preserving original save."));
  return false;
 }
 // Teleport can synchronously cause shrine overlap callbacks; do not let
 // those callbacks save stale loot, NPC state or the previous shrine ledger.
 bWorldProgressLoadInProgress = true;
 FVector ResolvedCheckpoint = SavedCheckpoint;
 if (!TeleportTo(SavedCheckpoint, GetActorRotation(), false, false)) {
  // Saved position may be blocked by new dungeon geometry. Recover at an
  // authored PlayerStart instead of reviving inside a wall or at death site.
  if (!RestoreSafeSpawnLocation()) {
   bPersistentSaveWritesBlocked = true;
   UE_LOG(LogTemp, Warning, TEXT("Moonfall checkpoint and PlayerStart are blocked; writes disabled."));
   bWorldProgressLoadInProgress = false;
   return false;
  }
  // The fallback teleport is collision-checked. Remember its actual location
  // instead of persisting the blocked shrine coordinates on the next autosave.
  // Retain the discovered-shrine ledger so existing progression is not lost.
  ResolvedCheckpoint = GetActorLocation();
  UE_LOG(LogTemp, Warning, TEXT("Moonfall checkpoint blocked; using a safe PlayerStart."));
 }
 bPersistentSaveWritesBlocked = false;
 RespawnCheckpointLocation = ResolvedCheckpoint;
 bHasRespawnCheckpointLocation = true;
 ActiveShrineId = Save->Version >= 7 ? Save->ActiveShrineId : NAME_None;
 DiscoveredShrineIds = Save->Version >= 7 ? Save->DiscoveredShrineIds : TArray<FName>();
 ExploredZoneFogMasks = Save->Version >= 12 ? Save->ExploredZoneFogMasks : TArray<int64>();
 DiscoveredDungeonMask = Save->Version >= 8 ? Save->DiscoveredDungeonMask : 0;
 DefeatedDungeonBossMask = Save->Version >= 10 ? Save->DefeatedDungeonBossMask : 0;
 DungeonTreasureStates = Save->Version >= 9
   ? Save->DungeonTreasureStates : TArray<int32>();
 PendingWorldLoot = Save->Version >= 11
   ? Save->PendingWorldLoot : TArray<FMoonfallWorldLootRecord>();
 // Refresh any shrines that began play before the save was loaded.
 // This also fixes stale ACTIVE labels when death reloads an earlier save.
 // A shrine spawned later refreshes itself in its own BeginPlay.
 if (GetWorld()) {
  for (TActorIterator<AMoonfallCheckpointShrine> It(GetWorld()); It; ++It)
   if (IsValid(*It)) It->RefreshShrineDisplay(this);
 }
 ResidentQuestStates.Reset();
 if (Save->Version >= 6) {
  // Ignore unknown, malformed and oversized quest entries from disk.
  if (Save->ResidentQuestStates.Num() <= 64) {
   for (const auto& Entry : Save->ResidentQuestStates)
    if (IsNamedMoonfallResident(Entry.Key) && (Entry.Value == 1 || Entry.Value == 2))
     ResidentQuestStates.Add(Entry.Key, Entry.Value);
  }
 }
 // Old saves begin with no personal bonds. Only named settlement residents are restored.
 ResidentBonds.Reset();
 if (Save->Version >= 5) {
  static const FName ValidIds[] = {
   FName(TEXT("CrossroadsGuide")), FName(TEXT("Alden")),
   FName(TEXT("Starweaver")), FName(TEXT("Leafwhisper")),
   FName(TEXT("Anvilkeeper")), FName(TEXT("PipHearthfoot")),
   FName(TEXT("EmberScout")), FName(TEXT("Geargrin")),
   FName(TEXT("Mossfang")), FName(TEXT("Birchroot")),
   FName(TEXT("Duskveil")), FName(TEXT("Silverwatch")),
   FName(TEXT("Ashscale"))
  };
  for (FName Key : ValidIds) {
   if (const int32* Value = Save->ResidentBonds.Find(Key))
    ResidentBonds.Add(Key, FMath::Clamp(*Value, -100, 100));
  }
 }
 // Do not trust malformed faction records or unknown save keys.
 FactionReputation.Reset();
 if (Save->Version >= 4) {
  static const FName Allowed[] = {FName(TEXT("Wanderers")),
   FName(TEXT("VerdantCircle")), FName(TEXT("StoneHearth")),
   FName(TEXT("MoonCoven")), FName(TEXT("AshenClans"))};
  for (const FName Key : Allowed) {
   if (const int32* Value = Save->FactionReputation.Find(Key))
    FactionReputation.Add(Key, FMath::Clamp(*Value, -100, 100));
  }
 }
 // Old save versions remain valid. Cooldowns cannot be cleared by reloading.
 const int64 NowUtc = FDateTime::UtcNow().ToUnixTimestamp();
 const int64 Remaining = Save->Version >= 3 && Save->RacePowerReadyUtc > NowUtc
     ? FMath::Min<int64>(Save->RacePowerReadyUtc - NowUtc, 45) : 0;
 RacePowerReadyAt = GetWorld() ? GetWorld()->GetTimeSeconds() + static_cast<float>(Remaining) : 0.f;
 RacePowerActiveUntil = 0.f;
 RaceId = Save->Version >= 2 ? FMath::Clamp(Save->RaceId, 0, 11) : 0;
 bRaceChosen = Save->Version >= 2 && Save->bRaceChosen;
 Level = FMath::Clamp(Save->Level, 1, 100);
 Experience = FMath::Clamp(Save->Experience, 0, 1000000);
 ExperienceToNextLevel = 100 + (Level - 1) * 55;
 Salvage = FMath::Clamp(Save->Salvage, 0, 100000000);
 MaxHealth = FMath::IsFinite(Save->MaxHealth)
     ? FMath::Clamp(Save->MaxHealth, 1.f, 100000.f) : 100.f;
 InventoryItemCodes.Reset();
 // Cap the number of inspected entries, not just accepted items: an oversized
 // corrupted save with invalid codes must not cause an unbounded load loop.
 const int32 LoadScanCount = FMath::Min(Save->InventoryItemCodes.Num(), 10000);
 for (int32 Index = 0; Index < LoadScanCount; ++Index) {
  const int32 Code = Save->InventoryItemCodes[Index];
  if (ValidMoonfallItemCode(Code)) InventoryItemCodes.Add(Code);
 }
 const auto RestoreGear = [this](int32 Code, int32 Slot, int32& Equipped, int32& Tier, int32& Affix) {
  if (ValidMoonfallItemCode(Code) && ((Code / 10000) % 10) == Slot && InventoryItemCodes.Contains(Code)) {
   Equipped = Code;
   Tier = Code % 1000;
   Affix = Code / 100000;
  } else {
   Equipped = 0;
   Tier = 0;
   Affix = 0;
  }
 };
 RestoreGear(Save->EquippedWeaponCode, 0, EquippedWeaponCode, WeaponTier, WeaponAffix);
 RestoreGear(Save->EquippedArmorCode, 1, EquippedArmorCode, ArmorTier, ArmorAffix);
 RestoreGear(Save->EquippedAccessoryCode, 2, EquippedAccessoryCode, AccessoryTier, AccessoryAffix);
 EquipmentTier = FMath::Max(WeaponTier, FMath::Max(ArmorTier, AccessoryTier));
 LeftSpell = FMath::Clamp(Save->LeftSpell, 0, 100);
 RightSpell = FMath::Clamp(Save->RightSpell, 0, 100);
 Health = MaxHealth;
 Mana = FMath::Max(0.f, MaxMana);
 Stamina = FMath::Max(0.f, MaxStamina);
 // Location was already collision-checked before restoring inventory.
 OnHealthChanged(Health, MaxHealth);
 bWorldProgressLoadInProgress = false;
 return true;
}
void AMoonfallHeroPawn::RespawnAtCheckpoint() {
 DefeatElapsedSeconds = 0.f;
 // No new run and no loot reset. Prefer a fully restored on-disk checkpoint.
 if (!LoadWorldProgress()) {
  // A missing, unreadable or blocked save should not erase progress. Try the
  // last known in-memory shrine (if valid) before the authored level spawn.
  // All teleports use collision checks, never a forced placement.
  bool bFoundSafeRespawn = bHasRespawnCheckpointLocation
      && !RespawnCheckpointLocation.ContainsNaN()
      && RespawnCheckpointLocation.SizeSquared() < 1.0e14f
      && TeleportTo(RespawnCheckpointLocation, GetActorRotation(), false, false);
  if (!bFoundSafeRespawn) {
   bFoundSafeRespawn = RestoreSafeSpawnLocation();
   if (bFoundSafeRespawn) {
    // A memory-only checkpoint can become blocked when world geometry changes.
    // Remember the collision-checked fallback, otherwise a subsequent autosave
    // could re-persist the blocked position despite a successful rescue.
    RespawnCheckpointLocation = GetActorLocation();
    bHasRespawnCheckpointLocation = true;
   }
  }
  if (!bFoundSafeRespawn) {
   // Do not resurrect at the lethal position when every spawn is blocked.
   // Keep the defeat state and retry through the normal 3-second timer.
   // An incompatible save remains write-protected.
   UE_LOG(LogTemp, Warning, TEXT("Moonfall has no collision-safe shrine, PlayerStart or initial spawn; staying defeated."));
   return;
  }
  Health = FMath::Max(1.f, MaxHealth);
  Mana = FMath::Max(0.f, MaxMana);
  Stamina = FMath::Max(0.f, MaxStamina);
  OnHealthChanged(Health, MaxHealth);
 }
 if (!bPersistentSaveWritesBlocked) RestorePendingWorldEquipmentDrops();
 if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
 if (GetWorld()) {
  const float Now = GetWorld()->GetTimeSeconds();
  DamageImmunityUntil = FMath::Max(DamageImmunityUntil, Now + 3.f);
  NextSafeCheckpointAt = Now + 5.f;
  SecondsSinceLastWorldSave = 0.f;
  // A collision-aware respawn can trigger shrine overlap while Health <= 0,
  // before the saved or fallback health restoration. That overlap is refused
  // and does not necessarily fire again once the hero is alive. Recheck once
  // after a successful respawn, never while stuck at an unsafe death site.
  for (TActorIterator<AMoonfallCheckpointShrine> It(GetWorld()); It; ++It)
   if (IsValid(*It)) It->CheckHeroAtSpawn(this);
 }
}

// One unified roster. Every ancestry shares the same world, quests and progression.
// Index order is stable across saves; never reorder IDs in a shipped build.
namespace {
struct FMoonfallRaceProfile {
 const TCHAR* Name;
 int32 Might;
 int32 Insight;
 int32 Resolve;
 float StaminaRegen;
 float ManaRegen;
};
constexpr FMoonfallRaceProfile MoonfallRaces[] = {
 {TEXT("Human"),      2, 2, 2, 0.02f, 0.02f},
 {TEXT("High Elf"),   0, 6, 0, 0.00f, 0.10f},
 {TEXT("Wood Elf"),   3, 3, 0, 0.10f, 0.02f},
 {TEXT("Dwarf"),      3, 0, 5, 0.06f, 0.00f},
 {TEXT("Halfling"),   1, 3, 2, 0.12f, 0.02f},
 {TEXT("Orc"),        6, 0, 2, 0.07f, 0.00f},
 {TEXT("Goblin"),     3, 3, 0, 0.12f, 0.02f},
 {TEXT("Troll"),      4, 0, 5, 0.03f, 0.00f},
 {TEXT("Entkin"),     1, 2, 6, 0.02f, 0.04f},
 {TEXT("Shadowborn"), 4, 4, 0, 0.05f, 0.05f},
 {TEXT("Moonborn"),   0, 5, 3, 0.00f, 0.12f},
 {TEXT("Draconid"),   5, 2, 2, 0.04f, 0.04f}
};
const FMoonfallRaceProfile& RaceProfile(int32 Id) {
 return MoonfallRaces[FMath::Clamp(Id, 0, 11)];
}
}
bool AMoonfallHeroPawn::ChooseRace(int32 NewRaceId) {
 // Character creation is a single deliberate choice, not an unlimited respec.
 if (bRaceChosen || NewRaceId < 0 || NewRaceId > 11 || Health <= 0.f) return false;
 const int32 OldId = RaceId;
 RaceId = NewRaceId;
 bRaceChosen = true;
 // Fail closed on save error: no unsaved race lock.
 if (!SaveWorldProgress()) {
  RaceId = OldId;
  bRaceChosen = false;
  return false;
 }
 return true;
}
FString AMoonfallHeroPawn::GetRaceName() const { return FString(RaceProfile(RaceId).Name); }
int32 AMoonfallHeroPawn::GetRaceMightBonus() const { return bRaceChosen ? RaceProfile(RaceId).Might : 0; }
int32 AMoonfallHeroPawn::GetRaceInsightBonus() const { return bRaceChosen ? RaceProfile(RaceId).Insight : 0; }
int32 AMoonfallHeroPawn::GetRaceResolveBonus() const { return bRaceChosen ? RaceProfile(RaceId).Resolve : 0; }
float AMoonfallHeroPawn::GetRaceStaminaRegenBonus() const { return bRaceChosen ? RaceProfile(RaceId).StaminaRegen : 0; }
float AMoonfallHeroPawn::GetRaceManaRegenBonus() const { return bRaceChosen ? RaceProfile(RaceId).ManaRegen : 0; }


namespace {
const TCHAR* MoonfallRacePowerNames[] = {
 TEXT("Adventurer's Resolve"), TEXT("Starfire Focus"), TEXT("Forest Step"),
 TEXT("Stoneguard"), TEXT("Lucky Escape"), TEXT("War Cry"),
 TEXT("Quickcraft"), TEXT("Trollhide"), TEXT("Rootward"),
 TEXT("Night Veil"), TEXT("Moon Surge"), TEXT("Drakeheart")
};
}
FString AMoonfallHeroPawn::GetRacePowerName() const {
 return FString(MoonfallRacePowerNames[FMath::Clamp(RaceId, 0, 11)]);
}
float AMoonfallHeroPawn::GetRacePowerCooldownRemaining() const {
 if (!GetWorld()) return 0.f;
 return FMath::Max(0.f, RacePowerReadyAt - GetWorld()->GetTimeSeconds());
}
float AMoonfallHeroPawn::GetRacePowerDurationRemaining() const {
 if (!GetWorld()) return 0.f;
 return FMath::Max(0.f, RacePowerActiveUntil - GetWorld()->GetTimeSeconds());
}
bool AMoonfallHeroPawn::ActivateRacePower() {
 if (!bRaceChosen || !GetWorld() || Health <= 0.f || bInventoryOpen || bOptionsOpen) return false;
 const float Now = GetWorld()->GetTimeSeconds();
 if (!FMath::IsFinite(Now) || Now < RacePowerReadyAt) return false;
 // Preserve state so a failed disk save never consumes a one-time power.
 const float PreviousHealth = Health;
 const float PreviousMana = Mana;
 const float PreviousStamina = Stamina;
 const float PreviousReadyAt = RacePowerReadyAt;
 const float PreviousActiveUntil = RacePowerActiveUntil;
 // Unique racial power effects (bounded resources and temporary buffs).
 const float SafeMaxMana = FMath::IsFinite(MaxMana) ? FMath::Clamp(MaxMana, 0.f, 100000.f) : 0.f;
 const float SafeMaxStamina = FMath::IsFinite(MaxStamina) ? FMath::Clamp(MaxStamina, 0.f, 100000.f) : 0.f;
 const float SafeMaxHealth = FMath::IsFinite(MaxHealth) ? FMath::Clamp(MaxHealth, 1.f, 100000.f) : 100.f;
 float Duration = 8.f;
 switch (RaceId) {
 case 0: // Human: balanced recovery.
  Mana = FMath::Clamp(Mana + SafeMaxMana * 0.15f, 0.f, SafeMaxMana);
  Stamina = FMath::Clamp(Stamina + SafeMaxStamina * 0.15f, 0.f, SafeMaxStamina);
  break;
 case 1: // High Elf: arcane focus, increases spells while active.
  Mana = FMath::Clamp(Mana + SafeMaxMana * 0.30f, 0.f, SafeMaxMana);
  break;
 case 2: // Wood Elf: longer agility / stamina boon.
  Duration = 12.f;
  Stamina = FMath::Clamp(Stamina + SafeMaxStamina * 0.35f, 0.f, SafeMaxStamina);
  break;
 case 3: // Dwarf: short damage ward.
  Duration = 10.f;
  Stamina = FMath::Clamp(Stamina + SafeMaxStamina * 0.10f, 0.f, SafeMaxStamina);
  break;
 case 4: // Halfling: large stamina refill.
  Duration = 6.f;
  Stamina = FMath::Clamp(Stamina + SafeMaxStamina * 0.45f, 0.f, SafeMaxStamina);
  break;
 case 5: // Orc: battle stamina and a small restorative heal.
  Health = FMath::Clamp(Health + SafeMaxHealth * 0.08f, 0.f, SafeMaxHealth);
  Stamina = FMath::Clamp(Stamina + SafeMaxStamina * 0.20f, 0.f, SafeMaxStamina);
  break;
 case 6: // Goblin: rapid mana and stamina refill.
  Duration = 5.f;
  Mana = FMath::Clamp(Mana + SafeMaxMana * 0.22f, 0.f, SafeMaxMana);
  Stamina = FMath::Clamp(Stamina + SafeMaxStamina * 0.22f, 0.f, SafeMaxStamina);
  break;
 case 7: // Troll: regeneration and ward.
  Health = FMath::Clamp(Health + SafeMaxHealth * 0.12f, 0.f, SafeMaxHealth);
  break;
 case 8: // Entkin: strongest protective duration, small heal.
  Duration = 14.f;
  Health = FMath::Clamp(Health + SafeMaxHealth * 0.08f, 0.f, SafeMaxHealth);
  break;
 case 9: // Shadowborn: shadow-powered spells and stamina.
  Stamina = FMath::Clamp(Stamina + SafeMaxStamina * 0.22f, 0.f, SafeMaxStamina);
  break;
 case 10: // Moonborn: arcane surge.
  Mana = FMath::Clamp(Mana + SafeMaxMana * 0.38f, 0.f, SafeMaxMana);
  break;
 case 11: // Draconid: balanced magic and a burst of recovery.
  Health = FMath::Clamp(Health + SafeMaxHealth * 0.07f, 0.f, SafeMaxHealth);
  Mana = FMath::Clamp(Mana + SafeMaxMana * 0.18f, 0.f, SafeMaxMana);
  break;
 default: return false;
 }
 RacePowerActiveUntil = Now + Duration;
 RacePowerReadyAt = Now + 45.f;
 // Persist cooldown and refreshed resources together. On disk failure do not
 // grant free power, consume cooldown, or tell UI the ability succeeded.
 if (!SaveWorldProgress()) {
  Health = PreviousHealth;
  Mana = PreviousMana;
  Stamina = PreviousStamina;
  RacePowerReadyAt = PreviousReadyAt;
  RacePowerActiveUntil = PreviousActiveUntil;
  OnHealthChanged(Health, MaxHealth);
  return false;
 }
 OnHealthChanged(Health, MaxHealth);
 OnRacePowerActivated(FName(MoonfallRacePowerNames[RaceId]));
 return true;
}

AMoonfallWorldNPC* AMoonfallHeroPawn::GetNearestResidentNPC() const {
 if (!GetWorld() || Health <= 0.f || bOptionsOpen || bInventoryOpen) return nullptr;
 AMoonfallWorldNPC* Nearest = nullptr;
 float BestDistSq = FMath::Square(400.f);
 int32 Checked = 0;
 // Query only after explicit input; no per-frame scanning or NPC Tick.
 for (TActorIterator<AMoonfallWorldNPC> It(GetWorld()); It && Checked < 64; ++It, ++Checked) {
  AMoonfallWorldNPC* Candidate = *It;
  if (!IsValid(Candidate)) continue;
  const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
  // Choose the closest *reachable* resident. A nearer NPC behind a wall
  // must not prevent talking to another visible NPC on controller.
  if (FMath::IsFinite(DistSq) && DistSq < BestDistSq &&
      Candidate->CanHeroInteract(this)) {
   BestDistSq = DistSq;
   Nearest = Candidate;
  }
 }
 return Nearest;
}
void AMoonfallHeroPawn::CloseResidentContext() {
 // Never depend on actor lifetime for menu cleanup: streaming or destruction
 // can invalidate the NPC pointer without closing the active input modal.
 if (!bResidentContextOpen && !FocusedResident.IsValid()) return;
 bResidentContextOpen = false;
 FocusedResident.Reset();
 // Return cursor ownership to whichever native modal is still active.
 if (APlayerController* PC = Cast<APlayerController>(Controller))
  PC->bShowMouseCursor = bOptionsOpen || bInventoryOpen;
 ResidentContextSelection = 0;
 bResidentTurnInPending = false;
 ResidentTurnInConfirmUntil = 0.f;
 ResidentTurnInConfirmAfter = 0.f;
 ResidentFeedbackMessage.Empty();
 ResidentFeedbackUntil = 0.f;
 OnResidentContextClosed();
}
void AMoonfallHeroPawn::OpenFocusedResidentContext() {
 // One controller D-pad Up mapping serves modal navigation and the NPC menu.
 // Never acquire an NPC target behind the open Atlas.
 if (bOptionsOpen || bInventoryOpen || bAtlasOpen) { OptionsUp(); return; }
 if (!GetWorld() || Health <= 0.f) return;
 if (bResidentContextOpen && FocusedResident.IsValid() && FocusedResident->CanHeroInteract(this)) {
  ResidentContextSelection = (ResidentContextSelection + 2) % 3;
  bResidentTurnInPending = false;
  return;
 }
 AMoonfallWorldNPC* Resident = GetNearestResidentNPC();
 if (!IsValid(Resident) || !Resident->CanHeroInteract(this)) return;
 FocusedResident = Resident;
 bResidentContextOpen = true;
 bResidentTurnInPending = false;
 if (APlayerController* PC = Cast<APlayerController>(Controller)) {
  PC->bShowMouseCursor = true;
 }
 ResidentContextSelection = 0;
 OnResidentContextOpened(Resident, Resident->DisplayName,
     GetResidentQuestState(Resident->ResidentId), Resident->GetContractSalvageCost());
}
void AMoonfallHeroPawn::CastRightMouse() {
 // The map owns right-clicks; never hit-test NPCs behind the Atlas.
 if (bAtlasOpen) return;
 if (bInventoryOpen) {
  if (HitTestInventorySortButton()) return;
  if (HitTestInventoryEquipButton()) return;
  const int32 HitIndex = HitTestInventoryRow();
  if (HitIndex != INDEX_NONE) {
   InventorySelection = HitIndex;
   EquipInventoryItem(HitIndex);
  }
  return;
 }
 if (bResidentContextOpen) { CloseResidentContext(); return; }
 if (Health <= 0.f || bOptionsOpen || bInventoryOpen || bAtlasOpen || !GetWorld()) return;
 // WoW-style mouse pointer selection when a cursor is visible, or
 // AC-like central aim selection when using hidden-cursor mouse look.
 FHitResult Hit;
 bool bHit = false;
 if (APlayerController* PC = Cast<APlayerController>(Controller)) {
  if (PC->bShowMouseCursor) {
   bHit = PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);
  } else {
   FVector Eye;
   FRotator ViewRotation;
   PC->GetPlayerViewPoint(Eye, ViewRotation);
   FCollisionQueryParams Params(SCENE_QUERY_STAT(MoonfallNPCContext), false);
   Params.AddIgnoredActor(this);
   bHit = GetWorld()->LineTraceSingleByChannel(
       Hit, Eye, Eye + ViewRotation.Vector() * 500.f, ECC_Visibility, Params);
  }
 }
 if (bHit) {
  if (AMoonfallWorldNPC* Resident = Cast<AMoonfallWorldNPC>(Hit.GetActor())) {
   // A click on a civilian is never converted to a spell, even when too far.
   if (Resident->CanHeroInteract(this)) {
    FocusedResident = Resident;
    bResidentContextOpen = true;
    bResidentTurnInPending = false;
       if (APlayerController* PC = Cast<APlayerController>(Controller)) {
        PC->bShowMouseCursor = true;
    }
    ResidentContextSelection = 0;
    OnResidentContextOpened(Resident, Resident->DisplayName,
        GetResidentQuestState(Resident->ResidentId), Resident->GetContractSalvageCost());
   } else {
    OnResidentQuestFeedback(FText::FromString(TEXT("Move closer to interact.")), false);
   }
   return;
  }
 }
 // All non-NPC right clicks retain right-hand attack behavior.
 CastRight();
}
void AMoonfallHeroPawn::ReleaseInventoryPointer()
{
 bInventoryScrollbarDragging = false;
}

void AMoonfallHeroPawn::ResidentContextMouseClick() {
 // Handle all Atlas pointer actions before gameplay/NPC input.
 if (bAtlasOpen) {
  if (APlayerController* PC = Cast<APlayerController>(Controller)) {
   int32 ViewW = 0, ViewH = 0;
   float MouseX = 0.f, MouseY = 0.f;
   PC->GetViewportSize(ViewW, ViewH);
   if (ViewW >= 300 && ViewH >= 240 && PC->bShowMouseCursor && PC->GetMousePosition(MouseX, MouseY)
       && FMath::IsFinite(MouseX) && FMath::IsFinite(MouseY)) {
    // Keep hit tests in sync with MoonfallCombatHUD's Atlas panel.
    const float PanelW = FMath::Min(900.f, float(ViewW) - 24.f);
    const float PanelH = FMath::Min(650.f, float(ViewH) - 24.f);
    const float PanelL = (float(ViewW) - PanelW) * 0.5f;
    const float PanelT = (float(ViewH) - PanelH) * 0.5f;
    const float CloseX = PanelL + PanelW - 42.f;
    if (MouseX >= CloseX && MouseX <= CloseX + 28.f
        && MouseY >= PanelT + 12.f && MouseY <= PanelT + 40.f) {
     ToggleAtlas();
     return;
    }
    const float SidebarWidth = FMath::Clamp(PanelW * 0.27f, 132.f, 238.f);
    const float RowGap = FMath::Clamp((PanelH - 145.f) / 6.f, 24.f, 78.f);
    // Region selections are confined to the sidebar, never the painted map.
    if (MouseX >= PanelL + 13.f && MouseX <= PanelL + SidebarWidth - 9.f) {
     for (int32 Zone = 0; Zone < 6; ++Zone) {
      const float RowY = PanelT + 110.f + Zone * RowGap;
      if (MouseY >= RowY - 4.f && MouseY < RowY - 4.f + FMath::Max(20.f, RowGap - 4.f)) {
       AtlasSelectedRegion = Zone;
       return;
      }
     }
    }
    // Clicking the parchment chart pans toward the chosen XY world coordinate.
    const float MapL = PanelL + SidebarWidth + 19.f;
    const float MapW = FMath::Max(40.f, PanelW - SidebarWidth - 36.f);
    const float MapT = PanelT + 105.f;
    const float MapH = FMath::Max(74.f, PanelH - 141.f);
    if (MouseX >= MapL && MouseX <= MapL + MapW
        && MouseY >= MapT && MouseY <= MapT + MapH) {
     const float Zoom = FMath::Clamp(FMath::IsFinite(AtlasZoom) ? AtlasZoom : 1.f, 0.8f, 1.8f);
     const float WorldCenterX = (float(AtlasSelectedRegion * 5 + 2) - 14.5f) * 1350.f;
     const float WorldX = WorldCenterX + ((MouseX - MapL) / MapW - 0.5f) * (42000.f / Zoom);
     const float SafeAtlasPanY = FMath::Clamp(FMath::IsFinite(AtlasPanY) ? AtlasPanY : -2500.f, -12000.f, 12000.f);
     const float WorldY = SafeAtlasPanY + (0.5f - (MouseY - MapT) / MapH) * (13000.f / Zoom);
     if (FMath::IsFinite(WorldX) && FMath::IsFinite(WorldY)) {
      const int32 Slot = FMath::Clamp(FMath::RoundToInt(WorldX / 1350.f + 14.5f), 0, 29);
      AtlasSelectedRegion = Slot / 5;
      AtlasPanY = FMath::Clamp(WorldY, -12000.f, 12000.f);
     }
     return;
    }
   }
  }
  // Clicks outside Atlas controls must never cast or activate NPCs.
  return;
 }
 // All native modal panels own their mouse input before gameplay combat.
 if (bOptionsOpen) {
  if (APlayerController* PC = Cast<APlayerController>(Controller)) {
   int32 Width = 0, Height = 0;
   float MouseX = 0.f, MouseY = 0.f;
   PC->GetViewportSize(Width, Height);
   if (Width > 0 && Height > 0 && PC->bShowMouseCursor
       && PC->GetMousePosition(MouseX, MouseY)) {
    const float MenuWidth = FMath::Min(560.f, FMath::Max(240.f, float(Width) - 24.f));
    const float MenuHeight = FMath::Min(550.f, FMath::Max(280.f, float(Height) - 24.f));
    const float MenuLeft = (float(Width) - MenuWidth) * 0.5f;
    const float MenuTop = (float(Height) - MenuHeight) * 0.5f;
    const float CloseX = MenuLeft + MenuWidth - 42.f;
    if (MouseX >= CloseX && MouseX <= CloseX + 28.f
        && MouseY >= MenuTop + 12.f && MouseY <= MenuTop + 40.f) {
     OpenOptions();
    } else {
     // Match all eight options rows and +/- button bounds to MoonfallCombatHUD.
     const float RowGap = (MenuHeight - 130.f) / 8.f;
     const float RowHeight = FMath::Min(38.f, RowGap - 2.f);
     const float FirstRowY = MenuTop + 63.f;
     const int32 ClickedRow = FMath::FloorToInt((MouseY - FirstRowY) / RowGap);
     const float RowY = FirstRowY + ClickedRow * RowGap;
     if (MouseX >= MenuLeft + 10.f && MouseX <= MenuLeft + MenuWidth - 10.f
         && ClickedRow >= 0 && ClickedRow < 8
         && MouseY >= RowY && MouseY <= RowY + RowHeight) {
      OptionsSelection = ClickedRow;
      const float MinusX = MenuLeft + MenuWidth - 145.f;
      const float PlusX = MenuLeft + MenuWidth - 37.f;
      const float ButtonHeight = FMath::Min(26.f, RowHeight);
      if (MouseY <= RowY + ButtonHeight) {
       if (MouseX >= MinusX && MouseX <= MinusX + 25.f) AdjustOption(-1);
       else if (MouseX >= PlusX && MouseX <= PlusX + 25.f) AdjustOption(1);
      }
     }
    }
   }
  }
  return;
 }
 if (bInventoryOpen) {
  if (HitTestInventoryCloseButton()) {
   // The X and Esc must back out of character details the same way.
   OpenOptions();
   return;
  }
  if (HitTestInventorySortButton()) {
   InventorySortMode = (InventorySortMode + 1) % 3;
   KeepInventorySelectionVisible();
   return;
  }
  if (HitTestInventoryEquipButton()) {
   EquipSelectedInventoryItem();
   return;
  }
  if (ScrollInventoryToMouseTrack()) return;
  const int32 HitIndex = HitTestInventoryRow();
  if (HitIndex != INDEX_NONE) InventorySelection = HitIndex;
  return;
 }
 if (!bResidentContextOpen) { CastLeft(); return; }
 if (!FocusedResident.IsValid()) { CloseResidentContext(); return; }
 if (!GetWorld() || bOptionsOpen || bInventoryOpen) { CloseResidentContext(); return; }
 APlayerController* PC = Cast<APlayerController>(Controller);
 if (!PC) return;
 int32 Width = 0, Height = 0;
 float MouseX = 0.f, MouseY = 0.f;
 PC->GetViewportSize(Width, Height);
 if (Width <= 0 || Height <= 0 || !PC->GetMousePosition(MouseX, MouseY)) return;
 const float PanelW = FMath::Min(490.f, FMath::Max(260.f, Width - 24.f));
 const float PanelH = FMath::Min(345.f, FMath::Max(270.f, Height - 24.f));
 const float Left = (Width - PanelW) * 0.5f, Top = (Height - PanelH) * 0.5f;
 const float NpcCloseX = Left + PanelW - 42.f;
 if (MouseX >= NpcCloseX && MouseX <= NpcCloseX + 28.f
     && MouseY >= Top + 12.f && MouseY <= Top + 40.f) {
  CloseResidentContext();
  return;
 }
 if (MouseX < Left || MouseX > Left + PanelW || MouseY < Top || MouseY > Top + PanelH) {
  CloseResidentContext();
  return;
 }
 // Shared exact row bounds with the Canvas HUD renderer.
 for (int32 Row = 0; Row < 3; ++Row) {
  const float RowTop = Top + 122.f + Row * 53.f;
  if (MouseY >= RowTop && MouseY <= RowTop + 43.f) {
   if (ResidentContextSelection != Row) bResidentTurnInPending = false;
   ResidentContextSelection = Row;
   if (Row == 2) CloseResidentContext();
   else ChooseResidentContextAction(Row);
   return;
  }
 }
}
void AMoonfallHeroPawn::TalkToNearestNPC() {
 if (AMoonfallWorldNPC* Resident = GetNearestResidentNPC()) {
  if (Resident->CanHeroInteract(this)) Resident->TryTalk(this);
 }
}
void AMoonfallHeroPawn::InteractWithNearestResidentQuest() {
 // Existing Blueprint/API compatibility. Never auto-spend salvage.
 OpenFocusedResidentContext();
}
bool AMoonfallHeroPawn::ChooseResidentContextAction(int32 Choice) {
 AMoonfallWorldNPC* Resident = FocusedResident.Get();
 if (!bResidentContextOpen || !IsValid(Resident) || !Resident->CanHeroInteract(this)) {
  CloseResidentContext();
  return false;
 }
 if (Choice == 2) { CloseResidentContext(); return true; }
 if (Choice == 0) {
  bResidentTurnInPending = false;
  if (!Resident->TryTalk(this)) return false;
  ResidentFeedbackMessage = Resident->LastSpokenLine.ToString();
  ResidentFeedbackUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 8.f : 0.f;
  return true;
 }
 if (Choice != 1) return false;
 const int32 State = GetResidentQuestState(Resident->ResidentId);
 bool bSucceeded = false;
 FText Message;
 if (State == 0) {
  bSucceeded = Resident->OfferResidentQuest(this);
  Message = bSucceeded ? FText::FromString(TEXT("Contract accepted."))
    : FText::FromString(TEXT("Contract unavailable."));
 } else if (State == 1) {
  if (Salvage < Resident->GetContractSalvageCost()) {
   bResidentTurnInPending = false;
   Message = FText::FromString(TEXT("Not enough salvage to complete this contract."));
  } else if (!bResidentTurnInPending || !GetWorld()
      || GetWorld()->GetTimeSeconds() > ResidentTurnInConfirmUntil) {
   bResidentTurnInPending = true;
   ResidentTurnInConfirmUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 6.f : 0.f;
   ResidentTurnInConfirmAfter = GetWorld() ? GetWorld()->GetTimeSeconds() + 0.35f : 0.f;
   Message = FText::FromString(FString::Printf(
     TEXT("Confirm spending %d salvage: choose Turn In again."),
     Resident->GetContractSalvageCost()));
  } else if (GetWorld()->GetTimeSeconds() < ResidentTurnInConfirmAfter) {
   Message = FText::FromString(TEXT("Confirm once more after a short moment."));
  } else {
   bResidentTurnInPending = false;
   bSucceeded = Resident->TurnInResidentQuest(this);
   Message = bSucceeded ? FText::FromString(TEXT("Contract complete."))
     : FText::FromString(TEXT("Contract could not be completed."));
  }
 } else {
  Message = FText::FromString(TEXT("This contract is already complete."));
 }
 ResidentFeedbackMessage = Message.ToString();
 ResidentFeedbackUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 4.f : 0.f;
 OnResidentQuestFeedback(Message, bSucceeded);
 // Keep menu available after accepting. Update quest state for UI.
 if (bSucceeded) OnResidentContextOpened(Resident, Resident->DisplayName,
      GetResidentQuestState(Resident->ResidentId), Resident->GetContractSalvageCost());
 return bSucceeded;
}

// Bounded, persistent faction relationship API. No free reputation on repeated talk.
namespace {
bool IsMoonfallFaction(FName Faction) {
 return Faction == FName(TEXT("Wanderers"))
    || Faction == FName(TEXT("VerdantCircle"))
    || Faction == FName(TEXT("StoneHearth"))
    || Faction == FName(TEXT("MoonCoven"))
    || Faction == FName(TEXT("AshenClans"));
}
}
int32 AMoonfallHeroPawn::GetFactionReputation(FName Faction) const {
 if (const int32* Value = FactionReputation.Find(Faction)) return FMath::Clamp(*Value, -100, 100);
 return 0;
}
bool AMoonfallHeroPawn::ChangeFactionReputation(FName Faction, int32 Delta) {
 if (!IsMoonfallFaction(Faction) || Delta == 0 || Health <= 0.f) return false;
 const int32 OldValue = GetFactionReputation(Faction);
 const int32 NewValue = static_cast<int32>(FMath::Clamp(
    static_cast<int64>(OldValue) + static_cast<int64>(Delta),
    static_cast<int64>(-100), static_cast<int64>(100)));
 if (OldValue == NewValue) return false;
 FactionReputation.Add(Faction, NewValue);
 if (!SaveWorldProgress()) {
  if (OldValue == 0) FactionReputation.Remove(Faction);
  else FactionReputation.Add(Faction, OldValue);
  return false;
 }
 return true;
}

namespace {
bool IsNamedMoonfallResident(FName Id) {
 static const FName ResidentIds[] = {
  FName(TEXT("CrossroadsGuide")), FName(TEXT("Alden")),
  FName(TEXT("Starweaver")), FName(TEXT("Leafwhisper")),
  FName(TEXT("Anvilkeeper")), FName(TEXT("PipHearthfoot")),
  FName(TEXT("EmberScout")), FName(TEXT("Geargrin")),
  FName(TEXT("Mossfang")), FName(TEXT("Birchroot")),
  FName(TEXT("Duskveil")), FName(TEXT("Silverwatch")),
  FName(TEXT("Ashscale"))
 };
 for (const FName Candidate : ResidentIds) if (Id == Candidate) return true;
 return false;
}
}
int32 AMoonfallHeroPawn::GetResidentBond(FName ResidentId) const {
 if (!IsNamedMoonfallResident(ResidentId)) return 0;
 if (const int32* Value = ResidentBonds.Find(ResidentId))
  return FMath::Clamp(*Value, -100, 100);
 return 0;
}
bool AMoonfallHeroPawn::ChangeResidentBond(FName ResidentId, int32 Delta) {
 if (!IsNamedMoonfallResident(ResidentId) || Delta == 0 || Health <= 0.f) return false;
 const int32 Previous = GetResidentBond(ResidentId);
 const int32 Next = static_cast<int32>(FMath::Clamp(
     static_cast<int64>(Previous) + static_cast<int64>(Delta),
     static_cast<int64>(-100), static_cast<int64>(100)));
 if (Next == Previous) return false;
 ResidentBonds.Add(ResidentId, Next);
 // Failed disk writes must not grant a relationship or destroy the old one.
 if (!SaveWorldProgress()) {
  if (Previous == 0) ResidentBonds.Remove(ResidentId);
  else ResidentBonds.Add(ResidentId, Previous);
  return false;
 }
 return true;
}

int32 AMoonfallHeroPawn::GetResidentQuestState(FName ResidentId) const {
 if (!IsNamedMoonfallResident(ResidentId)) return 0;
 if (const int32* Value = ResidentQuestStates.Find(ResidentId))
  return (*Value == 1 || *Value == 2) ? *Value : 0;
 return 0;
}
bool AMoonfallHeroPawn::AcceptResidentQuest(FName ResidentId) {
 if (!IsNamedMoonfallResident(ResidentId) || Health <= 0.f
     || bOptionsOpen || bInventoryOpen || GetResidentQuestState(ResidentId) != 0) return false;
 ResidentQuestStates.Add(ResidentId, 1);
 if (!SaveWorldProgress()) {
  ResidentQuestStates.Remove(ResidentId);
  return false;
 }
 return true;
}
bool AMoonfallHeroPawn::CompleteResidentQuest(AMoonfallWorldNPC* Resident) {
 if (!IsValid(Resident) || !Resident->CanHeroInteract(this)
     || !IsNamedMoonfallResident(Resident->ResidentId)
     || GetResidentQuestState(Resident->ResidentId) != 1) return false;
 const int32 Cost = Resident->GetContractSalvageCost();
 if (Cost < 1 || Cost > 30 || Salvage < Cost) return false;
 const FName Id = Resident->ResidentId;
 const FName Faction = Resident->FactionId;
 if (!IsMoonfallFaction(Faction)) return false;
 // One atomic save: pay salvage, earn 6 bond and 3 faction reputation.
 const int32 PreviousSalvage = Salvage;
 const int32 PreviousBond = GetResidentBond(Id);
 const int32 PreviousRep = GetFactionReputation(Faction);
 Salvage -= Cost;
 ResidentQuestStates.Add(Id, 2);
 ResidentBonds.Add(Id, FMath::Clamp(PreviousBond + 6, -100, 100));
 FactionReputation.Add(Faction, FMath::Clamp(PreviousRep + 3, -100, 100));
 if (!SaveWorldProgress()) {
  Salvage = PreviousSalvage;
  ResidentQuestStates.Add(Id, 1);
  if (PreviousBond == 0) ResidentBonds.Remove(Id);
  else ResidentBonds.Add(Id, PreviousBond);
  if (PreviousRep == 0) FactionReputation.Remove(Faction);
  else FactionReputation.Add(Faction, PreviousRep);
  return false;
 }
 return true;
}
