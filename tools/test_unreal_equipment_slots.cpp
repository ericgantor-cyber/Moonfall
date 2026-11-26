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
constexpr bool Migration() {
 const int Bag[9]={1001,11002,21003,31004,41005,51006,61007,71008,81009};
 const S::FExtra Extra{31004,41005,51006,61007,71008,81009};
 S::FLoadout Worn{};
 for(int V=1;V<17;++V) {
  if(!S::ReadSavedLoadout(V,Old,nullptr,0,Bag,3,Worn))return false;
  if(S::ReadSavedLoadout(V,Old,nullptr,0,Bag,9,Worn))return false;
  for(int I=0;I<9;++I)if(Worn[I]!=Migrated[I])return false;
  if(S::ReadSavedLoadout(V,Old,Extra.data(),Extra.size(),Bag,9,Worn))return false;
 }
 if(!S::ReadSavedLoadout(17,Old,Extra.data(),Extra.size(),Bag,9,Worn))return false;
 for(int I=0;I<9;++I)if(Worn[I]!=Bag[I])return false;
 const auto Before=Worn;
 const int CorruptBag[10]={1001,11002,21003,31004,41005,51006,61007,71008,81009,100001};
 if(S::ReadSavedLoadout(17,Old,Extra.data(),6,CorruptBag,10,Worn))return false;
 const int InvalidLegacyBag[4]={1001,11002,21003,0};
 if(S::ReadSavedLoadout(16,Old,nullptr,0,InvalidLegacyBag,4,Worn))return false;
 if(S::ReadSavedLoadout(17,Old,Extra.data(),6,Bag,10001,Worn))return false;
 if(S::ReadSavedLoadout(17,Old,Extra.data(),6,nullptr,9,Worn))return false;
 if(S::ReadSavedLoadout(17,Old,Extra.data(),5,Bag,9,Worn))return false;
 if(S::ReadSavedLoadout(17,Old,nullptr,6,Bag,9,Worn))return false;
 if(S::ReadSavedLoadout(17,Old,Extra.data(),6,Bag,8,Worn))return false;
 if(S::ReadSavedLoadout(18,Old,Extra.data(),6,Bag,9,Worn))return false;
 if(S::ReadSavedLoadout(0,Old,nullptr,0,Bag,9,Worn))return false;
 auto Wrong=Extra; Wrong[0]=Extra[1];
 if(S::ReadSavedLoadout(17,Old,Wrong.data(),6,Bag,9,Worn))return false;
 for(int I=0;I<9;++I)if(Worn[I]!=Before[I])return false;
 const auto RoundTrip=S::ToExtra(Worn);
 for(int I=0;I<6;++I)if(RoundTrip[I]!=Extra[I])return false;
 return true;
}
static_assert(Migration());
constexpr bool Removal() {
 S::FLoadout Worn{};
 int Bag[10]{};
 for(int Slot=0;Slot<S::SlotCount;++Slot)
  Bag[Slot]=Worn[Slot]=Slot*10000+1001;
 for(int Slot=0;Slot<S::SlotCount;++Slot) {
  const int Code=Bag[Slot];
  if(S::CanRemoveCopy(Worn,Code,Bag,9))return false;
  Bag[9]=Code;
  if(!S::CanRemoveCopy(Worn,Code,Bag,10))return false;
  auto Unequipped=Worn;
  Unequipped[Slot]=0;
  if(!S::CanRemoveCopy(Unequipped,Code,Bag,9))return false;
 }
 return !S::CanRemoveCopy(Worn,1002,Bag,10)
     && !S::CanRemoveCopy(Worn,0,Bag,10)
     && !S::CanRemoveCopy(Worn,100001,Bag,10)
     && !S::CanRemoveCopy(Worn,1001,nullptr,10)
     && !S::CanRemoveCopy(Worn,1001,Bag,10001);
}
static_assert(Removal());
int main(){return AllSlots()&&Migration()&&Removal()?0:1;}
