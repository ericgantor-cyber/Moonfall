#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MoonfallUnrealGameMode.generated.h"
UCLASS()
class MOONFALLUNREAL_API AMoonfallUnrealGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 AMoonfallUnrealGameMode();
 virtual void BeginPlay() override;
};
