#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallEquipmentSlots.h"
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallGearAdvisorPolicy.h"
#include <climits>
namespace S=MoonfallEquipmentSlots;
constexpr bool AllSlots() {
 S::FLoadout Worn{};
 int Bag[S::SlotCount]{};
 for(int Slot=0;Slot<S::SlotCount;++Slot) {
  if(!S::Known(Slot)||S::Legacy(Slot)!=(Slot<3))return false;
  for(int A=0;A<=3;++A)for(int R=0;R<=5;++R)for(int T=1;T<=25;++T) {
   const int Code=A*100000+Slot*10000+R*1000+T;
   const bool Expected=A==0||R!=0;
   if(S::Valid(Code,Slot)!=Expected)return false;
   if(MoonfallGearAdvisor::Valid(Code,Slot)!=(Expected&&Slot<3))return false;
   if(S::Valid(Code,(Slot+1)%S::SlotCount))return false;
  }
  Bag[Slot]=Worn[Slot]=1000+Slot*10000+1;
 }
 if(!S::ValidateOwned(Worn,Bag,S::SlotCount))return false;
 if(S::ValidateOwned(Worn,Bag,S::SlotCount-1))return false;
 if(S::ValidateOwned(Worn,nullptr,S::SlotCount))return false;
 if(S::ValidateOwned(Worn,Bag,10001))return false;
 Worn[8]=Bag[7];
 return !S::ValidateOwned(Worn,Bag,S::SlotCount);
}
static_assert(AllSlots());
constexpr S::FLegacy Old{1001,11002,21003};
constexpr auto Migrated=S::FromLegacy(Old);
static_assert(Migrated[0]==1001&&Migrated[1]==11002&&Migrated[2]==21003);
static_assert(Migrated[3]==0&&Migrated[8]==0);
static_assert(S::ToLegacy(Migrated)[2]==21003);
static_assert(!S::Valid(INT_MAX,8)&&!S::Valid(INT_MIN,0));
static_assert(!S::Valid(91001,9)&&!S::Valid(0,0)&&!S::Valid(1000,0));
static_assert(!S::Valid(6026,0)&&!S::Valid(100001,0));
static_assert(S::ValidateOwned(S::FLoadout{},nullptr,0));
int main(){return AllSlots()?0:1;}
