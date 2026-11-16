#pragma once

#include "CoreMinimal.h"

// Pure rules adapted from Godot scripts/systems/inventory_rules.gd.
// No assets, runtime singleton, save or gameplay actor dependencies.
namespace MoonfallInventoryRules
{
    constexpr int32 BagStartCapacity = 100;
    constexpr int32 BagMaxCapacity = 500;
    constexpr int32 BagUpgradeStep = 50;
    constexpr int32 VaultLimit = 1000;
    constexpr int32 MaterialStackSize = 999;
    constexpr int32 MaterialLimit = 99999;
    constexpr int32 BagPageSize = 20;

    inline int32 ValidCapacity(int32 Value)
    {
        return FMath::Clamp(Value, BagStartCapacity, BagMaxCapacity);
    }

    inline int32 UpgradeCost(int32 Capacity)
    {
        return Capacity >= BagMaxCapacity ? 0 :
            100 + ((ValidCapacity(Capacity) - BagStartCapacity) / BagUpgradeStep) * 150;
    }

    inline bool CanStoreGear(int32 Count, int32 Capacity = BagStartCapacity)
    {
        return Count < ValidCapacity(Capacity);
    }

    inline TArray<int32> MaterialStacks(int32 Quantity)
    {
        TArray<int32> Result;
        int32 Remaining = FMath::Clamp(Quantity, 0, MaterialLimit);
        while (Remaining > 0)
        {
            const int32 Stack = FMath::Min(Remaining, MaterialStackSize);
            Result.Add(Stack);
            Remaining -= Stack;
        }
        return Result;
    }

    inline int32 PageCount(int32 Count, int32 PageSize = BagPageSize)
    {
        return FMath::Max(1, (FMath::Max(0, Count) + FMath::Max(1, PageSize) - 1) / FMath::Max(1, PageSize));
    }

    inline int32 EquipmentPower(int32 Power)
    {
        return FMath::Clamp(Power, 0, 250);
    }

    inline int32 AddMaterial(int32 Current, int32 Amount)
    {
        return static_cast<int32>(FMath::Clamp(static_cast<int64>(Current) + Amount, static_cast<int64>(0), static_cast<int64>(MaterialLimit)));
    }

    inline bool ConsumeOne(TMap<FString, int32>& Materials, const FString& Name)
    {
        int32* Count = Materials.Find(Name);
        if (!Count || *Count < 1) return false;
        --(*Count);
        return true;
    }

    // Empty string means recipe can be crafted. Validate before any mutation.
    inline FString CraftIssue(const FString& RecipeName, const TMap<FString, int32>& Costs,
                              const TMap<FString, int32>& Materials, int32 BagCount,
                              int32 Capacity, int32 ResultPower)
    {
        for (const TPair<FString, int32>& Cost : Costs)
        {
            if (Cost.Key.IsEmpty() || Cost.Value <= 0 || Cost.Value > MaterialLimit)
                return TEXT("Invalid recipe cost.");
            const int32* Available = Materials.Find(Cost.Key);
            if (!Available || *Available < Cost.Value)
                return TEXT("Missing ") + Cost.Key + TEXT(" for ") + (RecipeName.IsEmpty() ? TEXT("recipe") : RecipeName);
        }
        if (ResultPower > 0 && !CanStoreGear(BagCount, Capacity))
            return TEXT("Bag full.");
        return FString();
    }

    inline bool TryCraft(const FString& RecipeName, const TMap<FString, int32>& Costs,
                         TMap<FString, int32>& Materials, int32 BagCount,
                         int32 Capacity, int32 ResultPower, FString& OutIssue)
    {
        OutIssue = CraftIssue(RecipeName, Costs, Materials, BagCount, Capacity, ResultPower);
        if (!OutIssue.IsEmpty()) return false;
        for (const TPair<FString, int32>& Cost : Costs)
            Materials.FindChecked(Cost.Key) -= Cost.Value;
        return true;
    }
}
