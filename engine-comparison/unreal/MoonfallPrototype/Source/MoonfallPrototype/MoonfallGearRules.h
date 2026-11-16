#pragma once

#include "CoreMinimal.h"

// Pure, experimental counterpart of the Godot inventory_rules.gd item operations.
// Intentionally no UObjects, UI, saves, or production asset dependencies.
namespace MoonfallGearRules
{
    struct FGear
    {
        FString Name;
        FString Rarity = TEXT("Common");
        FString Affix;
        int32 Power = 0;
    };

    inline bool Transfer(TArray<FGear>& Source, TArray<FGear>& Destination,
                         int32 Index, int32 Capacity)
    {
        if (&Source == &Destination || Index < 0 || Index >= Source.Num()
            || Destination.Num() >= Capacity)
            return false;
        Destination.Add(Source[Index]); // FString value-copy, no shared mutable item.
        Source.RemoveAt(Index);
        return true;
    }

    inline int32 RarityWeight(const FString& Rarity)
    {
        if (Rarity == TEXT("Uncommon")) return 1;
        if (Rarity == TEXT("Rare")) return 2;
        if (Rarity == TEXT("Crafted")) return 3;
        if (Rarity == TEXT("Epic")) return 4;
        if (Rarity == TEXT("Legendary")) return 5;
        if (Rarity == TEXT("Mythic")) return 6;
        return 0;
    }

    inline TArray<int32> MatchingIndices(const TArray<FGear>& Items,
        const FString& Search, const FString& FilterMode, const FString& SortMode)
    {
        TArray<int32> Indices;
        const FString Needle = Search.TrimStartAndEnd().Left(80).ToLower();
        for (int32 I = 0; I < Items.Num(); ++I)
        {
            const FGear& Item = Items[I];
            if (!FilterMode.IsEmpty() && FilterMode != TEXT("All")
                && Item.Rarity != FilterMode)
                continue;
            const FString Text = (Item.Name + TEXT(" ") + Item.Rarity
                + TEXT(" ") + Item.Affix).ToLower();
            if (!Needle.IsEmpty() && !Text.Contains(Needle)) continue;
            Indices.Add(I);
        }
        if (SortMode == TEXT("Power") || SortMode == TEXT("Rarity")
            || SortMode == TEXT("Name"))
        {
            Indices.Sort([&Items, &SortMode](int32 A, int32 B)
            {
                const FGear& First = Items[A];
                const FGear& Second = Items[B];
                if (SortMode == TEXT("Power") && First.Power != Second.Power)
                    return First.Power > Second.Power;
                if (SortMode == TEXT("Rarity"))
                {
                    int32 Wa = RarityWeight(First.Rarity), Wb = RarityWeight(Second.Rarity);
                    if (Wa != Wb) return Wa > Wb;
                }
                if (SortMode == TEXT("Name"))
                {
                    int32 Comparison = First.Name.Compare(Second.Name, ESearchCase::IgnoreCase);
                    if (Comparison != 0) return Comparison < 0;
                }
                return A < B; // Stable original-index tie breaker.
            });
        }
        return Indices;
    }

    inline int32 FirstDiscardableDrop(const TArray<FGear>& Drops)
    {
        for (int32 I = 0; I < Drops.Num(); ++I)
            if (Drops[I].Rarity == TEXT("Common") ||
                Drops[I].Rarity == TEXT("Uncommon")) return I;
        return INDEX_NONE;
    }
}
