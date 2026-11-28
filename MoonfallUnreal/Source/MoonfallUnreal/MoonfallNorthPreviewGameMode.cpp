#include "MoonfallNorthPreviewGameMode.h"
#include "Engine/World.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallCombatHUD.h"
#include "MoonfallProfessionSave.h"
#include "MoonfallProfessionLedger.h"
#include "MoonfallGrottoTreasure.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"

AMoonfallNorthPreviewGameMode::AMoonfallNorthPreviewGameMode() {
 DefaultPawnClass=AMoonfallHeroPawn::StaticClass();
 HUDClass=AMoonfallCombatHUD::StaticClass();
 // No spawning in BeginPlay: preview geography only.
}

int32 AMoonfallNorthPreviewGameMode::GetOceanActivity() const {
 const float Now=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
 if (!FMath::IsFinite(Now) || Now<0.f) return 0;
 return MoonfallOceanEvents::Activity(FMath::FloorToInt(FMath::Fmod(Now,180.f)));
}

namespace {
FString ProfessionSlot(int32 Index) { return FString::Printf(TEXT("MoonfallDeepWatersProfessions_%d"),Index); }
}
void AMoonfallNorthPreviewGameMode::StartPlay() {
 bProfessionSaveEnabled=HasAuthority()&&GetWorld()&&GetWorld()->WorldType==EWorldType::Game&&
  UGameplayStatics::GetCurrentLevelName(this,true)==TEXT("DeepWatersPreview");
 if(bProfessionSaveEnabled) LoadProfessions();
 Super::StartPlay();
 if(!bProfessionSaveEnabled) return;
 TArray<AMoonfallGrottoTreasure*> Claimed;
 for(TActorIterator<AMoonfallGrottoTreasure> It(GetWorld());It;++It)
  if(IsValid(*It)&&It->ResourceIndex==-1&&HasGrottoClaim(It->GrottoIndex)) Claimed.Add(*It);
 for(auto* Chest:Claimed) if(IsValid(Chest)) Chest->Destroy();
 GetWorldTimerManager().SetTimer(ProfessionSaveTimer,this,&AMoonfallNorthPreviewGameMode::FlushProfessions,2.f,true);
}
void AMoonfallNorthPreviewGameMode::EndPlay(const EEndPlayReason::Type Reason) {
 if(GetWorld()) GetWorldTimerManager().ClearTimer(ProfessionSaveTimer);
 FlushProfessions();
 Super::EndPlay(Reason);
}
void AMoonfallNorthPreviewGameMode::LoadProfessions() {
 using namespace MoonfallProfessionLedger;
 FSnapshot Best;
 bool bAny=false,bValid=false;
 for(int32 I=0;I<2;++I) {
  const FString Slot=ProfessionSlot(I);
  if(!UGameplayStatics::DoesSaveGameExist(Slot,0)) continue;
  bAny=true;
  const auto* Save=Cast<UMoonfallProfessionSave>(UGameplayStatics::LoadGameFromSlot(Slot,0));
  if(IsValid(Save)&&Save->Version>3) { bProfessionSaveBlocked=true; return; }
  FSnapshot Candidate;
  if(!IsValid(Save)||(Save->Version!=1&&Save->Version!=2&&Save->Version!=3)||Save->Ledger.Num()!=(Save->Version==1?LegacyCount:Save->Version==2?RegionalCount:Count)||Save->Sequence<=0||Save->Sequence%2!=I||
    !Candidate.Decode(Save->Ledger.GetData(),Save->Ledger.Num())) continue;
  if(!bValid||Save->Sequence>ProfessionSequence) { Best=Candidate; ProfessionSequence=Save->Sequence; bValid=true; }
 }
 if(!bValid) { bProfessionSaveBlocked=bAny; return; }
 Gathering=Best.Gathering; Workshop=Best.Workshop; GrottoGear=Best.Gear; IslandJournal=Best.Journal;
 Regions=Best.Regions; Sightings=Best.Sightings; WreckClaims=uint8(Best.Wrecks); CaveClaims=uint8(Best.Caves);
 bProfessionDirty=false;
}
void AMoonfallNorthPreviewGameMode::FlushProfessions() {
 using namespace MoonfallProfessionLedger;
 if(!bProfessionSaveEnabled||bProfessionSaveBlocked||!bProfessionDirty||ProfessionSequence==MAX_int64) return;
 FSnapshot State;
 State.Gathering=Gathering; State.Workshop=Workshop; State.Gear=GrottoGear; State.Journal=IslandJournal;
 State.Regions=Regions; State.Sightings=Sightings; State.Wrecks=WreckClaims; State.Caves=CaveClaims;
 int Values[Count]={}; State.Encode(Values);
 FSnapshot Validation;
 if(!Validation.Decode(Values,Count)) { bProfessionSaveBlocked=true; return; }
 auto* Save=Cast<UMoonfallProfessionSave>(UGameplayStatics::CreateSaveGameObject(UMoonfallProfessionSave::StaticClass()));
 if(!IsValid(Save)) return;
 Save->Sequence=ProfessionSequence+1;
 Save->Ledger.Append(Values,Count);
 if(UGameplayStatics::SaveGameToSlot(Save,ProfessionSlot(int32(Save->Sequence%2)),0)) {
  ProfessionSequence=Save->Sequence; bProfessionDirty=false;
 }
}
