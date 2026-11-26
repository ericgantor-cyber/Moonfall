// Native UE serialization and resident-contract eligibility; no disk save or world mutation.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "MoonfallHeroPawn.h"
#include "MoonfallPersistentSave.h"
#include "MoonfallQuestJournal.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMoonfallQuestSaveContractTest,
 "Moonfall.Quests.ResidentSaveRoundTrip",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoonfallQuestSaveContractTest::RunTest(const FString& Parameters)
{
 namespace J=MoonfallQuestJournal;
 const AMoonfallHeroPawn* Hero=GetDefault<AMoonfallHeroPawn>();
 if(!TestNotNull(TEXT("Hero defaults"),Hero)) return false;
 UMoonfallPersistentSave* Original=Cast<UMoonfallPersistentSave>(
   UGameplayStatics::CreateSaveGameObject(UMoonfallPersistentSave::StaticClass()));
 if(!TestNotNull(TEXT("Quest save object"),Original)) return false;
 TestEqual(TEXT("Fresh quest ledger is empty"),Original->ResidentQuestStates.Num(),0);
 TestFalse(TEXT("Unknown resident cannot offer a contract"),
   Hero->SupportsResidentContract(FName(TEXT("MoonfallUnknownTestResident"))));
 TestFalse(TEXT("Empty resident ID cannot offer a contract"),Hero->SupportsResidentContract(NAME_None));
 for(int I=0;I<J::ResidentCount;++I) {
  const FName Id(UTF8_TO_TCHAR(J::Residents[I].Id));
  TestTrue(TEXT("Authored resident is supported by native hero"),Hero->SupportsResidentContract(Id));
  TestFalse(TEXT("No case-insensitive duplicate resident ID"),Original->ResidentQuestStates.Contains(Id));
  Original->ResidentQuestStates.Add(Id,1+(I%2));
 }
 TestEqual(TEXT("Every catalog resident has a distinct save record"),
   Original->ResidentQuestStates.Num(),J::ResidentCount);
 TArray<uint8> Bytes;
 if(!TestTrue(TEXT("Serialize resident contracts to memory"),
    UGameplayStatics::SaveGameToMemory(Original,Bytes))) return false;
 UMoonfallPersistentSave* Restored=Cast<UMoonfallPersistentSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
 if(!TestNotNull(TEXT("Restore resident contracts from memory"),Restored)) return false;
 TestEqual(TEXT("Save schema unchanged"),Restored->Version,Original->Version);
 TestEqual(TEXT("All resident states survive"),Restored->ResidentQuestStates.Num(),J::ResidentCount);
 for(int I=0;I<J::ResidentCount;++I) {
  const FName Id(UTF8_TO_TCHAR(J::Residents[I].Id));
  const int32* State=Restored->ResidentQuestStates.Find(Id);
  if(TestNotNull(TEXT("Resident state restored"),State))
   TestEqual(TEXT("Accepted/completed status preserved"),*State,1+(I%2));
 }
 J::FMask Active{},Tracked{};
 for(int I=0;I<J::MaxActive;++I) Active.Add(I);
 for(int I=0;I<J::MaxTracked;++I) Tracked=J::Toggle(Tracked,Active,I);
 // Verify the world HUD's regional count in the real Editor automation suite.
 int Seen[MoonfallWorldQuest::ZoneCount]{}, Defeated[MoonfallWorldQuest::ZoneCount]{};
 TestEqual(TEXT("Fresh regional quest count"),MoonfallWorldQuest::ActiveCount(Seen,Defeated),6);
 Seen[0]=1;
 TestEqual(TEXT("First discovery unlocks regional boss quest"),MoonfallWorldQuest::ActiveCount(Seen,Defeated),7);
 Seen[0]=5;
 TestEqual(TEXT("Discovery completion removes its quest"),MoonfallWorldQuest::ActiveCount(Seen,Defeated),6);
 Defeated[0]=5;
 TestEqual(TEXT("Boss completion removes its quest"),MoonfallWorldQuest::ActiveCount(Seen,Defeated),5);
 TestEqual(TEXT("100 active capacity"),J::Count(Active),100);
 TestEqual(TEXT("Seven tracked capacity"),J::Count(Tracked),7);
 TestTrue(TEXT("Eighth quest does not displace tracked quests"),J::Toggle(Tracked,Active,7)==Tracked);
 Tracked=J::Toggle(Tracked,Active,0);
 Tracked=J::Toggle(Tracked,Active,99);
 TestTrue(TEXT("Last active slot can be tracked"),Tracked.Has(99));
 Active.Remove(99);
 const auto Reconciled=Tracked&Active;
 TestFalse(TEXT("Completed quest no longer tracked"),Reconciled.Has(99));
 TestTrue(TEXT("Unrelated active quest remains tracked"),Reconciled.Has(1));
 TestEqual(TEXT("Only completed quest leaves tracking"),J::Count(Reconciled),6);
 TestTrue(TEXT("99 active contracts permits acceptance"),J::CanAccept(99));
 TestFalse(TEXT("100 active contracts blocks acceptance"),J::CanAccept(100));
 TestEqual(TEXT("Final partial page remains reachable"),J::SelectPage(98,3,100,1),99);
 TestEqual(TEXT("Next on final page retains selection"),J::SelectPage(99,3,100,1),99);
 return !HasAnyErrors();
}
#endif
