using System;
using System.Collections.Generic;

// Godot equipment_rng.gd rarity selection, separated from engine RNG.
// Input chance must be 0..999; seed-to-chance equivalence is NOT yet established.
public static class MoonfallLootRules
{
    public static readonly string[] Rarities = { "Common", "Uncommon", "Rare", "Epic", "Legendary" };
    public static readonly string[] AffixKinds = { "Power", "Vitality", "Haste", "Crit", "Ward", "Elemental" };

    public static int RarityIndex(int chance, int luck, int pity)
    {
        if (chance < 0 || chance > 999) throw new ArgumentOutOfRangeException(nameof(chance));
        int boost = Math.Max(0, Math.Min(40, luck));
        // Godot's integer division truncates toward zero; boost is always nonnegative.
        int rarity = chance < 10 + boost / 2 ? 4 :
                     chance < 50 + boost ? 3 :
                     chance < 170 + boost * 2 ? 2 :
                     chance < 410 + boost * 3 ? 1 : 0;
        if (pity >= 30) rarity = Math.Max(rarity, 2);
        if (pity >= 80) rarity = Math.Max(rarity, 3);
        return rarity;
    }

    public static int NextPity(int current, int rarityIndex)
        => rarityIndex >= 2 ? 0 : Math.Min(80, Math.Max(0, current) + 1);

    // Godot selects at most three DISTINCT affixes for rarity ranks 1..4.
    // Selection is intentionally left to the engine's explicitly seeded RNG.
    public static int AffixCount(int rarityIndex)
        => Math.Max(0, Math.Min(3, rarityIndex));

    public static List<string> PickAffixes(int rarityIndex, Func<int, int> drawIndex)
    {
        if (drawIndex == null) throw new ArgumentNullException(nameof(drawIndex));
        var pool = new List<string>(AffixKinds);
        var result = new List<string>();
        for (int i = 0; i < AffixCount(rarityIndex); i++)
        {
            int index = drawIndex(pool.Count);
            if (index < 0 || index >= pool.Count)
                throw new ArgumentOutOfRangeException(nameof(drawIndex), "Invalid RNG index.");
            result.Add(pool[index]);
            pool.RemoveAt(index);
        }
        return result;
    }
}
