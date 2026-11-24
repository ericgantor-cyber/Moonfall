#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MoonfallNpcSocialSessionSubsystem.generated.h"

class UMoonfallLivingWorldSubsystem;

// Session ledger; only authoritative standalone game worlds enable disk IO.
UCLASS()
class MOONFALLUNREAL_API UMoonfallNpcSocialSessionSubsystem : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 virtual void Deinitialize() override;
private:
 friend class UMoonfallLivingWorldSubsystem;
 struct FRelationship { FName Other; int32 Score=0; double LastAmbientTrustAt=-1.0; };
 TMap<FName,TArray<FRelationship>> Relationships;
 bool bPersistenceInitialized=false;
 bool bPersistenceEnabled=false;
 bool bWriteBlocked=false;
 bool bDirty=false;
 int32 ActiveSlot=INDEX_NONE;
 int64 Generation=0;
 double LastSaveAttempt=-1.0;
 void InitializePersistence(bool bAllowDisk);
 bool Flush(bool bForce=false);
 bool ResetLedger();
};
