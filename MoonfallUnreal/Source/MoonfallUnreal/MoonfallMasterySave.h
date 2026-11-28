#pragma once
#include "MoonfallPersistentSave.h"
#include "MoonfallSkillMastery.h"

namespace MoonfallMasterySave {
inline bool Read(const UMoonfallPersistentSave& Save,MoonfallSkillMastery::State& Out) {
 namespace M=MoonfallSkillMastery;
 if(Save.SkillMasteryVersion==0){
  if(!Save.SkillMasteryXP.IsEmpty()||!Save.SkillMasteryOrbWords.IsEmpty())return false;
  Out=M::State{};return true;
 }
 if(Save.SkillMasteryVersion!=M::RecordVersion||Save.SkillMasteryXP.Num()!=M::SkillCount
    ||Save.SkillMasteryOrbWords.Num()!=2*M::RegionCount)return false;
 std::array<int,M::SkillCount> XP{};
 std::array<std::uint64_t,M::RegionCount> Masks{};
 for(int32 I=0;I<M::SkillCount;++I)XP[I]=Save.SkillMasteryXP[I];
 for(int32 I=0;I<M::RegionCount;++I){
  const int64 Low=Save.SkillMasteryOrbWords[2*I],High=Save.SkillMasteryOrbWords[2*I+1];
  if(Low<0||High<0||Low>0xffffffffLL||High>0xffffffffLL)return false;
  Masks[I]=static_cast<std::uint64_t>(Low)|(static_cast<std::uint64_t>(High)<<32);
 }
 return M::TryRestoreRecord(Save.SkillMasteryVersion,XP,Masks,Out);
}
inline bool Write(const MoonfallSkillMastery::State& State,UMoonfallPersistentSave& Save) {
 namespace M=MoonfallSkillMastery;
 if(!M::Valid(State))return false;
 TArray<int32> XP;
 TArray<int64> Words;
 XP.Reserve(M::SkillCount);Words.Reserve(2*M::RegionCount);
 for(int Value:State.XP)XP.Add(Value);
 for(std::uint64_t Mask:State.Collected){
  Words.Add(static_cast<int64>(Mask&0xffffffffULL));
  Words.Add(static_cast<int64>(Mask>>32));
 }
 Save.SkillMasteryVersion=M::RecordVersion;
 Save.SkillMasteryXP=MoveTemp(XP);
 Save.SkillMasteryOrbWords=MoveTemp(Words);
 return true;
}
}
