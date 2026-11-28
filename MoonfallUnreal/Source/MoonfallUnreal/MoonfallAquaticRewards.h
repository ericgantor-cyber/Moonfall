#pragma once
#include "MoonfallGearAdvisorPolicy.h"

namespace MoonfallAquaticRewards
{
    // Three existing gear slots. Preview loaned gear never enters the campaign bag.
    constexpr int RewardSlot(int Index) { return Index==0||Index==3 ? 2 : Index==1||Index==4 ? 7 : Index==2||Index==5 ? 6 : -1; }
    constexpr int RewardCode(int Index, int Roll)
    {
        const int Slot=RewardSlot(Index);
        if (Slot<0 || Roll<0 || Roll>99) return 0;
        const int Rarity=Index==5 ? (Roll<20?5:4) : Index>=3 ? (Roll<15?4:3) : (Roll<20?3:2);
        return MoonfallEquipmentLayout::EncodeFullItem(Slot,3,Rarity,Index+1);
    }
    struct FSessionGear {
        unsigned Claims=0;
        int Codes[3]={};
        int LastCode=0;
        int TonicCharges=0;
        bool Claim(int Index,int Roll)
        {
            const int Code=RewardCode(Index,Roll);
            if (!Code || (Claims&(1u<<Index))) return false;
            const int SlotIndex=Index%3;
            const auto Old=MoonfallEquipmentLayout::DecodeFullItem(Codes[SlotIndex]);
            const auto New=MoonfallEquipmentLayout::DecodeFullItem(Code);
            if (!Codes[SlotIndex] || New.Rarity>Old.Rarity || (New.Rarity==Old.Rarity && New.Tier>Old.Tier))
                Codes[SlotIndex]=Code;
            LastCode=Code;
            Claims|=1u<<Index;
            if (Index%2==0 && TonicCharges<3) ++TonicCharges;
            return true;
        }
        bool ConsumeTonic() {
            if (TonicCharges<=0 || TonicCharges>3) return false;
            --TonicCharges;
            return true;
        }
        int Count() const {
            int N=0; for (int I=0;I<6;++I) if (Claims&(1u<<I)) ++N; return N;
        }
        int Pieces() const {
            int N=0; for (int Code:Codes) if (Code>0) ++N; return N;
        }
        int Rarity(int Piece) const {
            return Piece>=0 && Piece<3 && Codes[Piece]>0 ? MoonfallEquipmentLayout::DecodeFullItem(Codes[Piece]).Rarity : -1;
        }
    };
}
