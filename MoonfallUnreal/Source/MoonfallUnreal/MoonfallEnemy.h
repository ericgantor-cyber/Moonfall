#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallEnemy.generated.h"
class UStaticMeshComponent;
UCLASS()
class MOONFALLUNREAL_API AMoonfallEnemy : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallEnemy();
 UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat") float Health = 80.f;
 UFUNCTION(BlueprintCallable, Category="Combat") void TakeSpellDamage(float Amount);
};
