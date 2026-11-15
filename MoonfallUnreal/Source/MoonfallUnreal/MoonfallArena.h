#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallArena.generated.h"
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
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
 UPROPERTY(EditAnywhere, Category="Dungeon") int32 EnemyCount = 5;
 // NAME_None keeps the previous combined showcase. Select one god per arena
 // by setting this to z_void_reaper or nyra_eclipse_queen in the editor.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon|Divine") FName FeaturedDivineBossId = NAME_None;
private:
 void CreateBlock(FVector Position, FVector Scale);
};
