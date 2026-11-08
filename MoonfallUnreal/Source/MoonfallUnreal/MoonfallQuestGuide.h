#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallQuestGuide.generated.h"

class UStaticMeshComponent;

// Stationary, non-hostile landmark. Existing Unreal gameplay manages quest state.
UCLASS()
class MOONFALLUNREAL_API AMoonfallQuestGuide : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallQuestGuide();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quests")
 int32 RegionIndex = 0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Moonfall|Quests")
 UStaticMeshComponent* Body = nullptr;
};
