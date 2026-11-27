#pragma once
#include "MoonfallOceanGathering.h"
#include "MoonfallAquaticRewards.h"

namespace MoonfallOceanCrafting
{
    constexpr unsigned MaterialMask(int Recipe) {
        return Recipe==0?1u:Recipe==1?6u:Recipe==2?8u:0u;
    }
    struct FWorkshop {
        int Spent[4]={};
        int FishSpent[3]={};
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
            if(Recipe!=1) {
                for(int I=0;I<3;++I) {
                    if(FishSpent[I]<0 || FishSpent[I]>C.Species(I)) return false;
                    if(FishId<0 && FishSpent[I]<C.Species(I)) FishId=I;
                }
                if(FishId<0) return false;
            }
            if(Recipe==0) {
                if(Gear.TonicCharges<0||Gear.TonicCharges>=3) return false;
                ++Gear.TonicCharges;
            } else {
                const int Piece=Recipe-1;
                const int Slot=Piece==0?2:7;
                const auto Item=MoonfallEquipmentLayout::DecodeFullItem(Gear.Codes[Piece]);
                if(!MoonfallEquipmentLayout::ValidFullItem(Gear.Codes[Piece],Slot)
                   || Item.Trait!=3 || Item.Rarity<1 || Item.Rarity>=5) return false;
                const int Code=MoonfallEquipmentLayout::EncodeFullItem(Slot,Item.Trait,Item.Rarity+1,Item.Tier);
                if(!Code) return false;
                Gear.Codes[Piece]=Code;
            }
            for(int I=0;I<4;++I) if(Mask&(1u<<I)) ++Spent[I];
            if(FishId>=0) ++FishSpent[FishId];
            return true;
        }
    };
}
