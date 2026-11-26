#pragma once
#include <array>
#include <cstddef>
namespace MoonfallEquipmentSlots {
// Stable decimal item codes; never renumber the three legacy slots.
enum ESlot : int { Weapon=0, Armor, Accessory, Head, Shoulders, Hands, Legs, Feet, Ring, Count };
inline constexpr int LegacyCount=3, SlotCount=Count;
using FLegacy=std::array<int,LegacyCount>;
using FLoadout=std::array<int,SlotCount>;
constexpr bool Known(int Slot) {return Slot>=0&&Slot<SlotCount;}
constexpr bool Legacy(int Slot) {return Slot>=0&&Slot<LegacyCount;}
constexpr const char* Name(int Slot) {
 constexpr const char* Names[SlotCount]={"Weapon","Armor","Accessory","Head","Shoulders","Hands","Legs","Feet","Ring"};
 return Known(Slot)?Names[Slot]:"Unknown";
}
struct FItem { int Affix=0, Slot=-1, Rarity=0, Tier=0; };
constexpr FItem Decode(int Code) {
 if(Code<=0)return {};
 const FItem Item{Code/100000,(Code/10000)%10,(Code/1000)%10,Code%1000};
 if(!Known(Item.Slot)||Item.Affix<0||Item.Affix>3||Item.Rarity<0||Item.Rarity>5
    ||Item.Tier<1||Item.Tier>25||(Item.Rarity==0&&Item.Affix!=0)
    ||Code!=Item.Affix*100000+Item.Slot*10000+Item.Rarity*1000+Item.Tier)return {};
 return Item;
}
constexpr bool Valid(int Code,int Slot) {return Known(Slot)&&Decode(Code).Slot==Slot;}
constexpr FLoadout FromLegacy(const FLegacy& Old) {
 FLoadout Out{};
 for(int Slot=0;Slot<LegacyCount;++Slot)Out[Slot]=Old[Slot];
 return Out;
}
constexpr FLegacy ToLegacy(const FLoadout& Worn) {return {Worn[0],Worn[1],Worn[2]};}
// Validation is separate from migration: never silently erase an invalid or
// unowned item. Callers must refuse the whole write/load before mutation.
constexpr bool ValidateOwned(const FLoadout& Worn,const int* Bag,std::size_t Size) {
 if(Size>10000||(!Bag&&Size!=0))return false;
 for(int Slot=0;Slot<SlotCount;++Slot) {
  const int Code=Worn[Slot];
  if(Code==0)continue;
  if(!Valid(Code,Slot))return false;
  bool Found=false;
  for(std::size_t I=0;I<Size;++I)if(Bag[I]==Code){Found=true;break;}
  if(!Found)return false;
 }
 return true;
}
// V17 stores only the six additional codes alongside the three existing fields.
// Reject unknown versions, incomplete arrays, and pre-V17 extra fields rather
// than silently accepting data that could be lost on a later autosave.
inline constexpr int ExpandedVersion=17, ExtraCount=SlotCount-LegacyCount;
using FExtra=std::array<int,ExtraCount>;
constexpr FExtra ToExtra(const FLoadout& Worn) {
 FExtra Out{};
 for(int I=0;I<ExtraCount;++I)Out[I]=Worn[LegacyCount+I];
 return Out;
}
constexpr bool ReadSavedLoadout(int Version,const FLegacy& Old,
 const int* Extra,std::size_t ExtraSize,const int* Bag,std::size_t BagSize,FLoadout& Out) {
 if(Version<1||Version>ExpandedVersion)return false;
 if(Version<ExpandedVersion?ExtraSize!=0:ExtraSize!=ExtraCount||!Extra)return false;
 // Validate the whole inventory, including unworn gear, before publishing a loadout.
 if(BagSize>10000||(!Bag&&BagSize!=0))return false;
 for(std::size_t I=0;I<BagSize;++I) {
  const auto Item=Decode(Bag[I]);
  if(!Known(Item.Slot)||(Version<ExpandedVersion&&!Legacy(Item.Slot)))return false;
 }
 FLoadout Candidate=FromLegacy(Old);
 if(Version==ExpandedVersion)
  for(int I=0;I<ExtraCount;++I)Candidate[LegacyCount+I]=Extra[I];
 if(!ValidateOwned(Candidate,Bag,BagSize))return false;
 Out=Candidate;
 return true;
}
static_assert(Weapon==0&&Armor==1&&Accessory==2&&Ring==8&&SlotCount==9);
}
