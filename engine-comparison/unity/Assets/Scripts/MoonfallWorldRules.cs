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

    public static float EnemyMaxHealth(int region)
    {
        return 35f + 14f * Mathf.Clamp(region, 0, RegionCount - 1);
    }
}
