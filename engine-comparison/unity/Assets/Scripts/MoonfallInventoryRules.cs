using System;
using System.Collections.Generic;

// Ported rule behavior from godot-v38/scripts/systems/inventory_rules.gd.
// Pure rules: no Unity scene, asset, save, or UI dependencies.
public static class MoonfallInventoryRules
{
    public const int BagStartCapacity = 100;
    public const int BagMaxCapacity = 500;
    public const int BagUpgradeStep = 50;
    public const int VaultLimit = 1000;
    public const int MaterialStackSize = 999;
    public const int BagPageSize = 20;
    public const int MaterialLimit = 99999;

    public static int ValidCapacity(int size) => Math.Min(BagMaxCapacity, Math.Max(BagStartCapacity, size));

    public static int UpgradeCost(int capacity)
    {
        if (capacity >= BagMaxCapacity) return 0;
        return 100 + ((ValidCapacity(capacity) - BagStartCapacity) / BagUpgradeStep) * 150;
    }

    public static bool CanStoreGear(int bagCount, int capacity = BagStartCapacity)
        => bagCount < ValidCapacity(capacity);

    public static List<int> MaterialStackCounts(int quantity)
    {
        var stacks = new List<int>();
        int remainder = Math.Min(MaterialLimit, Math.Max(0, quantity));
        while (remainder > 0)
        {
            int stack = Math.Min(remainder, MaterialStackSize);
            stacks.Add(stack);
            remainder -= stack;
        }
        return stacks;
    }

    public static int PageCount(int count, int pageSize = BagPageSize)
        => Math.Max(1, (int)Math.Ceiling((double)Math.Max(0, count) / Math.Max(1, pageSize)));

    public static int EquipmentPower(int power) => Math.Min(250, Math.Max(0, power));

    public static int AddMaterial(int current, int amount)
        => (int)Math.Min(MaterialLimit, Math.Max(0L, (long)current + amount));

    public static bool TryConsumeOne(Dictionary<string, int> materials, string name)
    {
        if (materials == null || name == null || !materials.TryGetValue(name, out int count) || count < 1)
            return false;
        materials[name] = count - 1;
        return true;
    }

    // Returns an empty string on success, matching Godot's craft_issue contract.
    public static string CraftIssue(
        string recipeName, IReadOnlyDictionary<string, int> costs,
        Dictionary<string, int> materials, int bagCount, int capacity, int resultingPower)
    {
        if (costs == null || materials == null) return "Invalid recipe cost.";
        foreach (var cost in costs)
        {
            int needed = cost.Value;
            if (needed <= 0 || needed > MaterialLimit || string.IsNullOrEmpty(cost.Key))
                return "Invalid recipe cost.";
            if (!materials.TryGetValue(cost.Key, out int amount) || amount < needed)
                return "Missing " + cost.Key + " for " + (recipeName ?? "recipe");
        }
        if (resultingPower > 0 && !CanStoreGear(bagCount, capacity)) return "Bag full.";
        return "";
    }

    // Validate before mutating; a failed craft never consumes materials.
    public static bool TryCraft(
        string recipeName, IReadOnlyDictionary<string, int> costs,
        Dictionary<string, int> materials, int bagCount, int capacity, int resultingPower,
        out string issue)
    {
        issue = CraftIssue(recipeName, costs, materials, bagCount, capacity, resultingPower);
        if (issue.Length != 0) return false;
        foreach (var cost in costs) materials[cost.Key] -= cost.Value;
        return true;
    }
}
