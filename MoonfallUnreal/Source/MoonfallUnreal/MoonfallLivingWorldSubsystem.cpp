#include "MoonfallLivingWorldSubsystem.h"
#include "MoonfallLivingWorldPolicy.h"
#include "Engine/World.h"

bool UMoonfallLivingWorldSubsystem::CanWrite() const {return IsInGameThread()&&GetWorld()&&GetWorld()->IsGameWorld()&&GetWorld()->GetNetMode()!=NM_Client;}
double UMoonfallLivingWorldSubsystem::Now() const {return GetWorld()?GetWorld()->GetTimeSeconds():0.;}
bool UMoonfallLivingWorldSubsystem::GetNpcRelationship(FName Observer,FName Other,int32& Score) const {
 Score=0;if(const auto* List=Relationships.Find(Observer))for(const auto& R:*List)if(R.Other==Other){Score=R.Score;return true;}return false;
}
bool UMoonfallLivingWorldSubsystem::AdjustNpcRelationship(FName Observer,FName Other,int32 Delta){
 if(!CanWrite()||Observer.IsNone()||Other.IsNone()||Observer==Other||Delta<-100||Delta>100)return false;
 auto* List=Relationships.Find(Observer);if(!List){if(Relationships.Num()>=512)return false;List=&Relationships.Add(Observer);}
 for(auto& R:*List)if(R.Other==Other){R.Score=MoonfallWorldPolicy::Relationship(R.Score,Delta);return true;}
 if(List->Num()>=64)return false;List->Add(FRelationship{Other,Delta});return true;
}
bool UMoonfallLivingWorldSubsystem::GetRumor(FName Npc,FName RumorId,FMoonfallWorldRumor& Rumor) const {
 Rumor=FMoonfallWorldRumor{};const double Time=Now();if(const auto* List=Knowledge.Find(Npc))for(const auto& R:*List)if(R.RumorId==RumorId&&R.ExpiresAt>Time){Rumor=R;return true;}return false;
}
bool UMoonfallLivingWorldSubsystem::StoreRumor(FName Npc,const FMoonfallWorldRumor& Rumor){
 if(Npc.IsNone()||Rumor.ExpiresAt<=Now())return false;
 auto* List=Knowledge.Find(Npc);if(!List){if(Knowledge.Num()>=512)return false;List=&Knowledge.Add(Npc);}
 const double Time=Now();List->RemoveAll([Time](const auto& R){return R.ExpiresAt<=Time;});
 for(auto& R:*List)if(R.RumorId==Rumor.RumorId){if(R.bDirectWitness||(R.bWitnessReport&&!Rumor.bDirectWitness)||(R.Confidence>=Rumor.Confidence&&!Rumor.bWitnessReport&&!Rumor.bDirectWitness))return false;R=Rumor;return true;}
 // Never evict a direct witness record to make room for gossip.
 if(List->Num()>=32){int32 Worst=INDEX_NONE;for(int32 I=0;I<List->Num();++I){const auto& R=(*List)[I];if(!R.bDirectWitness&&!R.bWitnessReport&&(Worst==INDEX_NONE||R.Confidence<(*List)[Worst].Confidence))Worst=I;}if(Worst==INDEX_NONE||(!Rumor.bDirectWitness&&!Rumor.bWitnessReport&&(*List)[Worst].Confidence>=Rumor.Confidence))return false;List->RemoveAtSwap(Worst);}
 List->Add(Rumor);return true;
}
bool UMoonfallLivingWorldSubsystem::PublishObservation(FName Witness,FName RumorId,FVector Location,bool bCrime,float LifetimeSeconds){
 if(!CanWrite()||Witness.IsNone()||RumorId.IsNone()||Location.ContainsNaN()||!FMath::IsFinite(LifetimeSeconds)||LifetimeSeconds<=0)return false;
 const double Time=Now();for(auto It=Observations.CreateIterator();It;++It)if(It.Value().ExpiresAt<=Time)It.RemoveCurrent();
 auto* Existing=Observations.Find(RumorId);if(Existing&&(Existing->bCrime!=bCrime||!Existing->Location.Equals(Location,1.f)))return false;
 if(!Existing&&Observations.Num()>=1024)return false;
 FMoonfallWorldRumor R;R.RumorId=RumorId;R.Origin=Witness;R.LastSpeaker=Witness;R.Confidence=100;R.bDirectWitness=true;R.bCrime=bCrime;R.Location=Location;R.ExpiresAt=Existing?Existing->ExpiresAt:Time+FMath::Clamp(LifetimeSeconds,1.f,3600.f);
 if(!StoreRumor(Witness,R))return false;if(!Existing){FObservation O;O.Location=Location;O.bCrime=bCrime;O.ExpiresAt=R.ExpiresAt;Observations.Add(RumorId,O);}return true;
}
bool UMoonfallLivingWorldSubsystem::RelayRumor(FName Speaker,FName Listener,FName RumorId){
 if(!CanWrite()||Speaker==Listener||Listener.IsNone())return false;FMoonfallWorldRumor R;
 if(!GetRumor(Speaker,RumorId,R)||!MoonfallWorldPolicy::CanRelay(R.Confidence,R.Hops))return false;
 R.LastSpeaker=Speaker;R.bDirectWitness=false;R.bWitnessReport=false;++R.Hops;R.Confidence=MoonfallWorldPolicy::RelayConfidence(R.Confidence);return StoreRumor(Listener,R);
}
bool UMoonfallLivingWorldSubsystem::ShareUsefulRumor(FName Speaker,FName Listener){
 if(!CanWrite())return false;
 const FName BestId=FindUsefulRumor(Speaker,Listener);
 if(BestId.IsNone())return false;
 FMoonfallWorldRumor Previous;
 const bool bAlreadyKnown=GetRumor(Listener,BestId,Previous);
 if(!RelayRumor(Speaker,Listener,BestId))return false;
 FMoonfallWorldRumor Received;
 if(!bAlreadyKnown&&GetRumor(Listener,BestId,Received))
  CreditUsefulNews(Listener,Speaker,Received.Confidence);
 return true;
}
void UMoonfallLivingWorldSubsystem::CreditUsefulNews(FName Listener,FName Speaker,int32 Confidence){
 if(!CanWrite())return;
 const double Time=Now();
 if(!FMath::IsFinite(Time)||Time<0)return;
 if(auto* List=Relationships.Find(Listener)){
  for(auto& R:*List)if(R.Other==Speaker){
   const double Elapsed=R.LastAmbientTrustAt<0?60.0:Time-R.LastAmbientTrustAt;
   const int32 Delta=MoonfallWorldPolicy::AmbientTrustDelta(R.Score,Confidence,Elapsed);
   if(Delta>0){R.Score=MoonfallWorldPolicy::Relationship(R.Score,Delta);R.LastAmbientTrustAt=Time;}
   return;
  }
 }
 const int32 Delta=MoonfallWorldPolicy::AmbientTrustDelta(0,Confidence,60.0);
 if(Delta<=0||!AdjustNpcRelationship(Listener,Speaker,Delta))return;
 if(auto* List=Relationships.Find(Listener))for(auto& R:*List)if(R.Other==Speaker){R.LastAmbientTrustAt=Time;break;}
}
FName UMoonfallLivingWorldSubsystem::FindUsefulRumor(FName Speaker,FName Listener) const {
 if(Speaker.IsNone()||Listener.IsNone()||Speaker==Listener)return NAME_None;
 int32 Attitude=0;GetNpcRelationship(Speaker,Listener,Attitude);if(Attitude<=-20)return NAME_None;
 const auto* Source=Knowledge.Find(Speaker);if(!Source)return NAME_None;
 // Select at most one useful rumor from the existing 32-entry memory.
 // Do not mutate Knowledge while holding its source pointer.
 FName BestId=NAME_None;int32 BestConfidence=0;const double Time=Now();
 for(const auto& R:*Source){
  if(R.bCrime||R.ExpiresAt<=Time||!MoonfallWorldPolicy::CanRelay(R.Confidence,R.Hops))continue;
  const int32 Confidence=MoonfallWorldPolicy::RelayConfidence(R.Confidence);
  FMoonfallWorldRumor Known;
  if(GetRumor(Listener,R.RumorId,Known)&&(Known.bDirectWitness||Known.bWitnessReport||Known.Confidence>=Confidence))continue;
  if(Confidence>BestConfidence){BestConfidence=Confidence;BestId=R.RumorId;}
 }
 return BestId;
}
bool UMoonfallLivingWorldSubsystem::GetNearbyCombatReport(FName Npc,const FVector& Location,FMoonfallWorldRumor& Report) const {
 Report=FMoonfallWorldRumor{};
 if(Npc.IsNone()||Location.ContainsNaN())return false;
 const auto* List=Knowledge.Find(Npc);if(!List)return false;
 const double Time=Now();
 for(const auto& R:*List){
  if(R.bCrime||R.bDirectWitness||R.bWitnessReport||R.Hops<1||R.Confidence<40||R.ExpiresAt<=Time
      ||!R.RumorId.ToString().StartsWith(TEXT("NearbyCombat_"))||R.Location.ContainsNaN()
      ||FVector::DistSquared(Location,R.Location)>FMath::Square(1800.0))continue;
  if(R.Confidence>Report.Confidence||(R.Confidence==Report.Confidence&&R.ExpiresAt>Report.ExpiresAt))Report=R;
 }
 return !Report.RumorId.IsNone();
}
bool UMoonfallLivingWorldSubsystem::ReportWitnessToGuard(FName Witness,FName Guard,FName RumorId){
 FMoonfallWorldRumor R;if(!CanWrite()||Witness==Guard||Guard.IsNone()||!GetRumor(Witness,RumorId,R)||!R.bCrime||!R.bDirectWitness)return false;
 // Host verifies Guard's role and contact. This transfers knowledge, not a conviction or arrest order.
 FMoonfallWorldRumor Known;if(GetRumor(Guard,RumorId,Known)&&(Known.bDirectWitness||Known.bWitnessReport))return false;
 R.LastSpeaker=Witness;R.bDirectWitness=false;R.bWitnessReport=true;R.Hops=1;R.Confidence=90;return StoreRumor(Guard,R);
}
bool UMoonfallLivingWorldSubsystem::RegisterEncounter(FName Encounter,bool bRepeatable,float CooldownSeconds){
 if(!CanWrite()||Encounter.IsNone()||Encounters.Contains(Encounter)||Encounters.Num()>=1024||!FMath::IsFinite(CooldownSeconds)||CooldownSeconds<0)return false;
 FEncounter E;E.bRepeatable=bRepeatable;E.Cooldown=FMath::Clamp(CooldownSeconds,0.f,86400.f);Encounters.Add(Encounter,E);return true;
}
EMoonfallEncounterState UMoonfallLivingWorldSubsystem::GetEncounterState(FName Encounter) const {if(const auto* E=Encounters.Find(Encounter))return E->State;return EMoonfallEncounterState::Unknown;}
bool UMoonfallLivingWorldSubsystem::AdvanceEncounter(FName Encounter,EMoonfallEncounterState Next){
 if(!CanWrite())return false;auto* E=Encounters.Find(Encounter);if(!E||!MoonfallWorldPolicy::CanTransition(int(E->State),int(Next),E->bRepeatable,Now()-E->CompletedAt>=E->Cooldown))return false;
 E->State=Next;if(Next==EMoonfallEncounterState::Completed)E->CompletedAt=Now();return true;
}
void UMoonfallLivingWorldSubsystem::Deinitialize(){Relationships.Reset();Knowledge.Reset();Observations.Reset();Encounters.Reset();Super::Deinitialize();}
