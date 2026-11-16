#pragma once

#include "CoreMinimal.h"

// Port of rarity thresholds/pity/affix-count rules in Godot equipment_rng.gd.
// Accept an already-generated integer roll [0,999] for cross-engine parity.
// Do not claim matching seeded RNG sequences until empirically verified.
namespace MoonfallLootRules
{
    inline constexpr const TCHAR* Rarities[5] =
        { TEXT("Common"), TEXT("Uncommon"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary") };
    inline constexpr const TCHAR* AffixKinds[6] =
        { TEXT("Power"), TEXT("Vitality"), TEXT("Haste"),
          TEXT("Crit"), TEXT("Ward"), TEXT("Elemental") };

    inline int32 RarityIndex(int32 Chance, int32 Luck, int32 Pity)
    {
        if (Chance < 0 || Chance > 999) return INDEX_NONE;
        const int32 Boost = FMath::Clamp(Luck, 0, 40);
        int32 Rarity = Chance < 10 + Boost / 2 ? 4 :
                       Chance < 50 + Boost ? 3 :
                       Chance < 170 + Boost * 2 ? 2 :
                       Chance < 410 + Boost * 3 ? 1 : 0;
        if (Pity >= 30) Rarity = FMath::Max(Rarity, 2);
        if (Pity >= 80) Rarity = FMath::Max(Rarity, 3);
        return Rarity;
    }

    inline int32 NextPity(int32 Current, int32 RarityIndexValue)
    {
        return RarityIndexValue >= 2 ? 0 : FMath::Min(80, FMath::Max(0, Current) + 1);
    }

    inline int32 AffixCount(int32 RarityIndexValue)
    {
        return FMath::Clamp(RarityIndexValue, 0, 3);
    }

    // Caller supplies RNG index draws; remove from the pool to prevent duplicates.
    template<typename TDrawIndex>
    TArray<FString> PickAffixes(int32 RarityIndexValue, TDrawIndex DrawIndex)
    {
        TArray<FString> Pool;
        for (const TCHAR* Name : AffixKinds) Pool.Add(Name);
        TArray<FString> Result;
        const int32 Count = AffixCount(RarityIndexValue);
        for (int32 I = 0; I < Count; ++I)
        {
            const int32 Index = DrawIndex(Pool.Num());
            if (!Pool.IsValidIndex(Index)) { Result.Reset(); return Result; }
            Result.Add(Pool[Index]);
            Pool.RemoveAt(Index);
        }
        return Result;
    }
}
