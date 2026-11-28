#pragma once
#include "MoonfallOceanGathering.h"
#include "MoonfallAquaticRewards.h"

namespace MoonfallOceanCrafting
{
    constexpr int BaitCapacity=20;
    constexpr int BaitPerCraft=5;
    constexpr int RecipeCount=5;
    constexpr int ProvisionCapacity=5;
    constexpr int MaxCraftCount=1000000;
    constexpr int ContractRegionCount=9;
    constexpr unsigned ContractMask=(1u<<ContractRegionCount)-1u;
    constexpr int ContractMineral(int Province) { return Province>=0&&Province<ContractRegionCount?1+Province%3:-1; }
    constexpr unsigned MaterialMask(int Recipe) {
        return Recipe==0?1u:Recipe==1?6u:Recipe==2?8u:Recipe==3?9u:Recipe==4?1u:0u;
    }
    struct FWorkshop {
        int Spent[4]={};
        int FishSpent[3]={};
        int Bait=0;
        int CookingCrafts=0,AlchemyCrafts=0,SmithingCrafts=0;
        int Provisions=0;
        unsigned DeliveredContracts=0;
        int ContractCount() const {
            int N=0; for(int I=0;I<ContractRegionCount;++I) N+=int(bool(DeliveredContracts&(1u<<I))); return N;
        }
        bool HasDelivered(int Province) const {
            return ContractMineral(Province)>=0 && (DeliveredContracts&(1u<<Province));
        }
        bool ConsumeProvision() {
            if(Provisions<=0||Provisions>ProvisionCapacity) return false;
            --Provisions; return true;
        }
        int FirstAvailableFish(const MoonfallOceanGathering::FCollection& C) const {
            int FishId=-1;
            for(int I=0;I<3;++I) {
                if(FishSpent[I]<0||FishSpent[I]>C.Species(I)) return -1;
                if(FishId<0&&FishSpent[I]<C.Species(I)) FishId=I;
            }
            return FishId;
        }
        bool CanDeliver(int Province,const MoonfallOceanGathering::FCollection& C,
                        const MoonfallAquaticRewards::FSessionGear& Gear) const {
            const int Mineral=ContractMineral(Province);
            return Mineral>=0 && !(DeliveredContracts&~ContractMask) && !HasDelivered(Province)
                && HasMaterial(0,C) && HasMaterial(Mineral,C) && FirstAvailableFish(C)>=0
                && Gear.TonicCharges>=0 && Gear.TonicCharges<3;
        }
        // Caller verifies regional discovery milestones and the trader context.
        bool Deliver(int Province,const MoonfallOceanGathering::FCollection& C,
                     MoonfallAquaticRewards::FSessionGear& Gear) {
            if(!CanDeliver(Province,C,Gear)) return false;
            const int FishId=FirstAvailableFish(C);
            ++Spent[0]; ++Spent[ContractMineral(Province)]; ++FishSpent[FishId];
            ++Gear.TonicCharges; DeliveredContracts|=1u<<Province;
            return true;
        }
        bool UseBait() { if(Bait<=0||Bait>BaitCapacity) return false; --Bait; return true; }
        int RemainingSpecies(int Id,const MoonfallOceanGathering::FCollection& C) const {
            return Id>=0&&Id<3?C.Species(Id)-FishSpent[Id]:0;
        }
        int RemainingFish(const MoonfallOceanGathering::FCollection& C) const {
            int N=0;
            for(int I=0;I<3;++I) N+=RemainingSpecies(I,C);
            return N;
        }
        int AvailableMaterials(const MoonfallOceanGathering::FCollection& C) const {
            int N=0;
            for(int I=0;I<4;++I) N+=C.Material(I)-Spent[I];
            return N;
        }
        int RemainingMaterial(int Id,const MoonfallOceanGathering::FCollection& C) const {
            return Id>=0&&Id<4?C.Material(Id)-Spent[Id]:0;
        }
        bool HasMaterial(int Id,const MoonfallOceanGathering::FCollection& C) const {
            return Id>=0&&Id<4&&Spent[Id]>=0&&Spent[Id]<C.Material(Id);
        }
        bool CanCraft(int Recipe,const MoonfallOceanGathering::FCollection& C,
                      const MoonfallAquaticRewards::FSessionGear& Gear) const {
            auto Copy=*this;
            auto Preview=Gear;
            return Copy.Craft(Recipe,C,Preview);
        }
        bool Craft(int Recipe,const MoonfallOceanGathering::FCollection& C,
                   MoonfallAquaticRewards::FSessionGear& Gear) {
            const unsigned Mask=MaterialMask(Recipe);
            if(!Mask) return false;
            for(int I=0;I<4;++I) if((Mask&(1u<<I))&&!HasMaterial(I,C)) return false;
            int FishId=-1;
            if(Recipe!=1 && Recipe!=3) {
                FishId=FirstAvailableFish(C);
                if(FishId<0) return false;
            }
            if(Recipe==0&&(AlchemyCrafts<0||AlchemyCrafts>=MaxCraftCount)) return false;
            if((Recipe==1||Recipe==2)&&(SmithingCrafts<0||SmithingCrafts>=MaxCraftCount)) return false;
            if(Recipe==4&&(CookingCrafts<0||CookingCrafts>=MaxCraftCount
                || Provisions<0||Provisions>=ProvisionCapacity||Provisions>CookingCrafts)) return false;
            if(Recipe==3) {
                if(Bait<0||Bait>BaitCapacity-BaitPerCraft) return false;
                Bait+=BaitPerCraft;
            } else if(Recipe==0) {
                if(Gear.TonicCharges<0||Gear.TonicCharges>=3) return false;
                ++Gear.TonicCharges; ++AlchemyCrafts;
            } else if(Recipe==4) {
                ++Provisions; ++CookingCrafts;
            } else {
                const int Piece=Recipe-1;
                const int Slot=Piece==0?2:7;
                const auto Item=MoonfallEquipmentLayout::DecodeFullItem(Gear.Codes[Piece]);
                if(!MoonfallEquipmentLayout::ValidFullItem(Gear.Codes[Piece],Slot)
                   || Item.Trait!=3 || Item.Rarity<1 || Item.Rarity>=5) return false;
                const int Code=MoonfallEquipmentLayout::EncodeFullItem(Slot,Item.Trait,Item.Rarity+1,Item.Tier);
                if(!Code) return false;
                Gear.Codes[Piece]=Code; ++SmithingCrafts;
            }
            for(int I=0;I<4;++I) if(Mask&(1u<<I)) ++Spent[I];
            if(FishId>=0) ++FishSpent[FishId];
            return true;
        }
    };
}
