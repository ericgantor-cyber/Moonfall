#pragma once
#include "MoonfallOceanCrafting.h"
#include "MoonfallIslandQuest.h"
#include "MoonfallOceanEvents.h"
#include "MoonfallRegionalDiscovery.h"
namespace MoonfallProfessionLedger {
 constexpr int LegacyCount=31;
 constexpr int RegionalCount=34;
 constexpr int WorkshopCount=39;
 constexpr int Count=40;
 struct FSnapshot {
  MoonfallOceanGathering::FCollection Gathering;
  MoonfallOceanCrafting::FWorkshop Workshop;
  MoonfallAquaticRewards::FSessionGear Gear;
  MoonfallIslandQuest::FJournal Journal;
  MoonfallOceanEvents::FSightings Sightings;
  unsigned Wrecks=0,Caves=0;
  MoonfallRegionalDiscovery::FProgress Regions;
  void Encode(int (&Out)[Count]) const {
   int N=0;
   for(int I:Gathering.Harvested) Out[N++]=I;
   for(int I:Workshop.Spent) Out[N++]=I;
   for(int I:Gathering.Fish) Out[N++]=I;
   for(int I:Workshop.FishSpent) Out[N++]=I;
   for(int I:Gathering.TrophySpecies) Out[N++]=I;
   Out[N++]=int(Gathering.LegendarySeen); Out[N++]=Workshop.Bait;
   for(int I:Gear.Codes) Out[N++]=I;
   Out[N++]=int(Gear.Claims); Out[N++]=Gear.LastCode; Out[N++]=Gear.TonicCharges;
   Out[N++]=int(Journal.Accepted); Out[N++]=int(Journal.Completed); Out[N++]=Journal.Credits;
   Out[N++]=int(Sightings.Seen); Out[N++]=int(Wrecks); Out[N++]=int(Caves);
   Out[N++]=int(Regions.Visited); Out[N++]=int(Regions.Caught); Out[N++]=int(Regions.Harvested);
   Out[N++]=Workshop.CookingCrafts; Out[N++]=Workshop.AlchemyCrafts; Out[N++]=Workshop.SmithingCrafts;
   Out[N++]=Workshop.Provisions; Out[N++]=int(Workshop.DeliveredContracts);
   Out[N++]=int(Regions.Treasures);
  }
  bool Decode(const int* In,int Size) {
   if(!In||(Size!=Count&&Size!=WorkshopCount&&Size!=RegionalCount&&Size!=LegacyCount)) return false;
   FSnapshot S;
   int N=0;
   for(int& I:S.Gathering.Harvested) I=In[N++];
   for(int& I:S.Workshop.Spent) I=In[N++];
   for(int& I:S.Gathering.Fish) I=In[N++];
   for(int& I:S.Workshop.FishSpent) I=In[N++];
   for(int& I:S.Gathering.TrophySpecies) I=In[N++];
   S.Gathering.LegendarySeen=unsigned(In[N++]); S.Workshop.Bait=In[N++];
   for(int& I:S.Gear.Codes) I=In[N++];
   S.Gear.Claims=unsigned(In[N++]); S.Gear.LastCode=In[N++]; S.Gear.TonicCharges=In[N++];
   S.Journal.Accepted=unsigned(In[N++]); S.Journal.Completed=unsigned(In[N++]); S.Journal.Credits=In[N++];
   S.Sightings.Seen=unsigned(In[N++]); S.Wrecks=unsigned(In[N++]); S.Caves=unsigned(In[N++]);
   if(Size>=RegionalCount) { S.Regions.Visited=unsigned(In[N++]); S.Regions.Caught=unsigned(In[N++]); S.Regions.Harvested=unsigned(In[N++]); }
   if(Size>=WorkshopCount) { S.Workshop.CookingCrafts=In[N++]; S.Workshop.AlchemyCrafts=In[N++]; S.Workshop.SmithingCrafts=In[N++]; S.Workshop.Provisions=In[N++]; S.Workshop.DeliveredContracts=unsigned(In[N++]); }
   if(Size==Count) S.Regions.Treasures=unsigned(In[N++]);
   if(!S.Regions.Valid()) return false;
   for(int I=0;I<4;++I) if(S.Gathering.Harvested[I]<0||S.Gathering.Harvested[I]>MoonfallOceanGathering::MaxMaterialPerType||
    S.Workshop.Spent[I]<0||S.Workshop.Spent[I]>S.Gathering.Harvested[I]) return false;
   for(int I=0;I<3;++I) {
    const int Fish=S.Gathering.Fish[I], Trophy=S.Gathering.TrophySpecies[I];
    if(Fish<0||Fish>MoonfallOceanGathering::MaxSessionCatches||S.Workshop.FishSpent[I]<0||
      S.Workshop.FishSpent[I]>Fish||Trophy<0||Trophy>Fish||(S.Gathering.HasLegendary(I)&&Trophy==0)) return false;
    S.Gathering.TrophyCatches+=Trophy;
    const int Code=S.Gear.Codes[I],Slot=MoonfallAquaticRewards::RewardSlot(I);
    if(Code&&(!MoonfallEquipmentLayout::ValidFullItem(Code,Slot)||MoonfallEquipmentLayout::DecodeFullItem(Code).Trait!=3)) return false;
    if(bool(Code)!=bool(S.Gear.Claims&((1u<<I)|(1u<<(I+3))))) return false;
   }
   const auto& W=S.Workshop;
   using namespace MoonfallOceanCrafting;
   if(W.CookingCrafts<0||W.CookingCrafts>MaxCraftCount||W.AlchemyCrafts<0||W.AlchemyCrafts>MaxCraftCount||
      W.SmithingCrafts<0||W.SmithingCrafts>MaxCraftCount||W.Provisions<0||W.Provisions>ProvisionCapacity||
      W.Provisions>W.CookingCrafts||(W.DeliveredContracts&~ContractMask)||
      (W.DeliveredContracts&~(S.Regions.Visited&S.Regions.Caught&S.Regions.Harvested))) return false;
   const int Delivered=W.ContractCount();
   if(W.CookingCrafts+W.AlchemyCrafts+Delivered>W.Spent[0]||
      W.CookingCrafts+W.AlchemyCrafts+Delivered>W.FishSpent[0]+W.FishSpent[1]+W.FishSpent[2]||
      W.SmithingCrafts>W.Spent[1]+W.Spent[3]) return false;
   for(int Mineral=1;Mineral<4;++Mineral) {
    int Required=0;
    for(int Province=0;Province<ContractRegionCount;++Province)
     if(W.HasDelivered(Province)&&ContractMineral(Province)==Mineral) ++Required;
    if(Required>W.Spent[Mineral]) return false;
   }
   if(S.Gathering.Catches()>MoonfallOceanGathering::MaxSessionCatches||S.Gathering.LegendarySeen>7||
    S.Workshop.Bait<0||S.Workshop.Bait>MoonfallOceanCrafting::BaitCapacity||
    S.Gear.Claims>63||S.Gear.TonicCharges<0||S.Gear.TonicCharges>3||
    S.Journal.Accepted>7||S.Journal.Completed>7||(S.Journal.Completed&~S.Journal.Accepted)||
    S.Journal.Credits<0||S.Journal.Credits>S.Journal.Count()*2||S.Sightings.Seen>63||S.Wrecks>3||S.Caves>3) return false;
   if((S.Journal.Completed&1u)&&S.Wrecks!=3) return false;
   if((S.Journal.Completed&2u)&&S.Caves!=3) return false;
   if((S.Journal.Completed&4u)&&S.Gear.Claims!=63) return false;
   if(S.Gear.LastCode) {
    bool Valid=false;
    for(int I=0;I<6;++I) if((S.Gear.Claims&(1u<<I))&&
     (S.Gear.LastCode==MoonfallAquaticRewards::RewardCode(I,0)||S.Gear.LastCode==MoonfallAquaticRewards::RewardCode(I,99))) Valid=true;
    if(!Valid) return false;
   } else if(S.Gear.Claims) return false;
   *this=S; return true;
  }
 };
}
