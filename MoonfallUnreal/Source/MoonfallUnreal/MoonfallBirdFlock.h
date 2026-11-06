#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MoonfallBirdFlock.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;

// One lightweight ambient flock actor, not individual bird AI actors.
UCLASS()
class MOONFALLUNREAL_API AMoonfallBirdFlock : public AActor
{
 GENERATED_BODY()
public:
 AMoonfallBirdFlock();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Wildlife", meta=(ClampMin="0", ClampMax="16"))
 int32 BirdCount = 10;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Wildlife", meta=(ClampMin="200.0", ClampMax="2000.0"))
 float FlockRadius = 720.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Moonfall|Wildlife", meta=(ClampMin="300.0", ClampMax="2200.0"))
 float FlyingHeight = 920.f;
private:
 UPROPERTY(VisibleAnywhere) USceneComponent* SceneRoot;
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* Bodies;
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* LeftWings;
 UPROPERTY(VisibleAnywhere) UInstancedStaticMeshComponent* RightWings;
 void UpdateFlock(float TimeSeconds);
};
