#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallWorldNPC.generated.h"
class UStaticMeshComponent;
class AMoonfallHeroPawn;

// Lightweight one-world NPC: idle without Tick; usable from player or UI actions.
UCLASS(Blueprintable)
class MOONFALLUNREAL_API AMoonfallWorldNPC : public AActor {
 GENERATED_BODY()
public:
 AMoonfallWorldNPC();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") UStaticMeshComponent* Body;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FText DisplayName;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FText Greeting;
 // Assigned by settlement/world spawner. Any ancestry may be friendly.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC|Race", meta=(ClampMin="0", ClampMax="11")) int32 RaceId = 0;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Race") FString GetRaceName() const;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FName SettlementId = NAME_None;
 // Stable authored ID, not GetName() or transient spawn index.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC|Bond") FName ResidentId = NAME_None;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Bond") int32 GetBondWithHero(const AMoonfallHeroPawn* Hero) const;
 // Quest/dialogue event may call this; conversations alone never award bond points.
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC|Bond") bool RewardResidentTrust(AMoonfallHeroPawn* Hero, int32 Points);

 // Server-side simple social role; branching dialogue may be authored in Blueprint.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FName SocialRole = FName(TEXT("Citizen"));
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") FString GetSocialDisposition(const AMoonfallHeroPawn* Hero) const;
 // Ancestry changes social dialogue but never overwrites earned reputation.
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC|Race") bool SharesRaceWith(const AMoonfallHeroPawn* Hero) const;
 UFUNCTION(BlueprintPure, Category="Moonfall|NPC") FString GetCurrentActivity() const;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC") FName FactionId = FName(TEXT("Wanderers"));
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|NPC", meta=(ClampMin="50.0", ClampMax="400.0")) float InteractRange = 240.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|NPC") int32 InteractionCount = 0;
 UFUNCTION(BlueprintCallable, Category="Moonfall|NPC") bool TryTalk(AMoonfallHeroPawn* Hero);
 UFUNCTION(BlueprintImplementableEvent, Category="Moonfall|NPC") void OnDialogueStarted(AMoonfallHeroPawn* Hero, FText SpokenGreeting);
};
