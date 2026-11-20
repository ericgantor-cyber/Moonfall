#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallArena.generated.h"
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class AMoonfallEnemy;
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
 UPROPERTY(Transient) TArray<TObjectPtr<UTextRenderComponent>> DungeonZoneSigns;
 // Nonblocking starter directions share the same no-Tick text approach as zone signs.
 UPROPERTY(Transient) TArray<TObjectPtr<UTextRenderComponent>> SanctuaryGuideSigns;
 // Keeps the new-player sanctuary on the same continuous world map.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Starting Zone")
 bool bEnableStartingSanctuary = true;
 FVector GetStartingSanctuaryLocation() const;
 UPROPERTY(EditAnywhere, Category="Dungeon") int32 EnemyCount = 5;
 // NAME_None keeps the previous combined showcase. Select one god per arena
 // by setting this to z_void_reaper, nyra_eclipse_queen, or death_goddess_crimson_reaper in the editor.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Divine") FName FeaturedDivineBossId = NAME_None;
 // A walkable optional boss location in the shared six-zone world.
 // Kept independent of the older Z/Nyra arena showcase and its default ID.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Divine|Morvella")
 bool bSpawnMorvellaInWorld = true;
 // Import the approved Morvella art into a subclass Blueprint and assign here.
 // Until then, the normal enemy class is a visible combat proxy, not final art.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Divine|Morvella")
 TSubclassOf<AMoonfallEnemy> MorvellaWorldActorClass;
private:
 void CreateBlock(FVector Position, FVector Scale);
 void SpawnMorvellaWorldEncounter();
};
