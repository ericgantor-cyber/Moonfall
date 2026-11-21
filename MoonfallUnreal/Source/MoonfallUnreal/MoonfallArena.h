#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallArena.generated.h"
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class AMoonfallWorldNPC;
UCLASS()
class MOONFALLUNREAL_API AMoonfallArena : public AActor {
 GENERATED_BODY()
public:
 AMoonfallArena();
 virtual void BeginPlay() override;
 // The public 30-entrance road uses one instanced mesh instead of
 // thirty individually ticking mesh actors.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Dungeon|World")
 UInstancedStaticMeshComponent* DungeonRoad;
 // Decorative, collision-free settlement sets, switched by the saved Council policy.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|World|Council")
 UInstancedStaticMeshComponent* CouncilProtectionDecor;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|World|Council")
 UInstancedStaticMeshComponent* CouncilReconstructionDecor;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|World|Council")
 UInstancedStaticMeshComponent* CouncilFortificationDecor;
 UPROPERTY(Transient) TObjectPtr<UTextRenderComponent> CouncilWorldSign;
 UPROPERTY(Transient) TArray<TObjectPtr<UTextRenderComponent>> DungeonZoneSigns;
 // Nonblocking starter directions share the same no-Tick text approach as zone signs.
 UPROPERTY(Transient) TArray<TObjectPtr<UTextRenderComponent>> SanctuaryGuideSigns;
 // Keeps the new-player sanctuary on the same continuous world map.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Starting Zone")
 bool bEnableStartingSanctuary = true;
 FVector GetStartingSanctuaryLocation() const;
 UPROPERTY(EditAnywhere, Category="Dungeon") int32 EnemyCount = 5;
 // NAME_None keeps the previous combined showcase. Select one god per arena
 // by setting this to z_void_reaper or nyra_eclipse_queen in the editor.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Divine") FName FeaturedDivineBossId = NAME_None;
private:
 void CreateBlock(FVector Position, FVector Scale);
 void RefreshCouncilSettlementVisuals();
 int32 LastDisplayedCouncilDecision = -2;
 // Max four authored movers; no world actor scans on each movement step.
 TArray<TWeakObjectPtr<AMoonfallWorldNPC>> CouncilMobileResidents;
 // Only authored merchants and tinkers, no world scan per refresh.
 TArray<TWeakObjectPtr<AMoonfallWorldNPC>> CouncilStationResidents;
 void StepCouncilWorkRoutes();
};
