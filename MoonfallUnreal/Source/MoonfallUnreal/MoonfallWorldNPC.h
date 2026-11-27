#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallWorldNPC.generated.h"
class UStaticMeshComponent;
class AMoonfallHeroPawn;

UENUM(BlueprintType)
enum class EMoonfallNpcMood : uint8 { Calm, Uneasy, Afraid, Vigilant, Wary, Resentful, Warm, Loyal };

// Lightweight one-world NPC: idle without Tick; usable from player or UI actions.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallWorldNPC : public AActor {
 GENERATED_BODY()
public:
 AMoonfallWorldNPC();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 // One world-level timer (not per-NPC Tick) drives small local routines.
 void AdvanceLivingRoutine(float Now, const FVector& HeroLocation,
                          bool bInConversation=false, int32 HeroBond=0, bool bAllowLocalGuide=false, bool bHoldCompanion=false);
 // Updated only by the bounded world pulse; an intent is not an animation or combat command.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC|Ambient")
 FString LivingIntent;
 // Indirect reports are not firsthand danger and cannot trigger arrest or combat.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC|Ambient")
 bool bReviewingCombatReport=false;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC|Ambient")
 bool bUsePersonalityRoutines=true;
 // Trusted scouts may offer short-range voluntary assistance; no automatic party recruitment.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC|Ambient")
 bool bAllowLocalScoutGuidance=true;
 // Public regional concern is not a personal witness report.
 int32 GetLocalRegionalDanger() const;
 void WitnessNearbyDanger(float Now);
 // Only the currently recruited or returning follower is advanced by the director.
 bool AdvanceFollowerToward(const FVector& Goal,float Now,float StopDistance);
 bool TrySafeFollowerCatchup(const AMoonfallHeroPawn* Hero);
 FVector FollowerWaypoint=FVector::ZeroVector;
 FVector FollowerPathGoal=FVector::ZeroVector;
 float NextFollowerPathAt=-1.f;
 bool bFollowerWaypointValid=false;
 int32 FollowerBlockedSteps=0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Companion")
 float CompanionHealth=100.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Companion")
 bool bCompanionDowned=false;
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Companion")
 void OnCompanionDowned();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|Companion")
 void OnCompanionRecovered();
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC|Ambient")
 bool bAmbientVisitor = false;
 float LastDangerAt = -1000.f;
 FVector AuthoredHomeLocation = FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") UStaticMeshComponent* Body;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FText DisplayName;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FText Greeting;
 // Native Canvas HUD can show spoken lines even without a Blueprint widget.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") FText LastSpokenLine;
 // Assigned by settlement/world spawner. Any ancestry may be friendly.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC|Race", meta=(ClampMin="0", ClampMax="11")) int32 RaceId = 0;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Race") FString GetRaceName() const;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FName SettlementId = NAME_None;
 // Stable authored ID, not GetName() or transient spawn index.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC|Bond") FName ResidentId = NAME_None;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Bond") int32 GetBondWithHero(const AMoonfallHeroPawn* Hero) const;
 // Quest/dialogue event may call this; conversations alone never award bond points.
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Bond") bool RewardResidentTrust(AMoonfallHeroPawn* Hero, int32 Points);
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Quest") int32 GetContractSalvageCost() const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Quest") bool CanHeroInteract(const AMoonfallHeroPawn* Hero) const;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Quest") bool OfferResidentQuest(AMoonfallHeroPawn* Hero);
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Quest") bool TurnInResidentQuest(AMoonfallHeroPawn* Hero);

 // Server-side simple social role; branching dialogue may be authored in Blueprint.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FName SocialRole = FName(TEXT("Citizen"));
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") FString GetSocialDisposition(const AMoonfallHeroPawn* Hero) const;
 // On-demand mood for animation/dialogue presentation, not an attack command.
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Ambient") EMoonfallNpcMood GetEmotionalState(const AMoonfallHeroPawn* Hero) const;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC|Ambient")
 EMoonfallNpcMood LastConversationMood=EMoonfallNpcMood::Calm;
 // Ancestry changes social dialogue but never overwrites earned reputation.
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Race") bool SharesRaceWith(const AMoonfallHeroPawn* Hero) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") FString GetCurrentActivity() const;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FName FactionId = FName(TEXT("Wanderers"));
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC", meta=(ClampMin="50.0", ClampMax="400.0")) float InteractRange = 240.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 InteractionCount = 0;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") bool TryTalk(AMoonfallHeroPawn* Hero);
 // Optional presentation hook for successful NPC-to-NPC exchanges.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|NPC|Ambient")
 void OnAmbientNewsShared(FName ListenerId,FText ListenerName);
 // Optional presentation event after a successful nonlethal assist.
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|NPC|Ambient")
 void OnCompanionSupportStrike();
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|NPC") void OnDialogueStarted(AMoonfallHeroPawn* Hero, FText SpokenGreeting);
};
