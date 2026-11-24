#include "MoonfallNpcSocialSessionSubsystem.h"
#include "MoonfallNpcSocialSave.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformTime.h"
#include "Math/NumericLimits.h"

namespace {
const TCHAR* const SocialSlots[2]={TEXT("MoonfallNpcSocial_A"),TEXT("MoonfallNpcSocial_B")};
constexpr int32 MaxObservers=512;
constexpr int32 MaxPerObserver=64;
}

void UMoonfallNpcSocialSessionSubsystem::InitializePersistence(bool bAllowDisk){
 if(bPersistenceInitialized)return;
 bPersistenceInitialized=true;
 bPersistenceEnabled=bAllowDisk;
 if(!bPersistenceEnabled)return;
 bool bAnyExisting=false;
 bool bFoundValid=false;
 for(int32 Slot=0;Slot<2;++Slot){
  if(!UGameplayStatics::DoesSaveGameExist(SocialSlots[Slot],0))continue;
  bAnyExisting=true;
  const auto* Save=Cast<UMoonfallNpcSocialSave>(UGameplayStatics::LoadGameFromSlot(SocialSlots[Slot],0));
  if(!Save)continue;
  if(Save->Version!=1){bWriteBlocked=true;continue;}
  if(Save->Generation<=0||Save->Entries.Num()>MaxObservers*MaxPerObserver)continue;
  decltype(Relationships) Candidate;
  bool bValid=true;
  for(const auto& Entry:Save->Entries){
   if(Entry.Observer.IsNone()||Entry.Other.IsNone()||Entry.Observer==Entry.Other
       ||Entry.Score<-100||Entry.Score>100){bValid=false;break;}
   auto* List=Candidate.Find(Entry.Observer);
   if(!List){if(Candidate.Num()>=MaxObservers){bValid=false;break;}List=&Candidate.Add(Entry.Observer);}
   if(List->Num()>=MaxPerObserver||List->ContainsByPredicate([&](const FRelationship& R){return R.Other==Entry.Other;})){
    bValid=false;break;
   }
   // Use a new monotonic timestamp on load; restarting cannot bypass the cooldown.
   List->Add(FRelationship{Entry.Other,Entry.Score,FPlatformTime::Seconds()});
  }
  if(bValid&&(!bFoundValid||Save->Generation>Generation)){
   Relationships=MoveTemp(Candidate);Generation=Save->Generation;ActiveSlot=Slot;bFoundValid=true;
  }
 }
 // A newer schema or wholly unreadable save must never be overwritten.
 if(bAnyExisting&&!bFoundValid)bWriteBlocked=true;
}

bool UMoonfallNpcSocialSessionSubsystem::Flush(bool bForce){
 if(!bPersistenceEnabled)return true;
 if(bWriteBlocked)return false;
 if(!bDirty)return true;
 const double Time=FPlatformTime::Seconds();
 if(!bForce&&LastSaveAttempt>=0&&Time-LastSaveAttempt<60.0)return false;
 LastSaveAttempt=Time;
 if(Generation>=TNumericLimits<int64>::Max())return false;
 auto* Save=NewObject<UMoonfallNpcSocialSave>();
 Save->Generation=Generation+1;
 for(const auto& Pair:Relationships)for(const auto& R:Pair.Value){
  FMoonfallSavedNpcRelationship Entry;
  Entry.Observer=Pair.Key;Entry.Other=R.Other;Entry.Score=R.Score;Save->Entries.Add(Entry);
 }
 const int32 NextSlot=ActiveSlot==0?1:0;
 if(!UGameplayStatics::SaveGameToSlot(Save,SocialSlots[NextSlot],0))return false;
 ActiveSlot=NextSlot;Generation=Save->Generation;bDirty=false;return true;
}

bool UMoonfallNpcSocialSessionSubsystem::ResetLedger(){
 if(bWriteBlocked)return false;
 auto Previous=MoveTemp(Relationships);
 const bool bWasDirty=bDirty;
 Relationships.Reset();bDirty=true;
 if(Flush(true))return true;
 Relationships=MoveTemp(Previous);bDirty=bWasDirty;return false;
}

void UMoonfallNpcSocialSessionSubsystem::Deinitialize(){
 Flush(true);
 Relationships.Reset();
 Super::Deinitialize();
}
