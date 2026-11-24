#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MoonfallNpcSocialSessionSubsystem.generated.h"

class UMoonfallLivingWorldSubsystem;

// Session-only, not a disk save. Holds no actor or world references.
// Separate game instances (including PIE sessions) never share a ledger.
UCLASS()
class MOONFALLUNREAL_API UMoonfallNpcSocialSessionSubsystem : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 virtual void Deinitialize() override;
private:
 friend class UMoonfallLivingWorldSubsystem;
 struct FRelationship { FName Other; int32 Score=0; double LastAmbientTrustAt=-1.0; };
 TMap<FName,TArray<FRelationship>> Relationships;
};
