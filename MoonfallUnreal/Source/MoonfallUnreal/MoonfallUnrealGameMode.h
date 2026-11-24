#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
class AMoonfallArena;
class AMoonfallWorldNPC;
#include "MoonfallUnrealGameMode.generated.h"

// Thirty physical dungeons in six world regions. Geometry is spawned in small
// time-sliced batches; the GameMode never polls each dungeon every frame.
UCLASS()
class MOONFALLUNREAL_API AMoonfallUnrealGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AMoonfallUnrealGameMode();
 virtual void BeginPlay() override;

 static constexpr int32 DungeonZoneCount = 6;
 static constexpr int32 DungeonsPerZone = 5;
 static constexpr int32 TotalDungeons = DungeonZoneCount * DungeonsPerZone;
 static constexpr float DungeonLaneSpacing = 1350.f;

 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Moonfall|Dungeon|World")
 bool bGenerateDungeonWorld = true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|World")
 int32 ReadyDungeonCount = 0;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Dungeon|World")
 void OnDungeonWorldReady(int32 ReadyCount);
 // Allows the atlas/quest system to locate a specific dungeon without
 // scanning every spawned actor or changing the existing save format.
 UFUNCTION(BlueprintPure, Category="Moonfall|Dungeon|World")
 bool GetDungeonEntranceTransform(int32 ZoneIndex, int32 DungeonIndex,
    FTransform& OutTransform) const;
 // Read-only HUD hint from the single cached ambient event. Never scans
 // all actors and never changes any player quest or world-save state.
 FString GetNearbyAmbientEventHint(const FVector& HeroLocation) const;
 // Lifecycle registration keeps late-spawned and designer-placed residents
 // participating in the living world without rescanning the entire map.
 void RegisterLivingResident(AMoonfallWorldNPC* Resident);
 void UnregisterLivingResident(AMoonfallWorldNPC* Resident);

private:
 FTimerHandle DungeonSpawnTimer;
 int32 NextDungeonSlot = 0;
 TArray<uint8> AuthoredDungeonSlots;
 void SpawnDungeonBatch();
 // One 4-second simulation pulse, no NPC Tick and no per-frame world scan.
 void PulseLivingWorld();
 FTimerHandle LivingWorldPulseTimer;
 bool bLivingResidentCacheInitialized = false;
 TWeakObjectPtr<AMoonfallArena> LivingArena;
 TArray<TWeakObjectPtr<AMoonfallWorldNPC>> LivingResidents;
 // Round-robin slices 20 nearby residents every four seconds. Registers
 // more than one settlement without unbounded per-pulse NPC updates.
 int32 NextResidentPulseIndex = 0;
 TWeakObjectPtr<AActor> ActiveAmbientEncounter;
 // World-pulse ownership prevents the sole active encounter despawning
 // during an open NPC conversation or an active nearby monster battle.
 float ActiveAmbientExpiresAt = -1.f;
 float NextAmbientEncounterAt = 120.f;
 int32 LastAmbientEventPhase = -1;
};
