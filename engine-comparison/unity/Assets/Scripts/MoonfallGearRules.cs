using System;
using System.Collections.Generic;

// Pure data-oriented translation of the gear search, sort, transfer and drop
// selection behavior in Godot scripts/systems/inventory_rules.gd.
public static class MoonfallGearRules
{
    public sealed class Gear
    {
        public string Name = "";
        public string Rarity = "Common";
        public string Affix = "";
        public int Power;
        public Gear Copy() => new Gear { Name = Name, Rarity = Rarity, Affix = Affix, Power = Power };
    }

    private static readonly Dictionary<string, int> RarityWeight =
        new Dictionary<string, int>(StringComparer.Ordinal)
        {
            { "Common", 0 }, { "Uncommon", 1 }, { "Rare", 2 },
            { "Crafted", 3 }, { "Epic", 4 }, { "Legendary", 5 }, { "Mythic", 6 }
        };

    public static bool Transfer(List<Gear> source, List<Gear> destination, int index, int capacity)
    {
        if (source == null || destination == null || ReferenceEquals(source, destination)
            || index < 0 || index >= source.Count || capacity <= destination.Count
            || source[index] == null)
            return false;
        Gear copy = source[index].Copy();
        destination.Add(copy);
        source.RemoveAt(index);
        return true;
    }

    public static List<int> MatchingIndices(IReadOnlyList<Gear> items, string search,
        string filterMode, string sortMode)
    {
        var indices = new List<int>();
        if (items == null) return indices;
        string needle = (search ?? "").Trim().ToLowerInvariant();
        if (needle.Length > 80) needle = needle.Substring(0, 80);
        for (int i = 0; i < items.Count; i++)
        {
            Gear item = items[i];
            if (item == null) continue;
            string rarity = item.Rarity ?? "Common";
            if ((filterMode ?? "All") != "All" && rarity != filterMode) continue;
            string searchable = ((item.Name ?? "") + " " + rarity + " " +
                (item.Affix ?? "")).ToLowerInvariant();
            if (needle.Length > 0 && !searchable.Contains(needle)) continue;
            indices.Add(i);
        }
        int Weight(string rarity) => RarityWeight.TryGetValue(rarity ?? "", out int w) ? w : 0;
        int Compare(int a, int b)
        {
            var first = items[a];
            var second = items[b];
            int result = 0;
            if (sortMode == "Power") result = second.Power.CompareTo(first.Power);
            else if (sortMode == "Rarity")
                result = Weight(second.Rarity).CompareTo(Weight(first.Rarity));
            else if (sortMode == "Name")
                result = string.Compare(first.Name ?? "", second.Name ?? "",
                    StringComparison.OrdinalIgnoreCase);
            return result != 0 ? result : a.CompareTo(b);
        }
        if (sortMode == "Power" || sortMode == "Rarity" || sortMode == "Name")
            indices.Sort(Compare);
        return indices;
    }

    public static int FirstDiscardableDrop(IReadOnlyList<Gear> drops)
    {
        if (drops == null) return -1;
        for (int i = 0; i < drops.Count; i++)
            if (drops[i] != null &&
                (drops[i].Rarity == "Common" || drops[i].Rarity == "Uncommon"))
                return i;
        return -1;
    }
}
