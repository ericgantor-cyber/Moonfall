using System;
using UnityEngine;

// Compact, deterministic, continuous six-region exploration layout.
// All region floors overlap by 2 units: no scene changes, portals, or wave spawns.
public static class MoonfallWorldRules
{
    public const int RegionCount = 6;
    public const int EnemiesPerRegion = 3;
    public const float RegionSpacing = 32f;
    public const float HalfRegion = 17f;
    public const int TotalEnemies = RegionCount * EnemiesPerRegion;
    public const int ChambersPerRegion = 5;
    public const int TotalChambers = RegionCount * ChambersPerRegion;
    public const float BossX = 160f;

    public static float RegionCenter(int region)
    {
        return Mathf.Clamp(region, 0, RegionCount - 1) * RegionSpacing;
    }

    public static int RegionAt(float positionX)
    {
        return Mathf.Clamp(Mathf.FloorToInt((positionX + HalfRegion) / RegionSpacing), 0, RegionCount - 1);
    }

    public static Vector3 EnemyPosition(int region, int slot)
    {
        float x = RegionCenter(region) + (slot - 1) * 8f;
        float z = (slot % 2 == 0 ? -1f : 1f) * (5f + region % 3);
        return new Vector3(x, 1f, z);
    }

    // Thirty side chambers are physically connected to the same world floor.
    public static Vector3 ChamberCenter(int region, int chamber)
    {
        return new Vector3(RegionCenter(region) + (chamber - 2) * 6f,
            0f, (chamber % 2 == 0 ? 24f : -24f));
    }

    public static Vector3 ChamberReward(int region, int chamber)
    {
        Vector3 center = ChamberCenter(region, chamber);
        return new Vector3(center.x, 1f, center.z);
    }

    public static float EnemyMaxHealth(int region)
    {
        return 35f + 14f * Mathf.Clamp(region, 0, RegionCount - 1);
    }
}
