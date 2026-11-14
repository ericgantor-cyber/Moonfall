#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "MoonfallCheckpointShrine.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPrimitiveComponent;
class AMoonfallHeroPawn;

// Discover once by approaching; the most recently discovered shrine becomes
// the persistent respawn point. The visual uses only engine primitive meshes.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallCheckpointShrine : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallCheckpointShrine();
 virtual void BeginPlay() override;
 // Update the status label when a shrine is discovered. No per-frame Tick.
 void RefreshShrineDisplay(const AMoonfallHeroPawn* Hero);

 // Set a unique authored ID per shrine. Never derive save identity from actor names.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Shrine") FName ShrineId = NAME_None;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Shrine") USphereComponent* DiscoveryRange = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Shrine") UStaticMeshComponent* ShrineBase = nullptr;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Shrine") UStaticMeshComponent* ShrineBeacon = nullptr;

 // Raise the character center above the plinth/floor. TeleportTo performs
 // collision checks before the saved point is used.
 FVector GetRespawnLocation() const { return GetActorLocation() + FVector(0.f, 0.f, 130.f); }

private:
 UPROPERTY(VisibleAnywhere) USceneComponent* SceneRoot = nullptr;
 UPROPERTY(VisibleAnywhere) UTextRenderComponent* ShrineLabel = nullptr;

 UFUNCTION()
 void OnDiscoveryOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                         bool bFromSweep, const FHitResult& SweepResult);
 UFUNCTION()
 void OnDiscoveryEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
 void TryDiscoverOrQueueRetry(AMoonfallHeroPawn* Hero);
 void RetryPendingDiscovery();
 TWeakObjectPtr<AMoonfallHeroPawn> PendingDiscoveryHero;
 FTimerHandle DiscoveryRetryTimer;
 int32 DiscoveryRetryAttempts = 0;
};
